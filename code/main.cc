#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include <iomanip>
#include <algorithm> // Thêm thư viện để sử dụng std::max

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CSMATrafficComparison");

struct TrafficMetrics {
    uint64_t totalPacketsReceived = 0;
    uint64_t totalPacketsSent = 0;
    uint64_t totalBytesReceived = 0;
    uint64_t totalBytesSent = 0;
    double firstRxTime = -1;
    double lastRxTime = 0;
};

TrafficMetrics tcpMetrics, udpMetrics;
std::ofstream csvFile;

// ===================== TCP CALLBACKS =====================
void TcpRxCallback (Ptr<const Packet> packet, const Address &address)
{
    // LƯU Ý: Đây là điểm đo sau PacketSink. Nó sẽ đếm Data Packets được nhận 
    // và có thể bao gồm các gói tin điều khiển khác nếu trace không lọc (ví dụ: ACKs)
    tcpMetrics.totalPacketsReceived++;
    tcpMetrics.totalBytesReceived += packet->GetSize();
    
    double now = Simulator::Now().GetSeconds();
    if (tcpMetrics.firstRxTime < 0) {
        tcpMetrics.firstRxTime = now;
    }
    tcpMetrics.lastRxTime = now;
}

void TcpTxCallback (Ptr<const Packet> packet)
{
    // Đây là điểm đo sau OnOff Application. Nó chỉ đếm Data Packets được gửi.
    tcpMetrics.totalPacketsSent++;
    tcpMetrics.totalBytesSent += packet->GetSize();
}

// ===================== UDP CALLBACKS =====================
void UdpRxCallback (Ptr<const Packet> packet, const Address &address)
{
    udpMetrics.totalPacketsReceived++;
    udpMetrics.totalBytesReceived += packet->GetSize();
    
    double now = Simulator::Now().GetSeconds();
    if (udpMetrics.firstRxTime < 0) {
        udpMetrics.firstRxTime = now;
    }
    udpMetrics.lastRxTime = now;
}

void UdpTxCallback (Ptr<const Packet> packet)
{
    udpMetrics.totalPacketsSent++;
    udpMetrics.totalBytesSent += packet->GetSize();
}

// ===================== PERIODIC STATISTICS =====================
void RecordStatistics(double simTime)
{
    double now = Simulator::Now().GetSeconds();
    
    double tcpThroughput = 0;
    double udpThroughput = 0;
    
    // Tính throughput dựa trên thời gian thực tế nhận dữ liệu
    if (tcpMetrics.lastRxTime > tcpMetrics.firstRxTime && tcpMetrics.firstRxTime > 0) {
        double tcpDuration = tcpMetrics.lastRxTime - tcpMetrics.firstRxTime;
        // Chuyển đổi Bytes/s sang Mbps
        tcpThroughput = (tcpMetrics.totalBytesReceived * 8) / tcpDuration / 1000000; 
    }
    
    if (udpMetrics.lastRxTime > udpMetrics.firstRxTime && udpMetrics.firstRxTime > 0) {
        double udpDuration = udpMetrics.lastRxTime - udpMetrics.firstRxTime;
        // Chuyển đổi Bytes/s sang Mbps
        udpThroughput = (udpMetrics.totalBytesReceived * 8) / udpDuration / 1000000;
    }
    
    double tcpPDR = 0, udpPDR = 0;
    // LƯU Ý QUAN TRỌNG: Công thức này sẽ cho TCP PDR gần 200% do đếm ACKs
    if (tcpMetrics.totalPacketsSent > 0) {
        tcpPDR = (double)tcpMetrics.totalPacketsReceived / tcpMetrics.totalPacketsSent * 100;
    }
    if (udpMetrics.totalPacketsSent > 0) {
        udpPDR = (double)udpMetrics.totalPacketsReceived / udpMetrics.totalPacketsSent * 100;
    }

    csvFile << now << ","
            << tcpMetrics.totalPacketsSent << ","
            << tcpMetrics.totalPacketsReceived << ","
            << tcpMetrics.totalBytesReceived << ","
            << tcpThroughput << ","
            << tcpPDR << ","
            << udpMetrics.totalPacketsSent << ","
            << udpMetrics.totalPacketsReceived << ","
            << udpMetrics.totalBytesReceived << ","
            << udpThroughput << ","
            << udpPDR << "\n";
    csvFile.flush();

    // Tiếp tục schedule cho đến gần cuối simTime
    if (now < simTime - 0.05) {
        Simulator::Schedule(Seconds(0.1), &RecordStatistics, simTime);
    }
}

