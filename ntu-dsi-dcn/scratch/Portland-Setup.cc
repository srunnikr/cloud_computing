/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Project Done as part of Cloud Computing Course at UCSD. (Fall 2016, CSE 291-G) 
// Network topology
// Similar to Fat-Tree, with Portland Switches at Edge, Aggregation and Core

// Traffic pattern: Randomly select client and server. Create flows equal to number of hosts. 
//		Use On/Off Traffic for simulation.

// - CBR/UDP flows from host i to host j

// IP address allocation is not location dependent. The IP address is not used here to optimise routing.
// Instead the PMAC will have location information. This IP address is serially assigned to hosts in 
// the same subnet 10.1.0.0 

// - Simulation Settings:
//                - Number of nodes: 16-3456
//                - Number of pods (k): 4, 8, 16, 24
//		- Simulation running time: 100 seconds
//		- Packet size: 1024 bytes
//		- Data rate for packet sending: 96 Mbps
//		- Data rate for device channel: 1536 Mbps
//		- Delay time for device: 0.001 ms
//		- Communication pairs selection: Random Selection with uniform probability
//		- Traffic flow pattern: Exponential random traffic
//		- Routing protocol: Portland


#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/portland-module.h"
#include "ns3/log.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PortlandSetup");

bool verbose = false;
bool use_drop = false;
ns3::Time timeout = ns3::Seconds (0);

bool
SetVerbose (std::string value)
{
  verbose = true;
  return true;
}

bool
SetDrop (std::string value)
{
  use_drop = true;
  return true;
}

bool
SetTimeout (std::string value)
{
  try {
      timeout = ns3::Seconds (atof (value.c_str ()));
      return true;
    }
  catch (...) { return false; }
  return false;
}

char * toString(int a,int b, int c, int d){

	int first = a;
	int second = b;
	int third = c;
	int fourth = d;

	char *address =  new char[30];
	char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];	
	//address = firstOctet.secondOctet.thirdOctet.fourthOctet;

	bzero(address,30);

	snprintf(firstOctet,10,"%d",first);
	strcat(firstOctet,".");
	snprintf(secondOctet,10,"%d",second);
	strcat(secondOctet,".");
	snprintf(thirdOctet,10,"%d",third);
	strcat(thirdOctet,".");
	snprintf(fourthOctet,10,"%d",fourth);

	strcat(thirdOctet,fourthOctet);
	strcat(secondOctet,thirdOctet);
	strcat(firstOctet,secondOctet);
	strcat(address,firstOctet);

	return address;
}

