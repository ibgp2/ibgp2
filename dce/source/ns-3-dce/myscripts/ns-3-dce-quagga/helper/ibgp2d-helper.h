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

#ifndef IBGP_CONTROLLER_HELPER_H
#define IBGP_CONTROLLER_HELPER_H

#include <cstdint>                      // uint*_t
#include <ostream>                      // std::ostream
#include <map>                          // std::map
#include <string>                       // std::string

#include "ns3/application-container.h"  // ns3::ApplicationContainer
#include "ns3/node.h"                   // ns3::Node
#include "ns3/node-container.h"         // ns3::NodeContainer
#include "ns3/object-factory.h"         // ns3::ObjectFactory
#include "ns3/ptr.h"                    // ns3::Ptr

namespace ns3 {

/**
 * @ingroup applications
 * @brief Create an iBGP controller.
 */

class Ibgp2dHelper
{
private:
    typedef std::map<Ptr<const Node>, uint32_t> MapNodeApplication;

    ObjectFactory      m_factory;             /**< Object factory. */
    uint32_t           m_asn;                 /**< ASN of the AS of the router. */
    MapNodeApplication mapNodeApplication;    /**< Stores for each Node embedding an Ibpg2d instance the corresponding application ID. */

    /**
     * Install a ns3::IBgpController on the node configured with all the
     * attributes set with SetAttribute.
     * @param node The ns3::Node on which an TcpClient will be installed.
     * @returns Ptr to the Application installed.
     */

    Ptr<Application> InstallPriv (Ptr<Node> node);

    /**
     * @brief (Internal usage): write in an output stream the graphviz representation
     *   of the IGP graph maintained by a given Node.
     * @param out The output stream.
     * @param node The Node.
     * @param indexApplication The ApplicationId of Ibgp2d on this Node.
     * @param drawNetworks Pass true if the graphviz must draw the network
     *   vertices. If you pass false, only the router vertices will be
     *   represented.
     * @return std::ostream&
     */

    std::ostream & WriteIgpGraphvizImpl(
        std::ostream & out,
        const Node   & node,
        uint32_t       indexApplication,
        bool           drawNetworks = false
    ) const;

public:

    /**
     * @brief Constructor.
     * @param asn The ASN number of the router on which we will install the next
     *   Ibgp2d instance(s).
     * @sa SetAsn().
     */

    Ibgp2dHelper (uint32_t asn);

    /**
     * @brief Create an Ibgp2d application on the specified node.
     * @param node The Ptr<Node> on which to create the IBgpController.
     * @returns An ApplicationContainer that holds a Ptr<Application> to the
     *    created Application.
     */

    ApplicationContainer Install (Ptr<Node> node);

    /**
     * @brief Create an Ibgp2d application on each nodes passed in parameter.
     * @param c The nodes.
     * @returns The created Ibgp2d instance, one instance per input node.
     */

    ApplicationContainer Install (NodeContainer & c);

    /**
     * @brief Record an attribute to be set in each Application after
     *   it is created.
     * @param name The name of the attribute to set.
     * @param value The value of the attribute to set.
     */

    void SetAttribute (
        const std::string & name,
        const AttributeValue & value
    );

    /**
     * @brief Dump the IGP graph corresponding all the managed Nodes.
     * @param out The output stream.
     * @param drawNetwork Pass true to represent the transit network(s)
     *    configured between each Nodes.
     * @returns The output stream.
     */

    std::ostream & DumpIgpGraphviz (
        std::ostream & out,
        bool drawNetworks = false
    ) const;

    /**
     * @brief Dump the IGP graph of a given Node.
     * @param out The output stream.
     * @param node A Node instance.
     * @param drawNetwork Pass true to represent the transit network(s)
     *    configured between each Nodes.
     * @returns The output stream.
     */

    std::ostream & DumpIgpGraphviz (
        std::ostream & out,
        const Node & node,
        bool drawNetworks = false
    ) const;

};

} // namespace ns3

#endif // IBGP_CONTROLLER_HELPER_H
