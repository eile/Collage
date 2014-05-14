
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

#ifndef CO_CONNECTIONOSTREAM_H
#define CO_CONNECTIONOSTREAM_H

#include <co/dataOStream.h> // base class

namespace co
{
namespace detail { class ConnectionOStream; }
namespace DataStreamTest { class Sender; }

/** @internal Augments DataOStream with the capability to emit to Connections. */
class ConnectionOStream : public DataOStream
{
public:
    /** @return the current active connections */
    CO_API const Connections& getConnections() const;

    /** Disable the stream and emit the remaining data. */
    void disable() override;

protected:
    CO_API ConnectionOStream();
    ConnectionOStream( ConnectionOStream& rhs );
    virtual CO_API ~ConnectionOStream();

    /**
     * Set up the connection list for a group of nodes, using multicast
     * where possible.
     */
    void setup( const Nodes& receivers );

    void setup( const Connections& connections );
    friend class DataStreamTest::Sender;

    /** Set up the connection (list) for one node. */
    void setup( NodePtr node, const bool useMulticast );

    /** Clear setup connections */
    void clear();

    void reset() override;

    /** @name Data sending, used by the subclasses */
    //@{
    /** Send a data buffer (command) to the receivers. */
    virtual void sendData( const CompressorResult& data, const bool last ) = 0;
    //@}

private:
    detail::ConnectionOStream* const _impl;

    void emit( void* src, const uint64_t size, const State state,
               const bool last ) final;
};

std::ostream& operator << ( std::ostream&, const ConnectionOStream& );
}

#endif //CO_CONNECTIONOSTREAM_H
