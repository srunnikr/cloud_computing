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
 *
 * Author: Dhruv Sharma  <dhsharma@cs.ucsd.edu>
 */
#ifndef PORTLAND_INTERFACE_H
#define PORTLAND_INTERFACE_H

#include <assert.h>
#include <errno.h>

// Include OFSI code
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/address.h"
#include "ns3/nstime.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4-address.h"
#include "portlandProtocol.h"

#include <set>
#include <map>
#include <limits>

// Include main header
// #include "portland/portland.h"

namespace ns3 {

class PortlandSwitchNetDevice;

namespace pld {
enum PortlandSwitchType {
    EDGE = 1,
    AGGREGATION,
    CORE
  };

/**
 * \brief Port and its metadata.
 */
struct Port
{
  Port () : netdev (0),
            rx_packets (0),
            tx_packets (0),
            rx_bytes (0),
            tx_bytes (0),
            tx_dropped (0)
  {
  }

  Ptr<NetDevice> netdev;
  unsigned long long int rx_packets, tx_packets;
  unsigned long long int rx_bytes, tx_bytes;
  unsigned long long int tx_dropped;
};

/**
 * \brief Packet Metadata, allows us to track the packet's metadata as it passes through the switch.
 */
typedef struct SwitchPacketMetadata
{
  Ptr<Packet> packet;
  uint16_t protocol_number;     ///< Protocol type of the Packet when the Packet is received
  Mac48Address src_amac;        ///< Actual Source MAC Address of the host when the Packet is received
  Mac48Address src_pmac;        ///< Psuedo Source MAC Address of the host
  Mac48Address dst_pmac;        ///< Destination MAC Address of the Packet when the Packet is received.
  Ipv4Address src_ip;           ///< Source IPv4 Address of the Packet when the Packet is received
  Ipv4Address dst_ip;           ///< Destination IPv4 Address of the Packet when the Packet is received.
  bool is_arp_request;          ///< True if it is an ARP Request; False otherwise.
} SwitchPacketMetadata;

// Message information for fabric manager
typedef struct BufferData {
    PACKET_TYPE pkt_type; // PACKET_TYPE enum defined in portlandProtocol.h
    void* message; // Need to cast the message based on the packet type
} BufferData;

typedef struct PMACRegister {
    Ipv4Address hostIP;
    Mac48Address PMACAddress;
} PMACRegister;

typedef struct ARPRequest {
    Ipv4Address destIPAddress;
    Ipv4Address srcIPAddress;
    Mac48Address srcPMACAddress;
} ARPRequest;

typedef struct ARPResponse {
	Ipv4Address destIPAddress;
	Mac48Address destPMACAddress;
	Ipv4Address srcIPAddress;
  Mac48Address srcPMACAddress;
} ARPResponse;

typedef struct ARPFloodRequest {
    Ipv4Address destIPAddress;
    Ipv4Address srcIPAddress;
    Mac48Address srcPMACAddress;
} ARPFloodIPAddress;

/**
 * \brief An interface for a FabricManager of PortlandSwitchNetDevices
 *
 * Follows the Portland specification for a fabric manager.
 */
class FabricManager : public Object {
public:
    static TypeId GetTypeId (void);

    ~FabricManager () {
        m_switches.clear ();
    }

  /**
   * Adds a switch to the fabric manager.
   *
   * \param swtch The switch to register.
   */
   virtual void AddSwitch (Ptr<PortlandSwitchNetDevice> swtch);

  /**
   * A switch calls this method to pass a message on to the Fabric Manager.
   *
   * \param swtch The switch the message was received from.
   * \param buffer The message.
   */
    void ReceiveFromSwitch (Ptr<PortlandSwitchNetDevice> swtch, BufferData buffer);

private:

    // IP - PMAC table and management functions
    std::map<Ipv4Address, Mac48Address> IpPMACTable;
    std::map<Ipv4Address, Mac48Address>::iterator it;

    void addPMACToTable(Ipv4Address ip, Mac48Address pmac);

    Ipv4Address getIPforPMAC(Mac48Address pmac);
    
    bool isIPRegistered(Ipv4Address ip);

    Mac48Address getPMACforIP(Ipv4Address ip);

    bool isPmacRegistered (Mac48Address pmac);

    // Fabric manager function handlers for packet types
    void PMACRegisterHandler(PMACRegister* message);

    void ARPRequestHandler(ARPRequest* message, Ptr<PortlandSwitchNetDevice> swtch);

    void ARPResponseHandler(ARPResponse* message, Ptr<PortlandSwitchNetDevice> swtch);

    // Packets being generated by the fabric manager
    void FloodARPRequest(ARPFloodRequest* message, Ptr<PortlandSwitchNetDevice> swtch);

protected:
  /**
   * \internal
   *
   * However the controller is implemented, this method is to
   * be used to pass a message on to a switch.
   */
   virtual void SendToSwitch (Ptr<PortlandSwitchNetDevice> swtch, BufferData buffer);

  /**
   * \internal
   *
   * Get the packet type on the buffer, which can then be used
   * to determine how to handle the buffer.
   */
    uint8_t GetPacketType (BufferData buffer);

    typedef std::set<Ptr<PortlandSwitchNetDevice> > Switches_t;
    Switches_t m_switches;  ///< The collection of switches registered to this controller.
};

}

}

#endif /* PORTLAND_INTERFACE_H */
