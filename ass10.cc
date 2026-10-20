#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/aodv-module.h"

using namespace ns3;

int main()
{
    // Source, two routers, destination, alternate router
    NodeContainer n;
    n.Create(5);

    PointToPointHelper p;
    p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p.SetChannelAttribute("Delay", StringValue("2ms"));

    // Main path: source -> router1 -> destination
    NetDeviceContainer d01 = p.Install(n.Get(0), n.Get(1));
    NetDeviceContainer d13 = p.Install(n.Get(1), n.Get(3));

    // Alternate path: source -> router2 -> destination
    NetDeviceContainer d02 = p.Install(n.Get(0), n.Get(2));
    NetDeviceContainer d23 = p.Install(n.Get(2), n.Get(3));

    AodvHelper aodv;
    InternetStackHelper stack;
    stack.SetRoutingHelper(aodv);
    stack.Install(n);

    Ipv4AddressHelper ip;

    ip.SetBase("10.1.1.0", "255.255.255.0");
    ip.Assign(d01);

    ip.SetBase("10.1.2.0", "255.255.255.0");
    ip.Assign(d13);

    ip.SetBase("10.1.3.0", "255.255.255.0");
    ip.Assign(d02);

    ip.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i23 = ip.Assign(d23);

    // UDP receiver at destination
    uint16_t port = 9;

    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer rx = sink.Install(n.Get(3));
    rx.Start(Seconds(1));
    rx.Stop(Seconds(10));

    // UDP sender at source
    OnOffHelper source("ns3::UdpSocketFactory",
        InetSocketAddress(i23.GetAddress(1), port));

    source.SetAttribute("DataRate", StringValue("1Mbps"));
    source.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer tx = source.Install(n.Get(0));
    tx.Start(Seconds(2));
    tx.Stop(Seconds(9));

    // Disable router1's interface on the router1-destination link at 5s
    Ptr<Ipv4> router1Ipv4 = n.Get(1)->GetObject<Ipv4>();
    int32_t interfaceIndex =
        router1Ipv4->GetInterfaceForDevice(d13.Get(0));

    Simulator::Schedule(Seconds(5),
        &Ipv4::SetDown, router1Ipv4, interfaceIndex);

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    uint64_t bytes =
        DynamicCast<PacketSink>(rx.Get(0))->GetTotalRx();

    std::cout << "Received bytes: " << bytes
              << "\nThroughput: "
              << bytes * 8.0 / 10.0 / 1e6
              << " Mbps\n";

    Simulator::Destroy();
    return 0;
}