std::string getHostName(int i, int j)
{
	std::stringstream ss;
	ss << i << "-" << j;
	return ss.str();
}
int
main (int argc, char *argv[])
{
 //#ifdef NS3_OPENFLOW
  //
  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  //
  CommandLine cmd;
  cmd.AddValue ("v", "Verbose (turns on logging).", MakeCallback (&SetVerbose));
  cmd.AddValue ("verbose", "Verbose (turns on logging).", MakeCallback (&SetVerbose));
  cmd.AddValue ("d", "Use Drop Controller (Learning if not specified).", MakeCallback (&SetDrop));
  cmd.AddValue ("drop", "Use Drop Controller (Learning if not specified).", MakeCallback (&SetDrop));
  cmd.AddValue ("t", "Learning Controller Timeout (has no effect if drop controller is specified).", MakeCallback ( &SetTimeout));
  cmd.AddValue ("timeout", "Learning Controller Timeout (has no effect if drop controller is specified).", MakeCallback ( &SetTimeout));

  cmd.Parse (argc, argv);

//=========== Define parameters based on value of k ===========//
//
	int k	= 8;
	int num_pod = k;		// number of pod
	int num_host = (k/2);		// number of hosts under a switch
	int num_edge = (k/2);		// number of edge switch in a pod
	int num_agg = (k/2);		// number of aggregation switch in a pod
	int num_group = (k/2);		// number of group of core switches
    int num_core = (k/2);		// number of core switch in a group
	int total_host = k*k*k/4;	// number of hosts in the entire network	
	char filename [] = "statistics/Portland.xml";// filename for Flow Monitor xml output file

// Define variables for On/Off Application
// These values will be used to serve the purpose that addresses of server and client are selected randomly
// Note: the format of host's address is 10.pod.switch.(host+2)
//
// Initialize other variables
//
	int i = 0;	
	int j = 0;	
	int h = 0;
	
//	
	std::cout << "Value of k =  "<< k<<"\n";
	std::cout << "Total number of hosts =  "<< total_host<<"\n";
	std::cout << "Number of hosts under each switch =  "<< num_host<<"\n";
	std::cout << "Number of edge switch under each pod =  "<< num_edge<<"\n";
	std::cout << "------------- "<<"\n";


  if (verbose)
    {
      LogComponentEnable ("PortlandSetup", LOG_LEVEL_INFO);
      LogComponentEnable ("PortlandInterface", LOG_LEVEL_INFO);
      LogComponentEnable ("PortlandSwitchNetDevice", LOG_LEVEL_INFO);
    }

  //
  // Explicitly create the nodes required by the topology (shown above).
  //


  NS_LOG_INFO ("Create nodes.");

	NodeContainer core[num_group];				// NodeContainer for core switches
	for (i=0; i<num_group;i++){  	
		core[i].Create (num_core);
	}
	NodeContainer agg[num_pod];				// NodeContainer for aggregation switches
	for (i=0; i<num_pod;i++){  	
		agg[i].Create (num_agg);
	}
	NodeContainer edge[num_pod];				// NodeContainer for edge switches
  	for (i=0; i<num_pod;i++){  	
		edge[i].Create (num_edge);
	}
	NodeContainer host[num_pod][num_edge];		// NodeContainer for hosts
  	for (i=0; i<num_pod;i++){
		for (j=0;j<num_edge;j++){  	
			host[i][j].Create (num_host);		
		}
	}

  NS_LOG_INFO ("Build Topology");
  // Initialize parameters for Csma protocol
//
	char dataRate [] = "1536Mbps";	// 1Gbps
	int delay = 0.001;		// 0.001 ms


  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));
  // Inintialize Address Helper
  //	
  	Ipv4AddressHelper address;

  // Create the csma links, from each terminal to the switch

  NetDeviceContainer hostdev[num_pod][num_edge][num_host];
  NetDeviceContainer edgedev[num_pod][num_edge][k];
  NetDeviceContainer aggdev[num_pod][num_agg][k];
  NetDeviceContainer coredev[num_group][num_core][k];
  	
  	for (i=0;i<num_pod;i++){
		for (j=0;j<num_edge; j++){
			for (h=0; h< num_host;h++){			
				NetDeviceContainer link1 = csma.Install (NodeContainer (host[i][j].Get(h), edge[i].Get(j)));
				hostdev[i][j][h].Add(link1.Get(0));			
				edgedev[i][j][h].Add(link1.Get(1));	
	
			}
		}
	}
	std::cout << "Finished connecting edge switches and hosts  "<< "\n";

  	for (i=0;i<num_pod;i++){
		for (j=0;j<num_agg; j++){
			for (h=0; h< num_edge;h++){			
				NetDeviceContainer link1 = csma.Install (NodeContainer (edge[i].Get(h), agg[i].Get(j)));
				edgedev[i][h][j+num_host].Add(link1.Get(0));			
				aggdev[i][j][h].Add(link1.Get(1));						
			}
		}
	}
	std::cout << "Finished connecting aggregate switches and edge switches  "<< "\n"; 

  	for (i=0;i<num_group;i++){
		for (j=0;j<num_core; j++){
			for (h=0; h< num_pod;h++){			
				NetDeviceContainer link1 = csma.Install (NodeContainer (agg[h].Get(i), core[i].Get(j)));
				aggdev[h][i][j+num_edge].Add(link1.Get(0));			
				coredev[i][j][h].Add(link1.Get(1));						
			}
		}
	}
	std::cout << "Finished connecting core switches and aggregate switches  "<< "\n"; 
	
  // Create the switch netdevice, which will do the packet switching

  	Ptr<Node> corenode[num_group][num_core];			
	for (i=0; i<num_group;i++){  	
		for (j=0; j<num_core; j++) {
			corenode[i][j] = core[i].Get(j);
		}
	}
  	Ptr<Node> aggnode[num_pod][num_agg];			
	for (i=0; i<num_pod;i++){  	
		for (j=0; j<num_agg; j++) {
			aggnode[i][j] = agg[i].Get(j);
		}
	}
  	Ptr<Node> edgenode[num_pod][num_edge];			
	for (i=0; i<num_pod;i++){  	
		for (j=0; j<num_edge; j++) {
			edgenode[i][j] = edge[i].Get(j);
		}
	}

	PortlandSwitchHelper coresw[num_group][num_core];
    PortlandSwitchHelper aggsw[num_pod][num_agg];
	PortlandSwitchHelper edgesw[num_pod][num_edge];	

  NetDeviceContainer WAN;
  
  Ptr<ns3::pld::FabricManager> fabricManager = Create<ns3::pld::FabricManager> ();

 	for (i=0;i<num_pod;i++){
		for (j=0;j<num_edge; j++){
			NetDeviceContainer low_edge;
			NetDeviceContainer up_edge;
			for (h=0; h< num_host;h++){
				low_edge.Add(edgedev[i][j][h]);
				up_edge.Add(edgedev[i][j][h+num_host]);			
			}
			edgesw[i][j].Install(edgenode[i][j], low_edge, up_edge, fabricManager, EDGE, i, j);
		}
	}

 	for (i=0;i<num_pod;i++){
		for (j=0;j<num_agg; j++){
			NetDeviceContainer low_agg;
			NetDeviceContainer up_agg;
			for (h=0; h< num_edge;h++){
				low_agg.Add(aggdev[i][j][h]);
				up_agg.Add(aggdev[i][j][h+num_edge]);			
			}
			aggsw[i][j].Install(aggnode[i][j], low_agg, up_agg, fabricManager, AGGREGATION, i, j);
		}
	}

 	for (i=0;i<num_group;i++){
		for (j=0;j<num_core; j++){
			NetDeviceContainer low_core;
			for (h=0; h< num_pod;h++){
				low_core.Add(coredev[i][j][h]);
			}
			coresw[i][j].Install(corenode[i][j], low_core, WAN, fabricManager, CORE, 0, 0);
		}
	}	
  	
  /*
  if (use_drop)
    {
      Ptr<ns3::ofi::DropController> controller = CreateObject<ns3::ofi::DropController> ();
      swtch.Install (switchNode, switchDevices, controller);
    }
  else
    {
      Ptr<ns3::ofi::LearningController> controller = CreateObject<ns3::ofi::LearningController> ();
      if (!timeout.IsZero ()) controller->SetAttribute ("ExpirationTime", TimeValue (timeout));
      swtch.Install (switchNode, switchDevices, controller);
    }
  */

  // Add internet stack to the terminals
  InternetStackHelper internet;
  	for (i=0; i<num_pod;i++){
		for (j=0;j<num_edge;j++){
			internet.Install (host[i][j]);		
		}
	}

  // We've got the "hardware" in place.  Now we need to add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
	
	ipv4.SetBase (toString(10, 1, 0, 0), "255.255.0.0");

 	for (i=0;i<num_pod;i++){
		for (j=0;j<num_edge; j++){
			//csma.EnablePcap(getHostName(i, j), host[i][j]);
			for (h=0; h< num_host;h++){
				ipv4.Assign(hostdev[i][j][h]);
			}
		}
	}

  // Create an OnOff application to send UDP datagrams from n0 to n1.
  std::cout << "creating On/Off traffic"<<"\n";
  //=========== Initialize settings for On/Off Application ===========//
