
if (NOT MSVC)
    return()
endif()

include(GroupSet)
set(TargetName ALL_CMAKE)
set(TargetGroup CMakePredefinedTargets)
set(GroupRoot ../)

set_ro_group(TargetName_SRC
    :${GroupRoot}/.github/workflows/*.yml
    :${GroupRoot}/CMakeLists.txt
    :${GroupRoot}/README.md
    Config:${GroupRoot}/CMake/CMakeLists.txt
    Config:${GroupRoot}/CMake/*.cmake
    Config/Modules:${GroupRoot}/CMake/Modules/*.cmake
)

add_custom_target(
    ${TargetName} 
    ALL SOURCES ${TargetName_SRC}
    )


set_target_properties(${TargetName} PROPERTIES FOLDER "${TargetGroup}")