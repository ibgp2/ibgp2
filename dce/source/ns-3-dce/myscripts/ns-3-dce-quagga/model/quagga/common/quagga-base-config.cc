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

#include "quagga-base-config.h"

#include <fstream>              // std::ofstream
#include <sstream>              // std::ostringstream

#include "ns3/assert.h"         // NS_ASSERT
#include "ns3/log.h"            // NS_LOG_COMPONENT_DEFINE
#include "ns3/quagga-fs.h"      // QuaggaFs::*

NS_LOG_COMPONENT_DEFINE ( "QuaggaBaseConfig" );

namespace ns3 {


    QuaggaBaseConfig::QuaggaBaseConfig (
        const std::string & protocolName,
        const std::string & daemonName,
        uint16_t vtyPort,
        const std::string & hostname,
        const std::string & password,
        const std::string & passwordEnable,
        bool debug
    ) :
        m_protocolName ( protocolName ),
        m_daemonName ( daemonName ),
        m_vtyPort ( vtyPort ),
        m_hostname ( hostname ),
        m_password ( password ),
        m_passwordEnable( passwordEnable ),
        m_debug ( debug ),
        m_startTime ( Seconds ( 0 ) ) {
        this->SetLogFilename ( MakeDefaultLogFilename ( daemonName ) );
        this->SetConfigFilename ( MakeDefaultConfigFilename ( daemonName ) );
        this->SetPidFilename ( MakeDefaultPidFilename ( daemonName ) );
    }

    uint16_t QuaggaBaseConfig::GetVtyPort() const {
        return this->m_vtyPort;
    }

    void QuaggaBaseConfig::SetVtyPort ( uint16_t port ) {
        this->m_vtyPort = port;
    }

    const std::string & QuaggaBaseConfig::GetDaemonName() const {
        return this->m_daemonName;
    }

    const std::string & QuaggaBaseConfig::GetProtocolName() const {
        return this->m_protocolName;
    }

    std::string QuaggaBaseConfig::MakeDefaultLogFilename ( const std::string & daemonName ) {
        std::ostringstream oss;
        oss << "/var/log/" << daemonName << ".log";
        std::string ret = oss.str();
        return ret;
    }

    std::string QuaggaBaseConfig::MakeDefaultConfigFilename ( const std::string & daemonName ) {
        std::ostringstream oss;
        oss << "/etc/" << daemonName << ".conf";
        std::string ret = oss.str();
        return ret;
    }

    std::string QuaggaBaseConfig::MakeDefaultPidFilename ( const std::string & daemonName ) {
        std::ostringstream oss;
        oss << "/var/run/" << daemonName << ".pid";
        std::string ret = oss.str();
        return ret;
    }

    const ns3::Time& QuaggaBaseConfig::GetStartTime() const {
        return this->m_startTime;
    }
    void QuaggaBaseConfig::SetStartTime ( const ns3::Time& time ) {
        this->m_startTime = time;
    }

    const std::string & QuaggaBaseConfig::GetConfigFilename() const {
        return this->m_configFilename;
    }

    void QuaggaBaseConfig::SetConfigFilename ( const std::string & filename ) {
        this->m_configFilename = filename;
    }

    const std::string & QuaggaBaseConfig::GetLogFilename() const {
        return this->m_logFilename;
    }

    void QuaggaBaseConfig::SetLogFilename ( const std::string & filename ) {
        this->m_logFilename = filename;
    }

    const std::string & QuaggaBaseConfig::GetPidFilename() const {
        return this->m_pidFilename;
    }

    void QuaggaBaseConfig::SetPidFilename ( const std::string & filename ) {
        this->m_pidFilename = filename;
    }

    bool QuaggaBaseConfig::GetDebug() const {
        return this->m_debug;
    }

    void QuaggaBaseConfig::SetDebug ( bool debug ) {
        this->m_debug = debug;
    }

    void QuaggaBaseConfig::SetDebugCommand ( const std::string & command, bool newState ) {
        if ( newState ) {
            this->m_debugCommands.insert ( command );
        } else {
            this->m_debugCommands.erase ( command );
        }
    }

    const std::string & QuaggaBaseConfig::GetHostname() const {
        return this->m_hostname;
    }

    void QuaggaBaseConfig::SetHostname ( const std::string & hostname ) {
        this->m_hostname = hostname;
    }

    const std::string& QuaggaBaseConfig::GetPassword() const {
        return this->m_password;
    }

    void QuaggaBaseConfig::SetPassword ( const std::string & password ) {
        this->m_password = password;
    }

    const std::string& QuaggaBaseConfig::GetPasswordEnable() const {
        return this->m_passwordEnable;
    }

    void QuaggaBaseConfig::SetPasswordEnable ( const std::string & password ) {
        this->m_passwordEnable = password;
    }

