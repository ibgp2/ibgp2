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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 * Modified by:
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 */

#ifndef QUAGGA_HELPER_H
#define QUAGGA_HELPER_H

#include <string>                           // std::string

#include "ns3/address.h"                    // ns3::Address
#include "ns3/application-container.h"      // ns3::ApplicationContainer
#include "ns3/ipv4-prefix.h"                // ns3::Ipv4Prefix
#include "ns3/node.h"                       // ns3::Node
#include "ns3/node-container.h"             // ns3::NodeContainer
#include "ns3/object.h"                     // CreateObject
#include "ns3/ptr.h"                        // ns3::Ptr

#include "ns3/zebra-config.h"               // ns3::ZebraConfig
#include "ns3/bgp-config.h"                 // ns3::BgpConfig
#include "ns3/ospf-config.h"                // ns3::OspfConfig
#include "ns3/ospf6-config.h"               // ns3::Ospf6Config
#include "ns3/rip-config.h"                 // ns3::RipConfig
#include "ns3/ripng-config.h"               // ns3::RipngConfig

#include "ns3/dce-application.h"            // ns3::Application
#include "ns3/dce-application-helper.h"     // ns3::DceApplicationHelper

namespace ns3
{

std::string GetNodeName ( Ptr<Node> node );

/**
 * @brief Create a quagga routing daemon as an application and associate it
 *    to a Node.
 *
 * Remember that all methods able to handle a NodeContainer may also handle
 * a Ptr<Node> also.
 */

class QuaggaHelper
{

private:

    /**
     * \internal
     */
  //  bool m_useManualConf; /**< Turn on this flag to use a manual Zebra configuration */

    ApplicationContainer InstallPriv ( Ptr<Node> node );

    /**
     * @brief Prepare the filesystem (directories, configuration files...) of a router.
     * Valid TConfig types are:
     *    BgpdConfig
     *    Ospf6Config
     *    OspfConfig
     *    RipConfig
     *    RipngConfig
     *    ZebraConfig
     * @param node The corresponding node.
     * @return The corresponding *Config object
     */

    template <typename TConfig>
    static void Generate ( Ptr<Node> node ) {
        Ptr<TConfig> config = node->GetObject<TConfig> ();

        // This is to get the node's name in the quagga's prompt.
        const std::string nodeName = GetNodeName ( node );
        if ( nodeName != "" ) {
            config->SetHostname ( nodeName );
        }

        // Write the configuration file on the filesystem
        config->CreateDirectories ( node );
        config->WriteConfigFile ( node );
    }

    /**
     * @brief Install a daemon on the router if the corresponding *Config object
     * is embedded on the Node. If sothe corresponding Application is appended
     * in the ApplicationContainer passed in parameter.
     *
     * Valid TConfig types are:
     *    BgpdConfig
     *    Ospf6Config
     *    OspfConfig
     *    RipConfig
     *    RipngConfig
     *    ZebraConfig
     * @param node The corresponding node.
     * @param apps The ApplicationContainer
     * @return The corresponding *Config object
     */

    template <typename TConfig>
    static void InstallDaemon ( Ptr<Node> node, ApplicationContainer & apps ) {
        DceApplicationHelper process;
        process.SetStackSize ( 1 << 16 );

        if ( Ptr<TConfig> config = node->GetObject<TConfig> () ) {
            QuaggaHelper::Generate<TConfig> ( node );
            process.ResetArguments ();
            process.SetBinary ( config->GetDaemonName() );
            process.AddArguments ( "-f", config->GetConfigFilename () );
            process.AddArguments ( "-i", config->GetPidFilename() );
            apps.Add ( process.Install ( node ) );

            Ptr<Application> app = apps.Get ( apps.GetN() - 1 );
            app->SetStartTime ( config->GetStartTime() );
            node->AddApplication ( app );
        }
    }

public:

    /**
     * Create a QuaggaHelper which is used to make life easier for people wanting
     * to use quagga Applications.
     */

    QuaggaHelper ();

    /**
     * Install a quagga application on each Node in the provided NodeContainer.
     * @param nodes The NodeContainer containing all of the nodes to get a quagga
     *    application via ProcessManager.
     * @returns A list of quagga applications, one for each input node
     */

    ApplicationContainer Install ( NodeContainer nodes );

    /**
     * Install a quagga application on the provided Node.  The Node is specified
     * directly by a Ptr<Node>
     * @param node The node to install the QuaggaApplication on.
     * @returns An ApplicationContainer holding the quagga application created.
     */

