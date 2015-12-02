#############################################################################
# Makefile for building: lightboard
# Generated by qmake (2.01a) (Qt 4.8.6) on: sab dic 5 16:17:32 2015
# Project:  lightboard.pro
# Template: app
# Command: /usr/bin/qmake-qt4 -o Makefile lightboard.pro
#############################################################################

####### Compiler, tools and options

CC            = gcc
CXX           = g++
DEFINES       = -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED
CFLAGS        = -m64 -pipe -O2 -w -D_REENTRANT $(DEFINES)
CXXFLAGS      = -m64 -pipe -O2 -w -D_REENTRANT $(DEFINES)
INCPATH       = -I/usr/share/qt4/mkspecs/linux-g++-64 -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I.
LINK          = g++
LFLAGS        = -m64 -Wl,-O1
LIBS          = $(SUBLIBS)  -L/usr/lib/x86_64-linux-gnu -ludev -lX11 -lXtst -lbluetooth -lQtGui -lQtCore -lpthread 
AR            = ar cqs
RANLIB        = 
QMAKE         = /usr/bin/qmake-qt4
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = $(COPY)
COPY_DIR      = $(COPY) -r
STRIP         = strip
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = src/calibration.cpp \
		src/CalibrationWindow.cpp \
		src/ConfigurationWindow.cpp \
		src/main.cpp \
		lib/core.c \
		lib/monitor.c \
		lib/QProgressIndicator.cpp moc_main.cpp \
		moc_QProgressIndicator.cpp \
		qrc_systray.cpp
OBJECTS       = calibration.o \
		CalibrationWindow.o \
		ConfigurationWindow.o \
		main.o \
		core.o \
		monitor.o \
		QProgressIndicator.o \
		moc_main.o \
		moc_QProgressIndicator.o \
		qrc_systray.o
DIST          = /usr/share/qt4/mkspecs/common/unix.conf \
		/usr/share/qt4/mkspecs/common/linux.conf \
		/usr/share/qt4/mkspecs/common/gcc-base.conf \
		/usr/share/qt4/mkspecs/common/gcc-base-unix.conf \
		/usr/share/qt4/mkspecs/common/g++-base.conf \
		/usr/share/qt4/mkspecs/common/g++-unix.conf \
		/usr/share/qt4/mkspecs/qconfig.pri \
		/usr/share/qt4/mkspecs/features/qt_functions.prf \
		/usr/share/qt4/mkspecs/features/qt_config.prf \
		/usr/share/qt4/mkspecs/features/exclusive_builds.prf \
		/usr/share/qt4/mkspecs/features/default_pre.prf \
		/usr/share/qt4/mkspecs/features/release.prf \
		/usr/share/qt4/mkspecs/features/default_post.prf \
		/usr/share/qt4/mkspecs/features/warn_off.prf \
		/usr/share/qt4/mkspecs/features/shared.prf \
		/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/share/qt4/mkspecs/features/qt.prf \
		/usr/share/qt4/mkspecs/features/unix/thread.prf \
		/usr/share/qt4/mkspecs/features/moc.prf \
		/usr/share/qt4/mkspecs/features/resources.prf \
		/usr/share/qt4/mkspecs/features/uic.prf \
		/usr/share/qt4/mkspecs/features/yacc.prf \
		/usr/share/qt4/mkspecs/features/lex.prf \
		/usr/share/qt4/mkspecs/features/include_source_dir.prf \
		lightboard.pro
QMAKE_TARGET  = lightboard
DESTDIR       = 
TARGET        = lightboard

first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)

