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
#include <QMenu>
#include <QMimeData>
#include <QSettings>
#include <QUrl>

#include <qmmp/soundcore.h>
#include <qmmpui/uihelper.h>

#include "abstractplaylistmodel.h"
#include "playlistview.h"

PlaylistView::PlaylistView(PlayListModel *_model, QWidget *parent)
        : QTreeView(parent)
        , BaseListWidget(_model, parent)
{
    QTreeView::setModel(new AbstractPlaylistModel(_model, PlayListManager::instance(), SoundCore::instance(), this));
    header()->setSectionsMovable(true);
    header()->setSectionsClickable(true);
    connect(header(), &QHeaderView::sectionClicked, this, [this](int section)
    {
        if (!isSortingEnabled())
        {
            setSortingEnabled(true);
            sortByColumn(section);
        }
    });
    connect(this, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        playAt(index.row());
    });
    setup();
}

PlaylistView::~PlaylistView()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("Simple");
    settings.setValue("pl_header_state", header()->saveState());
    settings.endGroup();
}

QWidget *PlaylistView::widget()
{
    return this;
}

void PlaylistView::readSettings()
{

    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("Simple");
    header()->restoreState(settings.value("pl_header_state", QByteArray("")).toByteArray());
    settings.endGroup();
}

void PlaylistView::setup()
{
    readSettings();
    header()->setContextMenuPolicy( Qt::ActionsContextMenu );

    for (int i = 0; i < model()->columnCount(); i++)
    {
        QAction *action = new QAction(model()->headerData(i, Qt::Horizontal).toString(), header());
        header()->addAction(action);
        action->setCheckable(true);
        if (!isColumnHidden(i))
            action->setChecked(true);
        connect(action, &QAction::toggled, this, [this, i](bool toggled) {
            if (toggled)
                showColumn(i);
            else
                hideColumn(i);
        });
    }
    setSelectionMode( ExtendedSelection );
    setContextMenuPolicy(Qt::CustomContextMenu);
    setRootIsDecorated(false);
    connect(this, &QTreeView::customContextMenuRequested, this, [this](const QPoint &point) {
        if (m_menu)
            m_menu->exec(viewport()->mapToGlobal(point));
    });
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
