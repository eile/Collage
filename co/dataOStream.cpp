
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
 *               2011-2014, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#include <lunchbox/compressor.h>
#include <lunchbox/clock.h>
#include <lunchbox/plugins/compressor.h>

namespace co
{
namespace
{
//#define INSTRUMENT
#ifdef INSTRUMENT
lunchbox::a_int32_t nBytesIn;
lunchbox::a_int32_t nBytesOut;
lunchbox::a_int32_t compressionTime;
#endif
}

namespace detail
{
class DataOStream
{
public:
    co::DataOStream::State state;

    /** The buffer used to save and buffer written data */
    lunchbox::Bufferb buffer;

    uint64_t chunkSize; //!< The flush granularity

    /** The start position of the buffering, always 0 if !save */
    uint64_t bufferStart;

    /** The uncompressed size of a completely compressed buffer. */
    uint64_t dataSize;

    /** The compressor instance. */
    lunchbox::Compressor compressor;

    /** The output stream is open/enabled for writing */
    bool isOpen;

    /** Data has been emitted since the last emitData */
    bool dataEmitted;

    /** Save all sent data */
    const bool save;

    explicit DataOStream( const bool save_ )
        : state( co::DataOStream::STATE_UNCOMPRESSED )
        , chunkSize( Global::getObjectBufferSize( ))
        , bufferStart( 0 )
        , dataSize( 0 )
        , isOpen( false )
        , dataEmitted( false )
        , save( save_ )
    {}

    DataOStream( const DataOStream& rhs )
        : state( rhs.state )
        , chunkSize( rhs.chunkSize )
        , bufferStart( rhs.bufferStart )
        , dataSize( rhs.dataSize )
        , isOpen( rhs.isOpen )
        , dataEmitted( rhs.dataEmitted )
        , save( rhs.save )
    {}

    /** Compress data if needed and update the compressor state. */
    const CompressorResult& compress( void* data, const uint64_t size,
                                      const co::DataOStream::State newState )
    {
        LBASSERT( newState == co::DataOStream::STATE_PARTIAL ||
                  newState == co::DataOStream::STATE_COMPLETE ||
                  newState == co::DataOStream::STATE_DONT_COMPRESS );

        if( state == newState ||
            state == co::DataOStream::STATE_UNCOMPRESSIBLE )
        {
            return _result;
        }
        const uint64_t threshold =
           uint64_t( Global::getIAttribute( Global::IATTR_OBJECT_COMPRESSION ));

        if( !compressor || size <= threshold ||
            newState == co::DataOStream::STATE_DONT_COMPRESS )
        {
            _result = CompressorResult( data, size );
            return _result;
        }

        const uint64_t inDims[2] = { 0, size };

        compressor.compress( data, inDims );
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
            _result = CompressorResult( data, size );
            return _result;
        }

#ifndef CO_AGGRESSIVE_CACHING
        if( newState == co::DataOStream::STATE_COMPLETE )
        {
            LBASSERTINFO( buffer.getSize() == dataSize,
                          "Buffered " << buffer.getSize() <<
                          " not complete with " << dataSize <<
                          " bytes in state " << uint32_t(state) );
            buffer.clear();
        }
#endif
        state = newState;
        _result = CompressorResult( result, size );
        return _result;
    }

private:
    CompressorResult _result;
};
}

DataOStream::DataOStream( const bool save_ )
    : _impl( new detail::DataOStream( save_ ))
{}

DataOStream::DataOStream( DataOStream& rhs )
    : boost::noncopyable()
    , _impl( new detail::DataOStream( *rhs._impl ))
{
    _impl->buffer.swap( rhs._impl->buffer );
}

DataOStream::~DataOStream()
{
    // Can't call close() from destructor since it uses virtual functions
    LBASSERT( !_impl->isOpen );
    delete _impl;
}

