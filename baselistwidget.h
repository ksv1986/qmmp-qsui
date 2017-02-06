#ifndef BASELISTWIDGET_H
#define BASELISTWIDGET_H

#include <QWidget>

class KeyboardManager;
class PlayListModel;
class QMenu;

class BaseListWidget : public QWidget
{
    Q_OBJECT
public:
    BaseListWidget(PlayListModel *model, QWidget *parent = 0);

    QMenu *menu() const;
    void setMenu(QMenu *menu);
    PlayListModel *model() const;
    virtual void setModel(PlayListModel *newModel);
    virtual QList<QAction*> actions();

signals:

public slots:
    virtual void readSettings() = 0;

protected:
    QMenu *m_menu;
    PlayListModel *m_model;
};

#endif // BASELISTWIDGET_H
