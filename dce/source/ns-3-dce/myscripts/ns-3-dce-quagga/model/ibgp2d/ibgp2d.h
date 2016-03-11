/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Alexandre Morignot, Marc-Olivier Buob
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
 * Authors:
 *    Alexandre Morignot <alexandre.morignot@orange.fr>
 *    Marc-Olivier Buob <marcolivier.buob@orange.fr>
 */

#ifndef IBGP2D_H
#define IBGP2D_H

#define IBGP2_DUMMY_NID           "0.0.0.0"
#define IBGP2_ROUTE_MAP_PREFIX    "ROUTE-MAP-"
#define IBGP2_ACCESS_LIST_PREFIX  "ACCESS-LIST-"

#include <map>                      // std::map
#include <vector>                   // std::vector

#include "ns3/application.h"        // ns3::Application
#include "ns3/ipv4-address.h"       // ns3::Ipv4Address
#include "ns3/packet.h"             // ns3::Packet
#include "ns3/ptr.h"                // ns3::Ptr
#include "ns3/socket.h"             // ns3::Socket

#include "../helper/ospf-graph-helper.h"    // ns3::OspfGraphHelper
#include "../ipv4-prefix.h"                 // ns3::Ipv4Prefix
#include "../telnet-wrapper.h"              // ns3::Telnet

