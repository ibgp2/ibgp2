/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Marc-Olivier Buob, Alexandre Morignot
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
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 *   Alexandre Morignot <alexandre.morignot@orange.fr>
 */

#ifndef OSPF_GRAPH
#define OSPF_GRAPH

#include <limits>                           // std::numeric_limits
#include <map>                              // std::map
#include <ostream>                          // std::ostream

#include <boost/graph/adjacency_list.hpp>   // boost::adjacency_list

#include "ns3/ipv4-address.h"               // ns3::Ipv4Address
#include "ns3/log.h"                        // NS_LOG*
#include "ns3/ospf-packet.h"                // ns3::network_id_t, ns3::router_id_t // TODO move router_id_t into a dedicated file

#include "graph-builder.h"                  // ns3::graph_builder

namespace ns3 {
namespace ospf {

/**
 * @class OspfVertex
 * @brief Structure storing the information of an arc in an OspfGraph.
 *
 * In OSPF, arcs usually connect a "router node" to a "network node"
 * and vice-versa.
 *
 * In OspfGraph, only "router nodes" are represented. Information related
 * to OSPF networks are stored in the appropriate OspfEdge structure.
 */

class OspfVertex {
private:

    router_id_t m_routerId;   /**< Router's OSPF router-id. */

public:

    /**
     * @brief Constructor.
     */

    OspfVertex();

    /**
     * @brief Constructor.
     * @param routerId The router-id related to the OSPF router
     *    represented by this OspfVertex.
     */

    OspfVertex(const router_id_t & routerId);

    /**
     * @brief Constructor by copy.
     * @param o The copied OspfVertex.
     */

    OspfVertex(const OspfVertex & o);

    /**
     * @brief Retrieve the router-id related to an OspfVertex.
     * @return The router-id related to this OspfVertex.
     */

    const router_id_t & GetRouterId() const;

    /**
     * @brief Copy an OspfVertex in *this.
     * @param o The copied OspfVertex.
     */

    void Copy(const OspfVertex & o);

    /**
     * @brief Copy an OspfVertex in *this.
     * @param o The copied OspfVertex.
     * @return *this.
     */

    OspfVertex & operator = (const OspfVertex & o);

    /**
     * @brief Compare two OspfVertex.
     * @param o The OspfVertex compared with *this.
     * @return true iif the both OspfVertex designs the same OSPF router.
     */

    bool operator == (const OspfVertex & o) const;

    /**
     * @brief Print this OspfVertex in an output stream.
     * @param os The output stream.
     */

    virtual void Print(std::ostream & os) const;
};

/**
 * @brief Print an OspfVertex in an output stream.
 * @param os The output stream.
 * @param v An OspfVertex.
 * @return The updated output stream.
 */

std::ostream & operator << (std::ostream & os, const OspfVertex & v);

/**
 * @class OspfEdge
 * @brief Structure storing the information of an arc in an OspfGraph.
 *
 * In OSPF, arcs usually connect a "router node" to a "network node"
 * and vice-versa.
 *
 * In OspfGraph, only "router nodes" are represented, and all the networks
 * shared by to neighboring OSPF router are stored in the corresponding
 * edge connecting the both vertices. The information stored in an OspfEdge
 * is always related to the router acting as the source of the arc.
 *
 * In OSPF, each "network node" is identified by the IP address of the
 * Designated Router (DR). This network identifier is used internally
 * in OspfEdge to maintain the information related to each network
 * e.g. the OSPF metric and the IP address of the source router.
 */

struct OspfEdge
{
public:
    // TODO use a single map std::map<network_id_t, std::pair<Ipv4Address, ospf::metric_t> >
    typedef std::map<network_id_t, ospf::metric_t>  MapDistances;
    typedef std::map<network_id_t, Ipv4Address>     MapInterfaces;
private:
    MapDistances    m_mapDistances;     /**< Stores the OSPF distances toward the connected networks of this OspfEdge. */
    MapInterfaces   m_mapInterfaces;    /**< Stores the OSPF interfaces of the source router of this OspfEdge. */
public:

    /**
     * @brief Default constructor.
     */

    OspfEdge();

    /**
     * @brief Constructor.
     * @param n A DR related to a network of this OspfEdge.
     * @param i The corresponding interface of the source router of this OspfEdge.
     * @param m The corresponding OSPF metric.
     */

    OspfEdge(
        const network_id_t & n,
        const Ipv4Address  & i,
        const metric_t     & m = std::numeric_limits<metric_t>::max()
    );

    /**
     * @brief Constructor by copy.
     * @param o The copied OspfEdge
     */

    OspfEdge(const OspfEdge & o);

    /**
     * @brief Get the metric configured along this edge. Since several
     *   metrics may be installed, it returns the lowest one.
     * @returns The lowest OSPF metric.
     */

    const ospf::metric_t GetDistance() const;

    /**
     * @brief Retrieve the distances related to each network involved in
     *   this OspfEdge.
     * @return The map storing the distances related to each network involved in
     *   this OspfEdge.
     */

    const MapDistances & GetDistances() const;

    /**
     * @brief Changes the OSPF metric assigned to a network embeded in
     *   this OspfEdge. Basically it calls SetDistance by transforming the
     *   metric into the corresponding OSPF distance.
     * @param n The OSPF network.
     * @param d The OSPF metric assigned to "n".
     * @sa SetDistance()
     */

    void SetMetric(const network_id_t & n, const metric_t & m);
    void SetInterface(const network_id_t & n, const Ipv4Address & i);

    /**
     * @brief Remove the information related to a given network stored in this OspfEdge.
     * @param n The network identifier of the network.
     */

    void DeleteNetwork(const network_id_t & n);

    /**
     * @brief Retrieve the number of networks represented in this OspfEdge.
     * @return The corresponding number of networks.
     */

    std::size_t GetNumNetworks() const;

    /**
     * @brief Retrieve the network configured along this OspfEdge. Since several
     *   networks may be embedded, it returns the network having the lowest metric.
     * @returns The corresponding network identifier.
     */

    const network_id_t GetNetwork() const;

    /**
     * @brief Retrieve the IP address of the source router involved in the network
     *   having the lowest OSPF cost.
     * @throw std::runtime_error if not found.
     * @returns The Ipv4Address of the interface related to this->GetNetwork().
     */

    const Ipv4Address & GetInterface() const;

    /**
     * @brief Copy an OspfEdge into *this.
     * @param o The copied OspfEdge.
     */

    void Copy(const OspfEdge & o);

    /**
     * @brief Copy an OspfEdge into *this.
     * @param o The copied OspfEdge.
     * @returns *this.
     */

    OspfEdge & operator = (const OspfEdge & o);

    /**
     * @brief Print this OspfEdge in an output stream.
     * @param os The output stream.
     */

    virtual void Print(std::ostream & os) const;
};

/**
 * @brief Print an OspfEdge in an output stream.
 * @param os The output stream.
 * @param e An OspfEdge.
 * @return The updated output stream.
 */

std::ostream & operator << (std::ostream & os, const OspfEdge & e);

/**
 * @class OspfGraph
 * @brief Structure representing an OSPF graph.
 */

typedef boost::adjacency_list<
    boost::multisetS,
    boost::vecS,
    boost::bidirectionalS,
    OspfVertex,
    OspfEdge
> OspfGraph;

/**
 * @class OspfGraphBuilder
 * @brief Allow to easily maintain an OspfGraph.
 */

typedef ns3::graph_builder_t<
    OspfGraph,
    router_id_t
> OspfGraphBuilder;

} // namespace ospf
} // namespace ns3

#endif

