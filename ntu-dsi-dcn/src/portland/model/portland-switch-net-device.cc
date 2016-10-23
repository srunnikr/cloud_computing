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
#ifdef NS3_PORTLAND

#include "portland-switch-net-device.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PortlandSwitchNetDevice");

NS_OBJECT_ENSURE_REGISTERED (PortlandSwitchNetDevice);

const char *
PortlandSwitchNetDevice::GetManufacturerDescription ()
{
  return "The ns-3 team";
}

const char *
PortlandSwitchNetDevice::GetHardwareDescription ()
{
  return "N/A";
}

const char *
PortlandSwitchNetDevice::GetSoftwareDescription ()
{
  return "Simulated Portland Switch";
}

const char *
PortlandSwitchNetDevice::GetSerialNumber ()
{
  return "N/A";
}

static uint64_t
GenerateId ()
{
  uint8_t ea[ETH_ADDR_LEN];
  eth_addr_random (ea);
  return eth_addr_to_uint64 (ea);
}

TypeId
PortlandSwitchNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PortlandSwitchNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("Portland")
    .AddConstructor<PortlandSwitchNetDevice> ()
    .AddAttribute ("ID",
                   "The identification of the PortlandSwitchNetDevice/Datapath, needed for OpenFlow compatibility.",
                   UintegerValue (GenerateId ()),
                   MakeUintegerAccessor (&PortlandFlowSwitchNetDevice::m_id),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("FlowTableLookupDelay",
                   "A real switch will have an overhead for looking up in the flow table. For the default, we simulate a standard TCAM on an FPGA.",
                   TimeValue (NanoSeconds (30)),
                   MakeTimeAccessor (&PortlandSwitchNetDevice::m_lookupDelay),
                   MakeTimeChecker ())
    .AddAttribute ("Flags", // Note: The Controller can configure this value, overriding the user's setting.
                   "Flags to turn different functionality on/off, such as whether to inform the controller when a flow expires, or how to handle fragments.",
                   UintegerValue (0), // Look at the ofp_config_flags enum in openflow/include/openflow.h for options.
                   MakeUintegerAccessor (&PortlandSwitchNetDevice::m_flags),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("FlowTableMissSendLength", // Note: The Controller can configure this value, overriding the user's setting.
                   "When forwarding a packet the switch didn't match up to the controller, it can be more efficient to forward only the first x bytes.",
                   UintegerValue (OFP_DEFAULT_MISS_SEND_LEN), // 128 bytes
                   MakeUintegerAccessor (&PortlandSwitchNetDevice::m_missSendLen),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

// Portland NetDevice constructor
PortlandSwitchNetDevice::PortlandSwitchNetDevice ()
  : m_node (0),
    m_ifIndex (0),
    m_mtu (0xffff),
    m_device_type(1),
    m_pod(0),
    m_position(0),
    m_packetData(),
    m_upper_ports(),
    m_lower_ports(),
    m_fabricManager(0),
    m_lookupDelay(0),
    m_table()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_channel = CreateObject<BridgeChannel> ();

  m_fabricManager = 0;

  m_upper_ports.reserve (DP_MAX_PORTS);
  m_lower_ports.reserve (DP_MAX_PORTS);
}

PortlandSwitchNetDevice::~PortlandSwitchNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

// PortlandSwitchNetDevice cleanup function
void
PortlandSwitchNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (Ports_t::iterator b = m_upper_ports.begin (), e = m_upper_ports.end (); b != e; b++)
    {
      b->netdev = 0;
    }
  m_upper_ports.clear ();
  for (Ports_t::iterator b = m_lower_ports.begin (), e = m_lower_ports.end (); b != e; b++)
    {
      b->netdev = 0;
    }
  m_lower_ports.clear ();

  m_fabricManager = 0;

  chain_destroy (m_chain);

  m_channel = 0;
  m_node = 0;
  NetDevice::DoDispose ();
}

// Add the FabricManager to the PortlandSwitchNetDevice
void
PortlandSwitchNetDevice::SetFabricManager (Ptr<pld::FabricManager> fm)
{
  if (m_fabricManager != 0)
    {
      NS_LOG_ERROR ("Fabric Manager already set.");
      return;
    }

  m_fabricManager = fm;
  m_fabricManager->AddSwitch (this);
}

// Registers other NetDevices/NICs (eg. CsmaNetDevice) on the switch with this  PortlandSwitchNetDevice as switch ports,
// and register the receive callback function with those devices.
int
PortlandSwitchNetDevice::AddSwitchPort (Ptr<NetDevice> switchPort, bool is_upper)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (switchPort != this);
  if (!Mac48Address::IsMatchingType (switchPort->GetAddress ()))
    {
      NS_FATAL_ERROR ("Device does not support eui 48 addresses: cannot be added to switch.");
    }
  if (!switchPort->SupportsSendFrom ())
    {
      NS_FATAL_ERROR ("Device does not support SendFrom: cannot be added to switch.");
    }
  if (m_address == Mac48Address ())
    {
      m_address = Mac48Address::ConvertFrom (switchPort->GetAddress ());
    }

  if (m_ports.size () < DP_MAX_PORTS)
    {
      pld::Port p;
      p.netdev = switchPort;
      if (is_upper)
      {
        m_upper_ports.push_back (p);
      } 
      else
      {
        m_lower_ports.push_back (p);
      }

      NS_LOG_DEBUG ("RegisterProtocolHandler for " << switchPort->GetInstanceTypeId ().GetName ());
      m_node->RegisterProtocolHandler (MakeCallback (&PortlandSwitchNetDevice::ReceiveFromDevice, this),
                                       0, switchPort, true);
      m_channel->AddChannel (switchPort->GetChannel ());
    }
  else
    {
      return EXFULL;
    }

  return 0;
}

