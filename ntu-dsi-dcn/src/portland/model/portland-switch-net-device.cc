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
//#ifdef NS3_PORTLAND

#include <cstdlib>

#include "portland-switch-net-device.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PortlandSwitchNetDevice");

NS_OBJECT_ENSURE_REGISTERED (PortlandSwitchNetDevice);

const char *
PortlandSwitchNetDevice::GetManufacturerDescription ()
{
  return "The UCSD cloud computing team";
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
                   "The identification of the PortlandSwitchNetDevice.",
                   UintegerValue (GenerateId ()),
                   MakeUintegerAccessor (&PortlandFlowSwitchNetDevice::m_id),
                   MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}

// PortlandSwitchNetDevice constructor
PortlandSwitchNetDevice::PortlandSwitchNetDevice (PortlandSwitchType device_type, uint8_t pod, uint8_t position)
  : m_node (0),
    m_ifIndex (0),
    m_mtu (0xffff),
    m_device_type(3),
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
  
  m_device_type = device_type;
  m_pod = pod;
  m_position = position;

  m_upper_ports.reserve (100);
  m_lower_ports.reserve (100);
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

  m_table.clear();

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
SwitchPacketMetadata
PortlandSwitchNetDevice::MetadataFromPacket (Ptr<const Packet> constPacket, Address src, Address dst, uint16_t protocol)
{
  NS_LOG_INFO ("Extracting metadata from packet.");

  Ptr<Packet> packet = constPacket->Copy ();

  pld::SwitchPacketMetadata metadata;

  metadata.packet = packet->Copy();
  metadata.src_amac = Mac48Address.ConvertFrom (src);             // Actual Source Mac Address
  metadata.dst_pmac = Mac48Address.ConvertFrom (dst);             // Destination Psuedo Mac Address
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

  Mac48Address dst_mac = Mac48Address::ConvertFrom (dst);
  Mac48Address src_mac = Mac48Address::ConvertFrom (src);
  NS_LOG_INFO ("Received packet from " << src_mac << " looking for " << dst_mac);

  bool from_upper = false;
  uint8_t in_port;
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

    
    if (packetType == PACKET_HOST && dst_mac == m_address)
      {
        m_rxCallback (this, packet, protocol, src);
      }
    else if (packetType == PACKET_BROADCAST || packetType == PACKET_MULTICAST || packetType == PACKET_OTHERHOST)
      {
        if (packetType == PACKET_OTHERHOST && dst_mac == m_address)
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
              // parse packet
              pld::SwitchPacketMetadata metadata;
              metadata = MetadataFromPacket (packet.Copy (), src, dst, protocol);

              if (!from_upper)
              {
                if (m_device_type == EDGE)
                { 
                  metadata.src_pmac = FindSourcePMAC (metadata, in_port, from_upper);
                  if (metadata.src_pmac == GetBroadcast())
                  {
                    return; // drop packet due to error in finding/allocating PMAC
                  }
                  
                  // Query for dst_pmac based on dst_ip from local table and/or Fabric Manager
                  Mac48Address dst_pmac = GetDestinationPMAC (metadata.dst_ip, metadata.src_ip, metadata.src_pmac);
                  // if dst_pmac is broadcast => failed to find dst_pmac; Fabric Manager will broadcast ARP Request
                  if (dst_pmac == GetBroadcast ())
                  {
                    return; // drop ARP Request here as it will be re-created at CORE switches
                  }
                  
                  metadata.dst_pmac = dst_pmac;
                  out_port = GetOutputPort(metadata.dst_pmac);
                  metadata.packet = packet->Copy ();
                  OutputPacket(metadata, out_port, true);
                }
                else if (m_device_type == AGGREGATION)
                {
                  // basic forwarding
                  out_port = GetOutputPort(metadata.dst_pmac);
                  metadata.packet = packet->Copy ();
                  OutputPacket(metadata, out_port, true);
                }
                else if (m_device_type == CORE)
                {
                  // basic forwarding
                  out_port = GetOutputPort(metadata.dst_pmac);
                  metadata.packet = packet->Copy ();
                  OutputPacket(metadata, out_port, false);
                }
                else
                {
                  // no-op; not valid switch type
                  return;
                }
              }
              else
              {
                if (m_device_type == EDGE)
                { 
                  if (metadata.dst_pmac == GetBroadcast())
                  {
                    for (size_t i = 0; i < m_lower_ports.size(); i++)
                    {
                      metadata.packet = packet->Copy ();
                      OutputPacket(metadata, i, false);
                    }
                  }
                  else
                  {
                    Mac48Address dst_amac = FindAMAC(metadata.dst_pmac);
                    if (dst_pmac == GetBroadcast ())
                    {
                      return; // drop packet due to error (AMAC never seen)
                    }

                    out_port = GetOutputPort(metadata.dst_pmac);
                    metadata.dst_pmac = dst_amac; // re-write dst_amac and forward to port
                    metadata.packet = packet->Copy ();
                    OutputPacket(metadata, out_port, false);
                  }
                }
                else if (m_device_type == AGGREGATION)
                {
                  // basic forwarding
                  out_port = GetOutputPort(metadata.dst_pmac);
                  metadata.packet = packet->Copy ();
                  OutputPacket(metadata, out_port, false);
                }
                else if (m_device_type == CORE)
                {
                  // no-op; as for CORE switch from_upper = false always so this case should never happen
                  return;
                }
                else
                {
                  // no-op; invalid switch type
                  return;
                }
              }
            } 
          }
      }

}

