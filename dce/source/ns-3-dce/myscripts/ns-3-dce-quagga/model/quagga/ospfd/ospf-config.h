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

#ifndef OSPF_CONFIG
#define OSPF_CONFIG

#define DEFAULT_OSPFD_HOSTNAME   "ospf"
#define DEFAULT_OSPFD_VTY_PORT   2604
#define OSPF_DUMMY_ROUTER_ID "0.0.0.0"

#include <list>                         // std::list
#include <map>                          // std::map
#include <set>                          // std::set
#include <string>                       // std::string

#include "ns3/quagga-base-config.h"     // ns3::QuaggaBaseConfig
#include "ns3/ipv4-address.h"           // ns3::Ipv4Address
#include "ns3/ipv4-prefix.h"            // ns3::Ipv4Prefix
#include "ns3/ospf-interface.h"         // ns3::OspfInterface
#include "ns3/ospf-redistribute.h"      // ns3::OspfRedistribute
#include "ns3/ospf-distribute-list.h"   // ns3::OspfDistributeList
#include "ns3/type-id.h"                // ns3::TypeId

namespace ns3 {

class OspfConfig :
    public QuaggaBaseConfig
{
private:
    typedef std::map<Ipv4Prefix, Ipv4Address>       NetworksV4;
    typedef std::map<std::string, OspfInterface>    Interfaces;
    typedef std::set<OspfRedistribute>              Redistributes;
    typedef std::set<OspfDistributeList>            DistributeLists;

    NetworksV4      m_networks;         /**< Maps a prefix and the corresponding area in which we can redistribute it. */
    Interfaces      m_interfaces;       /**< Maps for each interface identifier its corresponding OSPF weight. */
    Ipv4Address     m_routerId;         /**< The OSPF router-id of this router. */
    Redistributes   m_redistributes;    /**< Specify which kind of routes are redistributed in OSPF. */
    DistributeLists m_distributeLists;  /**< distribute-list configured on this OSPF router. */

public:

    /**
     * @brief Constructor.
     * @param hostname The name of the router.
     * @param password The password used to connect to ospfd (normal mode).
     * @param passwordEnable The password need to become root enable mode).
     * @param debug Pass true to enable usual debug instruction in the configuration file.
     */

    OspfConfig (
        const std::string & hostname       = DEFAULT_OSPFD_HOSTNAME,
        const std::string & password       = DEFAULT_PASSWORD,
        const std::string & passwordEnable = DEFAULT_PASSWORD_ENABLE,
        bool debug = true
    );

    /**
     * @brief Destructor.
     */

    virtual ~OspfConfig ();

    static TypeId GetTypeId ();

    TypeId GetInstanceTypeId() const;

    /**
     * @brief Accessor to the router-ID assigned to this router.
     * @returns The IPv4 address of the router-ID
     */

    const Ipv4Address & GetRouterId() const;

    /**
     * @brief Set the router-ID assigned to this router.
     * @param routerId The IPv4 address of the router-ID
     */

    void SetRouterId ( const Ipv4Address & routerId );

    /**
     * @brief Configure an OSPF network. All the interfaces falling
     *    in this prefix will speak OSPF.
     * @param prefix The corresponding prefix.
     * @param area The OSPF area (for instance 0.0.0.0)
     */

    void AddNetwork ( const Ipv4Prefix & prefix, const Ipv4Address & area = Ipv4Address ( "0.0.0.0" ));

    /**
     * @brief Redistribute some route in OSPF. It could be used for instance
     *    to reannounce a static route in OSPF.
     * @param redistribute An OspfRedistribute instance.
     */

    void AddRedistribute ( const OspfRedistribute & redistribute );

    /**
     * @brief Redistribute a kind of route in OSPF
     * @param x A REDISTRIBUTE_* constant (except REDISTRIBUTE_OSPF).
     *    This is not a mask so pass a single REDISTRIBUTE_* constant.
     */

    void SetRedistribute ( uint8_t x );

    /**
     * @brief Add a distribute-list to the configuration file.
     *   Do not forget to add the corresponding AccessList in OspfConfig.
     * @param distributeList An OspfDistributeList instance.
     */

    void AddDistributeList(const OspfDistributeList & distributeList);

    /**
     * @brief Add an interface with a named prefixed "ns3-device".
     * @param interface The parameter of the interface.
     */

    void AddInterface ( const OspfInterface & interface );

    /**
     * @brief Write this OspfConfig in an output stream.
     * @param os The output stream.
     */

    virtual void Print ( std::ostream & os ) const;

    static std::string MakeInterfaceName(uint32_t ifn);
};

} // end namespace ns3

#endif
