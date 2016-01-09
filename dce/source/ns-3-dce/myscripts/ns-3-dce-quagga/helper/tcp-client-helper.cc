/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modified by: Alexandre Morignot
 */
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "tcp-client-helper.h"
#include "ns3/tcp-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/packet.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpClientHelper");
NS_OBJECT_ENSURE_REGISTERED (TcpClientHelper);

TcpClientHelper::TcpClientHelper (const Address & address, uint16_t port) {
    NS_LOG_FUNCTION (this);

    m_factory.SetTypeId (TcpClient::GetTypeId ());
    SetAttribute ("RemoteAddress", AddressValue (address));
    SetAttribute ("RemotePort", UintegerValue (port));
}

TcpClientHelper::TcpClientHelper (const Ipv4Address & address, uint16_t port) {
    NS_LOG_FUNCTION (this << address << port);

    m_factory.SetTypeId (TcpClient::GetTypeId ());
    SetAttribute ("RemoteAddress", AddressValue (Address (address)));
    SetAttribute ("RemotePort", UintegerValue (port));
}

TcpClientHelper::TcpClientHelper (const Ipv6Address & address, uint16_t port) {
    NS_LOG_FUNCTION (this);

    m_factory.SetTypeId (TcpClient::GetTypeId ());
    SetAttribute ("RemoteAddress", AddressValue (Address (address)));
    SetAttribute ("RemotePort", UintegerValue (port));
}

void
TcpClientHelper::SetAttribute (
    const std::string & name,
    const AttributeValue & value
) {
    NS_LOG_FUNCTION (this);

    m_factory.Set (name, value);
}

void
TcpClientHelper::Send (Ptr<Application> app, const std::string & fill) {
    NS_LOG_FUNCTION (this);

    /*
    Ptr<Packet> p;
    uint32_t dataSize = fill.size () + 1;
    uint8_t data[dataSize];

    memcpy (data, fill.c_str (), dataSize);
    p = Create<Packet> (data, dataSize);

    app->GetObject<TcpClient>()->Enqueue (p);
    */
    app->GetObject<TcpClient>()->EnqueueString (fill);
}

void
TcpClientHelper::ScheduleSend (Time dt, Ptr<Application> app, const std::string & fill) {
    NS_LOG_FUNCTION (this);

    Simulator::Schedule (dt, &TcpClientHelper::Send, this, app, fill);
}

void
TcpClientHelper::SetRecvCallback (Ptr<Application> app, Callback<void, Ptr<Socket> > callback) {
    NS_LOG_FUNCTION (this);

    app->GetObject<TcpClient>()->SetRecvCallback (callback);
}

ApplicationContainer
TcpClientHelper::Install (Ptr<Node> node) const {
    NS_LOG_FUNCTION (this);

    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TcpClientHelper::Install (const std::string & nodeName) const {
    NS_LOG_FUNCTION (this);

    Ptr<Node> node = Names::Find<Node> (nodeName);
    return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TcpClientHelper::Install (NodeContainer & c) const {
    NS_LOG_FUNCTION (this);

    ApplicationContainer apps;

    for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
        apps.Add (InstallPriv (*i));
    }

    return apps;
}

void
TcpClientHelper::CloseSocket (Ptr<Application> app) {
    NS_LOG_FUNCTION (this);

    app->GetObject<TcpClient>()->CloseSocket ();
}

Ptr<Application>
TcpClientHelper::InstallPriv (Ptr<Node> node) const {
    NS_LOG_FUNCTION (this);

    Ptr<Application> app = m_factory.Create<TcpClient> ();
    node->AddApplication (app);

    return app;
}

} // namespace ns3
