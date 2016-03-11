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
 * Author:
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 *   Alexandre Morignot <alexandre.morignot@orange.fr>
 */
#include "ibgp2d.h"

#define LOCALHOST       "127.0.0.1"
#define DUMMY_ROUTER_ID "0.0.0.0"

#define EOT             char(0x4)            // End of Transmission
#define RE_IPV4      "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})"

#include <cstdlib>                          // malloc
#include <iostream>                         // std::cerr
#include <regex>                            // std:regex
#include <sstream>                          // std::ostringstream
#include <string>                           // std::string
#include <vector>                           // std::vector

#include <boost/foreach.hpp>                // BOOST_FOREACH
#include <boost/graph/adjacency_list.hpp>   // boost::target
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/function_property_map.hpp>

#include "ns3/bgp-config.h"                 // ns3::BgpConfig
#include "ns3/ipv4.h"                       // ns3::Ipv4
#include "ns3/ipv4-address.h"               // ns3::Ipv4Address
#include "ns3/log.h"                        // NS_LOG_*
#include "ns3/loopback-net-device.h"        // LoopbackNetDevice
#include "ns3/node.h"                       // ns3::Node
#include "ns3/object-factory.h"             // ns3::CreateObject
#include "ns3/ptr.h"                        // ns3::Ptr
#include "ns3/simulator.h"                  // ns3::Simulator
#include "ns3/type-id.h"                    // ns3::TypeId

#include "../quagga/bgpd/bgp-config.h"      // ns3::BgpConfig
#include "../ospf-graph/ospf-packet.h"      // ns3::OspfLsa*

// DEBUG
// #include "../pcap-wrapper.h"                // PacketWritePcap
// #include "../tcpdump-wrapper.h"             // tcpdump

NS_LOG_COMPONENT_DEFINE ("Ibgp2d");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Ibgp2d);

//---------------------------------------------------------------------------------
// Internal usage
//---------------------------------------------------------------------------------

/**
 * Ideally ns3::Packet should provide a GetBuffer method providing
 * a read-only access to the packet's buffer. Unfortunately, this
 * is not the case, so for now, we have to copy the carried bytes.
 * \param p A pointer to a ns3::Packet instance.
 * \returns The address of the Packet's buffer (copy), NULL if
 *    not enough memory.
 */

static uint8_t * PacketGetBuffer (Ptr<const Packet> p) {
    uint32_t packetSize = p->GetSize();
    uint8_t * buffer;

    if ( (buffer = (uint8_t *) malloc (packetSize))) {
        p->CopyData (buffer, packetSize);
    }

    return buffer;
}

//---------------------------------------------------------------------------------
// Ibgp2d
//---------------------------------------------------------------------------------

Ibgp2d::Ibgp2d() :
    m_routerId (DUMMY_ROUTER_ID),
    m_telnetBgp (0),
    m_lastFilterId (0),
    m_bgpdWasRunning (false)
{
    NS_LOG_FUNCTION (this);
    this->m_ospfGraphHelper = CreateObject<OspfGraphHelper>();
}

Ibgp2d::~Ibgp2d() {
    NS_LOG_FUNCTION (this);
}

TypeId Ibgp2d::GetTypeId () {
    static TypeId tid = TypeId ("ns3::Ibgp2d")
                        .SetParent<Application> ()
                        .AddConstructor<Ibgp2d> ()
                        ;
    return tid;
}

