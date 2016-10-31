#ifndef PORTLAND_PROTOCOL_H
#define PORTLAND_PROTOCOL_H 1

#include <cstring>
#include <iostream>
#include <cstdlib>

namespace ns3
{

/*
 * Packet types for interaction with Fabric Manager
 */
enum PACKET_TYPE {
	PKT_MAC_REGISTER = 1,
	PKT_ARP_REQUEST,
	PKT_ARP_RESPONSE,
	PKT_ARP_FLOOD
};

/*
 * Portland switch type based on the topology layer it is present in
 */ 
enum PortlandSwitchType {
    EDGE = 1,
    AGGREGATION,
    CORE
  };

}
#endif
