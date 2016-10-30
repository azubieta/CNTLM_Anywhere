QT       -= gui

CONFIG += c++11

TARGET = libcntlm
TEMPLATE = lib

DEFINES += LIBCNTLM_LIBRARY

SOURCES += \
    libcntlm.cpp \
    acl.c \
    auth.c \
    config.c \
    direct.c \
    forward.c \
    http.c \
    ntlm.c \
    pages.c \
    scanner.c \
    socket.c \
    utils.c \
    xcrypt.c \
    main.c

HEADERS += \
    libcntlm.h \
    libcntlm_global.h \
    acl.h \
    auth.h \
    config.h \
    direct.h \
    forward.h \
    globals.h \
    http.h \
    ntlm.h \
    pages.h \
    scanner.h \
    socket.h \
    swap.h \
    utils.h \
    xcrypt.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