// Function to forward a given packet to the specified out port.
void
PortlandSwitchNetDevice::OutputPacket (SwitchPacketMetadata metadata, int out_port, bool is_upper)
{
  Ports_t ports = (is_upper ? m_upper_ports : m_lower_ports);
  if (out_port >= 0 && out_port < ports.size())
    {
      pld::Port& p = ports[out_port];
      if (p.netdev != 0)
        {
          NS_LOG_INFO ("Sending packet " << packet->GetUid () << " over port " << out_port);
          if (p.netdev->SendFrom (metadata.packet->Copy (), metadata.src_pmac, metadata.dst_pmac, metadata.protocol_number))
            {
              p.tx_packets++;
              p.tx_bytes += metadata.packet->GetSize();
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


BufferData
PortlandSwitchNetDevice::SendBufferToFabricManager(BufferData request_buffer)
{
  BufferData response_buffer;
  if (m_fabricManager != 0)
  {
    m_fabricManager->ReceiveFromSwitch(this, buffer);
  }

  return response_buffer;
}

void
PortlandSwitchNetDevice::ReceiveBufferFromFabricManager(BufferData request_buffer)
{
  if (request_buffer.type == PKT_ARP_FLOOD)
  {
    ARPFloodRequest* msg = (ARPFloodRequest*)buffer.message;
    ARPFloodFromFabricManager(msg->destIPAddress, msg->srcIPAddress, msg->srcPMACAddress);
  }
  else
  {
    // no-op
  }

  free(msg);
  request_buffer.message = NULL;
}

void
PortlandSwitchNetDevice::UpdateFabricManager(Ipv4Address src_ip, Mac48Address src_pmac)
{
  PMACRegister* msg = (PMACRegister *) malloc (sizeof(PMACRegister));
  msg->hostIP = src_ip;
  msg->PMACAddress = src_pmac;
  
  BufferData buffer;
  buffer.type = PKT_MAC_REGISTER;
  buffer.message = msg;

  SendBufferToFabricManager(buffer);
}

Mac48Address
PortlandSwitchNetDevice::QueryFabricManager(Ipv4Address dst_ip, Ipv4Address src_ip, Mac48Address src_pmac)
{
  ARPRequest* msg = (ARPRequest *) malloc (sizeof(ARPRequest));
  msg->srcIPAddress = src_ip;
  msg->srcPMACAddress = src_pmac;
  msg->destIPAddress = dst_ip;
  
  BufferData buffer;
  buffer.type = PKT_ARP_REQUEST;
  buffer.message = msg;

  BufferData response = SendBufferToFabricManager(buffer);
  ARPResponse* arp_resp = (ARPResponse *)(response.message);
  Mac48Address dst_pmac = arp_resp->destPMACAddress;
  
  free(arp_resp);
  response.message = NULL;

  return dst_pmac;
}

void
PortlandSwitchNetDevice::ARPFloodFromFabricManager(Ipv4Address dst_ip, Ipv4Address src_ip, Mac48Address src_pmac)
{
  if (m_device_type == CORE)
  {
    ArpHeader arp;
    Ptr<Packet> packet = Create<Packet> ();
    NS_LOG_LOGIC ("ARP: sending request from node (CORE) "<<m_node->GetId ()<<
                 " || src: " << src_pmac << " / " << src_ip <<
                 " || dst: " << GetBroadcast () << " / " << dst_ip);
    arp.SetRequest (src_pmac.ConvertTo (), src_ip, GetBroadcast (), dst_ip);
    packet->AddHeader (arp);

    SwitchPacketMetadata metadata = MetadataFromPacket (packet.Copy (), src_pmac, GetBroadcast(), ArpL3Protocol::PROT_NUMBER);
    for (size_t i = 0; i < m_lower_ports.size(); i++)
    {
      metadata.packet = packet->Copy ();
      OutputPacket(metadata, i, false);
    }
  }
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
  else if (m_device_type == AGGREGATION)
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
      // random port connected to aggregation layer
      return (rand() % m_upper_ports.size());
    }
  }
  
  // if this device is edge: same pod and position -- downstream, otherwise -- upstream on random port to aggregate
  else if (m_device_type == EDGE)
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
      // random port connected to core layer
      return (rand() % m_upper_ports.size());
    }
  }
}

