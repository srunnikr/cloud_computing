#ifndef PORTLAND_PROTOCOL_H
#define PORTLAND_PROTOCOL_H 1

#include <cstring>
#include <iostream>
#include <cstdlib>

// Packet types
enum PACKET_TYPE {
	PKT_MAC_REGISTER = 1,
	PKT_ARP_REQUEST,
	PKT_ARP_RESPONSE,
	PKT_ARP_FLOOD
};

#endif
