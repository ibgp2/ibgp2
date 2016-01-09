/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015
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

#ifndef OSPF_DISTRIBUTE_LIST
#define OSPF_DISTRIBUTE_LIST

#include <string>                       // std::string

#include "ns3/quagga-redistribute.h"    // REDISTRIBUTE_*
#include "ns3/quagga-distribute-list.h" // ns3::QuaggaDistributeList

namespace ns3 {

class OspfDistributeList :
    public QuaggaDistributeList
{
private:
    uint8_t m_distribute;   /**< Indicates on which kind of route this distribute-list applies. */
public:

    /**
     * @brief Constructor.
     * @param filterName Name of the distribute list.
     * @param direction IN | OUT
     * @param distribute Kind of routes on which the distribute-list applies.
     */

    OspfDistributeList ( const std::string& filterName, const QuaggaDirection& direction, uint8_t distribute );

    /**
     * @brief Retrieve the kind of routes on which the distribute-list applies.
     * @return A REDISTRIBUTE_* constant.
     */

    uint8_t GetDistribute() const;

    /**
     * @brief Set the kind of routes on which the distribute-list applies.
     * @param distribute A REDISTRIBUTE_* constant. Valid values are:
     * REDISTRIBUTE_KERNEL, REDISTRIBUTE_CONNECTED, REDISTRIBUTE_STATIC,
     * REDISTRIBUTE_RIP, REDISTRIBUTE_BGP.
     */

    void SetDistribute(uint8_t distribute);

    /**
     * @brief Write this OspfDistributeList in an output stream.
     * @param os The output stream.
     */

    virtual void Print ( std::ostream & os ) const;
};

/**
 * @brief Compare two OspfDistributeList instances.
 * @param x An OspfDistributeList instance.
 * @param y An OspfDistributeList instance.
 * @return true iif x preceeds y.
 */

bool operator < (const OspfDistributeList & x, const OspfDistributeList & y);

} // namespace ns3

#endif