Makefile: lightboard.pro  /usr/share/qt4/mkspecs/linux-g++-64/qmake.conf /usr/share/qt4/mkspecs/common/unix.conf \
		/usr/share/qt4/mkspecs/common/linux.conf \
		/usr/share/qt4/mkspecs/common/gcc-base.conf \
		/usr/share/qt4/mkspecs/common/gcc-base-unix.conf \
		/usr/share/qt4/mkspecs/common/g++-base.conf \
		/usr/share/qt4/mkspecs/common/g++-unix.conf \
		/usr/share/qt4/mkspecs/qconfig.pri \
		/usr/share/qt4/mkspecs/features/qt_functions.prf \
		/usr/share/qt4/mkspecs/features/qt_config.prf \
		/usr/share/qt4/mkspecs/features/exclusive_builds.prf \
		/usr/share/qt4/mkspecs/features/default_pre.prf \
		/usr/share/qt4/mkspecs/features/release.prf \
		/usr/share/qt4/mkspecs/features/default_post.prf \
		/usr/share/qt4/mkspecs/features/warn_off.prf \
		/usr/share/qt4/mkspecs/features/shared.prf \
		/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/share/qt4/mkspecs/features/qt.prf \
		/usr/share/qt4/mkspecs/features/unix/thread.prf \
		/usr/share/qt4/mkspecs/features/moc.prf \
		/usr/share/qt4/mkspecs/features/resources.prf \
		/usr/share/qt4/mkspecs/features/uic.prf \
		/usr/share/qt4/mkspecs/features/yacc.prf \
		/usr/share/qt4/mkspecs/features/lex.prf \
		/usr/share/qt4/mkspecs/features/include_source_dir.prf \
		/usr/lib/x86_64-linux-gnu/libQtGui.prl \
		/usr/lib/x86_64-linux-gnu/libQtCore.prl
	$(QMAKE) -o Makefile lightboard.pro
/usr/share/qt4/mkspecs/common/unix.conf:
/usr/share/qt4/mkspecs/common/linux.conf:
/usr/share/qt4/mkspecs/common/gcc-base.conf:
/usr/share/qt4/mkspecs/common/gcc-base-unix.conf:
/usr/share/qt4/mkspecs/common/g++-base.conf:
/usr/share/qt4/mkspecs/common/g++-unix.conf:
/usr/share/qt4/mkspecs/qconfig.pri:
/usr/share/qt4/mkspecs/features/qt_functions.prf:
/usr/share/qt4/mkspecs/features/qt_config.prf:
/usr/share/qt4/mkspecs/features/exclusive_builds.prf:
/usr/share/qt4/mkspecs/features/default_pre.prf:
/usr/share/qt4/mkspecs/features/release.prf:
/usr/share/qt4/mkspecs/features/default_post.prf:
/usr/share/qt4/mkspecs/features/warn_off.prf:
/usr/share/qt4/mkspecs/features/shared.prf:
/usr/share/qt4/mkspecs/features/unix/gdb_dwarf_index.prf:
/usr/share/qt4/mkspecs/features/qt.prf:
/usr/share/qt4/mkspecs/features/unix/thread.prf:
/usr/share/qt4/mkspecs/features/moc.prf:
/usr/share/qt4/mkspecs/features/resources.prf:
/usr/share/qt4/mkspecs/features/uic.prf:
/usr/share/qt4/mkspecs/features/yacc.prf:
/usr/share/qt4/mkspecs/features/lex.prf:
/usr/share/qt4/mkspecs/features/include_source_dir.prf:
/usr/lib/x86_64-linux-gnu/libQtGui.prl:
/usr/lib/x86_64-linux-gnu/libQtCore.prl:
qmake:  FORCE
	@$(QMAKE) -o Makefile lightboard.pro

