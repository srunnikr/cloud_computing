#ifndef PORTLAND_PROTOCOL_H
#define PORTLAND_PROTOCOL_H

#include <string.h>
#include <iostream>
#include <stdlib.h>

// Packet types
enum PACKET_TYPE {
	PKT_MAC_REGISTER = 1,
	PKT_ARP_REQUEST,
	PKT_ARP_RESPONSE,
	PKT_ARP_FLOOD
};

#endif