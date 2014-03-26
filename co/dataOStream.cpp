
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
 *               2011-2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include "dataOStream.h"

#include "buffer.h"
#include "commands.h"
#include "compressorResult.h"
#include "global.h"
#include "log.h"
#include "node.h"
#include "types.h"

#include <lunchbox/compressor.h>
#include <lunchbox/plugins/compressor.h>

#include  <boost/foreach.hpp>

namespace co
{
namespace detail
{
class DataOStream
{
public:
    co::DataOStream::State state;

    /** The buffer used for saving and buffering */
    lunchbox::Bufferb buffer;

    /** The start position of the buffering, always 0 if !_save */
    uint64_t bufferStart;

    /** The uncompressed size of a completely compressed buffer. */
    uint64_t dataSize;

    /** The compressor instance. */
    lunchbox::Compressor compressor;

    /** The output stream is enabled for writing */
    bool enabled;

    /** Some data has been sent since it was enabled */
    bool dataSent;

    /** Save all sent data */
    bool save;

    DataOStream()
        : state( co::DataOStream::STATE_UNCOMPRESSED )
        , bufferStart( 0 )
        , dataSize( 0 )
        , enabled( false )
        , dataSent( false )
        , save( false )
    {}

    DataOStream( const DataOStream& rhs )
        : state( rhs.state )
        , bufferStart( rhs.bufferStart )
        , dataSize( rhs.dataSize )
        , enabled( rhs.enabled )
        , dataSent( rhs.dataSent )
        , save( rhs.save )
    {}

    uint32_t getCompressor() const
    {
        if( state == co::DataOStream::STATE_UNCOMPRESSED ||
            state == co::DataOStream::STATE_UNCOMPRESSIBLE )
        {
            return EQ_COMPRESSOR_NONE;
        }
        return compressor.getInfo().name;
    }

