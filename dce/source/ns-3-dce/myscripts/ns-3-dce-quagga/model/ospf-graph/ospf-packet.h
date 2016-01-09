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

#ifndef OSPF_PACKET_H
#define OSPF_PACKET_H

#define IPPROTO_OSPF    89

// see (RFC 2328, A.4.1, p204)
#define OSPF_LSA_TYPE_ROUTER          1
#define OSPF_LSA_TYPE_NETWORK         2
#define OSPF_LSA_TYPE_SUMMARY_NETWORK 3
#define OSPF_LSA_TYPE_SUMMARY_ASBR    4
#define OSPF_LSA_TYPE_EXTERNAL        5

// see (RFC 2328, A.4.2, p207)
#define OSPF_LSR_TYPE_PTP          1
#define OSPF_LSR_TYPE_TRANSIT      2
#define OSPF_LSR_TYPE_STUB         3
#define OSPF_LSR_TYPE_VIRTUAL_LINK 4

#include <cstdint>                          // uint*_t
#include <map>                              // std::map
#include <ostream>                          // std::ostream
#include <vector>                           // std::vector
#include <boost/operators.hpp>              // boost::totally_ordered

#include "ns3/ipv4-address.h"               // ns3::Ipv4Address, ns3::Ipv4Mask
#include "ns3/ipv4-prefix.h"                // ns3::Ipv4Prefix

namespace ns3 {
namespace ospf {

//---------------------------------------------------------------------
// Class router_id_t
// The router ID is an IP corresponding to a unique router and which
// identifies uniquely a router in the OSPF graph.
//---------------------------------------------------------------------

typedef Ipv4Address router_id_t;

//---------------------------------------------------------------------
// Class network_id_t
// The Designated Router's IP corresponds to the IP of a router
// directly connected to a given network and which uniquely identifies
// this network in the OSPF graph.
//---------------------------------------------------------------------

typedef Ipv4Address network_id_t;

//---------------------------------------------------------------------
// Class metric_t
//---------------------------------------------------------------------

//typedef basic_distance_t<float> metric_t;
typedef uint32_t metric_t;

} // namespace ospf


//---------------------------------------------------------------------
// OspfLsa (base class)
//---------------------------------------------------------------------

/**
 * \brief OSPF LSA message
 * This class gathers the information shared by all the types of LSA.
 */

class OspfLsa {
private:
    // uint16_t     m_age;
    uint8_t       m_lsaType;            /**< The LSA type of this OSPF message. */
    // uint8_t      m_option;
    Ipv4Address   m_advertisingRouter;  /**< The router-id of the router announcing the message. */
    // uint32_t     m_sequenceNumber;
    // uint16_t     m_checksum;
    // uint16_t     m_length;


protected:
    /**
     * @brief Constructor.
     * @param lsaType The LSA type of this OSPF message.
     */

    // TODO : this constructor should be removed
    OspfLsa (uint8_t lsaType);

public:

    /**
     * @brief Constructor.
     * @param lsaType The LSA type of this OSPF message.
     * @param advertisingRouter The Ipv4Address of the advertising router.
     */

    OspfLsa (uint8_t lsaType, const Ipv4Address & advertisingRouter);

    /**
     * @brief Destructor.
     */

    virtual ~OspfLsa ();

    /**
     * @brief Retrieve the advertising router of this OSPF message.
     * @return The corresponding ns3::Ipv4Address.
     */

    const Ipv4Address & GetAdvertisingRouter() const;

    /**
     * @brief Retrieve the LSA type of this OSPF message.
     * @return The type of LSA of this OSPF message.
     */

    uint8_t GetLsaType() const;

    /**
     * @brief Print this OspfLsa to an output stream.
     * @param os The output stream.
     */

