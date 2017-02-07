#include "baselistwidget.h"
#include <qmmpui/mediaplayer.h>

BaseListWidget::BaseListWidget(PlayListModel *model, QWidget */*parent*/)
    : m_menu(nullptr)
    , m_model(model)
{
    Q_ASSERT(m_model);
}

QMenu *BaseListWidget::menu() const
{
    return m_menu;
}

void BaseListWidget::setMenu(QMenu *menu)
{
    m_menu = menu;
}

PlayListModel *BaseListWidget::playlistModel() const
{
    Q_ASSERT(m_model);
    return m_model;
}

void BaseListWidget::setPlaylistModel(PlayListModel *newModel)
{
    m_model = newModel;
}

QList<QAction *> BaseListWidget::actions()
{
    return {};
}

void BaseListWidget::playAt(int index)
{
    m_model->setCurrent(index);
    MediaPlayer *player = MediaPlayer::instance();
    player->playListManager()->selectPlayList(m_model);
    player->playListManager()->activatePlayList(m_model);
    player->stop();
    player->play();
}
