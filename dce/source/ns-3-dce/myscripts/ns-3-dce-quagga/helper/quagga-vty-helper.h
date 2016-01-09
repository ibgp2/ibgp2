/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Hajime Tazaki, NICT
 *               2015 Marc-Olivier Buob
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 * Modified by:
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 */

#ifndef QUAGGA_VTY_HELPER_H
#define QUAGGA_VTY_HELPER_H

#include <list>                             // std::list
#include <set>                              // std::set
#include <string>                           // std::string

#include "ns3/node.h"                       // ns3::Node
#include "ns3/node-container.h"             // ns3::NodeContainer
#include "ns3/nstime.h"                     // ns3::Time
#include "ns3/ptr.h"                        // ns3::Ptr
#include "ns3/quagga-base-config.h"         // ns3::QuaggaBaseConfig
#include "ns3/telnet-wrapper.h"             // ns3::Telnet

namespace ns3
{


class QuaggaVtyHelper
{
private:

    typedef std::list<Telnet *> Telnets;
    Telnets m_telnets;  /**< Telnets object managed by this QuaggaVtyHelper. */

public:
    typedef std::list<std::string> Commands;

    /**
     * @brief Constructor.
     */

    QuaggaVtyHelper();

    /**
     * @brief Destructor.
     */

    ~QuaggaVtyHelper();

    /**
     * @brief Close gracefully all the Telnet managed by this helper.
     *   You should call this method only once the simulation ends.
     */

    void Close();

    /**
     * @brief Run a list of command on a group of Nodes at a given moment and
     *   for a given routing daemon.
     * @param nodes The nodes towards the telnet connections are issued.
     *    Note that you may also pass a single Ptr<Node> instance.
     * @param time The moment when the telnet connections are issued.
     * @param daemonName The name of the queried daemon. Expected values are:
     *    "zebra", "osfpd", "ospf6d", "bgpd", "ripd", "ripngd".
     * @param commands The sequence of commands to pass to telnet just after
     *   authentication in normal user.
     * @param enable Pass true if the commands require admin privileges.
     */

    void AddCommands (
        NodeContainer & nodes,
        const ns3::Time & time,
        const std::string & daemonName,
        const Commands & commands,
        bool enable = false
    );

private:

    /**
     * @brief Retrieve the *Config object related to a given daemon name.
     * @param daemonName The name of the queried daemon. Expected values:
     *    "zebra", "osfpd", "ospf6d", "bgpd", "ripd", "ripngd".
     * @param node A Node instance.
     * @return A ns3::Ptr< ns3::QuaggaBaseConfig > toward the corresponding
     *    *Config object. This Ptr may be null if this routing daemon is not
     *    installed on the Node.
     */

    static Ptr<QuaggaBaseConfig> GetConfig (
        const std::string & daemonName,
        Ptr<Node> node
    );
};

} // namespace ns3

#endif
