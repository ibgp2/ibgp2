/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Alexandre Morignot, Marc-Olivier Buob
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
 *   Alexandre Morignot <alexandre.morignot@orange.fr>
 *   Marc-Olivier Buob <marcolivier.buob@orange.fr>
 */

#ifndef OSPF_GRAPH_HELPER_H
#define OSPF_GRAPH_HELPER_H

#include <map>                  // std::map
#include <set>                  // std::set
#include <vector>               // std::vector

#include "ns3/ipv4-address.h"   // ns3::Ipv4Address
#include "ns3/object.h"         // ns3::Object

#include "../model/ospf-graph/graph-builder.h"  // ns3::ospf::OspfGraphBuilder
#include "../model/ospf-graph/ospf-graph.h"     // ns3::ospf::OspfGraph
#include "../model/ospf-graph/graph-builder.h"  // ns3::ospf::OspfGraph

namespace ns3
{

/**
 * @brief Help to manage an OspfGraph.
 */

class OspfGraphHelper :
    public Object
{
public:

    // Typedef vertices
    typedef ospf::OspfGraphBuilder::vertex_iterator_t   vit_t;
    typedef ospf::OspfGraphBuilder::vertex_descriptor_t vd_t;
    typedef ospf::OspfGraphBuilder::vertex_bundled_t    vb_t; // == OspfVertex

    // Typedef edges
    typedef ospf::OspfGraphBuilder::edge_iterator_t     eit_t;
    typedef ospf::OspfGraphBuilder::out_edge_iterator_t oeit_t;
    typedef ospf::OspfGraphBuilder::edge_descriptor_t   ed_t;
    typedef ospf::OspfGraphBuilder::edge_bundled_t      eb_t; // == OspfEdge

    // Typedef OSPF
    typedef Ipv4Address                         nid_t;          /**< Identifies a OSPF link. */
    typedef Ipv4Address                         rid_t;          /**< Identifies a OSPF node. */
    typedef std::pair<rid_t, nid_t>             OspfArc;        /**< Metrics are assigned from a node to a connected network (arc). */
    typedef ospf::metric_t                      OspfMetric;     /**< Metric assigned to an OspfArc. */

    typedef std::map<nid_t, std::set<rid_t> >   MapOspfNetwork; // TODO TO REMOVE
    typedef std::map<nid_t, Ipv4Prefix>         MapNetwork;
    typedef std::map<rid_t, std::set<nid_t> >   MapExternalNetwork;

private:

    ospf::router_id_t                   m_routerId; //!< OSPF router ID // TODO to remove DEBUG

    MapOspfNetwork                      m_mapOspfNetworks;      /**< List of networks that are ospf links and the routers that are part of them. */ // TODO to remove

    std::map<OspfArc, OspfMetric>       m_mapMetrics;           /**< List of metrics for each OspfArc. */
    std::map<OspfArc, Ipv4Address>      m_mapInterfaces;        /**< List of interface addresses for OspfArc. */

    // Deduced from LSA Router messages
    ospf::OspfGraph                     m_gospf;                /**< OSPF graph. */
    ospf::OspfGraphBuilder              m_gbOspf;               /**< OSPF graph builder. */

    // Deduced from LSA networks messages
    MapNetwork                          m_mapNetworks;          /**< Map for each nid_t the corresponding IPv4 prefix. */

    // Deduced from LSA external networks messages
    MapExternalNetwork                  m_mapExternalNetworks;  /**< List of external networks and the router-id of the corresponding ASBR. */
public:

    /**
     * @brief Constructor.
     */

    OspfGraphHelper ();

    /**
     * @brief Destructor.
     */

    ~OspfGraphHelper ();

    /**
     * @brief Add the arc (u,v) if it doesn not exist, and add the network n with the
     *   metric m to the edge.
     * @param u The router ID of the source of the arc.
     * @param v The router ID of the target of the arc.
     * @param n The network related to this arc.
     * @param i The IPv4 address of the interface of u connected to the network.
     * @param m The metric related to this arc.
     * @return true is the graph was modified, else false
     */

    bool AddAdjacency (
        const rid_t          & u,
        const rid_t          & v,
        const nid_t          & n,
        const Ipv4Address    & i,
        const ospf::metric_t & m
    );

    /**
     * @brief Remove a router from a network, and if necessary remove some edges.
     * @param u The router to remove
     * @param n The network
     * @return true is the graph was modified, else false
     */

    bool RemoveAdjacency (
        const rid_t & u,
        const nid_t & n
    );

    /**
     * @brief Remove a the network n from the edge (u,v), and if there is no more network
     * on the edge remove it.
     * @param u The source
     * @param v The destination
     * @param n The network
     * @return true is the graph was modified, else false
     */

    bool RemoveAdjacency (
        const rid_t & u,
        const rid_t & v,
        const nid_t & n
    );

    /**
     * @brief Set the OSPF router-id (this is only for debug purpose).
     * @param routerId The router-id.
     */

    // TODO to remove
    void SetRouterId(const rid_t & routerId);

    /**
     * @brief Handle a list of OSPF LSA, and consequently add or remove
     *   (if needed) edge and / or vertex.
     * @param lsas the list of OSPF Router LSA
     * @return true is the graph has been altered.
     */

    bool HandleLsa (std::vector<OspfLsa *> & lsas);

    /**
     * @brief Handle an OSPF Router LSA, and consequently add or remove
     *   (if needed) edge and / or vertex.
     * @param lsr The OspfRouterLsa.
     * @return true is the graph has been altered.
     */

    bool HandleLsr (const OspfRouterLsa & lsr);

    /**
     * @brief Handle an OSPF Network LSA, and consequently updates the OSPF
     *   graph.
     * @param lsr The OspfNetworkLsa.
     * @return true is the graph has been altered.
     */

    bool HandleLsn (const OspfNetworkLsa & lsn);

    /**
     * @brief Handle an OSPF External LSA, and consequently updates the OSPF
     *   graph.
     * @param lse The OspfExternalLsa.
     * @return true iif is the graph has been altered.
     */

    bool HandleLse (const OspfExternalLsa & lse);

    /**
     * @brief Accessor to the OSPF graph managed by this Ibgp2d instance.
     * @return The nested OspfGraph.
     */

    const ospf::OspfGraph & GetGraph () const;

    /**
     * @brief Get a vertex according to its router-id.
     * @param routerId The vertex's router-id.
     * @return A pair made of the vertex and a boolean which is true if the
     *   vertex was found, false else.
     */

    std::pair<vd_t, bool> GetVertex (const rid_t & routerId) const;

    /**
     * @brief Get the IPv4 address of the interface of u connected to v.
     * @param u The router from which we want the interface.
     * @param v The destination router of the (u, v) link.
     * @return The corresponding Ipv4Address.
     */

    const Ipv4Address & GetInterface (const rid_t & u, const rid_t & v) const;

    /**
     * @brief Retrieve the Ipv4Prefix corresponding to a network identifier.
     * @param nid The network identifier.
     * @param network The retrieved Ipv4Prefix (if found).
     * @returns true iif successful
     */

    bool GetNetwork(const nid_t & nid, Ipv4Prefix & network) const;

    /**
     * @brief Retrieve the prefixes corresponding to the external networks
     *    connected to a given OSPF router.
     * @param rid_n The OSPF router-id of the OSPF router.
     * @param networks A set of Ipv4Prefix where external networks will be inserted.
     * @return true iif successful.
     */

    bool GetExternalNetworks (
        const rid_t & rid_n,
        std::set<Ipv4Prefix> & networks
    ) const;

    /**
     * @brief Retrieve the prefixes corresponding to the transit networks shared
     *   by to neighboring routers (this is basically the prefixes corresponding
     *   to the links shared by the both routers).
     * @param rid_u The OSPF router-id of an OSPF router.
     * @param rid_v The OSPF router-id of another OSPF router.
     * @param networks A set of Ipv4Prefix where external networks will be
     *   inserted.
     * @return true iif successful.
     */

    bool GetTransitNetworks (
        const rid_t & rid_u,
        const rid_t & rid_v,
        std::set<Ipv4Prefix> & networks
    ) const;

    /**
     * @brief Write the graphviz output of the OSPF topology.
     * @param out The output stream.
     * @param drawNetworks Pass true to draw the network vertices, false
     *   otherwise.
     *    If true: all the vertices are routers, all the arcs are router->router
     *    If false: vertices may be either routers or either networks, all the
     *       arcs are router->network.
     * @returns The updated output stream.
     */

    std::ostream & WriteGraphviz (std::ostream & out, bool drawNetworks = false) const;
};

}

#endif // OSPF_GRAPH_HELPER_H
