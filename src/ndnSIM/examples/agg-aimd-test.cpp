#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/error-model.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include <string>

namespace ns3 {

    struct ConfigParams {
        int Constraint;
        std::string Window;
        double Alpha;
        double Beta;
        double Gamma;
        double EWMAFactor;
        double ThresholdFactor;
        bool UseCwa;
        int InterestQueue;
        int QueueSize;
        int Iteration;
    };

    /**
     * Get all required config parameters
     * @return
     */
    ConfigParams GetConfigParams() {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini("src/ndnSIM/experiments/config.ini", pt);

        ConfigParams params;
        params.Constraint = pt.get<int>("General.Constraint");
        params.Window = pt.get<std::string>("General.Window");
        params.Alpha = pt.get<double>("General.Alpha");
        params.Beta = pt.get<double>("General.Beta");
        params.Gamma = pt.get<double>("General.Gamma");
        params.EWMAFactor = pt.get<double>("General.EWMAFactor");
        params.ThresholdFactor = pt.get<double>("General.ThresholdFactor");
        params.UseCwa = pt.get<bool>("General.UseCwa");
        params.InterestQueue = pt.get<int>("Consumer.InterestQueue");
        params.QueueSize = pt.get<int>("Aggregator.QueueSize");
        params.Iteration = pt.get<int>("Consumer.Iteration");

        return params;
    }


    /**
     * Get constraints from config.ini
     * @return
     */
    int GetConstraint() {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini("src/ndnSIM/experiments/config.ini", pt);

        int constraint = pt.get<int>("General.Constraint");
        return constraint;
    }

    /**
     * Define packet loss tracing function
     * @param context
     * @param packet
     */
    void PacketDropCallback(std::string context, Ptr<const Packet> packet){
        uint32_t droppedPacket = 0;
        droppedPacket++;
        std::cout << "Packet dropped! Total dropped packets: " << droppedPacket << std::endl;
    }

    int main(int argc, char* argv[])
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

        // Get constraint from config.ini
        //int constraint = GetConstraint();
        ConfigParams params = GetConfigParams();

        for (NodeContainer::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
            Ptr<Node> node = *i;
            std::string nodeName = Names::FindName(node);

            if (nodeName.find("con") == 0) {
                // Config consumer's attribute on ConsumerINA class
                ndn::AppHelper consumerHelper("ns3::ndn::ConsumerINA");
                consumerHelper.SetAttribute("Iteration", IntegerValue(params.Iteration));
                consumerHelper.SetAttribute("UseCwa", BooleanValue(params.UseCwa));
                consumerHelper.SetAttribute("NodePrefix", StringValue("con0"));
                consumerHelper.SetAttribute("Constraint", IntegerValue(params.Constraint));
                consumerHelper.SetAttribute("Window", StringValue(params.Window));
                consumerHelper.SetAttribute("Alpha", DoubleValue(params.Alpha));
                consumerHelper.SetAttribute("Beta", DoubleValue(params.Beta));
                consumerHelper.SetAttribute("Gamma", DoubleValue(params.Gamma));
                consumerHelper.SetAttribute("EWMAFactor", DoubleValue(params.EWMAFactor));
                consumerHelper.SetAttribute("ThresholdFactor", DoubleValue(params.ThresholdFactor));
                consumerHelper.SetAttribute("InterestQueue", IntegerValue(params.InterestQueue));

                // Add consumer prefix in all nodes' routing info
                auto app1 = consumerHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                app1.Start(Seconds(1));
            } else if (nodeName.find("agg") == 0) {
                // Config aggregator's attribute on aggregator class
                ndn::AppHelper aggregatorHelper("ns3::ndn::Aggregator");
                aggregatorHelper.SetPrefix("/" + nodeName);
                aggregatorHelper.SetAttribute("Iteration", IntegerValue(params.Iteration));
                aggregatorHelper.SetAttribute("UseCwa", BooleanValue(params.UseCwa));
                aggregatorHelper.SetAttribute("Window", StringValue(params.Window));
                aggregatorHelper.SetAttribute("Alpha", DoubleValue(params.Alpha));
                aggregatorHelper.SetAttribute("Beta", DoubleValue(params.Beta));
                aggregatorHelper.SetAttribute("Gamma", DoubleValue(params.Gamma));
                aggregatorHelper.SetAttribute("EWMAFactor", DoubleValue(params.EWMAFactor));
                aggregatorHelper.SetAttribute("ThresholdFactor", DoubleValue(params.ThresholdFactor));
                aggregatorHelper.SetAttribute("QueueSize", IntegerValue(params.QueueSize));

                // Add aggregator prefix in all nodes' routing info
                auto app2 = aggregatorHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                GlobalRoutingHelper.AddOrigins("/" + nodeName, node);
                app2.Start(Seconds(0));
            } else if (nodeName.find("pro") == 0) {
                // Install Producer on producer nodes
                ndn::AppHelper producerHelper("ns3::ndn::Producer");
                producerHelper.SetPrefix("/" + nodeName);

                // Add producer prefix in all nodes' routing info
                producerHelper.Install(node);
                GlobalRoutingHelper.Install(node); // Ensure routing is enabled
                GlobalRoutingHelper.AddOrigins("/" + nodeName, node);

                // Add error rate to producer
                // Ptr<NetDevice> proDevice = node->GetDevice(0);
                // proDevice->SetAttribute("ReceiveErrorModel", PointerValue(em));
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
