#ifndef MYTABLEVIEW_H
#define MYTABLEVIEW_H

#include "baselistwidget.h"
#include <QTreeView>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMouseEvent;

/* QObject must go first in inheritance list or it would confuse moc */
class PlaylistView : private QTreeView, public BaseListWidget
{
    Q_OBJECT
public:
    explicit PlaylistView(PlayListModel *model, QWidget *parent = 0);
    ~PlaylistView();

    QWidget *widget() Q_DECL_OVERRIDE;
    void readSettings() Q_DECL_OVERRIDE;

private:
    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void startDrag(Qt::DropActions supportedActions) Q_DECL_OVERRIDE;
    void selectAll() Q_DECL_OVERRIDE;

private:
    void setup();

private:
    int m_anchor_row;
    int m_pressed_row;
    bool m_select_on_release;
    QPoint startPos;
};

#endif // MYTABLEVIEW_H
