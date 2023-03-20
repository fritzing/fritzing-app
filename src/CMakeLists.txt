# Find QT5
find_package(Qt5 COMPONENTS Widgets Xml Concurrent Svg Network PrintSupport SerialPort Sql REQUIRED)
# Find Boost
find_package(Boost REQUIRED)
# Find Libgit
find_package(libgit2 REQUIRED)
# Find Libgit
find_package(ZLIB REQUIRED)

set(FRITZING_SRCS
  commands.cpp
  debugdialog.cpp
  fapplication.cpp
  fsplashscreen.cpp
  fsvgrenderer.cpp
  itemdrag.cpp
  layerattributes.cpp
  main.cpp
  processeventblocker.cpp
  sketchtoolbutton.cpp
  viewgeometry.cpp
  viewlayer.cpp
  waitpushundostack.cpp)

set(FRITZING_HDRS
  commands.h
  debugdialog.h
  fapplication.h
  fsplashscreen.h
  fsvgrenderer.h
  installedfonts.h
  itemdrag.h
  layerattributes.h
  processeventblocker.h
  routingstatus.h
  sketchtoolbutton.h
  viewgeometry.h
  viewlayer.h
  waitpushundostack.h)

set(FRITZING_FORMS)

set(FRITZING_INC_DIRS
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBGIT2_INCLUDE_DIR})

add_subdirectory(autoroute)
add_subdirectory(connectors)
add_subdirectory(dialogs)
add_subdirectory(dock)
add_subdirectory(eagle)
add_subdirectory(help)
add_subdirectory(infoview)
add_subdirectory(items)
add_subdirectory(lib)
add_subdirectory(mainwindow)
add_subdirectory(model)
add_subdirectory(partsbinpalette)
add_subdirectory(partseditor)
add_subdirectory(program)
add_subdirectory(referencemodel)
add_subdirectory(sketch)
add_subdirectory(svg)
add_subdirectory(utils)
add_subdirectory(version)

add_executable(fritzing 
  ${CMAKE_SOURCE_DIR}/fritzing.rc
  ${CMAKE_SOURCE_DIR}/phoenixresources.qrc
  ${FRITZING_SRCS}
  ${FRITZING_HDRS}
  ${FRITZING_FORMS})

target_link_libraries(fritzing
    Boost::boost
    Qt5::Widgets
    Qt5::Concurrent
    Qt5::Xml
    Qt5::Svg
    Qt5::Network
    Qt5::PrintSupport
    Qt5::SerialPort
    Qt5::Sql
    ZLIB::ZLIB
    ${LIBGIT2_LIBRARIES})

target_include_directories(fritzing PRIVATE ${FRITZING_INC_DIRS})

# Git versioning
find_package(Git)

set(GIT_VERSION "N/A")
set(GIT_DATE "N/A")
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M")

if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always
    OUTPUT_VARIABLE GIT_VERSION)
  string(STRIP ${GIT_VERSION} GIT_VERSION)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} show --no-patch --no-notes --pretty='%cd' --date=iso-strict
    OUTPUT_VARIABLE GIT_DATE)
  string(STRIP ${GIT_DATE} GIT_DATE)
else()
  message(WARN "Can't get git versioning")
endif()

target_compile_definitions(fritzing PRIVATE GIT_VERSION="${GIT_VERSION}" GIT_DATE="${GIT_DATE}" BUILD_TIME="${BUILD_TIME}")