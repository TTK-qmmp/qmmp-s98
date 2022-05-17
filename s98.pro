include($$PWD/../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Input/s98

HEADERS += decoders98factory.h \
           decoder_s98.h \
           s98helper.h \
           s98metadatamodel.h
    
SOURCES += decoders98factory.cpp \
           decoder_s98.cpp \
           s98helper.cpp \
           s98metadatamodel.cpp \
           libs98/s98.cpp \
           libs98/device/emu2413/emu2413.c \
           libs98/device/fmgen/file.cpp \
           libs98/device/fmgen/fmgen.cpp \
           libs98/device/fmgen/fmtimer.cpp \
           libs98/device/fmgen/opm.cpp \
           libs98/device/fmgen/opna.cpp \
           libs98/device/fmgen/psg.cpp \
           libs98/device/mame/fmopl.c \
           libs98/device/mame/ymf262.c \
           libs98/device/s98mame.cpp \
           libs98/device/s98fmgen.cpp \
           libs98/device/s98opll.cpp \
           libs98/device/s98sng.cpp \
           libs98/device/s_logtbl.c \
           libs98/device/s_sng.c \
           libs98/zlib/adler32.c \
           libs98/zlib/compress.c \
           libs98/zlib/crc32.c \
           libs98/zlib/gzio.c \
           libs98/zlib/uncompr.c \
           libs98/zlib/deflate.c \
           libs98/zlib/trees.c \
           libs98/zlib/zutil.c \
           libs98/zlib/inflate.c \
           libs98/zlib/infback.c \
           libs98/zlib/inftrees.c \
           libs98/zlib/inffast.c

INCLUDEPATH += $$PWD/libs98

unix {
    target.path = $$PLUGIN_DIR/Input
    INSTALLS += target
}
