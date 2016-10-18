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
#include "portland/portland.h"

namespace ns3 {

class PortlandSwitchNetDevice;

namespace pld {

/**
 * \brief Port and its metadata.
 *
 * We need to store port metadata, because OpenFlow dictates that there
 * exists a type of request where the Controller asks for data about a
 * port, or multiple ports. Otherwise, we'd refer to it via Ptr<NetDevice>
 * everywhere.
 */
struct Port
{
  Port () : netdev (0),
            rx_packets (0),
            tx_packets (0),
            rx_bytes (0),
            tx_bytes (0),
            tx_dropped (0),
  {
  }

  Ptr<NetDevice> netdev;
  unsigned long long int rx_packets, tx_packets;
  unsigned long long int rx_bytes, tx_bytes;
  unsigned long long int tx_dropped;
};

/**
 * \brief Class for handling Portland fabric manager actions.
 */
struct Action
{
  /**
   * \param type Type of Portland Action.
   * \return true if the provided type is a type of flow table action.
   */
  static bool IsValidType (ofp_action_type type);

  /**
   * \brief Validates the action on whether its data is valid or not.
   *
   * \param type Type of action to validate.
   * \param len Length of the action data.
   * \param key Matching key for the flow that is tied to this action.
   * \param ah Action's data header.
   * \return ACT_VALIDATION_OK if the action checks out, otherwise an error type.
   */
  static uint16_t Validate (ofp_action_type type, size_t len, const sw_flow_key *key, const ofp_action_header *ah);

