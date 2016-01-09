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

#include "ospf-interface.h"

#include "ns3/log.h"        // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ( "OspfInterface" );

namespace ns3 {

OspfInterface::OspfInterface() {}

OspfInterface::OspfInterface ( const std::string& name, uint16_t cost ) :
    m_name ( name ),
    m_cost ( cost ),
    m_helloInterval ( 2 ),
    m_deadInterval ( 6 )
{}

const std::string& OspfInterface::GetName() const {
    return this->m_name;
}

uint16_t OspfInterface::GetCost() const {
    return this->m_cost;
}

void OspfInterface::SetCost ( uint16_t cost ) {
    this->m_cost = cost;
}

uint16_t OspfInterface::GetHelloInterval() const {
    return this->m_helloInterval;
}

void OspfInterface::SetHelloInterval ( uint16_t interval ) {
    this->m_helloInterval = interval;
}

uint16_t OspfInterface::GetDeadInterval() const {
    return this->m_deadInterval;
}

void OspfInterface::SetDeadInterval ( uint16_t interval ) {
    this->m_deadInterval = interval;
}

uint16_t OspfInterface::GetTransmitDelay() const {
    return this->m_transmitDelay;
}

void OspfInterface::SetTransmitDelay(uint16_t delay) {
    this->m_transmitDelay = delay;
}

uint16_t OspfInterface::GetRetransmitInterval() const {
    return this->m_retransmitInterval;
}

void OspfInterface::SetRetransmitInterval(uint16_t interval) {
    this->m_retransmitInterval = interval;
}

void OspfInterface::Print (std::ostream& os) const {
    std::string s ( "  ip ospf " );
    os << "interface " << this->GetName() << std::endl;

    if ( uint16_t t = this->GetTransmitDelay()) {
        os << s << "transmit-delay " << t << std::endl;
    }

    if ( uint16_t t = this->GetRetransmitInterval()) {
        os << s << "retransmit-interval " << t << std::endl;
    }

    if ( uint16_t t = this->GetHelloInterval() ) {
        os << s << "hello-interval " << t << std::endl;
    }

    if ( uint16_t t = this->GetDeadInterval() ) {
        // see also ip ospf dead-interval minimal hello-multiplier <2-20>  (we could set 3)
        os << s << "dead-interval " << t << std::endl;
    }

    os << s << "cost " << this->GetCost() << std::endl;
}

std::ostream& operator<< ( std::ostream& os, const ns3::OspfInterface& interface ) {
    interface.Print(os);
    return os;
}

} // namespace ns3
