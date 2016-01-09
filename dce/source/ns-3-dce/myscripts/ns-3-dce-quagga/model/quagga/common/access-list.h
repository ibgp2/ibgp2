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

#ifndef ACCESS_LIST_H
#define ACCESS_LIST_H

#include <list>                 // std::list
#include <ostream>              // std::ostream
#include <string>               // std::string

#include "ns3/ipv4-prefix.h"    // ns3::Ipv4Prefix

namespace ns3
{

//--------------------------------------------------------------------------------------
// AccessListElement
//--------------------------------------------------------------------------------------

class AccessListElement
{
private:
    bool        m_permit;       /**< Type of filter: false <=> deny ; true <=> permit. */
    Ipv4Prefix  m_prefixV4;     /**< IPv4 Prefix corresponding to this access list. */

public:

    /**
     * @brief Constructor.
     */

    AccessListElement();

    /**
     * @brief Constructor
     * @param permit Type of filter: false <=> deny ; true <=> permit
     * @param prefix The prefix on which the AccessList applies.
     */

    AccessListElement ( bool permit, const Ipv4Prefix & prefix );

    /**
     * @brief Destructor.
     */

    virtual ~AccessListElement();

    /**
     * @brief Retrieve the prefix related to this AccessList.
     * @return The corresponding prefix.
     */

    const Ipv4Prefix & GetPrefix() const;

    /**
     * @brief Set the prefix related to this AccessList.
     * @param prefixV4 The prefix assigned to this AccessList.
     */

    void SetPrefix ( const Ipv4Prefix & prefixV4 );

    /**
     * @brief Retrieve the type of filter.
     * @return Type of filter: false <=> deny ; true <=> permit
     */

    bool GetPermit() const;

    /**
     * @brief Set the type of filter.
     * @param permit Type of filter: false <=> deny ; true <=> permit
     */

    void SetPermit ( bool permit );

    /**
     * @brief Write this AccessList in an output stream.
     * @param os The output stream.
     */

    virtual void Print(std::ostream & os) const;
};

/**
 * @brief Write this AccessList in an output stream.
 * @param os The output stream.
 * @param accessList The AccessList to print.
 * @return The updated output stream.
 */

std::ostream & operator << (std::ostream & os, const AccessListElement & AccessListElement);

//--------------------------------------------------------------------------------------
// AccessList
//--------------------------------------------------------------------------------------

class AccessList {
private:
    typedef std::list<AccessListElement> Elements;
    std::string m_name;         /**< Name of this AccessList (READ ONLY) */
    Elements    m_elements;     /**< Rules emmbeded in this AccessList */
public:

    /**
     * @brief Constructor.
     */

    AccessList();

    /**
     * @brief Destructor.
     */

    ~AccessList();

    /**
     * @brief Constructor.
     * @param name The name of this AccessList.
     */

    AccessList(const std::string & name);

    /**
     * @brief Append an AccessListElement to this AccessList.
     * @param element The AccessListElement.
     */

    void Add(const AccessListElement & element);

    /**
     * @brief Retrieve the name of this AccessList.
     * @return The corresponding name.
     */

    const std::string & GetName() const;

    /**
     * @brief Write this AccessList in an output stream.
     * @param os The output stream.
     */

    virtual void Print(std::ostream & os) const;
};

/**
 * @brief Write this AccessList in an output stream.
 * @param os The output stream.
 * @param accessList The AccessList to print.
 * @return The updated output stream.
 */

std::ostream & operator << (std::ostream & os, const AccessList & accessList);

/**
 * @brief Compare two AccessList according to their name.
 * @param x An AccessList.
 * @param y An AccessList.
 * @return true iif x preceeds y.
 */

bool operator < (const AccessList & x, const AccessList & y);

} // namespace ns3

#endif
