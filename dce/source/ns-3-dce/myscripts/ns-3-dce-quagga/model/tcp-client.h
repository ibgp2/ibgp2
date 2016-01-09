/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author: Alexandre Morignot <alexandre.morignot@orange.fr>
 * Adapted from udp-echo-client.h
 */

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <string>                   // std::string

#include "ns3/application.h"        // ns3::Application
#include "ns3/callback.h"           // ns3::Callback
#include "ns3/drop-tail-queue.h"    // ns3::DropTailQueue
#include "ns3/event-id.h"           // ns3::EventId
#include "ns3/packet.h"             // ns3::Packet
#include "ns3/ptr.h"                // ns3::Ptr
#include "ns3/socket.h"             // ns3::Socket
#include "ns3/ipv4-address.h"       // ns3::Ipv4Address
#include "ns3/ipv6-address.h"       // ns3::Ipv6Address
#include "ns3/traced-callback.h"    // ns3::TracedCallback

namespace ns3 {


/**
 * \ingroup tcpclient
 * \brief A Tcp client
 * Every packet sent should be returned by the server and received here.
 */

class TcpClient :
    public Application
{
public:

  /**
   * \brief Constructor.
   */

  TcpClient ();

  /**
   * \brief Destructor.
   */

  virtual ~TcpClient ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */

  static TypeId GetTypeId (void);

  /**
   * \brief set the remote address and port
   * \param ip remote IPv4 address
   * \param port remote port
   */

  void SetRemote (const Ipv4Address & ip, uint16_t port);

  /**
   * \brief set the remote address and port
   * \param ip remote IPv6 address
   * \param port remote port
   */

  void SetRemote (const Ipv6Address & ip, uint16_t port);

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */

  void SetRemote (const Address & ip, uint16_t port);

  /**
   * Enqueue a packet to be send.
   * \param packet The packet to enqueue.
   * \return False if the packet was droped, else True.
   */

  bool Enqueue (Ptr<Packet> packet);
  bool EnqueueString (const std::string & fill);

  /**
   * \brief Send a packet
   */

  void Send (void);

  /**
   * \brief Assign a callback when getting the answer.
   */

  void SetRecvCallback (Callback<void, Ptr<Socket>> callback);

  /**
   * \brief Close the socket
   */

  void CloseSocket ();

  /**
   * \brief Open the socket
   */

  void OpenSocket ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Schedule the next packet transmission
   * \param dt time interval between packets.
   */

  void ScheduleTransmit (Time dt);

  /**
   * \brief Handle a packet reception.
   * This function is called by lower layers.
   * \param socket the socket the packet was received to.
   */

  void HandleRead (Ptr<Socket> socket);

  /**
   * \brief Handle the close of the socket.
   * This function is called by lower layers.
   * \param socket the socket thas was closed.
   */

  void HandleClose (Ptr<Socket> socket);

  /**
   * \brief Handle the close of the socket due to an error.
   * This function is called by lower layers.
   * \param socket the socket thas was closed.
   */

  void HandleErrorClose (Ptr<Socket> socket);

  /**
   * \brief Handle the connexion of the socket to the remote host.
   * This function is called by lower layers.
   * \param socket the socket thas was connected.
   */

  void HandleConnect (Ptr<Socket> socket);

  /**
   * \brief Handle the enqueue of a packet.
   * This function is called by the queue.
   * \param socket the socket thas was connected.
   */

  void HandleEnqueue (Ptr<Packet const> packet);

  Ptr<DropTailQueue>    m_queue;        //!< Queue to stock the packets before sending them
  Ptr<Socket>           m_socket;       //!< Socket
  Address               m_peerAddress;  //!< Remote peer address
  uint16_t              m_peerPort;     //!< Remote peer port
  EventId               m_sendEvent;    //!< Event to send the next packet

  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;

  /// Callback for received packets
  Callback<void, Ptr<Socket> > recvCallback;
};

} // namespace ns3

#endif /* TCP_ECHO_CLIENT_H */
