/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Marc-Olivier Buob
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
 */

#include "ns3/tcpdump-wrapper.h"

#include <sstream>              // std::ostringstream
#include <pstreams/pstream.h>   // redi::ipstream

#include "ns3/pcap-wrapper.h"   // PacketWritePcap
#include "ns3/trace-helper.h"   // PcapHelper::DLT_PPP

namespace ns3 {

//---------------------------------------------------------------------------
// TODO:
//
// Ideally this class should inherit properly of std::ostream
// http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
// We could use <pstreams/pstream.h> redi::ipstream ...
// http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c/10702464#10702464
// ... to stream tcpdump output.
//
// Once fixed:
//    We could expose hextream in header.
//    PcapWritePcap does not need to remain template.
//---------------------------------------------------------------------------

class hexstream
{
    private:
        std::ostringstream oss; /**< Internal buffer. */

    public:

        /**
         * @brief Constructor.
         */

        hexstream(){}

        /**
         * @brief Write primitive.
         * @param s The data to write in *this.
         * @param n The number of bytes that must be written in *this.
         * @returns The updated stream.
         */

        hexstream & write(const void * s, std::streamsize n) {
            char buffer[5];
            const uint8_t * p = (const uint8_t *) s;
            for (std::streamsize i = 0; i < n; ++i, ++p) {
                sprintf(buffer, "\\x%02x", *p);
                oss.write(buffer, 4);
            }
            return *this;
        }

        /**
         * @returns The binary shell representation of the data stored
         *    in the internal buffer.
         */

