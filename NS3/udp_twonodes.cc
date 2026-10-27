#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

void Run(int latMs, bool twoFlows)
{
    // 1. Topology: Node1 --- Node2
    NodeContainer n;
    n.Create(2);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps")); // ASSUMED: question gives no link rate
    p2p.SetChannelAttribute("Delay", StringValue(std::to_string(latMs) + "ms"));
    NetDeviceContainer d = p2p.Install(n.Get(0), n.Get(1));

    InternetStackHelper internet;
    internet.Install(n);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(d);

    // 2 and 4. Flow k: port 5000+1000k, Rate1 = 1 Mbps, Rate2 = 2 Mbps (ASSUMED rates)
    for (int k = 0; k < (twoFlows ? 2 : 1); k++)
    {
        int port = 5000 + 1000 * k;
        double rate = 1e6 * (k + 1);
        UdpServerHelper server(port);
        ApplicationContainer sApp = server.Install(n.Get(1));
        sApp.Start(Seconds(0.0));
        sApp.Stop(Seconds(12.0));
        UdpClientHelper client(i.GetAddress(1), port);
        client.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
        client.SetAttribute("PacketSize", UintegerValue(512));
        client.SetAttribute("Interval", TimeValue(Seconds(512 * 8.0 / rate)));
        ApplicationContainer cApp = client.Install(n.Get(0));
        cApp.Start(Seconds(1.0));
        cApp.Stop(Seconds(10.0));
    }

    // 3. Measure with FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();
    Simulator::Stop(Seconds(12.0)); // 2 s extra so in-flight packets arrive
    Simulator::Run();
    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    for (const auto &kv : mon->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto &s = kv.second;
        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        std::cout << (twoFlows ? "BOTH" : "ONE ") << " | " << latMs << " ms | Port "
                  << t.destinationPort << " | Rx pkts " << s.rxPackets
                  << " | Throughput " << s.rxBytes * 8.0 / dur / 1e6 << " Mbps"
                  << " | Delay " << s.delaySum.GetSeconds() / s.rxPackets * 1000.0 << " ms"
                  << " | Loss " << 100.0 * (s.txPackets - s.rxPackets) / s.txPackets << " %"
                  << " | PDR " << 100.0 * s.rxPackets / s.txPackets << " %\n";
    }
    Simulator::Destroy();
}

int main()
{
    int lat[4] = {2, 5, 10, 20};
    for (int two = 0; two < 2; two++) // 0 = Part 3 (one flow), 1 = Part 5 (two flows)
        for (int k = 0; k < 4; k++)
            Run(lat[k], two == 1);
    return 0;
}