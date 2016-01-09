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
 *   Alexandre Morignot <alexandre.morignot@orange.com>
 */

// Default argv values.
#define DEFAULT_STOP_TIME      20.0         // Stopping time of the experiment (in s.)
#define DEFAULT_ROUTE_INTERVAL 0.0          // Default interval to dump routes. 0 means that print is disabled.

#include <iostream>                         // std::cout, std::cerr
#include <limits>                           // std::numeric_limits
#include <map>                              // std::map
#include <ostream>                          // std::ostringstream, std::endl
#include <regex>                            // std::regex (Requires CXXFLAGS += "-std=c++11")
#include <set>                              // std::set
#include <stdexcept>                        // std::runtime_error
#include <string>                           // std::string
#include <vector>                           // std::vector

#include <boost/tuple/tuple.hpp>            // boost::tie

#include "ns3/command-line.h"               // ns3::CommandLine
#include "ns3/dce-manager-helper.h"         // ns3::DceManagerHelper
#include "ns3/ibgp2d-helper.h"              // ns3::IBgp2dHelper
#include "ns3/ipv4-dce-routing-helper.h"    // ns3::Ipv4DceRoutingHelper
#include "ns3/ipv4-address.h"               // ns3::Ipv4Address, ns3::Ipv4Mask
#include "ns3/ipv4-interface-address.h"     // ns3::Ipv4InterfaceAddress
#include "ns3/ipv4-address-helper.h"        // ns3::Ipv4AddressHelper
#include "ns3/ipv4-prefix.h"                // ns3::Ipv4Prefix
#include "ns3/internet-stack-helper.h"      // ns3::InternetStackHelper
#include "ns3/log.h"                        // NS_LOG_*
#include "ns3/names.h"                      // ns3::Names
#include "ns3/net-device-container.h"       // ns3::NetDeviceContainer
#include "ns3/node-container.h"             // ns3::NodeContainer
#include "ns3/point-to-point-helper.h"      // ns3::PointToPointHelper
#include "ns3/quagga-helper.h"              // ns3::QuaggaHelper
#include "ns3/quagga-vty-helper.h"          // ns3::QuaggaVtyHelper
#include "ns3/simulator.h"                  // ns3::Simulator::*
#include "ns3/string.h"                     // ns3::StringValue

//#include "ns3/loopback-net-device.h"          // ns3::LoopbackNetDevice
//#include "ns3/simple-net-device.h"            // ns3::SimpleNetDevice
//#include "ns3/point-to-point-net-device.h"    // ns3::PointToPointNetDevice
//#include "ns3/bgp-sniffer-helper.h"           // ns3::BgpSnifferHelper

// Regular expressions used for parsing
#define RE_SPACE     "\\s+"
#define RE_WORD      "([^\\s]+)"
#define RE_IPV4      "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})"
#define RE_MASK      "(\\d{1,3})"
#define RE_METRIC    "(\\d+)"
#define RE_PREFIX_V4 "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}/\\d{1,3})"
#define RE_IBGP      "(UP|OVER|DOWN)"
#define RE_COMMENT   "\\s*(#.*)?"

// Intern simulation parameters
#define ASN1                    1            // AS1 is the simulated AS
#define ASN2                    2            // AS2 contains a single router announcing eBGP routes to AS1
#define EXTERN_ROUTER_NAME      "nh"         // The name of the router in AS2
#define _DEFAULT_MTU            1500         // Default MTU set on each link.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ( "DceIbgpv2Simu" );


//-----------------------------------------------------------------------------
// STL containers' << operators
//-----------------------------------------------------------------------------

template <typename T1, typename T2>
std::ostream & operator << ( std::ostream & out, const std::pair<T1, T2> & pair ) {
    out << '(' << pair.first << ", " << pair.second << ')';
    return out;
}

template <typename T>
std::ostream & operator << ( std::ostream & out, const std::vector<T> & vector ) {
    out << "[";
    for ( auto & elt : vector ) {
        out << ' ' << elt;
    }
    out << " ]";
    return out;
}

template <typename T>
std::ostream & operator << ( std::ostream & out, const std::set<T> & set ) {
    out << "{";
    for ( auto & elt : set ) {
        out << ' ' << elt;
    }
    out << " }";
    return out;
}

template <typename K, typename V>
std::ostream & operator << ( std::ostream & out, const std::map<K, V> & map ) {
    out << "{" << std::endl;
    for ( auto & pair : map ) {
        out << '\t' << pair.first << " => " << pair.second << std::endl;
    }
    out << "}" << std::endl;
    return out;
}
/**
 * Find in a map the value corresponding to a key.
 * \param map The map in which the key must be searched.
 * \param key The searched key.
 * \throws std::runtime_error if not found.
 * \returns A reference to the corresponding Value if any.
 */

template <typename Key, typename Value>
const Value & GetValue (
    const std::map<Key, Value> & map,
    const Key & key
) {
    typename std::map<Key, Value>::const_iterator fit ( map.find ( key ) );

    if ( fit == map.end() ) {
        std::ostringstream oss;
        oss << "Key error (" << key << ") in map: " << map << std::endl;
        std::string message = oss.str();
        throw std::runtime_error ( message );
    }

    return fit->second;
}

//-----------------------------------------------------------------------------
// Maps used to rebuild the graphs.
//-----------------------------------------------------------------------------

// Base type (needed to build the IGP graph)
typedef uint32_t    IgpWeight;
typedef std::string Hostname;

// Map needed to build the iBGP (if using Route Reflection)
typedef std::map<Ptr<Node>, Ipv4Address> MapBgpLoopback; // unused


template <typename Key, typename Id>
Id GetId (
    std::map<Key, Id> & map,
    const Key & key
) {
    Id ret;
    typename std::map<Key, Id>::const_iterator fit ( map.find ( key ) );

    if ( fit == map.end() ) {
        ret = map.size();
        map[key] = ret;
    } else {
        ret = fit->second;
    }

    return ret;
}

//-----------------------------------------------------------------------------
// IPV4 topology
//-----------------------------------------------------------------------------

/// This map stores for each point to point links the both corresponding IP addresses.
typedef std::map<
std::pair<Ptr<Node>,   Ptr<Node> >,
    std::pair<Ipv4Address, Ipv4Address>
    > MapLinkIps;

/**
 * \brief Create the key used in a MapLinkIps related to a given link.
 * \param srcNode A Node involved in the link.
 * \param dstNode The other Node.
 * \returns The corresponding pair of Ipv4Address.
 */
MapLinkIps::key_type MakeKey (
    Ptr<Node> srcNode,
    Ptr<Node> dstNode
) {
    // By convention, always the smallest node address (and the corresponding IP) first.
    return srcNode < dstNode ?
           std::make_pair ( srcNode, dstNode ) :
           std::make_pair ( dstNode, srcNode );
}

/**
 * \brief Find in a MapLinkIps the Ipv4Address related to a link (if any).
 * \param srcNode A Node involved in the link.
 * \param dstNode The other Node.
 * \throws std::runtime_error if not found.
 * \returns The corresponding pair of Ipv4Address.
 */