void Ibgp2d::StartApplication() {
    NS_LOG_FUNCTION (this);

    Ptr<Node> node = this->GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    uint32_t numInterfaces = ipv4->GetNInterfaces();

    // We loop over each interface, and for each interface we subscribe to the
    // corresponding device. Directly loop over the devices with node->GetDevice (i)
    // would been an option, but this result in bugs with localhost interface not
    // being a loopback device while the last interface (not localhost) seems to
    // a loopback device.
    //
    // This bugs seems to be related : https://www.nsnam.org/bugzilla/show_bug.cgi?id=1627

    for (uint32_t i = 0; i < numInterfaces; i++) {
        Ptr<NetDevice> device =  ipv4->GetNetDevice (i);
        Ptr<LoopbackNetDevice> loopbackDev = device->GetObject<LoopbackNetDevice> ();

        // Skip loopback device
        if (loopbackDev) {
            NS_LOG_LOGIC (
                "Skipping device " << i
                << " (" << ipv4->GetAddress (i, 0)
                << "; MTU = " << device->GetMtu () << ")."
            );
            continue;
        }

        NS_LOG_LOGIC (
            "Connecting sink to device " << i
            << " (IP = " << ipv4->GetAddress (i, 0)
            << ", MTU = " << device->GetMtu () << ")."
        );

        bool result = device->TraceConnectWithoutContext (
            "Sniffer",
            MakeCallback (&Ibgp2d::HandlePacket, this)
        );

        NS_ASSERT_MSG (result, "Unable to hook \"Sniffer\"");
    }
}

void Ibgp2d::StopApplication () {
    NS_LOG_FUNCTION (this);
    this->BgpdDisconnect();
}

void Ibgp2d::SetAsn (uint32_t asn) {
    NS_LOG_FUNCTION (this << asn);
    this->m_asn = asn;
}

uint32_t Ibgp2d::GetAsn() const {
    NS_LOG_FUNCTION (this);
    return this->m_asn;
}

void Ibgp2d::SetRouterId (const ns3::Ibgp2d::rid_t & routerId) {
    NS_LOG_FUNCTION (this << routerId);
    this->m_routerId  = routerId;
}

const Ibgp2d::rid_t & Ibgp2d::GetRouterId() const {
    NS_LOG_FUNCTION (this);
    return this->m_routerId;
}

const OspfGraphHelper * Ibgp2d::GetOspfGraphHelper() const {
    NS_LOG_FUNCTION (this);
    return GetPointer (this->m_ospfGraphHelper);
}

void Ibgp2d::HandlePacket (Ptr<const Packet> p) {
    NS_LOG_FUNCTION (this << p);

    if (uint8_t * buffer =  PacketGetBuffer (p)) {
        if (!IsOspfPacket (buffer)) {
            NS_LOG_LOGIC ("Packet discarded (not OSPF)");
            return;
        }

        /*
        // DEBUG
        {
            std::cout << "===============================================================" << std::endl;

            std::string filename = MakePcapFilename();
            std::ofstream ofs(filename);
            if (ofs) {
                std::cout << Ipv4Address(this->GetRouterId())
                          << ": Writting " << filename << std::endl;
                PacketWritePcap(ofs, Simulator::Now(), p, PcapHelper::DLT_PPP);
                ofs.close();
                tcpdump(std::cout, p);
            }
        }
        */

        std::vector<OspfLsa *> lsas;
        ExtractOspfLsa (buffer, lsas);

        // Determine whether the IGP topology has changed.
        bool hasChanged = false;
        if (!lsas.empty ()) {
            this->m_ospfGraphHelper->SetRouterId (this->GetRouterId());
            hasChanged = this->m_ospfGraphHelper->HandleLsa (lsas);
            for (auto & lsa : lsas) delete lsa;
        }
        free (buffer);

        // Recompute iBGP2 redistribution.
        if (hasChanged) {
            UpdateIbgp2Redistribution();
        }

        // Push (if possible) iBGP2d filters in bgpd.

        // TODO:
        // We must check whether bgpd is running. This should be checked by
        // testing whether the telnet bgpd client embedded in iBGP2d connects
        // successfully to bgpd. For sake of simplicity, we simply tests here
        // if bgpd has already started.

        Ptr<BgpConfig> bgpConfig = this->GetNode()->GetObject<BgpConfig>();
        NS_ASSERT(bgpConfig);
        Time bgpdStartTime = bgpConfig->GetStartTime();
        bool bgpdIsRunning = ( bgpdStartTime < Simulator::Now() );

        if (bgpdIsRunning) {
            // Update filters
            if (hasChanged) {
                NS_LOG_DEBUG("[IBGP2]: " << this->GetRouterId() << ": IGP topology has changed");
                this->UpdateBgpConfiguration();
            } else if (!this->m_bgpdWasRunning) {
                NS_LOG_DEBUG("[IBGP2]: " << this->GetRouterId() << ": bgpd starts");
                this->UpdateBgpConfiguration();
            }
        } else if (!bgpdIsRunning) {
            /*
            NS_LOG_DEBUG(
                "[IBGP2]: " << this->GetRouterId()
                      << ": bgpd is not yet running: bgpdStartTime = "
                      << bgpdStartTime.GetSeconds() << " > t = "
                      << Simulator::Now().GetSeconds()
            );
            */
        }

        // Update bgpdWasRunning flag.
        this->m_bgpdWasRunning = bgpdIsRunning;
    }
}

