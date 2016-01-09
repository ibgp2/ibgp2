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

#include "bgp-config.h"

#include <sstream>              // std::ostringstream
#include <stdexcept>            // std::runtime_error
#include "ns3/log.h"            // NS_LOG_*
#include "ns3/quagga-utils.h"   // AddressToString

NS_LOG_COMPONENT_DEFINE ( "BgpConfig" );

#define BGP_DUMMY_ROUTER_ID "0.0.0.0"

namespace ns3 {

    BgpConfig::BgpConfig ( const std::string& hostname, const std::string& password, const std::string & passwordEnable, bool debug ) :
        QuaggaBaseConfig ( "bgp", "bgpd", DEFAULT_BGPD_VTY_PORT, hostname, password, passwordEnable, debug ),
        m_routerId(Ipv4Address(BGP_DUMMY_ROUTER_ID)),
        m_synchronization ( false ),
        m_maskRedistribute ( 0 )
    {
        this->SetDebugCommand ( "fsm" );
        this->SetDebugCommand ( "events" );
        this->SetDebugCommand ( "filters" );
        this->SetDebugCommand ( "updates" );
    }

    BgpConfig::~BgpConfig() {}

    TypeId BgpConfig::GetTypeId() {
        static TypeId tid = TypeId ( "ns3::BgpConfig" )
                            .SetParent<Object> ()
                            .AddConstructor<BgpConfig> ();
        return tid;
    }

    TypeId BgpConfig::GetInstanceTypeId() const {
        return this->GetTypeId ();
    }

    void BgpConfig::SetAsn ( uint32_t asn ) {
        this->m_asn = asn;
    }

    uint32_t BgpConfig::GetAsn() const {
        return this->m_asn;
    }

    bool BgpConfig::SetSynchronization ( bool on ) {
        this->m_synchronization = on;
    }

    bool BgpConfig::GetSynchronization() const {
        return this->m_synchronization;
    }

    void BgpConfig::SetRedistribute ( uint8_t mask ) {
        this->m_maskRedistribute = mask;
    }

    uint8_t BgpConfig::GetRedistribute() const {
        return this->m_maskRedistribute;
    }

    void BgpConfig::SetRouterId ( const Address& routerId ) {
        this->m_routerId = routerId;
    }

    const BgpConfig::RouterId& BgpConfig::GetRouterId() const {
        return this->m_routerId;
    }

    // FOR BACKWARD COMPATIBILITY
    bool BgpConfig::AddNeighbor ( const std::string& neighborIp, uint32_t asn, const std::string& description ) {
        NS_LOG_WARN ( "OBSOLETE: AddNeighbor()" );
        Ipv4Address ipv4;
        if ( Ipv4AddressFromString ( neighborIp, ipv4 ) ) {
            BgpNeighbor neighbor ( ipv4, asn, description );
            this->m_neighborsV4[ipv4] = neighbor;
            return true;
        }

        Ipv6Address ipv6;
        if ( Ipv6AddressFromString ( neighborIp, ipv6 ) ) {
            BgpNeighbor neighbor ( ipv6, asn, description );
            this->m_neighborsV6[ipv6] = neighbor;
            return true;
        }

        NS_LOG_WARN ( "AddNeighbor: invalid address: " << neighborIp );
        return false;
    }

    bool BgpConfig::AddNeighbor ( const BgpNeighbor& neighbor ) {
        bool ret = true;
        const Address & address = neighbor.GetAddress();

        if ( Ipv4Address::IsMatchingType ( address ) ) {
            Ipv4Address ipv4 = Ipv4Address::ConvertFrom ( address );
            this->m_neighborsV4[ipv4] = neighbor;
        } else if ( Ipv6Address::IsMatchingType ( address ) ) {
            Ipv6Address ipv6 = Ipv6Address::ConvertFrom ( address );
            this->m_neighborsV6[ipv6] = neighbor;
        } else {
            NS_LOG_WARN ( "AddNeighbor: invalid address type." );
            ret = false;
        }

        return ret;
    }

    // FOR BACKWARD COMPATIBILITY
    void BgpConfig::AddNetwork ( const std::string& prefix ) {
        NS_LOG_WARN ( "OBSOLETE: AddNetwork()" );
        this->networks.push_back ( prefix );
    }

    void BgpConfig::AddNetwork ( const Ipv4Prefix& prefix ) {
        this->m_networksV4.insert ( prefix );
    }

    void BgpConfig::AddNetwork ( const Ipv6Prefix& prefix ) {
        this->m_networksV6.insert ( prefix );
    }

