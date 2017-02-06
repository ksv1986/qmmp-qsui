#include "baselistwidget.h"

BaseListWidget::BaseListWidget(PlayListModel *model, QWidget *parent)
    : QWidget(parent)
    , m_menu(nullptr)
    , m_model(model)
{
}

QMenu *BaseListWidget::menu() const
{
    return m_menu;
}

void BaseListWidget::setMenu(QMenu *menu)
{
    m_menu = menu;
}

PlayListModel *BaseListWidget::model() const
{
    Q_ASSERT(m_model);
    return m_model;
}

void BaseListWidget::setModel(PlayListModel *newModel)
{
    m_model = newModel;
}

QList<QAction *> BaseListWidget::actions()
{
    return {};
}
