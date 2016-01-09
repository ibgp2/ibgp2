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


#include "quagga-direction.h"

#include "ns3/log.h" // NS_LOG_COMPONENT_DEFINE

NS_LOG_COMPONENT_DEFINE ( "QuaggaDirection" );

namespace ns3 {

std::ostream& operator<< ( std::ostream& os, const ns3::QuaggaDirection& direction ) {
    switch ( direction ) {
        case IN:
            os << "in";
            break;
        case OUT:
            os << "out";
            break;
        default:
            os << "???";
            break;
    }
            return os;
}

} // namespace ns3
