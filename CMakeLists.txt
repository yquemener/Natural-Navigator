CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(NaturalNavigator)

FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
SET(QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})


find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} ${USB_INCLUDE_DIRS})


#include_directories (../OpenKinect-libfreenect-3b0f416/wrappers/c_sync/)
#include_directories (../OpenKinect-libfreenect-3b0f416/include/)
include_directories (/opt/local/var/macports/software/libusb-devel/1.0.8.20101017_1/opt/local/include/) 
include_directories (/usr/local/include/libfreenect)

link_directories(/usr/local/lib64/)


INCLUDE_DIRECTORIES  (
        ${CMAKE_CURRENT_BINARY_DIR}
        )

SET (NaturalNavigator_src
	main.cpp
	mainwindow.cpp
	display/glwidget.cpp
	zcam/zcamprocessing.cpp
		)

SET (NaturalNavigator_moc_headers
	mainwindow.h
	display/glwidget.h
	)

SET (UI_FILES
	mainwindow.ui
	)

# moc processing
QT4_WRAP_CPP (NaturalNavigator_moc_outfiles
	${NaturalNavigator_moc_headers})

# moc processing
QT4_WRAP_UI (UI_H_FILES ${UI_FILES})

add_executable(NaturalNavigator
	${NaturalNavigator_src}
	${NaturalNavigator_moc_outfiles}
	${UI_H_FILES}
	)

target_link_libraries (NaturalNavigator
	freenect_sync
	${QT_LIBRARIES}
	${QT_QTNETWORK_LIBRARIES}
	${QT_QTGUI_LIBRARIES}
	${QT_QTGL_LIBRARIES}
	${OPENGL_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	m
	)