void PrintComparison(double simTime)
{
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         CSMA NETWORK PERFORMANCE: TCP vs UDP COMPARISON        ║\n";
    std::cout << "║                (Both using OnOff Application)                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    // TCP Metrics
    std::cout << "【 TCP TRAFFIC METRICS 】\n";
    std::cout << "  Total Packets Sent:      " << tcpMetrics.totalPacketsSent << std::endl;
    std::cout << "  Total Packets Received:  " << tcpMetrics.totalPacketsReceived << std::endl;
    std::cout << "  Total Bytes Received:    " << tcpMetrics.totalBytesReceived << " bytes" << std::endl;

    double tcpPDR = 0;
    if (tcpMetrics.totalPacketsSent > 0) {
        tcpPDR = (double)tcpMetrics.totalPacketsReceived / tcpMetrics.totalPacketsSent * 100;
        // Đã sửa lỗi ký tự mở rộng ở đây
        std::cout << "  Packet Delivery Ratio:   " << std::fixed << std::setprecision(2) 
                  << tcpPDR << "%" << std::endl;
    }

    double tcpThroughput = 0;
    double tcpDuration = 0;
    if (tcpMetrics.lastRxTime > tcpMetrics.firstRxTime && tcpMetrics.firstRxTime > 0) {
        tcpDuration = tcpMetrics.lastRxTime - tcpMetrics.firstRxTime;
        tcpThroughput = (tcpMetrics.totalBytesReceived * 8) / tcpDuration / 1000000;
        // Đã sửa lỗi ký tự mở rộng ở đây
        std::cout << "  Throughput:              " << std::fixed << std::setprecision(2) 
                  << tcpThroughput << " Mbps" << std::endl;
        std::cout << "  Active Duration:         " << std::fixed << std::setprecision(3)
                  << tcpDuration << " seconds" << std::endl;
    }

    // UDP Metrics
    std::cout << "\n【 UDP TRAFFIC METRICS 】\n";
    std::cout << "  Total Packets Sent:      " << udpMetrics.totalPacketsSent << std::endl;
    std::cout << "  Total Packets Received:  " << udpMetrics.totalPacketsReceived << std::endl;
    std::cout << "  Total Bytes Received:    " << udpMetrics.totalBytesReceived << " bytes" << std::endl;

    double udpPDR = 0;
    double udpDuration = 0;
    if (udpMetrics.totalPacketsSent > 0) {
        udpPDR = (double)udpMetrics.totalPacketsReceived / udpMetrics.totalPacketsSent * 100;
        // Đã sửa lỗi ký tự mở rộng ở đây
        std::cout << "  Packet Delivery Ratio:   " << std::fixed << std::setprecision(2) 
                  << udpPDR << "%" << std::endl;
    }

    double udpThroughput = 0;
    if (udpMetrics.lastRxTime > udpMetrics.firstRxTime && udpMetrics.firstRxTime > 0) {
        udpDuration = udpMetrics.lastRxTime - udpMetrics.firstRxTime;
        udpThroughput = (udpMetrics.totalBytesReceived * 8) / udpDuration / 1000000;
        // Đã sửa lỗi ký tự mở rộng ở đây
        std::cout << "  Throughput:              " << std::fixed << std::setprecision(2) 
                  << udpThroughput << " Mbps" << std::endl;
        std::cout << "  Active Duration:         " << std::fixed << std::setprecision(3)
                  << udpDuration << " seconds" << std::endl;
    }

    // Comparison
    std::cout << "\n【 PERFORMANCE COMPARISON 】\n";
    
    if (tcpThroughput > 0 && udpThroughput > 0) {
        double throughputDiff = std::abs(tcpThroughput - udpThroughput);
        // Sử dụng std::max để tránh chia cho 0 hoặc giá trị quá nhỏ
        double throughputDiffPercent = (throughputDiff / std::max(tcpThroughput, udpThroughput)) * 100;
        
        if (tcpThroughput > udpThroughput) {
            // Đã sửa lỗi ký tự mở rộng ở đây
            std::cout << "  ✓ TCP faster by " << std::fixed << std::setprecision(2) 
                      << throughputDiffPercent << "% (" << tcpThroughput << " vs " << udpThroughput << " Mbps)" << std::endl;
        } else if (udpThroughput > tcpThroughput) {
            // Đã sửa lỗi ký tự mở rộng ở đây
            std::cout << "  ✓ UDP faster by " << std::fixed << std::setprecision(2) 
                      << throughputDiffPercent << "% (" << udpThroughput << " vs " << tcpThroughput << " Mbps)" << std::endl;
        } else {
            // Đã sửa lỗi unused variable bằng cách sử dụng throughputDiffPercent
            std::cout << "  ✓ TCP and UDP have similar throughput: " << tcpThroughput << " Mbps (Diff: " 
                      << std::fixed << std::setprecision(2) << throughputDiffPercent << "%)" << std::endl;
        }
    }
    
    if (tcpPDR > 0 && udpPDR > 0) {
        std::cout << "  ✓ PDR Comparison: TCP " << std::fixed << std::setprecision(2)
                  << tcpPDR << "% vs UDP " << udpPDR << "%";
        
        // Điều chỉnh so sánh PDR để làm rõ vấn đề TCP
        if (tcpPDR > 100.0) {
            std::cout << " (TCP PDR > 100% due to counting ACKs in received packets!)";
        } else if (tcpPDR > udpPDR) {
            std::cout << " (TCP more reliable by " << (tcpPDR - udpPDR) << "%)";
        } else if (udpPDR > tcpPDR) {
            std::cout << " (UDP more reliable by " << (udpPDR - tcpPDR) << "%)";
        }
        std::cout << std::endl;
    }

    std::cout << "\n【 KEY INSIGHTS 】\n";
    std::cout << "  • Both protocols use OnOff application with same data rate\n";
    std::cout << "  • TCP provides reliability through acknowledgments and retransmissions\n";
    std::cout << "  • UDP has lower overhead but no guaranteed delivery\n";
    std::cout << "  • Throughput difference shows protocol overhead impact\n";

    std::cout << "\n  ✓ CSV file saved: csma_metrics.csv" << std::endl;
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n\n";
}

