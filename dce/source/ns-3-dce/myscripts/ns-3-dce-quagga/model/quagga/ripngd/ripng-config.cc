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

#include "ripng-config.h"

#include "ns3/log.h"            // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ( "RipngConfig" );

namespace ns3 {
    RipngConfig::RipngConfig ( const std::string& hostname, const std::string& password, const std::string& passwordEnable, bool debug ) :
        QuaggaBaseConfig ( "ripng", "ripngd", DEFAULT_RIPNGD_VTY_PORT, hostname, password, passwordEnable, debug ) {

        this->SetDebugCommand ( "events" );
        this->SetDebugCommand ( "packet send detail" );
        this->SetDebugCommand ( "packet recv detail" );
        this->SetDebugCommand ( "zebra" );
    }

    RipngConfig::~RipngConfig() {}

    TypeId RipngConfig::GetTypeId ( void ) {
        static TypeId tid = TypeId ( "ns3::RipngConfig" )
                            .SetParent<Object> ()
                            .AddConstructor<RipngConfig> ()
                            ;
        return tid;
    }

    TypeId RipngConfig::GetInstanceTypeId () const {
        return GetTypeId ();
    }

    // FOR BACKWARD COMPATIBILITY
    void RipngConfig::AddInterface ( const std::string& name ) {
        this->m_enable_if.push_back ( name );
    }

    void RipngConfig::Print ( std::ostream& os ) const {
        this->PrintBegin ( os );

        os << "service advanced-vty" << std::endl;

        for ( std::vector<std::string>::const_iterator i = this->m_enable_if.begin (); i != this->m_enable_if.end (); ++i ) {
            if ( i == this->m_enable_if.begin () ) {
                os << "router ripng" << std::endl;
            }

            os << " network " << ( *i ) << std::endl
               << " redistribute connected" << std::endl;

            if ( i == this->m_enable_if.begin () ) {
                os << "!" << std::endl;
            }
        }

        this->PrintEnd ( os );
    }

}
