#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <map>
using namespace ns3;

// per-flow bookkeeping for latency (no FlowMonitor): match packets by UID
std::map<uint64_t, Time> sent[2];
double delaySum[2] = {0, 0};
uint32_t rxPkts[2] = {0, 0};

void TxCb(uint32_t f, Ptr<const Packet> p) { sent[f][p->GetUid()] = Simulator::Now(); }
void RxCb(uint32_t f, Ptr<const Packet> p, const Address &a)
{
  delaySum[f] += (Simulator::Now() - sent[f][p->GetUid()]).GetSeconds();
  rxPkts[f]++;
}

int main(int argc, char *argv[])
{
  NodeContainer nodes;
  nodes.Create(4);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));
  NetDeviceContainer d01 = p2p.Install(nodes.Get(0), nodes.Get(1));
  NetDeviceContainer d12 = p2p.Install(nodes.Get(1), nodes.Get(2));
  NetDeviceContainer d32 = p2p.Install(nodes.Get(3), nodes.Get(2));

  InternetStackHelper internet;
  internet.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i01 = address.Assign(d01);
  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i12 = address.Assign(d12);
  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i32 = address.Assign(d32);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Flow 1: n0 -> n2, 1 Mbps
  PacketSinkHelper sink1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
  ApplicationContainer sinkApp1 = sink1.Install(nodes.Get(2));
  sinkApp1.Start(Seconds(1.0));
  sinkApp1.Stop(Seconds(10.0));
  OnOffHelper source1("ns3::UdpSocketFactory", InetSocketAddress(i12.GetAddress(1), 5000));
  source1.SetAttribute("DataRate", StringValue("1Mbps"));
  source1.SetAttribute("PacketSize", UintegerValue(512));
  ApplicationContainer sourceApp1 = source1.Install(nodes.Get(0));
  sourceApp1.Start(Seconds(2.0));
  sourceApp1.Stop(Seconds(10.0));

  // Flow 2: n3 -> n0, 500 Kbps
  PacketSinkHelper sink2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 6000));
  ApplicationContainer sinkApp2 = sink2.Install(nodes.Get(0));
  sinkApp2.Start(Seconds(1.0));
  sinkApp2.Stop(Seconds(10.0));
  OnOffHelper source2("ns3::UdpSocketFactory", InetSocketAddress(i01.GetAddress(0), 6000));
  source2.SetAttribute("DataRate", StringValue("500Kbps"));
  source2.SetAttribute("PacketSize", UintegerValue(512));
  ApplicationContainer sourceApp2 = source2.Install(nodes.Get(3));
  sourceApp2.Start(Seconds(3.0));
  sourceApp2.Stop(Seconds(10.0));

  sourceApp1.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&TxCb, 0));
  sinkApp1.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&RxCb, 0));
  sourceApp2.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&TxCb, 1));
  sinkApp2.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&RxCb, 1));

  Simulator::Stop(Seconds(10.0));
  Simulator::Run();

  double bytes1 = DynamicCast<PacketSink>(sinkApp1.Get(0))->GetTotalRx();
  double bytes2 = DynamicCast<PacketSink>(sinkApp2.Get(0))->GetTotalRx();

  std::cout << "Total bytes received : " << bytes1 + bytes2 << std::endl;
  std::cout << "Flow 1 (n0->n2): " << bytes1 << " bytes, throughput "
            << bytes1 * 8 / 8.0 / 1e6 << " Mbps, avg latency "
            << 1000 * delaySum[0] / rxPkts[0] << " ms" << std::endl;   // active 2s-10s
  std::cout << "Flow 2 (n3->n0): " << bytes2 << " bytes, throughput "
            << bytes2 * 8 / 7.0 / 1e6 << " Mbps, avg latency "
            << 1000 * delaySum[1] / rxPkts[1] << " ms" << std::endl;   // active 3s-10s

  Simulator::Destroy();
  return 0;
}