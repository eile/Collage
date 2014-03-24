
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2012-2014, Stefan.Eilemann@epfl.ch
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

#include "objectDataOCommand.h"

#include "buffer.h"
#include "compressorResult.h"
#include "objectDataICommand.h"
#include <lunchbox/plugins/compressorTypes.h>
#include <boost/foreach.hpp>

namespace co
{

namespace detail
{

class ObjectDataOCommand
{
public:
    ObjectDataOCommand( co::DataOStream* stream_, const CompressorResult& data_)
        : data( data_ )
        , stream( stream_ )
    {}

    ObjectDataOCommand( const ObjectDataOCommand& rhs )
        : data( rhs.data )
        , stream( rhs.stream )
    {}

    const CompressorResult& data;
    co::DataOStream* stream;
};

}

ObjectDataOCommand::ObjectDataOCommand( const Connections& receivers,
                                        const uint32_t cmd, const uint32_t type,
                                        const UUID& id,
                                        const uint32_t instanceID,
                                        const uint128_t& version,
                                        const CompressorResult& data,
                                        const uint32_t sequence,
                                        const bool isLast,
                                        DataOStream* stream )
    : ObjectOCommand( receivers, cmd, type, id, instanceID )
    , _impl( new detail::ObjectDataOCommand( stream, data ))
{
    *this << version << data.rawSize << sequence << isLast
          << data.compressor << uint32_t( data.chunks.size( ));
}

ObjectDataOCommand::ObjectDataOCommand( const ObjectDataOCommand& rhs )
    : ObjectOCommand( rhs )
    , _impl( new detail::ObjectDataOCommand( *rhs._impl ))
{
}

ObjectDataOCommand::~ObjectDataOCommand()
{
    if( _impl->stream && _impl->data.rawSize > 0 )
        send( _impl->data );

    delete _impl;
}

ObjectDataICommand ObjectDataOCommand::_getCommand( LocalNodePtr node )
{
    lunchbox::Bufferb& outBuffer = getBuffer();
    uint8_t* bytes = outBuffer.getData();
    reinterpret_cast< uint64_t* >( bytes )[ 0 ] = outBuffer.getSize();

    BufferPtr inBuffer = node->allocBuffer( outBuffer.getSize( ));
    inBuffer->swap( outBuffer );
    return ObjectDataICommand( node, node, inBuffer, false );
}

}
