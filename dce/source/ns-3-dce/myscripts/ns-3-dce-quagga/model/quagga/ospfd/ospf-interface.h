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

#ifndef OSPF_INTERFACE_H
#define OSPF_INTERFACE_H

#include <cstdint>  // uint16_t
#include <ostream>  // std::ostream
#include <string>   // std::string

namespace ns3
{

typedef uint16_t OspfCost;

class OspfInterface
{
private:
    std::string m_name;                 // Read only
    uint16_t    m_helloInterval;        /**< Hello interval. <1-65535>, 0 means default value. */
    uint16_t    m_deadInterval;         /**< Dead interval. <1-65535>, 0 means default value.*/
    OspfCost    m_cost;                 /**< Ospf cost assigned to this interface. */
    uint16_t    m_retransmitInterval;   /**< Retransmit interval. <1-65535>, 0 means default value. */
    uint16_t    m_transmitDelay;        /**< Transmit delay. */

public:

    /**
     * @brief Constructor.
     */

    OspfInterface();

    /**
     * @brief Constructor.
     * @param name Name of the interface (for instance ns3-device0).
     * @param cost OSPF cost assigned to this interface.
     */

    OspfInterface ( const std::string & name, uint16_t cost );

    /**
     * @brief Accessor to the OspfInterface name.
     * @return The name of this OspfInterface
     */

    const std::string & GetName() const;

    /**
     * @brief Accessor to the OSPF cost of this OspfInterface.
     * @return The corresponding OspfCost.
     */

    OspfCost GetCost() const;

    /**
     * @brief Set the OSPF cost of this OspfInterface.
     * @param cost The new OSPF cost.
     */

    void SetCost ( uint16_t cost );

    /**
     * @brief Accessor to the Hello Interval of this OspfInterface.
     * @return The corresponding Hello Interval.
     */

    uint16_t GetHelloInterval() const;

    /**
     * @brief Set the Hello Interval of this OspfInterface.
     * @param interval The new Hello Interval.
     */

    void SetHelloInterval ( uint16_t interval );

    /**
     * @brief Accessor to the Dead Interval of this OspfInterface.
     * @return The corresponding Dead Interval.
     */

    uint16_t GetDeadInterval() const;

    /**
     * @brief Set the Dead Interval of this OspfInterface.
     * @param interval The new Dead Interval.
     */

    void SetDeadInterval ( uint16_t interval );

    /**
     * @brief Accessor to the Transmit delay of this OspfInterface.
     * @return The corresponding Transmit delay.
     */

    uint16_t GetTransmitDelay() const;

    /**
     * @brief Set the Dead Transmit delay of this OspfInterface.
     * @param delay The new Transmit delay.
     */

    void SetTransmitDelay ( uint16_t delay );

    /**
     * @brief Accessor to the Retransmit interval of this OspfInterface.
     * @return The corresponding Retransmit interval.
     */

    uint16_t GetRetransmitInterval() const;

    /**
     * @brief Set the Retransmit interval of this OspfInterface.
     * @param delay The new Retransmit interval.
     */

    void SetRetransmitInterval ( uint16_t interval );

    /**
     * @brief Write this OspfInterface in an output stream.
     * @param os The output stream.
     */

    virtual void Print(std::ostream & os) const;
};

/**
 * @brief Write an OspfInterface in an output stream.
 * @param os The output stream.
 * @param interface An OspfInterface.
 * @return The output stream.
 */

std::ostream & operator << ( std::ostream & os, const OspfInterface & interface );

} // end namespace ns3

#endif
