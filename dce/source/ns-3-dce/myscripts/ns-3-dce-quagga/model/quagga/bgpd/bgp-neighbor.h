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


#ifndef BGP_NEIGHBOR_H
#define BGP_NEIGHBOR_H

#include <cstdint>                  // uint32_t
#include <map>                      // std::map
#include <ostream>                  // std::ostream
#include <set>                      // std::set
#include <string>                   // std::string

#include "ns3/address.h"            // ns3::Address
#include "ns3/quagga-direction.h"   // ns3::QuaggaDirection

namespace ns3
{

class BgpNeighbor
{
private:
    typedef std::set<QuaggaDirection>           Directions;
    typedef std::map<std::string, Directions>   PrefixLists;
    typedef std::map<std::string, Directions>   AccessLists;
    typedef std::map<std::string, Directions>   RouteMaps;

    Address     m_address;              // Read only
    uint32_t    m_remoteAs;             /**< AS number of the AS containing the BGP neighboring router. */
    std::string m_description;          /**< Description of the BGP neighboring router. */
    bool        m_enableRrClient;       /**< Pass true if the neighboring router is a Route Reflector client of this router. */
    bool        m_enableNexthopSelf;    /**< Pass true if this router must overwrite the IP of the BGP nexthop using one of its own IP. */
    bool        m_updateSource;         /**< Pass true if this router must overwrite the IP source used to send BGP packets */
    Address     m_updateSourceAddress;  /**< Set the IP address use to overwrite the IP source used to send BGP packets. */
    bool        m_defaultOriginate;     /**< Pass true if this router must announce a default route. */
    PrefixLists m_prefixLists;          /**< The prefix-lists attached to this neighbor. */
    AccessLists m_accessLists;          /**< The access-lists attached to this neighbor. */
    RouteMaps   m_routeMaps;            /**< The route-maps attached to this neighbor. */
public:

    /**
     * @brief Constructor.
     */

    BgpNeighbor();

    /**
     * @brief Constructor.
     * @param address The IP address of the neighboring BGP router.
     * @param asn The AS number of the neighboring router.
     * @param description Description of the neighboring router.
     */

    BgpNeighbor (
        const Address & address,
        const uint32_t & asn,
        const std::string & description = ""
    );

    /**
     * @brief Copy constructor.
     * @param neighbor A BgpNeighbor instance.
     */

    BgpNeighbor ( const BgpNeighbor & neighbor );

    /**
     * @brief Set the AS Number of the neighboring BGP router.
     * @param asn An AS number.
     */

    void SetRemoteAs ( uint32_t asn );

    /**
     * @brief Accessor to the AS Number of the neighboring router.
     * @return The corresponding ASN.
     */

    uint32_t GetRemoteAs() const;

    /**
     * @brief Retrieve the IP address used to identify this BGP neighboring router.
     * @return The corresponding IP address.
     */

    const Address & GetAddress() const;

    /**
     * @brief Set the description corresponding to this BGP neighboring router.
     * @param description The description.
     */

    void SetDescription ( const std::string & description );

    /**
     * @brief Get the description corresponding to this BGP neighboring router.
     * @return The description.
     */

    const std::string & GetDescription() const;

    /**
     * @brief Enable or disable the route reflection for this BGP neighboring router.
     * @param on Pass true to enable route reflection, false otherwise.
     */

    void SetRouteReflectorClient ( bool on = true );

    /**
     * @brief Test whether the route reflection is enabled for this BGP neighboring router.
     * @return true if route reflection is enabled.
     */

    bool GetRouteReflectorClient() const;

    /**
     * @brief Enable or disable the next-hop-self for this BGP neighboring router.
     * @param on Pass true to enable next-hop-self, false otherwise.
     */

    void SetNextHopSelf ( bool on = true );

    /**
     * @brief Test whether next-hop-self is enabled for this BGP neighboring router.
     * @return true if next-hop-self is enabled.
     */

    bool GetNextHopSelf() const;

    /**
     * @brief Enable update-source command for this BGP neighboring router. This IP should
     *    be used to identify this router in the configuration file of the neighboring
     *    router.
     * @param address The IP address used for this command. Usually it is the an IP address
     *    assigned to a dummy interface and reannounced in OSPF or IS-IS. Hence the BGP
     *    session does not fall in case of failure of physical interface. Also it allows
     *    to easily establish multi-hop session.
     */

    void SetUpdateSource ( const Address & address );

    /**
     * @brief Disable update-source command for this BGP neighboring router.
     */

    void UnsetUpdateSource();

    /**
     * @brief Test whether update-source command is enabled for this BGP neighboring router.
     * @returns true iif update-source is enabled.
     */

    bool GetUpdateSource() const;

    /**
     * @brief Retrive the IP address used in the update-source command (if enabled) for this
     *   BGP neighboring router.
     * @returns The corresponding IP address.
     */

    const Address & GetUpdateSourceAddress() const;

    /**
     * @brief Enable or disable the default-originate for this BGP neighboring router.
     * @param on Pass true to enable default-originate, false otherwise.
     */

    void SetDefaultOriginate ( bool on = true );

    /**
     * @brief Test whether default-originate is enabled for this BGP neighboring router.
     * @return true if default-originate is enabled.
     */

    bool GetDefaultOriginate() const;

    /**
     * @brief Attach an access-list filter to this bgp-neighbor.
     *   Do not forget to add the corresponding access-list in BgpConfig (see BgpConfig::AddAccessList.
     * @param filterName The name of the filter.
     * @param direction The direction on which the filter applies.
     */

    void AddAccessList(const std::string & filterName, const QuaggaDirection & direction);

    /**
     * @brief Attach a prefix-list filter to this bgp-neighbor.
     *   Do not forget to add the corresponding prefix-list in BgpConfig (see BgpConfig::AddPrefixList).
     * @param filterName The name of the filter.
     * @param direction The direction on which the filter applies.
     */

    void AddPrefixList(const std::string & filterName, const QuaggaDirection & direction);

    /**
     * @brief Attach a route-map to this bgp-neighbor.
     *   Do not forget to add the corresponding route-map in BgpConfig (see BgpConfig::AddRouteMap).
     * @param routemapName The name of the route-map.
     * @param direction The direction on which the route-map applies.
     */

    void AddRouteMap(const std::string & routemapName, const QuaggaDirection & direction);

    /**
     * @brief Print this BgpNeighbor in an output stream.
     * @param os The output stream.
     */

    virtual void Print(std::ostream & os) const;
};

/**
 * \brief Write an BgpNeighbor in an output stream.
 * \param os The output stream.
 * \param interface An BgpNeighbor.
 * \return The output stream.
 */

std::ostream & operator << ( std::ostream & os, const BgpNeighbor & neighbor );

} // end namespace ns3

#endif
