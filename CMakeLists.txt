cmake_minimum_required(VERSION 3.16)

project(unisettings LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(unisettings SHARED
  src/unisettings_global.h
  src/unisettings_macros.h
  src/unisettings.cpp
  src/unisettings.h
  src/systemsettings.cpp
  src/systemsettings.h
)

target_link_libraries(unisettings PRIVATE Qt6::Core)

target_compile_definitions(unisettings PRIVATE UNISETTINGS_LIBRARY)

target_include_directories(unisettings PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

include(GNUInstallDirs)

install(TARGETS unisettings
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES 
    src/unisettings.h
    src/unisettings_global.h
    src/unisettings_macros.h
    src/systemsettings.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/unisettings
)