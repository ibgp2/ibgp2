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

#ifndef QUAGGA_DISTRIBUTE_LIST
#define QUAGGA_DISTRIBUTE_LIST

#include <ostream>  // std::ostream
#include <string>   // std::string

#include "ns3/quagga-direction.h"

namespace ns3
{

/**
 * \brief QuaggaDistributeList is the base class for the other *DistributeList classes.
 */

class QuaggaDistributeList
{
private:
    std::string         m_filterName;   /**< Name of the attached prefix-list or access-list. */
    QuaggaDirection     m_direction;    /**< Direction on which the distributed-list applies. */
public:

    /**
     * \brief Constructor.
     */

    QuaggaDistributeList();

    /**
     * \brief Constructor.
     * \param direction Direction (IN|OUT) on which the distributed-list applies.
     * \param filterName Name of the attached prefix-list or access-list.
     */

    QuaggaDistributeList ( const QuaggaDirection & direction, const std::string & filterName );

    /**
     * \brief Retrieve the name of the attached filter.
     * \returns The name of the attached prefix-list or access-list.
     */

    const std::string & GetFilterName() const;

    /**
     * \brief Set the name of the attached filter.
     * \param filterName The name of the attached prefix-list or access-list.
     */

    void SetFilterName ( const std::string & filterName );

    /**
     * \brief Retrieve the direction on which applies the attached filter.
     * \returns The corresponding direction.
     */

    const QuaggaDirection & GetDirection() const;

    /**
     * \brief Set the direction on which applies the attached filter.
     * \param direction The corresponding direction (IN|OUT).
     */

    void SetDirection ( const QuaggaDirection & direction );

    /**
     * \brief Write this OspfConfig in an output stream.
     * \param os The output stream.
     */

    virtual void Print ( std::ostream & os ) const = 0;
};

/**
 * \brief Write a QuaggaDistributeList instance in an output stream.
 * \param os The output stream.
 * \param distributeList The QuaggaDistributeList that must be written.
 * \return The output stream.
 */

std::ostream & operator << ( std::ostream & os, const QuaggaDistributeList & distributeList );

} // namespace ns3
#endif
