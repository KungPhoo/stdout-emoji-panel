# CMakeList.txt : CMake project for stdout-emoji-panel, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin")

project ("emoji-panel")

# Add source to this project's executable.
file(GLOB APP_SOURCES # cpp files
    "*.cpp" "*.md"
    )
file(GLOB APP_HEADERS # headers
    "*.h"
    )
add_executable (emoji-panel ${APP_SOURCES} ${APP_HEADERS} )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET emoji-panel PROPERTY CXX_STANDARD 20)
endif()

if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wa,-mbig-obj)
endif()
if (MSVC)
    # add_compile_options("/bigobj") add_compile_options -> does not seem to work. Must use CMAKE_CXX_FLAGS
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj") # Increase Number of Sections in .Obj file
endif()
