CONFIG(debug, debug|release) {
    # debug configuration
    DESTDIR = ../bin/debug
    LIBS += -L../bin/debug
    MOC_DIR = obj/debug/moc
    OBJECTS_DIR = obj/debug
    UI_DIR = obj/debug/ui
    DEFINES += _DEBUG
} else {
    # release configuration
    DESTDIR = ../bin/release
    LIBS += -L../bin/debug
    MOC_DIR = obj/release/moc
    OBJECTS_DIR = obj/release
    UI_DIR = obj/release/ui
    DEFINES += NDEBUG
}

win32:CONFIG(debug, debug|release):
else:unix: LIBS += -lm -ldl

# include root
INCLUDEPATH += ../src/
