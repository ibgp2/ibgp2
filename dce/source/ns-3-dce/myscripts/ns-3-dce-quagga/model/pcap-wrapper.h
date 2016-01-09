/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Marc-Olivier Buob
 * Adapted from:
 *  ./source/ns-3.20/src/network/utils/pcap-file.cc by Craig Dowell (craigdo@ee.washington.edu)
 */

#ifndef PCAP_WRAPPER_H
#define PCAP_WRAPPER_H

#include <string>               // std::string
#include <cstdint>              // uintXX_t
#include <iostream>             // std::cerr

#include "ns3/ptr.h"            // ns3::Ptr<>
#include "ns3/packet.h"         // ns3::Packet
#include "ns3/nstime.h"         // ns3::Time
#include "ns3/trace-helper.h"   // PcapHelper::DLT_PPP

namespace ns3 {

//---------------------------------------------------------------------------
// Pcap wrapping.
// Adapted from ./source/ns-3.20/src/network/utils/pcap-file.cc
// Unfortunately the PcapFile class assumes that we always want to write
// into a file. PcapWrapper allows to write in any stream.
//---------------------------------------------------------------------------

/**
 * @brief Pcap file header
 */

typedef struct {
    uint32_t m_magicNumber;   /**< Magic number identifying this as a pcap file */
    uint16_t m_versionMajor;  /**< Major version identifying the version of pcap used in this file */
    uint16_t m_versionMinor;  /**< Minor version identifying the version of pcap used in this file */
    int32_t  m_zone;          /**< Time zone correction to be applied to timestamps of packets */
    uint32_t m_sigFigs;       /**< Unused by pretty much everybody */
    uint32_t m_snapLen;       /**< Maximum length of packet data stored in records */
    uint32_t m_type;          /**< Data link type of packet data */
} PcapFileHeader;

/**
 * @brief Pcap record header
 */

typedef struct {
    uint32_t m_tsSec;         /**< seconds part of timestamp */
    uint32_t m_tsUsec;        /**< microseconds part of timestamp (nsecs for PCAP_NSEC_MAGIC) */
    uint32_t m_inclLen;       /**< number of octets of packet saved in file */
    uint32_t m_origLen;       /**< actual length of original packet */
} PcapRecordHeader;

/**
 * @brief Pcap wrapper (only std::ostream are currently supported)
 */

class PcapWrapper {
private:
    static const int32_t  ZONE_DEFAULT    = 0;           /**< Time zone offset for current location */
    static const uint32_t SNAPLEN_DEFAULT = 65535;       /**< Default value for maximum octets to save per packet */
    static const uint32_t MAGIC           = 0xa1b2c3d4;  /**< Magic number identifying standard pcap file format */
    static const uint16_t VERSION_MAJOR   = 2;           /**< Major version of supported pcap file format */
    static const uint16_t VERSION_MINOR   = 4;           /**< Minor version of supported pcap file format */

    PcapFileHeader  m_fileHeader;   /**< file header */
    bool            m_swapMode;     /**< swap mode */

//---------------------------------------------------------------------------
// Endianness stuff
// We could also use htons, htonl & co
//---------------------------------------------------------------------------

/**
 * @brief Changes endianness of an uint8_t
 * @param val The value to swap.
 * @returns The swapped value.
 */

uint8_t
Swap (uint8_t val);

/**
 * @brief Changes endianness of an uint16_t
 * @param val The value to swap.
 * @returns The swapped value.
 */

uint16_t
Swap (uint16_t val);

/**
 * @brief Changes endianness of an uint32_t
 * @param val The value to swap.
 * @returns The swapped value.
 */

uint32_t
Swap (uint32_t val);


    void
    Swap (PcapFileHeader *from, PcapFileHeader *to);

    void
    Swap (const PcapRecordHeader *from, PcapRecordHeader *to);

public:

    /**
     * @brief Swap a value byte order
     * @param val the value
     * @returns the value with byte order swapped
     */

    PcapWrapper (
        uint32_t dataLinkType,
        uint32_t snapLen = SNAPLEN_DEFAULT,
        int32_t timeZoneCorrection = ZONE_DEFAULT,
        bool swapMode = false
    );

    // TODO
    // Currently the following functions are template because I need to use hexstream
    // which do not inherits std::ostream. Ideally the should use std::ostream & as
    // parameters...

