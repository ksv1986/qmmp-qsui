/***************************************************************************
 *   Copyright (C) 2012 by Ilya Kotov                                      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "qsuitabbar.h"
#include <qmmp/qmmp.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QSettings>

QSUiTabBar::QSUiTabBar(QWidget *parent) : QTabBar(parent)
{
    setMovable(true);
    m_menu = new QMenu(this);
    m_group = new QActionGroup(this);
    m_group->setExclusive(true);
    connect(this, &QSUiTabBar::tabMoved, [this](int, int)
    {
        updateActions();
    });
    connect(this, &QSUiTabBar::currentChanged, [this](int index)
    {
        if (index >= m_menu->actions().count())
            return;
        m_menu->actions().at(index)->setChecked(true);
    });
    connect(m_menu, &QMenu::triggered, this, [this](QAction *action)
    {
        setCurrentIndex(m_menu->actions().indexOf(action));
    });
    readSettings();
}

QMenu *QSUiTabBar::menu() const
{
    return m_menu;
}

void QSUiTabBar::tabInserted(int index)
{
    QAction *action = new QAction(m_menu);
    action->setCheckable(true);
    action->setActionGroup(m_group);
    action->setText(tabText(index));

    if (m_menu->actions().isEmpty() || index == m_menu->actions().count())
    {
        m_menu->addAction(action);
    }
    else
    {
        QAction *before = m_menu->actions().at(index);
        m_menu->insertAction(before, action);
    }
    if (currentIndex() == index)
        action->setChecked(true);
    QTabBar::tabInserted(index);
}

void QSUiTabBar::tabRemoved(int index)
{
    QAction *a = m_menu->actions().at(index);
    m_menu->removeAction(a);
    delete a;
    QTabBar::tabRemoved(index);
}

void QSUiTabBar::setTabText(int index, const QString &text)
{
    QTabBar::setTabText(index, text);
    m_menu->actions().at(index)->setText(text);
}

void QSUiTabBar::readSettings()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("Simple");
    QFont tab_font = qApp->font(this);
    if(!settings.value("use_system_fonts", true).toBool())
    {
        tab_font.fromString(settings.value("pl_tabs_font", tab_font.toString()).toString());
    }
    setFont(tab_font);
}

void QSUiTabBar::updateActions()
{
    for (int i = 0; i < m_menu->actions().size(); ++i)
    {
         m_menu->actions().at(i)->setText(tabText(i));
    }
    m_menu->actions().at(currentIndex())->setChecked(true);
}

void QSUiTabBar::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::MidButton)
    {
        int i = tabAt(e->pos());
        if(i >= 0)
        {
            e->accept();
            emit tabCloseRequested(i);
        }
    }
    QTabBar::mouseReleaseEvent(e);
}
