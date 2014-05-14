
/* Copyright (c) 2014, Stefan.Eilemann@epfl.ch
 *
 * This file is part of Collage <https://github.com/Eyescale/Collage>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "connectionOStream.h"

#include "compressorResult.h"
#include "connections.h"
#include <lunchbox/clock.h>

namespace co
{
namespace
{
#define INSTRUMENT
#ifdef INSTRUMENT
lunchbox::a_int32_t nBytesIn;
lunchbox::a_int32_t nBytesOut;
lunchbox::a_int32_t compressionTime;
#endif
}

namespace detail
{
class ConnectionOStream
{
public:
    /** Connections to the receivers */
    Connections connections;
};
}

ConnectionOStream::ConnectionOStream()
    : _impl( new detail::ConnectionOStream )
{}

ConnectionOStream::ConnectionOStream( ConnectionOStream& rhs )
    : DataOStream( rhs )
    , _impl( new detail::ConnectionOStream( *rhs._impl ))
{
    setup( rhs.getConnections( ));
    // disable send of rhs
    rhs.setup( Connections( ));
    rhs.disable();
}

ConnectionOStream::~ConnectionOStream()
{
    delete _impl;
}

void ConnectionOStream::setup( const Nodes& receivers )
{
    _impl->connections = gatherConnections( receivers );
}

void ConnectionOStream::setup( const Connections& connections )
{
    _impl->connections = connections;
}

void ConnectionOStream::setup( NodePtr node, const bool useMulticast )
{
    LBASSERT( _impl->connections.empty( ));
    _impl->connections.push_back( node->getConnection( useMulticast ));
}

void ConnectionOStream::clear()
{
    _impl->connections.clear();
}

void ConnectionOStream::reset()
{
    DataOStream::reset();
    _impl->connections.clear();
}

const Connections& ConnectionOStream::getConnections() const
{
    return _impl->connections;
}

void ConnectionOStream::disable()
{
    DataOStream::disable();
    _impl->connections.clear();
}

void ConnectionOStream::emit( void* ptr, const uint64_t size,
                              const State state, const bool last )
{
    if( _impl->connections.empty( ))
        return;

    if( size == 0 )
    {
        if( last ) // always send to finalize istream
            sendData( CompressorResult(), true );
        return;
    }

#ifdef INSTRUMENT
    nBytesIn += size;
    lunchbox::Clock clock;
#endif

    const CompressorResult& data = compress( ptr, size, state );

#ifdef INSTRUMENT
    compressionTime += uint32_t( clock.getTimef() * 1000.f );
    nBytesOut += data.getSize();
    if( compressionTime > 100000 )
        LBWARN << *this << std::endl;
#endif

    sendData( data, last );
}

std::ostream& operator << ( std::ostream& os,
                            const ConnectionOStream& cos LB_UNUSED )
{
#ifdef INSTRUMENT
    os << "ConnectionOStream "
       << "send " << nBytesOut << " of " << nBytesIn << " in "
       << compressionTime/1000 << "ms, saved "
       << int(( nBytesIn - nBytesOut ) / float( nBytesIn ) * 100.f ) << "%";

    nBytesIn = 0;
    nBytesOut = 0;
    compressionTime = 0;
    return os;
#else
    return os << "@" << (void*)&cos;
#endif
}

}