void DataOStream::_initCompressor( const uint32_t name )
{
    LBCHECK( _impl->compressor.setup( Global::getPluginRegistry(), name ));
    LB_TS_RESET( _impl->compressor._thread );
}

void DataOStream::open()
{
    LBASSERT( !_impl->isOpen );
    _impl->state = STATE_UNCOMPRESSED;
    _impl->bufferStart = 0;
    _impl->dataEmitted    = false;
    _impl->dataSize    = 0;
    _impl->isOpen     = true;
    _impl->buffer.setSize( 0 );
#ifdef CO_AGGRESSIVE_CACHING
    _impl->buffer.reserve( COMMAND_ALLOCSIZE );
#else
    _impl->buffer.reserve( COMMAND_MINSIZE );
#endif
}

void DataOStream::reemitData()
{
    LBASSERT( !_impl->isOpen );
    LBASSERT( _impl->save );
    LBASSERT( _impl->dataEmitted );

    emitData( compress( _impl->buffer.getData(), _impl->dataSize,
                        STATE_COMPLETE ), true );
}

void DataOStream::close()
{
    if( !_impl->isOpen )
        return;

    _impl->dataSize = _impl->buffer.getSize();

    if( _impl->dataSize > 0 )
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

        // always emit to finalize
        emitData( compress( ptr, size, state ), true );
        _impl->dataEmitted = true;
    }

#ifndef CO_AGGRESSIVE_CACHING
    if( !_impl->save )
        _impl->buffer.clear();
#endif
    _impl->isOpen = false;
}

bool DataOStream::hasData() const
{
    return _impl->dataEmitted;
}

void DataOStream::_write( const void* data, uint64_t size )
{
    LBASSERT( _impl->isOpen );

    if( _impl->buffer.getSize() - _impl->bufferStart > _impl->chunkSize )
        flush( false );

    _impl->buffer.append( static_cast< const uint8_t* >( data ), size );
}

void DataOStream::flush( const bool last )
{
    LBASSERT( _impl->isOpen );
    void* ptr = _impl->buffer.getData() + _impl->bufferStart;
    const State state = _impl->bufferStart == 0 ? STATE_COMPLETE
                                                : STATE_PARTIAL;

    _impl->state = STATE_UNCOMPRESSED;
    _impl->dataSize = _impl->buffer.getSize() - _impl->bufferStart;

    emitData( compress( ptr, _impl->dataSize, state ), last );
    _impl->dataEmitted = true;

    _resetBuffer();
}

void DataOStream::reset()
{
    _resetBuffer();
    _impl->isOpen = false;
}

void DataOStream::setChunkSize( const uint64_t size )
{
    LBASSERT( !_impl->isOpen );
    LBASSERT( size > 0 );
    _impl->chunkSize = size;
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

const CompressorResult& DataOStream::compress( void* data, const uint64_t size,
                                               const State newState )
{
#ifdef INSTRUMENT
    nBytesIn += size;
    lunchbox::Clock clock;
#endif
    const CompressorResult& result = _impl->compress( data, size, newState );
#ifdef INSTRUMENT
    compressionTime += uint32_t( clock.getTimef() * 1000.f );
    nBytesOut += result.getSize();
    if( compressionTime > 100000 )
        LBWARN << *this << std::endl;
#endif
    return result;
}

lunchbox::Bufferb& DataOStream::getBuffer()
{
    return _impl->buffer;
}

std::ostream& operator << ( std::ostream& os,
                            const DataOStream& dos LB_UNUSED )
{
#ifdef INSTRUMENT
    os << "DataOStream "
       << "emit " << nBytesOut << " of " << nBytesIn << " byte in "
       << compressionTime/1000 << "ms, saved "
       << int(( nBytesIn - nBytesOut ) / float( nBytesIn ) * 100.f ) << "%";

    nBytesIn = 0;
    nBytesOut = 0;
    compressionTime = 0;
    return os;
#else
    return os << "@" << (void*)&dos;
#endif
}

}