    template <typename Tstream>
    void
    WriteFileHeader (Tstream & out) {
        //NS_LOG_FUNCTION (this);

        //
        // We have the ability to write out the pcap file header in a foreign endian
        // format, so we need a temp place to swap on the way out.
        //
        PcapFileHeader header;

        //
        // the pointer headerOut selects either the swapped or non-swapped version of
        // the pcap file header.
        //
        PcapFileHeader *headerOut = 0;

        if (m_swapMode == false) {
            headerOut = &m_fileHeader;
        } else {
            Swap (&m_fileHeader, &header);
            headerOut = &header;
        }

        //
        // Watch out for memory alignment differences between machines, so write
        // them all individually.
        //
        out.write ((const char *) &headerOut->m_magicNumber,  sizeof(headerOut->m_magicNumber));
        out.write ((const char *) &headerOut->m_versionMajor, sizeof(headerOut->m_versionMajor));
        out.write ((const char *) &headerOut->m_versionMinor, sizeof(headerOut->m_versionMinor));
        out.write ((const char *) &headerOut->m_zone,         sizeof(headerOut->m_zone));
        out.write ((const char *) &headerOut->m_sigFigs,      sizeof(headerOut->m_sigFigs));
        out.write ((const char *) &headerOut->m_snapLen,      sizeof(headerOut->m_snapLen));
        out.write ((const char *) &headerOut->m_type,         sizeof(headerOut->m_type));
    }

    template <typename Tstream>
    uint32_t WritePacketHeader (Tstream & out, uint32_t tsSec, uint32_t tsUsec, uint32_t totalLen)
    {
        //NS_LOG_FUNCTION (this << tsSec << tsUsec << totalLen);

        uint32_t inclLen = totalLen > m_fileHeader.m_snapLen ? m_fileHeader.m_snapLen : totalLen;

        PcapRecordHeader header;
        header.m_tsSec   = tsSec;
        header.m_tsUsec  = tsUsec;
        header.m_inclLen = inclLen;
        header.m_origLen = totalLen;

        if (m_swapMode) {
            Swap (&header, &header);
        }

        // Watch out for memory alignment differences between machines, so write
        // them all individually.
        out.write ((const char *) &header.m_tsSec,   sizeof(header.m_tsSec));
        out.write ((const char *) &header.m_tsUsec,  sizeof(header.m_tsUsec));
        out.write ((const char *) &header.m_inclLen, sizeof(header.m_inclLen));
        out.write ((const char *) &header.m_origLen, sizeof(header.m_origLen));
        return inclLen;
    }

    template <typename Tstream>
    void Write (Tstream & out, uint32_t tsSec, uint32_t tsUsec, Ptr<const Packet> p)
    {
        //NS_LOG_FUNCTION (this << tsSec << tsUsec << p);

        uint32_t inclLen = this->WritePacketHeader (out, tsSec, tsUsec, p->GetSize ());

        // We cannot call this with hexstream since it does not inherits std::ostream
        //p->CopyData (&out, inclLen);

        // Work around: use an intermediate buffer (Packet does not offer access to its buffer and so we have to copy it).
        uint8_t * buffer = (uint8_t *) malloc(inclLen);
        if (buffer) {
            p->CopyData(buffer, inclLen);
            out.write((const char *) buffer, inclLen);
            free(buffer);
        } else {
            std::cerr << "PcapWrapper::Write(): Not enough memory" << std::endl;
        }
    }
};

/**
 * @brief Write a single packet in pcap format in an output (empty) stream.
 *   Other packet could be append using successively PcapWrapper::WritePacketHeader
 *   and PcapWrapper::Write methods.
 * @param out The output stream.
 * @param t The date of the packet's capture.
 * @param p The captured packet
 * @param dataLinkType The dataLinkType.
 */

template <typename Tstream>
void PacketWritePcap(Tstream & out, const ns3::Time & t, const Ptr<const Packet> p, uint32_t dataLinkType = PcapHelper::DLT_PPP) {
    uint64_t current = t.GetMicroSeconds ();
    uint64_t tsSec  = current / 1000000;
    uint64_t tsUsec = current % 1000000;
    PcapWrapper pcapWrapper(dataLinkType);
    pcapWrapper.WriteFileHeader(out);
    pcapWrapper.Write(out, tsSec, tsUsec, p);
}

/**
 * Craft a new pcap filename ($NS3_DIR/dce/source/ns-3-dce/packet_XXX.pcap)
 */

std::string
MakePcapFilename();

} // namespace ns3

#endif
