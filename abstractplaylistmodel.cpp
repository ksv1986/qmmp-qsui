/***************************************************************************
 *   Copyright (C) 2009 by Ilya Kotov                                      *
 *   forkotov02@hotmail.ru                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QApplication>
#include <QChar>
#include <QDebug>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFont>
#include <QMimeData>
#include <QPalette>
#include <QSet>
#include <QUrl>

#include <qmmp/soundcore.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlisttrack.h>

#include "abstractplaylistmodel.h"

enum {
    columnPlaying = 0,
    columnTrack,
    columnArtist,
    columnTitle,
    columnYear,
    columnAlbum,
    columnGenre,
    columnLength,
    columnCount
};

AbstractPlaylistModel::AbstractPlaylistModel(PlayListModel *pl, PlayListManager *manager,
                                             SoundCore *core, QObject *parent)
    : QAbstractListModel(parent)
    , m_pl(pl)
    , m_pl_manager(manager)
    , m_core(core)
    , m_previousRow(-1)
{
    connect(m_pl, SIGNAL(listChanged(int)), SLOT(listChanged(int)));
    if (m_pl)
        m_previousRow = m_pl->currentIndex();
    connect(m_pl_manager, SIGNAL(selectedPlayListChanged(PlayListModel*,PlayListModel*)), SLOT(selectedPlayListChanged(PlayListModel*,PlayListModel*)));
    connect(m_core, SIGNAL(stateChanged(Qmmp::State)), SLOT(stateChanged(Qmmp::State)));
}

AbstractPlaylistModel::~AbstractPlaylistModel() {}

int AbstractPlaylistModel::columnCount (const QModelIndex &) const
{
    return ::columnCount;
}

void AbstractPlaylistModel::listChanged(int flags)
{
    if (flags & PlayListModel::STRUCTURE) {
        beginResetModel();
        endResetModel();
        return;
    }
    if (flags & PlayListModel::CURRENT) {
        emit dataChanged(index(m_previousRow, columnPlaying), index(m_previousRow, columnPlaying));
        m_previousRow = m_pl->currentIndex();
        emit dataChanged(index(m_previousRow, columnPlaying), index(m_previousRow, columnPlaying));
    }
}

void AbstractPlaylistModel::stateChanged(Qmmp::State)
{
    if (m_pl_manager->currentPlayList() == m_pl)
        emit dataChanged(index(m_pl->currentIndex(), columnPlaying), index(m_pl->currentIndex(), columnPlaying));
}

QVariant AbstractPlaylistModel::data (const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.row () < m_pl->count())
    {
        PlayListTrack *item = m_pl->track(index.row ());
        switch (index.column()) {
        case columnTrack:
            return (*item)[Qmmp::TRACK];
        case columnArtist:
            return (*item)[Qmmp::ARTIST];
        case columnTitle:
            return (*item)[Qmmp::TITLE].isEmpty()
                    ? QFileInfo(item->url()).fileName()
                    : (*item)[Qmmp::TITLE];
        case columnYear:
            return (*item)[Qmmp::YEAR];
        case columnAlbum:
            return (*item)[Qmmp::ALBUM];
        case columnGenre:
            return (*item)[Qmmp::GENRE];
        case columnLength:
            return item->formattedLength();
        }
    } else
    if (role == Qt::DecorationRole)
    {
        if (!m_pl->currentTrack())
            return QVariant();

        if (m_pl != m_pl_manager->currentPlayList())
            return QVariant();
        if (index.column() == columnPlaying && index.row() == m_pl->currentIndex()) {
            switch (m_core->state())
            {
            case Qmmp::Buffering:
            case Qmmp::Playing:
                return QPixmap(":/images/play.png");
            case Qmmp::Paused:
                return QPixmap(":/images/pause.png");
            case Qmmp::Stopped:
            case Qmmp::NormalError:
            case Qmmp::FatalError:
                return QPixmap(":/images/stop.png");
            }
        }
    }
    return QVariant();
}

QVariant AbstractPlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case columnTrack:
            return tr("#");
        case columnArtist:
            return tr("Artist");
        case columnTitle:
            return tr("Title");
        case columnYear:
            return tr("Year");
        case columnAlbum:
            return tr("Album");
        case columnGenre:
            return tr("Genre");
        case columnLength:
            return tr("Length");
        }
    }
    else if (role == Qt::DisplayRole)
    {
        return section;
    }
    return QVariant();
}

int AbstractPlaylistModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_pl->count();
}

QString AbstractPlaylistModel::formatTime(qint64 time) const
{
    return QString("%1:%2").arg(time/60).arg(time%60,2,10,QChar('0'));
}

void AbstractPlaylistModel::addItem(const QString& path)
{
    m_pl->add(path);
}

void AbstractPlaylistModel::insertItem(const QString &path, int row)
{
    QFileInfo file(path);
    if (file.isDir())
    {
        m_pl->add(path);
    }
    else
    {
        m_pl->add(path);
        m_pl->clearSelection();
        m_pl->setSelected(m_pl->count() - 1, true);
        m_pl->moveItems(m_pl->count() - 1, row);
    }
}

void AbstractPlaylistModel::sort(int column, Qt::SortOrder)
{
    int mode = 0;
    switch (column)
    {
    case columnTrack:
        mode = PlayListModel::TRACK;
        break;
    case columnArtist:
        mode = PlayListModel::ARTIST;
        break;
    case columnTitle:
        mode = PlayListModel::TITLE;
        break;
    case columnYear:
        mode = PlayListModel::DATE;
        break;
    case columnAlbum:
        mode = PlayListModel::ALBUM;
        break;
    default:
        return;
    }

    m_pl->sort(mode);
}

void AbstractPlaylistModel::showDetails()
{
    m_pl->showDetails();
}

PlayListTrack *AbstractPlaylistModel::selectedTrack() const
{
    int index = m_pl->firstSelectedLower(-1);
    if (index < 0)
        return NULL;
    return m_pl->track(index);
}

void AbstractPlaylistModel::openDirectory()
{
    PlayListTrack *selected = selectedTrack();
    if (!selected)
        return;

    QString path(selected->url());
    if (path.contains(":///")) { //pseudo-protocol
        path = QUrl(path).path();
        path.replace(QString(QUrl::toPercentEncoding("#")), "#");
        path.replace(QString(QUrl::toPercentEncoding("?")), "?");
        path.replace(QString(QUrl::toPercentEncoding("%")), "%");
    } else if (path.contains("://")) {
        return;
    }
    QFileInfo info(path);
    if (info.exists())
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
}

Qt::ItemFlags AbstractPlaylistModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
    {
        return Qt::ItemIsDragEnabled
               | Qt::ItemIsDropEnabled
               | QAbstractListModel::flags(index);
    }
    return  Qt::ItemIsDropEnabled
            | QAbstractListModel::flags(index);
}

Qt::DropActions AbstractPlaylistModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions AbstractPlaylistModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList AbstractPlaylistModel::mimeTypes() const
{
    return QStringList() << "text/uri-list";
}

QMimeData *AbstractPlaylistModel::mimeData(const QModelIndexList &indexes) const
{
    QList<int> rows;
    foreach(QModelIndex index, indexes)
    {
        if (index.isValid() && !rows.contains(index.row()))
        {
            rows << index.row();
        }
    }
    qSort(rows);
    QList<QUrl> urls;
    foreach(int row, rows)
    {
        PlayListTrack *item = m_pl->track(row);
        itemsToMove.append(item);
        urls << QUrl(item->url());
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);
    return mimeData;
}

bool AbstractPlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
        int row, int column, const QModelIndex &)
{
    Q_UNUSED(column)

    if (action == Qt::IgnoreAction)
        return true;
    if (!data->hasUrls())
        return false;

    if (row == -1)
        row = rowCount() - 1;

    beginResetModel();
    if(action == Qt::MoveAction && itemsToMove.count() > 0)
    {
        foreach(PlayListTrack *item, itemsToMove)
        {
            m_pl->clearSelection();
            int index = m_pl->items().indexOf(item);
            m_pl->setSelected(index, true);
            m_pl->moveItems(index, row);
            if (index > row)
                row ++;
        }
    }
    else
    {
        foreach(QUrl url, data->urls())
        {
            if (row >= 0)
                insertItem(url.toLocalFile(), row++);
            else
                addItem(url.toLocalFile());
        }
    }
    itemsToMove.clear();
    endResetModel();
    return true;
}

PlayListTrack *AbstractPlaylistModel::track(int row)
{
    return m_pl->track(row);
}

void AbstractPlaylistModel::removeAt(int row)
{
    m_pl->removeTrack(row);
}

void AbstractPlaylistModel::removeItem(PlayListTrack *track)
{
    m_pl->removeTrack(track);
}

void AbstractPlaylistModel::clearSelection()
{
    m_pl->clearSelection();
}

bool AbstractPlaylistModel::isSelected(int row)
{
    return m_pl->isSelected(row);
}

void AbstractPlaylistModel::setSelected(int row, bool selected)
{
    m_pl->setSelected(row, selected);
}

void AbstractPlaylistModel::selectAll()
{
    m_pl->selectAll();
}

const SimpleSelection& AbstractPlaylistModel::getSelection(int row)
{
    return m_pl->getSelection(row);
}

void AbstractPlaylistModel::setPlaylist(PlayListModel *model)
{
    disconnect(m_pl, SIGNAL(listChanged(int)), this, SLOT(listChanged(int)));
    m_pl = model;
    listChanged(PlayListModel::STRUCTURE);
    connect(m_pl, SIGNAL(listChanged(int)), SLOT(listChanged(int)));
}

void AbstractPlaylistModel::selectedPlayListChanged(PlayListModel *current, PlayListModel *previous)
{
    Q_UNUSED(previous)
    setPlaylist(current);
}
