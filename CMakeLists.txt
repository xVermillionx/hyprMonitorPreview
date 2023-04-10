# CMakeLists.txt
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(hyprMonitorPreview)
# project(main LANGUAGES CXX C)

set(EXECUTABLE_NAME ${CMAKE_PROJECT_NAME})

IF( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    SET(IS_FREEBSD 0)
    SET(IS_LINUX 1)
    SET(IS_SUNOS 0)
ELSEIF( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
    SET(IS_MACOS 0)
  ELSEIF( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
    SET(IS_WINOS 0)
ELSEIF( CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" )
    SET(IS_FREEBSD 1)
    SET(IS_LINUX 0)
    SET(IS_SUNOS 0)
ELSEIF( CMAKE_SYSTEM_NAME STREQUAL "SunOS" )
    SET(IS_FREEBSD 0)
    SET(IS_LINUX 0)
    SET(IS_SUNOS 1)
ENDIF()

# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Never set in cmake file use env vars and can only be set once
# set(CMAKE_C_COMPILER gcc)
# set(CMAKE_CXX_COMPILER g++)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}) # Change if needed
if (NOT EXISTS ${ROOT_DIR}/.ccls)
  file(CONFIGURE OUTPUT ${ROOT_DIR}/.ccls CONTENT compile_commands.json)
endif()
if (EXISTS ${ROOT_DIR}/.ccls)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  file(CREATE_LINK ${CMAKE_BINARY_DIR}/compile_commands.json ${ROOT_DIR}/compile_commands.json COPY_ON_ERROR SYMBOLIC)
endif()

set(MY_LIBS -lm -ljsoncpp -lncurses)

if(CMAKE_COMPILER_IS_GNUCC)
  message(NOTICE "Using gcc")
  add_library(compile_flags INTERFACE)
  target_compile_options(compile_flags INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Werror=vla -lstdc++)
elseif(CMAKE_COMPILER_IS_GNUCXX)
  message(NOTICE "Using g++")
  add_library(compile_flags INTERFACE)
  target_compile_options(compile_flags INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Werror=vla)
else()
  message(FATAL_ERROR "unknown compiler detected")
endif()

add_custom_target(run
        COMMAND ${CMAKE_BINARY_DIR}/${EXECUTABLE_NAME}
        DEPENDS ${EXECUTABLE_NAME}
        COMMENT "Running Program")

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")
add_executable(${EXECUTABLE_NAME} ${SOURCES})
target_link_libraries(${EXECUTABLE_NAME} compile_flags ${MY_LIBS})

target_compile_options(${EXECUTABLE_NAME} PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Wconversion
  -Werror=vla
  -lstdc++
)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

set(INSTALLPATH ${CMAKE_INSTALL_BINDIR} "/usr/bin")
install(TARGETS ${EXECUTABLE_NAME}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION ${INSTALLPATH})

add_custom_target(uninstall
        COMMAND rm -rf ${INSTALLPATH}/${EXECUTABLE_NAME}
        DEPENDS ${EXECUTABLE_NAME}
        COMMENT "Installing Program")