    ApplicationContainer Install ( Ptr<Node> node );

    /**
     * Install a quagga application on the provided Node.  The Node is specified
     * by a string that must have previosly been associated with a Node using the
     * Object Name Service.
     * @param nodeName The node to install the ProcessApplication on.
     * @returns An ApplicationContainer holding the quagga application created.
     */

    ApplicationContainer Install ( const std::string & nodeName );

    //----------------------------------------------------------------------------
    // Method common to all the daemons
    //----------------------------------------------------------------------------

    /**
     * @brief Enable or disable the debug instructions on a range *Config object where TConfig may be:
     *    BgpdConfig
     *    Ospf6Config
     *    OspfConfig
     *    RipConfig
     *    RipngConfig
     *    ZebraConfig
     * @param nodes The corresponding nodes.
     * @param on Pass true to enable debug, false otherwise.
     * @return The corresponding *Config object
     */

    template <typename TConfig>
    static void SetDebug ( NodeContainer nodes, bool on = true ) {
        for ( NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it ) {
            Ptr<Node> node = *it;
            Ptr<TConfig> config = QuaggaHelper::GetConfig<TConfig> ( node );
            config->SetDebug ( on );
        }
    }

    /**
     * @brief Retrieve (and create if not exists) on a given router a *Config object where TConfig may be:
     *    BgpdConfig
     *    Ospf6Config
     *    OspfConfig
     *    RipConfig
     *    RipngConfig
     *    ZebraConfig
     * Usually you should call Enable* methods to set default parameters.
     * @param node The corresponding node.
     * @return The corresponding *Config object
     */

    template <typename TConfig>
    static Ptr<TConfig> GetConfig ( Ptr<Node> node ) {
        Ptr<TConfig> config = node->GetObject<TConfig>();

        if ( !config ) {
            config = CreateObject<TConfig>();
            node->AggregateObject ( config );
        }

        return config;
    }

    //----------------------------------------------------------------------------
    // Zebra
    //----------------------------------------------------------------------------

    /**
     * @brief Implicitely called when installing the ndoe
     * @param nodes The node(s) to enable OSPFv2 (quagga ospfd).
     */

    static void EnableZebra ( NodeContainer nodes );

    //----------------------------------------------------------------------------
    // OSPF
    //----------------------------------------------------------------------------

    /**
     * @brief Enable the ospfd daemon to the nodes.
     * @param nodes The node(s) to enable OSPFv2 (quagga ospfd).
     * @param network The network to enable ospf protocol.
     */

    static void EnableOspf ( NodeContainer nodes, const Ipv4Prefix & prefix );

    /**
     * @brief Enable the ospfd daemon to the nodes.
     * @param nodes The node(s) to enable OSPFv2 (quagga ospfd).
     * @param network The network to enable ospf protocol.
     */

    void EnableOspf ( NodeContainer nodes, const char * network ) const;

    //----------------------------------------------------------------------------
    // BGP
    //----------------------------------------------------------------------------

    /**
     * @brief Enable the bgpd daemon to the nodes.
     * @param nodes The node(s) to enable BGP (quagga bgpd).
     */

    static void EnableBgp ( NodeContainer nodes );

    /**
     * @brief Set the Router ID of the nodes.
     * @param node The node to set Router ID.
     * @param routerId The Router ID.
     */

    static void SetRouterId ( Ptr<Node> node, const std::string & routerId );

    /**
     * @brief Set the Autonomous System number (AS number) of the nodes.
     * @param nodes The nodes to set ASN.
     * @param asn The ASN.
     */

    static void SetAsn ( NodeContainer nodes, uint32_t asn );

    /**
     * @brief Get the Autonomous System number (AS number) of the nodes.
     * This function is OBSOLETE, please use instead:
     *    Ptr<BgpConfig> bgpConf = QuaggaHelper::GetConfig<BgpConfig>(node);
     *    uint32_t asn = bgpConf->GetAsn();
     * @param node The node to obtain ASN
     * @returns The corresponding ASN (>0). Returns 0 in case of failure.
     */

    static uint32_t GetAsn ( Ptr<Node> node );

    /**
     * @brief Configure the neighbor of BGP peer to exchange the route
     * information to the bgp daemon (via neighbor remote-as command).
     * @param neighborIp An IP of the neighboring BGP router.
     * @param asn The AS number of the remote neighbor.
     *    Pass this->GetAsn(node) if this is an iBGP session.
     * @param description An optional description of this neighbor.
     */

