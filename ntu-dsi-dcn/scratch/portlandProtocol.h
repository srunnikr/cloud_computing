#include <string.h>
#include <iostream>
#include <stdlib.h>

// Packet types
enum PACKET_TYPE {
	PKT_ARP_REQUEST = 1,
	PKT_ARP_RESPONSE,
	PKT_MAC_REGISTER
};