int main (int argc, char *argv[])
{
    uint32_t nNodes = 5;
    uint32_t dataRate = 100;
    uint32_t delay = 500; // Where we change its value
    uint32_t simTime = 10;
    uint32_t packetSize = 1024;
    uint32_t appRate = 200; // Application data rate in Mbps (same for both TCP and UDP) - Where we change its value

    CommandLine cmd (__FILE__);
    cmd.AddValue ("nNodes", "Number of CSMA nodes", nNodes);
    cmd.AddValue ("dataRate", "Channel data rate in Mbps", dataRate);
    cmd.AddValue ("delay", "Channel delay in microseconds", delay);
    cmd.AddValue ("simTime", "Simulation time in seconds", simTime);
    cmd.AddValue ("packetSize", "Packet size in bytes", packetSize);
    cmd.AddValue ("appRate", "Application sending rate in Mbps (for both TCP and UDP)", appRate);
    cmd.Parse (argc, argv);

    LogComponentEnable ("CSMATrafficComparison", LOG_LEVEL_INFO);

    // ================ OPEN CSV FILE ================
    csvFile.open("csma_metrics.csv", std::ios::out);
    csvFile << "Time,TCP_Packets_Sent,TCP_Packets_Received,TCP_Bytes_Received,"
            << "TCP_Throughput_Mbps,TCP_PDR_%,UDP_Packets_Sent,UDP_Packets_Received,"
            << "UDP_Bytes_Received,UDP_Throughput_Mbps,UDP_PDR_%\n";

    // ================ CREATE TOPOLOGY ================
    NodeContainer tcpNodes, udpNodes;
    tcpNodes.Create (nNodes);
    udpNodes.Create (nNodes);

    // TCP CSMA Channel
    CsmaHelper csmaTcp;
    csmaTcp.SetChannelAttribute ("DataRate", StringValue (std::to_string(dataRate) + "Mbps"));
    csmaTcp.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delay)));
    NetDeviceContainer tcpDevices = csmaTcp.Install (tcpNodes);

    // UDP CSMA Channel
    CsmaHelper csmaUdp;
    csmaUdp.SetChannelAttribute ("DataRate", StringValue (std::to_string(dataRate) + "Mbps"));
    csmaUdp.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delay)));
    NetDeviceContainer udpDevices = csmaUdp.Install (udpNodes);

    // ================ MOBILITY MODEL ================
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (tcpNodes);
    mobility.Install (udpNodes);

    // ================ INTERNET STACK ================
    InternetStackHelper internet;
    internet.Install (tcpNodes);
    internet.Install (udpNodes);

    Ipv4AddressHelper ipv4Tcp;
    ipv4Tcp.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer tcpInterfaces = ipv4Tcp.Assign (tcpDevices);

    Ipv4AddressHelper ipv4Udp;
    ipv4Udp.SetBase ("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer udpInterfaces = ipv4Udp.Assign (udpDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // ================ TCP APPLICATIONS (OnOff) ================
    uint16_t tcpPort = 9;
    Address tcpSinkAddress (InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
    PacketSinkHelper tcpSinkHelper ("ns3::TcpSocketFactory", tcpSinkAddress);
    ApplicationContainer tcpServerApps = tcpSinkHelper.Install (tcpNodes.Get (nNodes - 1));
    tcpServerApps.Start (Seconds (0.0));
    tcpServerApps.Stop (Seconds (simTime));

    // TCP Sender - OnOff Application
    OnOffHelper tcpOnOff ("ns3::TcpSocketFactory",
                          InetSocketAddress (tcpInterfaces.GetAddress (nNodes - 1), tcpPort));
    tcpOnOff.SetConstantRate (DataRate (std::to_string(appRate) + "Mbps"), packetSize);
    tcpOnOff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
    tcpOnOff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
    ApplicationContainer tcpClientApps = tcpOnOff.Install (tcpNodes.Get (0));
    tcpClientApps.Start (Seconds (1.0));
    tcpClientApps.Stop (Seconds (simTime));

    // ================ UDP APPLICATIONS (OnOff) ================
    uint16_t udpPort = 10;
    Address udpSinkAddress (InetSocketAddress (Ipv4Address::GetAny (), udpPort));
    PacketSinkHelper udpSinkHelper ("ns3::UdpSocketFactory", udpSinkAddress);
    ApplicationContainer udpServerApps = udpSinkHelper.Install (udpNodes.Get (nNodes - 1));
    udpServerApps.Start (Seconds (0.0));
    udpServerApps.Stop (Seconds (simTime));

    // UDP Sender - OnOff Application (same rate as TCP for fair comparison)
    OnOffHelper udpOnOff ("ns3::UdpSocketFactory",
                          InetSocketAddress (udpInterfaces.GetAddress (nNodes - 1), udpPort));
    udpOnOff.SetConstantRate (DataRate (std::to_string(appRate) + "Mbps"), packetSize);
    udpOnOff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
    udpOnOff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
    ApplicationContainer udpClientApps = udpOnOff.Install (udpNodes.Get (0));
    udpClientApps.Start (Seconds (1.0));
    udpClientApps.Stop (Seconds (simTime));

    // ================ TRACE CALLBACKS (Application Layer) ================
    // TCP traces
    Ptr<PacketSink> tcpSink = DynamicCast<PacketSink>(tcpServerApps.Get(0));
    tcpSink->TraceConnectWithoutContext("Rx", MakeCallback(&TcpRxCallback));
    
    Ptr<OnOffApplication> tcpSender = DynamicCast<OnOffApplication>(tcpClientApps.Get(0));
    tcpSender->TraceConnectWithoutContext("Tx", MakeCallback(&TcpTxCallback));

    // UDP traces
    Ptr<PacketSink> udpSink = DynamicCast<PacketSink>(udpServerApps.Get(0));
    udpSink->TraceConnectWithoutContext("Rx", MakeCallback(&UdpRxCallback));
    
    Ptr<OnOffApplication> udpSender = DynamicCast<OnOffApplication>(udpClientApps.Get(0));
    udpSender->TraceConnectWithoutContext("Tx", MakeCallback(&UdpTxCallback));

    // ================ CONFIGURATION INFO ================
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         CSMA NETWORK CONFIGURATION - TCP vs UDP                ║\n";
    std::cout << "║             (Fair Comparison: Both using OnOff)                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "  Number of Nodes per Network:   " << nNodes << std::endl;
    std::cout << "  Channel Data Rate:             " << dataRate << " Mbps" << std::endl;
    std::cout << "  Propagation Delay:             " << delay << " microseconds" << std::endl;
    std::cout << "  Packet Size:                   " << packetSize << " bytes" << std::endl;
    std::cout << "  Application Data Rate:         " << appRate << " Mbps (same for both)" << std::endl;
    std::cout << "  TCP Application:               OnOff (always on)" << std::endl;
    std::cout << "  UDP Application:               OnOff (always on)" << std::endl;
    std::cout << "  Simulation Time:               " << simTime << " seconds\n" << std::endl;
    std::cout << "  Starting simulation...\n" << std::endl;

    // ================ SCHEDULE STATISTICS ================
    Simulator::Schedule(Seconds(1.1), &RecordStatistics, (double)simTime);

    // ================ RUN SIMULATION ================
    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();

    // ================ PRINT RESULTS ================
    PrintComparison((double)simTime);

    csvFile.close();
    Simulator::Destroy ();
    return 0;
}
