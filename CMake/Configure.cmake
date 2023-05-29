include(GitUpdate)
if (NOT GitUpdate_SUCCESS)
    return()
endif()

include(StaticRuntime)
include(GTestUtils)
include(ExternalTarget)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(Sockets_BUILD_TEST          "Build the unit test program." ON)
option(Sockets_AUTO_RUN_TEST       "Automatically run the test program." OFF)
option(Sockets_USE_STATIC_RUNTIME  "Build with the MultiThreaded(Debug) runtime library." ON)

if (Sockets_USE_STATIC_RUNTIME)
    set_static_runtime()
else()
    set_dynamic_runtime()
endif()


configure_gtest(${Sockets_SOURCE_DIR}/Test/googletest 
                ${Sockets_SOURCE_DIR}/Test/googletest/googletest/include)



DefineExternalTargetEx(
    Utils Extern
    ${Sockets_SOURCE_DIR}/Internal/Utils 
    ${Sockets_SOURCE_DIR}/Internal/Utils
    ${Sockets_BUILD_TEST}
    ${Sockets_AUTO_RUN_TEST}
)

DefineExternalTargetEx(
    Thread Extern
    ${Sockets_SOURCE_DIR}/Internal/Thread 
    ${Sockets_SOURCE_DIR}/Internal/Thread
    ${Sockets_BUILD_TEST}
    ${Sockets_AUTO_RUN_TEST}
)

set(Configure_SUCCEEDED TRUE)