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

#ifndef ZEBRA_CONFIG
#define ZEBRA_CONFIG

#define DEFAULT_ZEBRA_HOSTNAME "zebra"
#define DEFAULT_ZEBRA_SERVICE_PORT 2600
#define DEFAULT_ZEBRA_VTY_PORT 2601

#include <map>                          // std::map
#include <set>                          // std::set
#include <string>                       // std::string

#include "ns3/quagga-base-config.h"     // ns3::QuaggaBaseConfig
#include "ns3/ipv4-address.h"           // ns3::Ipv4Address
#include "ns3/ipv4-prefix.h"            // ns3::Ipv4Prefix
#include "ns3/zebra-interface.h"        // ns3::ZebraInterface
#include "ns3/type-id.h"                // ns3::TypeId

namespace ns3
{

class ZebraConfig :
    public QuaggaBaseConfig
{
public:

    typedef std::map<Ipv4Address, std::set<Ipv4Prefix> >   static_routes_v4_t;
    typedef std::map<std::string, ZebraInterface>          interfaces_t;

private:

    std::map<std::string, std::string>      m_radvd_if;         /**< FOR BACKWARD COMPATILIBILITY. */
    std::set<std::string>                   m_haflag_if;        /**< FOR BACKWARD COMPATILIBILITY Set of interfaces enabling Home Agent Flag. */
    interfaces_t                            m_interfaces;       /**< Set of IP assign to the loopback. */
    static_routes_v4_t                      m_static_routes_v4; /**< Set static IPv4 routes. */

public:

    /**
     * @brief Constructor
     * @param hostname The name of the router.
     * @param password The password used to connect to zebra (normal mode).
     * @param passwordEnable The password need to become root enable mode).
     * @param debug Pass true to enable usual debug instruction in the configuration file.
     */

    ZebraConfig (
        const std::string & hostname       = DEFAULT_ZEBRA_HOSTNAME,
        const std::string & password       = DEFAULT_PASSWORD,
        const std::string & passwordEnable = DEFAULT_PASSWORD_ENABLE,
        bool debug = true
    );

    /**
     * @brief Destructor.
     */

    ~ZebraConfig ();

    static TypeId GetTypeId ();
    TypeId GetInstanceTypeId () const;

    /**
     * @brief Configure a new interface in Zebra.
     * @param name The name of the interface.
     * @param interface The parameters of this interface.
     */

    void AddInterface ( const std::string & name, const ZebraInterface & interface );

    /**
     * @brief Configure a static route.
     * @param gateway The IPv4 address of the out interface.
     * @param prefix The target IPv4 prefix.
     */

    void AddStaticRoute ( const Ipv4Prefix & prefix, const Ipv4Address & gateway );

//    void AddLoopback ( const std::string & name, const Address & address );

    /**
     * @brief Write this ZebraConfig in an output stream.
     * @param os The output stream.
     */

    virtual void Print ( std::ostream & os ) const;

    // FOR BACKWARD COMPATILIBILITY

    void AddRadvdIf ( const std::string & ipInterface, const std::string & prefix );

    void EnableHomeAgentFlag ( const std::string & ipInterface );

};

} // end namespace ns3

#endif
