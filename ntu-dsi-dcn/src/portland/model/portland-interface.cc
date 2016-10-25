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
#include "portland-switch-net-device.h"
#include "portlandProtocol.h"

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

	swtch->ForwardControlInput(buffer);
}

// Function to extract Packet type/action from the message received from switch
uint8_t
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

void
FabricManager::ReceiveFromSwitch (Ptr<PortlandSwitchNetDevice> swtch, BufferData buffer)
{
  PACKET_TYPE packet_type = GetPacketType(buffer);
  switch (packet_type)
  {
    case PKT_MAC_REGISTER:
    PMACRegister* message = (PMACRegister*) (buffer.message);
    PMACRegisterHandler(message);
    break;

    case PKT_ARP_REQUEST:
    ARPRequest* message = (ARPRequest*) (buffer.message);
    ARPRequestHandler(message, swtch);
    break;

    case PKT_ARP_RESPONSE:
    NS_LOG_COND("PKT_ARP_RESPONSE is to be generated by FM and not consumed");
    break;

    case PKT_ARP_FLOOD: // This is a packet type that the FM should send to core
    NS_LOG_COND("PKT_ARP_FLOOD is to be generated by FM and not consumed");
    break;

    default:
    NS_LOG_COND("Wrong packet type detected : ReceiveFromSwitch Fabric Manager");
    break;
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

  return IPv4Address("255.255.255.255");
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
  if (isIpRegistered(ip))
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

void
FabricManager::PMACRegisterHandler(PMACRegister* message)
{
  addPMACToTable(message->hostIP, message->PMACRegister);
}

void
FabricManager::ARPRequestHandler(ARPRequest* message, Ptr<PortlandSwitchNetDevice> swtch)
{
  if (isIpRegistered(message->destIPAddress))
  {
    // IP address present
    ARPResponse* msg = (ARPResponse*) malloc (sizeof(ARPResponse));
    msg->srcIPAddress = message->srcIPAddress;
    msg->srcPMACAddress = message->srcPMACAddress;
    msg->destIPAddress = message->destIPAddress;
    msg->destPMACAddress = getPMACforIP(message->destIPAddress);
    ARPResponseHandler(msg, swtch);

    // free the message struct
    free(message);
  } else {
    // Miss in the store, flood it to core
    FloodARPRequest((ARPFloodRequest *)message, swtch);
  }
}

void
FabricManager::ARPResponseHandler(ARPResponse* msg, Ptr<PortlandSwitchNetDevice> swtch)
{
  BufferData buffer;
  buffer.pkt_type = PKT_ARP_RESPONSE;
  buffer.message = msg;	

  SendToSwitch(swtch, buffer);
}

void
FabricManager::FloodARPRequest(ARPFloodRequest* msg, Ptr<PortlandSwitchNetDevice> swtch)
{
  BufferData buffer;
  buffer.pkt_type = PKT_ARP_FLOOD;
  buffer.message = msg;

  for (set<Ptr<PortlandSwitchNetDevice>>::iterator it = m_switches.begin(); it != m_switches.end(); it++)
  {
    if (*it->isCore() && *it != swtch)
    {
      SendToSwitch(*it, buffer);
    }
  }
  // no free
}


} // end pld namespace

} // end ns3 namespace

//#endif // NS3_PORTLAND
