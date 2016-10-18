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

#include "portland-interface.h"
#include "portland-switch-net-device.h"

namespace ns3 {

namespace pld {

NS_LOG_COMPONENT_DEFINE ("PortlandInterface");

bool
Action::IsValidType (ofp_action_type type)
{
  switch (type)
    {
    case OFPAT_OUTPUT:
    case OFPAT_SET_VLAN_VID:
    case OFPAT_SET_VLAN_PCP:
    case OFPAT_STRIP_VLAN:
    case OFPAT_SET_DL_SRC:
    case OFPAT_SET_DL_DST:
    case OFPAT_SET_NW_SRC:
    case OFPAT_SET_NW_DST:
    case OFPAT_SET_TP_SRC:
    case OFPAT_SET_TP_DST:
    case OFPAT_SET_MPLS_LABEL:
    case OFPAT_SET_MPLS_EXP:
      return true;
    default:
      return false;
    }
}

uint16_t
Action::Validate (ofp_action_type type, size_t len, const sw_flow_key *key, const ofp_action_header *ah)
{
  size_t size = 0;

  switch (type)
    {
    case OFPAT_OUTPUT:
      {
        if (len != sizeof(ofp_action_output))
          {
            return OFPBAC_BAD_LEN;
          }

        ofp_action_output *oa = (ofp_action_output *)ah;

        // To prevent loops, make sure there's no action to send to the OFP_TABLE virtual port.

        // port is now 32-bit
        if (oa->port == OFPP_NONE || oa->port == key->flow.in_port) // htonl(OFPP_NONE);
          { // if (oa->port == htons(OFPP_NONE) || oa->port == key->flow.in_port)
            return OFPBAC_BAD_OUT_PORT;
          }

        return ACT_VALIDATION_OK;
      }
    case OFPAT_SET_VLAN_VID:
      size = sizeof(ofp_action_vlan_vid);
      break;
    case OFPAT_SET_VLAN_PCP:
      size = sizeof(ofp_action_vlan_pcp);
      break;
    case OFPAT_STRIP_VLAN:
      size = sizeof(ofp_action_header);
      break;
    case OFPAT_SET_DL_SRC:
    case OFPAT_SET_DL_DST:
      size = sizeof(ofp_action_dl_addr);
      break;
    case OFPAT_SET_NW_SRC:
    case OFPAT_SET_NW_DST:
      size = sizeof(ofp_action_nw_addr);
      break;
    case OFPAT_SET_TP_SRC:
    case OFPAT_SET_TP_DST:
      size = sizeof(ofp_action_tp_port);
      break;
    case OFPAT_SET_MPLS_LABEL:
      size = sizeof(ofp_action_mpls_label);
      break;
    case OFPAT_SET_MPLS_EXP:
      size = sizeof(ofp_action_mpls_exp);
      break;
    default:
      break;
    }

  if (len != size)
    {
      return OFPBAC_BAD_LEN;
    }
  return ACT_VALIDATION_OK;
}

void
Action::Execute (ofp_action_type type, ofpbuf *buffer, sw_flow_key *key, const ofp_action_header *ah)
{
  switch (type)
    {
    case OFPAT_OUTPUT:
      break;
    case OFPAT_SET_VLAN_VID:
      set_vlan_vid (buffer, key, ah);
      break;
    case OFPAT_SET_VLAN_PCP:
      set_vlan_pcp (buffer, key, ah);
      break;
    case OFPAT_STRIP_VLAN:
      strip_vlan (buffer, key, ah);
      break;
    case OFPAT_SET_DL_SRC:
    case OFPAT_SET_DL_DST:
      set_dl_addr (buffer, key, ah);
      break;
    case OFPAT_SET_NW_SRC:
    case OFPAT_SET_NW_DST:
      set_nw_addr (buffer, key, ah);
      break;
    case OFPAT_SET_TP_SRC:
    case OFPAT_SET_TP_DST:
      set_tp_port (buffer, key, ah);
      break;
    case OFPAT_SET_MPLS_LABEL:
      set_mpls_label (buffer, key, ah);
      break;
    case OFPAT_SET_MPLS_EXP:
      set_mpls_exp (buffer, key, ah);
      break;
    default:
      break;
    }
}

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

void
FabricManager::SendToSwitch (Ptr<PortlandSwitchNetDevice> swtch, void * msg, size_t length)
{
  if (m_switches.find (swtch) == m_switches.end ())
    {
      NS_LOG_ERROR ("Can't send to this switch, not registered to the Fabric Manager.");
      return;
    }

  swtch->ForwardControlInput (msg, length);
}

ofp_flow_mod*
FabricManager::BuildFlow (sw_flow_key key, uint32_t buffer_id, uint16_t command, void* acts, size_t actions_len, int idle_timeout, int hard_timeout)
{
  ofp_flow_mod* ofm = (ofp_flow_mod*)malloc (sizeof(ofp_flow_mod) + actions_len);
  ofm->header.version = OFP_VERSION;
  ofm->header.type = OFPT_FLOW_MOD;
  ofm->header.length = htons (sizeof(ofp_flow_mod) + actions_len);
  ofm->command = htons (command);
  ofm->idle_timeout = htons (idle_timeout);
  ofm->hard_timeout = htons (hard_timeout);
  ofm->buffer_id = htonl (buffer_id);
  ofm->priority = OFP_DEFAULT_PRIORITY;
  memcpy (ofm->actions,acts,actions_len);

  ofm->match.wildcards = key.wildcards;                                 // Wildcard fields
  ofm->match.in_port = key.flow.in_port;                                // Input switch port
  memcpy (ofm->match.dl_src, key.flow.dl_src, sizeof ofm->match.dl_src); // Ethernet source address.
  memcpy (ofm->match.dl_dst, key.flow.dl_dst, sizeof ofm->match.dl_dst); // Ethernet destination address.
  ofm->match.dl_vlan = key.flow.dl_vlan;                                // Input VLAN OFP_VLAN_NONE;
  ofm->match.dl_type = key.flow.dl_type;                                // Ethernet frame type ETH_TYPE_IP;
  ofm->match.nw_proto = key.flow.nw_proto;                              // IP Protocol
  ofm->match.nw_src = key.flow.nw_src;                                  // IP source address
  ofm->match.nw_dst = key.flow.nw_dst;                                  // IP destination address
  ofm->match.tp_src = key.flow.tp_src;                                  // TCP/UDP source port
  ofm->match.tp_dst = key.flow.tp_dst;                                  // TCP/UDP destination port
  ofm->match.mpls_label1 = key.flow.mpls_label1;                        // Top of label stack htonl(MPLS_INVALID_LABEL);
  ofm->match.mpls_label2 = key.flow.mpls_label1;                        // Second label (if available) htonl(MPLS_INVALID_LABEL);

  return ofm;
}

uint8_t
FabricManager::GetPacketType (ofpbuf* buffer)
{
  ofp_header* hdr = (ofp_header*)ofpbuf_try_pull (buffer, sizeof (ofp_header));
  uint8_t type = hdr->type;
  ofpbuf_push_uninit (buffer, sizeof (ofp_header));
  return type;
}

TypeId FabricManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::pld::FabricManager")
    .SetParent (Object)
    .AddConstructor<FabricManager> ()
    .AddAttribute ("ExpirationTime",
                   "Time it takes for learned MAC state entry to expire.",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&FabricManager::m_expirationTime),
                   MakeTimeChecker ())
  ;
  return tid;
}

