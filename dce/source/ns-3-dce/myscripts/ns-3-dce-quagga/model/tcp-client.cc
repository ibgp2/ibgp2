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
 */

#include <cstring> // memcpy

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "tcp-client.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpClientApplication");
NS_OBJECT_ENSURE_REGISTERED (TcpClient);

TypeId
TcpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpClient")
    .SetParent<Application> ()
    .AddConstructor<TcpClient> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&TcpClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("Tx", "A new is sent",
                     MakeTraceSourceAccessor (&TcpClient::m_txTrace))
  ;
  return tid;
}

TcpClient::TcpClient ()
  : recvCallback (MakeCallback (&TcpClient::HandleRead, this)) // default callback
{
  NS_LOG_FUNCTION (this);
  m_queue = CreateObject<DropTailQueue> ();
  m_socket = 0;
  m_sendEvent = EventId ();
}

TcpClient::~TcpClient()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
TcpClient::SetRemote (const Address & ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
TcpClient::SetRemote (const Ipv4Address & ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
TcpClient::SetRemote (const Ipv6Address & ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
TcpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
TcpClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  OpenSocket ();

  NS_LOG_LOGIC ("Connecting to queue " << m_queue);
  bool connected = m_queue->TraceConnectWithoutContext ("Enqueue", MakeCallback (&TcpClient::HandleEnqueue, this));
  NS_ASSERT_MSG (connected == true, "Unable to hook \"Enqueue\"");
}

void
TcpClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  CloseSocket ();
  Simulator::Cancel (m_sendEvent);
}

void
TcpClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);

  m_sendEvent = Simulator::Schedule (dt, &TcpClient::Send, this);
}

bool
TcpClient::Enqueue (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this);
  return m_queue->Enqueue (packet);
}

bool
TcpClient::EnqueueString (const std::string & fill)
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> p;
  uint32_t dataSize = fill.size () + 1;
  uint8_t data[dataSize];

  memcpy (data, fill.c_str (), dataSize);
  p = Create<Packet> (data, dataSize);

  return this->Enqueue (p);
}

void
TcpClient::Send (void)
{
  NS_LOG_FUNCTION (this);

  // if the socket is closed, we re-open it
  if (m_socket == 0)
    {
      OpenSocket ();
      return;
    }

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p;

  // send all the packets in the queue
  while ((p = m_queue->Dequeue ()))
    {
      // call to the trace sinks before the packet is actually sent,
      // so that tags added to the packet can be sent as well
      m_txTrace (p);

      int result = m_socket->Send (p);
      NS_ASSERT_MSG (result >= 0, "TcpClient::Send: unable to send the packet");

      if (Ipv4Address::IsMatchingType (m_peerAddress))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << p->GetSize () << " bytes to " <<
                       Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
        }
      else if (Ipv6Address::IsMatchingType (m_peerAddress))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << p->GetSize () << " bytes to " <<
                       Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
        }
      else
        {
          NS_LOG_WARN ("Peer address type unknown");
        }
    }
}

void
TcpClient::SetRecvCallback (Callback<void, Ptr<Socket>> callback)
{
  if (m_socket != 0)
    {
      // the socket is already created, we change the callback
      m_socket->SetRecvCallback (callback);
    }

  recvCallback = callback;
}

void
TcpClient::CloseSocket ()
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      NS_LOG_LOGIC ("The socket is already closed");
    }
  else
    {
      NS_LOG_LOGIC ("Closing the socket");
      //m_socket->Close (); // TODO: Why iBGPv2 enters in infinite loop if this is uncommented?
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

void
TcpClient::OpenSocket ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("\tm_peerAddress = " << Ipv4Address::ConvertFrom (m_peerAddress)
      << "\tm_peerPort = " << m_peerPort
  );

  if (m_socket)
    {
      NS_LOG_LOGIC ("The socket is already open");
      return;
    }

  int result;
  TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), tid);

  if (Ipv4Address::IsMatchingType(m_peerAddress))
    {
      m_socket->Bind();
      result = m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
    }
  else if (Ipv6Address::IsMatchingType(m_peerAddress))
    {
      m_socket->Bind6();
      result = m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
    }

  NS_ASSERT_MSG (result == 0,
      "TcpClient::OpenSocket: unable to Connect to "
      << Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort
  );
  m_socket->SetRecvCallback (recvCallback);

  // we don't distinguish the normal or the error case
  m_socket->SetCloseCallbacks (MakeCallback (&TcpClient::HandleClose, this), MakeCallback (&TcpClient::HandleErrorClose, this));
  m_socket->SetConnectCallback (MakeCallback (&TcpClient::HandleConnect, this), MakeNullCallback<void, Ptr<Socket> > ());
}

void
TcpClient::HandleRead (Ptr<Socket> socket)
{
/*
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO (
              "At time " << Simulator::Now ().GetSeconds ()
              << "s client received " << packet->GetSize ()
              << " bytes from " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ()
              << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
          );
        }

      uint8_t buffer[1000];
      packet->CopyData (buffer, 1000);
      NS_LOG_DEBUG (buffer);
    }
*/
}

void
TcpClient::HandleClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_INFO ("The remote host has closed the socket");
  CloseSocket ();
}

void
TcpClient::HandleErrorClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_INFO ("The socket was closed due to an error : "
      << gai_strerror (socket->GetErrno ()) << " ("
      << socket->GetErrno () << ")"
      << std::endl
      << "\tm_peerAddress = " << Ipv4Address::ConvertFrom (m_peerAddress)
      << "\tm_peerPort = " << m_peerPort
  );
  CloseSocket ();
}

void
TcpClient::HandleConnect (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // The socket is ready, we can send the packets waiting in the queue
  Send ();
}

void
TcpClient::HandleEnqueue (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // If the socket doesn't exist, we create it
  if (m_socket == 0)
    {
      OpenSocket ();
    }

  // We can not juste send instantly the packet, this callback is called
  // before the enqueue is finished.
  // If the event is not expired, it means that a Send () is already scheduled,
  // so we do not need to schedule an other one.
  if (m_sendEvent.IsExpired ())
    ScheduleTransmit (Seconds (0));
}

} // Namespace ns3
