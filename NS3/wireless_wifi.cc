#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

// ---- Rx signal strength (dBm) collected at the AP (Sec. 10 callback) ----
static double rxPowerSum = 0;
static uint32_t rxPowerCnt = 0;

static void SniffRxSignal(Ptr<const Packet> p, uint16_t channelFreq,
                          WifiTxVector txVector, MpduInfo aMpdu,
                          SignalNoiseDbm sn, uint16_t staId)
{
    rxPowerSum += sn.signal;
    rxPowerCnt++;
}

static void RunSim(double speed)
{
    rxPowerSum = 0;
    rxPowerCnt = 0;

    // 1. Nodes: AP = node 0 (gateway), 3 mobile stations
    NodeContainer apNode, staNodes;
    apNode.Create(1);
    staNodes.Create(3);

    // 2. Wi-Fi (802.11n) infrastructure mode
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("TxPowerStart", DoubleValue(16.0));
    wifiPhy.Set("TxPowerEnd", DoubleValue(16.0));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("HtMcs7"),
                                 "ControlMode", StringValue("HtMcs0"));

    WifiMacHelper wifiMac;
    Ssid ssid = Ssid("campus-wifi");
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, apNode);
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer staDevices = wifi.Install(wifiPhy, wifiMac, staNodes);

    // 3. Mobility: AP static, stations RandomWalk2d at given speed
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(5.0), "MinY", DoubleValue(5.0),
                                  "DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(5.0),
                                  "GridWidth", UintegerValue(3), "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"));
    mobility.Install(staNodes);

    // 4. Internet stack + IP addressing
    InternetStackHelper internet;
    internet.Install(apNode);
    internet.Install(staNodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apIf = ipv4.Assign(apDevice);
    Ipv4InterfaceContainer staIf = ipv4.Assign(staDevices);

    // 5. Applications: UDP server on AP (port 5000), UDP client on each STA (1 Mbps)
    UdpServerHelper server(5000);
    ApplicationContainer sApp = server.Install(apNode.Get(0));
    sApp.Start(Seconds(0.0));
    sApp.Stop(Seconds(20.0));

    UdpClientHelper client(apIf.GetAddress(0), 5000);
    client.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    client.SetAttribute("PacketSize", UintegerValue(512));
    client.SetAttribute("Interval", TimeValue(Seconds(512 * 8.0 / 1e6)));
    ApplicationContainer cApp = client.Install(staNodes);
    cApp.Start(Seconds(2.0));
    cApp.Stop(Seconds(20.0));

    // 6. Rx power callback (only on the AP = NodeList/0)
    Config::ConnectWithoutContext(
        "/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
        MakeCallback(&SniffRxSignal));

    // 7. FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = mon->GetFlowStats();

    std::cout << "=== WIFI | Speed = " << speed << " m/s ===\n";
    std::cout << "Avg Rx Power (AP): "
              << (rxPowerCnt ? rxPowerSum / rxPowerCnt : 0) << " dBm\n";
    for (const auto &kv : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto &s = kv.second;
        if (t.destinationPort != 5000)
            continue;
        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double mbps = (dur > 0) ? (s.rxBytes * 8.0 / dur / 1e6) : 0;
        double delay = (s.rxPackets > 0) ? (s.delaySum.GetSeconds() / s.rxPackets * 1000.0) : 0;
        double loss = (s.txPackets > 0) ? (100.0 * (s.txPackets - s.rxPackets) / s.txPackets) : 0;
        double pdr = (s.txPackets > 0) ? (100.0 * s.rxPackets / s.txPackets) : 0;
        std::cout << "Flow " << kv.first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << " Port " << t.destinationPort << "):\n"
                  << "  Throughput: " << mbps << " Mbps\n"
                  << "  Avg Delay: " << delay << " ms\n"
                  << "  Loss: " << loss << " %\n"
                  << "  PDR: " << pdr << " %\n";
    }
    Simulator::Destroy();
}

int main()
{
    // Speed sweep inside the code: 1, 5, 10 m/s (no command-line input needed)
    double speeds[] = {1.0, 5.0, 10.0};
    for (double v : speeds)
    {
        RunSim(v);
    }
    return 0;
}