Mac48Address
PortlandSwitchDevice::GetSourcePMAC (SwitchPacketMetadata metadata, uint8_t in_port, bool from_upper)
{
  // port checking
  if ((from_upper && in_port < m_upper_ports.size()) || (!from_upper && in_port < m_lower_ports.size()))
  {
    NS_LOG_INFO("Illegal in-port" << in_port);
    return;
  }
  
  Mac48Address src_amac = metadata.src_mac;
  Ipv4Address dst_ip = metadata.dst_ip;
  Ipv4Address src_ip = metadata.src_ip;
  Mac48Address src_pmac;
  
  // check for src_PMAC existence
  if (m_table.FindPort(src_ip) != -1)
  {
    src_pmac = m_table.FindPMAC(src_amac);
  }
  // else create src_ip - src_PMAC entry
  else
  {
    uint8_t src_pmac_buffer[6];
    
    // assigning src PMAC address
    src_pmac_buffer[0] = 0;
    src_pmac_buffer[1] = m_pod;
    src_pmac_buffer[2] = m_position;
    src_pmac_buffer[3] = in_port;
    src_pmac_buffer[4] = 0;
    src_pmac_buffer[5] = 1;
    
    src_pmac.CopyFrom(src_pmac_buffer);
    
    // adding entry to the table
    m_table.Add(src_pmac, src_amac, src_ip, in_port);
    
    // register this entry with fabric manager
    UpdateFabricManager(src_ip, src_pmac);
  }
  
  return src_pmac;
}

Mac48Address
PortlandSwitchDevice::GetDestinationPMAC (Ipv4Address dst_ip, Ipv4Address src_ip, Mac48Address src_pmac)
{
  Mac48Address dst_pmac;
  // check locally if dst is connected to same edge switch
  // and has been assigned PMAC already
  if (m_table.FindPort(dst_ip) != -1)
  {
    dst_pmac = m_table.FindPMAC(dst_ip);
  }
  else
  {
    dst_pmac = QueryFabricManager(dst_ip, src_ip, dst_pmac);
  }

  return dst_pmac;
}


//dummy impl
uint32_t
PortlandSwitchNetDevice::GetNSwitchPorts (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports.size ();
}


// dummy impl
pld::Port
PortlandSwitchNetDevice::GetSwitchPort (uint32_t n) const
{
  NS_LOG_FUNCTION_NOARGS ();
  pld::Port port;
  return port;
}

// dummy impl
int
PortlandSwitchNetDevice::GetSwitchPortIndex (ofi::Port p)
{
  /*for (size_t i = 0; i < m_ports.size (); i++)
    {
      if (m_ports[i].netdev == p.netdev)
        {
          return i;
        }
    }
  */
  return -1;
}

void
PortlandSwitchNetDevice::PMACTable::Add(const Mac48Address& pmac, const Mac48Address& amac, const Ipv4Address ip_address, const uint32_t port)
{
  PMACEntry entry;
  entry.pmac = pmac;
  entry.amac = amac;
  entry.ip_address = ip_address;
  entry.port = port;
  mapping.insert(std::make_pair(pmac, entry));
}

void
PortlandSwitchNetDevice::PMACTable::Remove(const Mac48Address& amac)
{
  std::map<Mac48Address, PMACEntry>::iterator it = mapping.begin();
  bool found = false;
  while(it != mapping.end())
  {
    if ((it->second).amac == amac)
    {
      found = true;
      break;
    }
  }

  if (found == true)
  {
    mapping.erase(it);
  }
}

int
PortlandSwitchNetDevice::PMACTable::FindPort(const Mac48Address& pmac) const
{
  uint32_t port;
  auto pair = example.find(pmac);
  if (pair != example.end()) {
    return (pair->second).port;
  }

  return -1;
}

int
PortlandSwitchNetDevice::PMACTable::FindPort(const Ipv4Address& ip_address) const
{
  std::map<Mac48Address, PMACEntry>::iterator it = mapping.begin();
  while(it != mapping.end())
  {
    if ((it->second).ip_address == ip_address)
    {
      return (it->second).port;
    }
  }

  return -1;
}

Mac48Address
PortlandSwitchNetDevice::PMACTable::FindAMAC(const Mac48Address& pmac) const
{
  auto pair = example.find(pmac);
  if (pair != example.end()) {
    return (pair->second).amac;
  }

    return Mac48Address("ff:ff:ff:ff:ff:ff");
}

Mac48Address
PortlandSwitchNetDevice::PMACTable::FindPMAC(const Mac48Address& amac) const
{
  std::map<Mac48Address, PMACEntry>::iterator it = mapping.begin();
  while(it != mapping.end())
  {
    if ((it->second).ip_amac == amac)
    {
      return it->first;
    }
  }

  return Mac48Address("ff:ff:ff:ff:ff:ff");
}



} // namespace ns3

//#endif // NS3_PORTLAND
