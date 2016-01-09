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

#ifndef QUAGGA_FS_H
#define QUAGGA_FS_H

#include <string>       // std::string
#include "ns3/ptr.h"    // ns3::Ptr<>
#include "ns3/node.h"   // ns3::Node

namespace ns3
{

namespace QuaggaFs
{
/**
 * \return The path separator ('/' under linux).
 */

char GetSep();

/**
 * \brief Return the relative path of the directory storing DCE files..
 * \param node The corresponding node..
 * \return The path separator ('/' under linux).
 */

std::string GetRootDirectory ( Ptr<Node> node );

/**
 * \brief Create a directory (like mkdir -p).
 * \param directory The directory path (absolute or relative).
 * \return true iif successful.
 */

bool mkdir ( const char * directory );

/**
 * \brief Create a directory (like mkdir -p in shell).
 * \param directory The directory path (absolute or relative).
 * \return true iif successful.
 */

bool mkdir ( const std::string & directory );

/**
 * \brief Return the directory corresponding to a path (like dirname in shell).
 * \param path A path (absolute or relative).
 * \return The corresponding directory.
 */

 std::string dirname ( const char * path );

/**
 * \brief Return the directory corresponding to a path (like dirname in shell).
 * \param path A path (absolute or relative).
 * \return The corresponding directory.
 */

std::string dirname ( const std::string & path );

} // end namespace QuaggaFs
} // end namespace ns3

#endif
