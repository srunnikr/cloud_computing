#include <iostream>
#include <string>
#include <stdlib.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;
using namespace std;

class portlandHeader {

	uint32_t packet_size;
	uint32_t packet_type;
	uint8_t *data;

public:
	portlandHeader(uint32_t pktType, uint8_t message[], uint32_t message_len) {
		packet_type = pktType;
		packet_size = message_len + sizeof(packet_type);

		data = (uint8_t *)malloc (packet_size * sizeof(uint8_t));
		memset(data, 0, packet_size);

		if (sizeof(message) > packet_size) {
			NS_LOG_UNCOND("WARNING : Message size exceeds protocol buffer size");
		}

		memcpy(data, &packet_type, sizeof(uint32_t));
		memcpy(data + sizeof(packet_type), message, message_len);

	}

	void printHeader() {

		cout << "portlandHeader: " << endl;
		cout << "packet type : " << packet_type <<endl;
		cout << "Data : ";
		for (uint32_t i=0; i<packet_size; ++i) {
			cout << data[i] <<" ";
		}
		cout <<endl;

	}

	Ptr<Packet> createPacketWithHeader() {

		uint32_t packetsize = packet_size + sizeof(packet_type);
		uint8_t pkt_buff[packetsize];
		memset(pkt_buff, 0, packetsize);
		memcpy(pkt_buff, data, packetsize);	
		Ptr<Packet> pkt = Create<Packet> (pkt_buff, packetsize);
		return pkt; 

	}

	void parsePacket(Ptr<Packet> pkt) {

		uint32_t packetsize = packet_size + sizeof(packet_type);
		uint8_t buff[packetsize];
		memset(data, 0, packetsize);
		pkt->CopyData(buff, pkt->GetSize());
		for (uint32_t i=0; i<packetsize; ++i) {
			cout << buff[i] << " ";
		}
		cout <<endl;

	}

};

static void GenerateTraffic (Ptr<Socket> socket, uint32_t size) {

	if (size > 0) {

		uint8_t data[64];
		memset(data, 0 , 64);
		data[0] = 1;
		data[1] = 2;
		data[2] = 3;

		portlandHeader p(7, data, 64);
		Ptr<Packet> packet = p.createPacketWithHeader();
		socket->Send(packet, packet->GetSize());

//      	std::ostringstream msg; msg << "Hello from CSE291 " << size << '\0';
//      	socket->Send (Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length()));
      	Simulator::Schedule (Seconds (0.5), &GenerateTraffic, socket, size - 1);

	} else {

      	socket->Close ();
    
    }

}

static void SocketPrinter (Ptr<Socket> socket) {

	Ptr<Packet> packet;
	Address receiver;
	socket->GetSockName (receiver);
	InetSocketAddress receiverAddress = InetSocketAddress::ConvertFrom (receiver);

	Address sender;
	while (packet = socket->RecvFrom (sender)) {
	// read buffer
	uint8_t *buffer = new uint8_t[packet->GetSize ()];
	packet->CopyData(buffer, packet->GetSize ());
	std::string s = std::string((char*)buffer);
	
	for (uint32_t i=0; i<packet->GetSize(); ++i) {
		printf("%d ", buffer[i]);
	}

	cout <<endl;

	InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom (sender);

	cout << "Received at=" << receiverAddress.GetIpv4() << ", rx bytes=" << packet->GetSize ()
	    << ", payload= " << s << ", sender=" << senderAddress.GetIpv4() << endl;
	}

}

void RunSimulation (void){
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

int main (int argc, char *argv[]) {
  RunSimulation ();

  return 0;
}