bool Ibgp2d::UpdateBgpConfiguration() {
    NS_LOG_FUNCTION (this);
    std::ostringstream oss;
    std::set<Ipv4Address> neighborsAltered;

    // Craft the string of command that will be transmitted to bgpd.
    oss << "#-------------------------BEGIN------------------- t = "
        << Simulator::Now().GetSeconds() << std::endl;
    size_t numNeighborsAltered = this->WriteIbgp2Filters (oss, neighborsAltered);
    oss << "#--------------------------END--------------------" << std::endl;

    NS_LOG_DEBUG(numNeighborsAltered << " altered neighbors:" << std::endl << oss.str());

    if (numNeighborsAltered) {
        // Note; previously I was directly calling:
        // this->RefreshIbgp2Neighbors(neighborsAltered);

        // Wait 1s in order to let the FIB being updated. This avoid to
        // forward the current best route which sometimes, is no more the
        // optimal one once the IGP packet is processed.

        Simulator::ScheduleWithContext (
            Simulator::GetContext(),
            Seconds (1),
            &Ibgp2d::RefreshIbgp2Neighbors,
            this,
            neighborsAltered
        );

    } else {
        oss.flush();
        oss << "write terminal" << std::endl;
    }

    this->BgpdConnect();
    this->m_telnetBgp->AppendCommand (oss.str());
    return (numNeighborsAltered > 0);
}

void Ibgp2d::RefreshIbgp2Neighbors (std::set< Ipv4Address > & neighborsAltered) {
    NS_LOG_FUNCTION (this);
    std::cout << "t = " << Simulator::Now().GetSeconds()
              << ": Node [" << this->GetRouterId()
              << "]: updating iBGP configuration." << std::endl;

    for (auto & n : neighborsAltered) {
        std::cout << "t = " << Simulator::Now().GetSeconds()
                  << ": Node [" << this->GetRouterId() << "]:   updating "
                  << n << std::endl;
    }

    if (neighborsAltered.empty()) {
        return;
    }

    std::ostringstream oss;

    // bgpd(config)#
    oss << EOT << std::endl;

    // bgpd#
    for (auto & ip_v : neighborsAltered) {
        oss << "clear ip bgp " << ip_v << " soft" << std::endl;
    }

    // bgpd(config)#
    oss << "configure terminal" << std::endl;

    this->BgpdConnect();
    this->m_telnetBgp->AppendCommand (oss.str());
}


//////////DEBUG
template <typename T>
std::ostream & operator << (std::ostream & os, const std::set<T> & s) {
    os << '{';
    for (auto & x : s) os << ' ' << x;
    os << " }";
    return os;
}
//////////DEBUG

