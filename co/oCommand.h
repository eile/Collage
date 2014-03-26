
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

#ifndef CO_OCOMMAND_H
#define CO_OCOMMAND_H

#include <co/commands.h>       // for COMMANDTYPE_NODE
#include <co/connectionOStream.h>    // base class


namespace co
{

namespace detail { class OCommand; }

/**
 * A class for sending commands with data to local and external nodes.
 *
 * The data to this command is added via the interface provided by DataOStream.
 * The command is send or dispatched after it goes out of scope, i.e. during
 * destruction.
 */
class OCommand : public ConnectionOStream
{
public:
    /** @internal
     * Construct a command which is send & dispatched typically to a co::Node.
     *
     * @param receivers list of connections where to send the command to.
     * @param cmd the command.
     * @param type the command type for dispatching.
     */
    CO_API OCommand( const Connections& receivers, const uint32_t cmd,
                     const uint32_t type );

    /** @internal
     * Construct a command which is dispatched locally typically to a co::Node.
     *
     * @param dispatcher the dispatcher to dispatch this command.
     * @param localNode the local node that holds the command cache.
     * @param cmd the command.
     * @param type the command type for dispatching.
     */
    CO_API OCommand( Dispatcher* const dispatcher, LocalNodePtr localNode,
                     const uint32_t cmd, const uint32_t type );

    /** @internal "move" ctor */
    CO_API OCommand( const OCommand& rhs );

    /** Send or dispatch this command during destruction. */
    CO_API virtual ~OCommand();

    /** @internal
     * Allow external send of data along with this command.
     *
     * Atomically sends the current buffer, body and padding to fill up to
     * COMMAND_MINSIZE.
     *
     * @param body the payload
     */
    CO_API void send( const CompressorResult& body );

    /** @internal @return the static base header size of this command. */
    CO_API static size_t getSize();

protected:
    /** @internal */
    CO_API void sendData( const CompressorResult& data,
                          const bool last ) override;

private:
    OCommand& operator = ( const OCommand& );
    detail::OCommand* const _impl;

    void _init( const uint32_t cmd, const uint32_t type );

    /** @internal @name DataIStream functions */
    //@{
    CO_API uint128_t getVersion() const override;
    //@}
};
}

#endif //CO_OCOMMAND_H
