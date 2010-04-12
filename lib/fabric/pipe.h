
/* Copyright (c) 2010, Stefan Eilemann <eile@equalizergraphics.com>
 * Copyright (c) 2010, Cedric Stalder <cedric.stalder@gmail.com> 
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

#ifndef EQFABRIC_PIPE_H
#define EQFABRIC_PIPE_H

#include <eq/fabric/entity.h>        // base class
#include <eq/fabric/paths.h>
#include <eq/fabric/pixelViewport.h> // base class
#include <eq/fabric/types.h>

namespace eq
{

namespace fabric
{
    template< class, class, class > class Window;
    template< class N, class P, class W > class Pipe : public Entity
    {
    public:
        
        /** A vector of pointers to Window */
        typedef std::vector< W* >  WindowVector; 
        
        N*       getNode()       { return _node; }
        const N* getNode() const { return _node; }

         /**
         * Returns the port number of this pipe.
         * 
         * The port number identifies the X server for systems using the
         * X11/GLX window system. It currently has no meaning on other systems.
         *
         * @return the port number of this pipe, or
         *         <code>EQ_UNDEFINED_UINT32</code>.
         */
        uint32_t getPort() const { return _port; }
        
        EQFABRIC_EXPORT void setPort( const uint32_t port ); //!< @internal

        /** 
         * Returns the device number of this pipe.
         * 
         * The device number identifies the X screen for systems using the
         * X11/GLX window system, or the number of the virtual screen for the
         * AGL window system. On Windows systems it identifies the graphics
         * adapter. Normally the device identifies a GPU.
         *
         * @return the device number of this pipe, or 
         *         <code>EQ_UNDEFINED_UINT32</code>.
         */
        uint32_t getDevice() const { return _device; }

        EQFABRIC_EXPORT void setDevice( const uint32_t device ); //!< @internal

        /** @return the pixel viewport. */
        const PixelViewport& getPixelViewport() const { return _pvp; }

        void setCudaGLInterop( const bool cudaGLInterop )
            { _cudaGLInterop = cudaGLInterop; } 

        bool getCudaGLInterop() const { return _cudaGLInterop; }

        /**
         * Set the pipes's pixel viewport.
         *
         *  Used from _osPipe calls
         *
         * @param pvp the viewport in pixels.
         */
        EQFABRIC_EXPORT void setPixelViewport( const eq::PixelViewport& pvp );

        void notifyPixelViewportChanged();

        /** @return the index path to this pipe. @internal */
        EQFABRIC_EXPORT PipePath getPath() const;
        //@}

        /** @return the vector of windows. */
        const WindowVector& getWindows() const { return _windows; }
        /**
         * @name Attributes
         */
        //@{
        // Note: also update string array initialization in pipe.cpp
        enum IAttribute
        {
            IATTR_HINT_THREAD,
            IATTR_HINT_CUDA_GL_INTEROP,
            IATTR_FILL1,
            IATTR_FILL2,
            IATTR_ALL
        };
        
        /** @return the name of a window attribute. */
        EQFABRIC_EXPORT static const std::string& getIAttributeString(
                                                      const IAttribute attr );
        //@}
    protected:
        //-------------------- Members --------------------

        /** Constructs a new pipe. */
        Pipe( N* parent );       
        
        /** Constructs a new deep copy of a pipe. */
        Pipe( const Pipe& from, N* parent );

        EQFABRIC_EXPORT virtual ~Pipe( );

        enum DirtyBits
        {
            DIRTY_ATTRIBUTES      = Object::DIRTY_CUSTOM << 0,
            DIRTY_PIXELVIEWPORT   = Object::DIRTY_CUSTOM << 1,
            DIRTY_MEMBER          = Object::DIRTY_CUSTOM << 2,
        };

        /** @internal */
        EQFABRIC_EXPORT virtual void serialize( net::DataOStream& os,
                                                const uint64_t dirtyBits );
        /** @internal */
        EQFABRIC_EXPORT virtual void deserialize( net::DataIStream& is, 
                                                  const uint64_t dirtyBits );

        /** @return the vector of windows. */
        WindowVector& _getWindows() { return _windows; }

        template< class, class, class > friend class Window;
        void _addWindow( W* window );
        EQFABRIC_EXPORT bool _removeWindow( W* window );
        W* _findWindow( const uint32_t id );

        virtual ChangeType getChangeType() const { return UNBUFFERED; }

    private:
        /** The display (GLX) or ignored (Win32, AGL). */
        uint32_t _port;

        /** The screen (GLX), GPU (Win32) or virtual screen (AGL). */
        uint32_t _device;

        /** The size (and location) of the pipe. */
        PixelViewport _pvp;

        /** The parent node. */
        N* const _node;
        
        /** The list of windows. */
        WindowVector _windows;

        /** CUDA GL interop mode. */
        bool _cudaGLInterop;

        union // placeholder for binary-compatible changes
        {
            char dummy[32];
        };
    };
}
}

#endif // EQFABRIC_PIPE_H