void
FabricManager::ReceiveFromSwitch (Ptr<PortlandSwitchNetDevice> swtch, ofpbuf* buffer)
{
  if (m_switches.find (swtch) == m_switches.end ())
    {
      NS_LOG_ERROR ("Can't receive from this switch, not registered to the Fabric Manager.");
      return;
    }

  // We have received any packet at this point, so we pull the header to figure out what type of packet we're handling.
  uint8_t type = GetPacketType (buffer);

  if (type == OFPT_PACKET_IN) // The switch didn't understand the packet it received, so it forwarded it to the fabric manager.
    {
      ofp_packet_in * opi = (ofp_packet_in*)ofpbuf_try_pull (buffer, offsetof (ofp_packet_in, data));
      int port = ntohs (opi->in_port);

      // Create matching key.
      sw_flow_key key;
      key.wildcards = 0;
      flow_extract (buffer, port != -1 ? port : OFPP_NONE, &key.flow);

      uint16_t out_port = OFPP_FLOOD;
      uint16_t in_port = ntohs (key.flow.in_port);

      // If the destination address is learned to a specific port, find it.
      Mac48Address dst_addr;
      dst_addr.CopyFrom (key.flow.dl_dst);
      if (!dst_addr.IsBroadcast ())
        {
          LearnState_t::iterator st = m_learnState.find (dst_addr);
          if (st != m_learnState.end ())
            {
              out_port = st->second.port;
            }
          else
            {
              NS_LOG_INFO ("Setting to flood; don't know yet what port " << dst_addr << " is connected to");
            }
        }
      else
        {
          NS_LOG_INFO ("Setting to flood; this packet is a broadcast");
        }

      // Create output-to-port action
      ofp_action_output x[1];
      x[0].type = htons (OFPAT_OUTPUT);
      x[0].len = htons (sizeof(ofp_action_output));
      x[0].port = out_port;

      // Create a new flow that outputs matched packets to a learned port, OFPP_FLOOD if there's no learned port.
      ofp_flow_mod* ofm = BuildFlow (key, opi->buffer_id, OFPFC_ADD, x, sizeof(x), OFP_FLOW_PERMANENT, m_expirationTime.IsZero () ? OFP_FLOW_PERMANENT : m_expirationTime.GetSeconds ());
      SendToSwitch (swtch, ofm, ofm->header.length);

      // We can learn a specific port for the source address for future use.
      Mac48Address src_addr;
      src_addr.CopyFrom (key.flow.dl_src);
      LearnState_t::iterator st = m_learnState.find (src_addr);
      if (st == m_learnState.end ()) // We haven't learned our source MAC yet.
        {
          LearnedState ls;
          ls.port = in_port;
          m_learnState.insert (std::make_pair (src_addr,ls));
          NS_LOG_INFO ("Learned that " << src_addr << " can be found over port " << in_port);

          // Learn src_addr goes to a certain port.
          ofp_action_output x2[1];
          x2[0].type = htons (OFPAT_OUTPUT);
          x2[0].len = htons (sizeof(ofp_action_output));
          x2[0].port = in_port;

          // Switch MAC Addresses and ports to the flow we're modifying
          src_addr.CopyTo (key.flow.dl_dst);
          dst_addr.CopyTo (key.flow.dl_src);
          key.flow.in_port = out_port;
          ofp_flow_mod* ofm2 = BuildFlow (key, -1, OFPFC_MODIFY, x2, sizeof(x2), OFP_FLOW_PERMANENT, m_expirationTime.IsZero () ? OFP_FLOW_PERMANENT : m_expirationTime.GetSeconds ());
          SendToSwitch (swtch, ofm2, ofm2->header.length);
        }
    }
}

