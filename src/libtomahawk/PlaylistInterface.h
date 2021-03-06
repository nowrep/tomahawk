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

#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QtCore/QModelIndex>

#include "Typedefs.h"
#include "DllMacro.h"
#include "utils/Logger.h"

namespace Tomahawk
{

class DLLEXPORT PlaylistInterface : public QObject
{
Q_OBJECT

public:
    explicit PlaylistInterface();
    virtual ~PlaylistInterface();

    const QString id() { return m_id; }

    virtual QList< Tomahawk::query_ptr > tracks() = 0;
    virtual bool isFinished() const { return m_finished; }

    virtual int unfilteredTrackCount() const = 0;
    virtual int trackCount() const = 0;

    virtual Tomahawk::result_ptr currentItem() const = 0;
    virtual Tomahawk::result_ptr previousItem();
    virtual bool hasNextItem() { return true; }
    virtual Tomahawk::result_ptr nextItem();
    virtual Tomahawk::result_ptr siblingItem( int itemsAway ) = 0;

    virtual PlaylistModes::RepeatMode repeatMode() const = 0;

    virtual bool shuffled() const = 0;

    virtual PlaylistModes::ViewMode viewMode() const { return PlaylistModes::Unknown; }

    virtual PlaylistModes::SeekRestrictions seekRestrictions() const { return PlaylistModes::NoSeekRestrictions; }
    virtual PlaylistModes::SkipRestrictions skipRestrictions() const { return PlaylistModes::NoSkipRestrictions; }

    virtual PlaylistModes::RetryMode retryMode() const { return PlaylistModes::NoRetry; }
    virtual quint32 retryInterval() const { return 30000; }

    virtual PlaylistModes::LatchMode latchMode() const { return m_latchMode; }
    virtual void setLatchMode( PlaylistModes::LatchMode latchMode ) { m_latchMode = latchMode; }

    virtual QString filter() const { return m_filter; }
    virtual void setFilter( const QString& pattern ) { m_filter = pattern; }

    virtual void reset() {}

    //TODO: Get rid of the next two functions once all playlsitinterfaces are factored out
    // Some playlist interfaces can wrap other interfaces. When checking for top-level
    // equality (say, to compare the currently playing interface) this might be needed
    virtual bool hasChildInterface( Tomahawk::playlistinterface_ptr ) { return false; }

public slots:
    virtual void setRepeatMode( PlaylistModes::RepeatMode mode ) = 0;
    virtual void setShuffled( bool enabled ) = 0;

signals:
    void repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode );
    void shuffleModeChanged( bool enabled );
    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );
    void latchModeChanged( Tomahawk::PlaylistModes::LatchMode mode );
    void nextTrackReady();

protected:
    virtual QList<Tomahawk::query_ptr> filterTracks( const QList<Tomahawk::query_ptr>& queries );

    PlaylistModes::LatchMode m_latchMode;
    bool m_finished;

private:
    Q_DISABLE_COPY( PlaylistInterface )

private:
    QString m_id;
    QString m_filter;
};

}

Q_DECLARE_METATYPE( Tomahawk::playlistinterface_ptr )

#endif // PLAYLISTINTERFACE_H