MapLinkIps::mapped_type GetIpv4Link (
    const  MapLinkIps & mapLinkIps,
    Ptr<Node> srcNode,
    Ptr<Node> dstNode
) {
    MapLinkIps::mapped_type ret;
    MapLinkIps::key_type key = MakeKey ( srcNode, dstNode );
    ret = GetValue ( mapLinkIps, key );
    if ( srcNode > dstNode ) {
        ret = std::make_pair ( ret.second, ret.first );
    }
    return ret;
}

/**
 * \brief Stores in a MapLinkIps the pair of Ipv4Address related to a link (if any).
 * \param srcNode A Node involved in the link.
 * \param dstNode The other Node.
 */

void AddIpv4Link (
    MapLinkIps & mapLinkIps,
    Ptr<Node> srcNode,
    Ptr<Node> dstNode,
    const Ipv4Address & srcIp,
    const Ipv4Address & dstIp
) {
    // By convention, always the smallest node address (and the corresponding IP) first.
    if ( srcNode < dstNode ) {
        mapLinkIps[std::make_pair ( srcNode, dstNode )] = std::make_pair ( srcIp, dstIp );
    } else {
        mapLinkIps[std::make_pair ( dstNode, srcNode )] = std::make_pair ( dstIp, srcIp );
    }
}

/**
 * \brief Configure a point-to-point link between two nodes.
 *    Note that a link is not directed.
 * \param ptp A PointToPointHelper instance which describe the delay and
 *    the bandwidth of the link that will be added.
 * \param srcNode A Node bounding this link.
 * \param dstNode A Node bounding this link.
 * \param ipv4AddrHelper
 * \param mapLinkIps
 * \returns The pair of Ipv4InterfaceAddress assigned resp. to srcNode and dstNode.
 */

std::pair<Ipv4InterfaceAddress, Ipv4InterfaceAddress> InstallLink (
    PointToPointHelper & ptp,
    Ptr<Node>            srcNode,
    Ptr<Node>            dstNode,
    Ipv4AddressHelper  & ipv4AddrHelper,
    MapLinkIps         & mapLinkIps
) {
    NS_ASSERT ( srcNode );
    NS_ASSERT ( dstNode );

    // Install the link. This adds a new interface on the both routers.
    NetDeviceContainer dc = ptp.Install ( srcNode, dstNode );

    // Configure an IP address on each interfaces
    Ipv4InterfaceContainer ifs = ipv4AddrHelper.Assign ( dc );

    // This network is reserved for those both nodes, iterate to the next network.
    ipv4AddrHelper.NewNetwork();

    const Ipv4InterfaceAddress
    & srcIf = ifs.Get ( 0 ).first->GetAddress ( ifs.Get ( 0 ).second, 0 ),
      & dstIf = ifs.Get ( 1 ).first->GetAddress ( ifs.Get ( 1 ).second, 0 );

    const Ipv4Address
    & srcIp = srcIf.GetLocal(),
      & dstIp = dstIf.GetLocal();

    AddIpv4Link ( mapLinkIps, srcNode, dstNode, srcIp, dstIp );

    return std::make_pair ( srcIf, dstIf );
}

/**
 * \brief Fix MTU on each interface. There is a bug with DCE (see
 * https://www.nsnam.org/bugzilla/show_bug.cgi?id=1627)
 * that result in wrong MTU for the last device of a node. The workaround
 * consists in setting the MTU of all the devices (even localhost).
 */

void FixMtu ( NodeContainer & nodes ) {
    for ( NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it ) {
        Ptr<Node> node = *it;
        size_t numDevices = node->GetNDevices();

        for ( size_t j = 0; j < numDevices ; ++j ) {
            Ptr<NetDevice> device = node->GetDevice ( j );
            device->SetMtu ( _DEFAULT_MTU );
        }
    }
}

/**
 * @brief Prepare an ns3::Ipv4AddressHelper that can be used to assign distinct
 *    IP addresses belonging to a pool of addresses.
 * @param prefix The pool of addresses
 * @return An ns3::Ipv4AddressHelper instance.
 */

Ipv4AddressHelper MakeIpv4AddressHelper ( const Ipv4Prefix & prefix ) {
    Ipv4AddressHelper ipv4AddressHelper;
    const Ipv4Mask    & mask = prefix.GetMask();
    const Ipv4Address & ipv4 = prefix.GetAddress().CombineMask ( mask );
    ipv4AddressHelper.SetBase ( ipv4, mask, Ipv4Address ( 1 ) ); // start allocation from x.x.x.1
    return ipv4AddressHelper;
}

//-----------------------------------------------------------------------------
// IGP topology
//-----------------------------------------------------------------------------

typedef std::map<
std::pair<Hostname, Hostname>,
    std::pair<IgpWeight, IgpWeight>
    > IgpLinks;

/**
 * \brief Parse an input stream and convert it into a IgpLinks structure.
 *   This stream must contains TSV lines containing successively :
 *   hostname_u hostname_v weight_uv weight_vu
 * \param ifs The input stream
 */

NodeContainer ParseIgpFile (
    std::istream & ifs,
    IgpLinks     & igpLinks
) {
    std::string line;
    std::set<Hostname> nodeNames;

    // Parse input IGP topology
    const std::regex regexIgp1 ( RE_WORD RE_SPACE RE_WORD RE_SPACE RE_METRIC );
    const std::regex regexIgp2 ( RE_WORD RE_SPACE RE_WORD RE_SPACE RE_METRIC RE_SPACE RE_METRIC );
    const std::regex regexComment ( RE_COMMENT );
    const IgpWeight infinity = std::numeric_limits<uint32_t>::max();

    for ( std::string line; std::getline ( ifs, line ); ) {
        std::smatch sm;
        Hostname srcName, dstName;
        IgpWeight srcWeight, dstWeight;

        if ( std::regex_match ( line, sm, regexComment ) ) {
            continue;
        } else if ( std::regex_match ( line, sm, regexIgp1 ) ) {
            srcName = sm[1];
            dstName = sm[2];
            srcWeight = std::stoi ( sm[3] );
            dstWeight = infinity;
        } else if ( std::regex_match ( line, sm, regexIgp2 ) ) {
            srcName = sm[1];
            dstName = sm[2];
            srcWeight = std::stoi ( sm[3] );
            dstWeight = std::stoi ( sm[4] );
        } else {
            std::cerr << "[??] " << line << std::endl;
            continue;
        }

        // Swap pair if needed

        std::pair<Hostname, Hostname> link;
        std::pair<IgpWeight, IgpWeight> metrics;

        if ( srcName < dstName ) {
            link  = std::make_pair ( srcName, dstName );
            metrics = std::make_pair ( srcWeight, dstWeight );
        } else {
            link  = std::make_pair ( dstName, srcName );
            metrics = std::make_pair ( dstWeight, srcWeight );
        }

        // Update igpLinks
        {
            IgpLinks::iterator fit ( igpLinks.find ( link ) );
            if ( fit != igpLinks.end() ) {
                const std::pair<IgpWeight, IgpWeight> & oldMetric = fit->second;

                if ((oldMetric.first  != infinity && metrics.first  != infinity) ||
                    (oldMetric.second != infinity && metrics.second != infinity)
                ) {
                    std::cerr << "[!!] Parallel IGP links ("
                        << srcName << " -> "
                        << dstName << "): picking lowest IGP metrics: "
                        << metrics << std::endl;
                }

                metrics = std::make_pair (
                    std::min ( metrics.first,  oldMetric.first ),
                    std::min ( metrics.second, oldMetric.second )
                );
            }
            igpLinks[link] = metrics;
        }

        // Update nodeNames
        nodeNames.insert ( srcName );
        nodeNames.insert ( dstName );
    }

    // Create the appropriate number of Nodes
    NodeContainer nodes;
    nodes.Create ( nodeNames.size() );

    // Assign name to each Node.
    uint32_t nodeId = 0;
    for ( std::set<Hostname>::const_iterator it = nodeNames.begin(); it != nodeNames.end(); ++it, ++nodeId ) {
        const std::string & nodeName = *it;
        Names::Add ( nodeName, nodes.Get ( nodeId ) );
    }

    return nodes;
}

