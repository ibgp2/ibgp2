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

#ifndef TELNET_WRAPPER_H
#define TELNET_WRAPPER_H

#include <cstdint>                      // uint*_t
#include <ostream>                      // std::ostream
#include <fstream>                      // std::ostream
#include <string>                       // std::string

#include "ns3/application.h"            // ns3::Application
#include "ns3/address.h"                // ns3::Address
#include "ns3/callback.h"               // ns3::Callback
#include "ns3/node.h"                   // ns3::Node
#include "ns3/ptr.h"                    // ns3::Ptr
#include "ns3/socket.h"                 // ns3::Socket
#include "ns3/nstime.h"                 // ns3::Time
#include "ns3/tcp-client.h"             // ns3::TcpClient

namespace ns3 {

class TelnetSink {
private:
    size_t m_bufferSize; /**< Size of the internal buffer. */
public:
    /**
     * @brief Constructor.
     * @param bufferSize Size of the nested buffer.
     */

    TelnetSink(size_t bufferSize = 200);

    /**
     * @brief Retrieve the size of the buffer used to handle data from the socket.
     * @return The buffer size.
     */

    size_t GetBufferSize() const;

    /**
     * @brief Function called back when telnet response is handled.
     * @param socket The socket read by this TelnetSimpleSink.
     */

    void virtual HandleData(Ptr<Socket> socket);

    /**
     * @brief Function called back when a batch of response is handled.
     * @param buffer The buffer containing the batch (null terminated) and
     *    of size this->GetBufferSize().
     */

    void virtual HandleBatch(uint8_t * buffer);
};

/**
 * \class TelnetSimpleSink
 * @brief Handle telnet results and write them in an output file.
 */

class TelnetSimpleSink :
    public TelnetSink
{
private:
    std::string     m_outputFilename;     /**< Absolute path of the output file. */
    std::ofstream   m_ofs;                /**< Stream to the output file. */
public:

    TelnetSimpleSink(){}

    /**
     * @brief Constructor.
     * @param outputFilename The absolute path of the file where we write results
     * @param bufferSize Size of the nested buffer.
     */

    TelnetSimpleSink(const std::string & outputFilename, size_t bufferSize = 200);

    /**
     * @brief Close the nested ofstream.
     */

    void Close();

    /**
     * @returns Retrieve the absolute path corresponding to the nested ofstream.
     */

    const std::string & GetOutputFilename() const;

    /**
     * @brief Function called back when a batch of response is handled.
     * @param buffer The buffer containing the batch (null terminated) and
     *    of size this->GetBufferSize().
     */

    void virtual HandleBatch(uint8_t * buffer);
};

/**
 * \class Telnet
 * @brief Connect to a node able to handle telnet connections (for instance running DCE quagga)
 */

class Telnet
{

private:
    TelnetSimpleSink    m_sink;             /**< Sink where telnet results are written. */
    Ptr<TcpClient>      m_tcpClient;        /**< TCP client used to send telnet commands. */
    Address             m_remoteAddress;    /**< Address of the telnet server. */
    uint16_t            m_remotePort;       /**< Listening port of the telnet server. */
public:
    static TypeId GetTypeId (void);

    /**
     * @brief Constructor.
     * @param node The remote Node.
     * @param port The remote port.
     * @param address The IP address used to contact the remote node.
     * @param outputFilename The absolute path of the output file in which
     *    the telnet results are written.
     * @param time The time on which the telnet command are run. Be careful
     *    to choose a time before the end of the simulation.
     */

    Telnet(
        Ptr<Node> node,
        const Address & address,
        uint16_t port,
        const std::string & outputFilename,
        const Time & time
    );

    Telnet(
        Ptr<Node> node,
        const Address & address,
        uint16_t port,
        Callback<void, Ptr<Socket> > callback,
        const Time & time
    );

    /**
     * @brief Constructor.
     * @param node The remote Node. We will connect using its first
     *    IPv4 address (127.0.0.1 excluded).
     * @param port The remote port.
     * @param outputFilename The absolute path of the output file in which
     *    the telnet results are written.
     * @param time The time on which the telnet command are run. Be careful
     *    to choose a time before the end of the simulation.
     */

    Telnet(
        Ptr<Node> node,
        uint16_t port,
        const std::string & outputFilename,
        const Time & time
    );

    /**
     * @brief Retrieve the Address of the telnet server.
     * @return The Address of the telnet server.
     */

    const Address & GetRemoteAddress() const;

    /**
     * @brief Retrieve the listening port of the telnet server.
     * @return The telnet server's listening port.
     */

    uint16_t GetRemotePort() const;

    /**
     * @brief Retrieve the sink where the results are written.
     * @return The sink.
     */

    const TelnetSimpleSink & GetSink() const;

    /**
     * @brief Close the file in which the result are written. You can call this
     * method after running Simulator::Destroy()
     */

    void Close();

    /**
     * @brief Send a line (or several lines) through telnet to the remote node.
     * @param command A string.
     */

    Telnet & AppendCommand(const std::string & command);

    /**
     * @brief Print this Telnet instance.
     * @param os The output stream.
     */

    virtual std::ostream & Print(std::ostream & os) const;
};
} // namespace ns3

#endif
