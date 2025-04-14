cmake_minimum_required(VERSION 3.12.0)
project(
    file-vtf
    DESCRIPTION "GIMP plugin to read and write Valve Texture Format (VTF) files"
    VERSION 0.1.0
    HOMEPAGE_URL "https://github.com/chev2/gimp-vtf"
    LANGUAGES C CXX)

# Build debug symbols
#set(CMAKE_BUILD_TYPE Debug)

# Force C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SKIP_BUILD_RPATH OFF)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_INSTALL_RPATH $ORIGIN)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GIMPUI_3_0 REQUIRED IMPORTED_TARGET GLOBAL gimpui-3.0)
pkg_check_modules(GIMP_3_0 REQUIRED IMPORTED_TARGET GLOBAL gimp-3.0)

# Set sourcepp compile options
set(SOURCEPP_LIBS_START_ENABLED OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_BSPPP          OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_DMXPP          OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_FSPP           OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_GAMEPP         OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_KVPP           OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_MDLPP          OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_SNDPP          OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_STEAMPP        OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_TOOLPP         OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VCRYPTPP       OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VPKPP          OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VTFPP          ON  CACHE INTERNAL "" FORCE)

set(CRYPTOPP_BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
set(CRYPTOPP_INSTALL       OFF CACHE INTERNAL "" FORCE)

# cryptopp (a CMake version, anyway) is already included in sourcepp,
#  but from my testing, it always errors when it compiles.
# Providing the source files manually fixes it.
set(CRYPTOPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ext/cryptopp")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/sourcepp")

add_executable(file-vtf src/file-vtf.cpp)
target_link_libraries(file-vtf PRIVATE PkgConfig::GIMP_3_0 PkgConfig::GIMPUI_3_0 sourcepp::vtfpp)
