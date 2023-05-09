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
    process.cpp \
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
    process.h \
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

# copy files needed for distribution
QMAKE_POST_LINK += xcopy /E /Y \"$$PWD\\data\\\" \"$$OUT_PWD\\dist\\\"
QMAKE_POST_LINK += && xcopy /Y \"$$OUT_PWD\\release\\$${TARGET}.exe\" \"$$OUT_PWD\\dist\\\"
QMAKE_POST_LINK += && xcopy /Y \"$$OUT_PWD\\release\\RE_Kenshi_de.qm\" \"$$OUT_PWD\\dist\\translations\\\"
QMAKE_POST_LINK += && xcopy /Y \"$$OUT_PWD\\release\\RE_Kenshi_ru.qm\" \"$$OUT_PWD\\dist\\translations\\\"
QMAKE_POST_LINK += && xcopy /Y \"$$PWD\\LICENSE\" \"$$OUT_PWD\\dist\\\"
QMAKE_POST_LINK += && "C:\\Qt\\5.15.2\\msvc2019_64\\bin\\windeployqt.exe" --translations en,de,ru --no-compiler-runtime --no-system-d3d-compiler --no-opengl-sw \"$$OUT_PWD\\dist\\$${TARGET}.exe\"
# pretty sure these aren't needed
QMAKE_POST_LINK += && rmdir /s /q \"$$OUT_PWD\\dist\\bearer\"
QMAKE_POST_LINK += && rmdir /s /q \"$$OUT_PWD\\dist\\iconengines\"
QMAKE_POST_LINK += && rmdir /s /q \"$$OUT_PWD\\dist\\imageformats\"
QMAKE_POST_LINK += && rmdir /s /q \"$$OUT_PWD\\dist\\styles\"
# copy files needed for translation
QMAKE_POST_LINK += && xcopy /I /Y \"$$PWD\\*.cpp\" \"$$OUT_PWD\\translation\\\"
QMAKE_POST_LINK += && xcopy /Y \"$$PWD\\*.h\" \"$$OUT_PWD\\translation\\\"
QMAKE_POST_LINK += && xcopy /Y \"$$PWD\\*.ui\" \"$$OUT_PWD\\translation\\\"
QMAKE_POST_LINK += && xcopy /Y \"$$PWD\\*.ts\" \"$$OUT_PWD\\translation\\\"
# delete discord info
QMAKE_POST_LINK += && del \"$$OUT_PWD\\translation\\discord.h\"