//
// Define variables for On/Off Application
// These values will be used to serve the purpose that addresses of server and client are selected randomly
// Note: the format of host's address is 10.pod.switch.(host+2)
//
	int podRand = 0;	//	
	int swRand = 0;		// Random values for servers' address
	int hostRand = 0;	//

	int rand1 =0;		//
	int rand2 =0;		// Random values for clients' address	
	int rand3 =0;		//
// Initialize parameters for On/Off application
//
	int port = 9;
	int packetSize = 1024;		// 1024 bytes
	char dataRate_OnOff [] = "96Mbps";
	char maxBytes [] = "70000";	// 70,000 bytes
	
// Generate traffics for the simulation
//	
	ApplicationContainer app[total_host];
	srand (time(NULL));
	for (i=0;i<num_pod;i++){
		for (j=0;j<num_edge; j++){
			for (h=0; h<num_host; h++){
	//for (i=0;i<total_host;i++){	
		// Randomly select a server

		rand1 = rand() % num_pod + 0;
		rand2 = rand() % num_edge + 0;
		rand3 = rand() % num_host + 0;
 			
		while (rand1== podRand && swRand == rand2 && (rand3) == hostRand){
			rand1 = rand() % num_pod + 0;
			rand2 = rand() % num_edge + 0;
			rand3 = rand() % num_host + 0;
		} // to make sure that client and server are different
		
		// podRand = rand() % num_pod + 0;
		// swRand = rand() % num_edge + 0;
		// hostRand = rand() % num_host + 0;
	
		// char *add;
		// add = toString(10, podRand, swRand, hostRand);
		
		//int deviceRand = rand() % total_host + 1;
		//add = toString(10, 1, 1, deviceRand);

		Ipv4Address dstAddr = host[rand1][rand2].Get(rand3)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
		OnOffHelper oo = OnOffHelper("ns3::UdpSocketFactory",Address(InetSocketAddress(dstAddr, port))); // ip address of server
	        oo.SetAttribute("OnTime",RandomVariableValue(ExponentialVariable(1)));  
	        oo.SetAttribute("OffTime",RandomVariableValue(ExponentialVariable(1))); 
 	        oo.SetAttribute("PacketSize",UintegerValue (packetSize));
 	       	oo.SetAttribute("DataRate",StringValue (dataRate_OnOff));      
	        oo.SetAttribute("MaxBytes",StringValue (maxBytes));

// 		app[i] = oo.Install (host[podRand][swRand].Get (hostRand));
/*		app[i].Start (Seconds (1.0));
		app[i].Stop (Seconds (100.0));	  */
		
 		NodeContainer onoff;
		onoff.Add(host[i][j].Get(h));
	    app[i] = oo.Install (onoff);
		
		
  		PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
		app[i] = sink.Install(host[i][j].Get (h));
		app[i].Start (Seconds (0.0)); 	

		std::cout << "From: " 
		<< host[i][j].Get(h)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()
		<< " dstaddr: "<< dstAddr << "\n";
	//}
		}
	}
	}
	std::cout << "Finished creating On/Off traffic"<<"\n";
	
	//=========== Start the simulation ===========//
