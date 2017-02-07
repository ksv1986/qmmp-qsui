#ifndef BASELISTWIDGET_H
#define BASELISTWIDGET_H

#include <QList>

class KeyboardManager;
class PlayListModel;
class QAction;
class QMenu;
class QWidget;

class BaseListWidget
{
public:
    explicit BaseListWidget(PlayListModel *model, QWidget *parent = 0);
    virtual ~BaseListWidget() {}

    void playAt(int index);
    QMenu *menu() const;
    void setMenu(QMenu *menu);
    PlayListModel *playlistModel() const;

    virtual void setPlaylistModel(PlayListModel *newModel);
    virtual QList<QAction*> actions();

    virtual QWidget *widget() = 0;
    virtual void readSettings() = 0;

protected:
    QMenu *m_menu;
    PlayListModel *m_model;
};

#endif // BASELISTWIDGET_H
