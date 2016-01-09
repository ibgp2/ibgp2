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

#include "ospf-redistribute.h"
#include "ns3/log.h"        // NS_LOG_*

NS_LOG_COMPONENT_DEFINE ( "OspfRedistribute" );

namespace ns3 {

    OspfRedistribute::OspfRedistribute() {}
    OspfRedistribute::OspfRedistribute ( uint8_t from, uint8_t metric_type, uint32_t metric, const std::string& route_map ) :
        m_from ( from ),
        m_metricType ( metric_type ),
        m_metric ( metric ),
        m_routeMap ( route_map )
    {}

    uint8_t OspfRedistribute::GetFrom() const {
        return this->m_from;
    }

    uint8_t OspfRedistribute::GetMetricType() const {
        return this->m_metricType;
    }

    uint32_t OspfRedistribute::GetMetric() const {
        return this->m_metric;
    }

    const std::string& OspfRedistribute::GetRouteMap() const {
        return this->m_routeMap;
    }

    std::ostream& operator<< ( std::ostream& out, const OspfRedistribute& redistribute ) {
        uint8_t from = redistribute.GetFrom();

        if ( from ) {
            out << "redistribute ";
            if ( from & REDISTRIBUTE_KERNEL ) {
                out << "kernel";
            } else if ( from & REDISTRIBUTE_CONNECTED ) {
                out << "connected";
            } else if ( from & REDISTRIBUTE_STATIC ) {
                out << "static";
            } else if ( from & REDISTRIBUTE_RIP ) {
                out << "rip";
            } else if ( from & REDISTRIBUTE_BGP ) {
                out << "bgp";
            }

            uint32_t metric_type = redistribute.GetMetricType();
            switch ( metric_type ) {
                case 1:
                case 2:
                    out << " metric-type " << metric_type;
                    break;
            }

            if ( redistribute.GetMetric() ) {
                out << " metric " << redistribute.GetMetric();
            }

            if ( redistribute.GetRouteMap() != "" ) {
                out << redistribute.GetRouteMap();
            }
        }

        return out;
    }
    bool operator< ( const OspfRedistribute& x, const OspfRedistribute& y ) {
        return x.GetFrom() < y.GetFrom();
    }
}