void Ibgp2d::UpdateIbgp2Redistribution() {
    NS_LOG_FUNCTION (this);

    // We denote by u this router and v each of its iBGP2/IGP neighbor
    const ospf::router_id_t & rid_u = this->GetRouterId();
    NS_ASSERT (rid_u != DUMMY_ROUTER_ID);

    const ospf::OspfGraph & gospf = this->m_ospfGraphHelper->GetGraph ();

    // Find the vertex corresponding to the managed router
    vd_t u;
    {
        bool ok;
        boost::tie (u, ok) = this->m_ospfGraphHelper->GetVertex (rid_u);

        // The router embedding this iBGP2d instance does not yet belong to
        // the IGP graph or the router-id of this router is not yet known.
        // We have to wait a bit more that the IGP converges...
        if (!ok) {
            // Note this "error" is normal while the IGP converges the first time.
            //NS_LOG_INFO ("[iBGP2]: " << rid_u << ": cannot find " << rid_u << " IGP graph." );
            return;
        }
    }

    // Compute the Dijkstra's algorithm from each IGP neighbor point of view.
    BOOST_FOREACH (const ed_t & e_uv, boost::out_edges (u, gospf)) {
        const vd_t & v = boost::target (e_uv, gospf);

        if (u == v) {
            // Some loops are built in the OSPF graph to store metrics toward
            // external networks. We can ignore those arc since we consider
            // only neighbors v != u in iBGP2.
            continue;
        }

        const rid_t & rid_v = gospf[v].GetRouterId();
        const ospf::OspfGraph & ospfGraph = this->m_ospfGraphHelper->GetGraph();
        std::map<vd_t, vd_t> predecessors;
        std::map<vd_t, uint32_t> distances;

        boost::dijkstra_shortest_paths (
            ospfGraph,
            v,
            predecessor_map (boost::make_assoc_property_map (predecessors)).
            weight_map (boost::make_function_property_map<ed_t, uint32_t> (
                [&] (const ed_t & e) -> uint32_t {
                    return ospfGraph[e].GetDistance();
                }
            )).
            distance_map (boost::make_assoc_property_map (distances))
        );

        std::set<rid_t> rids_n_enabled;

        if (predecessors[u] == v) {
            // If we are here u is a child of v in the SPT rooted in v. Each node
            // n belonging to the subtree rooted in u is potentially a nexthop
            // announcing BGP routes that must be redistributed by u to v.

            BOOST_FOREACH (const vd_t & n, boost::vertices (gospf)) {
                if (n == v) {
                    // n announcements have no reason to transit via u to return
                    // to v == n, so we filter them.
                    continue;
                }

                const ospf::router_id_t & rid_n = gospf[n].GetRouterId();
                bool enableIbgp2_nuv = false;

                // Walk along the shortest path from n to v. If we reach u just
                // before reaching v, (n, u, v) satisfies the iBGP2 criterion

                unsigned i;
                for (vd_t vcur = n, i = 0; vcur != v ; vcur = predecessors[vcur], i++) {

                    if (vcur == predecessors[vcur]) {
                        // vcur has no predecessor : it occurs if n is in the OSPF graph, but is unreachable from v.
                        break;
                    }

                    NS_ASSERT (i < boost::num_vertices (gospf)); // corrupted predecessors map ?

                    if (vcur == u) {
                        enableIbgp2_nuv = true;
                        break;
                    }
                }

                if (enableIbgp2_nuv) {
                    rids_n_enabled.insert (rid_n);
                }
            } // for n
        } else {
            // predecessors[u] != v, so we must filter any iBGP announce from u to v.
            // rids_n_enabled remains empty.
        }

        // Deduces from rids_n_enabled the corresponding prefixes
        std::set<Ipv4Prefix> & enabledNexthops = this->m_mapFilters[rid_v];

        if (predecessors[u] == v) {
            // TODO We should enumerate the IP of u in the filter.
            // For the moment we use a simpler implementation : we only accept
            // the interface of v directly connected to u.
            this->m_ospfGraphHelper->GetTransitNetworks (rid_u, rid_v, enabledNexthops);

            for (const rid_t & rid_n : rids_n_enabled) {
                // External networks connected to the ASBR identified by rid_n
                this->m_ospfGraphHelper->GetExternalNetworks (rid_n, enabledNexthops);
            }
        }

    } // for e_uv
}

