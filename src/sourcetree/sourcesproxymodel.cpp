/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "sourcesproxymodel.h"

#include <QTreeView>

#include "sourcelist.h"
#include "sourcesmodel.h"
#include "sourcetree/items/collectionitem.h"

#include "utils/logger.h"


SourcesProxyModel::SourcesProxyModel( SourcesModel* model, QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( model )
    , m_filtered( false )
{
    setDynamicSortFilter( true );
    setSortRole( SourcesModel::SortRole );

    setSourceModel( model );

    if ( model && model->metaObject()->indexOfSignal( "expandRequest(QPersistentModelIndex)" ) > -1 )
        connect( model, SIGNAL( expandRequest( QPersistentModelIndex ) ), this, SLOT( expandRequested( QPersistentModelIndex ) ), Qt::QueuedConnection );
    if ( model && model->metaObject()->indexOfSignal( "selectRequest(QPersistentModelIndex)" ) > -1 )
        connect( model, SIGNAL( selectRequest( QPersistentModelIndex ) ), this, SLOT( selectRequested( QPersistentModelIndex ) ), Qt::QueuedConnection );
}


void
SourcesProxyModel::showOfflineSources( bool offlineSourcesShown )
{
    m_filtered = !offlineSourcesShown;
    invalidateFilter();
}


bool
SourcesProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( !m_filtered )
        return true;

    CollectionItem* sti = qobject_cast< CollectionItem* >( m_model->data( sourceModel()->index( sourceRow, 0, sourceParent ), SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
    if ( sti )
    {
        if ( sti->source().isNull() || sti->source()->isOnline() )
            return true;
        else if ( m_model->sourcesWithViewPage().contains( sti->source() ) )
            return true;
        else
            return false;
    }
    // accept rows that aren't sources
    return true;
}


void
SourcesProxyModel::selectRequested( const QPersistentModelIndex& idx )
{
    qDebug() << "selectRequested for idx" << idx << idx.data(Qt::DisplayRole).toString() << mapFromSource( idx ) << mapFromSource( idx ).data(Qt::DisplayRole).toString();
    emit selectRequest( QPersistentModelIndex( mapFromSource( idx ) ) );
}


void
SourcesProxyModel::expandRequested( const QPersistentModelIndex& idx )
{
    qDebug() << "emitting expand for idx" << idx << idx.data(Qt::DisplayRole).toString() << mapFromSource( idx ) << mapFromSource( idx ).data(Qt::DisplayRole).toString();
    emit expandRequest( QPersistentModelIndex( mapFromSource( idx ) ) );
}


bool
SourcesProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    if ( m_model->data( left, SourcesModel::SortRole ) != m_model->data( right, SourcesModel::SortRole ) )
        return ( m_model->data( left, SourcesModel::SortRole ).toInt() < m_model->data( right, SourcesModel::SortRole ).toInt() );

    const QString& lefts = left.data().toString().toLower();
    const QString& rights = right.data().toString().toLower();

    if ( lefts == rights )
        return ( m_model->data( left, SourcesModel::IDRole ).toInt() < m_model->data( right, SourcesModel::IDRole ).toInt() );
    else
        return QString::localeAwareCompare( lefts, rights ) < 0;
}