    void QuaggaBaseConfig::AddPrefixList ( const PrefixList& prefixList ) {
        const std::string & name = prefixList.GetName();
        NS_ASSERT ( name != "" );
        this->m_prefixLists[name] = prefixList;
    }


    PrefixList& QuaggaBaseConfig::GetPrefixList ( const std::string& name ) {
        PrefixLists::iterator it = this->m_prefixLists.find ( name );
        if ( it == this->m_prefixLists.end() ) {
            throw std::runtime_error ( "QuaggaBaseConfig::GetPrefixList(): key " + name + " not found" );
        }
        return it->second;
    }

    const PrefixList& QuaggaBaseConfig::GetPrefixList ( const std::string& name ) const {
        PrefixLists::const_iterator it = this->m_prefixLists.find ( name );
        if ( it == this->m_prefixLists.end() ) {
            throw std::runtime_error ( "QuaggaBaseConfig::GetPrefixList(): key "  + name + " not found" );
        }
        return it->second;
    }

    AccessList& QuaggaBaseConfig::GetAccessList ( const std::string& name ) {
        AccessLists::iterator it = this->m_accessLists.find ( name );
        if ( it == this->m_accessLists.end() ) {
            throw std::runtime_error ( "QuaggaBaseConfig::GetAccessList(): key " + name + " not found" );
        }
        return it->second;
    }

    const AccessList& QuaggaBaseConfig::GetAccessList ( const std::string& name ) const {
        AccessLists::const_iterator it = this->m_accessLists.find ( name );
        if ( it == this->m_accessLists.end() ) {
            throw std::runtime_error ( "QuaggaBaseConfig::GetAccessList(): key " + name + " not found" );
        }
        return it->second;
    }

    void QuaggaBaseConfig::AddAccessList ( const ns3::AccessList& accessList ) {
        const std::string & name = accessList.GetName();
        NS_ASSERT ( name != "" );
        this->m_accessLists[name] = accessList;
    }

    void QuaggaBaseConfig::PrintBegin ( std::ostream & os ) const {

        os << "hostname " << this->GetHostname() << std::endl;

        // Passwords

        if (this->GetPassword().size()) {
           os << "password " << this->GetPassword() << std::endl;
        }

        if (this->GetPasswordEnable().size()) {
            os << "enable password " << this->GetPasswordEnable() << std::endl;
        }

        // Log

        os << "log file " << this->GetLogFilename() << " debugging" << std::endl
           << "no log syslog" << std::endl
           << "!" << std::endl;

        // Debug section

        if ( this->GetDebug() && this->m_debugCommands.size() > 0 ) {
            std::set<std::string>::const_iterator
            sit ( this->m_debugCommands.begin() ),
                send ( this->m_debugCommands.end() );
            for ( ; sit != send; ++sit ) {
                const std::string & command = *sit;
                os << "debug " << this->GetProtocolName() << " " << command << std::endl;
            }
            os << "!" << std::endl;
        }
    }

    void QuaggaBaseConfig::Print ( std::ostream & os ) const { // should be virtual pure
        this->PrintBegin ( os );
    }

    void QuaggaBaseConfig::PrintEnd ( std::ostream & os ) const {
        // access-list
        for ( AccessLists::const_iterator it = this->m_accessLists.begin(); it != this->m_accessLists.end(); ++it ) {
            const AccessList & accessList = it->second;
            os << accessList;
        }

        // prefix-list
        for ( PrefixLists::const_iterator it = this->m_prefixLists.begin(); it != this->m_prefixLists.end(); ++it ) {
            const PrefixList & prefixList = it->second;
            os << prefixList;
        }
    }

    std::ostream& operator<< ( std::ostream& os, const QuaggaBaseConfig& config ) {
        config.Print ( os );
        return os;
    }

    bool QuaggaBaseConfig::WriteConfigFile ( Ptr<Node> node ) const {
        bool ret = true;
        const std::string realFilename = QuaggaFs::GetRootDirectory ( node ) +  this->GetConfigFilename();

        std::ofstream conf ( realFilename );
        if ( conf ) {
            this->Print ( conf );
            conf.close ();
        } else {
            ret = false;
            NS_LOG_WARN ( "Can't write [" << realFilename << "]" );
        }

        return ret;
    }

    bool QuaggaBaseConfig::CreateDirectories ( Ptr< Node > node ) const {
        bool ret = true;
        std::string realFilename;

        // Create the directory containing the configuration file
        realFilename = QuaggaFs::GetRootDirectory ( node ) +  this->GetConfigFilename();
        ret &= QuaggaFs::mkdir ( QuaggaFs::dirname ( realFilename ) );

        // Create the directory containing the pid file
        realFilename = QuaggaFs::GetRootDirectory ( node ) +  this->GetPidFilename();
        ret &= QuaggaFs::mkdir ( QuaggaFs::dirname ( realFilename ) );

        // Quagga will create by itself the directory containing the log file.
        return ret;
    }

} // end namespace ns3
