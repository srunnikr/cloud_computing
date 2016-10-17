#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;
using namespace std;

static void
GenerateTraffic (Ptr<Socket> socket, uint32_t size)
{
  if (size > 0)
    {
      std::ostringstream msg; msg << "Hello from CSE291 " << size << '\0';
      socket->Send (Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length()));
      Simulator::Schedule (Seconds (0.5), &GenerateTraffic, socket, size - 1);
    }
  else
    {
      socket->Close ();
    }
}

static void
SocketPrinter (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address receiver;
  socket->GetSockName (receiver);
  InetSocketAddress receiverAddress = InetSocketAddress::ConvertFrom (receiver);

  Address sender;
  while (packet = socket->RecvFrom (sender))
    {
      // read buffer
      uint8_t *buffer = new uint8_t[packet->GetSize ()];
      packet->CopyData(buffer, packet->GetSize ());
      std::string s = std::string((char*)buffer);

      InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom (sender);

      uint32_t addr_hostorder = senderAddress.GetIpv4().Get();
      cout << "host order val " << addr_hostorder << endl;

      Ipv4Address newAddr = Ipv4Address(addr_hostorder);
      cout << "The address constructed from the host num : " << newAddr << endl;

      //string sendAddr = senderAddress.GetIpv4();
      //cout << sendAddr << endl;

      cout << "Received at=" << receiverAddress.GetIpv4() << ", rx bytes=" << packet->GetSize ()
            << ", payload= " << s << ", sender=" << senderAddress.GetIpv4() << endl;
    }
}

void
RunSimulation (void)
{
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500kb/s"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper internet;
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (nodes.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (i.GetAddress(0, 0), 80);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&SocketPrinter)); // callback

  Ptr<Socket> source = Socket::CreateSocket (nodes.Get (1), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress(0, 0), 80);
  source->Connect (remote);

  // run the test 10 times
  GenerateTraffic (source, 10);
  Simulator::Run ();
  Simulator::Destroy ();
}

int main (int argc, char *argv[])
{
  RunSimulation ();

  return 0;
}
