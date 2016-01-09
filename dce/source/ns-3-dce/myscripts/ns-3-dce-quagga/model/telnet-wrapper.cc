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

#include "telnet-wrapper.h"

#include <iostream>                     // std::cout
#include <sstream>                      // std::istringstream
#include <string>                       // std::getline, std::string

#include "ns3/ipv4.h"                   // ns3::Ipv4
#include "ns3/tcp-client.h"             // ns3::TcpClient
#include "ns3/tcp-client-helper.h"      // ns3::TcpClientHelper

namespace ns3 {

// Internals

static Ptr<TcpClient> MakeTelnet(Ptr<Node> node, const Address & address, uint16_t port, const Time & time) {
    TcpClientHelper tcpHelper = TcpClientHelper(address, port);
    Ptr<Application> app = tcpHelper.Install(node).Get(0);
    Ptr<TcpClient> telnet = app->GetObject<TcpClient>();
    telnet->SetStartTime(time);
    return telnet;
}

// TelnetSink

TelnetSink::TelnetSink(size_t bufferSize):
    m_bufferSize(bufferSize)
{}

size_t TelnetSink::GetBufferSize() const {
    return this->m_bufferSize;
}

void TelnetSink::HandleData(Ptr<Socket> socket) {
    NS_ASSERT(this->m_bufferSize > 0);
    uint8_t buffer[this->m_bufferSize];
    while (unsigned numBytes = socket->Recv(buffer, this->m_bufferSize - 1, 0)) {
        buffer[numBytes] = '\0';
        this->HandleBatch(buffer);
    }
}

void TelnetSink::HandleBatch(uint8_t * buffer) {
    std::cout << buffer;
}

// TelnetSimpleSink

TelnetSimpleSink::TelnetSimpleSink(const std::string & outputFilename, size_t bufferSize):
    TelnetSink(bufferSize),
    m_outputFilename(outputFilename),
    m_ofs(outputFilename)
{}

void TelnetSimpleSink::Close() {
    if (this->m_ofs) this->m_ofs.close();
}

const std::string& TelnetSimpleSink::GetOutputFilename() const {
    return this->m_outputFilename;
}

void TelnetSimpleSink::HandleBatch(uint8_t * buffer) {
    if (this->m_ofs) this->m_ofs << buffer;
}

// Telnet

Telnet::Telnet(
    Ptr<Node> node,
    const Address & address,
    uint16_t port,
    Callback<void, Ptr<Socket> > callback,
    const Time & time
):
    m_remoteAddress(address),
    m_remotePort(port)
{
    this->m_tcpClient = MakeTelnet(node, address, port, time);
    this->m_tcpClient->SetRecvCallback(callback);
}

Telnet::Telnet(
    Ptr<Node> node,
    const Address & address,
    uint16_t port,
    const std::string & outputFilename,
    const Time & time
):
    m_remoteAddress(address),
    m_remotePort(port),
    m_sink(outputFilename)
{
    this->m_tcpClient = MakeTelnet(node, address, port, time);
    this->m_tcpClient->SetRecvCallback(MakeCallback(&TelnetSimpleSink::HandleData, &(this->m_sink)));
}

Telnet::Telnet(
    Ptr<Node> node,
    uint16_t port,
    const std::string & outputFilename,
    const Time & time = Time(0)
):
    m_remotePort(port),
    m_sink(outputFilename)
{
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    const Ipv4InterfaceAddress & ipv4InterfaceAddress = ipv4->GetAddress(1, 0);
    this->m_remoteAddress = ipv4InterfaceAddress.GetLocal();
    this->m_tcpClient = MakeTelnet(node, this->m_remoteAddress, port, time);
    this->m_tcpClient->SetRecvCallback(MakeCallback(&TelnetSimpleSink::HandleData, &(this->m_sink)));
}

void Telnet::Close() {
    this->m_sink.Close();
}

const Address& Telnet::GetRemoteAddress() const {
    return this->m_remoteAddress;
}

uint16_t Telnet::GetRemotePort() const {
    return this->m_remotePort;
}

const TelnetSimpleSink& Telnet::GetSink() const {
    return this->m_sink;
}

Telnet & Telnet::AppendCommand(const std::string & command) {
    std::istringstream iss(command.c_str());
    for (std::string line; std::getline (iss, line);) {
        line.push_back('\n');
        this->m_tcpClient->EnqueueString(line);
    }
    return *this;
}

std::ostream & Telnet::Print(std::ostream & os) const {
    os << "Telnet(" << this->m_remoteAddress << ':' << this->m_remotePort << ')';
    return os;
}

}
