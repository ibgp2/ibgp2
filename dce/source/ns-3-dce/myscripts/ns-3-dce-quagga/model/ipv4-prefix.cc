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

#include "ns3/ipv4-prefix.h"

namespace ns3 {

Ipv4Prefix::Ipv4Prefix() {}

Ipv4Prefix::Ipv4Prefix (const char * prefix) {
    unsigned i;
    const char * pc = prefix;

    for (i = 0; *pc; ++pc, ++i) {
        if (*pc == '/') {
            std::string
            a (prefix, 0, i),
              m (pc);
            this->address = Ipv4Address (a.c_str());
            this->mask    = Ipv4Mask (m.c_str());
            break;
        }
    }
}

Ipv4Prefix::Ipv4Prefix (const ns3::Ipv4Prefix & prefix) :
    address (prefix.GetAddress()),
    mask (prefix.GetMask())
{}

Ipv4Prefix::Ipv4Prefix (
    const ns3::Ipv4Address & address,
    const ns3::Ipv4Mask & mask
) :
    address (address),
    mask (mask)
{}

const ns3::Ipv4Address & Ipv4Prefix::GetAddress() const {
    return this->address;
}

const ns3::Ipv4Mask & Ipv4Prefix::GetMask() const {
    return this->mask;
}

void Ipv4Prefix::SetAddress (const ns3::Ipv4Address & address) {
    this->address = address;
}

void Ipv4Prefix::SetMask (const ns3::Ipv4Mask & mask) {
    this->mask = mask;
}

uint16_t Ipv4Prefix::GetPrefixLength() const {
    return this->GetMask().GetPrefixLength();
}

bool Ipv4Prefix::IsEqual (const ns3::Ipv4Prefix & other) const {
    return (
       this->GetAddress().CombineMask (this->GetMask())
       == other.GetAddress().CombineMask (other.GetMask())
    ) && this->GetMask() == other.GetMask();
}

Ipv4Prefix Ipv4Prefix::Any() {
    return Ipv4Prefix (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/0"));
}

void Ipv4Prefix::Print (std::ostream & os) const {
    os << this->GetAddress() << '/' << this->GetPrefixLength();
}

ns3::Ipv4Prefix & Ipv4Prefix::operator= (const ns3::Ipv4Prefix & prefix) {
    this->SetAddress (prefix.GetAddress());
    this->SetMask (prefix.GetMask());
    return *this;
}

bool operator== (const ns3::Ipv4Prefix & a, const ns3::Ipv4Prefix & b) {
    return a.IsEqual (b);
}

bool operator!= (const ns3::Ipv4Prefix & a, const ns3::Ipv4Prefix & b) {
    return !a.IsEqual (b);
}

bool operator< (const ns3::Ipv4Prefix & a, const ns3::Ipv4Prefix & b) {
    const Ipv4Mask
          &  a_m (a.GetMask()),
          & b_m (b.GetMask());
    const Ipv4Address
          & a_a (a.GetAddress().CombineMask (a_m)),
          & b_a (b.GetAddress().CombineMask (b_m));
    return a_a < b_a
      || (a_a == b_a && a_m.GetPrefixLength() < b_m.GetPrefixLength());
}

bool operator> (const ns3::Ipv4Prefix & a, const ns3::Ipv4Prefix & b) {
    return ! (a < b) && (a != b);
}

std::ostream & operator<< (std::ostream & out, const ns3::Ipv4Prefix & prefix) {
    prefix.Print (out);
    return out;
}

}