namespace ns3 {

/**
 * \ingroup applications
 * \brief iBGP2 daemon.
 * The current implementation is specific to IPv4/OSPF. This daemon also support
 * any IGP link state such as IS-IS and IPv6.
 *
 * The daemon sniffes OSPF packets, and reconfigure consequently BGP daemon in
 * order to mimic the iBGP2 diffusion rule:
 *
 * "A BGP route issued by a nexthop (eventually an ASBR nexthop self) n
 * is redistributed by the router u to a neighboring router v (in the
 * OSPF/iBGP2 graph) graph if and only if the successor of v in the
 * shortest path from v to n is u."
 *
 *       spf(v -> n)[1] == u
 *
 * The iBGP2 deamon of u maintains the OSPF graph perceived by u and
 * adapt dynamically its iBGP filters for each potential nexthop n and
 * for each neighbor v.
 *
 * If the OSPF graph topology changes, iBGP2 updates consequently the
 * iBGP filters configured on u.
 *
 * WARNING:
 * - This implementation assumes that the underlying bgp daemon the one
 *   provided by quagga.
 * - It assumes that the OSPF router-id assigned to a router remains constant
 *   during the whole simulation. Therefore, the router-id of u is queried only
 *   once. It is needed to detect which nodes of the OSPF graph are its
 *   OSPF/iBGP2 neighbors.
 * - If the router-id of a neighbor v changes, some obsolete filters
 *   may be kept in memory (and in the bgpd configuration).
 */

class Ibgp2d :
    public Application
{
public:
    typedef uint32_t FilterId;
private:

    //-----------------------------------------------------------------
    // Types
    //-----------------------------------------------------------------

    typedef OspfGraphHelper::vd_t   vd_t;
    typedef OspfGraphHelper::vb_t   vb_t;
    typedef OspfGraphHelper::oeit_t oeit_t;
    typedef OspfGraphHelper::ed_t   ed_t;
    typedef Ipv4Address rid_t;  /**< OSPF router-id (identifies a router in the OSPF graph). */
    typedef Ipv4Address nid_t;  /**< OSPF network link-id (identifies a network in the OSPF graph). */

    typedef std::map<rid_t, std::set<Ipv4Prefix> >    MapFilters;
    typedef std::map<rid_t, FilterId>                 MapFilterId;

    //-----------------------------------------------------------------
    // Members
    //-----------------------------------------------------------------

    // BGPd
    Telnet *                m_telnetBgp;        /**< Telnet connection to the bgpd running on the Node. */
    uint32_t                m_asn;              /**< AS number of the Node. */

    // OSPF
    Ptr<OspfGraphHelper>    m_ospfGraphHelper;  /**< IGP graph helper. */
    rid_t                   m_routerId;         /**< OSPF router-id of the Node. */

    // For each router, store a set of networks from which transmission of
    // BGP announcements is allowed. We identify routers by their IPv4 address
    // (used by BGP), not their router ID.

    MapFilters              m_mapFilters;        /**< BGP filters that will be installed by iBGP2. */
    MapFilters              m_mapFiltersPrev;    /**< Filters previously installed, needed to make a diff. */

    // iBGP2 manages an access-list per IGP/iBGP2 neighbor, identified by an
    // integer. This mapping and the last used identifier are stored.

    MapFilterId             m_mapFilterId;      /**< Mapping neighbor / access-list identifier. */
    FilterId                m_lastFilterId;     /**< Last used access-list identifier. */

    bool                    m_bgpdWasRunning;

    //-----------------------------------------------------------------
    // Application methods
    //-----------------------------------------------------------------

    virtual void StartApplication ();
    virtual void StopApplication ();

    //-----------------------------------------------------------------
    // Packet handling
    //-----------------------------------------------------------------

    /**
     * @brief Sink to handle a packet.
     * Sink that receive packets. Used with the NetDevice "Sniffer" source.
     * @param p The packet to handle.
     */

    void HandlePacket (const Ptr<const Packet> p);

    //-----------------------------------------------------------------
    // Filters
    //-----------------------------------------------------------------

    /**
     * @brief Retrieve the filter id assigned to a given neighboring router.
     *   This id is use to identify the corresponding route-map and the
     *   corresponding filter.
     * @param rid_v The OSPF router-id of a neighbor of the router embedding
     *   this iBGP2d instance.
     * @returns The corresponding FilterId (>1), 0 otherwise.
     */

    Ibgp2d::FilterId GetFilterId(const rid_t & rid_v) const;

    /**
     * @brief Assign a new filter identifier to a neighbor.This id is use
     *   to identify the corresponding route-map and the corresponding filter
     * @param rid_v The OSPF router-id of a neighbor of the router embedding
     *   this iBGP2d instance.
     * @return The filter identifier assigned to this neighbor.
     */

    const Ibgp2d::FilterId & AssignFilterId(const rid_t & rid_v);

    /**
     * @brief Compute for each neighbor v which external IGP networks contains
     *    (potential) BGP nexthop(s) n that must be announced to v. Indeed
     *    we assume in this implementation that BGP nexthop are always in such
     *    a network.
     * @returns A map which associates neighbors' router-id with the
     *    corresponding external IGP networks.
     */

    void UpdateIbgp2Redistribution();

    /**
     * @brief Update filters installed on each iBGP2 session accordingly the
     *   iBGP2 diffusion criterion.
     */

    bool UpdateBgpConfiguration ();

    /**
     * @brief Perform a "clear ip bgp ... soft" over a set of neighbors.
     * @param neighborsAltered The set of iBGP2 neighbors that must be
     *   refreshed.
     */

    void RefreshIbgp2Neighbors(std::set<Ipv4Address> & neighborsAltered);

    /**
     * @brief Write in an output stream the quagga commands that must be issued
     *   in bgpd (in "configure terminal mode") to update the iBGP routing policy
     *   in order to mimic the iBGP2 diffusion criterion.
     * @param os The output stream.
     * @param alteredNeighbors The set of IpAddress (used in the configuration
     *   file) corresponding to the iBGP2 peers altered.
     * @return alteredNeighbors.size()
     */

    size_t WriteIbgp2Filters(std::ostream & os, std::set<Ipv4Address> & alteredNeighbors);

    /**
     * @brief Build a route-map identifier.
     * @param filterId The filter identifier which will uses the route-map.
     * @return The corresponding route-map name
     *   (used in bgpd configuration file).
     */

    static std::string MakeRouteMapName(const FilterId & filterId);

    /**
     * @brief Build a access-list identifier.
     * @param filterId The filter identifier which will uses the access-list.
     * @return The corresponding access-list name (used in bgpd configuration file).
     */

    static std::string MakeAccessListName(const FilterId & filterId);

public:

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */

    static TypeId GetTypeId (void);

    /**
     * @brief Constructor.
     * WARNING: some attributes of this iBGP2d instance must be initialized.
     * @sa SetConnectionBgpd
     * @sa SetConnectionOspfd
     * @sa SetAsn
     * @sa SetRouterId (if not done, iBGP2d connect to Ospfd
     *   to retrieve this information).
     */

    Ibgp2d ();

    /**
     * @brief Destructor.
     */

    virtual ~Ibgp2d ();

    /**
     * @brief Set the ASN assigned to the Node embedding this iBGP2d
     *   instance..
     * @param asn The AS number corresponding to this router.
     */

    void SetAsn(uint32_t asn);

    /**
     * @brief Retrieve the ASN configured for this iBGP2d instance.
     * @returns The corresponding ASN
     */

    uint32_t GetAsn() const;

    /**
     * @brief Set the router-id assigned to the Node embedding this Ibgp2d
     *    instance.
     * @param routerId The router-id corresponding to this router.
     */

    void SetRouterId(const rid_t & routerId);

    /**
     * @brief Retrieve the router-id configured for this iBGP2d instance.
     * @returns The corresponding router-id;
     */

    const rid_t& GetRouterId() const;

    /**
     * @brief Accessor to the OSPF graph managed by this IBgpController.
     * @returns The corresponding pointer.
     */

    const OspfGraphHelper * GetOspfGraphHelper() const;

    /**
     * @brief Authenticate iBGP2d to BGPd.
     */

    void BgpdConnect();

    /**
     * @brief Disconnect iBGP2 from BGPd.
     */

    void BgpdDisconnect();

    /**
     * @brief Write in an output stream the quagga commands required to
     *    enter in the BGP configuration (terminal mode).
     * @param os The output stream.
     */

    void BgpWriteBegin(std::ostream & os) const;

    /**
     * @brief Write in an output stream the quagga commands to configure
     *   an iBGP2 peer.
     * @param os The output stream.
     * @param rid_v The router-id of the neighboring router v.
     * @param nexthopPrefixesEnabled The prefixes containing the nexthops n
     *   such as (n, u, v) now/still satisfies the iBGP2 criterion.
     * @param nexthopPrefixesDisabled The prefixes containing the nexthops n
     *   such as (n, u, v) does not satisfy the iBGP2 criterion anymore.
     */

    void BgpWriteIbgp2Peer(
        std::ostream & os,
        const rid_t & rid_v,
        const std::set<Ipv4Prefix> & nexthopPrefixesEnabled,
        const std::set<Ipv4Prefix> & nexthopPrefixesDisabled
    );

};

} // namespace ns3

#endif // IBGP_CONTROLLER_H
