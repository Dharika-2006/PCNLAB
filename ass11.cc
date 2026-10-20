#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main()
{
    NodeContainer nodes;
    nodes.Create(2); // Wi-Fi station and access point

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"),
        "Bounds", RectangleValue(Rectangle(0, 50, 0, 50)));
    mobility.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ip.Assign(devices);

    uint16_t port = 9;

    PacketSinkHelper sink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer rx = sink.Install(nodes.Get(1));
    rx.Start(Seconds(1));
    rx.Stop(Seconds(10));

    OnOffHelper source("ns3::UdpSocketFactory",
        InetSocketAddress(interfaces.GetAddress(1), port));
    source.SetAttribute("DataRate", StringValue("1Mbps"));
    source.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer tx = source.Install(nodes.Get(0));
    tx.Start(Seconds(2));
    tx.Stop(Seconds(9));

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