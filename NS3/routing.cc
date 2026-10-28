#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/aodv-module.h"   // EXTRA: AODV is not in your cheatsheet
using namespace ns3;

int spf = 0;  // EXTRA: counts OSPF (link-state) table recomputations
void Spf() { Ipv4GlobalRoutingHelper::RecomputeRoutingTables(); spf++; }

void Run(std::string proto, std::string cond) {
  // Nodes 0,1 = sources, 2,3 = destinations, 4..8 = routers r0..r4
  NodeContainer n; n.Create(9);
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  // Short path: r0-r1-r3. Long backup path: r0-r2-r4-r3
  int L[9][2] = {{0,4},{1,4},{4,5},{4,6},{5,7},{6,8},{8,7},{7,2},{7,3}};
  NetDeviceContainer d[9];
  for (int i = 0; i < 9; i++) d[i] = p2p.Install(n.Get(L[i][0]), n.Get(L[i][1]));

  // Routing protocol choice
  InternetStackHelper st;
  RipHelper rip; rip.Set("UnsolicitedRoutingUpdate", TimeValue(Seconds(5)));  // faster RIP updates
  AodvHelper aodv;
  if (proto == "RIP") st.SetRoutingHelper(rip);
  else if (proto == "AODV") st.SetRoutingHelper(aodv);
  st.Install(n);

  Ipv4AddressHelper ipv4;
  Ipv4InterfaceContainer ifc[9];
  for (int i = 0; i < 9; i++) {
    ipv4.SetBase(Ipv4Address(("10.1." + std::to_string(i + 1) + ".0").c_str()), "255.255.255.0");
    ifc[i] = ipv4.Assign(d[i]);
  }
  spf = 0;
  if (proto == "OSPF") { Ipv4GlobalRoutingHelper::PopulateRoutingTables(); spf = 1; }

  // Sinks on both destinations, port 9090
  PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9090));
  ApplicationContainer sa = sink.Install(n.Get(2)); sa.Add(sink.Install(n.Get(3)));
  sa.Start(Seconds(0.0)); sa.Stop(Seconds(60.0));

  // CBR flows: normal/failure = 1 flow at 1 Mbps; congestion = 4 flows at 2 Mbps
  int src[4] = {0, 1, 0, 1}, dst[4] = {2, 3, 3, 2};
  int nf = (cond == "Congestion") ? 4 : 1;
  for (int k = 0; k < nf; k++) {
    Ipv4Address a = ifc[dst[k] == 2 ? 7 : 8].GetAddress(1);
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(a, 9090));
    onoff.SetConstantRate(DataRate(nf == 4 ? "2Mbps" : "1Mbps"), 512);
    ApplicationContainer c = onoff.Install(n.Get(src[k]));
    c.Start(Seconds(20.0)); c.Stop(Seconds(59.0));
  }

  // Link failure: take down r0-r1 at 40 s (both ends)
  if (cond == "LinkFailure") {
    Ptr<Ipv4> ip0 = n.Get(4)->GetObject<Ipv4>();
    Ptr<Ipv4> ip1 = n.Get(5)->GetObject<Ipv4>();
    Simulator::Schedule(Seconds(40.0), &Ipv4::SetDown, ip0, 3);
    Simulator::Schedule(Seconds(40.0), &Ipv4::SetDown, ip1, 1);
    if (proto == "OSPF") Simulator::Schedule(Seconds(40.1), &Spf);
  }

  // Routing table snapshots before and after failure (section 7 of your sheet)
  Ptr<OutputStreamWrapper> rt = Create<OutputStreamWrapper>(proto + "-" + cond + "-routes.log", std::ios::out);
  Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(30.0), rt);
  Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(45.0), rt);

  FlowMonitorHelper fmh;
  Ptr<FlowMonitor> mon = fmh.InstallAll();
  Simulator::Stop(Seconds(60.0));
  Simulator::Run();
  mon->CheckForLostPackets();

  Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
  double tx = 0, rx = 0, dly = 0, ctrl = 0;
  for (const auto &kv : mon->GetFlowStats()) {
    auto t = cl->FindFlow(kv.first);
    if (t.destinationPort == 9090) { tx += kv.second.txPackets; rx += kv.second.rxPackets; dly += kv.second.delaySum.GetSeconds(); }
    else if (t.destinationPort == 520 || t.destinationPort == 654) ctrl += kv.second.txPackets; // EXTRA: RIP/AODV control msgs
  }
  if (proto == "OSPF") ctrl = spf;
  std::cout << "CSV," << proto << "," << cond << "," << (tx > 0 ? 100.0 * rx / tx : 0)
            << "," << (rx > 0 ? dly / rx * 1000.0 : 0) << "," << ctrl << "\n";
  Simulator::Destroy();
}

int main() {
  std::string P[3] = {"RIP", "OSPF", "AODV"};
  std::string C[3] = {"Normal", "Congestion", "LinkFailure"};
  for (auto &p : P) for (auto &c : C) Run(p, c);
  return 0;
}
