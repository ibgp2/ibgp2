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
 *   Alexandre Morignot <alexandre.morignot@orange.fr>
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 */

#include "ospf-graph-helper.h"

#include <iostream>                         // std::cout
#include <ostream>                          // std::ostream
#include <stdexcept>                        // std::runtime_error
#include <boost/foreach.hpp>                // BOOST_FOREACH
#include <boost/graph/adjacency_list.hpp>   // boost::adjacency_list

#include "ns3/log.h"                        // ns3::NS_LOG
#include "ns3/ospf-graph.h"                 // ns3::OspfGraph
#include "ns3/ospf-packet.h"                // ns3::LsaType

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("OspfGraphHelper");


OspfGraphHelper::OspfGraphHelper () :
    m_gbOspf (m_gospf)
{
    NS_LOG_FUNCTION (this);
}

OspfGraphHelper::~OspfGraphHelper () {
    NS_LOG_FUNCTION (this);
    this->WriteGraphviz(std::cout, false);
}

std::ostream & OspfGraphHelper::WriteGraphviz (
    std::ostream & out,
    bool drawNetworks
) const {
    std::cout << "Router ID: " << this->m_routerId << std::endl; // DEBUG
    out << "digraph ospf_graph {" << std::endl;

    // Print vertices corresponding to networks
    if (drawNetworks) {
        for (const auto & network : this->m_mapOspfNetworks) {
            const uint32_t & networkInt = network.first.Get();
            out << "\t" << networkInt << " [label=\"" << network.first << "\"]" << std::endl;
        }

        for (const auto & p : this->m_mapExternalNetworks) {
            const std::set<nid_t> & networks = p.second;
            for (const auto & network : networks) {
                const uint32_t & networkInt = network.Get();
                out << "\t" << networkInt << " [label=\"" << network << "\"]" << std::endl;
            }
        }
    }

    // Print vertices corresponding to routers
    BOOST_FOREACH (const vd_t & vd, boost::vertices(m_gospf)) {
        const vb_t & vb = m_gospf[vd];
        out << "\t" << vd << " [label=\"" << vb.GetRouterId() << "\"]" << std::endl;
    }

    // Print OSPF arcs (either router->router, either router->network depending on drawNetworks)
    BOOST_FOREACH(const ed_t & ed, boost::edges(m_gospf)) {
        const eb_t & eb = m_gospf[ed];
        const vd_t
        & vd_u = boost::source(ed, m_gospf),
          & vd_v = boost::target(ed, m_gospf);

        for (const auto & d : eb.GetDistances()) {
            const uint32_t & networkAddress = d.first.Get ();
            const ospf::metric_t & metric = d.second;
            if (drawNetworks) {
                out << "\t" << vd_u << " -> " << networkAddress << " [label=\"" << metric << "\"]" << std::endl
                    << "\t" << networkAddress << " -> " << vd_v << " [label=\"" << metric << "\"]" << std::endl;
            } else {
                out << "\t" << vd_u << " -> " << vd_v << " [label=\"" << metric << "\"]" << std::endl;
            }
        }
    }
    out << "}" << std::endl;

    return out;
}

bool OspfGraphHelper::HandleLsa (std::vector<OspfLsa *> & lsas) {
    NS_LOG_FUNCTION (this);
    bool changed = false;

    for (OspfLsa * lsa : lsas) {
        switch (lsa->GetLsaType()) {
            case OSPF_LSA_TYPE_ROUTER:
                changed |= this->HandleLsr (*(dynamic_cast<OspfRouterLsa *> (lsa)));
                break;
            case OSPF_LSA_TYPE_NETWORK:
                changed |= this->HandleLsn (*(dynamic_cast<OspfNetworkLsa *> (lsa)));
                break;
            case OSPF_LSA_TYPE_EXTERNAL:
                changed |= this->HandleLse (*(dynamic_cast<OspfExternalLsa *> (lsa)));
                break;
        }
    }

    return changed;
}

