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

#include "quagga-utils.h"

#include <arpa/inet.h>          // ::inet_pton, AF_INET, AF_INET6
#include <iostream>             // std::cerr

#include "ns3/address.h"        // ns3::Address
#include "ns3/ipv4-address.h"   // ns3::Ipv4Address
#include "ns3/ipv6-address.h"   // ns3::Ipv6Address
#include "ns3/log.h"            // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ("QuaggaUtils");

namespace ns3 {

std::string AddressToString (const Address & address) {
    std::ostringstream oss;

    if (Ipv4Address::IsMatchingType (address)) {
        oss << Ipv4Address::ConvertFrom (address);
    } else if (Ipv6Address::IsMatchingType (address)) {
        oss << Ipv6Address::ConvertFrom (address);
    } else {
        std::cerr <<  "AddressToString: unknown address type."  << std::endl;
    }

    return oss.str();
}

bool Ipv4AddressFromString (const std::string & s, Ipv4Address & ipv4) {
    struct in_addr addr;
    bool ret = ::inet_pton (AF_INET, s.c_str(), &addr);

    if (ret) {
        ipv4 = Ipv4Address (s.c_str());
    }

    return ret;
}

bool Ipv6AddressFromString (const std::string & s, Ipv6Address & ipv6) {
    struct in_addr addr;
    bool ret = ::inet_pton (AF_INET6, s.c_str(), &addr);

    if (ret) {
        ipv6 = Ipv6Address (s.c_str());
    }

    return ret;
}

bool CompareIpv6Prefix::operator() (const Ipv6Prefix & x, const Ipv6Prefix & y) {
    std::ostringstream oss_x, oss_y;
    oss_x << x;
    oss_y << y;
    return oss_x.str() < oss_y.str();
}

} // end namespace ns3
