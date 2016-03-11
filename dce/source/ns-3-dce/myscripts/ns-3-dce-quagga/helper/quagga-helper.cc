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
 *
 * Author:
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 */

#include "quagga-helper.h"

#include <sstream>                      // std::ostringstream

#include "ns3/ipv4.h"                   // ns3::Ipv4
#include "ns3/ipv4-address.h"           // ns3::Ipv4Address
#include "ns3/ipv4-prefix.h"            // ns3::Ipv4Prefix
#include "ns3/log.h"                    // NS_LOG_*
#include "ns3/names.h"                  // ns3::Names
#include "ns3/quagga-fs.h"              // QuaggaFs::*

#define DUMMY_ASN 0

NS_LOG_COMPONENT_DEFINE ("QuaggaHelper");


namespace ns3 {

static Ipv4Address GetDefaultRouterId (Ptr<Node> node) {
    NS_LOG_FUNCTION (node);

    const Ipv4Address localhost ("127.0.0.1");
    Ipv4Address ret ("0.0.0.0");

    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    uint32_t numInterfaces = ipv4->GetNInterfaces();

    // In practice interface 0 corresponds to localhost and then we will
    // pick the IP address of the interface 1.
    for (uint32_t j = 0; j < numInterfaces; ++j) {
        const Ipv4InterfaceAddress & ipv4InterfaceAddress = ipv4->GetAddress (j, 0);
        Ipv4Address ipv4Address = ipv4InterfaceAddress.GetLocal();

        if (ipv4Address != localhost) {
            ret = ipv4Address;
            break;
        }
    }

    return ret;
}

std::string GetNodeName (Ptr<Node> node) {
    std::string nodeName = Names::FindName (node);

    if (nodeName == "") {
        std::ostringstream oss;
        oss << node->GetId();
        nodeName = oss.str();
    }

    return nodeName;
}

//---------------------------------------------------------------------------
// QuaggaHelper
//---------------------------------------------------------------------------

QuaggaHelper::QuaggaHelper () {}

//---------------------------------------------------------------------------
// Zebra
//---------------------------------------------------------------------------

void QuaggaHelper::EnableZebra (NodeContainer nodes) {
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it) {
        Ptr<Node> node = *it;

        // Create configuration if it does not yet exist.
        Ptr<ZebraConfig> zebraConf = QuaggaHelper::GetConfig<ZebraConfig> (node);

        // Set default parameters
        zebraConf->SetStartTime (Seconds (1.0 + 0.01 * node->GetId ()));
    }
}

// FOR BACKWARD COMPATIBILITY
void QuaggaHelper::EnableRadvd (Ptr<Node> node, const char * ifname, const char * prefix) {
    QuaggaHelper::GetConfig<ZebraConfig> (node)->AddRadvdIf (std::string (ifname), std::string (prefix));
}

// FOR BACKWARD COMPATIBILITY
void QuaggaHelper::EnableHomeAgentFlag (Ptr<Node> node, const char * ifname) {
    QuaggaHelper::GetConfig<ZebraConfig> (node)->EnableHomeAgentFlag (std::string (ifname));
}

//---------------------------------------------------------------------------
// BGP
//---------------------------------------------------------------------------

void QuaggaHelper::EnableBgp (NodeContainer nodes) {
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it) {
        Ptr<Node> node = *it;

        // Create configuration if it does not yet exist.
        Ptr<BgpConfig> bgpConf = QuaggaHelper::GetConfig<BgpConfig> (node);

        // Set default parameters
        //bgpConf->SetStartTime (Seconds (5.0 + 0.3 * node->GetId ()));
        bgpConf->SetStartTime (Seconds (0.3 * node->GetId () + 30));
        bgpConf->SetAsn (node->GetId());
        bgpConf->SetRouterId (GetDefaultRouterId (node));
    }
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::SetRouterId (Ptr<Node> node, const std::string & routerId) {
    //this->SetRouterId ( node, Ipv4Address ( routerId.c_str() ) );
    QuaggaHelper::GetConfig<BgpConfig> (node)->SetRouterId (Ipv4Address (routerId.c_str()));
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::SetAsn (NodeContainer nodes, uint32_t asn) {
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it) {
        Ptr<Node>  node = *it;
        QuaggaHelper::GetConfig<BgpConfig> (node)->SetAsn (asn);
    }
}

