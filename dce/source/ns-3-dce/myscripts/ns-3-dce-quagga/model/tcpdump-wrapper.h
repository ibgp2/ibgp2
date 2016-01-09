/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Marc-Olivier Buob
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
 * Author:
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 */

#ifndef TCPDUMP_WRAPPER_H
#define TCPDUMP_WRAPPER_H

#include <string>               // std::string
#include <cstdint>              // uintXX_t

#include "ns3/ptr.h"            // ns3::Ptr<>
#include "ns3/packet.h"         // ns3::Packet
#include "ns3/trace-helper.h"   // PcapHelper::DLT_PPP

namespace ns3 {

/**
 * \brief Pipe a binary shell string in tcpdump and write
 *    the result in an output stream.
 * \param out The output stream.
 * \param packet The binary shell string.
 */

void
tcpdump(std::ostream & out, const char * packet);

/**
 * \brief Dissect a Packet using tcpdump.
 * \param p A pointer to the Packet instance.
 * \param dataLinkType Type of link.
 */

std::string
tcpdump(Ptr<const Packet> p, uint32_t dataLinkType = PcapHelper::DLT_PPP);

} // namespace ns3

//---------------------------------------------------------------
// The following is an example of usage of tcpdump
//---------------------------------------------------------------
//
// #include <map>                  // std::map
// #include <vector>               // std::vector
// #include "ns3/ospf-packet.h"    // ns3::ospf::network_id_t, ns3::network_prefix_t
//
// namespace ns3 {
//
// /**
//  * \brief Extract the OSPF LSA embedded in a given Packet.
//  *   Only OSPF LSA Update are considered (type 1, 2, 5)
//  * \param p The Packet to dissect.
//  * \param prefixMap
//  * \returns The corresponding LSAs.
//  */
//
// std::vector<OspfLsa *>
// LegacyExtractOspfLsa (
//     const Ptr<const Packet> p,
//     std::map<ospf::network_id_t, ospf::network_prefix_t> & prefixMap
// );
//
// } // namespace ns3

#endif