        inline std::string str() const {
            return this->oss.str();
        }
};

void tcpdump(std::ostream & out, const char * packet)
{
    // Craft command
    // Traps:
    // - Run "/bin/echo", not "echo", otherwise you run the built-in shell!
    // - Do not forget "-n" otherwise you'll get a trailing character which make crash tcpdump
    // - We don't want to write a temporary pcap file because it will slow the program

    std::ostringstream oss;
    oss << "/bin/echo -ne \"" << packet << "\" | /usr/sbin/tcpdump -nnv -r - 2> /dev/null";
    const std::string command = oss.str();

    // For obscure reasons popen(command, "r") won't work, so let's use pstream
    // We could also get stderr as shown here:
    // http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c

    redi::ipstream proc(command);
    std::string line;

    // Read child's stdout

    while (std::getline(proc.out(), line))
        out << line << std::endl;
}

std::string
tcpdump(Ptr<const Packet> p, uint32_t dataLinkType)
{
    std::string dumpStr;
    hexstream ohs;
    std::ostringstream oss;
    PacketWritePcap(ohs, Simulator::Now(), p, dataLinkType);
    tcpdump(oss, ohs.str().c_str());
    dumpStr = oss.str();
    return dumpStr;
}

//---------------------------------------------------------------
//
// // This is an example of parsing of tcpdump's output
//
// #include <boost/algorithm/string.hpp>   // boost::split
// #include "ns3/pcap-wrapper.h"           // PacketWritePcap
// #include "ns3/ospf-packet.h"            // Ospf*,
//
// // Some indexes used to parse the tcpdump output
// #define IP_PROTO_I                      5
// #define OSPF_TYPE_I                     1
// #define OSPF_RID_I                      1
//
// // Some strings used to parse the tcpdump output
// #define PROTO_OSPF_S                    " proto OSPF (89)"
// #define OSPF_RID_S                      "Router-ID"
// #define OSPF_LSU_S                      " LS-Update"
// #define OSPF_LSA_S                      "LSA #"
// #define OSPF_LSA_NETWORK_S              "Network"
// #define OSPF_LSA_EXTERNAL_S             "External"
// #define OSPF_LSA_ROUTER_S               "Router"
// #define OSPF_LSA_ROUTER_NETWORK_S       "Network"
// #define OSPF_LSA_ROUTER_STUB_NETWORK    "Stub"
// #define OSPFD_RID_S                     "Router ID"
//
// // Author: Alexandre Morignot <alexandre.morignot@orange.fr>
//
// namespace ns3 {
//
// std::vector<OspfLsa *>
// LegacyExtractOspfLsa (
//     const Ptr<const Packet> p,
//     std::map<ospf::network_id_t, ospf::network_prefix_t> & prefixMap
// ) {
// //    NS_LOG_FUNCTION (this << p);
//
//     std::vector<OspfLsa *> lsas;
//     std::string dumpStr;
//
//     dumpStr = tcpdump(p, PcapHelper::DLT_PPP);
//
//     // Parse the output of tcpdump
//     std::vector<std::string> lines;
//     boost::split (lines, dumpStr, boost::is_any_of ("\n"));
//
//     // we remove the last line, which is empty
//     lines.erase (lines.end ());
//
//     if (lines.size () <= 3)
//     {
// //        NS_LOG_INFO ("[!!] Not an OSPF packet: [" << dumpStr << ']');
//         return lsas;
//     }
//
//     // First line, IP header
//     std::vector<std::string> fields;
//     boost::split (fields, lines[0], boost::is_any_of (","), boost::token_compress_on);
//     if (fields.empty () || fields.size () <= IP_PROTO_I || fields[IP_PROTO_I] != PROTO_OSPF_S)
//     {
// //        NS_LOG_WARN ("Malformed IP packet");
//         return lsas;
//     }
//
//     // 3d line, Router ID
//     boost::split (fields, lines[2], boost::is_any_of ("\t ,"), boost::token_compress_on);
//     if (fields.empty () || fields.size () <= OSPF_RID_I + 1 || fields[OSPF_RID_I] != OSPF_RID_S)
//     {
// //        NS_LOG_WARN ("Malformed OSPF packet");
//         return lsas;
//     }
//
//     // 2d line, OSPF packet type
//     boost::split (fields, lines[1], boost::is_any_of (","), boost::token_compress_on);
//     if (fields.empty () || fields.size () <= OSPF_TYPE_I)
//     {
// //        NS_LOG_WARN ("Malformed OSPF packet");
//         return lsas;
//     }
//
//     // We are only interested on LS-Update packet
//     if (fields[OSPF_TYPE_I] == OSPF_LSU_S)
//     {
// //        NS_LOG_DEBUG ("OSPF packet\n" << dumpStr);
//
//         int cursor = 3;
//         while (cursor < lines.size ())
//         {
//             while (cursor < lines.size () && lines[cursor].find (OSPF_LSA_S) == std::string::npos)
//             {
//                 cursor++;
//             }
//
//             if (cursor + 2 >= lines.size ())
//             {
//                 break;
//             }
//
//             // the LSA start on the next line
//             cursor++;
//
//             // first line : advertising router
//             boost::split (fields, lines[cursor], boost::is_any_of ("\t ,"), boost::token_compress_on);
//             if (fields.size () <= 3)
//             {
//                 continue;
//             }
//
//             std::cout << "ALEX: getting rid: " << lines[cursor] << " --> " << fields[3].c_str () << std::endl;
//             ospf::router_id_t rid (fields[3].c_str ());
//
//             // second line : lsa type
//             cursor++;
//             boost::split (fields, lines[cursor], boost::is_any_of ("\t ,"), boost::token_compress_on);
//             if (fields.size () <= 1)
//             {
//                 continue;
//             }
//
//             if (fields[1] == OSPF_LSA_ROUTER_S)
//             {
//                 struct OspfRouterLsa * routerLsa = new struct OspfRouterLsa;
//                 routerLsa->rid = rid;
//
//                 cursor++;
//                 // we search for the beginning of the network list
//                 while (cursor < lines.size () && lines[cursor].find (OSPF_LSA_ROUTER_NETWORK_S) == std::string::npos)
//                 {
//                     cursor++;
//                 }
//
//                 // we loop till the end or the next LSA
//                 while (cursor < lines.size () && lines[cursor].find (OSPF_LSA_S) == std::string::npos)
//                 {
//                     const char * nid;
//
//                     // first line of LSA: Network-ID
//                     boost::split (fields, lines[cursor], boost::is_any_of ("\t ,"), boost::token_compress_on);
//                     std::string networkState = fields[1];
//                     nid = fields[3].c_str ();
//
//                     ospf::network_id_t networkAddress (nid);
//
//                     if (networkState == OSPF_LSA_ROUTER_STUB_NETWORK)
//                     {
//                         cursor += 2;
//                         continue;
//                     }
//
//                     Ipv4Address ifAddr (fields[6].c_str ());
//                     routerLsa->ifs[networkAddress] = ifAddr;
//
//                     cursor++;
//                     // second line of LSA: metric
//                     boost::split (fields, lines[cursor], boost::is_any_of ("\t ,"), boost::token_compress_on);
//                     int networkMetric = std::stoi (fields[5]);
//
//                     routerLsa->networks[networkAddress] = networkMetric;
//                     cursor++;
//                 }
//
//                 lsas.push_back (routerLsa);
//             }
//             else if (fields[1] == OSPF_LSA_EXTERNAL_S)
//             {
//                 const char * nid = fields[5].c_str ();
//                 cursor += 2;
//
//                 // this line contain the mask
//                 boost::split (fields, lines[cursor], boost::is_any_of ("\t ,"), boost::token_compress_on);
//                 if (fields.size () <= 2)
//                 {
//                     // malformed packet
//                     continue;
//                 }
//
//                 const char * mask = fields[2].c_str ();
//                 ospf::network_prefix_t prefix (nid, mask);
//                 ospf::network_id_t network (nid);
//
//                 prefixMap[network] = prefix;
//
//                 cursor++;
//                 // next line: metric
//                 boost::split (fields, lines[cursor], boost::is_any_of ("\t ,"), boost::token_compress_on);
//                 if (fields.size () <= 7)
//                 {
//                     // malformed packet
//                     continue;
//                 }
//
//                 ospf::metric_t metric (std::stoi (fields[7]));
//
//                 struct OspfExternalLsa * externalLsa = new struct OspfExternalLsa (network, metric);
//                 externalLsa->rid = rid;
//
//                 lsas.push_back (externalLsa);
//             }
//             if (fields[1] == OSPF_LSA_NETWORK_S)
//             {
//                 const char * nid = fields[5].c_str (); // Link State ID
//                 cursor += 2;
//
//                 boost::split (fields, lines[cursor], boost::is_any_of ("\t ,"), boost::token_compress_on);
//                 if (fields.size () <= 2)
//                 {
//                     // malformed packet
//                     continue;
//                 }
//
//                 const char * mask = fields[2].c_str ();
//                 ospf::network_prefix_t prefix (nid, mask);
//                 ospf::network_id_t network (nid);
//
//                 prefixMap[network] = prefix;
//             }
//         }
//     }
//
//     return lsas;
// }

} // namespace ns3