//
	//csma.EnablePcapAll ("portland-switch", false);

	std::cout << "Start Simulation.. "<<"\n";
	for (i=0;i<total_host;i++){
		app[i].Start (Seconds (0.0));
  		app[i].Stop (Seconds (100.0));
	}
//  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
// Calculate Throughput using Flowmonitor
//
  	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
// Run simulation.
//
  	// NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(101.0));
  	Simulator::Run ();

  	monitor->CheckForLostPackets ();
  	monitor->SerializeToXmlFile(filename, true, true);
	monitor->PrintAggregatedStatistics();

	std::cout << "Simulation finished "<<"\n";
	
  	Simulator::Destroy ();
  	NS_LOG_INFO ("Done.");
  /* 
  uint16_t port = 9;   // Discard port (RFC 863)

  OnOffHelper onoff ("ns3::UdpSocketFactory",
                     Address (InetSocketAddress (Ipv4Address ("10.1.1.5"), port)));
  onoff.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (1)));
  onoff.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));

  ApplicationContainer app = onoff.Install (host[0][0].Get (0));
  // Start the application
  app.Start (Seconds (1.0));
  app.Stop (Seconds (10.0));

  // Create an optional packet sink to receive these packets
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  app = sink.Install (host[0][0].Get(0));
  app.Start (Seconds (0.0));

  //
  // Create a similar flow from n3 to n0, starting at time 1.1 seconds
  //
  onoff.SetAttribute ("Remote",
                      AddressValue (InetSocketAddress (Ipv4Address ("10.1.1.11"), port)));
  app = onoff.Install (host[1][1].Get (0));
  app.Start (Seconds (1.1));
  app.Stop (Seconds (10.0));

  app = sink.Install (host[1][1].Get (0));
  app.Start (Seconds (0.0));

  NS_LOG_INFO ("Configure Tracing.");

  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "openflow-switch.tr"
  //
  //AsciiTraceHelper ascii;
  //csma.EnableAsciiAll (ascii.CreateFileStream ("portland-switch.tr"));

  //
  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     openflow-switch-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  //
 // csma.EnablePcapAll ("portland-switch", false);

  //
  // Now, do the actual simulation.
  //
// NS_LOG_INFO ("Run Simulation.");
// Simulator::Run ();
// Simulator::Destroy ();
  // Calculate Throughput using Flowmonitor
//
  	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
// Run simulation.
//
  	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(101.0));
  	Simulator::Run ();

  	monitor->CheckForLostPackets ();
  	monitor->SerializeToXmlFile(filename, true, true);
	monitor->PrintAggregatedStatistics();

  	Simulator::Destroy ();
  	NS_LOG_INFO ("Done.");
 */
	
  NS_LOG_INFO ("Done.");
  //#else
  //NS_LOG_INFO ("NS-3 OpenFlow is not enabled. Cannot run simulation.");
  //#endif // NS3_OPENFLOW
}
