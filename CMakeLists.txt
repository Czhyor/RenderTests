cmake_minimum_required(VERSION 3.20.0)
project(XEngine)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)
#
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /Od")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(CMAKE_CONFIGURATION_TYPES "Release")
set(CMAKE_BUILD_TYPE Release)

set(3RD_DIR "D:/3rd")
set(OSG_ROOT "${3RD_DIR}/OpenSceneGraph/3.6.5")
set(OCC_ROOT "${3RD_DIR}/OpenCASCADE-7.7.0-vc14-64/opencascade-7.7.0")
set(OCC_LIB_DIR "${OCC_ROOT}/win64/vc14")
set(TBB_ROOT "${3RD_DIR}/OpenCASCADE-7.7.0-vc14-64/tbb_2021.5-vc14-64")

set(Qt6_ROOT "C:/Qt/6.5.2/msvc2019_64")
SET(_qt_deploy_exe_ "${Qt6_ROOT}/bin/windeployqt.exe")

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${Qt6_ROOT};${OSG_ROOT}/cmake")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_INSTALL_PREFIX})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_INSTALL_PREFIX})

find_package(OSG CONFIG)
find_package(Qt6 REQUIRED CONFIG COMPONENTS Core Gui Qml Quick Concurrent)

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(math)
add_subdirectory(common)
add_subdirectory(canvas3d)
add_subdirectory(engine)
add_subdirectory(creator)