//-----------------------------------------------------------------------------
// BGP topology
//-----------------------------------------------------------------------------


/**
 * \brief Work around to build BGP session between not neighboring routers (in
 *    the IP graph). We build an IP link which will be invisible in OSPF and it
 *    won't affect the shortest paths computed in OSPF. Those links are basically
 *    only use when the both router exchanges BGP information.
 * \param srcNode A Node involved in the session.
 * \param dstNode The other Node involved in the session.
 * \param ptp PointToPointHelper used to build the fake link.
 * \param mapLinkIps Maps the IP addresses assigned to each link of the IP topology.
 * \param Ipv4AddressHelper Used to assign addresses to the interfaces involved in
 *    the fake link.
 * \param prefix The prefix used to setup "ipv4AddressHelper" and used to configure
 *    the filter in OSPF.
 * \returns The IP addresses assigned respectively to "srcNode" and "dstNode".
 */

std::pair<Ipv4Address, Ipv4Address> InstallFakeLink (
    Ptr<Node>              srcNode,
    Ptr<Node>              dstNode,
    PointToPointHelper   & ptp,
    MapLinkIps           & mapLinkIps,
    Ipv4AddressHelper    & ipv4AddressHelper,
    const Ipv4Prefix     & prefix
) {
    Ipv4Address srcIp, dstIp;
    Ipv4InterfaceAddress srcIf, dstIf;

    boost::tie ( srcIf, dstIf ) = InstallLink ( ptp, srcNode, dstNode, ipv4AddressHelper, mapLinkIps );
    srcIp = srcIf.GetLocal();
    dstIp = dstIf.GetLocal();
    Ptr<OspfConfig> srcOspfConf = QuaggaHelper::GetConfig<OspfConfig> ( srcNode );
    Ptr<OspfConfig> dstOspfConf = QuaggaHelper::GetConfig<OspfConfig> ( dstNode );

    // This fake link must not be reannounced in OSPF (it must only be used to establish the BGP session)
    const char *aclName = "nofakelink";

    // a) Define distribute list
    //   distribute-list noeth0 out connected
    srcOspfConf->AddDistributeList ( OspfDistributeList ( aclName, OUT, REDISTRIBUTE_CONNECTED ) );
    dstOspfConf->AddDistributeList ( OspfDistributeList ( aclName, OUT, REDISTRIBUTE_CONNECTED ) );

    // b) Define access list
    //   access-list noeth0 deny x.x.x.x/y"
    //   access-list noeth0 permit any"
    AccessList acl ( aclName );
    acl.Add ( AccessListElement ( DENY, prefix ) );
    acl.Add ( AccessListElement ( PERMIT, Ipv4Prefix::Any() ) );

    srcOspfConf->AddAccessList ( acl );
    dstOspfConf->AddAccessList ( acl );

    return std::make_pair ( srcIp, dstIp );
}

/// Type of session. Notation from "A Model of BGP Routing for Network Engineering", Feamster & al, Sigcomm2004.

typedef enum {
    EBGP,   /**< eBGP session. */
    UP,     /**< iBGP session client to RR. */
    OVER,   /**< standard iBGP session.     */
    DOWN    /**< iBGP session RR to client. */
} BgpSessionType;

/**
 * \brief Write an BgpSessionType in an output stream.
 * \param out The output stream.
 * \param bgpSessionType The type of BGP session.
 * \returns The updated output stream.
 */

std::ostream & operator << ( std::ostream & out, const BgpSessionType & bgpSessionType ) {
    switch ( bgpSessionType ) {
    case EBGP:
        out <<  "eBGP";
        break;
    case DOWN:
        out <<  "iBGP (RR to client)";
        break;
    case UP:
    case OVER:
        out << "iBGP";
        break;
    }
    return out;
}

/**
 * \brief Configure a BGP session on a Node. To configure the BGP session, you must
 *    call this method for each of the both routers involved in the BGP session.
 * \param srcNode The router on which we configure the session.
 * \param dstNode Its BGP peer.
 * \param bgpSessionType The type of BGP session.
 * \param mapLinkIps This map will be completed to store the added IP links.
 * \return true iif successfull.
 */

bool InstallBgpSession (
    Ptr<Node>              srcNode,
    Ptr<Node>              dstNode,
    const BgpSessionType & bgpSessionType,
    MapLinkIps           & mapLinkIps
) {
    NS_ASSERT ( srcNode != dstNode );

    const std::string & srcName = Names::FindName ( srcNode );
    const std::string & dstName = Names::FindName ( dstNode );

    //  const Address & dstIp = GetValue(mapBgpLoopback, dstNode);
    Ipv4Address srcIp, dstIp;

    boost::tie ( srcIp, dstIp ) = GetIpv4Link ( mapLinkIps, srcNode, dstNode );

    std::cout << "[" << ( bgpSessionType == EBGP ? "EBGP" : "IBGP" )
              << "]: Establishing [" <<  bgpSessionType  << "] session between ["
              << srcName << "] (" << srcIp << ") and ["
              << dstName << "] (" << dstIp << ")" << std::endl;

    const std::string & description = dstName;

    // Configure the BGP session on srcNode
    {
        Ptr<BgpConfig> srcBgpConf = QuaggaHelper::GetConfig<BgpConfig> ( srcNode );
        Ptr<BgpConfig> dstBgpConf = QuaggaHelper::GetConfig<BgpConfig> ( dstNode );
        uint32_t srcAsn = srcBgpConf->GetAsn();
        uint32_t dstAsn = dstBgpConf->GetAsn();
        BgpNeighbor neighbor ( dstIp, dstAsn, description );
        if ( srcAsn == dstAsn && bgpSessionType == DOWN ) {
            neighbor.SetRouteReflectorClient ( true );
        }
        srcBgpConf->AddNeighbor ( neighbor );
    }

    return true;
}

