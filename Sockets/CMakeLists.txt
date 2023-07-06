set(TargetName Sockets)
include(GroupSet)

set_group(Target_SRC
    Source:*.cpp
    Header:*.h
    Source/Net:Net/*.cpp
    Header/Net:Net/*.h
)

include_directories(
    ${Utils_INCLUDE} 
    ${Sockets_INCLUDE}  
    ${Thread_INCLUDE}
)

add_library(
    ${TargetName} 
    ${Target_SRC}
)

target_link_libraries(
    ${TargetName} 
    ${Utils_LIBRARY} 
    ${Thread_LIBRARY} 
    ${Sockets_PLATFORM}
)

set_target_properties(
    ${TargetName} 
    PROPERTIES FOLDER "${TargetGroup}"
)
