
/* Copyright (c) 2012, Daniel Nachbaur <danielnachbaur@gmail.com>
 *               2013-2014, Stefan.Eilemann@epfl.ch
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

#include "oCommand.h"

#include "buffer.h"
#include "compressorResult.h"
#include "iCommand.h"

namespace co
{
namespace detail
{

class OCommand
{
public:
    OCommand( co::Dispatcher* const dispatcher_, LocalNodePtr localNode_ )
        : isLocked( false )
        , bodySize( 0 )
        , dispatcher( dispatcher_ )
        , localNode( localNode_ )
    {}

    bool isLocked;
    uint64_t bodySize;
    co::Dispatcher* const dispatcher;
    LocalNodePtr localNode;
};

}

OCommand::OCommand( const Connections& receivers, const uint32_t cmd,
                    const uint32_t type )
    : _impl( new detail::OCommand( 0, 0 ))
{
    setChunkSize( std::numeric_limits< uint64_t >::max( ));
    setup( receivers );
    _init( cmd, type );
}

OCommand::OCommand( Dispatcher* const dispatcher, LocalNodePtr localNode,
                    const uint32_t cmd, const uint32_t type )
    : _impl( new detail::OCommand( dispatcher, localNode ))
{
    setChunkSize( std::numeric_limits< uint64_t >::max( ));
    _init( cmd, type );
}

OCommand::OCommand( const OCommand& rhs )
    : _impl( new detail::OCommand( *rhs._impl ))
{
}

OCommand::~OCommand()
{
    disable();

    if( _impl->dispatcher )
    {
        LBASSERT( _impl->localNode );

        // #145 proper local command dispatch?
        LBASSERT( _impl->bodySize == 0 );
        const uint64_t size = getBuffer().getSize();
        BufferPtr buffer = _impl->localNode->allocBuffer( size );
        buffer->swap( getBuffer( ));
        reinterpret_cast< uint64_t* >( buffer->getData( ))[ 0 ] = size;

        ICommand cmd( _impl->localNode, _impl->localNode, buffer, false );
        _impl->dispatcher->dispatchCommand( cmd );
    }

    delete _impl;
}

void OCommand::_init( const uint32_t cmd, const uint32_t type )
{
#ifndef COLLAGE_BIGENDIAN
    // big endian hosts swap handshake commands to little endian...
    LBASSERTINFO( cmd < CMD_NODE_MAXIMUM, std::hex << "0x" << cmd << std::dec );
#endif
    enableSave();
    enable();
    *this << 0ull /* size */ << type << cmd;
}

void OCommand::send( const CompressorResult& body )
{
    LBASSERT( !_impl->dispatcher );
    LBASSERT( !_impl->isLocked );
    LBASSERT( body.compressor != EQ_COMPRESSOR_NONE ||
              body.chunks.size() == 1 );

    const Connections& connections = getConnections();
    BOOST_FOREACH( ConnectionPtr connection, connections )
        connection->lockSend();

    // header
    _impl->isLocked = true;
    _impl->bodySize = body.getSize();
    if( body.compressor != EQ_COMPRESSOR_NONE )
        _impl->bodySize += body.chunks.size() * sizeof( uint64_t );
    LBASSERT( _impl->bodySize > 0 );

    flush( true );

    // body
    BOOST_FOREACH( const lunchbox::CompressorChunk& chunk, body.chunks )
    {
        const uint64_t size = chunk.getNumBytes();
        BOOST_FOREACH( ConnectionPtr connection, connections )
        {
            if( body.compressor != EQ_COMPRESSOR_NONE )
                LBCHECK( connection->send( &size, sizeof( size ), true ));
            if( size > 0 )
                LBCHECK( connection->send( chunk.data, size, true ));
        }
    }

    // padding
    const uint64_t size = _impl->bodySize + getBuffer().getSize();
    if( size < COMMAND_MINSIZE ) // Fill send to minimal size
    {
        const size_t delta = COMMAND_MINSIZE - size;
        void* padding = alloca( delta );
        BOOST_FOREACH( ConnectionPtr connection, connections )
            connection->send( padding, delta, true );
    }
    BOOST_FOREACH( ConnectionPtr connection, connections )
        connection->unlockSend();

    _impl->isLocked = false;
    _impl->bodySize = 0;
    reset();
}

size_t OCommand::getSize()
{
    return sizeof( uint64_t ) + sizeof( uint32_t ) + sizeof( uint32_t );
}

uint128_t OCommand::getVersion() const
{
    return VERSION_NONE;
}

void OCommand::sendData( const CompressorResult& data,
                         const bool last LB_UNUSED )
{
    LBASSERT( !_impl->dispatcher );
    LBASSERT( last );
    LBASSERT( data.compressor == EQ_COMPRESSOR_NONE );

    if( data.chunks.size() != 1 )
    {
        LBASSERT( data.chunks.size() == 1 );
        return;
    }

    const lunchbox::CompressorChunk& chunk = data.chunks.front();
    const uint64_t size = chunk.getNumBytes();
    LBASSERT( getBuffer().getData() == chunk.data );
    LBASSERT( getBuffer().getSize() == size );
    LBASSERT( data.rawSize == size );
    LBASSERT( getBuffer().getMaxSize() >= COMMAND_MINSIZE );

    // Update size field
    uint8_t* bytes = (uint8_t*)( chunk.data );
    reinterpret_cast< uint64_t* >( bytes )[ 0 ] = _impl->bodySize + size;
    const uint64_t sendSize = _impl->isLocked ? size : LB_MAX( size,
                                                               COMMAND_MINSIZE);
    const Connections& connections = getConnections();
    BOOST_FOREACH( ConnectionPtr connection, connections )
    {
        if ( connection )
            connection->send( bytes, sendSize, _impl->isLocked );
        else
            LBERROR << "Can't send data, node is closed" << std::endl;
    }
}

}