/**
 * \brief Parse the input eBGP file, corresponding to routers receiving
 *   a quasi-equivalent route toward a same destination prefix from eBGP.
 *   In practice such routers are simply configured to announce this prefix
 *   inside the AS.
 * \param ifsEbgp The stream corresponding to the input file.
 * \param ptp PointToPointHelper used to build the IP links.
 * \param mapLinkIps This map will be completed to store the added IP links.
 * \param ipv4AddressHelper This Ipv4AddressHelper is used to assign the IP addresses
 *    to the interfaces which will be created.
 * \return true iif all the prefixes have been set successfully.
 */

bool ParseEbgpFile (
    std::istream       & ifsEbgp,
    PointToPointHelper & ptp,
    MapLinkIps         & mapLinkIps,
    Ipv4AddressHelper  & ipv4AddressHelper
) {
    const std::regex regexPrefix ( RE_WORD RE_SPACE RE_PREFIX_V4 );
    const std::regex regexComment ( RE_COMMENT );
    bool ret = true;
    const std::string FILTER_PREFIX = "filter_out_";

    // Find the Node of AS2 (it will be the BGP nexthop of each eBGP route entering AS1).
    Ptr<Node> node2 = Names::Find<Node> ( EXTERN_ROUTER_NAME );
    NS_ASSERT ( node2 );
    Ptr<BgpConfig> bgpConfig2 = QuaggaHelper::GetConfig<BgpConfig> ( node2 );
    NS_ASSERT ( bgpConfig2 );

    // For each input prefix...
    for ( std::string line; std::getline ( ifsEbgp, line ); ) {
        std::smatch sm;

        if ( std::regex_match ( line, sm, regexComment ) ) {
            continue;
        } else if ( std::regex_match ( line, sm, regexPrefix ) ) {
            const std::string
              & nodeName = sm[1],
              & prefix   = sm[2];

            if ( Ptr<Node> node1 = Names::Find<Node> ( nodeName ) ) {

                std::cout << "[EBGP]: Node [" << nodeName << "]: will receive an eBGP announce toward [" << prefix << "]" << std::endl;

                // Create the IP link between node1 and node2 if it does not yet exists.
                bool newSession = false;
                Ipv4Address ip1, ip2;
                try {
                    boost::tie ( ip1, ip2 ) = GetIpv4Link ( mapLinkIps, node1, node2 );
                } catch ( ... ) {
                    newSession  = true;
                    Ipv4InterfaceAddress i1, i2;
                    boost::tie ( i1, i2 ) = InstallLink ( ptp, node1, node2, ipv4AddressHelper, mapLinkIps );
                    ip1 = i1.GetLocal();
                    ip2 = i2.GetLocal();
                }

                const std::string filterName = FILTER_PREFIX + nodeName;

                if ( newSession ) {

                    // Setup the eBGP session on the both routers.
                    ret = InstallBgpSession ( node2, node1, EBGP, mapLinkIps );
                    NS_ASSERT ( ret );
                    ret &= InstallBgpSession ( node1, node2, EBGP, mapLinkIps );
                    NS_ASSERT ( ret );

                    // Install the filter to this new eBGP session.
                    BgpNeighbor & dstNeighbor = bgpConfig2->GetNeighbor ( ip1 );
                    dstNeighbor.AddPrefixList ( filterName, OUT );

                    // Install the prefix-list in the configuration file of node2
                    PrefixList prefixList ( filterName );
                    bgpConfig2->AddPrefixList ( prefixList );
                }

                // Configure node2 to originate the prefix
                bgpConfig2->AddNetwork ( Ipv4Prefix ( prefix.c_str() ) );

                // Allow this prefix to flow along the eBGP session established between node1 and node2
                PrefixList &  prefixList = bgpConfig2->GetPrefixList ( filterName );
                prefixList.Add ( PrefixListElement ( PERMIT, Ipv4Prefix ( prefix.c_str() ) ) );

            } else {
                std::cerr << "ParseEbgpFile: Invalid router name [" << nodeName << "] in line [" << line << "]" << std::endl;
                throw;
                ret = false;
            }

        } else {
            std::cerr << "ParseEbgpFile: Ignoring invalid line [" << line << "]" << std::endl;
        }
    }

    return ret;
}

/**
 * \brief Parse an input iBGP file, describing iBGP sessions established
 *   between routers. Each line are coma separated and correspond to a
 *   session and contains a first router name, a second router name, and
 *   a session type (UP|OVER|DOWN). See IbgpSession for further details.
 *   Sessions are bidirectional so it is useless to write an iBGP link
 *   in the both ways.
 * \param staticPrefix Pool of addresses use to install fake physical
 *   link between non adjacent routers (in OSPF) sharing an iBGP session.
 * \param ptp A PointToPointHelper instance.
 * \param mapLinkIps
 * \return true iif all the prefixes have been set successfully.
 */

