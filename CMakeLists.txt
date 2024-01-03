cmake_minimum_required(VERSION 3.26)
project(ambilight)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(ambilight main.cpp
        capturer_config.h
        color.h
        displayer.h
        capturer.h
        tray_app.h
        serial_writer.h
        xdisplay.h
        app_settings.h
        constants.h
        sender.h
        ambilight.h)
add_executable(ambilight_test test.cpp)


find_package(Qt6 COMPONENTS Widgets SerialPort REQUIRED)
target_link_libraries(ambilight Qt6::Widgets Qt6::SerialPort)
target_link_libraries(ambilight_test Qt6::Widgets Qt6::SerialPort)

# QT_NO_KEYWORDS is needed to avoid conflict with X11
target_compile_definitions(ambilight PRIVATE QT_NO_KEYWORDS)
target_compile_definitions(ambilight_test PRIVATE QT_NO_KEYWORDS)

# x11
find_package(X11 REQUIRED)
target_link_libraries(ambilight ${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIR})

target_link_libraries(ambilight_test ${X11_LIBRARIES})

# gtkmm
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM gtkmm-3.0)
include_directories(${GTKMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS})
target_link_libraries(ambilight ${GTKMM_LIBRARIES})
target_link_libraries(ambilight_test ${GTKMM_LIBRARIES})

#libappindicator
pkg_check_modules(APPINDICATOR REQUIRED appindicator3-0.1)
include_directories(${APPINDICATOR_INCLUDE_DIRS})
link_directories(${APPINDICATOR_LIBRARY_DIRS})
target_link_libraries(ambilight ${APPINDICATOR_LIBRARIES})
target_link_libraries(ambilight_test ${APPINDICATOR_LIBRARIES})

# tbb
find_package(TBB REQUIRED)
target_link_libraries(ambilight TBB::tbb)
target_link_libraries(ambilight_test TBB::tbb)
