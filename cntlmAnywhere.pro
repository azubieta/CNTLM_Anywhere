TEMPLATE = app

QT += qml quick
CONFIG += c++11

SOURCES += main.cpp \
    libcntlm/acl.c \
    libcntlm/auth.c \
    libcntlm/config.c \
    libcntlm/direct.c \
    libcntlm/forward.c \
    libcntlm/http.c \
    libcntlm/ntlm.c \
    libcntlm/pages.c \
    libcntlm/scanner.c \
    libcntlm/socket.c \
    libcntlm/utils.c \
    libcntlm/xcrypt.c \
    libcntlm/libcntlm.c \
    cntlmwrapper.cpp

HEADERS += \
    libcntlm/acl.h \
    libcntlm/auth.h \
    libcntlm/config.h \
    libcntlm/direct.h \
    libcntlm/forward.h \
    libcntlm/globals.h \
    libcntlm/http.h \
    libcntlm/ntlm.h \
    libcntlm/pages.h \
    libcntlm/scanner.h \
    libcntlm/socket.h \
    libcntlm/swap.h \
    libcntlm/utils.h \
    libcntlm/xcrypt.h \
    libcntlm/libcntlm.h \
    cntlmwrapper.h

RESOURCES += qml.qrc \
    drawables.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

