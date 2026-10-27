#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

// EXTRA: manual latency/throughput bookkeeping (replaces FlowMonitor)
std::map<uint64_t, double> txTime; // packet UID -> send time
double firstTx[2] = {-1, -1}, lastRx[2] = {0, 0}, delaySum[2] = {0, 0};
uint32_t rxPkts[2] = {0, 0}, txPkts[2] = {0, 0};

void TxCb(int f, Ptr<const Packet> p)
{
    double t = Simulator::Now().GetSeconds();
    if (firstTx[f] < 0)
        firstTx[f] = t;
    txTime[p->GetUid()] = t;
    txPkts[f]++;
}
void RxCb(int f, Ptr<const Packet> p)
{
    double t = Simulator::Now().GetSeconds();
    delaySum[f] += t - txTime[p->GetUid()];
    lastRx[f] = t;
    rxPkts[f]++;
}

int main()
{
    // 1. Nodes
    NodeContainer n;
    n.Create(4);

    // 2. Links: 5 Mbps, 2 ms
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d01 = p2p.Install(n.Get(0), n.Get(1));
    NetDeviceContainer d12 = p2p.Install(n.Get(1), n.Get(2));
    NetDeviceContainer d13 = p2p.Install(n.Get(1), n.Get(3));

    // 3. Internet stack
    InternetStackHelper internet;
    internet.Install(n);

    // 4. IP addressing
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i01 = ipv4.Assign(d01);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i12 = ipv4.Assign(d12);
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i13 = ipv4.Assign(d13);

    // 5. Routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 6. UDP flows
    // Flow 1: n0 -> n2, port 5000, 1 Mbps
    UdpServerHelper server1(5000);
    ApplicationContainer sApp1 = server1.Install(n.Get(2));
    sApp1.Start(Seconds(0.0));
    sApp1.Stop(Seconds(10.0));
    UdpClientHelper client1(i12.GetAddress(1), 5000);
    client1.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    client1.SetAttribute("PacketSize", UintegerValue(512));
    client1.SetAttribute("Interval", TimeValue(Seconds(512 * 8.0 / 1e6)));
    ApplicationContainer cApp1 = client1.Install(n.Get(0));
    cApp1.Start(Seconds(1.0));
    cApp1.Stop(Seconds(10.0));

    // Flow 2: n3 -> n0, port 6000, 500 Kbps
    UdpServerHelper server2(6000);
    ApplicationContainer sApp2 = server2.Install(n.Get(0));
    sApp2.Start(Seconds(0.0));
    sApp2.Stop(Seconds(10.0));
    UdpClientHelper client2(i01.GetAddress(0), 6000);
    client2.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    client2.SetAttribute("PacketSize", UintegerValue(512));
    client2.SetAttribute("Interval", TimeValue(Seconds(512 * 8.0 / 5e5)));
    ApplicationContainer cApp2 = client2.Install(n.Get(3));
    cApp2.Start(Seconds(1.0));
    cApp2.Stop(Seconds(10.0));

    // EXTRA: connect Tx/Rx trace callbacks to the apps
    cApp1.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&TxCb, 0));
    sApp1.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&RxCb, 0));
    cApp2.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&TxCb, 1));
    sApp2.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&RxCb, 1));

    // 7. Run 10 s
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    // 8. Metrics
    const char *name[2] = {"Flow 1 (n0->n2, 1 Mbps)", "Flow 2 (n3->n0, 500 Kbps)"};
    for (int f = 0; f < 2; f++)
    {
        double bytes = rxPkts[f] * 512.0;
        double dur = lastRx[f] - firstTx[f];
        std::cout << name[f] << ":\n"
                  << "  Tx packets: " << txPkts[f] << "  Rx packets: " << rxPkts[f] << "\n"
                  << "  Total bytes received: " << bytes << " B\n"
                  << "  Throughput: " << bytes * 8.0 / dur / 1e6 << " Mbps\n"
                  << "  Avg delay: " << delaySum[f] / rxPkts[f] * 1000.0 << " ms\n"
                  << "  Loss: " << 100.0 * (txPkts[f] - rxPkts[f]) / txPkts[f] << " %\n"
                  << "  PDR: " << 100.0 * rxPkts[f] / txPkts[f] << " %\n";
    }
    Simulator::Destroy();
    return 0;
}