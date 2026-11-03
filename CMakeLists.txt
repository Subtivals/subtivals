cmake_minimum_required(VERSION 3.21)

# Minimum supported by Qt 6.8
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "")

project(ractisubs LANGUAGES CXX)

set(PACKAGE_VERSION "0.0.0" CACHE STRING "Version shown in the application")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

if(APPLE)
  # Qt < 6.9 links the AGL framework, which was removed from the macOS 26 SDK.
  set(WrapOpenGL_AGL "-framework OpenGL" CACHE STRING "")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS
  Core Gui Widgets WebSockets Xml Svg HttpServer LinguistTools)
qt_standard_project_setup()

qt_add_executable(ractisubs WIN32 MACOSX_BUNDLE
  src/main.cpp
  src/mainwindow.cpp src/mainwindow.h src/mainwindow.ui
  src/script.cpp src/script.h
  src/subtitlestyle.cpp src/subtitlestyle.h
  src/subtitle.cpp src/subtitle.h
  src/subtitlesform.cpp src/subtitlesform.h src/subtitlesform.ui
  src/projectionwindow.cpp src/projectionwindow.h
  src/styleeditor.cpp src/styleeditor.h src/styleeditor.ui
  src/configeditor.cpp src/configeditor.h src/configeditor.ui
  src/player.cpp src/player.h
  src/recorder.cpp src/recorder.h
  src/styleadvanced.cpp src/styleadvanced.h src/styleadvanced.ui
  src/shortcuteditor.cpp src/shortcuteditor.h src/shortcuteditor.ui
  src/wizard.cpp src/wizard.h src/wizard.ui
  src/remoteservice.cpp src/remoteservice.h
  src/remoteoptionsdialog.cpp src/remoteoptionsdialog.h
  src/remoteoptionsdialog.ui
  src/macwindowhelper.h
  src/qrcodegen.cpp src/qrcodegen.hpp
  resources/icons.qrc
  resources/samples.qrc
  resources/fonts.qrc
  resources/www.qrc
)

target_compile_definitions(ractisubs PRIVATE
  VERSION="${PACKAGE_VERSION}"
  DEFAULT_LINESPACING=0.3
  NB_PRESETS=6
  DEFAULT_PROJECTION_WINDOW_HEIGHT=200
  ORG_NAME="RactiSubs"
  APP_NAME="RactiSubs"
  MAIN_WEBSITE="https://www.racti.com"
  HELP_WEBSITE="https://www.racti.com"
  COMMUNITY_WEBSITE="https://www.racti.com"
  WIZARD_ENABLED=false
  HELP_ENABLED=false
)

target_compile_options(ractisubs PRIVATE -Wall -Wextra)

target_link_libraries(ractisubs PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::WebSockets Qt6::Xml Qt6::Svg
  Qt6::HttpServer)

# Translations are compiled and embedded as resources under :/i18n
qt_add_translations(ractisubs
  TS_FILES locale/fr_FR.ts locale/es_ES.ts locale/ca_ES.ts locale/it_IT.ts
  RESOURCE_PREFIX /i18n)

if(APPLE)
  enable_language(OBJCXX)
  target_sources(ractisubs PRIVATE src/macwindowhelper.mm)
  target_link_libraries(ractisubs PRIVATE "-framework AppKit")

  set_source_files_properties(resources/mainwindow.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)
  target_sources(ractisubs PRIVATE resources/mainwindow.icns)
  set_target_properties(ractisubs PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Racti Subs"
    MACOSX_BUNDLE_GUI_IDENTIFIER com.racti.RactiSubs
    MACOSX_BUNDLE_ICON_FILE mainwindow.icns
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PACKAGE_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PACKAGE_VERSION}")
elseif(WIN32)
  target_sources(ractisubs PRIVATE resources/main.rc)
else()
  # Used to disable screensaver
  target_link_libraries(ractisubs PRIVATE xcb xcb-screensaver xcb-dpms)
endif()

install(TARGETS ractisubs
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE)
  install(FILES resources/ractisubs.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES resources/mainwindow.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME ractisubs.svg)
endif()
