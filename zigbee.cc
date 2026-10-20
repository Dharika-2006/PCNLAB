
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main()
{
    NodeContainer nodes;
    nodes.Create(2); // 0 = gateway, 1 = sensor/mobile node

    // Give nodes positions for wireless propagation
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Install IEEE 802.15.4 LR-WPAN devices
    LrWpanHelper lrWpan;
    NetDeviceContainer lrDevices = lrWpan.Install(nodes);

    // Associate devices with a PAN
    lrWpan.AssociateToPan(lrDevices, 0);

    // Install IPv6 stack only
    InternetStackHelper internet;
    internet.SetIpv4StackInstall(false);
    internet.Install(nodes);

    // Add 6LoWPAN adaptation over LR-WPAN
    SixLowPanHelper sixLowPan;
    NetDeviceContainer devices = sixLowPan.Install(lrDevices);

    // Assign IPv6 addresses
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer interfaces = ipv6.Assign(devices);

    // UDP receiver on gateway, port 9000
    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        Inet6SocketAddress(Ipv6Address::GetAny(), 9000));

    ApplicationContainer sinkApp = sink.Install(nodes.Get(0));
    sinkApp.Start(Seconds(0));
    sinkApp.Stop(Seconds(10));

    // UDP sender on sensor node sends to gateway
    OnOffHelper source(
        "ns3::UdpSocketFactory",
        Inet6SocketAddress(interfaces.GetAddress(0, 1), 9000));

    source.SetAttribute("DataRate", StringValue("20kbps"));
    source.SetAttribute("PacketSize", UintegerValue(50));

    ApplicationContainer sourceApp = source.Install(nodes.Get(1));
    sourceApp.Start(Seconds(1));
    sourceApp.Stop(Seconds(9));

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    Ptr<PacketSink> receiver = DynamicCast<PacketSink>(sinkApp.Get(0));
    std::cout << "Zigbee-style UDP bytes received: "
              << receiver->GetTotalRx() << std::endl;

    Simulator::Destroy();
    return 0;
}
