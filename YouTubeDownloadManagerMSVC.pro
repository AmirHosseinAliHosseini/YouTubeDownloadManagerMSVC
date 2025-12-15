QT += core gui widgets network
CONFIG += c++17 thread

SOURCES += main.cpp
HEADERS += \
    ProgressRing.h

PYDIR = "C:/Program Files/Python313"

INCLUDEPATH += $$PYDIR/include
LIBS += -L$$PYDIR/libs -lpython313

RESOURCES += \
    resources.qrc

RC_FILE = appicon.rc
