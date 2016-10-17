#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/openflow-module.h"
#include "ns3/csma-module.h"

using namespace ns3;
using namespace std;

static bool handlePacketCallback(Ptr<NetDevice> dev, Ptr<const Packet> packet, uint16_t protocol,  const Address & receiveAddr) {
  // fabric manager logic
  return true;
}

int main (int argc, char *argv[])
{
  NodeContainer openFlowSwitch;
  openFlowSwitch.Create (1);

  NodeContainer terminalNode;
  terminalNode.Create(1);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  NetDeviceContainer terminalDevice, link, switchDevice;

  // connect the node to the switch
  link = csma.Install (NodeContainer (terminalNode.Get (0), openFlowSwitch.Get(0)));
  terminalDevice.Add (link.Get (0));
  switchDevice.Add (link.Get (1));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Create the switch netdevice, which will do the packet switching
  Ptr<Node> node = openFlowSwitch.Get (0);
  OpenFlowSwitchHelper OFSwHelper;

 // Install controller for OFSw
  Ptr<ns3::ofi::LearningController> controller = CreateObject<ns3::ofi::LearningController>();
  OFSwHelper.Install (node, switchDevice, controller);

  InternetStackHelper internet;
  internet.Install (terminalNode);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (terminalDevice);

  // ip address for the terminal node
  Ptr<Node> n = terminalNode.Get (0);
  Ptr<Ipv4> ipv4 = n->GetObject<Ipv4>();
  Ipv4InterfaceAddress ipv4_int_addr = ipv4->GetAddress (1, 0);
  Ipv4Address ip_addr = ipv4_int_addr.GetLocal ();
  cout << "IP Address: "<< ip_addr << endl;

  // mac address for the switch
  Ptr<NetDevice> s = switchDevice.Get (0);
  cout << "Mac Address: "<< s->GetAddress() << endl;

  // possible fabric manager handler
  s->SetReceiveCallback(MakeCallback (&handlePacketCallback));

  return 0;
}