bool ParseIbgpFile (
    std::istream         & ifs,
    Ipv4AddressHelper    & ipv4AddressHelper,
//    const MapBgpLoopback & mapBgpLoopback,
// The following parameters are only required to build fake links
    PointToPointHelper   & ptp,
    MapLinkIps           & mapLinkIps,
    Ipv4Prefix           & fakePrefix
) {
    bool ret = true;
    const std::regex regexIbgp ( RE_WORD RE_SPACE RE_WORD RE_SPACE RE_IBGP );
    const std::regex regexComment ( RE_COMMENT );
    std::string line;

    // Parse input iBGP topology
    for ( std::string line; std::getline ( ifs, line ); ) {
        std::smatch sm;

        if ( std::regex_match ( line, sm, regexComment ) ) {
            continue;
        } else if ( std::regex_match ( line, sm, regexIbgp ) ) {

            const std::string
                & srcName = sm[1],
                & dstName = sm[2],
                & type    = sm[3];

            // Get session type

            BgpSessionType bgpSessionType;

            if ( type == "UP" ) {
                bgpSessionType = UP;
            } else if ( type == "OVER" ) {
                bgpSessionType = OVER;
            } else if ( type == "DOWN" ) {
                bgpSessionType = DOWN;
            } else {
                std::cerr << "[!!] ParseIbgpFile: line : [" << line << "]: invalid iBGP session type ["
                          << type << "] (valid values are UP, OVER, DOWN)"  << std::endl;
                ret = false;
            }

            if ( srcName == dstName ) {
                std::cerr << "Skipping iBGP session from [" << srcName << "] to [" << dstName << ']' << std::endl;
                continue;
            }

            // Get Nodes

            const Ptr<Node> srcNode = Names::Find<Node> ( srcName );
            NS_ASSERT ( srcNode );

            const Ptr<Node> dstNode = Names::Find<Node> ( dstName );
            NS_ASSERT ( dstNode );

            try {
                GetIpv4Link ( mapLinkIps, srcNode, dstNode );
            } catch ( ... ) {
                Ipv4Address srcIp, dstIp;
                boost::tie ( srcIp, dstIp ) = InstallFakeLink ( srcNode, dstNode, ptp, mapLinkIps, ipv4AddressHelper, fakePrefix );
                std::cout << "[IPV4] Fake link installed from ["
                          << srcName << "] (" << srcIp << ") to ["
                          << dstName << "] (" << dstIp << ")" << std::endl;
            }
            ret &= InstallBgpSession ( srcNode, dstNode, bgpSessionType, mapLinkIps );

        } else {
            std::cout << "[??] " << line << std::endl;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------
// IGP topology utilities
//-----------------------------------------------------------------------------

/**
 * @brief Configure an OSPF adjacency on a router.
 * @param node The corresponding Node.
 * @param interface The interface that must be configured.
 * @param weight The corresponding OSPF weight.
 */

void SetupOspfInterface (
    Ptr<Node> node,
    const Ipv4InterfaceAddress & interface,
    const IgpWeight & weight
) {
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    const Ipv4Prefix prefix ( interface.GetLocal(), interface.GetMask() );
    uint32_t i = ipv4->GetInterfaceForPrefix ( prefix.GetAddress(), prefix.GetMask() );

    // This interface now speak OSPF.
    QuaggaHelper::EnableOspf ( node, prefix );
    Ptr<OspfConfig> ospfConfig = QuaggaHelper::GetConfig<OspfConfig> ( node );
    ospfConfig->AddRedistribute ( OspfRedistribute ( REDISTRIBUTE_CONNECTED, 1, 1000 ) );

    // Set the OSPF metric.
    //ospfConfig->AddInterface ( i, weight );
    std::string name = OspfConfig::MakeInterfaceName ( i );
    OspfInterface ospfInterface ( name, weight );
    ospfInterface.SetHelloInterval ( 2 );
    ospfInterface.SetDeadInterval ( 3 * ospfInterface.GetHelloInterval() );
    ospfInterface.SetTransmitDelay ( 1 );
    ospfInterface.SetRetransmitInterval ( 3 );
    ospfConfig->AddInterface ( ospfInterface );
}

/**
 * @brief Configure OSPF adjacencies and metrics. Nodes must be registered in Names::
 * @param ipv4AddressHelper Ipv4AddressHelper allocating IGP link per IGP link a /24 Ipv4Prefix.
 * @param ptp PointToPointHelper setting links set up between each pair of adjacent Nodes.
 * @param igpLinks The IGP links that must been configured.
 * @param mapLinkIps This map is filled according to the Ipv4Address assigned to each nodes.
 */

void BuildOspfTopology (
    Ipv4AddressHelper & ipv4AddressHelper, // based on igpPrefix
    PointToPointHelper & ptp,
    const IgpLinks & igpLinks,
    MapLinkIps & mapLinkIps
) {
    const uint32_t infinity = std::numeric_limits<uint32_t>::max();
    for ( auto & lw : igpLinks ) {
        std::string srcName, dstName;
        IgpWeight srcWeight, dstWeight;
        boost::tie ( srcName, dstName ) = lw.first;
        boost::tie ( srcWeight, dstWeight ) = lw.second;

        // Find src Node
        Ptr<Node> srcNode = Names::Find<Node> ( srcName );
        if ( !srcNode ) {
            throw std::runtime_error ( std::string ( "BuildOspfTopology: router not found: " ) + srcName );
        }

        // Find dst Node
        Ptr<Node> dstNode = Names::Find<Node> ( dstName );
        if ( !srcNode ) {
            throw std::runtime_error ( std::string ( "BuildOspfTopology: router not found: " ) + dstName );
        }

        // Install a new link (note that several links might connect srcNode and dstNode) if several line "u v w_u w_v" occurs
        Ipv4InterfaceAddress srcIf, dstIf;
        boost::tie ( srcIf, dstIf ) = InstallLink ( ptp, srcNode, dstNode, ipv4AddressHelper, mapLinkIps );
        std::cout << "[IPV4]: Link installed: [" << srcName << "] (" << srcIf.GetLocal() << ") -- [" << dstName << "] (" << dstIf.GetLocal() << ")" << std::endl;

        // Configure src interface
        if ( srcWeight != infinity ) {
            SetupOspfInterface ( srcNode, srcIf, srcWeight );
            std::cout << "[OSPF]: Node [" << srcName << "]: interface " << srcIf.GetLocal() << ": metric = " << srcWeight << std::endl;
        }

        if ( dstWeight != infinity ) {
            SetupOspfInterface ( dstNode, dstIf, dstWeight );
            std::cout << "[OSPF]: Node [" << dstName << "]: interface " << dstIf.GetLocal() << ": metric = " << dstWeight << std::endl;
        }
    }
}

//-----------------------------------------------------------------------------
// BGP utilities
//-----------------------------------------------------------------------------

/**
 * \brief Determine for each router an arbitrary IP address that will be used
 *   to establish BGP session.
 * \param nodes The Node embedding BGPd.
 * \param mapBgpLoopback The MapBgpLoopback that will be populated consequently.
 */

// plop
bool InstallBgpLoopback (
    NodeContainer    & nodes,
    MapBgpLoopback   & mapBgpLoopback,
    const Ipv4Prefix & prefixLoopback,
    const Ipv4DceRoutingHelper & ipv4DceRoutingHelper
) {
    //////////////////////////////////////////////////////////////////////////////////////////
    // INTERFACE PHYSIQUES
    //////////////////////////////////////////////////////////////////////////////////////////

    /*
    const Ipv4Address LOCALHOST("127.0.0.1");
    uint32_t numNodes = nodes.GetN();

    // If a neighbor v is reached using its interface i1 but return BGP packets using i2, the session won't be mounted.
    for (uint32_t i = 0; i < numNodes; ++i) {
        Ptr<Node> node = nodes.Get(i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        uint32_t numInterfaces = ipv4->GetNInterfaces();
        bool assigned = false;

        for (uint32_t j = 0; j < numInterfaces; ++j) {
            const Ipv4InterfaceAddress & ipv4InterfaceAddress = ipv4->GetAddress(j, 0);
            const Ipv4Address & ipv4Address = ipv4InterfaceAddress.GetLocal();
            if (ipv4Address != LOCALHOST) {
                assigned = true;
                mapBgpLoopback[node] = ipv4Address;
                QuaggaHelper::SetRouterId(node, ipv4Address);
                break;
            }
        }

        if (!assigned) {
            std::cerr << "The loopback prefix is too small, cannot assign distinct loopback for each routers" << std::endl;
            return false;
        }
    }
    */

    //////////////////////////////////////////////////////////////////////////////////////////
    /*
    const Ipv4Mask & mask = prefixLoopback.GetMask();
    uint32_t numNodes = nodes.GetN();
    Ipv4Address startAddress = prefixLoopback.GetAddress().CombineMask(mask);
    uint32_t start = startAddress.Get();
    Ipv4Address loopbackAddress;

    // Assign a loopback per node
    uint32_t i = 1;
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it, ++i) {
        Ptr<Node> node = *it;

        if (!(i & 0x000000ff)) i++; // skip ip x.x.x.0
        loopbackAddress.Set(start + i);

        if (loopbackAddress.CombineMask(mask) == startAddress) {

            // Memorize the loopback address
            mapBgpLoopback[node] = loopbackAddress;

            // Configure the loopback address (NS side)
            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
            ipv4->RemoveAddress(0, 0);
            ipv4->AddAddress(0, Ipv4InterfaceAddress(loopbackAddress, Ipv4Mask::GetOnes()));

            // Assign the router ID (requires that this script is run using root privilege)
            QuaggaHelper::SetRouterId(node, loopbackAddress);

            // Configure the loopback address (DCE side)
                        QuaggaHelper::AddLoopback(node, loopbackAddress);

        } else {
            std::cerr << "Can't set loopback for node [" << Names::FindName(node) << "] (outside " << address << "/" << mask << ")" << std::endl;
            return false;
        }
    }
    */

//    //////////////////////////////////////////////////////////////////////////////////////////
//
//    // Install a loopback per router
//    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
//    std::cout << "Installing loopback" << std::endl;
//    NetDeviceContainer loopbacks;
//
//    uint32_t i = 1;
//    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it, ++i) {
//        Ptr<Node> node = *it;
//        //Ptr<LoopbackNetDevice> loopback = CreateObject<LoopbackNetDevice> ();
//        Ptr<PointToPointNetDevice> loopback = CreateObject<PointToPointNetDevice> ();
//        loopbacks.Add(loopback);
//
//        // v1 Add the interface
//        {
//            node->AddDevice(loopback);
//        }
//
//        /*
//        // v2 Add the interface
//        {
//            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//            uint32_t indexLoopback = ipv4->AddInterface(loopback);
//
//            // Craft loopback address TO FIX
//            std::ostringstream oss;
//            oss << "50.0.0." << i;
//            Ipv4Address loopbackAddress(oss.str().c_str());
//            mapBgpLoopback[node] = loopbackAddress;
//
//            // Install address
//            Ipv4InterfaceAddress address = Ipv4InterfaceAddress(loopbackAddress, Ipv4Mask::GetOnes());
//            ipv4->AddAddress(indexLoopback, address);
//            ipv4->SetMetric(indexLoopback, 1);
//            ipv4->SetUp(indexLoopback);
//
//            // Install route
//            Ipv4StaticRoutingHelper ipv4RoutingHelper;
//            Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
//            staticRouting->AddHostRouteTo(loopbackAddress, indexLoopback);
//
//            // Add loopback address to each interface
//            uint32_t numInterfaces = ipv4->GetNInterfaces();
//            for (uint32_t j = 1; j < numInterfaces - 1; ++j) {
//                ipv4->AddAddress(j, address);
//            }
//
//        }
//        */
//
//    }
//
//    // Assign an IP for each loopback
//    std::cout << "Assigning loopback IPs" << std::endl;
//    Ipv4AddressHelper ipv4AddrHelper;
//    ipv4AddrHelper.SetBase(prefixLoopback.GetAddress(), Ipv4Mask::GetOnes());
//    Ipv4InterfaceContainer iic = ipv4AddrHelper.Assign(loopbacks);
//
//    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
//        Ptr<Node> node = nodes.Get(i);
//
//        // Memorize the loopback assigned to each router
//        const Ipv4Address & loopbackAddress = iic.GetAddress(i);
//        mapBgpLoopback[node] = loopbackAddress;
//
//        // Use this address as router-id
//        QuaggaHelper::SetRouterId(node, loopbackAddress);
//
//        /*
//        // Declare the loopback in quagga
//        {
//            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//            uint32_t indexLoopback = ipv4->GetNInterfaces();
//            QuaggaHelper::AddLoopback(node, "ns3-device" + indexLoopback, loopbackAddress);
//        }
//        QuaggaHelper::AddStaticRoute(node, loopbackAddress, Ipv4Prefix(loopbackAddress, Ipv4Mask::GetOnes()));
//        */
//
//        // Change loopback address
//        /*
//        {
//            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//            ipv4->RemoveAddress(0, 0);
//            ipv4->AddAddress(0, Ipv4InterfaceAddress(loopbackAddress, Ipv4Mask::GetOnes()));
//        }
//        */
//
//        // v1) Add static route ns3-side to the loopback we've just installed on each Node
//        /*
//        {
//            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//            uint32_t indexLoopback = ipv4->GetNInterfaces();
//
//            Ipv4StaticRoutingHelper ipv4RoutingHelper;
//            Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting(ipv4);
//            staticRouting->AddHostRouteTo(loopbackAddress, indexLoopback, indexLoopback);
//        }
//        */
//
//        // v2) Add static route ns3-side to the loopback we've just installed on each Node
//        /*
//        {
//            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//            uint32_t indexLoopback = ipv4->GetNInterfaces();
//            Ptr<Ipv4StaticRouting> staticRouting = ipv4DceRoutingHelper.GetStaticRouting(ipv4);
//            staticRouting->AddHostRouteTo(loopbackAddress, indexLoopback);
//        }
//        */
//
//        /*
//        {
//            Ptr<Ipv4L3Protocol> ipv4l3 = node->GetObject<Ipv4L3Protocol>();
//            std::cout << "ipv4l3 = " << ipv4l3 << std::endl;
//        }
//        */
//
//        // Configure an OSPF adjacency toward the loopback's prefix
//        /*
//        {
//            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//            uint32_t indexLoopback = ipv4->GetNInterfaces();
//            QuaggaHelper::OspfAddIf(node, indexLoopback, 10);
//        }
//        */
//    }
//    std::cout << mapBgpLoopback << std::endl;
//
//    std::cout << "OK" << std::endl;
    return true;
}

//-----------------------------------------------------------------------------
// Simulation utilities
//-----------------------------------------------------------------------------

/**
 * \brief Install on each Node an OutputStreamWrapper which logs to a
 *   dedicated output file the routes installed in the FIB of each router.
 * \param nodes The Node instances we wan't to monitor.
 * \param ipv4AddrHelper
 * \param routeInterval The interval (in seconds) between each dump. No dump is
 *    performed at t = 0.
 */

void DumpRoutesPeriodically (
    NodeContainer        & nodes,
    Ipv4DceRoutingHelper & ipv4DceRoutingHelper,
    Time                   routeInterval
) {
    for ( NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it ) {
        Ptr<Node> node = *it;
        const std::string & nodeName = Names::FindName ( node );

        std::ostringstream oss;
        oss << "routes_" << nodeName << ".log";
        const std::string outputFilename = oss.str();

        Ptr<OutputStreamWrapper> rs = Create<OutputStreamWrapper> ( outputFilename, std::ios::out );

        ipv4DceRoutingHelper.PrintRoutingTableEvery ( routeInterval, node, rs );
    }
}

/**
 * \brief Print the IP addresses assigned to the interfaces of a given Node.
 * \param out The output stream.
 * \param node A pointer to the Node.
 */

void IfConfig (
    std::ostream & out,
    uint32_t       nodeId,
    Ptr<Node>      node
) {
    const std::string & nodeName = Names::FindName ( node );
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    uint32_t numInterfaces = ipv4->GetNInterfaces();

    for ( uint32_t j = 0; j < numInterfaces; ++j ) {
        for ( uint32_t k = 0; k < ipv4->GetNAddresses ( j ); ++k ) {
            const Ipv4InterfaceAddress & ipv4InterfaceAddress = ipv4->GetAddress ( j, k );
            const Ipv4Address & ipv4Address = ipv4InterfaceAddress.GetLocal();
            out << nodeId << '\t' << nodeName << "\t@" << j << "," << k << '\t' << ipv4Address << std::endl;
        }
    }
}

/**
 * \brief Print the IP addresses assigned to the interfaces of a set of Nodes.
 * \param out The output stream.
 * \param nodes The Node instances we want to monitor.
 */

void DumpIfConfig (
    std::ostream        & out,
    const NodeContainer & nodes
) {
    out << "# id\tname\tif\taddress" << std::endl;
    uint32_t numNodes = nodes.GetN();
    for ( uint32_t i = 0; i < numNodes; ++i ) {
        Ptr<Node> node = nodes.Get ( i );
        IfConfig ( out, i, node );
    }
}

//-----------------------------------------------------------------------------
// Main program
//-----------------------------------------------------------------------------

#define HELP_STOP_TIME       "Time to stop (in seconds)"
#define HELP_VERBOSE         "Set verbose mode"
#define HELP_DEBUG           "Set debug mode"
#define HELP_QUAGGA          "Set debug mode (quagga)"
#define HELP_ROUTES          "Output route every 10s if set to true"
#define HELP_IGP             "Path to an input CSV file (router_src,router_dst,network,metric) describing the IGP network topology"
#define HELP_EBGP            "Path to an input CSV file (border_router,prefix) describing the concurrent quasi-equivalent eBGP routes"
#define HELP_IBGP            "Path to an input CSV file (router_src,router_dst,UP|OVER|DOWN) where DOWN stands for a RR-to-client iBGP session, OVER for a legacy iBGP session"
#define HELP_IBGP_MODE       "Set the iBGP topology: 0 = iBGP full mesh, 1 = Route Reflection (requires --ibgp), 2 = iBGPv2. Default: 2"
#define HELP_ROUTES_INTERVAL "Specify the interval (in seconds) between each route dump (see ns3/source/ns-3-dce/routes_*.log). If set to 0, no route dump is performed. Default: 0"

typedef enum {
    IBGP_FM = 0,
    IBGP_RR,
    IBGP_V2,
} IBgpMode;

int main ( int argc, char *argv[] ) {
    // Parameters
    double   stopTime      = DEFAULT_STOP_TIME;
    bool     verbose       = false;
    bool     debug         = false;
    bool     debugQuagga   = false;
    double   routeInterval = DEFAULT_ROUTE_INTERVAL;
    int      ibgpMode      = IBGP_V2;
    std::string filenameIbgp, filenameIgp, filenameEbgp;

    CommandLine cmd;
    cmd.AddValue ( "stopTime",       HELP_STOP_TIME,       stopTime );
    cmd.AddValue ( "verbose",        HELP_VERBOSE,         verbose );
    cmd.AddValue ( "debug",          HELP_DEBUG,           debug );
    cmd.AddValue ( "debugQuagga",    HELP_QUAGGA,          debugQuagga );
    cmd.AddValue ( "routesInterval", HELP_ROUTES_INTERVAL, routeInterval );
    cmd.AddValue ( "igp",            HELP_IGP,             filenameIgp );
    cmd.AddValue ( "ibgp",           HELP_IBGP,            filenameIbgp );
    cmd.AddValue ( "ibgpMode",       HELP_IBGP_MODE,       ibgpMode );
    cmd.AddValue ( "ebgp",           HELP_EBGP,            filenameEbgp );
    cmd.Parse ( argc, argv );

    if ( verbose ) {
        LogComponentEnable ( "Ibgp2d",       LOG_LEVEL_INFO );
        LogComponentEnable ( "QuaggaHelper", LOG_LEVEL_INFO );
        LogComponentEnableAll ( LOG_PREFIX_TIME );
    }

    if ( debug ) {
        LogComponentEnable ( "TcpClientHelper", LOG_LEVEL_ALL );
        LogComponentEnable ( "Ibgp2d",          LOG_LEVEL_ALL );
        LogComponentEnable ( "OspfGraphHelper", LOG_LEVEL_ALL );
        LogComponentEnable ( "QuaggaHelper",    LOG_LEVEL_ALL );
        LogComponentEnableAll ( LOG_PREFIX_TIME );
    }

    Ipv4Prefix as1IgpPrefix ( "1.0.0.0/24" );   /// Pool of addresses use to install IP links between adjacent routers (in OSPF) sharing an iBGP session.
    Ipv4Prefix as1as2Prefix ( "2.0.0.0/24" );   /// Pool of addresses use to install IP links between AS1 and AS2 (static routes)
    Ipv4Prefix as1FakePrefix ( "254.0.0.0/24" ); /// Pool of addresses use to install fake IP links between non adjacent routers (in AS1, in OSPF).


    // Build routers of AS1.
    // Count the number of routers listed in the input IGP topology.
    NodeContainer nodes1;
    IgpLinks igpLinks;
    {
        std::ifstream ifsIgp ( filenameIgp );
        if ( !ifsIgp ) {
            std::cerr << "Can't read input IGP file [" << filenameIgp << ']' << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Reading IGP topology [" << filenameIgp << ']' << std::endl;
        nodes1 = ParseIgpFile ( ifsIgp, igpLinks );
        ifsIgp.close();
    }

    // Build router of AS2.
    // This AS is made of 1 router which will announce eBGP routes to ASBRs of AS1.
    NodeContainer nodes2;
    {
        NS_ASSERT ( !Names::Find<Node> ( EXTERN_ROUTER_NAME ) );
        nodes2.Create ( 1 );
        Names::Add ( EXTERN_ROUTER_NAME, nodes2.Get ( 0 ) );
    }

    // Nodes gather all the nodes of AS1 and AS2
    NodeContainer nodes;
    nodes.Add ( nodes1 );
    nodes.Add ( nodes2 );

    // Install ns3 socket stack for each node
    DceManagerHelper processManager;
    processManager.SetNetworkStack ( "ns3::Ns3SocketFdFactory" );
    processManager.Install ( nodes );

    InternetStackHelper internetStackHelper;
    Ipv4DceRoutingHelper ipv4DceRoutingHelper;
    internetStackHelper.SetRoutingHelper ( ipv4DceRoutingHelper );
    internetStackHelper.Install ( nodes );

    if ( routeInterval ) {
        DumpRoutesPeriodically ( nodes, ipv4DceRoutingHelper, Seconds ( routeInterval ) );
    }

    // Enable debug in quagga if required
    if ( debugQuagga ) {
        QuaggaHelper::SetDebug<ZebraConfig> ( nodes, true );
        QuaggaHelper::SetDebug<OspfConfig> ( nodes, true );
        QuaggaHelper::SetDebug<BgpConfig> ( nodes, true );
    }

    // Define physical links attribute
    PointToPointHelper ptp;
    ptp.SetDeviceAttribute ( "DataRate", StringValue ( "5Mbps" ) );
    ptp.SetChannelAttribute ( "Delay", StringValue ( "2ms" ) );

    // Build IGP topology intern to AS1
    MapLinkIps mapLinkIps;
    {
        Ipv4AddressHelper ipv4AddressHelper = MakeIpv4AddressHelper ( as1IgpPrefix );
        BuildOspfTopology ( ipv4AddressHelper, ptp, igpLinks, mapLinkIps );
    }

    FixMtu ( nodes );

    // Configure eBGP settings: we announce through BGP some prefix to mimic eBGP announces
    QuaggaHelper::EnableBgp ( nodes );
    QuaggaHelper::SetAsn ( nodes1, ASN1 );
    QuaggaHelper::SetAsn ( nodes2, ASN2 );
    {
        std::ifstream ifsEbgp ( filenameEbgp );
        if ( !ifsEbgp ) {
            std::cerr << "Can't read prefix file [" << filenameEbgp << ']' << std::endl;
            return EXIT_FAILURE;
        }

        // Parse the input file and configure consequently the routers.
        Ipv4AddressHelper ipv4AddressHelper = MakeIpv4AddressHelper ( as1as2Prefix );
        ParseEbgpFile ( ifsEbgp, ptp, mapLinkIps, ipv4AddressHelper );
        ifsEbgp.close();
    }

    // Configure iBGP settings on the routers
    Ibgp2dHelper ibgp2dHelper ( ASN1 ); // iBGP2 specific

    switch ( ibgpMode ) {
    case IBGP_V2:
        break;

    case IBGP_FM: {
        std::cout << "[IBGP]: Configuring the iBGP full mesh on the routers" << std::endl;
        Ipv4AddressHelper ipv4AddressHelper = MakeIpv4AddressHelper ( as1FakePrefix );


        // For each BGP router, choose an IP address which will identify them
        // in the BGP configuration files.
//                MapBgpLoopback mapBgpLoopback;
//                InstallBgpLoopback(nodes, mapBgpLoopback, loopbackPrefix, ipv4DceRoutingHelper);

        // Establish iBGP session between each pair of distinct Nodes
        // belonging to the same Autonomous System.
        for ( NodeContainer::Iterator srcIt = nodes.Begin(); srcIt != nodes.End(); ++srcIt ) {
            Ptr<Node> srcNode = *srcIt;
            const uint32_t srcAsn = QuaggaHelper::GetConfig<BgpConfig> ( srcNode )->GetAsn ();
            for ( NodeContainer::Iterator dstIt = nodes.Begin(); dstIt != nodes.End(); ++dstIt ) {
                Ptr<Node> dstNode = *dstIt;
                if ( srcNode == dstNode ) {
                    continue;
                }

                const uint32_t dstAsn = QuaggaHelper::GetConfig<BgpConfig> ( dstNode )->GetAsn ();

                if ( srcAsn == dstAsn ) {
                    // iBGP session
                    try {
                        GetIpv4Link ( mapLinkIps, srcNode, dstNode );
                    } catch ( ... ) {
                        Ipv4Address srcIp, dstIp;
                        std::string srcName = Names::FindName ( srcNode );
                        std::string dstName = Names::FindName ( dstNode );
                        boost::tie ( srcIp, dstIp ) = InstallFakeLink ( srcNode, dstNode, ptp, mapLinkIps, ipv4AddressHelper, as1FakePrefix );
                        std::cout << "[IPV4] Fake link installed from ["
                                  << srcName << "] (" << srcIp << ") to ["
                                  << dstName << "] (" << dstIp << ")" << std::endl;

                    }
                    InstallBgpSession ( srcNode, dstNode, OVER, mapLinkIps );
                }
            }
        }
    }
    break;

    case IBGP_RR: {
        std::cout << "[IBGP]: Configuring the iBGP Route Reflection topology" << std::endl;
        Ipv4AddressHelper ipv4AddressHelper = MakeIpv4AddressHelper ( as1FakePrefix );

        // For each BGP router, choose an IP address which will identify them
        // in the BGP configuration files.
//                MapBgpLoopback mapBgpLoopback;
//                InstallBgpLoopback(nodes, mapBgpLoopback, loopbackPrefix, ipv4DceRoutingHelper);

        // Load the concurrent BGP prefix file
        {
            std::ifstream ifsIbgp ( filenameIbgp );
            if ( !ifsIbgp ) {
                std::cerr << "Can't iBGP topology file [" << filenameIbgp << ']' << std::endl;
                return EXIT_FAILURE;
            }

            // Parse the input file and configure consequently the routers.
            if ( !ParseIbgpFile ( ifsIbgp, ipv4AddressHelper, ptp, mapLinkIps, as1FakePrefix ) ) {
                std::cerr << "Error while parsing the iBGP topology file [" << filenameIbgp << ']' << std::endl;
                ifsIbgp.close();
                return EXIT_FAILURE;
            }

            ifsIbgp.close();
        }

    }
    break;
    }

    // Dump IP configuration of each router
    std::ofstream ofsIfconfig ( "ifconfig.csv" );
    if ( ofsIfconfig ) {
        DumpIfConfig ( ofsIfconfig, nodes );
        ofsIfconfig.close();
    }

    DumpIfConfig ( std::cout, nodes );
    QuaggaHelper quaggaHelper;
    quaggaHelper.Install ( nodes );

    // Install iBGPv2 after quagga.
    if ( ibgpMode == IBGP_V2 ) {

        std::cout << "[IBGP]: Configuring iBGPv2 on the routers" << std::endl;
        ibgp2dHelper.Install ( nodes1 );
    }

    // Prepare telnet to fetch result at the end of the simulation
    QuaggaVtyHelper quaggaVtyHelper;
    {
        // bgpd
        {
            QuaggaVtyHelper::Commands bgpdCommands;
            bgpdCommands.push_back ( "show ip bgp" );
            bgpdCommands.push_back ( "show ip bgp summary" );
            bgpdCommands.push_back ( "show bgp memory" );
            bgpdCommands.push_back ( "show ip bgp neighbor" );
            quaggaVtyHelper.AddCommands ( nodes, Seconds ( stopTime - 1 ), "bgpd", bgpdCommands );
        }

        // zebra
        {
            QuaggaVtyHelper::Commands zebraCommands;
            zebraCommands.push_back ( "show ip route" );
            quaggaVtyHelper.AddCommands ( nodes, Seconds ( stopTime - 1 ), "zebra", zebraCommands );
        }
    }

    // Run the simulation and stop at stopTime.
    // Note that if you choose a too small value, BGPd won't have enough time to run.
    NS_LOG_INFO ( "Starting experiment until t = " << stopTime << "s." );
    Simulator::Stop ( Seconds ( stopTime ) );
    Simulator::Run();

    /*
    if ( ibgpMode == IBGP_V2 ) {
        // At the end of the simulation, draw the IGP graph of each routers
        for ( NodeContainer::Iterator it = nodes1.Begin(); it != nodes1.End(); ++it ) {
            Ptr<Node> node = *it;

            // Print IGP graph
            const std::string & nodeName = Names::FindName ( node );
            const Node & _node = *node;
            std::cout << "Router " << nodeName << std::endl;
            ibgp2dHelper.DumpIgpGraphviz ( std::cout, _node );
        }
    }
    */

    // Release the memory and leave gracefully
    Simulator::Destroy();
    quaggaVtyHelper.Close();

    NS_LOG_INFO ( "End of experiment" );

    return EXIT_SUCCESS;
}


