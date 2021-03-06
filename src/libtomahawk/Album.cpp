/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Album.h"

#include "Artist.h"
#include "AlbumPlaylistInterface.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "Query.h"
#include "Source.h"

#include "utils/Logger.h"

using namespace Tomahawk;


Album::~Album()
{
    m_ownRef.clear();

#ifndef ENABLE_HEADLESS
    delete m_cover;
#endif
}


album_ptr
Album::get( const Tomahawk::artist_ptr& artist, const QString& name, bool autoCreate )
{
    if ( !Database::instance() || !Database::instance()->impl() )
        return album_ptr();

    int albid = Database::instance()->impl()->albumId( artist->id(), name, autoCreate );
    if ( albid < 1 && autoCreate )
        return album_ptr();

    return Album::get( albid, name, artist );
}


album_ptr
Album::get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist )
{
    static QHash< unsigned int, album_ptr > s_albums;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_albums.contains( id ) )
    {
        return s_albums.value( id );
    }

    album_ptr a = album_ptr( new Album( id, name, artist ), &QObject::deleteLater );
    a->setWeakRef( a.toWeakRef() );

    if ( id > 0 )
        s_albums.insert( id, a );

    return a;
}


Album::Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist )
    : QObject()
    , m_id( id )
    , m_name( name )
    , m_artist( artist )
    , m_coverLoaded( false )
    , m_coverLoading( false )
#ifndef ENABLE_HEADLESS
    , m_cover( 0 )
#endif
{
    m_sortname = DatabaseImpl::sortname( name );
}


void
Album::onTracksLoaded( Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection )
{
    emit tracksAdded( playlistInterface( mode, collection )->tracks(), mode, collection );
}


artist_ptr
Album::artist() const
{
    return m_artist;
}


#ifndef ENABLE_HEADLESS
QPixmap
Album::cover( const QSize& size, bool forceLoad ) const
{
    if ( !m_coverLoaded && !m_coverLoading )
    {
        if ( !forceLoad )
            return QPixmap();

        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = artist()->name();
        trackInfo["album"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = infoid();
        requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        requestData.customData = QVariantMap();

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                SIGNAL( finished( QString ) ),
                SLOT( infoSystemFinished( QString ) ) );

        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

        m_coverLoading = true;
    }

    if ( !m_cover && !m_coverBuffer.isEmpty() )
    {
        m_cover = new QPixmap();
        m_cover->loadFromData( m_coverBuffer );
    }

    if ( m_cover && !m_cover->isNull() && !size.isEmpty() )
    {
        if ( m_coverCache.contains( size.width() ) )
        {
            return m_coverCache.value( size.width() );
        }

        QPixmap scaledCover;
        scaledCover = m_cover->scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        m_coverCache.insert( size.width(), scaledCover );
        return scaledCover;
    }

    if ( m_cover )
        return *m_cover;
    else
        return QPixmap();
}
#endif


void
Album::infoSystemInfo( const Tomahawk::InfoSystem::InfoRequestData& requestData, const QVariant& output )
{
    if ( requestData.caller != infoid() ||
         requestData.type != Tomahawk::InfoSystem::InfoAlbumCoverArt )
    {
        return;
    }

    if ( !output.isNull() && output.isValid() )
    {
        QVariantMap returnedData = output.value< QVariantMap >();
        const QByteArray ba = returnedData["imgbytes"].toByteArray();
        if ( ba.length() )
        {
            m_coverBuffer = ba;
            emit coverChanged();
        }

        m_coverLoaded = true;
    }
}


void
Album::infoSystemFinished( const QString& target )
{
    if ( target != infoid() )
        return;

    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                this, SLOT( infoSystemFinished( QString ) ) );

    m_coverLoading = false;

    emit updated();
}


Tomahawk::playlistinterface_ptr
Album::playlistInterface( ModelMode mode, const Tomahawk::collection_ptr& collection )
{
    playlistinterface_ptr pli = m_playlistInterface[ mode ][ collection ];

    if ( pli.isNull() )
    {
        pli = Tomahawk::playlistinterface_ptr( new Tomahawk::AlbumPlaylistInterface( this, mode, collection ) );
        connect( pli.data(), SIGNAL( tracksLoaded( Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                               SLOT( onTracksLoaded( Tomahawk::ModelMode, Tomahawk::collection_ptr ) ) );

        m_playlistInterface[ mode ][ collection ] = pli;
    }

    return pli;
}


QList<Tomahawk::query_ptr>
Album::tracks( ModelMode mode, const Tomahawk::collection_ptr& collection )
{
    return playlistInterface( mode, collection )->tracks();
}


QString
Album::infoid() const
{
    if ( m_uuid.isEmpty() )
        m_uuid = uuid();
    
    return m_uuid;
}