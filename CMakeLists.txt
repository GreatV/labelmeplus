cmake_minimum_required(VERSION 3.5)

project(
  labelmeplus
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# https://json.nlohmann.me/api/macros/json_use_implicit_conversions/
# set(nlohmann-json_IMPLICIT_CONVERSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(glog CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS context)

find_path(PNGPP_INCLUDE_DIRS "png++/color.hpp")
find_path(LIBEXIF_INCLUDE_DIRS "libexif/exif-data.h")
find_library(LIBEXIF_LIBRARY "libexif")

set(PROJECT_SOURCES
    init.h
    main.cpp
    app.cpp
    app.h
    shape.h
    shape.cpp
    base64.cpp
    base64.h
    label_file.cpp
    label_file.h
    imgviz.hpp
    utils/io.cpp
    utils/shape.cpp
    utils/qt.cpp
    utils/image.cpp
    utils/utils.h
    widgets/label_dialog.cpp
    widgets/label_dialog.h
    widgets/label_list_widget.cpp
    widgets/label_list_widget.h
    widgets/unique_label_qlist_widget.cpp
    widgets/unique_label_qlist_widget.h
    widgets/escapable_qlist_widget.cpp
    widgets/escapable_qlist_widget.h
    widgets/zoom_widget.cpp
    widgets/zoom_widget.h
    widgets/canvas.cpp
    widgets/canvas.h
    widgets/tool_bar.cpp
    widgets/tool_bar.h)

if(WIN32)
  add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(labelmeplus MANUAL_FINALIZATION ${PROJECT_SOURCES})
  # Define target properties for Android with Qt 6 as: set_property(TARGET
  # labelmeplus APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
  # ${CMAKE_CURRENT_SOURCE_DIR}/android) For more information, see
  # https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
  if(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})
    # Define properties for Android with Qt 5 after find_package() calls as:
    # set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
  else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
  endif()
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
          glog::glog
          argparse::argparse
          fmt::fmt-header-only
          nlohmann_json::nlohmann_json
          yaml-cpp
          Boost::context
          ${OpenCV_LIBS}
          ${LIBEXIF_LIBRARY})

target_include_directories(${PROJECT_NAME} PRIVATE ${PNGPP_INCLUDE_DIRS}
                                                   ${LIBEXIF_INCLUDE_DIRS})

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME})
endif()
