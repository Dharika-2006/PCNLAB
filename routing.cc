#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/aodv-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

uint32_t ctrl = 0;   // routing control messages received (RIP: UDP 520, AODV: UDP 654)

void CtrlCb(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iif)
{
  if (h.GetProtocol() != 17)
    return;
  UdpHeader u;
  p->PeekHeader(u);
  if (u.GetDestinationPort() == 520 || u.GetDestinationPort() == 654)
    ctrl++;
}

void LinkDown(Ipv4InterfaceContainer i)   // disable both ends of a link
{
  for (uint32_t k = 0; k < 2; k++)
    i.Get(k).first->SetDown(i.Get(k).second);
}

int main(int argc, char *argv[])
{
  std::string proto = "rip";      // rip | ospf | aodv
  std::string cond = "normal";    // normal | congestion | failure
  CommandLine cmd;
  cmd.AddValue("proto", "rip | ospf | aodv", proto);
  cmd.AddValue("cond", "normal | congestion | failure", cond);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue(true));

  // 0=S0 1=S1 2=D0 3=D1 4=R1 5=R2 6=R3 7=R4 8=R5
  NodeContainer n;
  n.Create(9);
  int L[9][2] = {{0, 4}, {1, 4}, {4, 5}, {5, 7}, {4, 6}, {6, 8}, {8, 7}, {7, 2}, {7, 3}};
  // primary path R1-R2-R4, longer backup R1-R3-R5-R4

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));
  NetDeviceContainer d[9];
  for (int k = 0; k < 9; k++)
    d[k] = p2p.Install(n.Get(L[k][0]), n.Get(L[k][1]));

  InternetStackHelper internet;
  AodvHelper aodv;
  RipHelper rip;
  if (proto == "aodv")
    internet.SetRoutingHelper(aodv);
  else if (proto == "rip")
    internet.SetRoutingHelper(rip);
  internet.Install(n);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifc[9];
  for (int k = 0; k < 9; k++)
  {
    ifc[k] = address.Assign(d[k]);
    address.NewNetwork();
  }
  if (proto == "ospf")   // link-state (OSPF-like) global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // CBR/UDP traffic: S0->D0 and S1->D1; extra flows only under congestion
  PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
  ApplicationContainer sinks = sink.Install(n.Get(2));
  sinks.Add(sink.Install(n.Get(3)));
  sinks.Start(Seconds(0.0));

  uint32_t flows = (cond == "congestion") ? 6 : 2;
  for (uint32_t k = 0; k < flows; k++)
  {
    OnOffHelper src("ns3::UdpSocketFactory", InetSocketAddress(ifc[7 + k % 2].GetAddress(1), 9));
    src.SetAttribute("DataRate", StringValue(k < 2 ? "1Mbps" : "1.5Mbps"));
    src.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer a = src.Install(n.Get(k % 2));
    a.Start(Seconds(10.0));
    a.Stop(Seconds(30.0));
  }

  if (cond == "failure")
    Simulator::Schedule(Seconds(20.0), &LinkDown, ifc[2]);   // R1-R2 fails at t=20 s

  Config::ConnectWithoutContext("/NodeList/*/$ns3::Ipv4L3Protocol/LocalDeliver", MakeCallback(&CtrlCb));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop(Seconds(31.0));
  Simulator::Run();

  Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  double tx = 0, rx = 0, dly = 0;
  for (auto &f : monitor->GetFlowStats())
  {
    if (cl->FindFlow(f.first).destinationPort != 9)
      continue;   // ignore routing-protocol traffic
    tx += f.second.txPackets;
    rx += f.second.rxPackets;
    dly += f.second.delaySum.GetSeconds();
  }
  // CSV: protocol, condition, PDR(%), delay(ms), routing control messages
  std::cout << proto << "," << cond << "," << 100.0 * rx / tx << "," << 1000 * dly / rx << "," << ctrl << std::endl;

  Simulator::Destroy();
  return 0;
}