// BACKWARD COMPATIBILITY
uint32_t QuaggaHelper::GetAsn (Ptr<Node> node) {
    Ptr<BgpConfig> bgpConf = node->GetObject<BgpConfig> ();
    return bgpConf ? bgpConf->GetAsn () : DUMMY_ASN;
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::BgpAddPeerLink (Ptr<Node> node, const std::string & neighbor) {
    std::cerr << "BgpAddPeerLink is not anymore supported (routemap not yet implemented): neighbor" << neighbor << std::endl;
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::BgpAddNetwork (Ptr<Node> node, const std::string & network) {
    QuaggaHelper::GetConfig<BgpConfig> (node)->AddNetwork (network);
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::BgpAddNeighbor (
    Ptr<Node> node,
    const std::string & neighborIp,
    uint32_t asn,
    const std::string & description
) {
    QuaggaHelper::GetConfig<BgpConfig> (node)->AddNeighbor (neighborIp, asn, description);
}

//---------------------------------------------------------------------------
// OSPF
//---------------------------------------------------------------------------

void QuaggaHelper::QuaggaHelper::EnableOspf (NodeContainer nodes, const Ipv4Prefix & prefix) {
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it) {
        Ptr<Node> node = *it;

        // Create configuration if it does not yet exist.
        Ptr<OspfConfig> ospfConf = QuaggaHelper::GetConfig<OspfConfig> (node);

        // Set default parameters
        ospfConf->SetStartTime (Seconds (2.0 + 0.1 * node->GetId ()));
        ospfConf->AddNetwork (prefix);
        ospfConf->SetRouterId (GetDefaultRouterId (node));
    }
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::EnableOspf (NodeContainer nodes, const char * prefix) const {
    NS_LOG_WARN ("OBSOLETE please use QuaggaHelper::EnableOspf(nodes, Ipv4Prefix(prefix)) instead");
    QuaggaHelper::EnableOspf (nodes, Ipv4Prefix (prefix));
}

//---------------------------------------------------------------------------
// OSPF6
//---------------------------------------------------------------------------

void QuaggaHelper::EnableOspf6 (NodeContainer nodes, const char * ifname) {
    for (uint32_t i = 0; i < nodes.GetN (); i++) {
        Ptr<Node> node = nodes.Get (i);

        // Create configuration if it does not yet exist.
        Ptr<Ospf6Config> ospf6Conf = QuaggaHelper::GetConfig<Ospf6Config> (node);

        // Set default parameters
        ospf6Conf->SetStartTime (Seconds (5.0 + 0.1 * node->GetId ()));
        ospf6Conf->AddInterface (std::string (ifname));
        std::ostringstream oss;
        oss << "255.1.1." << i;
        ospf6Conf->SetRouterId (Ipv4Address (oss.str().c_str()));
    }
}

//---------------------------------------------------------------------------
// RIP
//---------------------------------------------------------------------------

void QuaggaHelper::EnableRip (NodeContainer nodes, const char * ifname) {
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End (); it++) {
        Ptr<Node> node = *it;

        // Create configuration if it does not yet exist.
        Ptr<RipConfig> ripConfig = QuaggaHelper::GetConfig<RipConfig> (node);

        // Set default parameters
        ripConfig->SetStartTime (Seconds (5.0 + 0.1 * node->GetId ()));
        ripConfig->AddInterface (std::string (ifname));
    }
}

//---------------------------------------------------------------------------
// QuaggaHelper / RIPNG
//---------------------------------------------------------------------------

void QuaggaHelper::EnableRipng (NodeContainer nodes, const char * ifname) {
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End (); it++) {
        Ptr<Node> node = *it;

        // Create configuration if it does not yet exist.
        Ptr<RipngConfig> ripngConfig = QuaggaHelper::GetConfig<RipngConfig> (node);

        // Initialize the configuration
        ripngConfig->SetStartTime (Seconds (5.0 + 0.1 * node->GetId ()));
        ripngConfig->AddInterface (std::string (ifname));
    }
}

//---------------------------------------------------------------------------
// Install*
//---------------------------------------------------------------------------

ApplicationContainer QuaggaHelper::Install (Ptr<Node> node) {
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer QuaggaHelper::Install (const std::string & nodeName) {
    Ptr<Node> node = Names::Find<Node> (nodeName);
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer QuaggaHelper::Install (NodeContainer c) {
    ApplicationContainer apps;

    for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
        apps.Add (InstallPriv (*i));
    }

    return apps;
}

ApplicationContainer QuaggaHelper::InstallPriv (Ptr<Node> node) {
    DceApplicationHelper process;
    ApplicationContainer apps;

    // Set stack size for each daemon
    process.SetStackSize (1 << 16);

    // Zebra is mandatory and is thus always enabled. This instruction is needed
    // to avoid users to invoke explictely EnableZebra.
    this->EnableZebra (node);

    // Install each (needed) daemon
    QuaggaHelper::InstallDaemon<ZebraConfig> (node, apps);
    QuaggaHelper::InstallDaemon<OspfConfig> (node, apps);
    QuaggaHelper::InstallDaemon<BgpConfig> (node, apps);
    QuaggaHelper::InstallDaemon<Ospf6Config> (node, apps);
    QuaggaHelper::InstallDaemon<RipConfig> (node, apps);
    QuaggaHelper::InstallDaemon<RipngConfig> (node, apps);

    // Return the corresponding DceApplication
    return apps;
}

//=====================================================================================
// OBSOLETE METHODS, KEPT FOR BACKWARD COMPATIBILITY
//=====================================================================================


// FOR BACKWARD COMPATIBILITY
void QuaggaHelper::EnableZebraDebug (NodeContainer nodes) {
    NS_LOG_WARN ("OBSOLETE please use QuaggaHelper::SetDebug<ZebraConfig>(nodes, true) instead");
    QuaggaHelper::SetDebug<OspfConfig> (nodes, true);
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::EnableOspfDebug (NodeContainer nodes) {
    NS_LOG_WARN ("OBSOLETE please use QuaggaHelper::SetDebug<OspfConfig>(nodes, true) instead");
    QuaggaHelper::SetDebug<OspfConfig> (nodes, true);
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::EnableOspf6Debug (NodeContainer nodes) {
    NS_LOG_WARN ("OBSOLETE please use QuaggaHelper::SetDebug<Ospf6Config>(nodes, true) instead");
    QuaggaHelper::SetDebug<Ospf6Config> (nodes, true);
}


// BACKWARD COMPATIBILITY
void QuaggaHelper::EnableRipDebug (NodeContainer nodes) {
    NS_LOG_WARN ("OBSOLETE please use QuaggaHelper::SetDebug<RipConfig>(nodes, true) instead");
    QuaggaHelper::SetDebug<RipConfig> (nodes, true);
}

// BACKWARD COMPATIBILITY
void QuaggaHelper::EnableRipngDebug (NodeContainer nodes) {
    NS_LOG_WARN ("OBSOLETE please use QuaggaHelper::SetDebug<RipConfig>(nodes, true) instead");
    QuaggaHelper::SetDebug<RipngConfig> (nodes, true);
}


} // namespace ns3


