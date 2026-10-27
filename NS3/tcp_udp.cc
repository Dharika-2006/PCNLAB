#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
using namespace ns3;

// EXTRA: Run() wrapper so one program covers every experiment
void Run(std::string label, std::string btlBw, int btlDelayMs,
         std::string udpRate, int qSize, bool trace)
{
    NodeContainer hosts, routers;
    hosts.Create(4);                             // n0, n1, n2, n3
    routers.Create(2);                           // r1, r2
    std::string q = std::to_string(qSize) + "p"; // EXTRA: variable queue size

    // Access links: 10 Mbps, 2 ms. Bottleneck: variable. DropTail queues.
    PointToPointHelper acc, btl;
    acc.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    acc.SetChannelAttribute("Delay", StringValue("2ms"));
    acc.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(q));
    btl.SetDeviceAttribute("DataRate", StringValue(btlBw));
    btl.SetChannelAttribute("Delay", StringValue(std::to_string(btlDelayMs) + "ms"));
    btl.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(q));

    NetDeviceContainer d0 = acc.Install(hosts.Get(0), routers.Get(0));
    NetDeviceContainer d1 = acc.Install(hosts.Get(1), routers.Get(0));
    NetDeviceContainer dB = btl.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer d2 = acc.Install(routers.Get(1), hosts.Get(2));
    NetDeviceContainer d3 = acc.Install(routers.Get(1), hosts.Get(3));

    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(routers);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    ipv4.Assign(d0);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    ipv4.Assign(d1);
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    ipv4.Assign(dB);
    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer if2 = ipv4.Assign(d2);
    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer if3 = ipv4.Assign(d3);

    // Queue-disc gotcha: remove default queue-disc so DropTail 50p really drops
    TrafficControlHelper tch;
    tch.Uninstall(d0);
    tch.Uninstall(d1);
    tch.Uninstall(dB);
    tch.Uninstall(d2);
    tch.Uninstall(d3);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // TCP flow n0 -> n3 (port 8080, BulkSend, 1024 B)
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory",
                             InetSocketAddress(Ipv4Address::GetAny(), 8080));
    ApplicationContainer tSink = tcpSink.Install(hosts.Get(3));
    tSink.Start(Seconds(0.0));
    tSink.Stop(Seconds(20.0));
    BulkSendHelper tcpSrc("ns3::TcpSocketFactory",
                          InetSocketAddress(if3.GetAddress(1), 8080));
    tcpSrc.SetAttribute("SendSize", UintegerValue(1024));
    tcpSrc.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tSrc = tcpSrc.Install(hosts.Get(0));
    tSrc.Start(Seconds(1.0));
    tSrc.Stop(Seconds(19.0)); // EXTRA: stop 1 s early (see notes)

    // UDP flow n1 -> n2 (port 9090, OnOff CBR, 512 B)
    PacketSinkHelper udpSink("ns3::UdpSocketFactory",
                             InetSocketAddress(Ipv4Address::GetAny(), 9090));
    ApplicationContainer uSink = udpSink.Install(hosts.Get(2));
    uSink.Start(Seconds(0.0));
    uSink.Stop(Seconds(20.0));
    OnOffHelper onoff("ns3::UdpSocketFactory",
                      InetSocketAddress(if2.GetAddress(1), 9090));
    onoff.SetConstantRate(DataRate(udpRate), 512);
    ApplicationContainer uSrc = onoff.Install(hosts.Get(1));
    uSrc.Start(Seconds(1.0));
    uSrc.Stop(Seconds(19.0));

    // Packet-level tracing (+ enqueue, - dequeue, r receive, d drop)
    if (trace)
    {
        AsciiTraceHelper ascii;
        acc.EnableAsciiAll(ascii.CreateFileStream("packet-trace.tr"));
    }

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    mon->CheckForLostPackets();

    // EXTRA: unique bytes the TCP sink received, used for the retransmission estimate
    uint64_t tcpUniqueBytes = DynamicCast<PacketSink>(tSink.Get(0))->GetTotalRx();

    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    std::cout << label << "\n";
    for (const auto &kv : mon->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto &s = kv.second;
        if (t.destinationPort != 8080 && t.destinationPort != 9090)
            continue; // skip ACK flow
        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double mbps = (dur > 0) ? s.rxBytes * 8.0 / dur / 1e6 : 0;
        double delay = (s.rxPackets > 0) ? s.delaySum.GetSeconds() / s.rxPackets * 1000.0 : 0;
        double loss = (s.txPackets > 0) ? 100.0 * (s.txPackets - s.rxPackets) / s.txPackets : 0;
        bool isTcp = (t.destinationPort == 8080);
        std::cout << "  " << (isTcp ? "TCP" : "UDP")
                  << " | Thr " << mbps << " Mbps | Delay " << delay
                  << " ms | Loss " << loss << " %";
        if (isTcp)
        {
            // EXTRA: retx = segments sent - unique segments delivered
            uint64_t uniq = (tcpUniqueBytes + 1023) / 1024;
            double retx = (s.txPackets > uniq) ? (double)(s.txPackets - uniq) : 0;
            std::cout << " | Retx " << retx << " pkts (" << 100.0 * retx / s.txPackets << " %)";
        }
        std::cout << "\n";
    }
    Simulator::Destroy();
}

int main()
{
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1024));

    // Exp 1: bottleneck bandwidth (first run also writes packet-trace.tr)
    Run("EXP1 btl 2Mbps", "2Mbps", 10, "1Mbps", 50, true);
    Run("EXP1 btl 5Mbps", "5Mbps", 10, "1Mbps", 50, false);
    Run("EXP1 btl 10Mbps", "10Mbps", 10, "1Mbps", 50, false);
    // Exp 2: UDP rate
    Run("EXP2 udp 500Kbps", "2Mbps", 10, "500Kbps", 50, false);
    Run("EXP2 udp 1Mbps", "2Mbps", 10, "1Mbps", 50, false);
    Run("EXP2 udp 2Mbps", "2Mbps", 10, "2Mbps", 50, false);
    Run("EXP2 udp 5Mbps", "2Mbps", 10, "5Mbps", 50, false);
    // Exp 3: queue size
    Run("EXP3 queue 20p", "2Mbps", 10, "1Mbps", 20, false);
    Run("EXP3 queue 50p", "2Mbps", 10, "1Mbps", 50, false);
    Run("EXP3 queue 100p", "2Mbps", 10, "1Mbps", 100, false);
    // Exp 4: bottleneck propagation delay
    Run("EXP4 delay 5ms", "2Mbps", 5, "1Mbps", 50, false);
    Run("EXP4 delay 10ms", "2Mbps", 10, "1Mbps", 50, false);
    Run("EXP4 delay 20ms", "2Mbps", 20, "1Mbps", 50, false);
    Run("EXP4 delay 50ms", "2Mbps", 50, "1Mbps", 50, false);
    return 0;
}