void
PortlandSwitchNetDevice::SetDeviceType (const PortlandSwitchType device_type)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_device_type = device_type;
}

PortlandSwitchType
PortlandSwitchNetDevice::GetDeviceType (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_device_type;
}

void
PortlandSwitchNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ifIndex = index;
}

uint32_t
PortlandSwitchNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel>
PortlandSwitchNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_channel;
}

void
PortlandSwitchNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_address = Mac48Address::ConvertFrom (address);
}

Address
PortlandSwitchNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool
PortlandSwitchNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_mtu = mtu;
  return true;
}

uint16_t
PortlandSwitchNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}


bool
PortlandSwitchNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


void
PortlandSwitchNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
}

bool
PortlandSwitchNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
PortlandSwitchNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
PortlandSwitchNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
PortlandSwitchNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
  return multicast;
}


bool
PortlandSwitchNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool
PortlandSwitchNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

// Function to send the packet out through one of the regular switch ports or if undecided, forward to fabric manager
void
PortlandSwitchNetDevice::DoOutput (uint32_t packet_uid, int in_port, size_t max_len, int out_port, bool ignore_no_fwd)
{
  // outport = -1 indicates no outport found so forward to controller
  if (out_port != -1)
    {
      OutputPort (packet_uid, in_port, out_port, ignore_no_fwd);
    }
  else
    {
      OutputControl(srcIP, srcPMAC, destIP, action);
    }
}

bool
PortlandSwitchNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  return SendFrom (packet, m_address, dest, protocolNumber);
}

// Receives packet from higher layer application (if any) and prepares to forward it. (No changes required here)
bool
PortlandSwitchNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();

  ofpbuf *buffer = BufferFromPacket (packet, src, dest, GetMtu (), protocolNumber);

  uint32_t packet_uid = save_buffer (buffer);
  ofi::SwitchPacketMetadata data;
  data.packet = packet;
  data.buffer = buffer;
  data.protocolNumber = protocolNumber;
  data.src = Address (src);
  data.dst = Address (dest);
  m_packetData.insert (std::make_pair (packet_uid, data));

  RunThroughPMACTable (packet_uid, -1);

  return true;
}


Ptr<Node>
PortlandSwitchNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void
PortlandSwitchNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = node;
}

bool
PortlandSwitchNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
PortlandSwitchNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void
PortlandSwitchNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool
PortlandSwitchNetDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
PortlandSwitchNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

// Function to attract packet fields from the packet received and return a buffer with all necessary fields

PortlandSwitchNetDevice::MetadataFromPacket (Ptr<const Packet> constPacket, Address src, Address dst, int mtu, uint16_t protocol)
{
  NS_LOG_INFO ("Extracting metadata from packet.");

  Ptr<Packet> packet = constPacket->Copy ();

  pld::SwitchPacketMetadata metadata;
  metadata.src = src;              // Destination Mac Address
  metadata.dst = dst;              // Source Mac Address
  metadata.protocolNumber = protocol;
  if (protocol != ArpL3Protocol::PROT_NUMBER && protocol != Ipv4L3Protocol::PROT_NUMBER)
  {
    NS_LOG_WARN ("Protocol unsupported: " << protocol);
  }

  NS_LOG_INFO ("Parsed EthernetHeader");

  // We have to wrap this because PeekHeader has an assert fail if we check for an Ipv4Header that isn't there.
  if (protocol == Ipv4L3Protocol::PROT_NUMBER)
    {
      // IPv4 Packet
      Ipv4Header ip_hd;
      if (packet->PeekHeader (ip_hd))
        {
          metadata.src_ip = ip_hd.GetSource ();         // Source IP Address
          metadata.dst_ip = ip_hd.GetDestination ();    // Destination IP Address
          metadata.is_arp_request = false;              // This is not an ARP packet

          NS_LOG_INFO ("Parsed Ipv4Header");
          packet->RemoveHeader (ip_hd);
        }
    }
  else
    {
      // ARP Packet
      ArpHeader arp_hd;
      if (packet->PeekHeader (arp_hd))
        {
          metadata.src_ip = arp_hd.GetSourceIpv4Address ();       // Source IP Address
          metadata.dst_ip = arp_hd.GetDestinationIpv4Address ();  // Destination IP Address
          metadata.is_arp_request = arp_hd.IsRequest ();          // If it is an ARP Request
          
          NS_LOG_INFO ("Parsed ArpHeader");
          packet->RemoveHeader (arp_hd);
        }
    }

  return metadata;
}