dist: 
	@$(CHK_DIR_EXISTS) .tmp/lightboard1.0.0 || $(MKDIR) .tmp/lightboard1.0.0 
	$(COPY_FILE) --parents $(SOURCES) $(DIST) .tmp/lightboard1.0.0/ && $(COPY_FILE) --parents include/main.h include/QProgressIndicator.h .tmp/lightboard1.0.0/ && $(COPY_FILE) --parents systray.qrc .tmp/lightboard1.0.0/ && $(COPY_FILE) --parents src/calibration.cpp src/CalibrationWindow.cpp src/ConfigurationWindow.cpp src/main.cpp lib/core.c lib/monitor.c lib/QProgressIndicator.cpp .tmp/lightboard1.0.0/ && $(COPY_FILE) --parents resources/lightboard_en.ts resources/lightboard_it.ts .tmp/lightboard1.0.0/ && (cd `dirname .tmp/lightboard1.0.0` && $(TAR) lightboard1.0.0.tar lightboard1.0.0 && $(COMPRESS) lightboard1.0.0.tar) && $(MOVE) `dirname .tmp/lightboard1.0.0`/lightboard1.0.0.tar.gz . && $(DEL_FILE) -r .tmp/lightboard1.0.0


clean:compiler_clean 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


check: first

mocclean: compiler_moc_header_clean compiler_moc_source_clean

mocables: compiler_moc_header_make_all compiler_moc_source_make_all

compiler_moc_header_make_all: moc_main.cpp moc_QProgressIndicator.cpp
compiler_moc_header_clean:
	-$(DEL_FILE) moc_main.cpp moc_QProgressIndicator.cpp
moc_main.cpp: include/QProgressIndicator.h \
		include/main.h
	/usr/lib/x86_64-linux-gnu/qt4/bin/moc $(DEFINES) $(INCPATH) include/main.h -o moc_main.cpp

moc_QProgressIndicator.cpp: include/QProgressIndicator.h
	/usr/lib/x86_64-linux-gnu/qt4/bin/moc $(DEFINES) $(INCPATH) include/QProgressIndicator.h -o moc_QProgressIndicator.cpp

compiler_rcc_make_all: qrc_systray.cpp
compiler_rcc_clean:
	-$(DEL_FILE) qrc_systray.cpp
qrc_systray.cpp: systray.qrc \
		images/icon.png
	/usr/lib/x86_64-linux-gnu/qt4/bin/rcc -name systray systray.qrc -o qrc_systray.cpp

compiler_image_collection_make_all: qmake_image_collection.cpp
compiler_image_collection_clean:
	-$(DEL_FILE) qmake_image_collection.cpp
compiler_moc_source_make_all:
compiler_moc_source_clean:
compiler_uic_make_all:
compiler_uic_clean:
compiler_yacc_decl_make_all:
compiler_yacc_decl_clean:
compiler_yacc_impl_make_all:
compiler_yacc_impl_clean:
compiler_lex_make_all:
compiler_lex_clean:
compiler_clean: compiler_moc_header_clean compiler_rcc_clean 

####### Compile

calibration.o: src/calibration.cpp include/xwiimote.h \
		include/main.h \
		include/QProgressIndicator.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o calibration.o src/calibration.cpp

CalibrationWindow.o: src/CalibrationWindow.cpp include/QProgressIndicator.h \
		include/main.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o CalibrationWindow.o src/CalibrationWindow.cpp

ConfigurationWindow.o: src/ConfigurationWindow.cpp include/xwiimote.h \
		include/main.h \
		include/QProgressIndicator.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o ConfigurationWindow.o src/ConfigurationWindow.cpp

main.o: src/main.cpp include/xwiimote.h \
		include/main.h \
		include/QProgressIndicator.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o main.o src/main.cpp

core.o: lib/core.c include/xwiimote.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o core.o lib/core.c

monitor.o: lib/monitor.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o monitor.o lib/monitor.c

QProgressIndicator.o: lib/QProgressIndicator.cpp include/QProgressIndicator.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o QProgressIndicator.o lib/QProgressIndicator.cpp

moc_main.o: moc_main.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o moc_main.o moc_main.cpp

moc_QProgressIndicator.o: moc_QProgressIndicator.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o moc_QProgressIndicator.o moc_QProgressIndicator.cpp

qrc_systray.o: qrc_systray.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o qrc_systray.o qrc_systray.cpp

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:

