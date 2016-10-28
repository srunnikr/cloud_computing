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

// Network topology
//
//        n0     n1
//        |      |
//       ----------
//       | Switch |
//       ----------
//        |      |
//        n2     n3
//
//
// - CBR/UDP flows from n0 to n1 and from n3 to n0
// - DropTail queues
// - Tracing of queues and packet receptions to file "openflow-switch.tr"
// - If order of adding nodes and netdevices is kept:
//      n0 = 00:00:00;00:00:01, n1 = 00:00:00:00:00:03, n3 = 00:00:00:00:00:07
//	and port number corresponds to node number, so port 0 is connected to n0, for example.

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

NS_LOG_COMPONENT_DEFINE ("PortlandMiniExample");

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

  if (verbose)
    {
      LogComponentEnable ("PortlandMiniExample", LOG_LEVEL_INFO);
      LogComponentEnable ("PortlandInterface", LOG_LEVEL_INFO);
      LogComponentEnable ("PortlandSwitchNetDevice", LOG_LEVEL_INFO);
    }

    // char filename[] = "statistics/Portland-Mini-2.xml";
  //
  // Explicitly create the nodes required by the topology (shown above).
  //
  NS_LOG_INFO ("Create nodes.");
  NodeContainer terminals;
  terminals.Create (2);

  NodeContainer edgeSwitch1;
  edgeSwitch1.Create(1);
  NodeContainer edgeSwitch2;
  edgeSwitch2.Create(1);
  NodeContainer aggSwitch1;
  aggSwitch1.Create(1);
  NodeContainer aggSwitch2;
  aggSwitch2.Create(1);
  NodeContainer coreSwitch;
  coreSwitch.Create(1);
  
  NS_LOG_INFO ("Build Topology");
  // CsmaHelper csma;
  // csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  // csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("2000Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue (MilliSeconds (0.001)));

   int radix = 2;
  // Create the csma links, from each terminal to the switch
  NetDeviceContainer terminalDevices1;
  NetDeviceContainer terminalDevices2;
  NetDeviceContainer edgeDev1[radix];
  NetDeviceContainer edgeDev2[radix];
  NetDeviceContainer aggregationDev1[radix];
  NetDeviceContainer aggregationDev2[radix];
  NetDeviceContainer coreDev[radix]; 
  
    NetDeviceContainer link1 = csma.Install (NodeContainer(terminals.Get (0), edgeSwitch1));
    terminalDevices1.Add (link1.Get (0));
    edgeDev1[0].Add (link1.Get (1));
  
    NetDeviceContainer link2 = csma.Install (NodeContainer(terminals.Get (1), edgeSwitch2));
    terminalDevices1.Add (link2.Get (0));
    edgeDev2[0].Add (link2.Get (1));
	
	NetDeviceContainer link3 = csma.Install (NodeContainer(edgeSwitch1, aggSwitch1));
    edgeDev1[1].Add (link3.Get (0));
    aggregationDev1[0].Add (link3.Get (1));

	NetDeviceContainer link4 = csma.Install (NodeContainer(aggSwitch1, coreSwitch));
    aggregationDev1[1].Add (link4.Get (0));
    coreDev[0].Add (link4.Get (1));
	
	NetDeviceContainer link5 = csma.Install (NodeContainer(coreSwitch, aggSwitch2));
    coreDev[1].Add (link5.Get (0));
    aggregationDev2[1].Add (link5.Get (1));

	NetDeviceContainer link6 = csma.Install (NodeContainer(aggSwitch2, edgeSwitch2));
    aggregationDev2[0].Add (link6.Get (0));
	edgeDev2[1].Add (link6.Get (1));
  
  // Create the switch netdevice, which will do the packet switching
  Ptr<Node> edgenode1 = edgeSwitch1.Get (0);
  Ptr<Node> edgenode2 = edgeSwitch2.Get (0);
  Ptr<Node> aggnode1 = aggSwitch1.Get (0);
  Ptr<Node> aggnode2 = aggSwitch2.Get (0);
  Ptr<Node> corenode = coreSwitch.Get (0);
  
  PortlandSwitchHelper swtche1;
  PortlandSwitchHelper swtche2;
  PortlandSwitchHelper swtcha1;
  PortlandSwitchHelper swtcha2;
  PortlandSwitchHelper swtchc;
  
  NetDeviceContainer WAN;
  NetDeviceContainer low_core;
  low_core.Add(coreDev[0]);
  low_core.Add(coreDev[1]);
  
  Ptr<ns3::pld::FabricManager> fabricManager = Create<ns3::pld::FabricManager> ();
  swtche1.Install(edgenode1, edgeDev1[0], edgeDev1[1], fabricManager, EDGE, 0, 0);
  swtche2.Install(edgenode2, edgeDev2[0], edgeDev2[1], fabricManager, EDGE, 1, 0);
  swtcha1.Install(aggnode1, aggregationDev1[0], aggregationDev1[1], fabricManager, AGGREGATION, 0, 0);
  swtcha2.Install(aggnode2, aggregationDev2[0], aggregationDev2[1], fabricManager, AGGREGATION, 1, 0);
  swtchc.Install(corenode, low_core, WAN, fabricManager, CORE, 0, 0);
  	
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
  internet.Install (terminals);

  // We've got the "hardware" in place.  Now we need to add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (terminalDevices1);
  ipv4.Assign (terminalDevices2);	
  // Create an OnOff application to send UDP datagrams from n0 to n1.
  NS_LOG_INFO ("Create Applications.");
  uint16_t port = 9;   // Discard port (RFC 863)
  int packetSize = 1024;
  OnOffHelper oo ("ns3::UdpSocketFactory",
                     Address (InetSocketAddress (Ipv4Address ("10.1.1.2"), port)));
  oo.SetAttribute("OnTime",RandomVariableValue(ConstantVariable(1)));  
  oo.SetAttribute("OffTime",RandomVariableValue(ConstantVariable(0))); 
  oo.SetAttribute("PacketSize",UintegerValue (packetSize));
  oo.SetAttribute("DataRate",StringValue ("1000Mbps"));      
  oo.SetAttribute("MaxBytes",StringValue ("0"));

  ApplicationContainer m_sink;
  ApplicationContainer app = oo.Install (terminals.Get (0));
  // Start the application
  app.Start (Seconds (0.01));
  app.Stop (Seconds (0.15));

  // Create an optional packet sink to receive these packets
  // PacketSinkHelper sink ("ns3::UdpSocketFactory",
  //                        Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  // m_sink = sink.Install (terminals.Get(0));
  // m_sink.Start (Seconds (0.0));

  //
  // Create a similar flow from n3 to n0, starting at time 1.1 seconds
  //
  oo.SetAttribute ("Remote",
                      AddressValue (InetSocketAddress (Ipv4Address ("10.1.1.1"), port)));
  ApplicationContainer app1 = oo.Install (terminals.Get (1));
  app1.Start (Seconds (0.31));
  app1.Stop (Seconds (0.32));

  // m_sink = sink.Install (terminals.Get (1));
  // m_sink.Start (Seconds (0.0));

  NS_LOG_INFO ("Configure Tracing.");

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "openflow-switch.tr"
  //
  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("portland-switch.tr"));

  //
  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     openflow-switch-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  //
  csma.EnablePcapAll ("portland-switch", false);

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds(10.0));
  Simulator::Run ();

  monitor->CheckForLostPackets();
  monitor->SerializeToXmlFile("statistics/Portland-Mini-2.xml", true, true);
  monitor->PrintAggregatedStatistics();

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
  //#else
  //NS_LOG_INFO ("NS-3 OpenFlow is not enabled. Cannot run simulation.");
  //#endif // NS3_OPENFLOW
}
