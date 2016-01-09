/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Hajime Tazaki, NICT
 *               2015 Marc-Olivier Buob
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
 * Original author:
 *   Hajime Tazaki <tazaki@nict.go.jp>
 * Reworked:
 *   Marc-Olivier Buob <marcolivier.buob@orange.fr>
 */

#include "ospf-config.h"

#include <sstream>          // std::ostringstream
#include "ns3/log.h"        // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ( "OspfConfig" );

namespace ns3 {

OspfConfig::OspfConfig ( const std::string& hostname, const std::string& password, const std::string & passwordEnable, bool debug ) :
    QuaggaBaseConfig ( "ospf", "ospfd", DEFAULT_OSPFD_VTY_PORT, hostname, password, passwordEnable, debug ),
    m_routerId ( OSPF_DUMMY_ROUTER_ID ) {
    this->SetDebugCommand ( "event" );
    this->SetDebugCommand ( "nsm" );
    this->SetDebugCommand ( "ism" );
    this->SetDebugCommand ( "packet all" );
}

OspfConfig::~OspfConfig() {}

TypeId OspfConfig::GetTypeId() {
    static TypeId tid = TypeId ( "ns3::OspfConfig" )
                        .SetParent<Object>()
                        .AddConstructor<OspfConfig>();
    return tid;
}

TypeId OspfConfig::GetInstanceTypeId() const {
    return this->GetTypeId ();
}

const Ipv4Address& OspfConfig::GetRouterId() const {
    return this->m_routerId;
}

void OspfConfig::SetRouterId ( const Ipv4Address& routerId ) {
    this->m_routerId = routerId;
}

void OspfConfig::AddNetwork ( const Ipv4Prefix& prefix, const Ipv4Address& area ) {
    this->m_networks[prefix] = area;
}

void OspfConfig::AddRedistribute ( const OspfRedistribute& redistribute ) {
    this->m_redistributes.insert ( redistribute );
}

void OspfConfig::SetRedistribute ( uint8_t x ) {
    if ( x != REDISTRIBUTE_OSPF ) {
        this->AddRedistribute ( OspfRedistribute ( x ) );
    }
}

void OspfConfig::AddDistributeList ( const OspfDistributeList& distributeList ) {
    this->m_distributeLists.insert ( distributeList );
}

void OspfConfig::AddInterface ( const OspfInterface & interface ) {
    this->m_interfaces[interface.GetName()] = interface;
}

std::string OspfConfig::MakeInterfaceName ( uint32_t ifn ) {
    // In DCE the interface are names by default ns3-device0, ns3-device1, and so on.
    std::ostringstream oss;
    oss << "ns3-device" << ifn;
    return oss.str();
}

void OspfConfig::Print ( std::ostream& os ) const {
    this->PrintBegin ( os );

    for ( Interfaces::const_iterator it = this->m_interfaces.begin(); it != this->m_interfaces.end (); ++it ) {
        const OspfInterface & interface = it->second;
        os << interface
           << '!' << std::endl;
    }

    // interface ... (and ip ospf ...)
    os << "router ospf " << std::endl;

    os << "  timers throttle spf 100 100 1000" << std::endl;

    // router-id ...
    if ( this->GetRouterId() != Ipv4Address ( OSPF_DUMMY_ROUTER_ID ) ) {
        os << "  ospf router-id " << m_routerId << std::endl;
    }

    // network ... area ...
    for ( NetworksV4::const_iterator it = this->m_networks.begin(); it != this->m_networks.end(); ++it ) {
        const Ipv4Prefix  & prefix = it->first;
        const Ipv4Address & area   = it->second;
        os << "  network " << prefix << " area " << area << std::endl;
    }

    // redistribute ....
    for ( Redistributes::const_iterator it = this->m_redistributes.begin(); it != this->m_redistributes.end(); ++it ) {
        const OspfRedistribute & redistribute = *it;
        os << "  " << redistribute << std::endl;
    }

    os << "!" << std::endl;

    // distribute-list
    for ( DistributeLists::const_iterator it = this->m_distributeLists.begin(); it != this->m_distributeLists.end(); ++it ) {
        const OspfDistributeList & distributeList = *it;
        os << distributeList << std::endl;
    }

    os << "!" << std::endl;

    // access-list, prefix-list ...
    // Note: we could check that every filter are used at least once in this configuration file.
    this->PrintEnd ( os );
}

} // namespace ns3
