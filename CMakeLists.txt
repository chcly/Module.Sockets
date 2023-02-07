cmake_minimum_required(VERSION 3.15)
project(Sockets)

# Use C++-17 by default.
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Set the module path to include scripts
# CMake - Contains project local modules.
#
# CMake/Modules - Contains project independent modules.
#   GitUpdate.cmake invokes python ${Sockets_SOURCE_DIR}/gitupdate.py
#   to pull all submodules and update to head.
#   If this fails, it will attempt to invoke git directly to pull 
#   submodules. 
list(APPEND CMAKE_MODULE_PATH 
  ${Sockets_SOURCE_DIR}/CMake 
  ${Sockets_SOURCE_DIR}/CMake/Modules
)
include(${Sockets_SOURCE_DIR}/CMake/Configure.cmake)
if (NOT Configure_SUCCEEDED)
   return()
endif()



if (Sockets_ExternalTarget)
    set(TargetFolders ${Sockets_TargetFolders})
    set(TargetName    ${Sockets_TargetName})
    set(TargetGroup   ${Sockets_TargetGroup})
else()
    add_subdirectory(CMake)
    add_subdirectory(Internal/Utils)
    add_subdirectory(Internal/Threads)

    set(TargetFolders TRUE)
    set(TargetName Sockets)
    set(TargetGroup )

    set(Sockets_INCLUDE  ${Sockets_SOURCE_DIR})
    set(Sockets_LIBRARY  ${TargetName})
endif()

add_subdirectory(Sockets)

if (Sockets_BUILD_TEST)
    if (NOT Sockets_ExternalTarget)
        add_gtest_source(Extern/ThirdParty)
    endif()

    set(TargetGroup Units)
    add_subdirectory(Test)
endif()