size_t Ibgp2d::WriteIbgp2Filters (
    std::ostream & os,
    std::set<Ipv4Address> & alteredNeighbors
) {
    NS_LOG_FUNCTION (this);

    // We denote by u this router and v each of its iBGP2/IGP neighbor
    const ospf::router_id_t & rid_u = this->GetRouterId();
    NS_ASSERT (rid_u != DUMMY_ROUTER_ID);

    const ospf::OspfGraph & gospf = this->m_ospfGraphHelper->GetGraph ();

    for (auto & p : this->m_mapFilters) {

        const rid_t & rid_v = p.first;
        const std::set<Ipv4Prefix> & enabledNexthops = p.second;

        // Compute diff between the running and the new configuration.
        std::set<Ipv4Prefix> addedPrefixes;
        std::set<Ipv4Prefix> removedPrefixes;

        {
            MapFilters::iterator fit (this->m_mapFiltersPrev.find (rid_v));

            if (fit != this->m_mapFiltersPrev.end()) {
                // This iBPG2 neighbor v has some filters already installed.
                std::set<Ipv4Prefix> & enabledPrefixesPrev = fit->second;

                // Find next-hop prefixes that are now allowed.
                for (const Ipv4Prefix & prefix : enabledNexthops) {
                    if (enabledPrefixesPrev.find (prefix) == enabledPrefixesPrev.end()) {
                        addedPrefixes.insert (prefix);
                        enabledPrefixesPrev.insert (prefix);
                    }
                }

                // Find next-hop prefixes that are not allowed anymore.
                for (const Ipv4Prefix & prefix : enabledPrefixesPrev) {
                    if (enabledNexthops.find (prefix) == enabledNexthops.end()) {
                        removedPrefixes.insert (prefix);
                        enabledPrefixesPrev.erase (prefix);
                    }
                }

                enabledPrefixesPrev.insert (addedPrefixes.begin(), addedPrefixes.end());
            } else {
                // v is a new peer, so all the nexthops n such (n, u, v) satisfies the iBGP2
                // createrion must be enabled.
                addedPrefixes = enabledNexthops;
                this->m_mapFiltersPrev[rid_v] = enabledNexthops;
            }
        }

        if (!addedPrefixes.empty() || !removedPrefixes.empty()) {
            const Ipv4Address & ip_v  = this->m_ospfGraphHelper->GetInterface (rid_v, rid_u);
            alteredNeighbors.insert (ip_v);
            this->BgpWriteIbgp2Peer (os, rid_v, addedPrefixes, removedPrefixes);
        }
    } // for each neighbor

    return alteredNeighbors.size();
}

void Ibgp2d::BgpdConnect() {
    NS_LOG_FUNCTION (this);
    Ptr<Node> node = this->GetNode();

    // Connect iBGP2 to BGPd (if not yet connected)
    if (!this->m_telnetBgp) {

        Ptr<BgpConfig> bgpConfig = node->GetObject<BgpConfig>();
        NS_ASSERT (bgpConfig);
        this->m_telnetBgp = new Telnet (
            node,
            Ipv4Address (LOCALHOST),
            bgpConfig->GetVtyPort(),
            bgpConfig->GetHostname() + "_ibgpv2_bgp.txt",
            Seconds (0)
        );

        // Connection (mode normal)
        const std::string & password = bgpConfig->GetPassword();

        if (password.size()) {
            this->m_telnetBgp->AppendCommand (password);
        }

        // bgpd>

        const std::string & passwordEnable = bgpConfig->GetPasswordEnable();
        this->m_telnetBgp->AppendCommand ("enable");

        if (passwordEnable.size()) {
            this->m_telnetBgp->AppendCommand (passwordEnable);
        }

        // bgpd#

        this->m_telnetBgp->AppendCommand ("configure terminal");

        // bgpd(config)#
    }
}

void Ibgp2d::BgpdDisconnect() {
    NS_LOG_FUNCTION (this);

    if (this->m_telnetBgp) {
        this->m_telnetBgp->Close();
        delete this->m_telnetBgp;
        this->m_telnetBgp = 0;
    }
}

