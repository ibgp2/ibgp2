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

#ifndef PREFIX_LIST_H
#define PREFIX_LIST_H

#include <cstdint>              // std::uint*_t
#include <ostream>              // std::ostream
#include <string>               // std::string
#include <map>                  // std::map

#include "ns3/ipv4-prefix.h"    // ns3::Ipv4Prefix

namespace ns3
{

enum PrefixListOperator {
    EQ,
    LE,
    GE
};

enum PrefixListAction {
    DENY,
    PERMIT
};

/**
 * @brief A PrefixListElement correspond to a rule used in a a PrefixList.
 */

class PrefixListElement
{
private:
    PrefixListAction    m_action;       /**< Type of filter: false <=> deny ; true <=> permit. */
    Ipv4Prefix          m_prefixV4;     /**< IPv4 Prefix corresponding to this access list. */
    PrefixListOperator  m_operator;     /**< A mask indicating whether the PrefixListElement is applied to any prefix, to le prefix, to ge prefix (see PREFIX_LIST_MASK_*). */
    uint8_t             m_prefixLength; /**< specifies prefix length. The prefix list will be applied if the prefix length is less than or equal to the le prefix length. */
    uint32_t            m_seq;          /**< Sequence number. */

public:

    /**
     * @brief Default constructor.
     */

    PrefixListElement();

    /**
     * @brief Constructor.
     * @param action Action performed by the filter.
     * @param prefix The prefix related to this rule. If the prefix is any, you may
     *   pass Ipv4Prefix::Any().
     * @param seq The sequence number.
     */

    PrefixListElement ( const PrefixListAction & action, const Ipv4Prefix & prefix, uint32_t seq = 0 );

    /**
     * @brief Constructor.
     * @param action Action performed by the filter.
     * @param prefix The prefix related to this rule.
     * @param op Pass GE (greater or equal) or LE (less or equal).
     * @param prefixLength The prefix length related to "LE" or "GE" (>0)
     * @param seq The sequence number.
     */

    PrefixListElement ( const PrefixListAction & action, const Ipv4Prefix & prefix, const PrefixListOperator & op, uint8_t prefixLength, uint32_t seq = 0 );

    /**
     * @brief Copy constructor.
     * @param permit Type of filter: false <=> deny ; true <=> permit
     * @param prefix The prefix related to this rule.
     */

    PrefixListElement ( const PrefixListElement & x );

    /**
     * @brief Destructor.
     */

    virtual ~PrefixListElement();

    /**
     * @brief Retrieve the prefix related to this PrefixListElement.
     * @return The corresponding prefix.
     */

    const Ipv4Prefix & GetPrefix() const;

    /**
     * @brief Set the prefix related to this PrefixListElement.
     * @param prefixV4 The prefix assigned to this PrefixListElement.
     */

    void SetPrefix ( const Ipv4Prefix & prefixV4 );

    /**
     * @brief Test whether the Ipv4Prefix embedded in this filter is 0/0.
     * @returns true if the filter is related to any IP address.
     */

    bool IsAny() const;

    /**
     * @brief Retrieve the prefix length related to this PrefixListElement.
     * @return The corresponding prefix length.
     */

    uint8_t GetPrefixLength() const;

    /**
     * @brief Set the prefix length related to this PrefixListElement.
     * @param prefixLength The corresponding prefix length.
     */

    void SetPrefixLength ( uint8_t prefixLength );

    /**
     * @brief Retrieve the action of this filter.
     * @return The corresponding Action
     */

    const PrefixListAction &  GetAction() const;

    /**
     * @brief Set the action of this filter.
     * @param action The new action.
     */

    void SetAction ( const PrefixListAction & action);

    /**
     * @brief Retrieve the sequence number of this PrefixListElement.
     * @return The sequence number.
     */

    uint32_t GetSeq() const;

    /**
     * @brief Retrieve the sequence number of this PrefixListElement.
     * @param The sequence number. If you pass seq = 0, the seq instruction will be
     *   omitted when calling Print.
     */

    void SetSeq ( uint32_t seq );

    /**
     * @brief Retrieve the operator related to this PrefixListElement.
     * @return The corresponding operator
     */

    const PrefixListOperator & GetOperator() const;

    /**
     * @brief Set the operator related to this PrefixElement.
     * @param op The new operator.
     * @param prefixLength The new prefix length. This is only relevant if op == LE or op == GE.
     */

    void SetOperator ( const PrefixListOperator & op, uint8_t prefixLength = 0 );

    /**
     * @brief Write this PrefixListElement in an output stream.
     * @param os The output stream.
     */

    virtual void Print ( std::ostream & os ) const;
};

/**
 * @brief Write this PrefixListElement in an output stream.
 * @param os The output stream.
 * @param prefixListElement The PrefixListElement to print.
 * @return The updated output stream.
 */

std::ostream & operator << ( std::ostream & os, const PrefixListElement & prefixListElement );

/**
 * @brief Compare two PrefixListElement.
 * @param x An PrefixListElement instance.
 * @param y An PrefixListElement instance.
 * @return The updated output stream.
 */

bool operator < ( const PrefixListElement & x, const PrefixListElement & y );


/**
 * @brief A PrefixList is a filter which can be triggered according to the Ipv4Prefix
 * carried by a route (in any quagga daemon).
 */

class PrefixList
{
private:
    typedef std::map<uint32_t, PrefixListElement> Elements;
    Elements        m_elements; /**< The element of this PrefixList (Read only). */
    std::string     m_name;     /**< The name identifying the PrefixList. */
    uint32_t        m_lastSeq;  /**< The last used sequence number. */
public:

    /**
     * @brief Constructor.
     * @param name The name identifying the PrefixList.
     */

    PrefixList ( const std::string & name = "" );

    /**
     * @brief Destructor.
     */

    virtual ~PrefixList();

    /**
     * @brief Retrieve the name assigned to this PrefixList.
     * @return The name identifying the PrefixList.
     */

    const std::string & GetName() const;

    /**
     * @brief Add a rule in this PrefixList. If the seq number is omitted, we assign
     *   a seq number to run this rule after the rules already installed.
     * @param rule The PrefixListElement that must be added.
     */

    void Add ( const PrefixListElement & rule );

    /**
     * @brief Write this PrefixList in an output stream.
     * @param out The output stream.
     * @return The updated output stream.
     */

    virtual std::ostream & Print ( std::ostream & out ) const;
};

/**
 * @brief Write this PrefixList in an output stream.
 * @param out The output stream.
 * @param prefixList The PrefixList to print.
 * @return The updated output stream.
 */

std::ostream & operator << ( std::ostream & out, const PrefixList & prefixList );

/**
 * @brief Compare two PrefixList.
 * @param x An PrefixList instance.
 * @param y An PrefixList instance.
 * @return The updated output stream.
 */

bool operator < ( const PrefixList & x, const PrefixList & y );

} // namespace ns3

#endif
