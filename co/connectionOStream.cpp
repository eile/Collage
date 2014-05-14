
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

namespace co
{
namespace detail
{
class ConnectionOStream
{
public:
    /** Connections to the receivers */
    Connections connections;
};
}

ConnectionOStream::ConnectionOStream( const bool save )
    : DataOStream( save )
    , _impl( new detail::ConnectionOStream )
{}

ConnectionOStream::ConnectionOStream( ConnectionOStream& rhs )
    : DataOStream( rhs )
    , _impl( new detail::ConnectionOStream( *rhs._impl ))
{
    setup( rhs.getConnections( ));
    rhs.close();
}

ConnectionOStream::~ConnectionOStream()
{
    delete _impl;
}

void ConnectionOStream::setup( const Nodes& receivers, const bool useMulticast )
{
    _impl->connections = gatherConnections( receivers, useMulticast );
}

void ConnectionOStream::setup( const Connections& connections )
{
    _impl->connections = connections;
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

void ConnectionOStream::close()
{
    DataOStream::close();
    _impl->connections.clear();
}

void ConnectionOStream::resendData( const Nodes& receivers,
                                    const bool useMulticast )
{
    setup( receivers, useMulticast );
    reemitData();
    clear();
}

void ConnectionOStream::emitData( const CompressorResult& data,
                                  const bool last )
{
    if( _impl->connections.empty( ))
        return;

    if( data.getSize() == 0 && !last )
        return;

    sendData( data, last );
}

const CompressorResult& ConnectionOStream::compress( void* data,
                                                     const uint64_t size,
                                                     const State newState )
{
    // OPT: delay compression for buffered streams which send full instance
    // during map of slaves
    if( _impl->connections.empty( ))
        return DataOStream::compress( data, size, STATE_DONT_COMPRESS );

    return DataOStream::compress( data, size, newState );
}

}
