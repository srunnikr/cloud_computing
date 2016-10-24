/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Blake Hurd
 *
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

#include "portland-switch-helper.h"
#include "ns3/log.h"
#include "ns3/portland-switch-net-device.h"
#include "ns3/portland-interface.h"
#include "ns3/node.h"
#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("PortlandSwitchHelper");

namespace ns3 {

PortlandSwitchHelper::PortlandSwitchHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.SetTypeId ("ns3::PortlandSwitchNetDevice");
}

void
PortlandSwitchHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.Set (n1, v1);
}

NetDeviceContainer
PortlandSwitchHelper::Install (Ptr<Node> node, NetDeviceContainer c, Ptr<ns3::pld::FabricManager> fabric_manager)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("**** Install switch device on node " << node->GetId ());

  NetDeviceContainer devs;
  Ptr<PortlandSwitchNetDevice> dev = m_deviceFactory.Create<PortlandSwitchNetDevice> ();
  devs.Add (dev);
  node->AddDevice (dev);

  NS_LOG_INFO ("**** Set up Fabric Manager");
  dev->SetFabricManager (fabric_manager);

  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      NS_LOG_INFO ("**** Add SwitchPort " << *i);
      dev->AddSwitchPort (*i);
    }
  return devs;
}

NetDeviceContainer
PortlandSwitchHelper::Install (Ptr<Node> node, NetDeviceContainer c, Ptr<ns3::pld::FabricManager> fabric_manager, 
                                pld::PortlandSwitchType device_type, uint8_t pod, uint8_t position)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("**** Install switch device on node " << node->GetId ());

  NetDeviceContainer devs;
  Ptr<PortlandSwitchNetDevice> dev = m_deviceFactory.Create<PortlandSwitchNetDevice> (device_type, pod, position);
  devs.Add (dev);
  node->AddDevice (dev);

  NS_LOG_INFO ("**** Set up Fabric Manager");
  dev->SetFabricManager (fabric_manager);

  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      NS_LOG_INFO ("**** Add SwitchPort " << *i);
      dev->AddSwitchPort (*i);
    }
  return devs;
}

NetDeviceContainer
PortlandSwitchHelper::Install (Ptr<Node> node, NetDeviceContainer c)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("**** Install switch device on node " << node->GetId ());

  NetDeviceContainer devs;
  Ptr<PortlandSwitchNetDevice> dev = m_deviceFactory.Create<PortlandSwitchNetDevice> ();
  devs.Add (dev);
  node->AddDevice (dev);

  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      NS_LOG_INFO ("**** Add SwitchPort " << *i);
      dev->AddSwitchPort (*i);
    }
  return devs;
}

NetDeviceContainer
PortlandSwitchHelper::Install (std::string nodeName, NetDeviceContainer c)
{
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node, c);
}

} // namespace ns3

//#endif // NS3_PORTLAND
