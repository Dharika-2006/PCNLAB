#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/propagation-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

// k = 0 Wi-Fi, 1 Bluetooth (approximated, see note), 2 Zigbee
const char *techName[3] = {"Wi-Fi", "Bluetooth*", "Zigbee"};
double txDbm[3] = {16.0206, 4.0, 0.0};
Ptr<MobilityModel> gwMob[3], mnMob[3];
Ptr<LogDistancePropagationLossModel> loss;
double rxSum[3] = {0, 0, 0};
uint32_t rxCnt = 0;

void SamplePower()   // received power at the gateway, sampled every second
{
  for (int k = 0; k < 3; k++)
    rxSum[k] += loss->CalcRxPower(txDbm[k], mnMob[k], gwMob[k]);
  rxCnt++;
  Simulator::Schedule(Seconds(1.0), &SamplePower);
}

NetDeviceContainer InstallWifi(WifiHelper wifi, YansWifiPhyHelper phy, NodeContainer n)   // n: gateway, mobile
{
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  phy.SetChannel(channel.Create());
  WifiMacHelper mac;
  Ssid ssid("campus");
  mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
  NetDeviceContainer d = wifi.Install(phy, mac, n.Get(0));
  mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
  d.Add(wifi.Install(phy, mac, n.Get(1)));
  return d;
}

void Cell(NetDeviceContainer d, const char *net, uint16_t port, std::string rate, uint32_t size)
{
  Ipv6AddressHelper ipv6;
  ipv6.SetBase(Ipv6Address(net), Ipv6Prefix(64));
  Ipv6InterfaceContainer i = ipv6.Assign(d);

  PacketSinkHelper sink("ns3::UdpSocketFactory", Inet6SocketAddress(Ipv6Address::GetAny(), port));
  sink.Install(d.Get(0)->GetNode()).Start(Seconds(0.0));

  OnOffHelper src("ns3::UdpSocketFactory", Inet6SocketAddress(i.GetAddress(0, 1), port));
  src.SetAttribute("DataRate", StringValue(rate));
  src.SetAttribute("PacketSize", UintegerValue(size));
  ApplicationContainer a = src.Install(d.Get(1)->GetNode());   // mobile -> gateway
  a.Start(Seconds(5.0));
  a.Stop(Seconds(55.0));
}

int main(int argc, char *argv[])
{
  double speed = 1.0;
  CommandLine cmd;
  cmd.AddValue("speed", "Mobile node speed (m/s)", speed);
  cmd.Parse(argc, argv);

  NodeContainer gw, mn;
  gw.Create(3);
  mn.Create(3);

  MobilityHelper fixed;
  fixed.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  fixed.Install(gw);

  MobilityHelper mobile;
  mobile.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                          "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)),
                          "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"));
  mobile.Install(mn);

  // Wi-Fi cell
  WifiHelper wifi;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");
  YansWifiPhyHelper phyW;
  NetDeviceContainer dW = InstallWifi(wifi, phyW, NodeContainer(gw.Get(0), mn.Get(0)));

  // Bluetooth cell (no Bluetooth module in ns-3): low-power 1 Mbps 802.11b link
  WifiHelper bt;
  bt.SetStandard(WIFI_STANDARD_80211b);
  bt.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                             "DataMode", StringValue("DsssRate1Mbps"),
                             "ControlMode", StringValue("DsssRate1Mbps"));
  YansWifiPhyHelper phyB;
  phyB.Set("TxPowerStart", DoubleValue(4.0));
  phyB.Set("TxPowerEnd", DoubleValue(4.0));
  NetDeviceContainer dB = InstallWifi(bt, phyB, NodeContainer(gw.Get(1), mn.Get(1)));

  // Zigbee cell: IEEE 802.15.4 + 6LoWPAN
  LrWpanHelper lr;
  NetDeviceContainer dZ = lr.Install(NodeContainer(gw.Get(2), mn.Get(2)));
  lr.CreateAssociatedPan(dZ, 10);

  InternetStackHelper internet;
  internet.SetIpv4StackInstall(false);
  internet.Install(gw);
  internet.Install(mn);

  SixLowPanHelper six;
  NetDeviceContainer dZ6 = six.Install(dZ);

  Cell(dW, "2001:1::", 9000, "2Mbps", 512);
  Cell(dB, "2001:2::", 9001, "300kbps", 512);
  Cell(dZ6, "2001:3::", 9002, "20kbps", 50);

  loss = CreateObject<LogDistancePropagationLossModel>();
  for (int k = 0; k < 3; k++)
  {
    gwMob[k] = gw.Get(k)->GetObject<MobilityModel>();
    mnMob[k] = mn.Get(k)->GetObject<MobilityModel>();
  }
  Simulator::Schedule(Seconds(5.0), &SamplePower);

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop(Seconds(56.0));
  Simulator::Run();

  Ptr<Ipv6FlowClassifier> cl = DynamicCast<Ipv6FlowClassifier>(flowmon.GetClassifier6());
  for (auto &f : monitor->GetFlowStats())
  {
    int k = cl->FindFlow(f.first).destinationPort - 9000;
    if (k < 0 || k > 2)
      continue;   // ignore ND / ICMPv6 flows
    FlowMonitor::FlowStats s = f.second;
    std::cout << "speed=" << speed << " m/s  " << techName[k]
              << "  avgRxPower(dBm)=" << rxSum[k] / rxCnt
              << "  throughput(kbps)=" << s.rxBytes * 8.0 / 50.0 / 1e3
              << "  loss(%)=" << 100.0 * (s.txPackets - s.rxPackets) / s.txPackets
              << "  delay(ms)=" << 1000 * s.delaySum.GetSeconds() / s.rxPackets << std::endl;
  }

  Simulator::Destroy();
  return 0;
}