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

#ifndef BGP_CONFIG_H
#define BGP_CONFIG_H

#define DEFAULT_BGPD_HOSTNAME  "bgpd"
#define DEFAULT_BGPD_VTY_PORT  2605

#include <map>                          // std::map
#include <set>                          // std::set
#include <string>                       // std::string

#include "ns3/quagga-base-config.h"     // ns3::QuaggaBaseConfig
#include "ns3/quagga-redistribute.h"    // REDISTRIBUTE_*
#include "ns3/quagga-utils.h"           // CompareIpv6Prefix
#include "ns3/ipv4-address.h"           // ns3::Ipv4Address
#include "ns3/ipv4-prefix.h"            // ns3::Ipv4Prefix
#include "ns3/ipv6-address.h"           // ns3::Ipv6Prefix
#include "ns3/bgp-neighbor.h"           // ns3::BgpNeighbor
#include "ns3/type-id.h"                // ns3::TypeId


namespace ns3
{

class BgpConfig :
    public QuaggaBaseConfig
{
private:
    // Types
    typedef uint32_t                                    Asn;
    typedef Address                                     RouterId;
    typedef std::map<Ipv4Address, BgpNeighbor>          NeighborsV4;
    typedef std::map<Ipv6Address, BgpNeighbor>          NeighborsV6;
    typedef std::set<Ipv4Prefix>                        NetworksV4;
    typedef std::set<Ipv6Prefix, CompareIpv6Prefix>     NetworksV6;

    ///////// << TO REMOVE (FOR BACKWARD COMPATIBILITY)
    typedef std::list<std::string>                      networks_t;
    networks_t      networks;               /**< The prefixes announced by this router with BGP. */
    ///////// >> TO REMOVE

    // Members
    bool         m_synchronization;
    Asn          m_asn;                   /**< ASN of this BGP router. */
    RouterId     m_routerId;              /**< bgp router-id. */
    uint8_t      m_maskRedistribute;      /**< Mask specifying which routes must be redistributed. */
    NeighborsV4  m_neighborsV4;           /**< BGP neighbors (IPv4). */
    NeighborsV6  m_neighborsV6;           /**< BGP neighbors (IPv6). */
    NetworksV4   m_networksV4;            /**< IPv4 prefixes announced by this router. */
    NetworksV6   m_networksV6;            /**< IPv6 prefixes announced by this router. */

public:

    /**
     * \brief Constructor
     * \param hostname The name of the router.
     * \param password The password used to connect to bgpd (normal mode).
     * \param passwordEnable The password need to become root enable mode).
     * \param debug Pass true to enable usual debug instruction in the configuration file.
     */

    BgpConfig (
        const std::string & hostname       = DEFAULT_BGPD_HOSTNAME,
        const std::string & password       = DEFAULT_PASSWORD,
        const std::string & passwordEnable = DEFAULT_PASSWORD_ENABLE,
        bool debug = true
    );

    /**
     * \brief Destructor.
     */

    virtual ~BgpConfig ();

    static TypeId GetTypeId ();

    TypeId GetInstanceTypeId () const;

    /**
     * \brief Set the AS Number corresponding to this BGP router.
     * \param asn The AS Number (>0).
     */

    void SetAsn ( uint32_t asn );

    /**
     * \brief Retrieve the AS Number corresponding to this BGP router.
     * \returns The corresponding AS Number (>0).
     */

    uint32_t GetAsn() const;

    /**
     * \brief Enable or disable synchronization
     * \param on Pass true to enable synchronization, false otherwise.
     */

    bool SetSynchronization ( bool on = true );

    /**
     * \brief Accessor to the state of the synchronization feature.
     * \returns true iif synchronization is enabled.
     */

    bool GetSynchronization() const;

    /**
     * \brief Specifies which route must be redistributed in BGP.
     * \param mask The new mask.
     * Example: to redistribute connected route and OSPF routes, pass
     *   REDISTRIBUTE_CONNECTED|REDISTRIBUTE_OSPF
     */

    void SetRedistribute ( uint8_t mask );

    /**
     * \returns The mask specifiying which kind of route must be
     *   redistributed in BGP.
     * Example: to test whether connected routes are redistributed,
     *   (bgpConfig.GetRedistribute() & REDISTRIBUTE_CONNECTED)
     */

    uint8_t GetRedistribute() const;

    /**
     * \brief Set the BGP router id of this BGP router.
     * \param routerId The new router-id.
     */

    void SetRouterId ( const Address & routerId );

    /**
     * \brief Retrieve the BGP router id of this BGP router.
     * \returns The corresponding router-id.
     */

    const RouterId & GetRouterId() const;

    /**
     * \brief Configure a BGP neighbor on this BGP router.
     * \param neighbor The parameters characterizing the neighbor.
     */

    bool AddNeighbor ( const BgpNeighbor & neighbor );

    /**
     * \brief Retrieve the BgpNeighbor related to a given IP address.
     * \param address The IP address used to identify the remote BGP peer
     *    in the configuration file.
     * \throws std::runtime_error if not found (or if invalid type address).
     * \returns The corresponding BgpNeighbor.
     */

    BgpNeighbor & GetNeighbor ( const Address & address );

    /**
     * \brief Configure a BGP network announcement on this router.
     * \param prefix The IPv4 prefix describing the target destinations.
     */

    void AddNetwork ( const Ipv4Prefix & prefix );

    /**
     * \brief Configure a BGP network announcement on this router.
     * \param prefix The IPv6 prefix describing the target destinations.
     */

    void AddNetwork ( const Ipv6Prefix & prefix );

    // TODO AddRouteMap
    // TODO AddAccessList to support ip as-path access-list

    /**
    * \brief Write the BGP configuration file in an output stream.
    * \param os The output stream.
    */

    virtual void Print ( std::ostream & os ) const;

    //-----------------------------------------------------------------
    // Methods kept to remain backward-compatible
    //-----------------------------------------------------------------

    bool AddNeighbor ( const std::string & neighborIp, uint32_t asn, const std::string & description );
    void AddNetwork ( const std::string & prefix );

};

} // end namespace ns3

#endif

