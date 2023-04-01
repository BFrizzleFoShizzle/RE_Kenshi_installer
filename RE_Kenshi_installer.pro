QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    bugs.cpp \
    copythread.cpp \
    diskutil.cpp \
    hashthread.cpp \
    installwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    md5.cpp \
    optionswindow.cpp \
    shellthread.cpp \
    uninstallwindow.cpp

HEADERS += \
    Release_Assert.h \
    bugs.h \
    copythread.h \
    discord.h \
    diskutil.h \
    hashthread.h \
    installoptions.h \
    installwindow.h \
    mainwindow.h \
    md5.h \
    md5_loc.h \
    optionswindow.h \
    shellthread.h \
    uninstallwindow.h

FORMS += \
    installwindow.ui \
    mainwindow.ui \
    optionswindow.ui

TRANSLATIONS = RE_Kenshi_en.ts \
    RE_Kenshi_de.ts \
    RE_Kenshi_ru.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

unix|win32: LIBS += -L$$PWD/'../../../../../Program Files/Microsoft SDKs/Windows/v7.1/Lib/x64/' -lAdvAPI32

INCLUDEPATH += $$PWD/'../../../../../Program Files/Microsoft SDKs/Windows/v7.1/Lib/x64'
DEPENDPATH += $$PWD/'../../../../../Program Files/Microsoft SDKs/Windows/v7.1/Lib/x64'

DISTFILES += \
    RE_Kenshi_de.ts \
    RE_Kenshi_en.ts \
    RE_Kenshi_ru.ts
