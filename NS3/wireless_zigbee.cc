#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h" // EXTRA (not in cheat sheet): for Rx power

using namespace ns3;

// ---- EXTRA: Rx power sampling (no LR-WPAN Rx-power trace in cheat sheet) ----
// Uses LogDistancePropagationLossModel (same default model LrWpanHelper uses),
// default LR-WPAN Tx power = 0 dBm. Sampled every 0.5 s for every sensor.
static double pSum = 0;
static uint32_t pCnt = 0;

static void SampleRxPower(Ptr<LogDistancePropagationLossModel> m,
                          Ptr<MobilityModel> coord, NodeContainer sensors)
{
    for (uint32_t i = 0; i < sensors.GetN(); i++)
    {
        pSum += m->CalcRxPower(0.0, sensors.Get(i)->GetObject<MobilityModel>(), coord);
        pCnt++;
    }
    Simulator::Schedule(Seconds(0.5), &SampleRxPower, m, coord, sensors);
}

static void RunSim(double speed)
{
    pSum = 0;
    pCnt = 0;

    // 1. Nodes: coordinator (gateway) + 3 mobile Zigbee sensors
    NodeContainer coord, sensors, zigbeeNodes;
    coord.Create(1);
    sensors.Create(3);
    zigbeeNodes.Add(coord);   // index 0 = coordinator
    zigbeeNodes.Add(sensors); // index 1..3 = sensors

    // 2. Mobility (installed BEFORE devices so the PHY finds the mobility model)
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(coord);
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(5.0), "MinY", DoubleValue(5.0),
                                  "DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(5.0),
                                  "GridWidth", UintegerValue(3), "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"));
    mobility.Install(sensors);

    // 3. 802.15.4 + 6LoWPAN + IPv6
    LrWpanHelper lrWpan;
    NetDeviceContainer lrDevices = lrWpan.Install(zigbeeNodes);
    lrWpan.AssociateToPan(lrDevices, 0);

    SixLowPanHelper sixLowPan;
    NetDeviceContainer sixDevices = sixLowPan.Install(lrDevices);

    InternetStackHelper ipStack6;
    ipStack6.SetIpv4StackInstall(false);
    ipStack6.Install(zigbeeNodes);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer sixIfaces = ipv6.Assign(sixDevices);

    // 4. Applications: UDP server on coordinator, UDP client on each sensor
    //    (50 B payload <= 80 B; 10 kbps per sensor: 50*8/10e3 = 0.04 s)
    UdpServerHelper server(5000);
    ApplicationContainer sApp = server.Install(coord.Get(0));
    sApp.Start(Seconds(0.0));
    sApp.Stop(Seconds(20.0));

    // EXTRA: GetAddress(0,1) = global IPv6 addr of coordinator (index 0 is link-local)
    UdpClientHelper client(sixIfaces.GetAddress(0, 1), 5000);
    client.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    client.SetAttribute("PacketSize", UintegerValue(50));
    client.SetAttribute("Interval", TimeValue(Seconds(50 * 8.0 / 10e3)));
    ApplicationContainer cApp = client.Install(sensors);
    cApp.Start(Seconds(2.0));
    cApp.Stop(Seconds(20.0));

    // 5. EXTRA: Rx power sampling
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    Simulator::Schedule(Seconds(2.0), &SampleRxPower, loss,
                        coord.Get(0)->GetObject<MobilityModel>(), sensors);

    // 6. FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    mon->CheckForLostPackets();

    // EXTRA: Ipv6FlowClassifier instead of Ipv4FlowClassifier (we use IPv6)
    Ptr<Ipv6FlowClassifier> cl = DynamicCast<Ipv6FlowClassifier>(fmh.GetClassifier6());
    std::map<FlowId, FlowMonitor::FlowStats> stats = mon->GetFlowStats();

    std::cout << "=== ZIGBEE | Speed = " << speed << " m/s ===\n";
    std::cout << "Avg Rx Power (at coordinator): " << (pCnt ? pSum / pCnt : 0) << " dBm\n";
    for (const auto &kv : stats)
    {
        Ipv6FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto &s = kv.second;
        if (t.destinationPort != 5000)
            continue;
        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double kbps = (dur > 0) ? (s.rxBytes * 8.0 / dur / 1e3) : 0;
        double delay = (s.rxPackets > 0) ? (s.delaySum.GetSeconds() / s.rxPackets * 1000.0) : 0;
        double lossP = (s.txPackets > 0) ? (100.0 * (s.txPackets - s.rxPackets) / s.txPackets) : 0;
        double pdr = (s.txPackets > 0) ? (100.0 * s.rxPackets / s.txPackets) : 0;
        std::cout << "Flow " << kv.first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << " Port " << t.destinationPort << "):\n"
                  << "  Throughput: " << kbps << " kbps\n"
                  << "  Avg Delay: " << delay << " ms\n"
                  << "  Loss: " << lossP << " %\n"
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