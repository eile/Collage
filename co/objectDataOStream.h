
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2012, Daniel Nachbaur <danielnachbaur@gmail.com>
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

#ifndef CO_OBJECTDATAOSTREAM_H
#define CO_OBJECTDATAOSTREAM_H

#include <co/connectionOStream.h>   // base class

namespace co
{
/** The DataOStream for object data. */
class ObjectDataOStream : public ConnectionOStream
{
public:
    ObjectDataOStream( const ObjectCM* cm, const bool save );
    virtual ~ObjectDataOStream(){}

    void reset() override;

    /** Set up commit of the given version to the receivers. */
    virtual void enableCommit( const uint128_t& version,
                               const Nodes& receivers );

    uint128_t getVersion() const override { return _version; }

protected:
    ObjectDataOCommand send( const uint32_t cmd, const uint32_t type,
                             const uint32_t instanceID,
                             const CompressorResult& data,
                             const bool last );

    const ObjectCM* _cm;
    uint128_t _version;
    uint32_t _sequence;
};
}
#endif //CO_OBJECTDATAOSTREAM_H
