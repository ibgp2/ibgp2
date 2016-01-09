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

#include "rip-config.h"

#include "ns3/log.h"            // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ( "RipConfig" );

namespace ns3 {
    RipConfig::RipConfig ( const std::string& hostname, const std::string& password, const std::string & passwordEnable, bool debug ) :
    QuaggaBaseConfig ( "rip", "ripd", DEFAULT_RIPD_VTY_PORT, hostname, password, passwordEnable, debug ) {

        this->SetDebugCommand ( "events" );
        this->SetDebugCommand ( "packet send detail" );
        this->SetDebugCommand ( "packet recv detail" );
        this->SetDebugCommand ( "zebra" );
    }

    RipConfig::~RipConfig() {}

    TypeId RipConfig::GetTypeId ( void ) {
        static TypeId tid = TypeId ( "ns3::RipConfig" )
                            .SetParent<Object> ()
                            .AddConstructor<RipConfig> ()
                            ;
        return tid;
    }

    TypeId RipConfig::GetInstanceTypeId () const {
        return GetTypeId ();
    }

    // FOR BACKWARD COMPATIBILITY
    void RipConfig::AddInterface ( const std::string& name ) {
        this->m_enable_if.push_back ( name );
    }

    void RipConfig::Print ( std::ostream& os ) const {
        this->PrintBegin ( os );

        os << "service advanced-vty" << std::endl;

        for ( std::vector<std::string>::const_iterator i = this->m_enable_if.begin (); i != this->m_enable_if.end (); ++i ) {
            if ( i == this->m_enable_if.begin () ) {
                os << "router rip" << std::endl;
            }

            os << " network " << ( *i ) << std::endl;
            os << " redistribute connected" << std::endl;

            if ( i == this->m_enable_if.begin () ) {
                os << "!" << std::endl;
            }
        }

        this->PrintEnd ( os );
    }

}