void
ExecuteActions (Ptr<OpenFlowSwitchNetDevice> swtch, uint64_t packet_uid, ofpbuf* buffer, sw_flow_key *key, const ofp_action_header *actions, size_t actions_len, int ignore_no_fwd)
{
  NS_LOG_FUNCTION_NOARGS ();
  /* Every output action needs a separate clone of 'buffer', but the common
   * case is just a single output action, so that doing a clone and then
   * freeing the original buffer is wasteful.  So the following code is
   * slightly obscure just to avoid that. */
  int prev_port;
  size_t max_len = 0;     // Initialze to make compiler happy
  uint16_t in_port = key->flow.in_port; // ntohs(key->flow.in_port);
  uint8_t *p = (uint8_t *)actions;

  prev_port = -1;

  if (actions_len == 0)
    {
      NS_LOG_INFO ("No actions set to this flow. Dropping packet.");
      return;
    }

  /* The action list was already validated, so we can be a bit looser
   * in our sanity-checking. */
  while (actions_len > 0)
    {
      ofp_action_header *ah = (ofp_action_header *)p;
      size_t len = htons (ah->len);

      if (prev_port != -1)
        {
          swtch->DoOutput (packet_uid, in_port, max_len, prev_port, ignore_no_fwd);
          prev_port = -1;
        }

      if (ah->type == htons (OFPAT_OUTPUT))
        {
          ofp_action_output *oa = (ofp_action_output *)p;

          // port is now 32-bits
          prev_port = oa->port; // ntohl(oa->port);
          // prev_port = ntohs(oa->port);
          max_len = ntohs (oa->max_len);
        }
      else
        {
          uint16_t type = ntohs (ah->type);
          if (Action::IsValidType ((ofp_action_type)type)) // Execute a built-in OpenFlow action against 'buffer'.
            {
              Action::Execute ((ofp_action_type)type, buffer, key, ah);
            }
          else if (type == OFPAT_VENDOR)
            {
              ExecuteVendor (buffer, key, ah);
            }
        }

      p += len;
      actions_len -= len;
    }

  if (prev_port != -1)
    {
      swtch->DoOutput (packet_uid, in_port, max_len, prev_port, ignore_no_fwd);
    }
}

uint16_t
ValidateActions (const sw_flow_key *key, const ofp_action_header *actions, size_t actions_len)
{
  uint8_t *p = (uint8_t *)actions;
  int err;

  while (actions_len >= sizeof(ofp_action_header))
    {
      ofp_action_header *ah = (ofp_action_header *)p;
      size_t len = ntohs (ah->len);
      uint16_t type;

      /* Make there's enough remaining data for the specified length
        * and that the action length is a multiple of 64 bits. */
      if ((actions_len < len) || (len % 8) != 0)
        {
          return OFPBAC_BAD_LEN;
        }

      type = ntohs (ah->type);
      if (Action::IsValidType ((ofp_action_type)type)) // Validate built-in OpenFlow actions.
        {
          err = Action::Validate ((ofp_action_type)type, len, key, ah);
          if (err != ACT_VALIDATION_OK)
            {
              return err;
            }
        }
      else if (type == OFPAT_VENDOR)
        {
          err = ValidateVendor (key, ah, len);
          if (err != ACT_VALIDATION_OK)
            {
              return err;
            }
        }
      else
        {
          return OFPBAC_BAD_TYPE;
        }

      p += len;
      actions_len -= len;
    }

  // Check if there's any trailing garbage.
  if (actions_len != 0)
    {
      return OFPBAC_BAD_LEN;
    }

  return ACT_VALIDATION_OK;
}

}

}

#endif // NS3_PORTLAND
