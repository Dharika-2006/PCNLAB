#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    // 1. Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // 2. Connect nodes using point-to-point link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    // 3. Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // 4. Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign(devices);

    // 5. Install UDP Echo server on Node 1
    UdpEchoServerHelper server(9);

    ApplicationContainer serverApp;
    serverApp = server.Install(nodes.Get(1));

    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // 6. Install UDP Echo client on Node 0
    UdpEchoClientHelper client(interfaces.GetAddress(1), 9);

    client.SetAttribute("MaxPackets", UintegerValue(10));
    client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    client.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp;
    clientApp = client.Install(nodes.Get(0));

    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    // 7. Enable PCAP tracing
    p2p.EnablePcapAll("udp-echo");

    // 8. Run simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