    static void BgpAddNeighbor (
        Ptr<Node> node,
        const std::string & neighborIp,
        uint32_t asn,
        const std::string & description = ""
    );

    /**
     * @brief Configure the neighbor as peer link to filter-out the update route
     *   only with node's origin route (via neighbor A.B.C.D route-map MAP out command).
     * @param neighborIp An IP of the neighboring BGP router.
     */

    static void BgpAddPeerLink ( Ptr<Node> node, const std::string & neighborIp );

    /**
     * @brief Add the network
     * @param network A std::string containing a prefix that will be
     *    announced using BGP.
     */

    static void BgpAddNetwork ( Ptr<Node> node, const std::string & network );

    //----------------------------------------------------------------------------
    // OSPF6
    //----------------------------------------------------------------------------

    /**
     * @brief Enable the ospf6d daemon (OSPFv3) to the nodes.
     * @param nodes The node(s) to enable OSPFv3 (quagga ospf6d).
     * @param ifname The interface to enable OSPFv3.
     */

    static void EnableOspf6 ( NodeContainer nodes, const char * ifname );

    //----------------------------------------------------------------------------
    // RIP
    //----------------------------------------------------------------------------

    /**
     * @brief Enable the ripd daemon (RIP v1/v2, RFC2453) to the nodes.
     * @param nodes The node(s) to enable RIP (quagga ripd).
     * @param ifname The interface to enable RIP.
     */

    static void EnableRip ( NodeContainer nodes, const char *ifname );

    //----------------------------------------------------------------------------
    // RIPnG
    //----------------------------------------------------------------------------

    /**
     * @brief Enable the ripngd daemon (RIPng, RFC2080) to the nodes.
     * @param nodes The node(s) to enable RIPng (quagga ripngd).
     * @param ifname The interface to enable RIPng.
     */

    static void EnableRipng ( NodeContainer nodes, const char * ifname );

    //=====================================================================================
    // OBSOLETE METHODS, KEPT FOR BACKWARD COMPATIBILITY
    //=====================================================================================

    /**
     * @brief Enable Router Advertisement configuration to the zebra
     * daemon (via no ipv6 nd suppress-ra xxx).
     * @param node    The node to configure the options.
     * @param ifname  The string of the interface name to enable this option.
     * @param prefix  The string of the network prefix to advertise.
     */

    static void EnableRadvd ( Ptr<Node> node, const char * ifname, const char * prefix );

    /**
     * @brief Configure HomeAgent Information Option (RFC 3775) in
     * Router Advertisement to the zebra daemon (via ipv6 nd
     * home-agent-config-flag).
     * @param ifname The string of the interface name to enable this
     * option.
     */

    static void EnableHomeAgentFlag ( Ptr<Node> node, const char * ifname );

    /**
     * @brief Configure the debug option to the zebra daemon.
     * This function is OBSOLETE, please use instead:
     *    QuaggaHelper::SetDebug<ZebraConfig>(NodeContainer, true)
     * @param nodes The node(s) to configure the options.
     */

    static void EnableZebraDebug ( NodeContainer nodes );

    /**
     * @brief Configure the debug option to the ospfd daemon.
     * This function is OBSOLETE, please use instead:
     *    QuaggaHelper::SetDebug<OspfConfig>(NodeContainer, true)
     * @param nodes The node(s) to configure the options.
     */

    static void EnableOspfDebug ( NodeContainer nodes );

    /**
     * @brief Configure the debug option to the ospf6d daemon.
     * This function is OBSOLETE, please use instead:
     *    QuaggaHelper::SetDebug<Ospf6Config>(NodeContainer, true)
     * @param nodes The node(s) to configure the options.
     */

    void EnableOspf6Debug ( NodeContainer nodes );

    /**
     * @brief Configure the debug option to the ripd daemon.
     * This function is OBSOLETE, please use instead:
     *    QuaggaHelper::SetDebug<RipConfig>(NodeContainer, true)
     * @param nodes The node(s) to configure the options.
     */

    static void EnableRipDebug ( NodeContainer nodes );

    /**
     * @brief Configure the debug option to the ripngd daemon.
     * This function is OBSOLETE, please use instead:
     *    QuaggaHelper::SetDebug<RipngConfig>(NodeContainer, true)
     * @param nodes The node(s) to configure the options.
     */

    static void EnableRipngDebug ( NodeContainer nodes );

};

} // namespace ns3

#endif /* QUAGGA_HELPER_H */
