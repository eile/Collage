
/* Copyright (c) 2011-2013, Stefan Eilemann <eile@eyescale.ch>
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

#include <test.h>

#include <co/connection.h>
#include <co/connectionDescription.h>
#include <co/dataIStream.h>
#include <co/dataOStream.h>
#include <co/global.h>
#include <co/init.h>
#include <co/node.h>
#include <co/object.h>
#include <lunchbox/clock.h>
#include <lunchbox/monitor.h>
#include <lunchbox/rng.h>

#include <iostream>

using co::uint128_t;

namespace
{

lunchbox::Monitor< co::Object::ChangeType > monitor( co::Object::NONE );

static const std::string message = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut eget felis sed leo tincidunt dictum eu eu felis. Aenean aliquam augue nec elit tristique tempus. Pellentesque dignissim adipiscing tellus, ut porttitor nisl lacinia vel. Donec malesuada lobortis velit, nec lobortis metus consequat ac. Ut dictum rutrum dui. Pellentesque quis risus at lectus bibendum laoreet. Suspendisse tristique urna quis urna faucibus et auctor risus ultricies. Morbi vitae mi vitae nisi adipiscing ultricies ac in nulla. Nam mattis venenatis nulla, non posuere felis tempus eget. Cras dapibus ultrices arcu vel dapibus. Nam hendrerit lacinia consectetur. Donec ullamcorper nibh nisl, id aliquam nisl. Nunc at tortor a lacus tincidunt gravida vitae nec risus. Suspendisse potenti. Fusce tristique dapibus ipsum, sit amet posuere turpis fermentum nec. Nam nec ante dolor.";

static const std::string commitMessage = "Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?";
}

class Object : public co::Object
{
public:
    Object( const ChangeType type ) : nSync( 0 ), _type( type ) {}
    Object( const ChangeType type, co::DataIStream& is )
        : nSync( 0 ), _type( type )
    {
        applyInstanceData( is );
    }

    size_t nSync;

protected:
    virtual ChangeType getChangeType() const { return _type; }
    virtual void getInstanceData( co::DataOStream& os )
    {
        os << ( os.getVersion() <= co::VERSION_FIRST ? message : commitMessage )
           << _type;
    }

    virtual void applyInstanceData( co::DataIStream& is )
    {
        std::string msg;
        ChangeType type( co::Object::NONE );
        is >> msg >> type;
        ++nSync;

        TESTINFO( ( is.getVersion() <= co::VERSION_FIRST && msg == message ) ||
                  msg == commitMessage,
                  is.getVersion() << ": " << msg.substr( 0, 10 ));
        TESTINFO( _type == type, _type << " != " << int( type ));
    }

private:
    const ChangeType _type;
};

class Server : public co::LocalNode
{
public:
    Server() : object( 0 ) {}

    Object* object;

protected:
    void objectPush( const uint128_t& groupID, const uint128_t& typeID,
                     const co::UUID& objectID, co::DataIStream& is ) override
    {
        const co::Object::ChangeType type =
            co::Object::ChangeType( typeID.low( ));
        TESTINFO( is.nRemainingBuffers() == 1 || // buffered
                  is.nRemainingBuffers() == 2,   // unbuffered
                  is.nRemainingBuffers( ));
        TEST( !object );
        object = new Object( type, is );
        TESTINFO( !is.hasData(), is.nRemainingBuffers( ));
        TESTINFO( monitor != type, monitor << " == " << type );
        monitor = type;
    }
};

int main( int argc, char **argv )
{
    co::init( argc, argv );
    co::Global::setObjectBufferSize( 600 );

    lunchbox::RNG rng;
    const uint16_t port = (rng.get<uint16_t>() % 60000) + 1024;

    lunchbox::RefPtr< Server > server = new Server;
    co::ConnectionDescriptionPtr connDesc =
        new co::ConnectionDescription;

    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->port = port;
    connDesc->setHostname( "localhost" );

    server->addConnectionDescription( connDesc );
    TEST( server->listen( ));

    co::NodePtr serverProxy = new co::Node;
    serverProxy->addConnectionDescription( connDesc );

    connDesc = new co::ConnectionDescription;
    connDesc->type = co::CONNECTIONTYPE_TCPIP;
    connDesc->setHostname( "localhost" );

    co::LocalNodePtr client = new co::LocalNode;
    client->addConnectionDescription( connDesc );
    TEST( client->listen( ));
    TEST( client->connect( serverProxy ));

    co::Nodes nodes;
    nodes.push_back( serverProxy );

    lunchbox::Clock clock;
    for( uint64_t i = co::Object::NONE+1; i <= co::Object::UNBUFFERED; ++i )
    {
        const co::Object::ChangeType type = co::Object::ChangeType( i );
        Object object( type );
        TEST( client->registerObject( &object ));
        object.push( co::uint128_t(42), co::uint128_t(i), nodes );
        monitor.waitEQ( type );

        TEST( server->mapObject( server->object, object.getID(),
                                 co::VERSION_NONE ));
        TEST( object.nSync == 0 );
        TEST( server->object->nSync == 1 );
        TESTINFO( client->syncObject( server->object, serverProxy,
                                      object.getID( )),
                  "type " << type );
        TEST( object.nSync == 0 );
        TEST( server->object->nSync == 2 );

        if( type != co::Object::STATIC ) // no commits for static objects
        {
            object.commit();
            object.commit();
            server->object->sync( co::VERSION_FIRST + 2 );

            TESTINFO( server->object->getVersion() == co::VERSION_FIRST + 2,
                      server->object->getVersion( ));
            TESTINFO( object.getVersion() == co::VERSION_FIRST + 2,
                      object.getVersion( ));
            TEST( object.nSync == 2 );
            TEST( server->object->nSync == 2 );
        }

        server->unmapObject( server->object );
        delete server->object;
        server->object = 0;

        client->deregisterObject( &object );
    }
    const float time = clock.getTimef();
    nodes.clear();

    std::cout << time << "ms for " << int( co::Object::UNBUFFERED )
              << " object types" << std::endl;

    TEST( client->disconnect( serverProxy ));
    TEST( client->close( ));
    TEST( server->close( ));

    serverProxy->printHolders( std::cerr );
    TESTINFO( serverProxy->getRefCount() == 1, serverProxy->getRefCount( ));
    TESTINFO( client->getRefCount() == 1, client->getRefCount( ));
    TESTINFO( server->getRefCount() == 1, server->getRefCount( ));

    serverProxy = 0;
    client      = 0;
    server      = 0;

    co::exit();
    return EXIT_SUCCESS;
}
