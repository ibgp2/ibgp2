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

#ifndef IPV4_PREFIX_H
#define IPV4_PREFIX_H

#include <cstdint>                  // uintxx_t
#include <ostream>                  // std::ostream

#include "ns3/attribute-helper.h"   // ATTRIBUTE_HELPER_HEADER
#include "ns3/ipv4-address.h"       // ns3::Ipv4Address, ns3::Ipv4Mask

namespace ns3 {

/**
 * @ingroup address
 * @class Ipv4Prefix
 * @brief Describes an IPv4 prefix. It is just a bitmask like Ipv4Mask.
 * @see Ipv4Address
 */
class Ipv4Prefix
{
    private:
        Ipv4Address address;    /**< The nested Ipv4Address. */
        Ipv4Mask    mask;       /**< The nested Ipv4Mask. */

    public:

        /**
         * @brief Default constructor.
         */

        Ipv4Prefix ();

        /**
         * @brief Constructs an Ipv4Prefix by using the input string.
         * @param prefix the 128-bit prefix
         */

        Ipv4Prefix (const char * prefix);

        /**
         * @brief Copy constructor.
         * @param prefix Ipv4Prefix object
         */

        Ipv4Prefix (const Ipv4Prefix & prefix);

        /**
         * @brief Constructor
         * @param address An Ipv4Address instance.
         * @param mask An Ipv4Mask instance.
         */

        Ipv4Prefix(const Ipv4Address & address, const Ipv4Mask & mask);

        /**
         * @brief Destructor.
         */

        ~Ipv4Prefix () {}

        /**
         * @brief Retrieve the Ipv4Address corresponding to this Ipv4Prefix.
         * @return The nested address.
         */

        const Ipv4Address & GetAddress() const;

        /**
         * @brief Retrieve the Ipv4Mask corresponding to this Ipv4Prefix.
         * @return The nested mask.
         */

        const Ipv4Mask & GetMask() const;

        /**
         * @brief Set the Ipv4Address corresponding to this Ipv4Prefix.
         * @param address The new address.
         */

        void SetAddress(const Ipv4Address & address);

        /**
         * @brief Set the Ipv4Mask corresponding to this Ipv4Prefix.
         * @param mask The new mask.
         */

        void SetMask(const Ipv4Mask & mask);

        /**
         * @brief Get prefix length.
         * @return prefix length
         */

        uint16_t GetPrefixLength () const;

        /**
         * @brief Comparison operation between two Ipv4Prefix.
         * @param other the IPv4 prefix to which to compare this prefix
         * @return true if the prefixes are equal, false otherwise
         */

        bool IsEqual (const Ipv4Prefix & other) const;

        /**
         * @brief Build the Ipv4Prefix containing any IP address.
         * @returns The Ipv4Prefix containing any IP address (0/0).
         */

        static Ipv4Prefix Any();

        /**
         * @brief Print this address to the given output stream.
         *   The print format is in the typical "2001:660:4701::1".
         * @param os the output stream to which this Ipv4Address is printed
         */

        void Print (std::ostream & os) const;

        Ipv4Prefix & operator = (const Ipv4Prefix & prefix);
};

/**
 * @class ns3::Ipv4PrefixValue
 * @brief Hold objects of type ns3::Ipv4Prefix
 */

ATTRIBUTE_HELPER_HEADER (Ipv4Prefix);   //!< Macro to make help make class an ns-3 attribute

/**
 * @brief Test whether two Ipv4Prefix are equal.
 * @param a An Ipv4Prefix instance.
 * @param b An Ipv4Prefix instance.
 * @return true iff a and b are equal.
 */

bool operator == (const Ipv4Prefix & a, const Ipv4Prefix & b);

/**
 * @brief Test whether two Ipv4Prefix are distinct.
 * @param a An Ipv4Prefix instance.
 * @param b An Ipv4Prefix instance.
 * @return true iff a and b are distinct.
 */

bool operator != (const Ipv4Prefix & a, const Ipv4Prefix & b);

/**
 * @brief Test whether an Ipv4Prefix preceeds another Ipv4Prefix.
 * @param a An Ipv4Prefix instance.
 * @param b An Ipv4Prefix instance.
 * @return true iff a < b.
 */

bool operator < (const Ipv4Prefix & a, const Ipv4Prefix & b);

/**
 * @brief Compare two Ipv4Prefix instances.
 * @param a An Ipv4Prefix instance.
 * @param b An Ipv4Prefix instance.
 * @return true iff a > b.
 */

bool operator > (const Ipv4Prefix & a, const Ipv4Prefix & b);

/**
 * @brief Write an Ipv4Prefix instance in an output stream.
 * @param os The output stream.
 * @param prefix An Ipv4Prefix instance.
 * @return The output stream.
 */

std::ostream & operator << (std::ostream & os, const Ipv4Prefix & prefix);

} // namespace ns3

#endif /* IPV4_PREFIX_H */
