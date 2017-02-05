QT += widgets

CONFIG += warn_on \
    plugin \
    c++11

TARGET = qsui

TEMPLATE = lib

SOURCES += \
    aboutqsuidialog.cpp \
    actionmanager.cpp \
    colorwidget.cpp \
    coverwidget.cpp \
    elidinglabel.cpp \
    eqpreset.cpp \
    equalizer.cpp \
    fft.c \
    filesystembrowser.cpp \
    hotkeyeditor.cpp \
    keyboardmanager.cpp \
    listwidget.cpp \
    listwidgetdrawer.cpp \
    logo.cpp \
    mainwindow.cpp \
    playlistbrowser.cpp \
    playlistheader.cpp \
    popupsettings.cpp \
    popupwidget.cpp \
    positionslider.cpp \
    qsuianalyzer.cpp \
    qsuifactory.cpp \
    qsuisettings.cpp \
    qsuitabbar.cpp \
    shortcutdialog.cpp \
    shortcutitem.cpp \
    toolbareditor.cpp \
    visualmenu.cpp \
    volumeslider.cpp \

HEADERS += \
    aboutqsuidialog.h \
    actionmanager.h \
    colorwidget.h \
    coverwidget.h \
    elidinglabel.h \
    eqpreset.h \
    equalizer.h \
    fft.h \
    filesystembrowser.h \
    hotkeyeditor.h \
    inlines.h \
    keyboardmanager.h \
    listwidgetdrawer.h \
    listwidget.h \
    logo.h \
    mainwindow.h \
    playlistbrowser.h \
    playlistheader.h \
    popupsettings.h \
    popupwidget.h \
    positionslider.h \
    qsuianalyzer.h \
    qsuifactory.h \
    qsuisettings.h \
    qsuitabbar.h \
    shortcutdialog.h \
    shortcutitem.h \
    toolbareditor.h \
    visualmenu.h \
    volumeslider.h \

FORMS += \
    forms/aboutqsuidialog.ui \
    forms/hotkeyeditor.ui \
    forms/mainwindow.ui \
    forms/popupsettings.ui \
    forms/qsuisettings.ui \
    forms/shortcutdialog.ui \
    forms/toolbareditor.ui

RESOURCES += translations/translations.qrc resources/qsui_resources.qrc txt/qsui_txt.qrc

unix{
  LIBS += -lqmmpui -lqmmp
  isEmpty(LIB_DIR){
    !macx:contains(QMAKE_HOST.arch, x86_64) {
        LIB_DIR = /usr/lib64
    } else {
        LIB_DIR = /usr/lib
    }
  }

  target.path = $$LIB_DIR/qmmp/Ui
  INSTALLS += target
}

win32{
   LIBS += -lqmmpui0 -lqmmp0
   INCLUDEPATH += ./
}
