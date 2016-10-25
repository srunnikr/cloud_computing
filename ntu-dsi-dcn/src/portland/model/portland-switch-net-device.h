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

/**
 * \defgroup openflow OpenFlow Switch Device
 * This section documents the API of the ns-3 OpenFlow module. For a generic functional description, please refer to the ns-3 manual.
 */

#ifndef PORTLAND_SWITCH_NET_DEVICE_H
#define PORTLAND_SWITCH_NET_DEVICE_H

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4-address.h"

#include "ns3/ethernet-header.h"
#include "ns3/arp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"

#include "ns3/ipv4-l3-protocol.h"
#include "ns3/arp-l3-protocol.h"

#include "ns3/bridge-channel.h"
#include "ns3/node.h"
#include "ns3/enum.h"
#include "ns3/string.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"

#include <map>
#include <set>

#include "portland-interface.h"

namespace ns3 {

/**
 * \ingroup openflow
 * \brief A net device that switches multiple LAN segments via an OpenFlow-compatible flow table
 *
 * The OpenFlowSwitchNetDevice object aggregates multiple netdevices as ports
 * and acts like a switch. It implements OpenFlow-compatibility,
 * according to the OpenFlow Switch Specification v0.8.9
 * <www.openflowswitch.org/documents/openflow-spec-v0.8.9.pdf>.
 * It implements a flow table that all received packets are run through.
 * It implements a connection to a controller via a subclass of the Controller class,
 * which can send messages to manipulate the flow table, thereby manipulating
 * how the OpenFlow switch behaves.
 *
 * There are two controllers available in the original package. DropController
 * builds a flow for each received packet to drop all packets it matches (this
 * demonstrates the flow table's basic implementation), and the LearningController
 * implements a "learning switch" algorithm (see 802.1D), where incoming unicast
 * frames from one port may occasionally be forwarded throughout all other ports,
 * but usually they are forwarded only to a single correct output port.
 *
 * \attention The Spanning Tree Protocol part of 802.1D is not
 * implemented.  Therefore, you have to be careful not to create
 * bridging loops, or else the network will collapse.
 *
 * \attention Each NetDevice used must only be assigned a Mac Address, adding it
 * to an Ipv4 or Ipv6 layer will cause an error. It also must support a SendFrom
 * call.
 */

/*enum PortlandSwitchType {
    EDGE = 1,
    AGGREGATION,
    CORE
};
*/

/**
 * \ingroup openflow 
 * \brief A net device that switches multiple LAN segments via an OpenFlow-compatible flow table
 */
class PortlandSwitchNetDevice : public NetDevice
{
public:
  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Descriptive Data
   * \brief OpenFlowSwitchNetDevice Description Data
   *
   * These four data describe the OpenFlowSwitchNetDevice as if it were
   * a real OpenFlow switch.
   *
   * There is a type of stats request that OpenFlow switches are supposed
   * to handle that returns the description of the OpenFlow switch. Currently
   * manufactured by "The ns-3 team", software description is "Simulated
   * OpenFlow Switch", and the other two are "N/A".
   * @{
   */
  /** \returns The descriptive string. */
  static const char * GetManufacturerDescription ();
  static const char * GetHardwareDescription ();
  static const char * GetSoftwareDescription ();
  static const char * GetSerialNumber ();
  /**@}*/

  PortlandSwitchNetDevice ();
  
  // Constructor -- with device_type, pod and position
  PortlandSwitchNetDevice(pld::PortlandSwitchType& device_type, uint8_t& pod, uint8_t& position);
  virtual ~PortlandSwitchNetDevice ();

  /**
   * \brief Set up the Switch's controller connection.
   *
   * \param c Pointer to a Controller.
   */
  void SetFabricManager (Ptr<pld::FabricManager> c);
  void UpdateFabricManager(Ipv4Address src_ip, Mac48Address src_pmac);
  Mac48Address QueryFabricManager(Ipv4Address dst_ip, Ipv4Address src_ip, Mac48Address src_pmac);
  void ARPFloodFromFabricManager(Ipv4Address dst_ip, Ipv4Address src_ip, Mac48Address src_pmac);

  /**
   * \brief Add a 'port' to a switch device
   *
   * This method adds a new switch port to a OpenFlowSwitchNetDevice, so that
   * the new switch port NetDevice becomes part of the switch and L2
   * frames start being forwarded to/from this NetDevice.
   *
   * \note The netdevice that is being added as switch port must
   * _not_ have an IP address.  In order to add IP connectivity to a
   * bridging node you must enable IP on the OpenFlowSwitchNetDevice itself,
   * never on its port netdevices.
   *
   * \param switchPort The port to add.
   * \return 0 if everything's ok, otherwise an error number.
   * \sa #EXFULL
   */
  int AddSwitchPort (Ptr<NetDevice> switchPort, bool is_upper);
  
  /**
   * \brief Called from the OpenFlow Interface to output the Packet on either a Port or the Controller
   *
   * \param packet_uid Packet UID; used to fetch the packet and its metadata.
   * \param in_port The index of the port the Packet was initially received on.
   * \param max_len The maximum number of bytes the caller wants to be sent; a value of 0 indicates the entire packet should be sent. Used when outputting to controller.
   * \param out_port The port we want to output on.
   * \param ignore_no_fwd If true, Ports that are set to not forward are forced to forward.
   */
  void DoOutput (uint32_t packet_uid, int in_port, size_t max_len, int out_port, bool ignore_no_fwd);

  /**
   * \brief The registered controller calls this method when sending a message to the switch.
   *
   * \param msg The message received from the controller.
   * \param length Length of the message.
   * \return 0 if everything's ok, otherwise an error number.
   */
  int ForwardControlInput (pld::BufferData buffer);

  /**
   * \return Number of switch ports attached to this switch.
   */
  uint32_t GetNSwitchPorts (void) const;

  /**
   * \param p The Port to get the index of.
   * \return The index of the provided Port.
   */
  int GetSwitchPortIndex (pld::Port p);

  /**
   * \param n index of the Port.
   * \return The Port.
   */
  pld::Port GetSwitchPort (uint32_t n) const;

  void SetDeviceType (const pld::PortlandSwitchType device_type);

  pld::PortlandSwitchType GetDeviceType (void);

  void SetPod (const uint8_t pod);

  uint8_t GetPod (void) const;

  void SetPosition (const uint8_t position);

  uint8_t GetPosition (void) const;

  Mac48Address GetSourcePMAC (pld::SwitchPacketMetadata metadata, uint8_t in_port, bool from_upper); 
  Mac48Address GetDestinationPMAC (Ipv4Address dst_ip, Ipv4Address src_ip, Mac48Address src_pmac);

  // From NetDevice
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual void SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom () const;
  virtual Address GetMulticast (Ipv6Address addr) const;

protected:

  class PMACTable
  {
    public:
      void Add(const Mac48Address& pmac, const Mac48Address& amac, const Ipv4Address ip_address, const uint32_t port);
      void Remove(const Mac48Address& amac);
      int FindPort(const Mac48Address& pmac);
      int FindPort(const Ipv4Address& ip_address);
      Mac48Address FindAMAC(const Mac48Address& pmac);
      Mac48Address FindPMAC(const Mac48Address& amac);
      Mac48Address FindPMAC(const Ipv4Address& dst_ip);
      void clear();

    private:
      typedef struct PMACEntry {
        Mac48Address pmac;
        Mac48Address amac;
        Ipv4Address ip_address;
        uint8_t port;
      } PMACEntry;
      
      std::map<Mac48Address, PMACEntry> mapping; // PMAC -> <AMAC, IP, Port> mapping
  };

  virtual void DoDispose (void);

  /**
   * Called when a packet is received on one of the switch's ports.
   *
   * \param netdev The port the packet was received on.
   * \param packet The Packet itself.
   * \param protocol The protocol defining the Packet.
   * \param src The source address of the Packet.
   * \param dst The destination address of the Packet.
   * \param PacketType Type of the packet.
   */
  void ReceiveFromDevice (Ptr<NetDevice> netdev, Ptr<const Packet> packet, uint16_t protocol, const Address& src, const Address& dst, PacketType packetType);

  /**
   * Takes a packet and generates an OpenFlow buffer from it, loading the packet data as well as its headers.
   *
   * \param packet The packet.
   * \param src The source address.
   * \param dst The destination address.
   * \param protocol The protocol defining the packet.
   * \return The OpenFlow Buffer created from the packet.
   */
  pld::SwitchPacketMetadata MetadataFromPacket (Ptr<const Packet> packet, Address src, Address dst, uint16_t protocol);

private:
  /**
   * Sends a copy of the Packet over the provided output port
   *
   * \param packet_uid Packet UID; used to fetch the packet and its metadata.
   */
  void OutputPacket (pld::SwitchPacketMetadata metadata, int out_port, bool is_upper);

  /**
   * Gets the output port index based on the destination PMAC address
   */
  uint8_t GetOutputPort(Mac48Address dst_pmac);
  
  /// Callbacks
  NetDevice::ReceiveCallback m_rxCallback;
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;

  Mac48Address m_address;               ///< Address of this device.
  Ptr<Node> m_node;                     ///< Node this device is installed on.
  Ptr<BridgeChannel> m_channel;         ///< Collection of port channels into the Switch Channel.
  uint32_t m_ifIndex;                   ///< Interface Index
  uint16_t m_mtu;                       ///< Maximum Transmission Unit
  
  /// These are to be set during initialization of the device
  /// Used in PortlandSwitchNetDevice::GetOutputPort
  pld::PortlandSwitchType m_device_type;                  ///< Device type: 3 - Core, 2 - Aggregate, 1 - Edge
  uint8_t m_pod;                          ///< Pod in which the device is located -- valid for only device_type 1 or 2
  uint8_t m_position;                     ///< Position of the device in the pod -- valid for only device_type 1 or 2

  typedef std::map<uint32_t,pld::SwitchPacketMetadata> PacketData_t;
  PacketData_t m_packetData;            ///< Packet data

  typedef std::vector<pld::Port> Ports_t;
  Ports_t m_upper_ports;                      ///< Switch's ports for upper layer connections
  Ports_t m_lower_ports;                      ///< Switch's ports for lower layer connections

  Ptr<pld::FabricManager> m_fabricManager;    ///< Connection to fabric manager.

  uint64_t m_id;                        ///< Unique identifier for this switch, needed for OpenFlow

  // TODO: replace this with PMAC table
  PMACTable m_table;             ///< PMAC Table; AMAC-IP <-> inport.
};

} // namespace ns3

#endif /* PORTLAND_SWITCH_NET_DEVICE_H */
