#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <iostream>

using namespace ns3;

int main()
{
    NodeContainer n;
    n.Create(4);

    PointToPointHelper p;
    p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer d01 = p.Install(n.Get(0), n.Get(1));
    NetDeviceContainer d12 = p.Install(n.Get(1), n.Get(2));
    NetDeviceContainer d32 = p.Install(n.Get(3), n.Get(2));

    InternetStackHelper stack;
    stack.Install(n);

    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer a01 = ip.Assign(d01);
    ip.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer a12 = ip.Assign(d12);
    ip.SetBase("10.1.3.0", "255.255.255.0");
    ip.Assign(d32);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // UDP sinks
    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), 9000));
    ApplicationContainer s1 = sink.Install(n.Get(2));
    sink.SetAttribute("Local",
                      AddressValue(InetSocketAddress(Ipv4Address::GetAny(), 9001)));
    ApplicationContainer s2 = sink.Install(n.Get(0));
    s1.Start(Seconds(0)); s2.Start(Seconds(0));
    s1.Stop(Seconds(10)); s2.Stop(Seconds(10));

    // UDP sources: n0 -> n2 at 1 Mbps; n3 -> n0 at 500 Kbps
    OnOffHelper src1("ns3::UdpSocketFactory",
                     InetSocketAddress(a12.GetAddress(1), 9000));
    src1.SetAttribute("DataRate", StringValue("1Mbps"));
    src1.SetAttribute("PacketSize", UintegerValue(1024));
    src1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    src1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    OnOffHelper src2("ns3::UdpSocketFactory",
                     InetSocketAddress(a01.GetAddress(0), 9001));
    src2.SetAttribute("DataRate", StringValue("500Kbps"));
    src2.SetAttribute("PacketSize", UintegerValue(1024));
    src2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    src2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer c1 = src1.Install(n.Get(0));
    ApplicationContainer c2 = src2.Install(n.Get(3));
    c1.Start(Seconds(1)); c2.Start(Seconds(1));
    c1.Stop(Seconds(9));  c2.Stop(Seconds(9));

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    double duration = 8.0; // source active from 1s to 9s
    uint64_t b1 = DynamicCast<PacketSink>(s1.Get(0))->GetTotalRx();
    uint64_t b2 = DynamicCast<PacketSink>(s2.Get(0))->GetTotalRx();

    std::cout << "Flow n0 -> n2: " << b1 << " bytes, "
              << b1 * 8.0 / duration / 1e6 << " Mbps\n";
    std::cout << "Flow n3 -> n0: " << b2 << " bytes, "
              << b2 * 8.0 / duration / 1e6 << " Mbps\n";

    Simulator::Destroy();
    return 0;
}