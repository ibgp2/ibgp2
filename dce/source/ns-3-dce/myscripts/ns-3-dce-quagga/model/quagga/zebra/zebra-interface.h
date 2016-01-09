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

#ifndef QUAGGA_QUAGGA_INTERFACE_H
#define QUAGGA_QUAGGA_INTERFACE_H

#include <set>                  // std::set
#include <string>               // std::string
#include <ostream>              // std::ostream, std::endl

#include "ns3/ipv4-prefix.h"    // ns3::Ipv4Prefix
#include "ns3/ipv6-address.h"   // ns3::Ipv6Prefix
#include "ns3/quagga-utils.h"   // CompareIpv6Prefix

namespace ns3
{

class ZebraInterface
{
public:
    typedef std::set<Ipv4Prefix> prefixes_v4_t;
    typedef std::set<Ipv6Prefix, CompareIpv6Prefix> prefixes_v6_t;

private:
    std::string   name;        // Read only
    std::string   description; /**< Description of this interface. */
    prefixes_v4_t prefixes_v4; /**< IPv4 prefixes corresponding to this interface. */
    prefixes_v6_t prefixes_v6; /**< IPv6 prefixes corresponding to this interface. */
    bool          link_detect; /**< Indicates whether the "link-detect" feature is enabled. */

public:

    /**
     * \brief Default constructor.
     */

    ZebraInterface() {}

    /**
     * \brief Constructor.
     * \param name The name of the interface (for instance ns3-device0).
     * \param description An optional description of the interface
     */

    ZebraInterface (
        const std::string & name,
        const std::string & description = ""
    );

    /**
     * \returns The name related to this QuaggaInterface.
     */

    const std::string & GetName() const;

    /**
     * \brief Set the name of this QuaggaInterface
     * \param name The new name.
     */

    void SetName ( const std::string & name );

    /**
     * \returns The description related to this QuaggaInterface.
     */

    const std::string & GetDescription() const;

    /**
     * \brief Assign a new IPv4 prefix to this QuaggaInterface
     * \param prefix An IPv4 prefix
     */

    void AddPrefix ( const Ipv4Prefix & prefix );

    /**
     * \brief Assign a new IPv6 prefix to this QuaggaInterface
     * \param prefix An IPv6 prefix
     */

    void AddPrefix ( const Ipv6Prefix & prefix );

    /**
     * \returns The set of IPv4 prefixes assigned to this QuaggaInterface.
     */

    const prefixes_v4_t & GetPrefixesV4() const;

    /**
     * \returns The set of IPv6 prefixes assigned to this QuaggaInterface.
     */

    const prefixes_v6_t & GetPrefixesV6() const;

    /**
     * \returns true iif the link-detect feature is enabled on this QuaggaInterface.
     */

    bool GetLinkDetect() const;

    /**
     * \brief Set or unset the link-detect feature is enabled on this QuaggaInterface.
     * \param newState The new state of link-detect.
     */

    void SetLinKDetect(bool newState);

    /**
     * \brief Write this QuaggaInterface into an output stream.
     * \param os The output stream.
     */

    virtual std::ostream & Print ( std::ostream & out ) const;
};

/**
 * \brief Write a QuaggaInterface into an output stream.
 * \param os The output stream.
 * \param interface A QuaggaInterface instance.
 */

std::ostream & operator << ( std::ostream & out, const ZebraInterface & interface );

} // namespace ns3

#endif // QUAGGA_QUAGGA_INTERFACE_H
