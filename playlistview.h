#ifndef MYTABLEVIEW_H
#define MYTABLEVIEW_H

#include <QTreeView>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMouseEvent;

class PlaylistView : public QTreeView
{
    Q_OBJECT
public:
    PlaylistView(QWidget *parent = 0);
    virtual ~PlaylistView();
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void selectAll();

    void setup();

private slots:
    void toggleColumn(bool toggled);
    void sectionClicked(int section);
    void scrollToIndex(const QModelIndex& index);
    void removeSelected();

private:
    int m_anchor_row;
    int m_pressed_row;
    bool m_select_on_release;
    QPoint startPos;

    QList<QAction*> m_columnActions; //! Maintains the mapping action<->column
};

#endif // MYTABLEVIEW_H