// Actual callback function called when a packet is received on a switch port (i.e. netdev here)
void
PortlandSwitchNetDevice::ReceiveFromDevice (Ptr<NetDevice> netdev, Ptr<const Packet> packet, uint16_t protocol,
                                            const Address& src, const Address& dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("--------------------------------------------");
  NS_LOG_DEBUG ("UID is " << packet->GetUid ());

  if (!m_promiscRxCallback.IsNull ())
    {
      m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
    }

  Mac48Address dst48 = Mac48Address::ConvertFrom (dst);
  NS_LOG_INFO ("Received packet from " << Mac48Address::ConvertFrom (src) << " looking for " << dst48);

  bool from_upper = false;
  uint32_t in_port = -1;
  for (size_t i = 0; i < m_lower_ports.size (); i++)
    {
      if (m_lower_ports[i].netdev == netdev)
        {
          from_upper = false;
          in_port = i;
          m_lower_ports[i].rx_packets++;
          m_lower_ports[i].rx_bytes += packet.GetSize();
        }
    }

  for (size_t i = 0; i < m_upper_ports.size (); i++)
    {
      if (m_upper_ports[i].netdev == netdev)
        {
          from_upper = true;
          in_port = i;
          m_upper_ports[i].rx_packets++;
          m_upper_ports[i].rx_bytes += packet.GetSize();
        }
    }

    
    if (packetType == PACKET_HOST && dst48 == m_address)
      {
        m_rxCallback (this, packet, protocol, src);
      }
    else if (packetType == PACKET_BROADCAST || packetType == PACKET_MULTICAST || packetType == PACKET_OTHERHOST)
      {
        if (packetType == PACKET_OTHERHOST && dst48 == m_address)
          {
            m_rxCallback (this, packet, protocol, src);
          }
        else
          {
            if (packetType != PACKET_OTHERHOST)
              {
                m_rxCallback (this, packet, protocol, src);
              }
            else
            {
              if (protocol == ArpL3Protocol::PROT_NUMBER)
              {
                // parse packet
                pld::SwitchPacketMetadata metadata;
                metadata = MetadataFromPacket (packet.Copy (), src, dst, netdev->GetMtu (), protocol);

                RunThroughPMACTable (metadata, in_port);
              }
              else
              {
                // call PMAC-based forwarding logic
              }
            }

           
          }
      }

}

// Function to flood a given packet on all except incoming port.
int
PortlandSwitchNetDevice::OutputAll (uint32_t packet_uid, int in_port, bool flood)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("Flooding over ports.");

  int prev_port = -1;
  for (size_t i = 0; i < m_ports.size (); i++)
    {
      if (i == (unsigned)in_port) // Originating port
        {
          continue;
        }
      if (flood && m_ports[i].config & OFPPC_NO_FLOOD) // Port configured to not allow flooding
        {
          continue;
        }
      if (prev_port != -1)
        {
          OutputPort (packet_uid, in_port, prev_port, false);
        }
      prev_port = i;
    }
  if (prev_port != -1)
    {
      OutputPort (packet_uid, in_port, prev_port, false);
    }

  return 0;
}

// Function to forward a given packet to the specified out port.
void
PortlandSwitchNetDevice::OutputPacket (uint32_t packet_uid, int out_port)
{
  if (out_port >= 0 && out_port < DP_MAX_PORTS)
    {
      ofi::Port& p = m_ports[out_port];
      if (p.netdev != 0 && !(p.config & OFPPC_PORT_DOWN))
        {
          ofi::SwitchPacketMetadata data = m_packetData.find (packet_uid)->second;
          size_t bufsize = data.buffer->size;
          NS_LOG_INFO ("Sending packet " << data.packet->GetUid () << " over port " << out_port);
          if (p.netdev->SendFrom (data.packet->Copy (), data.src, data.dst, data.protocolNumber))
            {
              p.tx_packets++;
              p.tx_bytes += bufsize;
            }
          else
            {
              p.tx_dropped++;
            }
          return;
        }
    }

  NS_LOG_DEBUG ("can't forward to bad port " << out_port);
}

