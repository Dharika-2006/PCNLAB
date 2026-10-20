#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

int main(int argc, char *argv[])
{
  std::string latency = "2ms";
  double rate = 1.0;        // Rate1 in Mbps
  uint32_t flows = 1;       // 1 = first app only, 2 = both apps
  uint32_t pktSize = 1000;

  CommandLine cmd;
  cmd.AddValue("latency", "P2P link latency", latency);
  cmd.AddValue("rate", "UDP rate in Mbps", rate);
  cmd.AddValue("flows", "Number of UDP applications (1 or 2)", flows);
  cmd.Parse(argc, argv);

  NodeContainer nodes;
  nodes.Create(2);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue(latency));
  NetDeviceContainer devices = p2p.Install(nodes);

  InternetStackHelper internet;
  internet.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign(devices);

  ApplicationContainer servers;
  for (uint32_t i = 0; i < flows; i++)
  {
    uint16_t port = 5000 + 1000 * i;          // 5000, 6000

    UdpServerHelper server(port);
    ApplicationContainer s = server.Install(nodes.Get(1));
    s.Start(Seconds(0.0));
    s.Stop(Seconds(10.0));
    servers.Add(s);

    UdpClientHelper client(interfaces.GetAddress(1), port);
    client.SetAttribute("MaxPackets", UintegerValue(1000000));
    client.SetAttribute("Interval", TimeValue(Seconds(pktSize * 8 / (rate * 1e6))));
    client.SetAttribute("PacketSize", UintegerValue(pktSize));
    ApplicationContainer c = client.Install(nodes.Get(0));
    c.Start(Seconds(1.0));
    c.Stop(Seconds(9.0));
  }

  p2p.EnablePcapAll("udp-ports");

  Simulator::Stop(Seconds(10.0));
  Simulator::Run();

  for (uint32_t i = 0; i < flows; i++)
  {
    uint32_t rx = DynamicCast<UdpServer>(servers.Get(i))->GetReceived();
    std::cout << "Latency " << latency << "  Flow " << i + 1 << " (port " << 5000 + 1000 * i
              << ") throughput: " << rx * pktSize * 8 / 8.0 / 1e6 << " Mbps" << std::endl;  // client active 8 s
  }

  Simulator::Destroy();
  return 0;
}