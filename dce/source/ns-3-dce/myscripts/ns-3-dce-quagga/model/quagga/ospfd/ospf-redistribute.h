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

#ifndef OSPF_REDISTRIBUTE_H
#define OSPF_REDISTRIBUTE_H

#include <cstdint>                      // uint16_t
#include <ostream>                      // std::ostream
#include <string>                       // std::string
#include "ns3/quagga-redistribute.h"    // REDISTRIBUTE_* constants

namespace ns3
{


class OspfRedistribute
{
private:
    uint8_t     m_from;          /**< Any REDISTRIBUTE_* value except REDISTRIBUTE_OSPF. */
    uint8_t     m_metricType;    /**< 1 or 2. Ignored otherwise */
    uint32_t    m_metric;        /**< OSPF metric, encoded in 32 bits to exceed max cost that might be set in the AS. */
    std::string m_routeMap;      /**< Route-map name, ignored if empty. */
public:

    /**
     * @brief Constructor.
     */

    OspfRedistribute();

    /**
     * @brief Constructor.
     * @param from A mask made of REDISTRIBUTE_* constants
     * @param metricType 1 or 2. Ignored otherwise
     * @param metric Metric set while redistributing the route in OSPF.
     * @param routeMap Route-map name, ignored if empty.
     */

    OspfRedistribute (
        uint8_t  from,
        uint8_t  metricType = 0,
        uint32_t metric = 0,
        const std::string & routeMap = ""
    );

    /**
     * @brief Retrieve the mask made of REDISTRIBUTE_* constants.
     * @return The mask.
     */

    uint8_t GetFrom() const;

    /**
     * @brief Return the metric type.
     * @return The metric type.
     */

    uint8_t GetMetricType() const;

    /**
     * @brief Accessor to the metric.
     * @return The metric.
     */

    uint32_t GetMetric() const;

    /**
     * @brief ...
     * @return const std::string&
     */

    const std::string & GetRouteMap() const;
};

/**
 * @brief Write an OspfRedistribute instance in a stream.
 * @param out The output stream
 * @param redistribute An OspfRedistribute instance
 * @return The output stream
 */

std::ostream & operator << (
    std::ostream & out,
    const OspfRedistribute & redistribute
);

/**
 * @brief Compare two OspfRedistribute
 * @param x An OspfRedistribute instance
 * @param y An OspfRedistribute instance
 * @return bool
 */

bool operator < (const OspfRedistribute & x, const OspfRedistribute & y );

} // end namespace ns3

#endif
