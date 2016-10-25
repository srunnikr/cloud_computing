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
#ifndef PORTLAND_SWITCH_HELPER_H
#define PORTLAND_SWITCH_HELPER_H

#include "ns3/ipv4-address.h"
#include "ns3/portland-interface.h"
#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"
#include <string>

namespace ns3 {

/*enum PortlandSwitchType {
    EDGE = 1,
    AGGREGATION,
    CORE
  };*/

class Node;
class AttributeValue;

/**
 * \brief Add capability to switch multiple LAN segments (IEEE 802.1D bridging)
 */
class PortlandSwitchHelper
{
public:
  /*
   * Construct a PortlandSwitchHelper
   */
  PortlandSwitchHelper ();

  /**
   * Set an attribute on each ns3::PortlandSwitchNetDevice created by
   * PortlandSwitchHelper::Install
   *
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   */
  void
  SetDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * This method creates an ns3::PortlandSwitchNetDevice with the attributes
   * configured by PortlandSwitchHelper::SetDeviceAttribute, adds the device
   * to the node, attaches the given NetDevices as ports of the
   * switch, and sets up a controller connection using the provided
   * Controller.
   *
   * \param node The node to install the device in
   * \param c Container of NetDevices to add as switch ports
   * \param controller The controller connection.
   * \returns A container holding the added net device.
   */
  NetDeviceContainer
  Install (Ptr<Node> node, NetDeviceContainer c, Ptr<ns3::pld::FabricManager> fabric_manager);
  
  NetDeviceContainer
  Install (Ptr<Node> node, NetDeviceContainer lowerDevices, NetDeviceContainer upperDevices, Ptr<ns3::pld::FabricManager> fabric_manager,
	pld::PortlandSwitchType device_type, uint8_t pod, uint8_t position);
  
  /**
   * This method creates an ns3::PortlandSwitchNetDevice with the attributes
   * configured by PortlandSwitchHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * switch.
   *
   * \param node The node to install the device in
   * \param c Container of NetDevices to add as switch ports
   * \returns A container holding the added net device.
   */
  NetDeviceContainer
  Install (Ptr<Node> node, NetDeviceContainer c);

  /**
   * This method creates an ns3::PortlandSwitchNetDevice with the attributes
   * configured by PortlandSwitchHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * switch.
   *
   * \param nodeName The name of the node to install the device in
   * \param c Container of NetDevices to add as switch ports
   * \returns A container holding the added net device.
   */
  NetDeviceContainer
  Install (std::string nodeName, NetDeviceContainer c);

private:
  ObjectFactory m_deviceFactory;
};

} // namespace ns3

#endif /* PORTLAND_SWITCH_HELPER_H */
