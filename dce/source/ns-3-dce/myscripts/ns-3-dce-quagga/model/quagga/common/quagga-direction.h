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

#ifndef QUAGGA_DIRECTION
#define QUAGGA_DIRECTION

#include <ostream>  // std::ostream

namespace ns3
{

    //------------------------------------------------------------------------------------
    // QuaggaDirection
    //------------------------------------------------------------------------------------

    enum QuaggaDirection {
        IN,     /**< Applies on input routing messages. */
        OUT     /**< Applies on output routing messages. */
    };

    /**
     * @brief Write a QuaggaDirection instance in an output stream.
     * @param os The output stream.
     * @param direction The QuaggaDirection that must be written.
     * @return The output stream.
     */

    std::ostream & operator << (std::ostream & os, const QuaggaDirection & direction);

} // namespace ns3

#endif
