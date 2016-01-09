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

#include "quagga-distribute-list.h"

#include "ns3/log.h" // NS_LOG_COMPONENT_DEFINE

NS_LOG_COMPONENT_DEFINE ("QuaggaDistributeList");

namespace ns3 {

QuaggaDistributeList::QuaggaDistributeList() {}

QuaggaDistributeList::QuaggaDistributeList (
    const ns3::QuaggaDirection & direction,
    const std::string & filterName
) :
    m_direction (direction),
    m_filterName (filterName)
{}

const std::string & QuaggaDistributeList::GetFilterName() const {
    return this->m_filterName;
}

void QuaggaDistributeList::SetFilterName (const std::string & filterName) {
    this->m_filterName = filterName;
}

const ns3::QuaggaDirection & QuaggaDistributeList::GetDirection() const {
    return this->m_direction;
}

void QuaggaDistributeList::SetDirection (const ns3::QuaggaDirection & direction) {
    this->m_direction = direction;
}

std::ostream & operator<< (
    std::ostream & os,
    const ns3::QuaggaDistributeList & distributeList
) {
    distributeList.Print (os);
    return os;
}

}