//Function similar to previous one but with unconditional forwarding (ignore_no_fwd) to an out port.
void
PortlandSwitchNetDevice::OutputPort (uint32_t packet_uid, int in_port, int out_port, bool ignore_no_fwd)
{
  NS_LOG_FUNCTION_NOARGS ();

  // TODO: use this for flooding ARP if fabric manager doesn't hold a mapping
  if (out_port == OFPP_FLOOD)
    {
      OutputAll (packet_uid, in_port, true);
    }
  else if (out_port == OFPP_ALL)
    {
      OutputAll (packet_uid, in_port, false);
    }
  else if (out_port == FABRIC_MANAGER_PORT)
    {
      OutputControl(srcIP, srcPMAC, destIP, action);
    }
  else if (out_port == OFPP_IN_PORT)
    {
      OutputPacket (packet_uid, in_port);
    }
  else if (out_port == OFPP_TABLE)
    {
      RunThroughPMACTable (packet_uid, in_port < DP_MAX_PORTS ? in_port : -1, false);
    }
  else if (in_port == out_port)
    {
      NS_LOG_DEBUG ("can't directly forward to input port");
    }
  else
    {
      OutputPacket (packet_uid, out_port);
    }
}

// Function to prepare a buffer containing reply to  controller/fabric manager message
void*
PortlandSwitchNetDevice::MakeOpenflowReply (size_t openflow_len, uint8_t type, ofpbuf **bufferp)
{
  return make_openflow_xid (openflow_len, type, 0, bufferp);
}

// Function to send a buffer containing a message to the fabric manager
int
PortlandSwitchNetDevice::SendOpenflowBuffer (ofpbuf *buffer)
{
  if (m_fabricManager != 0)
    {
      update_openflow_length (buffer);
      m_fabricManager->ReceiveFromSwitch (this, buffer);
    }

  return 0;
}

// Function to forward packet info to the fabric manager
void
PortlandSwitchNetDevice::OutputControl (Ipv4Address srcIP, Mac48Address srcPMAC,
			Ipv4Address destIP, /* reqd only if action == PKT_ARP_REQUEST */
			PACKET_TYPE action)
{
	BufferData buffer;
	NS_LOG_INFO ("Sending arp_request packet to fabric manager");

	switch(action) {
		case PKT_MAC_REGISTER:
			PMACRegister *msg = (PMACRegister *) malloc (sizeof(PMACRegister));
			msg->hostIP = srcIP;
			msg->PMACAddress = srcPMAC;

			buffer.type = action;
			buffer.message = msg;
			SendOpenflowBuffer (buffer);
			break;

		case PKT_ARP_REQUEST:
			ARPRequest* msg = (ARPRequest *) malloc (sizeof(ARPRequest));
			msg->srcIPAddress = srcIP;
			msg->srcPMACAddress = srcPMAC;
			msg->destIPAddress = destIP;
			
			buffer.type = action;
			buffer.message = msg;
			SendOpenflowBuffer (buffer);
			break;

		case PKT_ARP_RESPONSE:
		case PKT_ARP_FLOOD:
		default:
			NS_LOG_COND("Invalid action requested from FM");

	}
}

// useless for now; can be used for understanding buffer handing etc.
void
PortlandSwitchNetDevice::FillPortDesc (ofi::Port p, ofp_phy_port *desc)
{
  desc->port_no = htons (GetSwitchPortIndex (p));

  std::ostringstream nm;
  nm << "eth" << GetSwitchPortIndex (p);
  strncpy ((char *)desc->name, nm.str ().c_str (), sizeof desc->name);

  p.netdev->GetAddress ().CopyTo (desc->hw_addr);
  desc->config = htonl (p.config);
  desc->state = htonl (p.state);

  /// \todo This should probably be fixed eventually to specify different available features.
  desc->curr = 0; // htonl(netdev_get_features(p->netdev, NETDEV_FEAT_CURRENT));
  desc->supported = 0; // htonl(netdev_get_features(p->netdev, NETDEV_FEAT_SUPPORTED));
  desc->advertised = 0; // htonl(netdev_get_features(p->netdev, NETDEV_FEAT_ADVERTISED));
  desc->peer = 0; // htonl(netdev_get_features(p->netdev, NETDEV_FEAT_PEER));
}

// useless for now; can be used to understand buffer handling
void
PortlandSwitchNetDevice::SendFlowExpired (sw_flow *flow, enum ofp_flow_expired_reason reason)
{
  ofpbuf *buffer;
  ofp_flow_expired *ofe = (ofp_flow_expired*)MakeOpenflowReply (sizeof *ofe, OFPT_FLOW_EXPIRED, &buffer);
  flow_fill_match (&ofe->match, &flow->key);

  ofe->priority = htons (flow->priority);
  ofe->reason = reason;
  memset (ofe->pad, 0, sizeof ofe->pad);

  ofe->duration     = htonl (time_now () - flow->created);
  memset (ofe->pad2, 0, sizeof ofe->pad2);
  ofe->packet_count = htonll (flow->packet_count);
  ofe->byte_count   = htonll (flow->byte_count);
  SendOpenflowBuffer (buffer);
}

