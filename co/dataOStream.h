
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder <cedric.stalder@gmail.com>
 *               2012-2014, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_DATAOSTREAM_H
#define CO_DATAOSTREAM_H

#include <co/api.h>
#include <co/types.h>

#include <lunchbox/array.h> // used inline
#include <lunchbox/nonCopyable.h> // base class
#include <lunchbox/stdExt.h> // used inline

#include <boost/type_traits.hpp> // is_pod
#include <map> // this and below to serialize the data type
#include <set>
#include <vector>

namespace co
{
namespace detail { class DataOStream; }

/**
 * A std::ostream-like interface for object serialization.
 *
 * Implements buffering, retaining and compressing data in a binary format.
 * Derived classes emit the data using the appropriate implementation.
 */
class DataOStream : public boost::noncopyable
{
public:
    /** @name Internal */
    //@{
    /** @internal Close stream and emit remaining data. */
    CO_API virtual void close();

    /** @internal @return if data was emitted since the last open() */
    CO_API bool hasData() const;
    //@}

    /** @name Data access */
    //@{
    /** @return the version of the data contained in this stream. */
    virtual uint128_t getVersion() const = 0;
    //@}

    /** @name Data output */
    //@{
    /** Write a plain data item by copying it to the stream. @version 1.0 */
    template< class T > DataOStream& operator << ( const T& value )
        { _write( &value, sizeof( value )); return *this; }

    /** Write a C array. @version 1.0 */
    template< class T > DataOStream& operator << ( const Array< T > array )
        { _writeArray( array, boost::is_pod<T>( )); return *this; }

    /**
     * Write a lunchbox::RefPtr. Refcount has to be managed by caller.
     * @version 1.1
     */
    template< class T >
    DataOStream& operator << ( const lunchbox::RefPtr< T >& ptr );

    /** Write a lunchbox::Buffer. @version 1.0 */
    template< class T >
    DataOStream& operator << ( const lunchbox::Buffer< T >& buffer );

    /** Transmit a request identifier. @version 1.1.1 */
    template< class T >
    DataOStream& operator << ( const lunchbox::Request<T>& request )
        { return (*this) << request.getID(); }

    /** Write a std::vector of serializable items. @version 1.0 */
    template< class T >
    DataOStream& operator << ( const std::vector< T >& value );

    /** Write a std::map of serializable items. @version 1.0 */
    template< class K, class V >
    DataOStream& operator << ( const std::map< K, V >& value );

    /** Write a std::set of serializable items. @version 1.0 */
    template< class T >
    DataOStream& operator << ( const std::set< T >& value );

    /** Write a stde::hash_map of serializable items. @version 1.0 */
    template< class K, class V >
    DataOStream& operator << ( const stde::hash_map< K, V >& value );

    /** Write a stde::hash_set of serializable items. @version 1.0 */
    template< class T >
    DataOStream& operator << ( const stde::hash_set< T >& value );

    /** @internal
     * Serialize child objects.
     *
     * The DataIStream has a deserialize counterpart to this method. All
     * child objects have to be registered or mapped beforehand.
     */
    template< typename C >
    void serializeChildren( const std::vector< C* >& children );
    //@}

protected:
    friend class detail::DataOStream;
    enum State
    {
        STATE_UNCOMPRESSED,
        STATE_PARTIAL,
        STATE_COMPLETE,
        STATE_UNCOMPRESSIBLE,
        STATE_DONT_COMPRESS
    };

    /** @internal
     * @param save enable/disable copying of written data into a saved buffer
     */
    CO_API explicit DataOStream( const bool save );
    DataOStream( DataOStream& rhs );  //!< @internal
    virtual CO_API ~DataOStream(); //!< @internal

    /** @internal @return written, not emitted data */
    CO_API lunchbox::Bufferb& getBuffer();

    /** @internal Initialize the given compressor. */
    void _initCompressor( const uint32_t compressor );

    /** @internal Open stream to write and emit data. */
    CO_API void open();

    /** @internal Emit written data since the last emitData(). */
    void flush( const bool last );

    /** @internal Re-emit all buffered data; requires save buffer (see ctor) */
    void reemitData();

    /** @internal Emit a data buffer. */
    virtual void emitData( const CompressorResult& data, const bool last ) = 0;

    /** @internal Reset the whole stream. */
    virtual CO_API void reset();

    /** @internal Set flush granularity, default co::getObjectBufferSize() */
    void setChunkSize( const uint64_t size );

    /** @internal compress, if needed, and return the result. */
    virtual const CompressorResult& compress( void* data, const uint64_t size,
                                              const State newState );

private:
    DataOStream();
    detail::DataOStream* const _impl;

    /**
     * Write data into to the stream buffer; will flush based on chunk size, see
     * setChunkSize()
     */
    CO_API void _write( const void* data, uint64_t size );

    /** Reset after sending a buffer. */
    void _resetBuffer();

    /** Write a vector of trivial data. */
    template< class T >
    DataOStream& _writeFlatVector( const std::vector< T >& value )
    {
        const uint64_t nElems = value.size();
        _write( &nElems, sizeof( nElems ));
        if( nElems > 0 )
            _write( &value.front(), nElems * sizeof( T ));
        return *this;
    }

    /** Write an Array of POD data */
    template< class T >
    void _writeArray( const Array< T > array, const boost::true_type& )
        { _write( array.data, array.getNumBytes( )); }

    /** Write an Array of non-POD data */
    template< class T >
    void _writeArray( const Array< T > array, const boost::false_type& )
    {
        for( size_t i = 0; i < array.num; ++i )
            *this << array.data[ i ];
    }
};

std::ostream& operator << ( std::ostream&, const DataOStream& );

}

#include "dataOStream.ipp" // template implementation

#endif //CO_DATAOSTREAM_H
