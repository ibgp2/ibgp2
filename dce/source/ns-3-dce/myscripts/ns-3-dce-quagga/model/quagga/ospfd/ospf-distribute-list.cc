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

#include "ospf-distribute-list.h"

#include "ns3/log.h" // NS_LOG_COMPONENT_DEFINE

NS_LOG_COMPONENT_DEFINE ("OspfDistributeList");

namespace ns3 {


OspfDistributeList::OspfDistributeList (
    const std::string & filterName,
    const QuaggaDirection & direction, uint8_t distribute
) :
    QuaggaDistributeList (direction, filterName),
    m_distribute (distribute)
{}

uint8_t OspfDistributeList::GetDistribute() const {
    return this->m_distribute;
}

void OspfDistributeList::SetDistribute (uint8_t distribute) {
    this->m_distribute = distribute;
}

void OspfDistributeList::Print (std::ostream & os) const {
    os << "distribute-list " << this->GetFilterName()
       << ' ' << this->GetDirection() << ' ';

    if (this->m_distribute & REDISTRIBUTE_KERNEL) {
        os << "kernel";
    } else if (this->m_distribute & REDISTRIBUTE_CONNECTED) {
        os << "connected";
    } else if (this->m_distribute & REDISTRIBUTE_STATIC) {
        os << "static";
    } else if (this->m_distribute & REDISTRIBUTE_RIP) {
        os << "rip";
    } else if (this->m_distribute & REDISTRIBUTE_BGP) {
        os << "bgp";
    }
}

bool operator< (const OspfDistributeList & x, const OspfDistributeList & y) {
    if (x.GetFilterName() < y.GetFilterName()) return true;
    if (y.GetFilterName() < x.GetFilterName()) return false;

    if (x.GetDirection() < y.GetDirection())   return true;
    if (y.GetDirection() < x.GetDirection())   return false;

    if (x.GetDistribute() < y.GetDistribute()) return true;
    if (y.GetDistribute() < x.GetDistribute()) return false;

    return false;
}

} // namespace ns3
