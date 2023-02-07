# -----------------------------------------------------------------------------
#
#   Copyright (c) Charles Carley.
#
#   This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
#   Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
# ------------------------------------------------------------------------------
set(TestFile ${CMAKE_SOURCE_DIR}/Test/googletest/CMakeLists.txt)
set(GitUpdate_SUCCESS FALSE)

# Attempts to pull submodules with python or git.
find_package(Python COMPONENTS Interpreter)

if(Python_Interpreter_FOUND)

	if (NOT EXISTS ${TestFile})
		execute_process(COMMAND ${Python_EXECUTABLE} 
				${CMAKE_SOURCE_DIR}/gitupdate.py
				WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
	endif()


	if (EXISTS ${TestFile})
		set(GitUpdate_SUCCESS TRUE)
	endif()

else()

	find_package(Git)

	if (GIT_FOUND)
		if (NOT EXISTS ${TestFile})
			execute_process(COMMAND ${GIT_EXECUTABLE} 
					submodule update --init
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			)
		endif()
		if (EXISTS ${TestFile})
			set(GitUpdate_SUCCESS TRUE)
		endif()
	endif()
endif()


if (NOT GitUpdate_SUCCESS)
	message("")
	message("GitUpdate: Was unable to automatically pull required submodules.")
	message("Please manually invoke: ")
	message("git submodule update --init")
endif()