bool OspfGraphHelper::HandleLsr (const OspfRouterLsa & lsr)
{
    const Ipv4Address & rid_u = lsr.GetAdvertisingRouter();
    NS_LOG_FUNCTION (this << lsr);

    for (auto & elt : this->m_mapOspfNetworks) {
        const ospf::network_id_t & nid = elt.first;
        const std::set<ospf::router_id_t> & rids = elt.second;

        // If the router annoncing (lsr->rid) is in the network, but the network
        // is not announced in the LSR, we must remove the router from it

        if (rids.find(rid_u) != rids.end() && lsr.networks.find (nid) == lsr.networks.end()) {
            NS_LOG_LOGIC ("\t\tRemove the network " << nid);
            RemoveAdjacency (rid_u, nid);
        }
    }

    for (auto & elt : lsr.networks) {
        const ospf::network_id_t & nid = elt.first;
        const ospf::metric_t & metric = elt.second; // metric from u to nid

        std::map<ospf::network_id_t, Ipv4Address>::const_iterator it_if = lsr.ifs.find(nid);

        Ipv4Address if_u = (it_if != lsr.ifs.end()) ?
            it_if->second :
            Ipv4Address ("69.69.69.69");

        NS_LOG_DEBUG ("\t\t" << nid << ": " << metric);
        NS_LOG_DEBUG ("\t\tinterface: " << if_u);

        // Search (network, router) pair in this->ospfNetworks
        // TODO to improve
////////////////// << DUPLICATED AddOspfArc(rid, nid, metric, false)
        for (auto & rid_v : this->m_mapOspfNetworks[nid]) {
            if (rid_v == rid_u) {
                continue;
            }

            // For each router in the network, we add an edge
            this->AddAdjacency (rid_u, rid_v, nid, if_u, metric);
            OspfGraphHelper::OspfArc arc = std::make_pair(rid_v, nid);
            this->AddAdjacency (rid_v, rid_u, nid, this->m_mapInterfaces[arc], this->m_mapMetrics[arc]);

        }
////////////////// >> DUPLICATED

        // Save the information related to this network
        this->m_mapOspfNetworks[nid].insert (rid_u);
        this->m_mapMetrics[std::make_pair(rid_u, nid)] = metric;
        this->m_mapInterfaces[std::make_pair(rid_u, nid)] = if_u;
    }

    return true;
}

bool OspfGraphHelper::HandleLsn(const OspfNetworkLsa& lsn)
{
    NS_LOG_FUNCTION (this);

    const Ipv4Address & nid  = lsn.GetLinkStateId();
    const Ipv4Mask    & mask = lsn.GetNetworkMask();
    this->m_mapNetworks[nid] = Ipv4Prefix(nid, mask);
    return true;
}

bool OspfGraphHelper::HandleLse (const OspfExternalLsa & lse)
{
    NS_LOG_FUNCTION (this << lse);

    const Ipv4Address        & ridAsbr = lse.GetAdvertisingRouter();
    const ospf::network_id_t & nid     = lse.GetLinkStateId();
    const ospf::metric_t     & metric  = lse.GetMetric();
    const Ipv4Mask           & mask    = lse.GetNetworkMask();

    this->m_mapNetworks[nid] = Ipv4Prefix(nid, mask);
    this->m_mapExternalNetworks[ridAsbr].insert(nid);
    this->m_mapMetrics[std::make_pair (ridAsbr, nid)] = metric;

    return true;
}

const ospf::OspfGraph & OspfGraphHelper::GetGraph () const {
    NS_LOG_FUNCTION (this);
    return this->m_gospf;
}

std::pair<OspfGraphHelper::vd_t, bool> OspfGraphHelper::GetVertex (const OspfGraphHelper::rid_t & rid) const {
    NS_LOG_FUNCTION (this);
    return m_gbOspf.get_vertex (rid);
}

const Ipv4Address & OspfGraphHelper::GetInterface(
    const OspfGraphHelper::rid_t & rid_u,
    const OspfGraphHelper::rid_t & rid_v
) const {
    NS_LOG_FUNCTION (this);

    ed_t ed;
    bool ok;
    boost::tie (ed, ok) = this->m_gbOspf.get_edge (rid_u, rid_v);
    NS_ASSERT(ok);

    return this->m_gospf[ed].GetInterface ();
}

bool OspfGraphHelper::GetNetwork(const nid_t & nid, Ipv4Prefix & network) const {
    MapNetwork::const_iterator fit(this->m_mapNetworks.find(nid));
    if (fit == this->m_mapNetworks.end()) return false;
    network = fit->second;
    network.SetAddress(network.GetAddress().CombineMask(network.GetMask()));
    return true;
}

