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
 *   Marc-Olivier Buob <marcolivier.buob@orange.fr>
 *   Alexandre Morignot <alexandre.morignot@orange.fr>
 */

#include "ospf-packet.h"

#include "ns3/log.h"        // ns3::NS_LOG_*

#include <arpa/inet.h>      // ntohs, ntohl

#define GET8(buffer, offset)  (((const uint8_t *) buffer)[offset])
#define GET16(buffer, offset) ntohs(*((const uint16_t *) (((const uint8_t *) buffer) + (offset))))
#define GET32(buffer, offset) ntohl(*((const uint32_t *) (((const uint8_t *) buffer) + (offset))))

NS_LOG_COMPONENT_DEFINE ( "OspfPacket" );

namespace ns3 {

//---------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------

/**
 * Tests whether a packet is an OSPF packet or not.
 * \param buffer The bytes of the packet (basically the contents of the buffer
 *    nested in a ns3::Packet, see PacketGetBuffer.
 * \returns true iif the Packet is an OSPF packet.
 */

bool IsOspfPacket(const uint8_t * buffer) {
    bool ret = false;

    // IP layer

    const uint32_t ipOffset  = 2;                 // NS3 add 2 bytes 0x0021 (for IPv4) to mimic PPP (see IANA PPP header)
    uint8_t ipVersion, ipLength, ipProtocol;

    ipVersion  = buffer[ipOffset] >> 4;
    switch (ipVersion) {
    case 4: // IPv4
        ipLength   = GET8(buffer, ipOffset) & 0x0f; // the size of the IPv4 header is here (and must be multiplied by 4)
        ipProtocol = GET8(buffer, ipOffset + 9);    // 9 = offsetof(struct iphdr, protocol), #include <stddef.h>, #include <netinet/ip.h>
        ret = (ipProtocol == IPPROTO_OSPF);
        break;
    case 6: // IPv6
        ipLength   = 40;                            // size of any IPv6 header
        ipProtocol = GET8(buffer, ipOffset + 6);    // 6 = offsetof(struct ip6_hdr, ip6_nxt), #include <stddef.h>, #include <netinet/ip6.h>
        ret = (ipProtocol == IPPROTO_OSPF);
        break;
    default:
        std::cerr << "Invalid IP version number: " << ipVersion << std::endl;
        break;
    }

    return ret;
}

void ExtractOspfLsa (
    const uint8_t * buffer,
    std::vector<OspfLsa *> & lsas
) {

    // IP layer

    const uint32_t ipOffset  = 2;                 // NS3 add 2 bytes 0x0021 (for IPv4) to mimic PPP (see IANA PPP header)
    uint8_t ipVersion, ipLength, ipProtocol;

    ipVersion  = buffer[ipOffset] >> 4;
    switch (ipVersion) {
    case 4: // IPv4
        ipLength   = GET8(buffer, ipOffset) & 0x0f; // the size of the IPv4 header is here (and must be multiplied by 4)
        ipProtocol = GET8(buffer, ipOffset + 9);    // 9 = offsetof(struct iphdr, protocol), #include <stddef.h>, #include <netinet/ip.h>
        break;
    case 6: // IPv6
        ipLength   = 40;                            // size of any IPv6 header
        ipProtocol = GET8(buffer, ipOffset + 6);    // 6 = offsetof(struct ip6_hdr, ip6_nxt), #include <stddef.h>, #include <netinet/ip6.h>
        break;
    default:
        std::cerr << "Invalid IP version number: " << ipVersion << std::endl;
        return;
    }

    // Check whether it is an IP/OSPF packet

    if (ipProtocol != IPPROTO_OSPF) {
        std::cerr << "Ignoring IP packet (protocol " << int(ipProtocol)
                  << ") (expected OSPF(" << IPPROTO_OSPF << "))" << std::endl;
        return;
    }

    // OSPF layer (IPv4 specific)

    const uint32_t ospfOffset  = ipOffset + (ipLength << 2);
    uint8_t ospfType = GET8(buffer, ospfOffset + 1);
    uint32_t routerId = GET32(buffer, ospfOffset + 4);

    // We only consider OSPF packet of type 4 (LS Update)
    // This is only kind of OSPF packet relevant in for iBGPv2.
    if (ospfType == 4) {
        uint32_t numLsas = GET32(buffer, ospfOffset + 24);

        // For each embedded LSA
        // &buffer[lsaOffset] points to the LSA's age.
        uint32_t lsaOffset = ospfOffset + 28; // the first LSA starts just after lsaSize
        uint16_t lsaSize;
        for (uint32_t iLsas = 0; iLsas < numLsas; ++iLsas , lsaOffset += lsaSize) {

            // The meaning of the 20 first bytes do not depends on the LSA type
            uint8_t  lsaType           = GET8( buffer, lsaOffset + 3);
            uint32_t linkStateId       = GET32(buffer, lsaOffset + 4);
            uint32_t advertisingRouter = GET32(buffer, lsaOffset + 8);
            lsaSize                    = GET16(buffer, lsaOffset + 18);

            // The last bytes depends on the LSA type
            switch (lsaType) {
            case OSPF_LSA_TYPE_ROUTER:
            {
                // Populate this OspfRouterLsa
                OspfRouterLsa * lsr = new OspfRouterLsa(Ipv4Address(advertisingRouter));

                // A router LSA embeds a list of triple (network ID, data IP, OSPF metric)
                // For each network (link):
                // - get the corresponding data IP
                // - get the corresponding OSPF metric

                uint16_t numLinks = GET16(buffer, lsaOffset + 22);
                uint32_t linkOffset = lsaOffset + 24;
                for (uint16_t iLinks = 0; iLinks < numLinks; ++iLinks, linkOffset += 12) {
                    uint32_t linkId   = GET32(buffer, linkOffset);     // IP of the DR
                    uint32_t linkData = GET32(buffer, linkOffset + 4);
                    uint8_t  linkType = GET8( buffer, linkOffset + 8);
                    uint8_t  numTos   = GET8( buffer, linkOffset + 9);
                    uint16_t metric   = GET16(buffer, linkOffset + 10);

                    // Complete the corresponding OspfRouterLsa
                    // We only consider transit network (type: 2)
                    // see (RFC 2328, A.4.2, p207)
                    //
                    // Note that some network may be stub and once BGPd
                    // started, become transit. In our case, transit
                    // networks are sufficient.

                    if (linkType == OSPF_LSR_TYPE_TRANSIT) {
                        Ipv4Address nid(linkId);
                        lsr->networks[nid] = metric; // The Ipv4Prefix of the corresponding link is learnt thanks to LSA Network
                        lsr->ifs[nid] = Ipv4Address(linkData);
                    }

                    // We skip the TOS metrics
                    linkOffset += 3 * numTos;
                }
                lsas.push_back(lsr);
            }
            break;
            case OSPF_LSA_TYPE_EXTERNAL:
            {
                uint32_t networkMask = GET32(buffer, lsaOffset + 20);
                uint8_t  type        = (GET8(buffer, lsaOffset + 24) & 0x80) ? 2 : 1; // first bit of the byte
                uint32_t metric      = GET32(buffer, lsaOffset + 24) & 0x7fff; // 3 bytes
                uint32_t fwdAddress  = GET32(buffer, lsaOffset + 28);

                ospf::network_id_t nid(linkStateId);
                OspfExternalLsa * lse = new OspfExternalLsa(Ipv4Address(advertisingRouter), nid, networkMask, metric);
                lsas.push_back(lse);
            }
            break;
            case OSPF_LSA_TYPE_NETWORK:
            {
                uint32_t networkMask = GET32(buffer, lsaOffset + 20);

                // (NOT NEEDED IN OUR CASE)
                // We must deduce according to lsaSize how many
                // attached routers are listed in this Network LSA
                // lsaSize is the sum of:
                // - 20 bytes of LSA headers
                // - 4 bytes of Network Mask
                // - 4*n bytes for the n attached routers.
                // The Designated Router (DR) address is the LinkStateID
                // (see RFC2328, sec A.4.3)

                // uint16_t numAttachedRouters = (lsaSize - 24) >> 2;
                //
                // uint32_t attachedRouterOffset = lsaOffset + 24;
                // for (uint16_t i = 0; i < numAttachedRouters; ++i, attachedRouterOffset += 4) {
                //    uint32_t attachedRouter = GET32(buffer, attachedRouterOffset);
                // }

                OspfNetworkLsa * lsn = new OspfNetworkLsa(
                    Ipv4Address(advertisingRouter),
                    Ipv4Address(linkStateId),
                    Ipv4Mask(networkMask)
                );
                lsas.push_back(lsn);
            }
            break;
            } // End switch LSA type
        } // End for each LSA
    } // End if LS-Update
}

//---------------------------------------------------------------------
// OspfLsa
//---------------------------------------------------------------------

OspfLsa::OspfLsa(uint8_t lsaType, const ns3::ospf::router_id_t& advertisingRouter) :
    m_lsaType (lsaType),
    m_advertisingRouter(advertisingRouter)
{}

OspfLsa::~OspfLsa() {}

const ns3::Ipv4Address& OspfLsa::GetAdvertisingRouter() const {
    return this->m_advertisingRouter;
}

uint8_t OspfLsa::GetLsaType() const {
    return this->m_lsaType;
}

void OspfLsa::Print(std::ostream& os) const {
    os << "LSA: rid = " << this->m_advertisingRouter << " type = " << this->m_lsaType << " (1: Router, 5: External)";
}

OspfLsa::OspfLsa(uint8_t lsaType) :
    m_lsaType(lsaType)
{}

std::ostream& operator<<(std::ostream& os, const ns3::OspfLsa& lsa) {
    lsa.Print(os);
    return os;
}

//---------------------------------------------------------------------
// OspfRouterLsa
//---------------------------------------------------------------------

OspfRouterLsa::OspfRouterLsa() :
    OspfLsa(OSPF_LSA_TYPE_ROUTER)
{}

OspfRouterLsa::OspfRouterLsa(const ospf::router_id_t& advertisingRouter) :
    OspfLsa(OSPF_LSA_TYPE_ROUTER, advertisingRouter)
{}

void OspfRouterLsa::Print(std::ostream& out) const {
    out << "Router LSA: rid = " << this->GetAdvertisingRouter()
        << " type = " << this->GetLsaType() << " (1: Router)" << std::endl;

    for (auto & nm : this->networks) {
        out << "\tnm: " << nm.first << " => " << nm.second << std::endl;
    }

    for (auto & ni : this->ifs) {
        out << "\tni: " << ni.first << " => " << ni.second << std::endl;
    }
}

//---------------------------------------------------------------------
// OspfNetworkLsa
//---------------------------------------------------------------------

OspfNetworkLsa::OspfNetworkLsa(const ns3::ospf::router_id_t& advertisingRouter, const ns3::ospf::network_id_t& linkStateId, const ns3::Ipv4Mask& networkMask) :
    OspfLsa (OSPF_LSA_TYPE_NETWORK, advertisingRouter),
    m_linkStateId(linkStateId),
    m_networkMask(networkMask)
{}

const Ipv4Address& OspfNetworkLsa::GetLinkStateId() const {
    return this->m_linkStateId;
}

const Ipv4Mask& OspfNetworkLsa::GetNetworkMask() const {
    return this->m_networkMask;
}

//---------------------------------------------------------------------
// OspfExternalRouterLsa
//---------------------------------------------------------------------

OspfExternalLsa::OspfExternalLsa(
    const ospf::router_id_t  & advertisingRouter,
    const ospf::network_id_t & linkStateId,
    const Ipv4Mask           & networkMask,
    const ospf::metric_t     & metric
) :
    OspfLsa (OSPF_LSA_TYPE_EXTERNAL, advertisingRouter),
    m_networkMask(networkMask),
    m_linkStateId (linkStateId),
    m_metric (metric)
{}

const ospf::metric_t& OspfExternalLsa::GetMetric() const {
    return this->m_metric;
}

void OspfExternalLsa::SetMetric(const ospf::metric_t& metric) {
    this->m_metric = metric;
}

const Ipv4Address& OspfExternalLsa::GetLinkStateId() const {
    return this->m_linkStateId;
}

void OspfExternalLsa::SetLinkStateId(const Ipv4Address& linkStateId) {
    this->m_linkStateId = linkStateId;
}

void OspfExternalLsa::Print(std::ostream& out) const {
    out << "External LSA(rid = " << this->GetAdvertisingRouter()
        << ", type: "    << this->GetLsaType() << " (5: External)"
        << ", network: " << this->m_linkStateId
        << ", metric: "  << this->m_metric
        << ')';
}

const Ipv4Mask& OspfExternalLsa::GetNetworkMask() const {
    return this->m_networkMask;
}

} // namespace ns3
