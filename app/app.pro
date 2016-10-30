QT += qml quick

CONFIG += c++11

TARGET = cntlm_anywhere
TEMPLATE = app

SOURCES += \
    src/main.cpp \


HEADERS +=


RESOURCES += qml.qrc \
    res.qrc

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

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libcntlm/release/ -llibcntlm
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libcntlm/debug/ -llibcntlm
else:unix: LIBS += -L$$OUT_PWD/../libcntlm/ -llibcntlm

INCLUDEPATH += $$PWD/../libcntlm
DEPENDPATH += $$PWD/../libcntlm
