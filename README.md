# Template

This template defines the CMake layout for a stand-alone / embedded project. The primary purpose for this structure is to have sub-modules that stand on their own, or included into larger projects as a component piece. If the module itself requires extra dependencies then they should be included into the module as sub-modules. 

The `${PROJECT}_ExternalTarget` variable is used to determine whether or not to compile dependencies locally or have them resolved later in the larger project. If `${PROJECT}_ExternalTarget` is false then the module should be built as a standalone project. 

Dependencies should adhere to the same naming strategy to keep everything consistent. Sub-modules should define the same include and library variables relative to the `${PROJECT}_ExternalTarget` variable.

For example:

```cmake
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

## CMakeLists.txt - in larger project

set(Utils_ExternalTarget  TRUE)
set(Utils_TargetName      UtilsAlias)
set(Utils_TargetGroup     VisualStudioFolderName)
set(Utils_INCLUDE         some/path/to/utils/module)
set(Utils_LIBRARY         ${Utils_TargetName})
subdirs(Utils)

## CMakeLists.txt - In Utils


if (Utils_ExternalTarget)
    # Copy of the larger project's settings

    set(TargetFolders ${Utils_TargetFolders})
    set(TargetName    ${Utils_TargetName})
    set(TargetGroup   ${Utils_TargetGroup})
else()
    # Settings for the standalone project

    set(TargetFolders FALSE)
    set(TargetName    Utils)
    set(Utils_INCLUDE internal/path/to/utils/module)
    set(Utils_LIBRARY ${TargetName})
endif()


## When building the library 

add_library(${TargetName} ... )
if (TargetFolders)
    set_target_properties(
        ${TargetName} 
        PROPERTIES FOLDER "${TargetGroup}"
    )
endif()


## When linking against the module

include_directories(${Utils_INCLUDE})

add_executable(HelloWorld main.cpp)

target_link_libraries(HelloWorld  ${Utils_LIBRARY})
```

## Sub-modules

The files [gitupdate.py](gitupdate.py) or [gitupdate.bat](gitupdate.bat) help automate initial cloning.

Once this project has been cloned. The following command will initialize any external modules.

```txt
python gitupdate.py 
...
gitupdate.bat 
```

`gitupdate.py` will also update sub-modules to the latest head. 

## Testing

The testing directory is setup to work with [googletest](https://github.com/google/googletest). As well as the initial setup for testing the standalone module either using GitHub actions or with Jenkins.

## Building

Building with CMake and Make

```sh
mkdir build
cd build
cmake ..

make
```

Optional defines.

| Option                      | Description                                          | Default |
|:----------------------------|:-----------------------------------------------------|:-------:|
| Template_BUILD_TEST         | Build the unit test program.                         |   ON    |
| Template_AUTO_RUN_TEST      | Automatically run the test program.                  |   OFF   |
| Template_USE_STATIC_RUNTIME | Build with the MultiThreaded(Debug) runtime library. |   ON    |
