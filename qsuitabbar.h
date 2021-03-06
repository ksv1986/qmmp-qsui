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

#ifndef QSUITABBAR_H
#define QSUITABBAR_H

#include <QTabBar>

class QAction;
class QActionGroup;
class QMenu;

/**
    @author Ilya Kotov <forkotov02@hotmail.ru>
*/
class QSUiTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit QSUiTabBar(QWidget *parent = 0);
    QMenu *menu() const;
    void setTabText(int index, const QString &text);
    void readSettings();

private slots:
    void updateActions();

protected:
    void tabInserted(int index) override;
    void tabRemoved(int index) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QMenu *m_menu;
    QActionGroup *m_group;
};

#endif // QSUITABBAR_H
