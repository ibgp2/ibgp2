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

#include "quagga-fs.h"

#include <sstream>                      // std::ostringstream
#include <sys/stat.h>                   // ::mkdir
#include <sys/types.h>                  // ::mkdir

#include "ns3/log.h" // NS_LOG_COMPONENT_DEFINE

NS_LOG_COMPONENT_DEFINE ( "QuaggaFs" );

namespace ns3 {
    namespace QuaggaFs {

        char GetSep() {
            return '/';
        }

        std::string GetRootDirectory ( Ptr< Node > node ) {
            std::string ret;
            std::ostringstream oss;
            oss << "files-" << node->GetId();
            ret = oss.str();
            return ret;
        }

        bool mkdir ( const char* directory ) {
            bool ret = true;
            char sep = GetSep();
            const char *pc = directory;
            const char *begin = directory;
            bool isSep = false;

            for ( pc = directory; *pc ; *pc++ ) {
                if ( isSep = ( *pc == sep ) ) {
                    std::string dir ( begin, pc + 1 );
                   ::mkdir ( dir.c_str (), S_IRWXU | S_IRWXG );
                }
            }

            std::string dir(directory);
            if (dir.back() != QuaggaFs::GetSep()) dir += GetSep();

            ret &= ::mkdir ( dir.c_str(), S_IRWXU | S_IRWXG );
            return ret;
        }

        bool mkdir ( const std::string& directory ) {
            return mkdir ( directory.c_str() );
        }

        std::string dirname ( const char* path ) {
            const char * begin = path;
            const char * end = NULL;
            for ( const char * pc = begin; *pc; pc++ ) {
                if ( *pc == GetSep() && * ( pc + 1 ) ) {
                    end = pc;
                }
            }

            if ( !end ) {
                return std::string ( "." );
            }
            if ( begin == end ) {
                return std::string ( begin, begin + 1 );    // return root dir
            }
            return std::string ( begin, end );
        }

        std::string dirname ( const std::string& path ) {
            return dirname ( path.c_str() );
        }

    } // namespace QuaggaFs
} // namespace ns3
