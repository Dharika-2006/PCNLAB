
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main()
{
    NodeContainer nodes;
    nodes.Create(2); // 0 = AP, 1 = station

    // Keep both nodes stationary
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Wi-Fi channel and PHY
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Configure 802.11n
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue("HtMcs7"),
        "ControlMode", StringValue("HtMcs0"));

    // AP and station MACs
    WifiMacHelper mac;
    Ssid ssid("campus");

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes.Get(0));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    devices.Add(wifi.Install(phy, mac, nodes.Get(1)));

    // Install Internet stack and assign IPv4 addresses
    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ip.Assign(devices);

    // UDP server on AP, port 9000
    UdpServerHelper server(9000);
    ApplicationContainer serverApp = server.Install(nodes.Get(0));
    serverApp.Start(Seconds(0));
    serverApp.Stop(Seconds(10));

    // UDP client on station sends to AP
    UdpClientHelper client(interfaces.GetAddress(0), 9000);
    client.SetAttribute("MaxPackets", UintegerValue(1000));
    client.SetAttribute("PacketSize", UintegerValue(512));
    client.SetAttribute("Interval", TimeValue(Seconds(0.01)));

    ApplicationContainer clientApp = client.Install(nodes.Get(1));
    clientApp.Start(Seconds(1));
    clientApp.Stop(Seconds(9));

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    Ptr<UdpServer> s = DynamicCast<UdpServer>(serverApp.Get(0));
    std::cout << "Wi-Fi UDP packets received: "
              << s->GetReceived() << std::endl;

    Simulator::Destroy();
    return 0;
}
