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

#include "access-list.h"

#include <sstream>

#include "ns3/log.h"            // NS_LOG_COMPONENT_DEFINE

NS_LOG_COMPONENT_DEFINE ( "AccessList" );

namespace ns3 {

    AccessListElement::AccessListElement() :
        m_permit ( false ),
        m_prefixV4 ()
    {}

    AccessListElement::AccessListElement ( bool permit, const Ipv4Prefix& prefix ) :
        m_permit ( permit ),
        m_prefixV4 ( prefix )
    {}

    AccessListElement::~AccessListElement() {}

    const Ipv4Prefix& AccessListElement::GetPrefix() const {
        return this->m_prefixV4;
    }

    void AccessListElement::SetPrefix ( const Ipv4Prefix& prefixV4 ) {
        this->m_prefixV4 = prefixV4;
    }

    bool AccessListElement::GetPermit() const {
        return this->m_permit;
    }

    void AccessListElement::SetPermit ( bool permit ) {
        this->m_permit = permit;
    }

    void AccessListElement::Print ( std::ostream& os ) const {
        if ( this->GetPermit() ) {
            os << "permit";
        } else {
            os << "deny";
        }

        if ( this->GetPrefix() == Ipv4Prefix::Any() ) {
            os << " any";
        } else {
            os << ' ' << this->GetPrefix();
        }
    }

    std::ostream& operator<< ( std::ostream& os, const AccessListElement& accessListElement ) {
        accessListElement.Print ( os );
        return os;
    }

    //--------------------------------------------------------------------------------------
    // AccessList
    //--------------------------------------------------------------------------------------

    AccessList::AccessList() {}

    AccessList::AccessList ( const std::string& name ) :
        m_name ( name )
    {}

    AccessList::~AccessList() {}

    void AccessList::Add ( const ns3::AccessListElement& element ) {
        this->m_elements.push_back ( element );
    }

    const std::string& AccessList::GetName() const {
        return this->m_name;
    }

    void AccessList::Print ( std::ostream& os ) const {
        std::ostringstream oss;
        oss << "access-list " << this->GetName() << ' ';
        const std::string & s = oss.str();
        for ( Elements::const_iterator it = this->m_elements.begin(); it != this->m_elements.end(); ++it ) {
            const AccessListElement & element = *it;
            os << s << element << std::endl;
        }
        os << '!' << std::endl;
    }

    std::ostream& operator<< ( std::ostream& os, const AccessList& accessList ) {
        accessList.Print ( os );
        return os;
    }

    bool operator< ( const AccessList& x, const AccessList& y ) {
        return x.GetName() < y.GetName();
    }

} // namespace ns3