  /**
   * \brief Executes the action.
   *
   * \param type Type of action to execute.
   * \param buffer Buffer of the Packet if it's needed for the action.
   * \param key Matching key for the flow that is tied to this action.
   * \param ah Action's data header.
   */
  static void Execute (ofp_action_type type, ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah);
};

/**
 * \brief Packet Metadata, allows us to track the packet's metadata as it passes through the switch.
 */
struct SwitchPacketMetadata
{
  Ptr<Packet> packet; ///< The Packet itself.
  ofpbuf* buffer;               ///< The OpenFlow buffer as created from the Packet, with its data and headers.
  uint16_t protocolNumber;      ///< Protocol type of the Packet when the Packet is received
  Address src;             ///< Source Address of the Packet when the Packet is received
  Address dst;             ///< Destination Address of the Packet when the Packet is received.
};

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
    Ipv4Address requestedIPAddress;
} ARPRequest;

typedef struct ARPResponse {
    Mac48Address responseMACAddress;
} ARPResponse;

typedef struct ARPFloodRequest {
    Ipv4Address ARPFloodIPAddress;
    Ipv4Address ARPSrcIPAddress;
    Mac48Address ARPSrcPMACAddress;
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
        m_learnState.clear ();
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
    void ReceiveFromSwitch (Ptr<PortlandSwitchNetDevice> swtch, ofpbuf* buffer) {
        PACKET_TYPE packet_type = GetPacketType(buffer);
        switch (packet_type) {
            case PKT_MAC_REGISTER:
                PMACRegister* message = (PMACRegister*) (buffer->message;
                FabricManager_PMACRegisterHandler(message);
                break;

            case PKT_ARP_REQUEST:
                ARPRequest* message = (ARPRequest*) (buffer->message);
                FabricManager_ARPRequestHandler(message);
                break;

            case PKT_ARP_RESPONSE:
                ARPResponse* message = (ARPResponse*) (buffer->message);
                FabricManager_ARPResponseHandler(message);
                break;

            case PKT_ARP_FLOOD: // This is a packet type that the FM should send to core
                NS_LOG_COND("PKT_ARP_FLOOD is to be generated by FM and not consumed");
                break;

            default:
                NS_LOG_COND("Wrong packet type detected : ReceiveFromSwitch Fabric Manager");
                break;
        }
    }

private:
    void FabricManager_PMACRegisterHandler(PMACRegister* message) {

    }

    void FabricManager_ARPRequestHandler(ARPRequest* message) {

    }

    void FabricManager_ARPResponseHandler(ARPResponse* message) {

    }

    // Packets being generated by the fabric manager
    void FabricManager_FloodARPRequest(Ipv4Address ARPFloodIPAddress,
                                        Ipv4Address ARPSrcIPAddress,
                                        Mac48Address ARPSrcPMACAddress) {
        
    }

protected:
    Time m_expirationTime;                ///< Time it takes for learned MAC state entry/created flow to expire.
    typedef std::map<Mac48Address, LearnedState> LearnState_t;
    LearnState_t m_learnState;
  /**
   * \internal
   *
   * However the controller is implemented, this method is to
   * be used to pass a message on to a switch.
   *
   * \param swtch The switch to receive the message.
   * \param msg The message to send.
   * \param length The length of the message.
   */
   virtual void SendToSwitch (Ptr<PortlandSwitchNetDevice> swtch, void * msg, size_t length);

  /**
   * \internal
   *
   * Construct flow data from a matching key to build a flow
   * entry for adding, modifying, or deleting a flow.
   *
   * \param key The matching key data; used to create a flow that matches the packet.
   * \param buffer_id The OpenFlow Buffer ID; used to run the actions on the packet if we add or modify the flow.
   * \param command Whether to add, modify, or delete this flow.
   * \param acts List of actions to execute.
   * \param actions_len Length of the actions buffer.
   * \param idle_timeout Flow expires if left inactive for this amount of time (specify OFP_FLOW_PERMANENT to disable feature).
   * \param hard_timeout Flow expires after this amount of time (specify OFP_FLOW_PERMANENT to disable feature).
   * \return Flow data that when passed to SetFlow will add, modify, or delete a flow it defines.
   */
  ofp_flow_mod* BuildFlow (sw_flow_key key, uint32_t buffer_id, uint16_t command, void* acts, size_t actions_len, int idle_timeout, int hard_timeout);

  /**
   * \internal
   *
   * Get the packet type on the buffer, which can then be used
   * to determine how to handle the buffer.
   *
   * \param buffer The packet in OpenFlow buffer format.
   * \return The packet type, as defined in the ofp_type struct.
   */
    uint8_t GetPacketType (ofpbuf* buffer);

    typedef std::set<Ptr<PortlandSwitchNetDevice> > Switches_t;
    Switches_t m_switches;  ///< The collection of switches registered to this controller.

    struct LearnedState {
        uint32_t port;                      ///< Learned port.
    };
};

/**
 * \brief Executes a list of flow table actions.
 *
 * \param swtch OpenFlowSwitchNetDevice these actions are being executed on.
 * \param packet_uid Packet UID; used to fetch the packet and its metadata.
 * \param buffer The Packet OpenFlow buffer.
 * \param key The matching key for the flow tied to this list of actions.
 * \param actions A buffer of actions.
 * \param actions_len Length of actions buffer.
 * \param ignore_no_fwd If true, during port forwarding actions, ports that are set to not forward are forced to forward.
 */
void ExecuteActions (Ptr<OpenFlowSwitchNetDevice> swtch, uint64_t packet_uid, ofpbuf* buffer, sw_flow_key *key, const ofp_action_header *actions, size_t actions_len, int ignore_no_fwd);

/**
 * \brief Validates a list of flow table actions.
 *
 * \param key The matching key for the flow tied to this list of actions.
 * \param actions A buffer of actions.
 * \param actions_len Length of actions buffer.
 * \return If the action list validates, ACT_VALIDATION_OK is returned. Otherwise, a code for the OFPET_BAD_ACTION error type is returned.
 */
uint16_t ValidateActions (const sw_flow_key *key, const ofp_action_header *actions, size_t actions_len);

}

}

#endif /* PORTLAND_INTERFACE_H */
