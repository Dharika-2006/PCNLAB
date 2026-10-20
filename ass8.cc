#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
using namespace ns3;

void Run(uint32_t bw)
{
    NodeContainer n;
    n.Create(6); // 0-3 end nodes, 4-5 routers

    PointToPointHelper p, b;
    p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p.SetChannelAttribute("Delay", StringValue("2ms"));
    b.SetDeviceAttribute("DataRate", StringValue(std::to_string(bw) + "Mbps"));
    b.SetChannelAttribute("Delay", StringValue("10ms"));
    b.SetQueue("ns3::DropTailQueue<Packet>",
               "MaxSize", QueueSizeValue(QueueSize("50p")));

    NetDeviceContainer d[5] = {
        p.Install(n.Get(0), n.Get(4)),
        p.Install(n.Get(1), n.Get(4)),
        b.Install(n.Get(4), n.Get(5)),
        p.Install(n.Get(5), n.Get(2)),
        p.Install(n.Get(5), n.Get(3))
    };

    InternetStackHelper stack;
    stack.Install(n);

    Ipv4AddressHelper ip;
    Ipv4InterfaceContainer i[5];
    for (int k = 0; k < 5; k++) {
        ip.SetBase(("10.1." + std::to_string(k + 1) + ".0").c_str(),
                   "255.255.255.0");
        i[k] = ip.Assign(d[k]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    PacketSinkHelper tcpSink("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5000));
    PacketSinkHelper udpSink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5001));

    auto tcpRx = tcpSink.Install(n.Get(3));
    auto udpRx = udpSink.Install(n.Get(2));
    tcpRx.Start(Seconds(0)); udpRx.Start(Seconds(0));
    tcpRx.Stop(Seconds(20)); udpRx.Stop(Seconds(20));

    BulkSendHelper tcp("ns3::TcpSocketFactory",
        InetSocketAddress(i[4].GetAddress(1), 5000));
    tcp.SetAttribute("SendSize", UintegerValue(1024));
    auto tcpTx = tcp.Install(n.Get(0));
    tcpTx.Start(Seconds(1)); tcpTx.Stop(Seconds(19));

    OnOffHelper udp("ns3::UdpSocketFactory",
        InetSocketAddress(i[3].GetAddress(1), 5001));
    udp.SetAttribute("DataRate", StringValue("1Mbps"));
    udp.SetAttribute("PacketSize", UintegerValue(512));
    udp.SetAttribute("OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udp.SetAttribute("OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    auto udpTx = udp.Install(n.Get(1));
    udpTx.Start(Seconds(1)); udpTx.Stop(Seconds(19));

    FlowMonitorHelper fm;
    auto monitor = fm.InstallAll();

    Simulator::Stop(Seconds(20));
    Simulator::Run();
    monitor->CheckForLostPackets();

    auto classifier = DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());
    for (auto f : monitor->GetFlowStats()) {
        auto t = classifier->FindFlow(f.first);
        if (t.destinationPort != 5000 && t.destinationPort != 5001)
            continue;

        auto s = f.second;
        std::cout << (t.destinationPort == 5000 ? "TCP" : "UDP")
                  << " | Rx bytes: " << s.rxBytes
                  << " | Throughput: " << s.rxBytes * 8.0 / 20 / 1e6
                  << " Mbps | Lost: " << s.lostPackets
                  << " | Avg delay: "
                  << (s.rxPackets ? s.delaySum.GetSeconds() / s.rxPackets * 1000 : 0)
                  << " ms\n";
    }

    Simulator::Destroy();
}

int main()
{
    Run(2);
    Run(5);
    Run(10);
}