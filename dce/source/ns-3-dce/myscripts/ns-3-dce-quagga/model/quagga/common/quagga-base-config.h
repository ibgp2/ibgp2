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

#ifndef BASE_CONFIG_H
#define BASE_CONFIG_H

#include <cstdint>              // uint16_t
#include <list>                 // std::list
#include <map>                  // std::map
#include <string>               // std::string

#include "ns3/access-list.h"    // ns3::AccessList
#include "ns3/node.h"           // ns3::Node
#include "ns3/nstime.h"         // ns3::Time
#include "ns3/object.h"         // ns3::Object
#include "ns3/prefix-list.h"    // ns3::PrefixList
#include "ns3/ptr.h"            // ns3::Ptr

#define DEFAULT_HOSTNAME        "zebra"
#define DEFAULT_PASSWORD        "zebra"
#define DEFAULT_PASSWORD_ENABLE ""

/**
 * \class BaseConfig.
 * Implements the basic functionnalities of a class managing a Quagga configuration file.
 * Each *Config class should inherits this calss
 * \sa BgpdConfig OspfConfig QuaggaConfig
 */

namespace ns3
{
class QuaggaBaseConfig :
    public Object
{
private:
    typedef std::map<std::string, PrefixList>   PrefixLists;
    typedef std::map<std::string, AccessList>   AccessLists;

    const std::string       m_protocolName;     // MUST BE SET IN CHILD CLASSES
    const std::string       m_daemonName;       // MUST BE SET IN CHILD CLASSES
    uint16_t                m_vtyPort;          // MUST BE SET IN CHILD CLASSES
    std::string             m_configFilename;   /**< Absolute path of the configuration file. */
    std::string             m_logFilename;      /**< Absolute path of the log file. */
    std::string             m_pidFilename;      /**< Absolute path of the pid file. */
    std::string             m_hostname;         /**< Hostname written in the configuration file. */
    std::string             m_password;         /**< Password used to authenticate to the daemon (normal mode). */
    std::string             m_passwordEnable;   /**< Password used to become root on the daemon (enable mode). */
    bool                    m_debug;            /**< Set to true to turn on Zebra's debug. */
    std::set<std::string>   m_debugCommands;    /**< Set of debug command. */
    ns3::Time               m_startTime;        /**< Time when the daemon is started. */
    PrefixLists             m_prefixLists;      /**< Contains the PrefixLists configured for this daemon. */
    AccessLists             m_accessLists;      /**< Contains the AccessLists configured for this daemon. */
public:

    /**
     * \brief Constructor.
     * \param protocolName The name of the protocol (this is the keyword corresponding
     *    to the daemon in the quagga configuration files, for instance "bgp").
     * \param daemonName The name of the daemon (executable), for instance "bgpd".
     * \param vtyPort The listening vty port related to this daemon.
     * \param hostname The name of the router.
     * \param password The password needed to access to this daemon.
     * \param debug Pass true to enable default debug instructions.
     */

    QuaggaBaseConfig (
        const std::string & protocolName,
        const std::string & daemonName,
        uint16_t vtyPort,
        const std::string & hostname = DEFAULT_HOSTNAME,
        const std::string & password = DEFAULT_PASSWORD,
        const std::string & passwordEnable = DEFAULT_PASSWORD_ENABLE,
        bool debug = true
    );

    /**
     * \brief Retrieve the listening vty port of the corresponding daemon.
     * \returns The vty port number.
     */

    uint16_t GetVtyPort() const;

    /**
     * \brief Set the listening vty port of the corresponding daemon.
     * \param port The vty port number.
     */

    void SetVtyPort(uint16_t port);

    /**
     * \brief Destructor
     */

    virtual ~QuaggaBaseConfig() {}

    /**
     * \return Return the name of this daemon
     */

    const std::string & GetDaemonName() const;

    /**
     * \return Return the name of the protocol (in the configuration file).
     */

    const std::string & GetProtocolName() const;

    /**
     * \brief Set the starting date of the daemon.
     * \param time start date.
     */

    void SetStartTime ( const ns3::Time & time);

    /**
     * \brief Retrieve the start time to run this daemon.
     * \returns The corresponding start time.
     */

    const ns3::Time & GetStartTime () const;

    /**
     * \brief Set the password needed to authenticate to the daemon.
     * \param password The new password.
     */

    void SetPassword ( const std::string & password );

    /**
     * \brief Retrieve the password needed to authenticate to the daemon.
     * \returns The corresponding password.
     */

    const std::string & GetPassword () const;

    /**
     * \brief Set the password needed to enable the root mode.
     * \param password The new password.
     */

    void SetPasswordEnable ( const std::string & password );

    /**
     * \brief Retrieve the password needed to enable the root mode.
     * \returns The corresponding password.
     */

    const std::string & GetPasswordEnable () const;

    /**
     * \brief Set the hostname that will be written in the configuration file.
     * \param hostname The new hostname.
     */

    void SetHostname ( const std::string & hostname );

    /**
     * \brief Retrieve the hostname that will be written in the configuration file.
     * \returns The corresponding hostname.
     */

    const std::string & GetHostname () const;

    /**
     * \brief Set the absolute path to the configuration file of the corresponding daemon.
     * \param filename The absolute path to the configuration file.
     */

    void SetConfigFilename ( const std::string & filename );

    /**
     * \brief Retrieve the absolute path of the configuration file of the daemon.
     * \return The absolute path of the configuration file of the daemon.
     */

    const std::string & GetConfigFilename () const;

    /**
     * \brief Set the absolute path of the log file of the daemon.
     * \param filename The absolute path of the configuration file of the daemon.
     */

    void SetLogFilename ( const std::string & filename );

    /**
     * \brief Retrieve the absolute path of the log file of the daemon.
     * \return The absolute path of the log file of the daemon.
     */

    const std::string & GetLogFilename() const;

    /**
     * \brief Set the absolute path of the pid file of the daemon.
     * \param filename The absolute path of the configuration file of the daemon.
     */

    void SetPidFilename ( const std::string & filename );

    /**
     * \brief Retrieve the absolute path of the pid file of the daemon.
     * \return The absolute path of the log file of the daemon.
     */

    const std::string & GetPidFilename() const;

    /**
     * \brief Enable or disable the debug instructions.
     * \param debug Pass true to enable the debug instruction, false otherwise.
     */

    void SetDebug ( bool debug = true );

    /**
     * \returns true iif the debug are enabled.
     */

    bool GetDebug() const;

    /**
     * \brief Add or remove a debug command to the configuration file.
     * \param command The command to add or to remove
     * \param newState Pass true to enable, false to disable.
     */

    void SetDebugCommand ( const std::string & command, bool newState = true );

    /**
     * \brief Write the configuration file of this daemon.
     * \param os The output stream.
     */

    virtual void Print ( std::ostream & os ) const;

    /**
     * \brief Write the beginning configuration file of this daemon.
     *  (hostname, password, debug instructions...)
     * \param os The output stream.
     */

    virtual void PrintBegin ( std::ostream & os ) const;

    /**
     * \brief Write the end configuration file of this daemon.
     *   (access-list, prefix-list, ...)
     * \param os The output stream.
     */

    virtual void PrintEnd ( std::ostream & os ) const;

    /**
     * \brief Prepare the directories required to run Quagga.
     * \param node The corresponding Node.
     * \returns true iif successful.
     */

    bool CreateDirectories ( Ptr<Node> node ) const;

    /**
     * \brief Write the configuration file of a router.
     *    The path of this file results from the DCE root directory
     *    assigned to the node and the result of GetConfigFilename().
     * \param node The corresponding Node.
     * \returns true iif successful.
     */

    bool WriteConfigFile ( Ptr<Node> node ) const;

    /**
     * \brief Set up a prefix-list for this daemon.
     * \param prefixList A PrefixList instance such as prefixList->GetName() != "".
     */

    void AddPrefixList(const PrefixList & prefixList);

    /**
     * \brief Set up a access-list for this daemon.
     * \param accessList An AccessList instance;
     */

    void AddAccessList(const AccessList & accessList);

    /**
     * @brief Retrieve a prefix-list configured on this daemon.
     * @param name The name of the prefix-list.
     * @throws std::runtime_error if not found.
     * @return The corresponding prefix-list if any/
     */

    PrefixList & GetPrefixList(const std::string & name);

    /**
     * @brief Retrieve a prefix-list configured on this daemon.
     * @param name The name of the prefix-list.
     * @throws std::runtime_error if not found.
     * @return The corresponding prefix-list if any/
     */

    const PrefixList & GetPrefixList(const std::string & name) const;

    /**
     * @brief Retrieve a prefix-list configured on this daemon.
     * @param name The name of the prefix-list.
     * @throws std::runtime_error if not found.
     * @return The corresponding prefix-list if any/
     */

    AccessList & GetAccessList(const std::string & name);

    /**
     * @brief Retrieve a prefix-list configured on this daemon.
     * @param name The name of the prefix-list.
     * @throws std::runtime_error if not found.
     * @return The corresponding prefix-list if any/
     */

    const AccessList & GetAccessList(const std::string & name) const;

private:
    /**
     * \returns Create a string containing the usual log filename for a given daemon.
     */

    static std::string MakeDefaultLogFilename ( const std::string & daemonName );

    /**
     * \returns Create a string containing the usual pid filename for a given daemon.
     */

    static std::string MakeDefaultPidFilename ( const std::string & daemonName );

    /**
     * \returns Create a string containing the usual configuration filename for a given daemon.
     */

    static std::string MakeDefaultConfigFilename ( const std::string & daemonName );

};

/**
 * \brief Write a BaseConfig instance in an output stream.
 * \param os The output stream.
 * \param config The QuaggaBaseConfig that must be written.
 * \return The output stream.
 */

std::ostream & operator << ( std::ostream & os, const QuaggaBaseConfig & config );

}

#endif
