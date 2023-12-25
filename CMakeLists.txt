cmake_minimum_required(VERSION 3.26)
project(ambilight)

set(CMAKE_CXX_STANDARD 20)

add_executable(ambilight main.cpp)
add_executable(ambilight_test test.cpp)

# qt
# to support #include <QLabel>
##include <QPixmap>
##include <QImage>
##include <QColor>

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(ambilight Qt5::Widgets)
target_link_libraries(ambilight_test Qt5::Widgets)

# x11
find_package(X11 REQUIRED)
target_link_libraries(ambilight ${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIR})

target_link_libraries(ambilight_test ${X11_LIBRARIES})

# gstreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
include_directories(${GST_VIDEO_INCLUDE_DIRS})
target_link_libraries(ambilight_test ${GST_VIDEO_LIBRARIES})
include_directories(${GST_APP_INCLUDE_DIRS})
target_link_libraries(ambilight_test ${GST_APP_LIBRARIES})
include_directories(${GST_INCLUDE_DIRS})
target_link_libraries(ambilight_test ${GST_LIBRARIES})


