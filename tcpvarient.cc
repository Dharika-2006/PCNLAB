#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>

using namespace ns3;

int main()
{
    // Create nodes: 0,1 senders; 2,3 receivers; 4,5 routers
    NodeContainer nodes;
    nodes.Create(6);

    // Configure links
    PointToPointHelper access, bottleneck;

    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));

    bottleneck.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));

    // Connect nodes
    NetDeviceContainer d0 = access.Install(nodes.Get(0), nodes.Get(4));
    NetDeviceContainer d1 = access.Install(nodes.Get(1), nodes.Get(4));
    NetDeviceContainer db = bottleneck.Install(nodes.Get(4), nodes.Get(5));
    NetDeviceContainer d2 = access.Install(nodes.Get(5), nodes.Get(2));
    NetDeviceContainer d3 = access.Install(nodes.Get(5), nodes.Get(3));

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses to each link
    Ipv4AddressHelper ip;

    ip.SetBase("10.1.1.0", "255.255.255.0");
    ip.Assign(d0);

    ip.SetBase("10.1.2.0", "255.255.255.0");
    ip.Assign(d1);

    ip.SetBase("10.1.3.0", "255.255.255.0");
    ip.Assign(db);

    ip.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i2 = ip.Assign(d2);

    ip.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i3 = ip.Assign(d3);

    // Populate routes
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // TCP receiver on node 3, port 5000
    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5000));

    ApplicationContainer tcpServer = tcpSink.Install(nodes.Get(3));
    tcpServer.Start(Seconds(0));
    tcpServer.Stop(Seconds(20));

    // UDP receiver on node 2, port 6000
    PacketSinkHelper udpSink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 6000));

    ApplicationContainer udpServer = udpSink.Install(nodes.Get(2));
    udpServer.Start(Seconds(0));
    udpServer.Stop(Seconds(20));

    // TCP sender: node 0 -> node 3
    BulkSendHelper tcp(
        "ns3::TcpSocketFactory",
        InetSocketAddress(i3.GetAddress(1), 5000));

    tcp.SetAttribute("SendSize", UintegerValue(1024));

    ApplicationContainer tcpClient = tcp.Install(nodes.Get(0));
    tcpClient.Start(Seconds(1));
    tcpClient.Stop(Seconds(20));

    // UDP sender: node 1 -> node 2
    OnOffHelper udp(
        "ns3::UdpSocketFactory",
        InetSocketAddress(i2.GetAddress(1), 6000));

    udp.SetAttribute("DataRate", StringValue("1Mbps"));
    udp.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer udpClient = udp.Install(nodes.Get(1));
    udpClient.Start(Seconds(1));
    udpClient.Stop(Seconds(20));

    // Install FlowMonitor
    FlowMonitorHelper fm;
    Ptr<FlowMonitor> monitor = fm.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(20));
    Simulator::Run();

    monitor->CheckForLostPackets();

    // Get flow classifier
    Ptr<Ipv4FlowClassifier> cl =
        DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());

    // Print metrics for TCP and UDP application flows
    for (const auto &kv : monitor->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);

        if (t.destinationPort != 5000 && t.destinationPort != 6000)
            continue;

        const auto &s = kv.second;

        double loss = s.txPackets > 0
            ? 100.0 * (s.txPackets - s.rxPackets) / s.txPackets
            : 0;

        double delay = s.rxPackets > 0
            ? 1000.0 * s.delaySum.GetSeconds() / s.rxPackets
            : 0;

        double duration =
            (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();

        double throughput = duration > 0
            ? s.rxBytes * 8.0 / duration / 1e6
            : 0;

        double pdr = s.txPackets > 0
            ? 100.0 * s.rxPackets / s.txPackets
            : 0;

        std::cout << (t.destinationPort == 5000 ? "TCP" : "UDP")
                  << " Loss=" << loss << "%"
                  << " Delay=" << delay << " ms"
                  << " Throughput=" << throughput << " Mbps"
                  << " PDR=" << pdr << "%\n";
    }
    Simulator::Destroy();
    return 0;
}
