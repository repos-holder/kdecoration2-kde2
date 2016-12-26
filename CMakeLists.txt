project(skeleton)
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 REQUIRED CONFIG COMPONENTS Core Gui Widgets)
find_package(KF5 REQUIRED COMPONENTS CoreAddons)
find_package(KDecoration2 REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

add_subdirectory(src)

