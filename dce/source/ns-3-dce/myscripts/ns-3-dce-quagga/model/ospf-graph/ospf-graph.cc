/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Marc-Olivier Buob, Alexandre Morignot
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
 * Authors:
 *   Marc-Olivier Buob <marcolivier.buob@orange.fr>
 *   Alexandre Morignot <alexandre.morignot@orange.fr>
 */


#include "ns3/ospf-graph.h"

#include <stdexcept> // std::runtime_error

namespace ns3 {
namespace ospf {

//-----------------------------------------------------------------
// OspfVertex
//-----------------------------------------------------------------

OspfVertex::OspfVertex() {}

OspfVertex::OspfVertex (const router_id_t & routerId) :
    m_routerId (routerId)
{}

OspfVertex::OspfVertex (const OspfVertex & o) {
    this->Copy (o);
}

const router_id_t & OspfVertex::GetRouterId() const {
    return this->m_routerId;
}

void OspfVertex::Copy (const OspfVertex & o) {
    this->m_routerId = o.GetRouterId();
}

OspfVertex & OspfVertex::operator= (const OspfVertex & o) {
    if (&o != this) this->Copy (o);

    return *this;
}

bool OspfVertex::operator== (const OspfVertex & o) const {
    return this->GetRouterId() == o.GetRouterId();
}

void OspfVertex::Print (std::ostream & os) const {
    os << this->GetRouterId();
}

std::ostream & operator<< (std::ostream & os, const OspfVertex & v) {
    v.Print (os);
    return os;
}

//-----------------------------------------------------------------
// OspfEdge
//-----------------------------------------------------------------

OspfEdge::OspfEdge() {}

OspfEdge::OspfEdge (
    const network_id_t & n,
    const Ipv4Address & i,
    const metric_t & m
) {
    this->m_mapDistances[n] = m;
    this->m_mapInterfaces[n] = i;
}

OspfEdge::OspfEdge (const OspfEdge & o) {
    this->Copy (o);
}

const ospf::metric_t OspfEdge::GetDistance() const {
    ospf::metric_t min = std::numeric_limits<ospf::metric_t>::max();

    for (auto & network : m_mapDistances) {
        if (network.second < min) {
            min = network.second;
        }
    }

    return min;
}

const network_id_t OspfEdge::GetNetwork() const {
    ospf::metric_t min = std::numeric_limits<ospf::metric_t>::max();
    network_id_t result;

    for (auto & network : m_mapDistances) {
        if (network.second < min) {
            min = network.second;
            result = network.first;
        }
    }

    return result;
}

const Ipv4Address & OspfEdge::GetInterface() const {
    const network_id_t & network = GetNetwork();
    std::map<ospf::network_id_t, Ipv4Address>::const_iterator it_if = m_mapInterfaces.find (network);

    NS_ASSERT_MSG (it_if != this->m_mapInterfaces.end(), "interface not found for " << network);

    if (it_if == this->m_mapInterfaces.end()) {
        throw std::runtime_error ("OspfEdge::GetInterface(): Interface not found.");
    }

    return it_if->second;
}

void OspfEdge::Copy (const OspfEdge & o) {
    this->m_mapDistances = o.m_mapDistances;
    this->m_mapInterfaces = o.m_mapInterfaces;
}

OspfEdge & OspfEdge::operator= (const OspfEdge & o) {
    if (&o != this) this->Copy (o);

    return *this;
}

const ns3::ospf::OspfEdge::MapDistances & OspfEdge::GetDistances() const {
    return this->m_mapDistances;
}

void OspfEdge::SetMetric (const network_id_t & n, const metric_t & m) {
    this->m_mapDistances[n] = m;
}

void OspfEdge::SetInterface (const network_id_t & n, const Ipv4Address & i) {
    this->m_mapInterfaces[n] = i;
}

std::size_t OspfEdge::GetNumNetworks() const {
    NS_ASSERT (this->m_mapDistances.size() == this->m_mapInterfaces.size());
    return this->m_mapDistances.size();
}

void OspfEdge::DeleteNetwork (const network_id_t & n) {
    this->m_mapDistances.erase (n);
    this->m_mapInterfaces.erase (n);
}

std::ostream & operator<< (std::ostream & os, const OspfEdge & e) {
    e.Print (os);
    return os;
}

void OspfEdge::Print (std::ostream & os) const {
    os << "-(" << this->GetDistance() << ")->";
}

} // namespace ospf
} // namespace ns3

