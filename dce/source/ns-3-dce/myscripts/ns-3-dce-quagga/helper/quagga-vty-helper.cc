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
 * Original author:
 *   Hajime Tazaki <tazaki@nict.go.jp>
 *
 * Modified by:
 *   Marc-Olivier Buob  <marcolivier.buob@orange.fr>
 */

#include "quagga-vty-helper.h"

#include <sstream>                      // std::ostringstream

#include "ns3/names.h"                  // ns3::Names
#include "ns3/log.h"                    // NS_LOG_COMPONENT_DEFINE

#include "ns3/zebra-config.h"           // ns3::ZebraConfig
#include "ns3/bgp-config.h"             // ns3::BgpConfig
#include "ns3/ospf-config.h"            // ns3::OspfConfig
#include "ns3/ospf6-config.h"           // ns3::Ospf6Config
#include "ns3/rip-config.h"             // ns3::RipConfig
#include "ns3/ripng-config.h"           // ns3::RipngConfig

NS_LOG_COMPONENT_DEFINE ("QuaggaVtyHelper");

namespace ns3 {


QuaggaVtyHelper::QuaggaVtyHelper() {}

QuaggaVtyHelper::~QuaggaVtyHelper() {
    this->Close();
}

void QuaggaVtyHelper::Close() {
    for (Telnets::iterator it = this->m_telnets.begin(); it != this->m_telnets.end(); ++it) {
        Telnet * telnet = *it;
        telnet->Close();
        delete telnet;
    }

    // This will avoid to keep invalid pointers
    this->m_telnets.resize (0);
}

void QuaggaVtyHelper::AddCommands (
    NodeContainer & nodes,
    const Time & time,
    const std::string & daemonName,
    const ns3::QuaggaVtyHelper::Commands & commands,
    bool enable
) {
    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it) {
        Ptr<Node> node = *it;

        // Retrieve from the configuration of the daemon information
        // needed to connect via vty.
        Ptr<QuaggaBaseConfig> conf = QuaggaVtyHelper::GetConfig (daemonName, node);
        uint16_t port = conf->GetVtyPort();
        const std::string & password = conf->GetPassword();

        // Create a Telnet object, sinked to a dedicated output file.
        const std::string & nodeName = Names::FindName (node);
        Telnet * telnet = new Telnet (node, port, daemonName + "_" + nodeName + ".txt", time);

        // Authentication
        if (password.size()) {
            telnet->AppendCommand (password);
        }

        if (enable) {
            const std::string & passwordEnable = conf->GetPasswordEnable();

            if (passwordEnable.size()) {
                telnet->AppendCommand (passwordEnable);
            }
        }

        // Commands
        for (Commands::const_iterator it2 = commands.begin(); it2 != commands.end(); ++it2) {
            telnet->AppendCommand (*it2);
        }

        telnet->AppendCommand ("quit");

        // Register this Telnet object to close it gracefully
        // once the simulation ends.
        this->m_telnets.push_back (telnet);
    }
}

Ptr< QuaggaBaseConfig > QuaggaVtyHelper::GetConfig (
    const std::string & daemonName,
    Ptr<Node> node
) {
    Ptr<QuaggaBaseConfig> conf;

    if (daemonName == "zebra") {
        conf = node->GetObject<ZebraConfig>();
    } else if (daemonName == "bgpd") {
        conf = node->GetObject<BgpConfig>();
    } else if (daemonName == "ospfd") {
        conf = node->GetObject<OspfConfig>();
    } else if (daemonName == "ospf6d") {
        conf = node->GetObject<Ospf6Config>();
    } else if (daemonName == "ripd") {
        conf = node->GetObject<RipConfig>();
    } else if (daemonName == "ripngd") {
        conf = node->GetObject<RipngConfig>();
    } else {
        std::ostringstream oss;
        oss << "QuaggaVtyHelper::GetConfig(): Invalid daemon name "
            << daemonName << std::endl;
        throw std::runtime_error (oss.str());
    }

    return conf;
}

} // namespace ns3