void Ibgp2d::BgpWriteBegin (std::ostream & os) const {
    NS_LOG_FUNCTION (this);
    // bgpd(config)#
    os << "router bgp " << this->GetAsn() << std::endl;
    // bgpd(config-router)#
}

void Ibgp2d::BgpWriteIbgp2Peer (
    std::ostream & os,
    const Ipv4Address & rid_v,
    const std::set<Ipv4Prefix> & nexthopPrefixesEnabled,
    const std::set<Ipv4Prefix> & nexthopPrefixesDisabled
) {
    NS_LOG_FUNCTION (this << rid_v);
    // bgpd(config)#
    const Ipv4Address & rid_u = this->GetRouterId();
    const Ipv4Address & ip_v  = this->m_ospfGraphHelper->GetInterface (rid_v, rid_u);

    // Get the filter-id corresponding to v. Create it if it does not yet exist.
    FilterId filterId_v = this->GetFilterId (rid_v);
    bool isNewNeighbor = (filterId_v == 0);

    // If v has no filter-id, then v is a new iBGP2 peer.
    if (isNewNeighbor) {
        filterId_v = this->AssignFilterId (rid_v);
    }

    std::string routeMap_v = Ibgp2d::MakeRouteMapName (filterId_v);
    std::string acl_v =  Ibgp2d::MakeRouteMapName (filterId_v);

    // Declare the new neighbor and the corresponding route_map
    if (isNewNeighbor) {
        // bgpd(config)#
        this->BgpWriteBegin (os);

        // bgpd(config-router)#
        os << "neighbor "  << ip_v << " remote-as " << this->GetAsn()       << std::endl
           << "neighbor "  << ip_v << " route-reflector-client"             << std::endl
           << "neighbor "  << ip_v << " route-map " << routeMap_v << " out" << std::endl
           << "route-map " << routeMap_v << " permit 1"                     << std::endl;

        // bgpd(config-route-map)#
        os << "match ip next-hop " << acl_v << std::endl
           << "exit"                        << std::endl;

        // bgpd(config)#
    }

    NS_ASSERT (filterId_v > 0);

    // Create/update the access-list acl_v
    // bgpd(config)#
    for (auto & prefix_n : nexthopPrefixesDisabled) {
        os << "no access-list " << acl_v << " permit " << prefix_n << std::endl;
    }

    for (auto & prefix_n : nexthopPrefixesEnabled) {
        os << "access-list " << acl_v << " permit " << prefix_n << std::endl;
    }

    // Do not add the last rule "access-list ACL-xxx deny any" which is
    // implicit and can be triggered before "access-list ACL-xxx permit ..."
    // rules inserted later.
}

//----------------------------------------------------------------------------
// Filter id management
//----------------------------------------------------------------------------

Ibgp2d::FilterId Ibgp2d::GetFilterId (const Ipv4Address & rid_v) const {
    NS_LOG_FUNCTION (this << rid_v);
    MapFilterId::const_iterator fit (this->m_mapFilterId.find (rid_v));
    return (fit != this->m_mapFilterId.end()) ? fit->second : 0;
}

const Ibgp2d::FilterId & Ibgp2d::AssignFilterId (const Ipv4Address & rid_v) {
    NS_LOG_FUNCTION (this << rid_v);
    FilterId acl_v = ++this->m_lastFilterId;
    return (this->m_mapFilterId[rid_v] = acl_v);
}

std::string Ibgp2d::MakeAccessListName (const Ibgp2d::FilterId & filterId) {
    NS_LOG_FUNCTION (filterId);   // static
    std::ostringstream oss;
    oss << IBGP2_ROUTE_MAP_PREFIX << filterId;
    return oss.str();
}

std::string Ibgp2d::MakeRouteMapName (const Ibgp2d::FilterId & filterId) {
    NS_LOG_FUNCTION (filterId);   // static
    std::ostringstream oss;
    oss << IBGP2_ACCESS_LIST_PREFIX << filterId;
    return oss.str();
}

} // namespace ns3
