#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

int main(int argc, char *argv[])
{
  std::string bw = "2Mbps", delay = "10ms", udpRate = "1Mbps", queue = "50p";

  CommandLine cmd;
  cmd.AddValue("bw", "Bottleneck bandwidth", bw);
  cmd.AddValue("delay", "Bottleneck propagation delay", delay);
  cmd.AddValue("udpRate", "UDP data rate", udpRate);
  cmd.AddValue("queue", "DropTail queue size, e.g. 50p", queue);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1024));

  NodeContainer nodes;
  nodes.Create(6);   // 0=n0 1=n1 2=n2 3=n3 4=r1 5=r2

  PointToPointHelper access, bott;
  access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  access.SetChannelAttribute("Delay", StringValue("2ms"));
  bott.SetDeviceAttribute("DataRate", StringValue(bw));
  bott.SetChannelAttribute("Delay", StringValue(delay));
  access.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(queue));
  bott.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(queue));

  NetDeviceContainer d0 = access.Install(nodes.Get(0), nodes.Get(4));
  NetDeviceContainer d1 = access.Install(nodes.Get(1), nodes.Get(4));
  NetDeviceContainer db = bott.Install(nodes.Get(4), nodes.Get(5));
  NetDeviceContainer d2 = access.Install(nodes.Get(5), nodes.Get(2));
  NetDeviceContainer d3 = access.Install(nodes.Get(5), nodes.Get(3));

  InternetStackHelper internet;
  internet.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  address.Assign(d0);
  address.SetBase("10.1.2.0", "255.255.255.0");
  address.Assign(d1);
  address.SetBase("10.1.3.0", "255.255.255.0");
  address.Assign(db);
  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = address.Assign(d2);
  address.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i3 = address.Assign(d3);

  // remove default 1000-packet queue disc so the 50-packet DropTail is the real queue
  TrafficControlHelper tch;
  tch.Uninstall(db);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  uint16_t tcpPort = 5000, udpPort = 6000;

  PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
  PacketSinkHelper udpSink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), udpPort));
  ApplicationContainer sinks = tcpSink.Install(nodes.Get(3));
  sinks.Add(udpSink.Install(nodes.Get(2)));
  sinks.Start(Seconds(0.0));
  sinks.Stop(Seconds(20.0));

  BulkSendHelper tcp("ns3::TcpSocketFactory", InetSocketAddress(i3.GetAddress(1), tcpPort));
  tcp.SetAttribute("SendSize", UintegerValue(1024));
  ApplicationContainer apps = tcp.Install(nodes.Get(0));

  OnOffHelper udp("ns3::UdpSocketFactory", InetSocketAddress(i2.GetAddress(1), udpPort));
  udp.SetAttribute("DataRate", StringValue(udpRate));
  udp.SetAttribute("PacketSize", UintegerValue(512));
  apps.Add(udp.Install(nodes.Get(1)));
  apps.Start(Seconds(1.0));
  apps.Stop(Seconds(20.0));

  access.EnablePcapAll("tcp-udp");

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop(Seconds(20.0));
  Simulator::Run();

  Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  for (auto &f : monitor->GetFlowStats())
  {
    Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(f.first);
    if (t.destinationPort != tcpPort && t.destinationPort != udpPort)
      continue;   // skip TCP ACK flow
    FlowMonitor::FlowStats s = f.second;
    bool isTcp = (t.destinationPort == tcpPort);
    double lossPkts = s.txPackets - s.rxPackets;
    std::cout << (isTcp ? "TCP" : "UDP")
              << "  loss(%)=" << 100.0 * lossPkts / s.txPackets
              << "  delay(ms)=" << 1000 * s.delaySum.GetSeconds() / s.rxPackets
              << "  throughput(Mbps)=" << s.rxBytes * 8.0 /
                     (s.timeLastRxPacket.GetSeconds() - s.timeFirstTxPacket.GetSeconds()) / 1e6;
    if (isTcp)
      std::cout << "  retx rate(%)=" << 100.0 * lossPkts / s.txPackets;   // lost segments get retransmitted
    std::cout << std::endl;
  }




















#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
using namespace ns3;

int main() {
    NodeContainer n;
    n.Create(6); // 0,1 = senders; 2,3 = receivers; 4,5 = routers

    PointToPointHelper access, bottleneck;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    bottleneck.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer d0 = access.Install(n.Get(0), n.Get(4));
    NetDeviceContainer d1 = access.Install(n.Get(1), n.Get(4));
    NetDeviceContainer db = bottleneck.Install(n.Get(4), n.Get(5));
    NetDeviceContainer d2 = access.Install(n.Get(5), n.Get(2));
    NetDeviceContainer d3 = access.Install(n.Get(5), n.Get(3));

    InternetStackHelper internet;
    internet.Install(n);

    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0"); ip.Assign(d0);
    ip.SetBase("10.1.2.0", "255.255.255.0"); ip.Assign(d1);
    ip.SetBase("10.1.3.0", "255.255.255.0"); ip.Assign(db);
    ip.SetBase("10.1.4.0", "255.255.255.0"); ip.Assign(d2);
    ip.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i3 = ip.Assign(d3);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    PacketSinkHelper tcpSink("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5000));
    PacketSinkHelper udpSink("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 6000));
    tcpSink.Install(n.Get(3)).Start(Seconds(0));
    udpSink.Install(n.Get(2)).Start(Seconds(0));

    BulkSendHelper tcp("ns3::TcpSocketFactory",
        InetSocketAddress(i3.GetAddress(1), 5000));
    tcp.SetAttribute("SendSize", UintegerValue(1024));
    tcp.Install(n.Get(0)).Start(Seconds(1));

    OnOffHelper udp("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address("10.1.4.2"), 6000));
    udp.SetAttribute("DataRate", StringValue("1Mbps"));
    udp.SetAttribute("PacketSize", UintegerValue(512));
    udp.Install(n.Get(1)).Start(Seconds(1));

    FlowMonitorHelper fm;
    Ptr<FlowMonitor> monitor = fm.InstallAll();

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> cl =
    DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());

    auto stats = monitor->GetFlowStats();

for (const auto &kv : stats) {
    Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
    const auto &s = kv.second;

    // Keep only our TCP and UDP application flows
    if (t.destinationPort != 5000 && t.destinationPort != 6000)
        continue;

    double dur = (s.timeLastRxPacket -
                  s.timeFirstTxPacket).GetSeconds();

    double mbps = dur > 0
        ? s.rxBytes * 8.0 / dur / 1e6
        : 0;

    double delay = s.rxPackets > 0
        ? s.delaySum.GetSeconds() / s.rxPackets * 1000.0
        : 0;

    double loss = s.txPackets > 0
        ? 100.0 * (s.txPackets - s.rxPackets) / s.txPackets
        : 0;

    double pdr = s.txPackets > 0
        ? 100.0 * s.rxPackets / s.txPackets
        : 0;

    std::cout << (t.destinationPort == 5000 ? "TCP" : "UDP")
              << " Flow " << kv.first
              << " (" << t.sourceAddress << " -> "
              << t.destinationAddress
              << ", Port " << t.destinationPort << ")\n"
              << " Throughput: " << mbps << " Mbps\n"
              << " Avg Delay: " << delay << " ms\n"
              << " Loss: " << loss << "%\n"
              << " PDR: " << pdr << "%\n";
}

Simulator::Destroy();
}