// useless for now;
void
PortlandSwitchNetDevice::SendErrorMsg (uint16_t type, uint16_t code, const void *data, size_t len)
{
  ofpbuf *buffer;
  ofp_error_msg *oem = (ofp_error_msg*)MakeOpenflowReply (sizeof(*oem) + len, OFPT_ERROR, &buffer);
  oem->type = htons (type);
  oem->code = htons (code);
  memcpy (oem->data, data, len);
  SendOpenflowBuffer (buffer);
}

// Function that gets the output port index based on destination PMAC address
// Each device has k ports; first half are south-bound, next half are north-bound
// Device type and position are assumed to be known
uint8_t
PortlandSwitchNetDevice::GetOutputPort(Mac48Address dst_pmac)
{   
  // dst_mac_buffer contains the six octets of the destination MAC address
  uint8_t dst_pmac_buffer[6];
  dst_pmac.CopyTo(dst_pmac_buffer);
  uint16_t dst_pod = ((uint16_t) dst_pmac_buffer[0]) << 8 | dst_pmac_buffer[1];
  
  // if this device is core, downstream to dst_pod
  if (m_device_type == CORE)
  {
    return dst_pod;
  }
  
  // if this device is aggregate: same pod -- downstream, otherwise -- upstream on random port to core
  if (m_device_type == AGGREGATION)
  {
    // same pod -- downstream
    if (m_pod == dst_pod)
    {
      // return the dst_position -- this is the port connected to edge
      return dst_pmac_buffer[2];
    }
    // different pod -- upstream
    else
    {
      // random between k / 2 and k
      return (k / 2) + rand() % (k / 2);
    }
  }
  
  // if this device is edge: same pod and position -- downstream, otherwise -- upstream on random port to aggregate
  if (m_device_type == EDGE)
  { 
    // same pod and position
    if (m_pod == dst_pod && m_position == dst_mac_buffer[2])
    { 
      // return the dst port -- this is the port connected to the dst host 
      return dst_pmac_buffer[3];
    }
    // upstream
    else
    {
      // random between k / 2 and k
      return (k / 2) + rand() % (k / 2);
    }
  }
}

// Function to lookup matching host fields in PMAC table return corresponding PMAC or assign PMAC and update fabric manager
void
PortlandSwitchNetDevice::PMACTableLookup (sw_flow_key key, ofpbuf* buffer, uint32_t packet_uid, int port, bool send_to_fabric_manager)
{
  sw_flow *flow = chain_lookup (m_chain, &key);
  if (flow != 0)
    {
      NS_LOG_INFO ("Flow matched");
      flow_used (flow, buffer);
      ofi::ExecuteActions (this, packet_uid, buffer, &key, flow->sf_acts->actions, flow->sf_acts->actions_len, false);
    }
  else
    {
      NS_LOG_INFO ("Flow not matched.");

      if (send_to_fabric_manager)
        {
          OutputControl (srcIP, srcPMAC, destIP, action);
        }
    }

  // Clean up; at this point we're done with the packet.
  m_packetData.erase (packet_uid);
  discard_buffer (packet_uid);
  ofpbuf_delete (buffer);
}

void
PortlandSwitchDevice::RunThroughPMACTable (SwitchPacketMetadata metadata, int port)
{
  // port checking
  if (port > (k / 2))
  {
    NS_LOG_INFO("Illegal port");
    return;
  }
  
  Mac48Address src_amac = Mac48Address.ConvertFrom(metadata.src_mac);
  Ipv4Address dst_ip = metadata.dst_ip;
  Ipv4Address src_ip = metadata.src_ip;
  Mac48Address src_pmac;
  
  // check for src_PMAC existence
  if (m_table.ContainsIP(src_ip))
  {
    src_pmac = m_table.FindPMAC(src_ip);
  }
  // else create src_ip - src_PMAC entry
  else
  {
    uint8_t src_pmac_buffer[6];
    
    // assigning src PMAC address
    src_pmac_buffer[0] = 0;
    src_pmac_buffer[1] = m_pod;
    src_pmac_buffer[2] = m_position;
    src_pmac_buffer[3] = port;
    src_pmac_buffer[4] = 0;
    src_pmac_buffer[5] = 1;
    
    src_pmac.CopyFrom(src_pmac_buffer);
    
    // adding entry to the table
    m_table.Add(src_amac, src_pmac, port);
    
    // register this entry with fabric manager
    OutputControl(src_ip, src_pmac, 0, PKT_MAC_REGISTER);
  }
  
  // if the edge switch has this IP-PMAC entry
  // send ARP response to the host
  if (m_table.ContainsIP(dst_ip))
  {
    Mac48Address dst_pmac = m_table.FindPMAC(dst_ip); 
    ArpHeader arp;
 		arp.SetReply (dst_pmac, dst_ip, src_pmac, src_ip);
	  Ptr<Packet> packet = Create<Packet> ();
	  packet->AddHeader(arp);
	  Send (packet, src_pmac, ArpL3Protocol::PROT_NUMBER);
  }
  // else send ARP request to the fabric manager
  else
  {
    OutputControl(src_ip, src_pmac, dst_ip, PKT_ARP_REQUEST);
  }
}

