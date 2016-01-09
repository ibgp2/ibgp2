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

#include "ospf6-config.h"

#include "ns3/log.h"            // NS_LOG_*
#include "ns3/quagga-utils.h"   // AddressToString

NS_LOG_COMPONENT_DEFINE ("Ospf6Config");

namespace ns3 {

Ospf6Config::Ospf6Config (const std::string & hostname, const std::string & password, const std::string & passwordEnable, bool debug) :
    QuaggaBaseConfig ("ospf6", "ospf6d", DEFAULT_OSPF6D_VTY_PORT, hostname, password, passwordEnable, debug) {

    this->SetDebugCommand ("neighbor");
    this->SetDebugCommand ("message all");
    this->SetDebugCommand ("zebra");
    this->SetDebugCommand ("interface");
}

Ospf6Config::~Ospf6Config() {}

TypeId Ospf6Config::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::Ospf6Config")
                        .SetParent<Object> ()
                        .AddConstructor<Ospf6Config> ()
                        ;
    return tid;
}


TypeId Ospf6Config::GetInstanceTypeId (void) const {
    return GetTypeId ();
}

const Ipv4Address & Ospf6Config::GetRouterId() const {
    return this->router_id;
}

void Ospf6Config::SetRouterId (const Ipv4Address & routerId) {
    this->router_id = routerId;
}

// FOR BACKWARD COMPATIBILITY
void Ospf6Config::AddInterface (const std::string & name) {
    this->m_enable_if.push_back (name);
}

void Ospf6Config::Print (std::ostream & os) const {
    this->PrintBegin (os);

    os << "service advanced-vty" << std::endl;

    for (std::vector<std::string>::const_iterator i = m_enable_if.begin (); i != m_enable_if.end (); ++i) {
        os << "interface " << (*i) << std::endl;
        os << " ipv6 ospf6 retransmit-interval 8" << std::endl;
        os << "!" << std::endl;
    }

    for (std::vector<std::string>::const_iterator i = m_enable_if.begin ();  i != m_enable_if.end (); ++i) {
        if (i == m_enable_if.begin ()) {
            os << "router ospf6" << std::endl;
        }

        os << " router-id " << AddressToString (this->GetRouterId()) << std::endl;
        os << " interface " << (*i) << " area 0.0.0.0" << std::endl;
        os << " redistribute connected" << std::endl;

        if (i == m_enable_if.begin ()) {
            os << "!" << std::endl;
        }
    }

    this->PrintEnd (os);
}

} // end namespace ns3