    virtual void Print(std::ostream & os) const;
};

/**
 * @brief Print an OspfLsa instance to an output stream.
 * @param os The output stream.
 * @param lsa An OspfLsa instance.
 * @return The updated output stream.
 */

std::ostream & operator << (std::ostream & os, const OspfLsa & lsa);

//---------------------------------------------------------------------
// OspfRouterLsa
//---------------------------------------------------------------------

/**
 * @class OspfRouterLsa
 * @brief OSPF Router LSA message
 * @sa https://www.ietf.org/rfc/rfc2328.txt section A.4.2
 */

class OspfRouterLsa :
    public OspfLsa
{
public:
    // TODO private members
    // A OspfRouterLsa carries a sequence of (Link ID, Link Data, Type, #TOS, Metric) triples
    std::map<ospf::network_id_t, ospf::metric_t>    networks;
    std::map<ospf::network_id_t, Ipv4Address>       ifs;

    // TODO remove this constructor
    OspfRouterLsa ();

    /**
     * @brief Constructor.
     * @param advertisingRouter Advertising router.
     */

    OspfRouterLsa (const ospf::router_id_t & advertisingRouter);

    /**
     * @brief Print this OspfRouterLsa to an output stream.
     * @param os The output stream.
     */

    void Print(std::ostream & out) const;
};

//---------------------------------------------------------------------
// OspfExternalLsa
//---------------------------------------------------------------------

/**
 * @class OspfExternalLsa
 * @brief OSPF Network LSA message
 * @sa https://www.ietf.org/rfc/rfc2328.txt section A.4.4
 */

class OspfNetworkLsa :
    public OspfLsa
{
private:
    Ipv4Address m_linkStateId; /**< The address part of the prefix. */
    Ipv4Mask    m_networkMask; /**< The mask part of the prefix. */
public:

    /**
     * @brief Constructor.
     * @param advertisingRouter IP address of the advertising router.
     * @param linkStateId The address part of the IPv4 prefix carried by
     *    this OspfNetworkLsa.
     * @param networkMask The mask part of the IPv4 prefix carried by
     *   this  OspfNetworkLsa.
     */

    OspfNetworkLsa(
        const ospf::router_id_t  & advertisingRouter,
        const ospf::network_id_t & linkStateId,
        const Ipv4Mask           & networkMask
    );

    /**
     * @brief Retrieve the link-state-id address transported by this
     *   OspfNetworkLsa.
     * @return The corresponding network Ipv4Address.
     */

    const Ipv4Address & GetLinkStateId() const;

    /**
     * @brief Retrieve the network mask transported by this
     *   OspfNetworkLsa.
     * @return The corresponding Ipv4Mask.
     */

    const Ipv4Mask & GetNetworkMask() const;
};

//---------------------------------------------------------------------
// OspfExternalLsa
//---------------------------------------------------------------------

/**
 * @class OspfExternalLsa
 * @brief OSPF External LSA message
 * @sa https://www.ietf.org/rfc/rfc2328.txt section A.4.5
 */

class OspfExternalLsa :
    public OspfLsa
{
private:
    ospf::network_id_t  m_linkStateId;  /**< The address part of the external network. */
    Ipv4Mask            m_networkMask;  /**< The mask part of the external network. */
    ospf::metric_t      m_metric;       /**< The OSPF metric from the connected router toward this network. */
public:

    /**
     * @brief Constructor.
     * @param advertisingRouter The router-id of the router on which the
     *   external network is connected to.
     * @param linkStateId The network identifier corresponding to the external
     *   network.
     * @param networkMask The network mask of the external network.
     *   (linkStateId, mask) forms the IPv4 prefix of the external network.
     * @param metric The OSPF metric assigned to the corresponding OSPF arc.
     */

    OspfExternalLsa (
        const ospf::router_id_t  & advertisingRouter,
        const ospf::network_id_t & linkStateId,
        const Ipv4Mask           & networkMask,
        const ospf::metric_t     & metric
    );

    /**
     * @brief Retrieve the OSPF metric to this  OspfExternalLsa instance.
     * @return The corresponding OSPF metric.
     */

    const ospf::metric_t & GetMetric() const;

    /**
     * @brief Set the OSPF metric to this  OspfExternalLsa instance.
     * @param metric The OSPF metric.
     */

    void SetMetric(const ospf::metric_t &  metric);

    /**
     * @brief Retrieve the link-state id of this OspfExternalLsa instance.
     * @returns The corresponding Ipv4Address.
     */

    const Ipv4Address & GetLinkStateId() const;

    /**
     * @brief Set the link-state id of this OspfExternalLsa instance.
     * @param linkStateId The new Ipv4Address.
     */

    void SetLinkStateId(const Ipv4Address & linkStateId);

    /**
     * @brief Print this OspfExternalLsa to an output stream.
     * @param os The output stream.
     */

    const Ipv4Mask & GetNetworkMask() const;
    void Print(std::ostream & out) const;
};


//---------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------

/**
 * Tests whether a packet is an OSPF packet or not.
 * \param buffer The bytes of the packet (basically the contents of the buffer
 *    nested in a ns3::Packet, see PacketGetBuffer.
 * \returns true iif the Packet is an OSPF packet.
 */

bool IsOspfPacket(const uint8_t * buffer);

/**
 * \brief Extract LSAs from an OSPF packet of type LS-Update
 * \param buffer The bytes transported in an IPv4/OSPF packet (starting
 *   from the beginning of the IPv4 header).
 * \param lsas An empty vector which will contains the LSA carried
 *    in "buffer".
 */

void ExtractOspfLsa(
    const uint8_t * buffer,
    std::vector<OspfLsa *> & lsas
);

} // namespace ns3

#endif // OSPF_PACKET_H
