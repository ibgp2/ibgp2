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

#include <sstream>              // std::ostringstream

#include "ns3/assert.h"         // NS_ASSERT
#include "ns3/log.h"            // NS_LOG_COMPONENT_DEFINE
#include "ns3/prefix-list.h"    // ns3::PrefixList, ns3::PrefixListElement

NS_LOG_COMPONENT_DEFINE ( "PrefixList" );

namespace ns3 {

    //-----------------------------------------------------------------------------------
    // PrefixListElement
    //-----------------------------------------------------------------------------------

    PrefixListElement::PrefixListElement() :
        PrefixListElement ( DENY, Ipv4Prefix(), EQ, 0, 0 )
    {}

    PrefixListElement::PrefixListElement ( const PrefixListAction & action, const Ipv4Prefix & prefix, uint32_t seq ) :
        PrefixListElement ( action, prefix, EQ, 0, seq )
    {}

    PrefixListElement::PrefixListElement (
        const PrefixListAction   & action,
        const Ipv4Prefix         & prefix,
        const PrefixListOperator & op,
        uint8_t                    length,
        uint32_t                   seq
    ) :
        m_action ( action ),
        m_prefixV4 ( prefix ),
        m_operator ( op ),
        m_prefixLength ( length ),
        m_seq ( seq )
    {}

    PrefixListElement::PrefixListElement ( const PrefixListElement& x ) :
        PrefixListElement ( x.GetAction(), x.GetPrefix(), x.GetOperator(), x.GetPrefixLength(), x.GetSeq() )
    {}

    PrefixListElement::~PrefixListElement() {}

    const Ipv4Prefix& PrefixListElement::GetPrefix() const {
        return this->m_prefixV4;
    }

    void PrefixListElement::SetPrefix ( const ns3::Ipv4Prefix& prefixV4 ) {
        this->m_prefixV4 = prefixV4;
    }

    uint8_t PrefixListElement::GetPrefixLength() const {
        return this->m_prefixLength;
    }

    void PrefixListElement::SetPrefixLength ( uint8_t length ) {
        this->m_prefixLength = length;
    }

    const PrefixListAction & PrefixListElement::GetAction() const {
        return this->m_action;
    }

    void PrefixListElement::SetAction ( const PrefixListAction & permit ) {
        this->m_action = permit;
    }

    uint32_t PrefixListElement::GetSeq() const {
        return this->m_seq;
    }

    void PrefixListElement::SetSeq ( uint32_t seq ) {
        this->m_seq = seq;
    }

    bool PrefixListElement::IsAny() const {
        return this->GetPrefix() == Ipv4Prefix::Any();
    }

    const PrefixListOperator & PrefixListElement::GetOperator() const {
        return this->m_operator;
    }

    void PrefixListElement::SetOperator ( const PrefixListOperator & op, uint8_t prefixLength ) {
        this->m_operator = op;
        this->m_prefixLength = prefixLength;
    }

    void PrefixListElement::Print ( std::ostream& out ) const {
        // Rq: The beginning of the corresponding line is written by PrefixList class.

        // seq
        if ( this->GetSeq() ) {
            out << " seq" << this->GetSeq();
        }

        // permit|deny
        switch ( this->GetAction() ) {
            case PERMIT:
                out << " permit";
                break;
            case DENY:
                out << " deny";
                break;
            default:
                out << "???";
                NS_ASSERT(true);
                break;
        }

        // prefix
        if ( this->IsAny() ) {
            out << " any";
        } else {
            out << ' ' << this->GetPrefix();
        }

        // le / ge
        switch ( this->GetOperator() ) {
            case LE:
                out << " le" << this->GetPrefixLength();
                break;
            case GE:
                out << " ge" << this->GetPrefixLength();
                break;
            case EQ:
                break;
        }
    }

    std::ostream& operator<< ( std::ostream& out, const PrefixListElement & prefixListElement ) {
        prefixListElement.Print ( out );
        return out;
    }

    bool operator < ( const PrefixListElement& x, const PrefixListElement& y ) {
        return x.GetSeq() < y.GetSeq();
    }

    //-----------------------------------------------------------------------------------
    // PrefixList
    //-----------------------------------------------------------------------------------

    PrefixList::PrefixList ( const std::string& name ) :
        m_elements(),
        m_name( name ),
        m_lastSeq ( 0 )
    {}

    PrefixList::~PrefixList() {}

    const std::string& PrefixList::GetName() const {
        return this->m_name;
    }

    void PrefixList::Add ( const PrefixListElement& rule ) {
        uint32_t seq;

        if ( rule.GetSeq() ) {
            seq = rule.GetSeq();
            this->m_lastSeq = std::max ( seq + 1, this->m_lastSeq );
        } else {
            seq = ++ ( this->m_lastSeq );
        }

        this->m_elements[seq] = rule;
    }

    std::ostream& PrefixList::Print ( std::ostream& os ) const {
        std::ostringstream oss;
        oss << "ip prefix-list " << this->GetName();
        const std::string & p = oss.str();

        for ( Elements::const_iterator it = this->m_elements.begin(); it != this->m_elements.end(); ++it ) {
            const PrefixListElement & element = it->second;
            os << p << element << std::endl;
        }
        os << '!' << std::endl;

        return os;
    }

    std::ostream& operator<< ( std::ostream& os, const PrefixList& prefixList ) {
        prefixList.Print ( os );
    }

    bool operator< ( const PrefixList& x, const PrefixList& y ) {
        return x.GetName() < y.GetName();
    }


} // namespace ns3
