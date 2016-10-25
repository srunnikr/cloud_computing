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

#include "portland-interface.h"

namespace ns3 {

namespace pld {

NS_LOG_COMPONENT_DEFINE ("PortlandInterface");


// Registers a PortlandSwitchNetDevice as a switch with the fabric manager.
void
FabricManager::AddSwitch (Ptr<PortlandSwitchNetDevice> swtch)
{
  if (m_switches.find (swtch) != m_switches.end ())
    {
      NS_LOG_INFO ("This Fabric Manager has already registered this switch!");
    }
  else
    {
      m_switches.insert (swtch);
    }
}

// Function to send a message/action to the switch (Eg: flood ARP-Req sent to core switch)
void
FabricManager::SendToSwitch (Ptr<PortlandSwitchNetDevice> swtch, BufferData buffer)
{
	if (m_switches.find (swtch) == m_switches.end ()) {
		NS_LOG_ERROR ("Can't send to this switch, not registered to the Fabric Manager.");
		return;
	}

	//swtch->ForwardControlInput(buffer);
}

// Function to extract Packet type/action from the message received from switch
PACKET_TYPE
FabricManager::GetPacketType (BufferData buffer)
{
	return buffer.pkt_type;
}

// generic function (no change required)
TypeId
FabricManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::pld::FabricManager")
    .AddConstructor<FabricManager> ()
    //.SetParent (Object)
  ;
  return tid;
}

BufferData
FabricManager::ReceiveFromSwitch (Ptr<PortlandSwitchNetDevice> swtch, BufferData buffer)
{
  PACKET_TYPE packet_type = GetPacketType(buffer);
  switch (packet_type)
  {
    case PKT_MAC_REGISTER:
    {
      pld::PMACRegister* message = (pld::PMACRegister*) (buffer.message);
      return PMACRegisterHandler(message);
    }

    case PKT_ARP_REQUEST:
    {
      pld::ARPRequest* message = (pld::ARPRequest*) (buffer.message);
      return ARPRequestHandler(message, swtch);
    }

    default:
    {
      NS_LOG_UNCOND("Wrong packet type detected : ReceiveFromSwitch Fabric Manager");
      BufferData buffer;
      return buffer;
    }
  }
}

void
FabricManager::addPMACToTable (Ipv4Address ip, Mac48Address pmac)
{
  IpPMACTable[ip] = pmac;
}


Ipv4Address
FabricManager::getIPforPMAC (Mac48Address pmac)
{
  it = IpPMACTable.begin();
  for (it = IpPMACTable.begin(); it != IpPMACTable.end(); ++it)
  {
    if (it->second == pmac)
    {
      return it->first;
    }
  }

  return Ipv4Address("255.255.255.255");
}

bool
FabricManager::isIPRegistered (Ipv4Address ip)
{
  it = IpPMACTable.find(ip);
  return (it == IpPMACTable.end() ? false : true);
}

Mac48Address
FabricManager::getPMACforIP (Ipv4Address ip)
{
  if (FabricManager::isIPRegistered(ip))
  {
    return IpPMACTable[ip];
  }
  else
  {
    return Mac48Address("ff:ff:ff:ff:ff:ff");
  }
}

bool
FabricManager::isPmacRegistered (Mac48Address pmac)
{
  Ipv4Address ip = getIPforPMAC(pmac);
  return (ip == Ipv4Address("255.255.255.255") ? false : true);
}

BufferData
FabricManager::PMACRegisterHandler(pld::PMACRegister* message)
{
  BufferData response_buffer;
  addPMACToTable(message->hostIP, message->PMACAddress);
  free(message);
  return response_buffer;
}

BufferData
FabricManager::ARPRequestHandler(pld::ARPRequest* message, Ptr<PortlandSwitchNetDevice> swtch)
{
  if (FabricManager::isIPRegistered(message->destIPAddress))
  {
    // IP address present
    ARPResponse* msg = (pld::ARPResponse*) malloc (sizeof(ARPResponse));
    msg->srcIPAddress = message->srcIPAddress;
    msg->srcPMACAddress = message->srcPMACAddress;
    msg->destIPAddress = message->destIPAddress;
    msg->destPMACAddress = getPMACforIP(message->destIPAddress);
    return ARPResponseHandler(msg, swtch);
  } else {
    // Miss in the store, flood it to core
    FloodARPRequest((pld::ARPFloodRequest *)message, swtch);
    ARPResponse* msg = (pld::ARPResponse*) malloc (sizeof(ARPResponse));
    msg->srcIPAddress = message->srcIPAddress;
    msg->srcPMACAddress = message->srcPMACAddress;
    msg->destIPAddress = message->destIPAddress;
    msg->destPMACAddress = Mac48Address("ff:ff:ff:ff:ff:ff");
    return ARPResponseHandler(msg, swtch);
  }
  
  free(message);
}

BufferData
FabricManager::ARPResponseHandler(pld::ARPResponse* msg, Ptr<PortlandSwitchNetDevice> swtch)
{
  BufferData buffer;
  buffer.pkt_type = PKT_ARP_RESPONSE;
  buffer.message = msg;	

  return buffer;
  //SendToSwitch(swtch, buffer);
}

void
FabricManager::FloodARPRequest(pld::ARPFloodRequest* msg, Ptr<PortlandSwitchNetDevice> swtch)
{
  BufferData buffer;
  buffer.pkt_type = PKT_ARP_FLOOD;
  buffer.message = msg;

  for (std::set<Ptr<PortlandSwitchNetDevice> >::iterator it = m_switches.begin(); it != m_switches.end(); it++)
  {
    if ( (*it)->GetDeviceType () == CORE && (*it) != swtch)
    {
      (*it)->ReceiveBufferFromFabricManager(buffer);
    }
  }
  // no free
}


} // end pld namespace

} // end ns3 namespace

//#endif // NS3_PORTLAND
