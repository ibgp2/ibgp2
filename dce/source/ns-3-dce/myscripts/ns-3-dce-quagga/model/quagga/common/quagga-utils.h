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

#ifndef QUAGGA_UTILS_H
#define QUAGGA_UTILS_H

#include <arpa/inet.h>          // ::inet_pton

#include "ns3/address.h"        // ns3::Address
#include "ns3/ipv4-address.h"   // ns3::Ipv4Address
#include "ns3/ipv6-address.h"   // ns3::Ipv6Address

namespace ns3
{

  std::string AddressToString (const Address & address);

  bool Ipv4AddressFromString (const std::string & s, Ipv4Address & ipv4);

  bool Ipv6AddressFromString (const std::string & s, Ipv6Address & ipv6);

  struct CompareIpv6Prefix
  {
    bool operator () (const Ipv6Prefix & x, const Ipv6Prefix & y);
  };

}				// end namespace ns3

#endif