void
PortlandSwitchNetDevice::RunThroughPMACTable (uint32_t packet_uid, int port, bool send_to_fabric_manager)
{
  ofi::SwitchPacketMetadata data = m_packetData.find (packet_uid)->second;
  ofpbuf* buffer = data.buffer;

  sw_flow_key key;
  key.wildcards = 0; // Lookup cannot take wildcards.
  // Extract the matching key's flow data from the packet's headers; if the policy is to drop fragments and the message is a fragment, drop it.
  if (flow_extract (buffer, port != -1 ? port : OFPP_NONE, &key.flow) && (m_flags & OFPC_FRAG_MASK) == OFPC_FRAG_DROP)
    {
      ofpbuf_delete (buffer);
      return;
    }

  // drop MPLS packets with TTL 1
  if (buffer->l2_5)
    {
      mpls_header mpls_h;
      mpls_h.value = ntohl (*((uint32_t*)buffer->l2_5));
      if (mpls_h.ttl == 1)
        {
          // increment mpls drop counter
          if (port != -1)
            {
              m_ports[port].mpls_ttl0_dropped++;
            }
          return;
        }
    }

  // If we received the packet on a port, and opted not to receive any messages from it...
  if (port != -1)
    {
      uint32_t config = m_ports[port].config;
      if (config & (OFPPC_NO_RECV | OFPPC_NO_RECV_STP)
          && config & (!eth_addr_equals (key.flow.dl_dst, stp_eth_addr) ? OFPPC_NO_RECV : OFPPC_NO_RECV_STP))
        {
          return;
        }
    }

  NS_LOG_INFO ("Matching against the PMAC table.");
  Simulator::Schedule (m_lookupDelay, &PortlandSwitchNetDevice::PMACTableLookup, this, key, buffer, packet_uid, port, send_to_fabric_manager);
}

// incoming packet/action from fabric manager (Eg: a flood ARP-Req to core switch)
int
PortlandSwitchNetDevice::ReceivePacketOut (const void *msg)
{
  const ofp_packet_out *opo = (ofp_packet_out*)msg;
  ofpbuf *buffer;
  size_t actions_len = ntohs (opo->actions_len);

  if (actions_len > (ntohs (opo->header.length) - sizeof *opo))
    {
      NS_LOG_DEBUG ("message too short for number of actions");
      return -EINVAL;
    }

  if (ntohl (opo->buffer_id) == (uint32_t) -1)
    {
      // FIXME: can we avoid copying data here?
      int data_len = ntohs (opo->header.length) - sizeof *opo - actions_len;
      buffer = ofpbuf_new (data_len);
      ofpbuf_put (buffer, (uint8_t *)opo->actions + actions_len, data_len);
    }
  else
    {
      buffer = retrieve_buffer (ntohl (opo->buffer_id));
      if (buffer == 0)
        {
          return -ESRCH;
        }
    }

  sw_flow_key key;
  flow_extract (buffer, ntohs(opo->in_port), &key.flow); // ntohs(opo->in_port)

  uint16_t v_code = ofi::ValidateActions (&key, opo->actions, actions_len);
  if (v_code != ACT_VALIDATION_OK)
    {
      SendErrorMsg (OFPET_BAD_ACTION, v_code, msg, ntohs (opo->header.length));
      ofpbuf_delete (buffer);
      return -EINVAL;
    }

  ofi::ExecuteActions (this, opo->buffer_id, buffer, &key, opo->actions, actions_len, true);
  return 0;
}

// useless for now; mostly to understand code
int
PortlandSwitchNetDevice::ReceivePortMod (const void *msg)
{
  ofp_port_mod* opm = (ofp_port_mod*)msg;

  int port = opm->port_no; // ntohs(opm->port_no);
  if (port < DP_MAX_PORTS)
    {
      ofi::Port& p = m_ports[port];

      // Make sure the port id hasn't changed since this was sent
      Mac48Address hw_addr = Mac48Address ();
      hw_addr.CopyFrom (opm->hw_addr);
      if (p.netdev->GetAddress () != hw_addr)
        {
          return 0;
        }

      if (opm->mask)
        {
          uint32_t config_mask = ntohl (opm->mask);
          p.config &= ~config_mask;
          p.config |= ntohl (opm->config) & config_mask;
        }

      if (opm->mask & htonl (OFPPC_PORT_DOWN))
        {
          if ((opm->config & htonl (OFPPC_PORT_DOWN)) && (p.config & OFPPC_PORT_DOWN) == 0)
            {
              p.config |= OFPPC_PORT_DOWN;
              /// \todo Possibly disable the Port's Net Device via the appropriate interface.
            }
          else if ((opm->config & htonl (OFPPC_PORT_DOWN)) == 0 && (p.config & OFPPC_PORT_DOWN))
            {
              p.config &= ~OFPPC_PORT_DOWN;
              /// \todo Possibly enable the Port's Net Device via the appropriate interface.
            }
        }
    }

  return 0;
}

