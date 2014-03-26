
/* Copyright (c) 2007-2014, Stefan Eilemann <eile@equalizergraphics.com>
 *                    2010, Cedric Stalder  <cedric.stalder@gmail.com>
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

#include "objectInstanceDataOStream.h"

#include "log.h"
#include "nodeCommand.h"
#include "object.h"
#include "objectDataIStream.h"
#include "objectDataOCommand.h"
#include "versionedMasterCM.h"


namespace co
{
ObjectInstanceDataOStream::ObjectInstanceDataOStream( const ObjectCM* cm )
        : ObjectDataOStream( cm )
        , _instanceID( CO_INSTANCE_ALL )
        , _command( 0 )
{}

ObjectInstanceDataOStream::~ObjectInstanceDataOStream()
{}

void ObjectInstanceDataOStream::reset()
{
    ObjectDataOStream::reset();
    _nodeID = 0;
    _instanceID = CO_INSTANCE_NONE;
    _command = 0;
}

void ObjectInstanceDataOStream::enableCommit( const uint128_t& version,
                                              const Nodes& receivers )
{
    _command = CMD_NODE_OBJECT_INSTANCE_COMMIT;
    _nodeID = 0;
    _instanceID = CO_INSTANCE_NONE;
    ObjectDataOStream::enableCommit( version, receivers );
}

void ObjectInstanceDataOStream::enablePush( const uint128_t& version,
                                            const Nodes& receivers )
{
    _command = CMD_NODE_OBJECT_INSTANCE_PUSH;
    _nodeID = 0;
    _instanceID = CO_INSTANCE_NONE;
    ObjectDataOStream::enableCommit( version, receivers );
}

void ObjectInstanceDataOStream::enableSync( const uint128_t& version,
                                            const MasterCMCommand& command )
{
    NodePtr node = command.getNode();

    _command = CMD_NODE_OBJECT_INSTANCE_SYNC;
    _nodeID = node->getNodeID();
    _instanceID = command.getRequestID(); // ugh
    ObjectDataOStream::enableCommit( version, Nodes( 1, node ));
}

void ObjectInstanceDataOStream::push( const Nodes& receivers,
                                      const UUID& objectID,
                                      const uint128_t& groupID,
                                      const uint128_t& typeID )
{
    _command = CMD_NODE_OBJECT_INSTANCE_PUSH;
    _nodeID = 0;
    _instanceID = CO_INSTANCE_NONE;

    setup( receivers );
    reemit();
    OCommand( getConnections(), CMD_NODE_OBJECT_PUSH, COMMANDTYPE_NODE )
        << objectID << groupID << typeID;
    clear();
}

void ObjectInstanceDataOStream::sync( const MasterCMCommand& command )
{
    NodePtr node = command.getNode();

    _command = CMD_NODE_OBJECT_INSTANCE_SYNC;
    _nodeID = node->getNodeID();
    _instanceID = command.getRequestID(); // ugh

    setup( Nodes( 1, node ));
    reemit();
    clear();
}

void ObjectInstanceDataOStream::sendInstanceData( const Nodes& receivers )
{
    _command = CMD_NODE_OBJECT_INSTANCE;
    _nodeID = 0;
    _instanceID = CO_INSTANCE_NONE;

    setup( receivers );
    reemit();
    clear();
}

void ObjectInstanceDataOStream::sendMapData( NodePtr node,
                                             const uint32_t instanceID )
{
    _command = CMD_NODE_OBJECT_INSTANCE_MAP;
    _nodeID = node->getNodeID();
    _instanceID = instanceID;

    setup( node, true /* useMulticast */ );
    reemit();
    clear();
}

void ObjectInstanceDataOStream::enableMap( const uint128_t& version,
                                           NodePtr node,
                                           const uint32_t instanceID )
{
    _command = CMD_NODE_OBJECT_INSTANCE_MAP;
    _nodeID = node->getNodeID();
    _instanceID = instanceID;
    _version = version;

    setup( node, true /* useMulticast */ );
    enable();
}

void ObjectInstanceDataOStream::sendData( const CompressorResult& data,
                                          const bool last )
{
    LBASSERT( _command );
    send( _command, COMMANDTYPE_NODE, _instanceID, data, last )
        << _nodeID << _cm->getObject()->getInstanceID();
}

}
