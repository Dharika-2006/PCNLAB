#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
using namespace ns3;

// EXTRA: cwnd tracking (cheatsheet has no cwnd trace)
double cwSum;
uint64_t cwCnt;
uint32_t cwMax;
void CwndCb(uint32_t oldV, uint32_t newV)
{
    cwSum += newV;
    cwCnt++;
    if (newV > cwMax)
        cwMax = newV;
}
void Hook()
{ // hooks vehicle sockets only (nodes 0-4)
    for (int i = 0; i < 5; i++)
        Config::ConnectWithoutContext("/NodeList/" + std::to_string(i) +
                                          "/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",
                                      MakeCallback(&CwndCb));
}

// EXTRA: Run() wrapper so one program compares all three variants
void Run(std::string tcp)
{
    cwSum = 0;
    cwCnt = 0;
    cwMax = 0;
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcp));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1024));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(tcp == "ns3::TcpBbr"));

    NodeContainer v, r, s;
    v.Create(5); // vehicles = nodes 0-4
    r.Create(2); // r0 = Roadside Router, r1 = Internet router
    s.Create(1); // traffic control server

    PointToPointHelper acc, btl;
    acc.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    acc.SetChannelAttribute("Delay", StringValue("2ms"));
    acc.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("50p"));
    btl.SetDeviceAttribute("DataRate", StringValue("5Mbps")); // bottleneck
    btl.SetChannelAttribute("Delay", StringValue("10ms"));
    btl.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("50p"));

    InternetStackHelper internet;
    internet.Install(v);
    internet.Install(r);
    internet.Install(s);

    Ipv4AddressHelper ipv4;
    TrafficControlHelper tch;
    for (int i = 0; i < 5; i++)
    {
        NetDeviceContainer d = acc.Install(v.Get(i), r.Get(0));
        ipv4.SetBase(Ipv4Address(("10.1." + std::to_string(i + 1) + ".0").c_str()), "255.255.255.0");
        ipv4.Assign(d);
        tch.Uninstall(d);
    }
    NetDeviceContainer dB = btl.Install(r.Get(0), r.Get(1));
    ipv4.SetBase("10.1.6.0", "255.255.255.0");
    ipv4.Assign(dB);
    tch.Uninstall(dB);
    NetDeviceContainer dS = acc.Install(r.Get(1), s.Get(0));
    ipv4.SetBase("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer ifS = ipv4.Assign(dS);
    tch.Uninstall(dS);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Server sink (port 8080) and 5 BulkSend vehicles
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 8080));
    ApplicationContainer sApp = sink.Install(s.Get(0));
    sApp.Start(Seconds(0.0));
    sApp.Stop(Seconds(20.0));
    for (int i = 0; i < 5; i++)
    {
        BulkSendHelper src("ns3::TcpSocketFactory", InetSocketAddress(ifS.GetAddress(1), 8080));
        src.SetAttribute("SendSize", UintegerValue(1024));
        src.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer a = src.Install(v.Get(i));
        a.Start(Seconds(1.0));
        a.Stop(Seconds(19.0)); // EXTRA: stop 1 s early to avoid fake loss
    }
    Simulator::Schedule(Seconds(1.1), &Hook); // EXTRA: sockets exist by 1.1 s

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    double totThr = 0, sumX = 0, sumX2 = 0, dSum = 0;
    uint64_t tx = 0, rx = 0, rxAll = 0;
    for (const auto &kv : mon->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto &f = kv.second;
        if (t.destinationPort != 8080)
            continue; // skip reverse ACK flows
        double dur = (f.timeLastRxPacket - f.timeFirstTxPacket).GetSeconds();
        double mbps = (dur > 0) ? f.rxBytes * 8.0 / dur / 1e6 : 0;
        totThr += mbps;
        sumX += mbps;
        sumX2 += mbps * mbps;
        dSum += f.delaySum.GetSeconds();
        rx += f.rxPackets;
        tx += f.txPackets;
    }
    double jain = sumX * sumX / (5 * sumX2);
    std::cout << tcp << "\n"
              << "  Total throughput: " << totThr << " Mbps\n"
              << "  Avg delay: " << dSum / rx * 1000.0 << " ms\n"
              << "  Loss: " << 100.0 * (tx - rx) / tx << " %\n"
              << "  Jain fairness: " << jain << "\n"
              << "  Avg cwnd: " << cwSum / cwCnt / 1024.0 << " segs, Max cwnd: " << cwMax / 1024.0 << " segs\n";
    Simulator::Destroy();
}

int main()
{
    Run("ns3::TcpNewReno");
    Run("ns3::TcpCubic");
    Run("ns3::TcpBbr");
    return 0;
}