//useless for now
int
PortlandSwitchNetDevice::AddFlow (const ofp_flow_mod *ofm)
{
  size_t actions_len = ntohs (ofm->header.length) - sizeof *ofm;

  // Allocate memory.
  sw_flow *flow = flow_alloc (actions_len);
  if (flow == 0)
    {
      if (ntohl (ofm->buffer_id) != (uint32_t) -1)
        {
          discard_buffer (ntohl (ofm->buffer_id));
        }
      return -ENOMEM;
    }

  flow_extract_match (&flow->key, &ofm->match);

  uint16_t v_code = ofi::ValidateActions (&flow->key, ofm->actions, actions_len);
  if (v_code != ACT_VALIDATION_OK)
    {
      SendErrorMsg (OFPET_BAD_ACTION, v_code, ofm, ntohs (ofm->header.length));
      flow_free (flow);
      if (ntohl (ofm->buffer_id) != (uint32_t) -1)
        {
          discard_buffer (ntohl (ofm->buffer_id));
        }
      return -ENOMEM;
    }

  // Fill out flow.
  flow->priority = flow->key.wildcards ? ntohs (ofm->priority) : -1;
  flow->idle_timeout = ntohs (ofm->idle_timeout);
  flow->hard_timeout = ntohs (ofm->hard_timeout);
  flow->used = flow->created = time_now ();
  flow->sf_acts->actions_len = actions_len;
  flow->byte_count = 0;
  flow->packet_count = 0;
  memcpy (flow->sf_acts->actions, ofm->actions, actions_len);

  // Act.
  int error = chain_insert (m_chain, flow);
  if (error)
    {
      if (error == -ENOBUFS)
        {
          SendErrorMsg (OFPET_FLOW_MOD_FAILED, OFPFMFC_ALL_TABLES_FULL, ofm, ntohs (ofm->header.length));
        }
      flow_free (flow);
      if (ntohl (ofm->buffer_id) != (uint32_t) -1)
        {
          discard_buffer (ntohl (ofm->buffer_id));
        }
      return error;
    }

  NS_LOG_INFO ("Added new flow.");
  if (ntohl (ofm->buffer_id) != std::numeric_limits<uint32_t>::max ())
    {
      ofpbuf *buffer = retrieve_buffer (ofm->buffer_id); // ntohl(ofm->buffer_id)
      if (buffer)
        {
          sw_flow_key key;
          flow_used (flow, buffer);
          flow_extract (buffer, ntohs(ofm->match.in_port), &key.flow); // ntohs(ofm->match.in_port);
          ofi::ExecuteActions (this, ofm->buffer_id, buffer, &key, ofm->actions, actions_len, false);
          ofpbuf_delete (buffer);
        }
      else
        {
          return -ESRCH;
        }
    }
  return 0;
}

// useless for now
int
PortlandSwitchNetDevice::ModFlow (const ofp_flow_mod *ofm)
{
  sw_flow_key key;
  flow_extract_match (&key, &ofm->match);

  size_t actions_len = ntohs (ofm->header.length) - sizeof *ofm;

  uint16_t v_code = ofi::ValidateActions (&key, ofm->actions, actions_len);
  if (v_code != ACT_VALIDATION_OK)
    {
      SendErrorMsg ((ofp_error_type)OFPET_BAD_ACTION, v_code, ofm, ntohs (ofm->header.length));
      if (ntohl (ofm->buffer_id) != (uint32_t) -1)
        {
          discard_buffer (ntohl (ofm->buffer_id));
        }
      return -ENOMEM;
    }

  uint16_t priority = key.wildcards ? ntohs (ofm->priority) : -1;
  int strict = (ofm->command == htons (OFPFC_MODIFY_STRICT)) ? 1 : 0;
  chain_modify (m_chain, &key, priority, strict, ofm->actions, actions_len);

  if (ntohl (ofm->buffer_id) != std::numeric_limits<uint32_t>::max ())
    {
      ofpbuf *buffer = retrieve_buffer (ofm->buffer_id); // ntohl (ofm->buffer_id)
      if (buffer)
        {
          sw_flow_key skb_key;
          flow_extract (buffer, ntohs(ofm->match.in_port), &skb_key.flow); // ntohs(ofm->match.in_port);
          ofi::ExecuteActions (this, ofm->buffer_id, buffer, &skb_key, ofm->actions, actions_len, false);
          ofpbuf_delete (buffer);
        }
      else
        {
          return -ESRCH;
        }
    }
  return 0;
}

