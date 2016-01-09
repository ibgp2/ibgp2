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

#include "bgp-neighbor.h"

#include "ns3/quagga-utils.h"   // AddressToString
#include "ns3/ipv6-address.h"   // ns3::Ipv6Address
#include "ns3/log.h"            // NS_LOG_COMPONENT_DEFINE

NS_LOG_COMPONENT_DEFINE ( "BgpNeighbor" );

namespace ns3 {

    BgpNeighbor::BgpNeighbor() {}
    BgpNeighbor::BgpNeighbor ( const Address& address, const uint32_t& asn, const std::string& description ) :
        m_address ( address ),
        m_remoteAs ( asn ),
        m_description ( description ),
        m_enableRrClient ( false ),
        m_enableNexthopSelf ( false ),
        m_updateSource ( false ),
        m_updateSourceAddress(),
        m_defaultOriginate ( false )
    {}

    BgpNeighbor::BgpNeighbor ( const BgpNeighbor& neighbor ) :
        m_remoteAs ( neighbor.GetRemoteAs() ),
        m_address ( neighbor.GetAddress() ),
        m_description ( neighbor.GetDescription() ),
        m_enableRrClient ( neighbor.GetRouteReflectorClient() ),
        m_enableNexthopSelf ( neighbor.GetNextHopSelf() ),
        m_updateSource ( neighbor.GetUpdateSource() ),
        m_updateSourceAddress ( neighbor.GetUpdateSourceAddress() ),
        m_defaultOriginate ( neighbor.GetDefaultOriginate() )
    {}

    void BgpNeighbor::SetRemoteAs ( uint32_t asn ) {
        this->m_remoteAs = asn;
    }

    uint32_t BgpNeighbor::GetRemoteAs() const {
        return this->m_remoteAs;
    }

    const Address& BgpNeighbor::GetAddress() const {
        return this->m_address;
    }

    void BgpNeighbor::SetDescription ( const std::string& description ) {
        this->m_description = description;
    }

    const std::string& BgpNeighbor::GetDescription() const {
        return this->m_description;
    }

    void BgpNeighbor::SetRouteReflectorClient ( bool on ) {
        this->m_enableRrClient = on;
    }

    bool BgpNeighbor::GetRouteReflectorClient() const {
        return this->m_enableRrClient;
    }

    void BgpNeighbor::SetNextHopSelf ( bool on ) {
        this->m_enableNexthopSelf = on;
    }

    bool BgpNeighbor::GetNextHopSelf() const {
        return this->m_enableNexthopSelf;
    }

    void BgpNeighbor::SetUpdateSource ( const Address& address ) {
        this->m_updateSource = true;
        this->m_updateSourceAddress = address;
    }

    void BgpNeighbor::UnsetUpdateSource() {
        this->m_updateSource = false;
    }

    bool BgpNeighbor::GetUpdateSource() const {
        return this->m_updateSource;
    }

    const Address& BgpNeighbor::GetUpdateSourceAddress() const {
        return this->m_updateSourceAddress;
    }

    bool BgpNeighbor::GetDefaultOriginate() const {
        return this->m_defaultOriginate;
    }

    void BgpNeighbor::SetDefaultOriginate ( bool on ) {
        this->m_defaultOriginate = on;
    }

    void BgpNeighbor::Print ( std::ostream& os ) const {
        std::ostringstream oss;
        oss << "  neighbor " << AddressToString ( this->GetAddress() ) << ' ';
        std::string s = oss.str();

        os << s << "remote-as " << this->GetRemoteAs() << std::endl;

        if ( this->GetDescription() != "" ) {
            os << s << "description " << this->GetDescription() << std::endl;
        }

        // prefix-list :  neighbor <peer> prefix-list <name> [in|out]
        for ( PrefixLists::const_iterator it = this->m_prefixLists.begin(); it != this->m_prefixLists.end(); ++it ) {
            const std::string & name = it->first;
            const Directions & directions = it->second;
            for ( Directions::const_iterator it2 = directions.begin(); it2 != directions.end(); ++it2 ) {
                const QuaggaDirection & direction = *it2;
                os << s << "prefix-list " << name << ' ' << direction << std::endl;
            }
        }

        // access-list : neighbor <peer> filter-list <name> [in|out]
        for ( AccessLists::const_iterator it = this->m_accessLists.begin(); it != this->m_accessLists.end(); ++it ) {
            const std::string & name = it->first;
            const Directions & directions = it->second;
            for ( Directions::const_iterator it2 = directions.begin(); it2 != directions.end(); ++it2 ) {
                const QuaggaDirection & direction = *it2;
                os << s << "filter-list " << name << ' ' << direction << std::endl;
            }
        }

        // route-map :  neighbor <peer> route-mpa <name> [in|out]
        for ( RouteMaps::const_iterator it = this->m_routeMaps.begin(); it != this->m_routeMaps.end(); ++it ) {
            const std::string & name = it->first;
            const Directions & directions = it->second;
            for ( Directions::const_iterator it2 = directions.begin(); it2 != directions.end(); ++it2 ) {
                const QuaggaDirection & direction = *it2;
                os << s << "route-map " << name << ' ' << direction << std::endl;
            }
        }

        if ( Ipv6Address::IsMatchingType ( this->GetAddress() ) ) {
            os << s << "activate" << std::endl;
        }

        if ( this->GetRouteReflectorClient() ) {
            os << s << "route-reflector-client" << std::endl;
        }

        if ( this->GetUpdateSource() ) {
            os << s << "update-source " << AddressToString ( this->GetUpdateSourceAddress() ) << std::endl;
        }

        if ( this->GetNextHopSelf() ) {
            os << s << "next-hop-self" << std::endl;
        }

        if ( this->GetDefaultOriginate() ) {
            os << s << "default-originate" << std::endl;
        }
    }

    void BgpNeighbor::AddAccessList ( const std::string& filterName, const QuaggaDirection& direction ) {
        this->m_accessLists[filterName].insert ( direction );
    }

    void BgpNeighbor::AddPrefixList ( const std::string& filterName, const QuaggaDirection& direction ) {
        this->m_prefixLists[filterName].insert ( direction );
    }

    void BgpNeighbor::AddRouteMap ( const std::string& routemapName, const QuaggaDirection& direction ) {
        this->m_routeMaps[routemapName].insert ( direction );
    }

    std::ostream& operator<< ( std::ostream& os, const BgpNeighbor& neighbor ) {
        neighbor.Print ( os );
        return os;
    }

} // namespace ns3
