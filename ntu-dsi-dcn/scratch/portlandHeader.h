#include <iostream>
#include <string>
#include <stdlib.h>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "portlandProtocol.h"
#include "portlandHelper.h"

using namespace ns3;
using namespace std;

class portlandHeader {

	uint32_t packet_size;
	PACKET_TYPE packet_type;
	uint8_t *data;
	map<string, string> IpPmacTable;
	map<string, string>::iterator it;
	portlandHelper helper;

public:

	portlandHeader() {
		packet_size = 0;
		packet_type = PKT_ARP_REQUEST;
		data = NULL;
	}

	void addToTable(string ip, string pmac) {

		IpPmacTable[ip] = pmac;

	}

	string getIpForPmac (string pmac) {

		it = IpPmacTable.begin();
		for (it = IpPmacTable.begin(); it != IpPmacTable.end(); ++it) {
			if (it->second == pmac) {
				return it->first;
			}
		}
		return NULL;

	}

	bool isIpRegistered (string ip) {

		it = IpPmacTable.find(ip);
		return (it == IpPmacTable.end() ? false : true);

	}

	string getPmacForIp (string ip) {

		if (isIpRegistered(ip)) {
			return IpPmacTable[ip];
		} else {
			return NULL;
		}

	}

	bool isPmacRegistered (string pmac) {

		string ip = getIpForPmac(pmac);
		return (ip.compare(NULL) == 0 ? false : true);

	}

	portlandHeader(PACKET_TYPE pktType, uint8_t message[], uint32_t message_len) {

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

	void parsePacket(uint8_t* buff, uint32_t len) {

		// Retrieve packet header
		char pkt_type[4];
		for (uint32_t i = 0; i < 4; ++i) {
			pkt_type[i] = buff[i] + 48;
		}

		// Retrieve packet data
		char data[len - 4];
		for (uint32_t i = 4; i < len; ++i) {
			data[i - 4] = buff[i] + 48;
		}

		string type(pkt_type, 4);
		string message(data, len - 4);

		cout << "Parsed type string : "<< type <<endl;
		cout << "Parsed data string : "<< message << endl;

	}

};