bool OspfGraphHelper::GetTransitNetworks(
    const OspfGraphHelper::rid_t& rid_u,
    const OspfGraphHelper::rid_t& rid_v,
    std::set< Ipv4Prefix >& transitNetworks
) const {
    // Find the edge from u to v
    bool found;
    ed_t e_uv;
    boost::tie(e_uv, found) = this->m_gbOspf.get_edge(rid_u, rid_v);
    if (!found) return false;

    // For each of the corresponding network identifier, retrieve the corresponding prefix
    const ospf::OspfEdge::MapDistances & distances = this->m_gospf[e_uv].GetDistances();
    for (auto & nm : distances) {
        const nid_t & nid = nm.first;
        Ipv4Prefix prefix;
        bool found = this->GetNetwork(nid, prefix);
        NS_ASSERT(found);
        transitNetworks.insert(prefix);
    }
    return true;
}

bool OspfGraphHelper::GetExternalNetworks (
    const OspfGraphHelper::rid_t & rid,
    std::set<Ipv4Prefix> & externalNetworks
) const {
    NS_LOG_FUNCTION (this);

    // Find the eventual external networks connected to the router identified by rid.
    MapExternalNetwork::const_iterator fit(this->m_mapExternalNetworks.find(rid));
    if (fit == this->m_mapExternalNetworks.end()) {
        return false;
    }

    // For each of the corresponding network identifier, retrieve the corresponding prefix.
    const std::set<nid_t> & nids = fit->second;
    for (auto & nid : nids) {
        Ipv4Prefix prefix;
        bool found = this->GetNetwork(nid, prefix);
        NS_ASSERT(found);
        externalNetworks.insert(prefix);
    }
    return true;
}

bool OspfGraphHelper::AddAdjacency (
    const OspfGraphHelper::rid_t      & rid_u,
    const OspfGraphHelper::rid_t      & rid_v,
    const OspfGraphHelper::nid_t      & nid,
    const Ipv4Address                 & if_u,
    const OspfGraphHelper::OspfMetric & m_uv
) {
    NS_LOG_FUNCTION (this);

    ed_t ed;
    bool ok;
    boost::tie (ed, ok) = this->m_gbOspf.get_edge (rid_u, rid_v);

    if (ok) {
        // The arc already exists, we just add/update the new network
        this->m_gospf[ed].SetMetric(nid, m_uv);
        this->m_gospf[ed].SetInterface(nid, if_u);
    } else {
        // The arc doesn't yet exist, we create it.
        this->m_gbOspf.add_vertex (rid_u, vb_t(rid_u));
        this->m_gbOspf.add_vertex (rid_v, vb_t(rid_v));
        this->m_gbOspf.add_edge (rid_u, rid_v, eb_t (nid, if_u, m_uv));
    }

    return true;
}

bool OspfGraphHelper::RemoveAdjacency (
    const OspfGraphHelper::rid_t & rid_u,
    const OspfGraphHelper::nid_t & nid
) {
    NS_LOG_FUNCTION (this);

    this->m_mapOspfNetworks[nid].erase (rid_u);

    if (this->m_mapOspfNetworks[nid].empty ()) {
        this->m_mapOspfNetworks.erase (nid);
    } else {
        for (auto & rid_v : this->m_mapOspfNetworks[nid]) {
            this->RemoveAdjacency (rid_u, rid_v, nid);
            this->RemoveAdjacency (rid_v, rid_u, nid);

            // Removing a vertex invalidates our vertex_descriptors and will break
            // graph_builder mapping, so even if the out-degree of u (resp v)
            // is now equal to 0, we cannot remove the corresponding vertex from
            // the boost graph.
            // http://www.boost.org/doc/libs/1_38_0/libs/graph/doc/adjacency_list.html
        }
    }

    return true;
}

bool OspfGraphHelper::RemoveAdjacency (
    const OspfGraphHelper::rid_t & rid_u,
    const OspfGraphHelper::rid_t & rid_v,
    const OspfGraphHelper::nid_t & nid
) {
    NS_LOG_FUNCTION (this);

    ed_t ed;
    bool ok;
    boost::tie (ed, ok) = this->m_gbOspf.get_edge (rid_u, rid_v);

    if (ok) {
        NS_LOG_LOGIC ("\t\t\t\tRemove " << nid << " on " << rid_u << " -> " << rid_v);
        this->m_gospf[ed].DeleteNetwork(nid);

        // The both routers don't share networks anymore, we remove the arc.
        if (this->m_gospf[ed].GetNumNetworks() == 0) {
            NS_LOG_LOGIC ("\t\t\t\tRemove " << rid_u << " -> " << rid_v);
            this->m_gbOspf.remove_edge (rid_u, rid_v);
        }
    }

    return true;
}

void OspfGraphHelper::SetRouterId(const OspfGraphHelper::rid_t & rid) {
    this->m_routerId = rid;
}

} // namespace ns3


