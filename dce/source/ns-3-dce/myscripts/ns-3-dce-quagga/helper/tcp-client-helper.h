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
 */
#ifndef TCP_CLIENT_HELPER_H
#define TCP_CLIENT_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/socket.h"

namespace ns3 {

/**
 * \ingroup tcpclient
 * \brief Create an application which sends a TCP packet.
 */
class TcpClientHelper : public Object
{
public:
  /**
   * Create TcpClientHelper which will make life easier for people trying
   * to send TCP packets.
   *
   * \param ip The IP address of the remote tcp server
   * \param port The port number of the remote tcp server
   */
  TcpClientHelper (const Address & ip, uint16_t port);
  /**
   * Create TcpClientHelper which will make life easier for people trying
   * to send TCP packets.
   *
   * \param ip The IPv4 address of the remote tcp server
   * \param port The port number of the remote tcp server
   */
  TcpClientHelper (const Ipv4Address & ip, uint16_t port);
  /**
   * Create TcpClientHelper which will make life easier for people trying
   * to send TCP packets.
   *
   * \param ip The IPv6 address of the remote tcp server
   * \param port The port number of the remote tcp server
   */
  TcpClientHelper (const Ipv6Address & ip, uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (const std::string & name, const AttributeValue & value);

  /**
   * Given a pointer to a TcpClient application, create a packet and add it
   * to send queue of the TcpClient.
   *
   * \param app Smart pointer to the application (real type must be TcpClient).
   * \param fill The string to use as the actual data bytes.
   */
  void Send (Ptr<Application> app, const std::string & fill);

  /**
   * Given a pointer to a TcpClient application, create a packet and then schedule
   * the add of the packet to the queue for dt.
   *
   * \param dt time interval.
   * \param app Smart pointer to the application (real type must be TcpClient).
   * \param fill The string to use as the actual data bytes.
   */
  void ScheduleSend (Time dt, Ptr<Application> app, const std::string & fill);

  /**
   * Given a pointer to a TcpClient application, set the callback called when
   * receiving a packet.
   *
   * \param app Smart pointer to the application (real type must be TcpClient).
   * \param callback The callback to use.
   */
  void SetRecvCallback (Ptr<Application> app, Callback<void, Ptr<Socket>> callback);

  /**
   * Create a tcp client application on the specified node.  The Node
   * is provided as a Ptr<Node>.
   *
   * \param node The Ptr<Node> on which to create the TcpClientApplication.
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the
   *          application created
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a tcp client application on the specified node.  The Node
   * is provided as a string name of a Node that has been previously
   * associated using the Object Name Service.
   *
   * \param nodeName The name of the node on which to create the TcpClientApplication
   *    See ns3::Names
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the
   *          application created
   */
  ApplicationContainer Install (const std::string & nodeName) const;

  /**
   * \param c the nodes
   *
   * Create one tcp client application on each of the input nodes
   *
   * \returns the applications created, one application per input node.
   */
  ApplicationContainer Install (NodeContainer & c) const;

  /**
   * Close the socket of app.
   *
   * \param app Smart Pointer to the application (real type must be TcpClient).
   */
  void CloseSocket (Ptr<Application> app);

private:
  /**
   * Install an ns3::TcpClient on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an TcpClient will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* TCP_ECHO_HELPER_H */