    void BgpConfig::Print ( std::ostream& os ) const {
        this->PrintBegin ( os );

        std::string routerId = AddressToString ( this->m_routerId );
        uint32_t asn = this->GetAsn();

        os << "router bgp "    << asn      << std::endl;

        if (this->GetRouterId() != Ipv4Address(BGP_DUMMY_ROUTER_ID)) {
            os << "bgp router-id " << routerId << std::endl;
        }

        if ( !this->GetSynchronization() ) {
            os << "no synchronization" << std::endl;
        }

        os << "!" << std::endl;

        // If configured, redistributed some kind of routes in BGP
        if ( uint8_t mask = this->GetRedistribute() ) {
            if ( mask & REDISTRIBUTE_KERNEL ) {
                os << "redistribute kernel"    << std::endl;
            }
            if ( mask & REDISTRIBUTE_STATIC ) {
                os << "redistribute static"    << std::endl;
            }
            if ( mask & REDISTRIBUTE_CONNECTED ) {
                os << "redistribute connected" << std::endl;
            }
            if ( mask & REDISTRIBUTE_RIP ) {
                os << "redistribute rip"       << std::endl;
            }
            if ( mask & REDISTRIBUTE_OSPF ) {
                os << "redistribute ospf"      << std::endl;
            }
            os << "!" << std::endl;
        }

        // IPv4 ---------------------------------------------------------------------------------------

        // <<<<<<<< FOR BACKWARD COMPATIBILITY (we should use networks_v4 or networks_v6)
        // Configure each network announced with BGP
        // os << "  address-family ipv4 unicast" << std::endl;
        for ( networks_t::const_iterator it = this->networks.begin(); it != this->networks.end(); it++ ) {
            const std::string & prefixNetwork = *it;
            os << "network " << prefixNetwork << std::endl
               << "!" << std::endl;
        }
        // >>>>>>>> FOR BACKWARD COMPATIBILITY

        if ( !this->m_networksV4.empty() || !this->m_neighborsV4.empty() ) {
            // Networks IPv4
            if ( !this->m_networksV4.empty() ) {
                for ( NetworksV4::const_iterator it = this->m_networksV4.begin(); it != this->m_networksV4.end(); it++ ) {
                    const Ipv4Prefix & prefix = *it;
                    os << "network " << prefix << std::endl;
                }
                os << "!" << std::endl;
            }

            // Neighbors IPv4
            for ( NeighborsV4::const_iterator it = this->m_neighborsV4.begin(); it != this->m_neighborsV4.end(); ++it ) {
                const BgpNeighbor & neighbor = it->second;
                os << neighbor
                   << "!" << std::endl;
            }
        }

        // IPv6 ---------------------------------------------------------------------------------------

        if ( !this->m_networksV6.empty() || !this->m_neighborsV6.empty() ) {
            os << "address-family ipv6 unicast" << std::endl
               << "!" << std::endl;

            // Networks IPv6
            if ( !this->m_networksV6.empty() ) {
                for ( NetworksV6::const_iterator it = this->m_networksV6.begin(); it != this->m_networksV6.end(); it++ ) {
                    const Ipv6Prefix & prefix = *it;
                    os << "network " << prefix << std::endl;
                }
                os << "!" << std::endl;
            }

            // Neighbors IPv6
            for ( NeighborsV6::const_iterator it = this->m_neighborsV6.begin(); it != this->m_neighborsV6.end(); ++it ) {
                const BgpNeighbor & neighbor = it->second;
                os << neighbor
                   << "!" << std::endl;
            }

            os << "exit address-family" << std::endl
               << "!" << std::endl;
        }

        // Filters ------------------------------------------------------------------------------------

        // TODO route-map
        // TODO ip as-path access-list <name> {permit|deny} <regexp>

        this->PrintEnd ( os );
    }

    BgpNeighbor& BgpConfig::GetNeighbor ( const Address& address ) {
        if ( Ipv4Address::IsMatchingType ( address ) ) {
            Ipv4Address ipv4 = Ipv4Address::ConvertFrom ( address );
            NeighborsV4::iterator it = this->m_neighborsV4.find ( ipv4 );
            if ( it == m_neighborsV4.end() ) {
                std::ostringstream oss;
                oss << "BgpConfig::GetNeighbor(): [" << ipv4 << "] not found";
                throw std::runtime_error ( oss.str() );
            }
            return it->second;
        } else if ( Ipv6Address::IsMatchingType ( address ) ) {
            Ipv6Address ipv6 = Ipv6Address::ConvertFrom ( address );
            NeighborsV6::iterator it = this->m_neighborsV6.find ( ipv6 );
            if ( it == m_neighborsV6.end() ) {
                std::ostringstream oss;
                oss << "BgpConfig::GetNeighbor(): [" << ipv6 << "] not found";
                throw std::runtime_error ( oss.str() );
            }
            return it->second;
        } else {
            throw std::runtime_error ( "BgpConfig::GetNeighbor(): invalid address type (not Ipv4Address nor Ipv6Address)." );
        }
    }

} // namespace ns3

