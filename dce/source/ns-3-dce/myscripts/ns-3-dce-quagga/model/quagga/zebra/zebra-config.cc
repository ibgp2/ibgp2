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

#include "zebra-config.h"

#include "ns3/log.h"        // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ( "ZebraConfig" );

namespace ns3 {
    ZebraConfig::ZebraConfig ( const std::string& hostname, const std::string& password, const std::string& passwordEnable, bool debug ) :
        QuaggaBaseConfig ( "zebra", "zebra", DEFAULT_ZEBRA_VTY_PORT, hostname, password, passwordEnable, debug ) {
        this->SetDebugCommand ( "kernel" );
        this->SetDebugCommand ( "events" );
    }

    ZebraConfig::~ZebraConfig() {}

    ns3::TypeId ZebraConfig::GetTypeId() {
        static TypeId tid = TypeId ( "ns3::ZebraConfig" )
                            .SetParent<Object>()
                            .AddConstructor<ZebraConfig>();
        return tid;
    }

    ns3::TypeId ZebraConfig::GetInstanceTypeId() const {
        return this->GetTypeId ();
    }

    void ZebraConfig::AddRadvdIf ( const std::string& ipInterface, const std::string& prefix ) {
        this->m_radvd_if[ipInterface] = prefix;
    }

    void ZebraConfig::EnableHomeAgentFlag ( const std::string& ipInterface ) {
        this->m_haflag_if.insert ( ipInterface );
    }

    void ZebraConfig::AddInterface ( const std::string& name, const ZebraInterface& interface ) {
        this->m_interfaces[name] = interface;
    }

    void ZebraConfig::AddStaticRoute ( const ns3::Ipv4Prefix& prefix, const ns3::Ipv4Address & gateway ) {
        this->m_static_routes_v4[gateway].insert ( prefix );
    }

    /*
    void ZebraConfig::AddLoopback ( const std::string& name, const ns3::Address& address ) {
        if ( Ipv4Address::IsMatchingType ( address ) ) {
            std::cout << "QuaggaConfig::AddLoopback(): ADDING" << std::endl;
            ZebraInterface loopbackInterface ( name, "loopback" );
            Ipv4Address loopbackAddress = Ipv4Address::ConvertFrom ( address );
            Ipv4Prefix loopbackPrefix = Ipv4Prefix ( loopbackAddress, Ipv4Mask::GetOnes() );
            loopbackInterface.AddPrefix ( loopbackPrefix );
            this->AddInterface ( name, loopbackInterface );
        } else if ( Ipv6Address::IsMatchingType ( address ) ) {
            std::cerr << "ZebraConfig::AddLoopback: address type not yet supported" << std::endl;
        } else {
            std::cerr << "ZebraConfig::AddLoopback: Invalid address type" << std::endl;
        }
    }
    */

    void ZebraConfig::Print ( std::ostream& os ) const {
        this->PrintBegin ( os );


        // FOR BACKWARD COMPATIBILITY
        {
            // radvd
            for ( std::map<std::string, std::string>::const_iterator i = this->m_radvd_if.begin (); i != this->m_radvd_if.end (); ++i ) {
                const std::string & ipInterface = i->first;
                const std::string & prefix = i->second;

                os << "interface " << ipInterface << std::endl
                   << " ipv6 nd ra-interval 5" << std::endl;

                if ( prefix.length () != 0 ) {
                    os << " ipv6 nd prefix " << prefix << " 300 150" << std::endl;
                }

                os << " no ipv6 nd suppress-ra" << std::endl
                   << '!' << std::endl;
            }

            // home agent flag
            for ( std::set<std::string>::const_iterator i = this->m_haflag_if.begin (); i != this->m_haflag_if.end (); ++i ) {
                const std::string & ipInterface = *i;
                os << "interface " << ipInterface << std::endl
                   << " ipv6 nd home-agent-osig-flag" << std::endl
                   << '!' << std::endl;
            }
        }

        // standard interfaces
        for ( interfaces_t::const_iterator i = this->m_interfaces.begin(); i != this->m_interfaces.end(); ++i ) {
            const ZebraInterface & interface = i->second;
            os << interface
               << '!' << std::endl;
        }

        // static routes
        for ( static_routes_v4_t::const_iterator i = this->m_static_routes_v4.begin(); i != this->m_static_routes_v4.end(); ++i ) {
            const Ipv4Address & interface = i->first;
            const std::set<Ipv4Prefix> & prefixes = i->second;
            for ( std::set<Ipv4Prefix>::const_iterator j = prefixes.begin(); j != prefixes.end(); ++j ) {
                const Ipv4Prefix & prefix = *j;
                os << "ip route " << prefix.GetAddress() << "/" << prefix.GetPrefixLength() << ' ' << interface << std::endl;
            }
        }

        this->PrintEnd ( os );
    }
}
