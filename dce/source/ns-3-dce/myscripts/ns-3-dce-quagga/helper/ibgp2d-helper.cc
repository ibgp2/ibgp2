/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * * Copyright (c) 2015 Marc-Olivier Buob, Alexandre Morignot
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

#include "ns3/ibgp2d-helper.h"

#include "ns3/application-container.h"  // ns3::ApplicationContainer
#include "ns3/ibgp2d.h"                 // ns3::Ibgp2d
#include "ns3/bgp-config.h"             // ns3::BgpConfig
#include "ns3/log.h"                    // NS_LOG_*
#include "ns3/object.h"                 // ns3::GetObject
#include "ns3/ospf-config.h"            // ns3::OspfConfig
#include "ns3/ospf-graph-helper.h"      // ns3::OspfGraphHelper

#include "ns3/ipv4.h"

NS_LOG_COMPONENT_DEFINE ( "Ibgp2dHelper" );

namespace ns3 {

Ibgp2dHelper::Ibgp2dHelper (uint32_t asn0):
    m_asn(asn0)
{
    NS_LOG_FUNCTION ( this );
    m_factory.SetTypeId (Ibgp2d::GetTypeId ());
}

void Ibgp2dHelper::SetAttribute (
    const std::string & name,
    const AttributeValue &value)
{
    NS_LOG_FUNCTION ( this << name );
    m_factory.Set (name, value);
}

ApplicationContainer Ibgp2dHelper::Install (Ptr<Node> node) {
    NS_LOG_FUNCTION ( this << node );
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer Ibgp2dHelper::Install (NodeContainer & c) {
    NS_LOG_FUNCTION ( this );
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
        apps.Add (InstallPriv (*i));
    }
    return apps;
}

Ptr<Application> Ibgp2dHelper::InstallPriv (Ptr<Node> node) {
    NS_LOG_FUNCTION ( this << node );
    Ptr<Ibgp2d> ibgp2d = m_factory.Create<Ibgp2d> ();

    // ASN
    ibgp2d->SetAsn(this->m_asn);

    // Router ID
    Ptr<OspfConfig> ospfConfig = node->GetObject<OspfConfig> ();
    Ipv4Address routerId = ospfConfig->GetRouterId();

    if (routerId == Ipv4Address ( OSPF_DUMMY_ROUTER_ID )) {
        // Setting unspecified OSPF router-id.
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        uint32_t numInterfaces = ipv4->GetNInterfaces();
        routerId = ipv4->GetAddress ( 1, 0 ).GetLocal();
        NS_ASSERT ( routerId != Ipv4Address ( OSPF_DUMMY_ROUTER_ID ) );
        NS_ASSERT ( routerId != Ipv4Address ( "127.0.0.1" ) );
        ospfConfig->SetRouterId(routerId);
        NS_LOG_INFO ( "[IBGP2] " << node << "'s router ID set to " << routerId );
    } else {
        NS_LOG_INFO ( "[IBGP2] " << node << "'s router ID already set to " << routerId );
    }
    ibgp2d->SetRouterId ( routerId );

    // Start time : iBGP2 must start just after ospfd to rebuild the IGP graph,
    // because once OSPF has converged, the OSPF LSA do not contains enough
    // information to rebuild the IGP graph. Since bgpd is not necessarily
    // (yet) running, ibgp2 may have to wait before altering iBGP filters.

    Time ospfdStartTime  = ospfConfig->GetStartTime();
    Time ibgp2StartTime = ospfdStartTime + Seconds(0.5);

    /*
    std::cout << "[IBGP2] " << routerId << ": ospfd starts at "
        << ospfdStartTime.GetSeconds() << ", so ibgp2d will start at "
        << ibgp2StartTime.GetSeconds() << std::endl;
    */

    ibgp2d->SetStartTime(ibgp2StartTime);

    // Map application
    this->mapNodeApplication[node] = node->AddApplication (ibgp2d);
    return ibgp2d;
}

std::ostream & Ibgp2dHelper::WriteIgpGraphvizImpl(
    std::ostream & out,
    const Node   & node,
    uint32_t       indexApplication,
    bool           drawNetworks
) const {
    NS_LOG_FUNCTION ( this );
    Ptr<Ibgp2d> ibgp2d = node.GetApplication(indexApplication)->GetObject<Ibgp2d>();
    const OspfGraphHelper * ospfGraphHelper = ibgp2d->GetOspfGraphHelper();
    if (!ospfGraphHelper) return out;
    ospfGraphHelper->WriteGraphviz (out, drawNetworks);

    return out;
}

std::ostream & Ibgp2dHelper::DumpIgpGraphviz(
    std::ostream & out,
    const Node   & node,
    bool           drawNetworks
) const {
    NS_LOG_FUNCTION ( this );
    MapNodeApplication::const_iterator fit(this->mapNodeApplication.find(Ptr<const Node>(&node)));
    uint32_t indexApplication = fit->second;
    return this->WriteIgpGraphvizImpl(out, node, indexApplication, drawNetworks);
}

std::ostream & Ibgp2dHelper::DumpIgpGraphviz(
    std::ostream & out,
    bool drawNetworks
) const {
    NS_LOG_FUNCTION ( this );
    for (const auto & p : this->mapNodeApplication) {
        const Node & node = *(p.first);
        uint32_t indexApplication = p.second;
        this->WriteIgpGraphvizImpl(out, node, indexApplication, drawNetworks);
    }
    return out;
}

} //namespace ns3
