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

#ifndef OSPF6_CONFIG_H
#define OSPF6_CONFIG_H

#define DEFAULT_OSPF6D_HOSTNAME "ospf6d"
#define DEFAULT_OSPF6D_VTY_PORT 2606

#include <string>                       // std::string
#include <ostream>                      // std::ostream
#include <vector>                       // std::vector

#include "ns3/ipv4-address.h"           // ns3::Ipv4Address
#include "ns3/quagga-base-config.h"     // ns3::QuaggaBaseConfig
#include "ns3/type-id.h"                // ns3::TypeId


namespace ns3
{

class Ospf6Config :
    public QuaggaBaseConfig
{
private:
    std::vector<std::string>  m_enable_if;
    Ipv4Address               router_id;      /**< The OSPF router-id of this router. */
public:

    /**
     * @brief Constructor
     * @param hostname The name of the router.
     * @param password The password used to connect to ospf6d (normal mode).
     * @param passwordEnable The password need to become root enable mode).
     * @param debug Pass true to enable usual debug instruction in the configuration file.
     */

    Ospf6Config (
        const std::string & hostname       = DEFAULT_OSPF6D_HOSTNAME,
        const std::string & password       = DEFAULT_PASSWORD,
        const std::string & passwordEnable = DEFAULT_PASSWORD_ENABLE,
        bool debug = true
    );

    /**
     * @brief Destructor.
     */

    virtual ~Ospf6Config ();

    static TypeId GetTypeId ( void );

    TypeId GetInstanceTypeId ( void ) const;

    /**
     * @brief Accessor to the router-ID assigned to this router.
     * @returns The IPv4 address of the router-ID
     */

    const Ipv4Address & GetRouterId() const;

    // FOR BACKWARD COMPATIBILITY
    void AddInterface(const std::string & name);

    /**
     * @brief Set the router-ID assigned to this router.
     * @param routerId The IPv4 address of the router-ID
     */

    void SetRouterId ( const Ipv4Address& routerId );

    /**
     * @brief Write this Ospf6Config in an output stream.
     * @param os The output stream.
     */

    virtual void Print ( std::ostream& os ) const;


};

} // end namespace ns3

#endif
