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
 * Adapted from:
 *   ./source/ns-3.20/src/network/utils/pcap-file.cc by Craig Dowell (craigdo@ee.washington.edu)
 * Author:
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 */

#include "pcap-wrapper.h"

namespace ns3 {

//---------------------------------------------------------------------------
// Endianness stuff
//---------------------------------------------------------------------------

uint8_t
PcapWrapper::Swap (uint8_t val)
{
//    NS_LOG_FUNCTION (this << static_cast<uint32_t> (val));
    return val;
}

uint16_t
PcapWrapper::Swap (uint16_t val)
{
//    NS_LOG_FUNCTION (this << val);
    return ((val >> 8) & 0x00ff) | ((val << 8) & 0xff00);
}

uint32_t
PcapWrapper::Swap (uint32_t val)
{
//    NS_LOG_FUNCTION (this << val);
    return ((val >> 24) & 0x000000ff)
         | ((val >> 8)  & 0x0000ff00)
         | ((val << 8)  & 0x00ff0000)
         | ((val << 24) & 0xff000000);
}

//---------------------------------------------------------------------------
// Pcap wrapping.
//---------------------------------------------------------------------------

void
PcapWrapper::Swap (PcapFileHeader *from, PcapFileHeader *to)
{
//    NS_LOG_FUNCTION (this << from << to);
    to->m_magicNumber = Swap (from->m_magicNumber);
    to->m_versionMajor = Swap (from->m_versionMajor);
    to->m_versionMinor = Swap (from->m_versionMinor);
    to->m_zone = Swap (uint32_t (from->m_zone));
    to->m_sigFigs = Swap (from->m_sigFigs);
    to->m_snapLen = Swap (from->m_snapLen);
    to->m_type = Swap (from->m_type);
}

void
PcapWrapper::Swap (const PcapRecordHeader *from, PcapRecordHeader *to)
{
//    NS_LOG_FUNCTION (this << from << to);
    to->m_tsSec = Swap (from->m_tsSec);
    to->m_tsUsec = Swap (from->m_tsUsec);
    to->m_inclLen = Swap (from->m_inclLen);
    to->m_origLen = Swap (from->m_origLen);
}

PcapWrapper::PcapWrapper (
    uint32_t dataLinkType,
    uint32_t snapLen,
    int32_t timeZoneCorrection,
    bool swapMode
) {
    m_fileHeader.m_magicNumber = MAGIC;
    m_fileHeader.m_versionMajor = VERSION_MAJOR;
    m_fileHeader.m_versionMinor = VERSION_MINOR;
    m_fileHeader.m_zone = timeZoneCorrection;
    m_fileHeader.m_sigFigs = 0;
    m_fileHeader.m_snapLen = snapLen;
    m_fileHeader.m_type = dataLinkType;

    union {
        uint32_t a;
        uint8_t  b[4];
    } u;

    u.a = 1;
    bool bigEndian = u.b[3];

    //
    // And set swap mode if requested or we are on a big-endian system.
    //
    m_swapMode = swapMode | bigEndian;
}

std::string
MakePcapFilename() {
    // pcapFilename = tmpnam(NULL);
    // Note that current directory is $NS3_DIR/dce/source/ns-3-dce/
    static unsigned numPacket = 0;
    std::string pcapFilename;

    numPacket++;
    std::ostringstream oss;
    oss << "packet" << numPacket << ".pcap";
    return oss.str();
}

} // namespace ns3

