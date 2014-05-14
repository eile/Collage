
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

#ifndef CO_COMPRESSORRESULT_H
#define CO_COMPRESSORRESULT_H

#include <lunchbox/compressorResult.h>

namespace co
{
/** Augment the lunchbox::CompressorResult with Collage-specific data. */
struct CompressorResult : public lunchbox::CompressorResult
{
    CompressorResult() : rawSize( 0 ) {}
    CompressorResult( void* src, const uint64_t size ) //!< uncompressed
        : lunchbox::CompressorResult( EQ_COMPRESSOR_NONE,
               lunchbox::CompressorChunks( 1,
                                        lunchbox::CompressorChunk( src, size )))
        , rawSize( size )
    {}

    CompressorResult( const lunchbox::CompressorResult& from, const uint64_t r )
        : lunchbox::CompressorResult( from ), rawSize( r ) {}

    uint64_t rawSize;
};
}
#endif  // CO_COMPRESSORRESULT_H