    /** Compress data and update the compressor state. */
    const CompressorResult& compress( void* src, const uint64_t size,
                                      const co::DataOStream::State newState )
    {
        if( state == newState ||
            state == co::DataOStream::STATE_UNCOMPRESSIBLE )
        {
            return result_;
        }
        const uint64_t threshold =
           uint64_t( Global::getIAttribute( Global::IATTR_OBJECT_COMPRESSION ));

        if( !compressor || size <= threshold )
        {
            result_ = CompressorResult( src, size );
            return result_;
        }

        const uint64_t inDims[2] = { 0, size };

        compressor.compress( src, inDims );
        const lunchbox::CompressorResult& result = compressor.getResult();
        LBASSERT( !result.chunks.empty( ));

        if( result.getSize() >= size )
        {
#ifndef CO_AGGRESSIVE_CACHING
            compressor.realloc();

            if( newState == co::DataOStream::STATE_COMPLETE )
                buffer.pack();
#endif

            state = co::DataOStream::STATE_UNCOMPRESSIBLE;
            result_ = CompressorResult( src, size );
            return result_;
        }

#ifndef CO_AGGRESSIVE_CACHING
        if( newState == co::DataOStream::STATE_COMPLETE )
        {
            LBASSERTINFO( buffer.getSize() == dataSize,
                          "Buffered " << buffer.getSize() <<
                          " not complete with " << dataSize <<
                          " bytes in state " << state );
            buffer.clear();
        }
#endif

        state = newState;
        result_ = CompressorResult( result, size );
        return result_;
    }

private:
    CompressorResult result_;
};
}

DataOStream::DataOStream()
    : _impl( new detail::DataOStream )
{}

DataOStream::DataOStream( DataOStream& rhs )
    : lunchbox::NonCopyable()
    , _impl( new detail::DataOStream( *rhs._impl ))
{
    getBuffer().swap( rhs.getBuffer( ));
}

DataOStream::~DataOStream()
{
    // Can't call disable() from destructor since it uses virtual functions
    LBASSERT( !_impl->enabled );
    delete _impl;
}

void DataOStream::_initCompressor( const uint32_t name )
{
    LBCHECK( _impl->compressor.setup( Global::getPluginRegistry(), name ));
    LB_TS_RESET( _impl->compressor._thread );
}

void DataOStream::enable()
{
    LBASSERT( !_impl->enabled );
    _impl->state = STATE_UNCOMPRESSED;
    _impl->bufferStart = 0;
    _impl->dataSent    = false;
    _impl->dataSize    = 0;
    _impl->enabled     = true;
    _impl->buffer.setSize( 0 );
#ifdef CO_AGGRESSIVE_CACHING
    _impl->buffer.reserve( COMMAND_ALLOCSIZE );
#else
    _impl->buffer.reserve( COMMAND_MINSIZE );
#endif
}

void DataOStream::reemit()
{
    LBASSERT( !_impl->enabled );
    LBASSERT( _impl->save );

    emit( _impl->buffer.getData(), _impl->dataSize, STATE_COMPLETE, true );
}

void DataOStream::disable()
{
    if( !_impl->enabled )
        return;

    _impl->dataSize = _impl->buffer.getSize();
    _impl->dataSent = _impl->dataSize > 0;

    if( _impl->dataSent )
    {
        const uint64_t size = _impl->buffer.getSize() - _impl->bufferStart;
        if( _impl->state == co::DataOStream::STATE_PARTIAL && size == 0 )
        {
            // OPT: all data has been sent in one compressed chunk
            _impl->state = co::DataOStream::STATE_COMPLETE;
#ifndef CO_AGGRESSIVE_CACHING
            _impl->buffer.clear();
#endif
        }
        else
            _impl->state = co::DataOStream::STATE_UNCOMPRESSED;

        void* ptr = _impl->buffer.getData() + _impl->bufferStart;
        const State state = _impl->bufferStart == 0 ? STATE_COMPLETE :
                                                      STATE_PARTIAL;
        emit( ptr, size, state, true ); // always emit to finalize
    }

#ifndef CO_AGGRESSIVE_CACHING
    if( !_impl->save )
        _impl->buffer.clear();
#endif
    _impl->enabled = false;
}

void DataOStream::enableSave()
{
    LBASSERTINFO( !_impl->enabled ||
                  ( !_impl->dataSent && _impl->buffer.getSize() == 0 ),
                  "Can't enable saving after data has been written" );
    _impl->save = true;
}

void DataOStream::disableSave()
{
    LBASSERTINFO( !_impl->enabled ||
                  (!_impl->dataSent && _impl->buffer.getSize() == 0 ),
                  "Can't disable saving after data has been written" );
    _impl->save = false;
}

bool DataOStream::hasData() const
{
    return _impl->dataSent;
}

void DataOStream::_write( const void* data, uint64_t size )
{
    LBASSERT( _impl->enabled );

    if( _impl->buffer.getSize() - _impl->bufferStart >
        Global::getObjectBufferSize( ))
    {
        flush( false );
    }
    _impl->buffer.append( static_cast< const uint8_t* >( data ), size );
}

void DataOStream::flush( const bool last )
{
    LBASSERT( _impl->enabled );
    void* ptr = _impl->buffer.getData() + _impl->bufferStart;
    const State state = _impl->bufferStart==0 ? STATE_COMPLETE : STATE_PARTIAL;

    _impl->state = STATE_UNCOMPRESSED;
    _impl->dataSize = _impl->buffer.getSize() - _impl->bufferStart;
    emit( ptr, _impl->dataSize, state, last );

    _impl->dataSent = true;
    _resetBuffer();
}

void DataOStream::reset()
{
    _resetBuffer();
    _impl->enabled = false;
}

void DataOStream::_resetBuffer()
{
    _impl->state = STATE_UNCOMPRESSED;
    if( _impl->save )
        _impl->bufferStart = _impl->buffer.getSize();
    else
    {
        _impl->bufferStart = 0;
        _impl->buffer.setSize( 0 );
    }
}

const CompressorResult& DataOStream::compress( void* src, const uint64_t size,
                                               const State newState )
{
    return _impl->compress( src, size, newState );
}

lunchbox::Bufferb& DataOStream::getBuffer()
{
    return _impl->buffer;
}

}
