/***************************************************************************
 *   Copyright (C) 2009-2015 by Ilya Kotov                                 *
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
#include <QPushButton>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QTreeView>
#include <QMessageBox>
#include <QSignalMapper>
#include <QMenu>
#include <QSettings>
#include <QInputDialog>
#include <qmmp/soundcore.h>
#include <qmmp/decoder.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/general.h>
#include <qmmpui/playlistparser.h>
#include <qmmpui/playlistformat.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/configdialog.h>
#include <qmmpui/qmmpuisettings.h>
#include "toolbareditor.h"
#include "actionmanager.h"
#include "qsuianalyzer.h"
#include "visualmenu.h"
#include "listwidget.h"
#include "positionslider.h"
#include "mainwindow.h"
#include "qsuisettings.h"
#include "hotkeyeditor.h"
#include "filesystembrowser.h"
#include "aboutqsuidialog.h"
#include "keyboardmanager.h"
#include "coverwidget.h"
#include "playlistbrowser.h"
#include "equalizer.h"

#define KEY_OFFSET 10000

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_ui.setupUi(this);
    m_balance = 0;
    m_update = false;
    m_wasMaximized = false;
    m_titleFormatter.setPattern("%if(%p,%p - %t,%t)");
    //qmmp objects
    m_player = MediaPlayer::instance();
    m_core = SoundCore::instance();
    m_pl_manager = PlayListManager::instance();
    m_uiHelper = UiHelper::instance();
    m_ui_settings = QmmpUiSettings::instance();
    connect(m_uiHelper, SIGNAL(toggleVisibilityCalled()), SLOT(toggleVisibility()));
    connect(m_uiHelper, SIGNAL(showMainWindowCalled()), SLOT(showAndRaise()));
    m_visMenu = new VisualMenu(this); //visual menu
    m_ui.actionVisualization->setMenu(m_visMenu);
    m_pl_menu = new QMenu(this); //playlist menu
    new ActionManager(this); //action manager
    //status
    connect(m_core, SIGNAL(elapsedChanged(qint64)), SLOT(updatePosition(qint64)));
    connect(m_core, SIGNAL(stateChanged(Qmmp::State)), SLOT(showState(Qmmp::State)));
    connect(m_core, SIGNAL(bitrateChanged(int)), SLOT(updateStatus()));
    connect(m_core, SIGNAL(bufferingProgress(int)), SLOT(showBuffering(int)));
    connect(m_core, SIGNAL(metaDataChanged()), SLOT(showMetaData()));
    //keyboard manager
    m_key_manager = new KeyboardManager(this);
    //create tabs
    foreach(PlayListModel *model, m_pl_manager->playLists())
    {
        ListWidget *list = new ListWidget(model, this);
        list->setMenu(m_pl_menu);
        if(m_pl_manager->currentPlayList() != model)
            m_ui.tabWidget->addTab(list, model->name());
        else
        {
            m_ui.tabWidget->addTab(list, "[" + model->name() + "]");
            m_ui.tabWidget->setCurrentWidget(list);
        }
        if(model == m_pl_manager->selectedPlayList())
        {
            m_ui.tabWidget->setCurrentWidget(list);
            m_key_manager->setListWidget(list);
        }
        connect(model, SIGNAL(nameChanged(QString)), SLOT(updateTabs()));
    }
    m_slider = new PositionSlider(this);
    m_slider->setFocusPolicy(Qt::NoFocus);
    m_slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_ui.progressToolBar->addWidget(m_slider);
    //prepare visualization
    Visual::initialize(this, m_visMenu, SLOT(updateActions()));
    //playlist manager
    connect(m_slider, SIGNAL(sliderReleased()), SLOT(seek()));
    connect(m_pl_manager, SIGNAL(currentPlayListChanged(PlayListModel*,PlayListModel*)),
            SLOT(updateTabs()));
    connect(m_pl_manager, SIGNAL(selectedPlayListChanged(PlayListModel*,PlayListModel*)),
            SLOT(updateTabs()));
    connect(m_pl_manager, SIGNAL(playListRemoved(int)), SLOT(removeTab(int)));
    connect(m_pl_manager, SIGNAL(playListAdded(int)), SLOT(addTab(int)));
    connect(m_ui.tabWidget,SIGNAL(currentChanged(int)), m_pl_manager, SLOT(selectPlayList(int)));
    connect(m_ui.tabWidget, SIGNAL(tabCloseRequested(int)), m_pl_manager, SLOT(removePlayList(int)));
    connect(m_ui.tabWidget, SIGNAL(tabMoved(int,int)), m_pl_manager, SLOT(move(int,int)));
    connect(m_ui.tabWidget, SIGNAL(createPlayListRequested()), m_pl_manager, SLOT(createPlayList()));

    m_ui.tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.tabWidget, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showTabMenu(QPoint)));
    m_tab_menu = new QMenu(m_ui.tabWidget);
    //status bar
    m_timeLabel = new QLabel(this);
    m_statusLabel = new QLabel(this);
    m_ui.statusbar->addPermanentWidget(m_statusLabel, 0);
    m_ui.statusbar->addPermanentWidget(m_timeLabel, 1);
    //volume
    m_ui.progressToolBar->addSeparator();
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setFocusPolicy(Qt::NoFocus);
    m_volumeSlider->setFixedWidth(100);
    m_volumeSlider->setRange(0,100);
    QIcon volumeIcon = QIcon::fromTheme("audio-volume-high", QIcon(":/qsui/audio-volume-high.png"));
    m_volumeAction = m_ui.progressToolBar->addAction(volumeIcon, tr("Volume"));
    m_volumeAction->setCheckable(true);
    connect(m_volumeAction, SIGNAL(triggered(bool)), m_core, SLOT(setMuted(bool)));
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), m_core, SLOT(setVolume(int)));
    connect(m_core, SIGNAL(volumeChanged(int)), m_volumeSlider, SLOT(setValue(int)));
    connect(m_core, SIGNAL(volumeChanged(int)), SLOT(updateVolumeIcon()));
    connect(m_core, SIGNAL(mutedChanged(bool)), SLOT(updateVolumeIcon()));
    connect(m_core, SIGNAL(mutedChanged(bool)), m_volumeAction, SLOT(setChecked(bool)));
    m_volumeSlider->setValue(m_core->volume());
    updateVolumeIcon();
    m_ui.progressToolBar->addWidget(m_volumeSlider);
    //visualization
    m_analyzer = new QSUiAnalyzer(this);
    m_ui.analyzerDockWidget->setWidget(m_analyzer);
    Visual::add(m_analyzer);
    //filesystem browser
    m_ui.fileSystemDockWidget->setWidget(new FileSystemBrowser(this));
    //cover
    m_ui.coverDockWidget->setWidget(new CoverWidget(this));
    //playlists
    m_ui.playlistsDockWidget->setWidget(new PlayListBrowser(m_pl_manager, this));

    createActions();
    createButtons();
    readSettings();
    updateStatus();
}

MainWindow::~MainWindow()
{
}

void MainWindow::addDir()
{
    m_uiHelper->addDirectory(this);
}

void MainWindow::addFiles()
{
    m_uiHelper->addFile(this);
}

void MainWindow::playFiles()
{
    m_uiHelper->playFiles(this);
}

void MainWindow::addUrl()
{
    m_uiHelper->addUrl(this);
}

void MainWindow::updatePosition(qint64 pos)
{
    m_slider->setMaximum(m_core->totalTime()/1000);
    if(!m_slider->isSliderDown())
        m_slider->setValue(pos/1000);

    QString text = MetaDataFormatter::formatLength(pos/1000);
    if(m_core->totalTime() > 0)
    {
        text.append("/");
        text.append(MetaDataFormatter::formatLength(m_core->totalTime()/1000));
    }
    m_timeLabel->setText(text);
}

void MainWindow::seek()
{
    m_core->seek(m_slider->value()*1000);
}

void MainWindow::showState(Qmmp::State state)
{
    switch((int) state)
    {
    case Qmmp::Playing:
    {
        updateStatus();
        m_analyzer->start();
        m_analyzer->setCover(MetaDataManager::instance()->getCover(m_core->url()));
        CoverWidget *cw = qobject_cast<CoverWidget *>(m_ui.coverDockWidget->widget());
        cw->setCover(MetaDataManager::instance()->getCover(m_core->url()));
        break;
    }
    case Qmmp::Paused:
        updateStatus();
        break;
    case Qmmp::Stopped:
        updateStatus();
        m_analyzer->stop();
        m_timeLabel->clear();
        m_slider->setValue(0);
        m_analyzer->clearCover();
        qobject_cast<CoverWidget *>(m_ui.coverDockWidget->widget())->clearCover();
        setWindowTitle("Qmmp");
        break;
    default:
        ;
    }
}

void MainWindow::updateTabs()
{
    for(int i = 0; i < m_pl_manager->count(); ++i)
    {
        PlayListModel *model = m_pl_manager->playListAt(i);
        if(model == m_pl_manager->currentPlayList())
            m_ui.tabWidget->setTabText(i, "[" + model->name() + "]");
        else
            m_ui.tabWidget->setTabText(i, model->name());
        //hack for displaying '&'
        m_ui.tabWidget->setTabText(i, m_ui.tabWidget->tabText(i).replace("&", "&&"));
        if(model == m_pl_manager->selectedPlayList())
        {
            m_ui.tabWidget->setCurrentIndex(i);
            m_key_manager->setListWidget(qobject_cast<ListWidget *>(m_ui.tabWidget->widget(i)));
        }
    }
}

void MainWindow::removePlaylist()
{
    m_pl_manager->removePlayList(m_pl_manager->selectedPlayList());
}

void MainWindow::removePlaylistWithIndex(int index)
{
    m_pl_manager->removePlayList(m_pl_manager->playListAt(index));
}

void MainWindow::addTab(int index)
{
    ListWidget *list = new ListWidget(m_pl_manager->playListAt(index), this);
    list->setMenu(m_pl_menu);
    m_ui.tabWidget->insertTab(index, list, m_pl_manager->playListAt(index)->name());
    connect(m_pl_manager->playListAt(index), SIGNAL(nameChanged(QString)), SLOT(updateTabs()));
    updateTabs();
}

void MainWindow::removeTab(int index)
{
    m_ui.tabWidget->widget(index)->deleteLater();
    m_ui.tabWidget->removeTab(index);
    updateTabs();
}

void MainWindow::renameTab()
{
    bool ok = false;
    QString name = QInputDialog::getText (this,
                                          tr("Rename Playlist"), tr("Playlist name:"),
                                          QLineEdit::Normal,
                                          m_pl_manager->selectedPlayList()->name(), &ok);
    if(ok)
        m_pl_manager->selectedPlayList()->setName(name);
}

void MainWindow::aboutUi()
{
    AboutQSUIDialog dialog(this);
    dialog.exec();
}

void MainWindow::about()
{
    m_uiHelper->about(this);
}

void MainWindow::toggleVisibility()
{
    if(isHidden() || isMinimized())
        showAndRaise();
    else
        hide();
}

void MainWindow::showAndRaise()
{
    show();
    if(m_wasMaximized)
        showMaximized();
    else
        showNormal();
    raise();
    activateWindow();
}

void MainWindow::showSettings()
{
    ConfigDialog *confDialog = new ConfigDialog(this);
    QSUISettings *simpleSettings = new QSUISettings(this);
    confDialog->addPage(tr("Appearance"), simpleSettings, QIcon(":/qsui/qsui_settings.png"));
    confDialog->addPage(tr("Shortcuts"), new HotkeyEditor(this), QIcon(":/qsui/qsui_shortcuts.png"));
    confDialog->exec();
    simpleSettings->writeSettings();
    confDialog->deleteLater();
    readSettings();
    ActionManager::instance()->saveActions();
    m_analyzer->readSettings();
}

void MainWindow::updateVolumeIcon()
{
    int maxVol = m_core->volume();

    QString iconName = "audio-volume-high";
    if(maxVol == 0 || m_core->isMuted())
        iconName = "audio-volume-muted";
    else if(maxVol < 30)
        iconName = "audio-volume-low";
    else if(maxVol >= 30 && maxVol < 60)
        iconName = "audio-volume-medium";

    m_volumeAction->setIcon(QIcon::fromTheme(iconName, QIcon(QString(":/qsui/") + iconName + ".png")));
}

void MainWindow::jumpTo()
{
    m_uiHelper->jumpToTrack(this);
}

void MainWindow::playPause()
{
    if (m_core->state() == Qmmp::Playing)
        m_core->pause();
    else
        m_player->play();
}

void MainWindow::updateStatus()
{
    int tracks = m_pl_manager->currentPlayList()->trackCount();
    int length = m_pl_manager->currentPlayList()->totalLength();

    if(m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused)
    {
        m_statusLabel->setText(tr("<b>%1</b>|%2 bit|%3 ch|%4 Hz|tracks: %5|total time: %6|%7 kbps|")
                               .arg(m_core->state() == Qmmp::Playing ? tr("Playing") : tr("Paused"))
                               .arg(m_core->sampleSize())
                               .arg(m_core->channels())
                               .arg(m_core->frequency())
                               .arg(tracks)
                               .arg(MetaDataFormatter::formatLength(length))
                               .arg(m_core->bitrate()));
    }
    else if(m_core->state() == Qmmp::Stopped)
    {
        m_statusLabel->setText(tr("<b>%1</b>|tracks: %2|total time: %3|")
                               .arg(tr("Stopped"))
                               .arg(tracks)
                               .arg(MetaDataFormatter::formatLength(length)));
    }
    else
        m_statusLabel->clear();
}

void MainWindow::closeEvent(QCloseEvent *)
{
    if(!m_hideOnClose || !m_uiHelper->visibilityControl())
        m_uiHelper->exit();

}

void MainWindow::hideEvent(QHideEvent *)
{
    writeSettings();
    m_wasMaximized = isMaximized();
}

void MainWindow::createActions()
{
    //preprare cheackable actions
    ACTION(ActionManager::REPEAT_ALL)->setChecked(m_ui_settings->isRepeatableList());
    ACTION(ActionManager::REPEAT_TRACK)->setChecked(m_ui_settings->isRepeatableTrack());
    ACTION(ActionManager::SHUFFLE)->setChecked(m_ui_settings->isShuffle());
    ACTION(ActionManager::NO_PL_ADVANCE)->setChecked(m_ui_settings->isNoPlayListAdvance());

    connect(m_ui_settings, SIGNAL(repeatableListChanged(bool)),
            ACTION(ActionManager::REPEAT_ALL), SLOT(setChecked(bool)));
    connect(m_ui_settings, SIGNAL (repeatableTrackChanged(bool)),
            ACTION(ActionManager::REPEAT_TRACK), SLOT(setChecked(bool)));
    connect(m_ui_settings, SIGNAL (noPlayListAdvanceChanged(bool)),
            ACTION(ActionManager::NO_PL_ADVANCE), SLOT(setChecked(bool)));
    connect(m_ui_settings, SIGNAL(shuffleChanged(bool)),
            ACTION(ActionManager::SHUFFLE), SLOT(setChecked(bool)));
    //register external actions
    ActionManager::instance()->registerAction(ActionManager::UI_ANALYZER,
                                              m_ui.analyzerDockWidget->toggleViewAction(),
                                              "analyzer", "");
    ActionManager::instance()->registerAction(ActionManager::UI_FILEBROWSER,
                                              m_ui.fileSystemDockWidget->toggleViewAction(),
                                              "file_browser", tr("Ctrl+0"));
    ActionManager::instance()->registerAction(ActionManager::UI_COVER,
                                              m_ui.coverDockWidget->toggleViewAction(),
                                              "cover", "");
    ActionManager::instance()->registerAction(ActionManager::UI_PLAYLISTBROWSER,
                                              m_ui.playlistsDockWidget->toggleViewAction(),
                                              "playlist_browser", tr("P"));
    //playback
    SET_ACTION(ActionManager::PREVIOUS, m_player, SLOT(previous()));
    SET_ACTION(ActionManager::PLAY, m_player, SLOT(play()));
    SET_ACTION(ActionManager::PAUSE, m_core, SLOT(pause()));
    SET_ACTION(ActionManager::STOP, m_player, SLOT(stop()));
    SET_ACTION(ActionManager::NEXT, m_player, SLOT(next()));
    SET_ACTION(ActionManager::EJECT,this, SLOT(playFiles()));

    //file menu
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_ADD_FILE, this, SLOT(addFiles())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_ADD_DIRECTORY, this, SLOT(addDir())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_ADD_URL, this, SLOT(addUrl())));
    m_ui.menuFile->addSeparator();
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_NEW, m_pl_manager, SLOT(createPlayList())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_CLOSE, this, SLOT(removePlaylist())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_RENAME, this, SLOT(renameTab())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_SELECT_NEXT, m_pl_manager,
                                        SLOT(selectNextPlayList())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_SELECT_PREVIOUS, m_pl_manager,
                                        SLOT(selectPreviousPlayList())));
    m_ui.menuFile->addSeparator();
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_LOAD, this, SLOT(loadPlayList())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_SAVE, this, SLOT(savePlayList())));
    m_ui.menuFile->addSeparator();
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::QUIT, m_uiHelper, SLOT(exit())));
    //edit menu
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_SELECT_ALL, m_pl_manager, SLOT(selectAll())));
    m_ui.menuEdit->addSeparator();
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_SELECTED, m_pl_manager,
                                        SLOT(removeSelected())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_UNSELECTED, m_pl_manager,
                                        SLOT(removeUnselected())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_ALL, m_pl_manager, SLOT(clear())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_INVALID, m_pl_manager,
                                        SLOT(removeInvalidTracks())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_DUPLICATES, m_pl_manager,
                                        SLOT(removeDuplicates())));
    m_ui.menuEdit->addSeparator();
    //view menu
    m_ui.menuView->addAction(SET_ACTION(ActionManager::WM_ALLWAYS_ON_TOP, this, SLOT(readSettings())));
    m_ui.menuView->addSeparator();
    m_ui.menuView->addAction(m_ui.analyzerDockWidget->toggleViewAction());
    m_ui.menuView->addAction(m_ui.fileSystemDockWidget->toggleViewAction());
    m_ui.menuView->addAction(m_ui.coverDockWidget->toggleViewAction());
    m_ui.menuView->addAction(m_ui.playlistsDockWidget->toggleViewAction());
    m_ui.menuView->addSeparator();
    m_ui.menuView->addAction(SET_ACTION(ActionManager::UI_SHOW_TABS, m_ui.tabWidget, SLOT(setTabsVisible(bool))));
    m_ui.menuView->addAction(SET_ACTION(ActionManager::UI_SHOW_TITLEBARS, this, SLOT(setTitleBarsVisible(bool))));
    m_ui.menuView->addAction(ACTION(ActionManager::PL_SHOW_HEADER));
    m_ui.menuView->addSeparator();
    m_ui.menuView->addAction(SET_ACTION(ActionManager::UI_BLOCK_TOOLBARS, this, SLOT(setToolBarsBlocked(bool))));
    m_ui.menuView->addAction(tr("Edit Toolbar"), this, SLOT(editToolBar()));

    QMenu* sort_mode_menu = new QMenu (tr("Sort List"), this);
    sort_mode_menu->setIcon(QIcon::fromTheme("view-sort-ascending"));
    QSignalMapper* signalMapper = new QSignalMapper (this);
    QAction* titleAct = sort_mode_menu->addAction (tr ("By Title"));
    connect (titleAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (titleAct, PlayListModel::TITLE);

    QAction* albumAct = sort_mode_menu->addAction (tr ("By Album"));
    connect (albumAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (albumAct, PlayListModel::ALBUM);

    QAction* artistAct = sort_mode_menu->addAction (tr ("By Artist"));
    connect (artistAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (artistAct, PlayListModel::ARTIST);

    QAction* albumArtistAct = sort_mode_menu->addAction (tr ("By Album Artist"));
    connect (albumArtistAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (albumArtistAct, PlayListModel::ALBUMARTIST);

    QAction* nameAct = sort_mode_menu->addAction (tr ("By Filename"));
    connect (nameAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (nameAct, PlayListModel::FILENAME);

    QAction* pathnameAct = sort_mode_menu->addAction (tr ("By Path + Filename"));
    connect (pathnameAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (pathnameAct, PlayListModel::PATH_AND_FILENAME);

    QAction* dateAct = sort_mode_menu->addAction (tr ("By Date"));
    connect (dateAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (dateAct, PlayListModel::DATE);

    QAction* trackAct = sort_mode_menu->addAction (tr("By Track Number"));
    connect (trackAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (trackAct, PlayListModel::TRACK);

    QAction* discAct = sort_mode_menu->addAction (tr("By Disc Number"));
    connect (discAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (discAct, PlayListModel::DISCNUMBER);

    QAction* fileCreationDateAct = sort_mode_menu->addAction (tr("By File Creation Date"));
    connect (fileCreationDateAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (fileCreationDateAct, PlayListModel::FILE_CREATION_DATE);

    QAction* fileModificationDateAct = sort_mode_menu->addAction (tr("By File Modification Date"));
    connect (fileModificationDateAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (fileModificationDateAct, PlayListModel::FILE_MODIFICATION_DATE);

    QAction* groupAct = sort_mode_menu->addAction (tr("By Group"));
    connect (groupAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (groupAct, PlayListModel::GROUP);

    connect (signalMapper, SIGNAL (mapped (int)), m_pl_manager, SLOT (sort (int)));

    m_ui.menuEdit->addMenu (sort_mode_menu);

    sort_mode_menu = new QMenu (tr("Sort Selection"), this);
    sort_mode_menu->setIcon(QIcon::fromTheme("view-sort-ascending"));
    signalMapper = new QSignalMapper (this);
    titleAct = sort_mode_menu->addAction (tr ("By Title"));
    connect (titleAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (titleAct, PlayListModel::TITLE);

    albumAct = sort_mode_menu->addAction (tr ("By Album"));
    connect (albumAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (albumAct, PlayListModel::ALBUM);

    artistAct = sort_mode_menu->addAction (tr ("By Artist"));
    connect (artistAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (artistAct, PlayListModel::ARTIST);

    albumArtistAct = sort_mode_menu->addAction (tr ("By Album Artist"));
    connect (albumArtistAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (albumArtistAct, PlayListModel::ALBUMARTIST);

    nameAct = sort_mode_menu->addAction (tr ("By Filename"));
    connect (nameAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (nameAct, PlayListModel::FILENAME);

    pathnameAct = sort_mode_menu->addAction (tr ("By Path + Filename"));
    connect (pathnameAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (pathnameAct, PlayListModel::PATH_AND_FILENAME);

    dateAct = sort_mode_menu->addAction (tr ("By Date"));
    connect (dateAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (dateAct, PlayListModel::DATE);

    trackAct = sort_mode_menu->addAction (tr ("By Track Number"));
    connect (trackAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (trackAct, PlayListModel::TRACK);

    discAct = sort_mode_menu->addAction (tr("By Disc Number"));
    connect (discAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (discAct, PlayListModel::DISCNUMBER);

    fileCreationDateAct = sort_mode_menu->addAction (tr("By File Creation Date"));
    connect (fileCreationDateAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (fileCreationDateAct, PlayListModel::FILE_CREATION_DATE);

    fileModificationDateAct = sort_mode_menu->addAction (tr("By File Modification Date"));
    connect (fileModificationDateAct, SIGNAL (triggered (bool)), signalMapper, SLOT (map()));
    signalMapper->setMapping (fileModificationDateAct, PlayListModel::FILE_MODIFICATION_DATE);

    connect (signalMapper, SIGNAL (mapped (int)), m_pl_manager, SLOT (sortSelection (int)));
    m_ui.menuEdit->addMenu (sort_mode_menu);
    m_ui.menuEdit->addSeparator();
    m_ui.menuEdit->addAction (QIcon::fromTheme("media-playlist-shuffle"), tr("Randomize List"),
                              m_pl_manager, SLOT(randomizeList()));
    m_ui.menuEdit->addAction (QIcon::fromTheme("view-sort-descending"), tr("Reverse List"),
                              m_pl_manager, SLOT(reverseList()));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_GROUP_TRACKS, m_ui_settings, SLOT(setGroupsEnabled(bool))));
    ACTION(ActionManager::PL_GROUP_TRACKS)->setChecked(m_ui_settings->isGroupsEnabled());
    m_ui.menuEdit->addSeparator();
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::SETTINGS, this, SLOT(showSettings())));
    //tools
    m_ui.menuTools->addMenu(m_uiHelper->createMenu(UiHelper::TOOLS_MENU, tr("Actions"), this));
    //playback menu
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PLAY));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::STOP));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PAUSE));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::NEXT));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PREVIOUS));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::PLAY_PAUSE,this,SLOT(playPause())));
    m_ui.menuPlayback->addSeparator();
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::JUMP, this, SLOT(jumpTo())));
    m_ui.menuPlayback->addSeparator();
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PL_ENQUEUE));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::CLEAR_QUEUE, m_pl_manager, SLOT(clearQueue())));
    m_ui.menuPlayback->addSeparator();
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::REPEAT_ALL, m_ui_settings,
                                            SLOT(setRepeatableList(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::REPEAT_TRACK, m_ui_settings,
                                            SLOT(setRepeatableTrack(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::SHUFFLE, m_ui_settings,
                                            SLOT(setShuffle(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::NO_PL_ADVANCE, m_ui_settings,
                                            SLOT(setNoPlayListAdvance(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::STOP_AFTER_SELECTED, m_pl_manager,
                                            SLOT(stopAfterSelected())));
    m_ui.menuPlayback->addSeparator();
    signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(ACTION(ActionManager::VOL_ENC), 5);
    signalMapper->setMapping(ACTION(ActionManager::VOL_DEC), -5);
    connect(signalMapper, SIGNAL(mapped(int)), m_core, SLOT(changeVolume(int)));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::VOL_ENC, signalMapper, SLOT(map())));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::VOL_DEC, signalMapper, SLOT(map())));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::VOL_MUTE, m_core, SLOT(setMuted(bool))));
    connect(m_core, SIGNAL(mutedChanged(bool)), ACTION(ActionManager::VOL_MUTE), SLOT(setChecked(bool)));

    //help menu
    m_ui.menuHelp->addAction(SET_ACTION(ActionManager::ABOUT_UI, this, SLOT(aboutUi())));
    m_ui.menuHelp->addAction(SET_ACTION(ActionManager::ABOUT, this, SLOT(about())));
    m_ui.menuHelp->addAction(SET_ACTION(ActionManager::ABOUT_QT, qApp, SLOT(aboutQt())));
    //playlist menu
    m_pl_menu->addAction(SET_ACTION(ActionManager::PL_SHOW_INFO, m_pl_manager, SLOT(showDetails())));
    m_pl_menu->addSeparator();
    m_pl_menu->addAction(ACTION(ActionManager::PL_REMOVE_SELECTED));
    m_pl_menu->addAction(ACTION(ActionManager::PL_REMOVE_ALL));
    m_pl_menu->addAction(ACTION(ActionManager::PL_REMOVE_UNSELECTED));
    m_pl_menu->addMenu(UiHelper::instance()->createMenu(UiHelper::PLAYLIST_MENU,
                                                        tr("Actions"), this));
    m_pl_menu->addSeparator();
    m_pl_menu->addAction(SET_ACTION(ActionManager::PL_ENQUEUE, m_pl_manager, SLOT(addToQueue())));
    //tools menu
    m_ui.menuTools->addAction(SET_ACTION(ActionManager::EQUALIZER, this, SLOT(showEqualizer())));

    //tab menu
    m_tab_menu->addAction(ACTION(ActionManager::PL_LOAD));
    m_tab_menu->addAction(ACTION(ActionManager::PL_SAVE));
    m_tab_menu->addSeparator();
    m_tab_menu->addAction(ACTION(ActionManager::PL_RENAME));
    m_tab_menu->addAction(ACTION(ActionManager::PL_CLOSE));
    //seeking
    QAction* forward = new QAction(this);
    forward->setShortcut(QKeySequence(Qt::Key_Right));
    connect(forward,SIGNAL(triggered(bool)),this,SLOT(forward()));
    QAction* backward = new QAction(this);
    backward->setShortcut(QKeySequence(Qt::Key_Left));
    connect(backward,SIGNAL(triggered(bool)),this,SLOT(backward()));

    addActions(QList<QAction*>() << forward << backward);
    addActions(ActionManager::instance()->actions());
    addActions(m_key_manager->actions());
}

void MainWindow::createButtons()
{
    //'new playlist' button
    m_addListButton = new QToolButton(m_ui.tabWidget);
    m_addListButton->setText("+");
    m_addListButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_addListButton->setAutoRaise(true);
    m_addListButton->setIcon(QIcon::fromTheme("list-add"));
    m_addListButton->setToolTip(tr("Add new playlist"));
    connect(m_addListButton, SIGNAL(clicked()), m_pl_manager, SLOT(createPlayList()));
    //playlist menu button
    m_tabListMenuButton = new QToolButton(m_ui.tabWidget);
    m_tabListMenuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_tabListMenuButton->setAutoRaise(true);
    m_tabListMenuButton->setToolTip(tr("Show all tabs"));
    m_tabListMenuButton->setArrowType(Qt::DownArrow);
    m_tabListMenuButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    m_tabListMenuButton->setPopupMode(QToolButton::InstantPopup);
    m_tabListMenuButton->setMenu(m_ui.tabWidget->menu());
}

void MainWindow::readSettings()
{
    QSettings settings (Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("Simple");
    m_titleFormatter.setPattern(settings.value("window_title_format","%if(%p,%p - %t,%t)").toString());
    if(m_update)
    {
        for(int i = 0; i < m_ui.tabWidget->count(); ++i)
        {
            qobject_cast<ListWidget *>(m_ui.tabWidget->widget(i))->readSettings();
        }
        qobject_cast<FileSystemBrowser *> (m_ui.fileSystemDockWidget->widget())->readSettings();

        if(ACTION(ActionManager::WM_ALLWAYS_ON_TOP)->isChecked())
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        else
            setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

        if(m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused)
            showMetaData();

        m_ui.tabWidget->readSettings();

        show();
    }
    else
    {
        restoreGeometry(settings.value("mw_geometry").toByteArray());
        QByteArray wstate = settings.value("mw_state").toByteArray();
        if(wstate.isEmpty())
        {
            m_ui.fileSystemDockWidget->hide();
            m_ui.coverDockWidget->hide();
            m_ui.playlistsDockWidget->hide();
        }
        else
            restoreState(settings.value("mw_state").toByteArray());
        if(settings.value("always_on_top", false).toBool())
        {
            ACTION(ActionManager::WM_ALLWAYS_ON_TOP)->setChecked(true);
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        }
        show();
        qApp->processEvents();
        if(settings.value("start_hidden").toBool())
            hide();

        bool state = settings.value("show_titlebars", true).toBool();
        ACTION(ActionManager::UI_SHOW_TITLEBARS)->setChecked(state);
        setTitleBarsVisible(state);

        state = settings.value("show_tabs", true).toBool();
        ACTION(ActionManager::UI_SHOW_TABS)->setChecked(state);
        m_ui.tabWidget->setTabsVisible(state);

        state = settings.value("block_toolbars", false).toBool();
        ACTION(ActionManager::UI_BLOCK_TOOLBARS)->setChecked(state);
        setToolBarsBlocked(state);

        m_update = true;
    }
    //load toolbar actions
    m_ui.buttonsToolBar->clear();
    QStringList names = ActionManager::instance()->toolBarActionNames();
    names = settings.value("toolbar_actions", names).toStringList();
    foreach (QString name, names)
    {
        if(name == "separator")
        {
            m_ui.buttonsToolBar->addSeparator();
            continue;
        }
        QAction *action = ActionManager::instance()->findChild<QAction *>(name);
        if(action)
            m_ui.buttonsToolBar->addAction(action);
    }

    m_hideOnClose = settings.value("hide_on_close", false).toBool();
    m_ui.tabWidget->setTabsClosable(settings.value("pl_tabs_closable", false).toBool());

    if(settings.value("pl_show_new_pl_button", false).toBool())
    {
        m_ui.tabWidget->setCornerWidget(m_addListButton, Qt::TopLeftCorner);
        m_addListButton->setIconSize(QSize(16, 16));
        m_addListButton->setVisible(true);
    }
    else
    {
        m_addListButton->setVisible(false);
        m_ui.tabWidget->setCornerWidget(0, Qt::TopLeftCorner);
    }
    if(settings.value("pl_show_tab_list_menu", false).toBool())
    {
        m_ui.tabWidget->setCornerWidget(m_tabListMenuButton, Qt::TopRightCorner);
        m_tabListMenuButton->setIconSize(QSize(16, 16));
        m_tabListMenuButton->setVisible(true);
    }
    else
    {
        m_tabListMenuButton->setVisible(false);
        m_ui.tabWidget->setCornerWidget(0, Qt::TopRightCorner);
    }
    settings.endGroup();
    addActions(m_uiHelper->actions(UiHelper::TOOLS_MENU));
    addActions(m_uiHelper->actions(UiHelper::PLAYLIST_MENU));
}

void MainWindow::showTabMenu(QPoint pos)
{
    QTabBar *tabBar = qobject_cast<QTabBar *> (m_ui.tabWidget->childAt(pos));
    if(!tabBar)
        return;

    int index = tabBar->tabAt(pos);
    if(index == -1)
        return;

    m_pl_manager->selectPlayList(index);
    m_tab_menu->popup(m_ui.tabWidget->mapToGlobal(pos));
}

void MainWindow::writeSettings()
{
    QSettings settings (Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("Simple/mw_geometry", saveGeometry());
    settings.setValue("Simple/mw_state", saveState());
    settings.setValue("Simple/always_on_top", ACTION(ActionManager::WM_ALLWAYS_ON_TOP)->isChecked());
    settings.setValue("Simple/show_analyzer", ACTION(ActionManager::UI_ANALYZER)->isChecked());
    settings.setValue("Simple/show_tabs", ACTION(ActionManager::UI_SHOW_TABS)->isChecked());
    settings.setValue("Simple/show_titlebars", ACTION(ActionManager::UI_SHOW_TITLEBARS)->isChecked());
    settings.setValue("Simple/block_toolbars", ACTION(ActionManager::UI_BLOCK_TOOLBARS)->isChecked());
}

void MainWindow::savePlayList()
{
    m_uiHelper->savePlayList(this);
}

void MainWindow::loadPlayList()
{
    m_uiHelper->loadPlayList(this);
}

void MainWindow::showBuffering(int percent)
{
    if(m_core->state() == Qmmp::Buffering)
        m_statusLabel->setText(tr("Buffering: %1%").arg(percent));
}

void MainWindow::showEqualizer()
{
    Equalizer equalizer(this);
    equalizer.exec();
}

void MainWindow::forward()
{
    m_core->seek(m_core->elapsed() + KEY_OFFSET);
}

void MainWindow::backward()
{
    m_core->seek(qMax(qint64(0), m_core->elapsed() - KEY_OFFSET));
}

void MainWindow::showMetaData()
{
    PlayListModel *model = m_pl_manager->currentPlayList();
    PlayListTrack *track = model->currentTrack();
    if(track && track->url() == m_core->metaData().value(Qmmp::URL))
    {
        setWindowTitle(m_titleFormatter.format(track));
    }
}

void MainWindow::setTitleBarsVisible(bool visible)
{
    if(visible)
    {
        QWidget *widget = 0;
        if((widget = m_ui.analyzerDockWidget->titleBarWidget()))
        {
            m_ui.analyzerDockWidget->setTitleBarWidget(0);
            delete widget;
        }
        if((widget = m_ui.fileSystemDockWidget->titleBarWidget()))
        {
            m_ui.fileSystemDockWidget->setTitleBarWidget(0);
            delete widget;
        }
        if((widget = m_ui.coverDockWidget->titleBarWidget()))
        {
            m_ui.coverDockWidget->setTitleBarWidget(0);
            delete widget;
        }
        if((widget = m_ui.playlistsDockWidget->titleBarWidget()))
        {
            m_ui.playlistsDockWidget->setTitleBarWidget(0);
            delete widget;
        }
    }
    else
    {
        if(!m_ui.analyzerDockWidget->titleBarWidget())
            m_ui.analyzerDockWidget->setTitleBarWidget(new QWidget());

        if(!m_ui.fileSystemDockWidget->titleBarWidget())
            m_ui.fileSystemDockWidget->setTitleBarWidget(new QWidget());

        if(!m_ui.coverDockWidget->titleBarWidget())
            m_ui.coverDockWidget->setTitleBarWidget(new QWidget());

        if(!m_ui.playlistsDockWidget->titleBarWidget())
            m_ui.playlistsDockWidget->setTitleBarWidget(new QWidget());
    }
}

void MainWindow::setToolBarsBlocked(bool blocked)
{
    m_ui.buttonsToolBar->setMovable(!blocked);
    m_ui.progressToolBar->setMovable(!blocked);
}

void MainWindow::editToolBar()
{
    ToolBarEditor *e = new ToolBarEditor(this);
    if(e->exec() == QDialog::Accepted)
    {
        readSettings();
    }
    e->deleteLater();
}
