#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QHeaderView>
#include <QMap>
#include <QMimeData>
#include <QUrl>

#include <qmmpui/uihelper.h>

#include "playlistview.h"
#include "abstractplaylistmodel.h"
#include "settings.h"

PlaylistView::PlaylistView(QWidget *parent)
        : QTreeView(parent)
{
    header()->setSectionsMovable(true);
    header()->setSectionsClickable(true);
    connect(header(), SIGNAL(sectionClicked(int)), this, SLOT(sectionClicked(int)));
}

PlaylistView::~PlaylistView()
{
    Settings::instance().setPlaylistState(header()->saveState());
}

void PlaylistView::setup()
{
    header()->restoreState(Settings::instance().playlistState());
    header()->setContextMenuPolicy( Qt::ActionsContextMenu );

    for ( int i = 0; i < model()->columnCount(); i++ )
    {
        QAction *action = new QAction( model()->headerData( i, Qt::Horizontal ).toString(), header() );
        header()->addAction( action );
        m_columnActions.append( action );
        action->setCheckable( true );
        if ( !isColumnHidden( i ) )
            action->setChecked( true );
        connect( action, SIGNAL( toggled(bool) ), this, SLOT(toggleColumn(bool) ) );
    }

    setContextMenuPolicy( Qt::ActionsContextMenu );

    QAction *action = new QAction( tr("View Track Details"), header() );
    action->setShortcut(tr("Ctrl+I"));
    addAction( action );

    QAction *openDir = new QAction( tr("Open Item Directory"), header() );
    openDir->setShortcut(QKeySequence("Ctrl+D"));
    addAction( openDir );

    addActions(UiHelper::instance()->actions(UiHelper::PLAYLIST_MENU));

    AbstractPlaylistModel *playlist = qobject_cast<AbstractPlaylistModel*>(model());
    connect( action, SIGNAL(triggered()), playlist, SLOT(showDetails()));
    connect(openDir, SIGNAL(triggered()), playlist, SLOT(openDirectory()));
    connect(playlist, SIGNAL(currentChanged(QModelIndex)), this, SLOT(scrollToIndex(QModelIndex)));
}

void PlaylistView::scrollToIndex(const QModelIndex &index)
{
    scrollTo(index);
}

void PlaylistView::sectionClicked(int section)
{
    if (!isSortingEnabled())
    {
        setSortingEnabled(true);
        sortByColumn(section);
    }
}

void PlaylistView::toggleColumn(bool toggled)
{
    int index = m_columnActions.indexOf(qobject_cast< QAction* >(sender()));
    if ( index != -1 )
    {
        if (toggled)
            showColumn(index);
        else
            hideColumn(index);
    }
}

void PlaylistView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void PlaylistView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
    QTreeView::dragMoveEvent(event);
}

void PlaylistView::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        int row = indexAt(event->pos()).row();
        Qt::DropAction action;
        if (event->source() == this) {
            action = Qt::MoveAction;
        } else {
            action = event->proposedAction();
        }
        model()->dropMimeData(event->mimeData(), action, row, 0, QModelIndex());
        event->setDropAction(action);
        event->accept();
    } else {
        event->ignore();
    }
}

void PlaylistView::selectAll()
{
    AbstractPlaylistModel *playlist = qobject_cast<AbstractPlaylistModel*>(model());
    playlist->selectAll();
    QTreeView::selectAll();
}

void PlaylistView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        startPos = e->pos();

    AbstractPlaylistModel *playlist = qobject_cast<AbstractPlaylistModel*>(model());
    int row = indexAt(e->pos()).row();
    if (playlist->rowCount() > row)
    {
        if (!(Qt::ControlModifier & e->modifiers () ||
                Qt::ShiftModifier & e->modifiers () ||
                playlist->isSelected(row)))
            playlist->clearSelection();

        if (playlist->isSelected(row) && (e->modifiers() == Qt::NoModifier))
            m_select_on_release = true;

        m_pressed_row = row;
        if ((Qt::ShiftModifier & e->modifiers()))
        {

            if (m_pressed_row > m_anchor_row)
            {
                playlist->clearSelection();
                for (int j = m_anchor_row; j <= m_pressed_row; j++)
                {
                    playlist->setSelected(j, true);
                }
            }
            else
            {
                playlist->clearSelection();
                for (int j = m_anchor_row; j >= m_pressed_row; j--)
                {
                    playlist->setSelected(j, true);
                }
            }
        }
        else
        {
            if (!playlist->isSelected(row) || (Qt::ControlModifier & e->modifiers()))
                playlist->setSelected(row, !playlist->isSelected(row));
        }

        if (playlist->getSelection(m_pressed_row).count() == 1)
            m_anchor_row = m_pressed_row;

        update();
    }
    QTreeView::mousePressEvent(e);
}

void PlaylistView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton & (event->modifiers() == Qt::NoModifier))
    {
        int distance = (event->pos() - startPos).manhattanLength();
        if (distance >= QApplication::startDragDistance())
            startDrag(model()->supportedDragActions());
    }
}

void PlaylistView::startDrag(Qt::DropActions supportedActions)
{
    QModelIndexList selection = selectedIndexes();
    if (selection.count())
    {
        QMimeData *mimeData = model()->mimeData(selectedIndexes());
        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec(supportedActions, Qt::CopyAction);
    }
}

void PlaylistView::removeSelected()
{
    QList<int> rowList;
    foreach(QModelIndex rowItem, selectedIndexes())
    {
        if (rowItem.column() == 0)
        {
            rowList.push_front(rowItem.row());
        }
    }

    AbstractPlaylistModel *playlist = qobject_cast<AbstractPlaylistModel*>(model());
    foreach(int row, rowList)
        playlist->removeAt(row);
}
