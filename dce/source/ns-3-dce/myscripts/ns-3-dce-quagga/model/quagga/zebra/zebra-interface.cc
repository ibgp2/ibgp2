/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015
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

#include "zebra-interface.h"

#include "ns3/quagga-utils.h"       // AddressToString
#include "ns3/log.h"                // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ( "ZebraInterface" );

namespace ns3 {

    ZebraInterface::ZebraInterface ( const std::string& name, const std::string& description ) :
        name ( name ),
        description ( description ),
        link_detect ( true )
    {}

    const std::string& ZebraInterface::GetName() const {
        return this->name;
    }

    void ZebraInterface::SetName ( const std::string& name ) {
        this->name = name;
    }

    const std::string& ZebraInterface::GetDescription() const {
        return this->description;
    }

    void ZebraInterface::AddPrefix ( const Ipv4Prefix& prefix ) {
        this->prefixes_v4.insert ( prefix );
    }

    void ZebraInterface::AddPrefix ( const Ipv6Prefix& prefix ) {
        this->prefixes_v6.insert ( prefix );
    }

    const ZebraInterface::prefixes_v4_t& ZebraInterface::GetPrefixesV4() const {
        return this->prefixes_v4;
    }

    const ZebraInterface::prefixes_v6_t& ZebraInterface::GetPrefixesV6() const {
        return this->prefixes_v6;
    }

    bool ZebraInterface::GetLinkDetect() const {
        return this->link_detect;
    }

    void ZebraInterface::SetLinKDetect ( bool newState ) {
        this->link_detect = newState;
    }

    std::ostream& ZebraInterface::Print ( std::ostream& out ) const {
        const ZebraInterface & interface = *this;
        const std::string & description = interface.GetDescription();

        out << "interface " << interface.GetName() << std::endl;

        if ( description != "" ) {
            out << "  description " << description << std::endl;
        }


        // IPv4 addresses & mask attached to this interface
        const ZebraInterface::prefixes_v4_t & prefixes_v4 = interface.GetPrefixesV4();
        {
            ZebraInterface::prefixes_v4_t::const_iterator
            it ( prefixes_v4.begin() ),
               end ( prefixes_v4.end() );
            for ( ; it != end; ++it ) {
                const Ipv4Prefix & prefix = *it;
                out << "  ip address " << prefix << std::endl;
            }
        }

        // IPv6 addresses & mask attached to this interface
        const ZebraInterface::prefixes_v6_t & prefixes_v6 = interface.GetPrefixesV6();
        {
            ZebraInterface::prefixes_v6_t::const_iterator
            it ( prefixes_v6.begin() ),
               end ( prefixes_v6.end() );
            for ( ; it != end; ++it ) {
                const Ipv6Prefix & prefix = *it;
                out << "  ipv6 address ";
                prefix.Print ( out );
                out << std::endl;
            }
        }

        if ( interface.GetLinkDetect() ) {
            out << "  link-detect" << std::endl;
        }

        return out;
    }

    std::ostream& operator<< ( std::ostream& out, const ZebraInterface& interface ) {
        return interface.Print ( out );
    }

}