// useless for now
int
PortlandSwitchNetDevice::ReceiveFlow (const void *msg)
{
  NS_LOG_FUNCTION_NOARGS ();
  const ofp_flow_mod *ofm = (ofp_flow_mod*)msg;
  uint16_t command = ntohs (ofm->command);

  if (command == OFPFC_ADD)
    {
      return AddFlow (ofm);
    }
  else if ((command == OFPFC_MODIFY) || (command == OFPFC_MODIFY_STRICT))
    {
      return ModFlow (ofm);
    }
  else if (command == OFPFC_DELETE)
    {
      sw_flow_key key;
      flow_extract_match (&key, &ofm->match);
      return chain_delete (m_chain, &key, ofm->out_port, 0, 0) ? 0 : -ESRCH;
    }
  else if (command == OFPFC_DELETE_STRICT)
    {
      sw_flow_key key;
      uint16_t priority;
      flow_extract_match (&key, &ofm->match);
      priority = key.wildcards ? ntohs (ofm->priority) : -1;
      return chain_delete (m_chain, &key, ofm->out_port, priority, 1) ? 0 : -ESRCH;
    }
  else
    {
      return -ENODEV;
    }
}

// useless for now
int
PortlandSwitchNetDevice::ReceiveEchoRequest (const void *oh)
{
  return SendOpenflowBuffer (make_echo_reply ((ofp_header*)oh));
}

// useless for now
int
PortlandSwitchNetDevice::ReceiveEchoReply (const void *oh)
{
  return 0;
}

// Main handler for incoming messages from controller; calls specialized body parsers depending on type of message
int
PortlandSwitchNetDevice::ForwardControlInput (BufferData buffer)
{
	int error = 0;
	assert(buffer.message != NULL);

	// Figure out how to handle it.
	switch (buffer.pkt_type) {
    case PKT_ARP_RESPONSE:
		ARPResponse* msg = (ARPResponse*)buffer.message;

		// marshall arp_response packet and send to src_pmac
		if (m_device_type == EDGE) {
			ArpHeader arp;
			arp.SetReply (msg->destPMACAddress, msg->destIPAddress,
						  msg->srcPMACAddress, msg->srcIPAddress);

			Ptr<Packet> packet = Create<Packet> ();
			packet->AddHeader(arp);
			this.Send(packet, msg->srcPMACAddress, ArpL3Protocol::PROT_NUMBER);
		} else {
			// non-edge switches shouldn't receive ARPRequest from FM.
			error = -EINVAL;
		}

		break;

    case PKT_ARP_FLOOD:
		ARPFloodRequest* msg = (ARPFloodRequest*)buffer.message;

		// marshall arp_request packet and flood downstream
		if (m_device_type == CORE) {
			ARPHeader arp;
			arp.SetRequest (msg->srcPMACAddess, msg->srcIPAddess,
							GetBroadcast (), msg->destIPAddess);

			packet->AddHeader (arp);
			this.Send(packet, GetBroadcast(), ArpL3Protocol::PROT_NUMBER);
		} else {
			// non-core switches shouldn't recieve flood request from FM.
			error = -EINVAL;
		}

		break;

    case PKT_ARP_REGISTER:
    case PKT_ARP_REQUEST:
	default :
		NS_LOG_DEBUG ("Incorrect message type received from FM");
		error = -EINVAL;
    }

	if (buffer.message != NULL) {
		free (buffer.message);
    }
	return error;
}

uint32_t
PortlandSwitchNetDevice::GetNSwitchPorts (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports.size ();
}

pld::Port
PortlandSwitchNetDevice::GetSwitchPort (uint32_t n) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports[n];
}

int
PortlandSwitchNetDevice::GetSwitchPortIndex (ofi::Port p)
{
  for (size_t i = 0; i < m_ports.size (); i++)
    {
      if (m_ports[i].netdev == p.netdev)
        {
          return i;
        }
    }
  return -1;
}

void
PortlandSwitchNetDevice::PMACTable::Add(const Mac48Address& amac, const Mac48Address& pmac, uint32_t port)
{
  port_mapping.insert(std::make_pair(pmac, std::make_pair(amac, port)));
  mac_mapping.insert(std::make_pair(amac, pmac));
}

void
PortlandSwitchNetDevice::PMACTable::Remove(const Mac48Address& amac)
{
  mac_mapping.erase(amac);
  std::map<Mac48Address, std::pair<Mac48Address, uint32_t>>::iterator it = port_mapping.begin();
  bool found = false;
  while(it != port_mapping.end())
  {
    if ((it->second)->first == amac)
    {
      found = true;
      break;
    }
  }

  if (found == true)
  {
    port_mapping.erase(it);
  }
}

uint32_t
PortlandSwitchNetDevice::PMACTable::FindPort(const Mac48Address& pmac)
{
  return port_mapping[pmac]->second;
}

Mac48Address
PortlandSwitchNetDevice::PMACTable::FindAMAC(const Mac48Address& pmac)
{
  return port_mapping[pmac]->first;
}

Mac48Address
PortlandSwitchNetDevice::PMACTable::FindPMAC(const Mac48Address& amac)
{
  return mac_mapping[amac];
}



} // namespace ns3

#endif // NS3_PORTLAND
