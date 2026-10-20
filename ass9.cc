#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <iostream>

using namespace ns3;

void Run(std::string tcpType)
{
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
        TypeIdValue(TypeId::LookupByName(tcpType)));

    NodeContainer n;
    n.Create(2);

    PointToPointHelper p;
    p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer d = p.Install(n);

    InternetStackHelper stack;
    stack.Install(n);

    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ip.Assign(d);

    PacketSinkHelper sink("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5000));

    ApplicationContainer rx = sink.Install(n.Get(1));
    rx.Start(Seconds(0));
    rx.Stop(Seconds(10));

    BulkSendHelper source("ns3::TcpSocketFactory",
        InetSocketAddress(i.GetAddress(1), 5000));

    source.SetAttribute("SendSize", UintegerValue(1024));

    ApplicationContainer tx = source.Install(n.Get(0));
    tx.Start(Seconds(1));
    tx.Stop(Seconds(9));

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    uint64_t bytes =
        DynamicCast<PacketSink>(rx.Get(0))->GetTotalRx();

    double throughput = bytes * 8.0 / 10.0 / 1e6;

    std::cout << tcpType
              << " received bytes: " << bytes
              << " | Throughput: " << throughput
              << " Mbps\n";

    Simulator::Destroy();
}

int main()
{
    Run("ns3::TcpNewReno");
    Run("ns3::TcpCubic");
    Run("ns3::TcpBbr");

    return 0;
}