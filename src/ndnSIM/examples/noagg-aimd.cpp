#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/error-model.h"

namespace ns3 {

    void PacketDropCallback(std::string context, Ptr<const Packet> packet){
        uint32_t droppedPacket = 0;
        droppedPacket++;
        std::cout << "Packet dropped! Total dropped packets: " << droppedPacket << std::endl;
    }

    int
    main(int argc, char* argv[])
    {
        CommandLine cmd;
        cmd.Parse(argc, argv);

        AnnotatedTopologyReader topologyReader("", 25);
        topologyReader.SetFileName("src/ndnSIM/examples/topologies/DataCenterTopology.txt");
        topologyReader.Read();

        // Create error model to add packet loss
        Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
        em->SetAttribute("ErrorUnit", EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
        em->SetAttribute("ErrorRate", DoubleValue(0.01));

        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.InstallAll();

        ndn::GlobalRoutingHelper GlobalRoutingHelper;

        // Set BestRoute strategy
        ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

        // Add packet drop tracing to all nodes
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxDrop", MakeCallback(&PacketDropCallback));

        for (NodeContainer::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
            Ptr<Node> node = *i;
            std::string nodeName = Names::FindName(node);

            if (nodeName.find("con") == 0) {
                // Install ConsumerCbr on consumer nodes
                ndn::AppHelper consumerHelper("ns3::ndn::ConsumerINA");
                consumerHelper.SetAttribute("Prefix", StringValue("pro0.pro1.pro2.pro3.pro4.pro5.pro6.pro7.pro8.pro9.pro10.pro11.pro12.pro13.pro14.pro15.pro16.pro17.pro18.pro19"));
                consumerHelper.SetAttribute("Window", StringValue("1"));
                consumerHelper.SetAttribute("UseCwa", BooleanValue(false));
                auto app1 = consumerHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                app1.Start(Seconds(1));
            } else if (nodeName.find("pro") == 0) {
                // Install Producer on producer nodes
                ndn::AppHelper producerHelper("ns3::ndn::Producer");
                producerHelper.SetPrefix("/" + nodeName);
                //producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
                producerHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                GlobalRoutingHelper.AddOrigins("/" + nodeName, node);

                // Add error rate to producer
                Ptr<NetDevice> proDevice = node->GetDevice(0);
                proDevice->SetAttribute("ReceiveErrorModel", PointerValue(em));
            }
        }
        // Calculate and install FIBs
        ndn::GlobalRoutingHelper::CalculateRoutes();

        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}