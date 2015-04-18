#----------------------------------------------------------------------
#
#  CMake Config
#
#  EODB - Experimental OSM Database
#
#----------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.5)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Dev"
    CACHE STRING
    "List of available configuration types"
    FORCE)

project(osmdb)

set(EODB_VERSION_MAJOR 0)
set(EODB_VERSION_MINOR 0)
set(EODB_VERSION_PATCH 1)

set(EODB_VERSION ${EODB_VERSION_MAJOR}.${EODB_VERSION_MINOR}.${EODB_VERSION_PATCH})

add_definitions(-DEODB_VERSION="${EODB_VERSION}")

set(AUTHOR "Jochen Topf <jochen@topf.org>")


#-----------------------------------------------------------------------------
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Osmium REQUIRED COMPONENTS io sparsehash)
include_directories(${OSMIUM_INCLUDE_DIRS})


#-----------------------------------------------------------------------------
#
#  Decide which C++ version to use (Minimum/default: C++11).
#
#-----------------------------------------------------------------------------
if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        set(USE_CPP_VERSION c++11)
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=${USE_CPP_VERSION})
endif()


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
if(MSVC)
    set(USUAL_COMPILE_OPTIONS "/Ox")
else()
    set(USUAL_COMPILE_OPTIONS "-O3 -g")
endif()

if(WIN32)
    add_definitions(-DWIN32 -D_WIN32 -DMSWIN32 -DBGDWIN32
                    -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0600)
endif()

set(CMAKE_CXX_FLAGS_DEV "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during developer builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV ""
    CACHE STRING "Flags used by the linker during developer builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during RELWITHDEBINFO builds."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Build Type
#
#-----------------------------------------------------------------------------

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    if(NOT MSVC)
        add_definitions(-Werror)
    endif()
    add_definitions(${OSMIUM_WARNING_OPTIONS})
endif()

# Force RelWithDebInfo build type if none was given
if(CMAKE_BUILD_TYPE)
    set(build_type ${CMAKE_BUILD_TYPE})
else()
    set(build_type "RelWithDebInfo")
endif()

set(CMAKE_BUILD_TYPE ${build_type}
    CACHE STRING
    "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Optional "man" target to generate man pages
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for pandoc")
find_program(PANDOC pandoc)

function(add_man_page _section _name)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/man/man${_section})
    set(_output_file ${CMAKE_BINARY_DIR}/man/man${_section}/${_name}.${_section})
    set(_source_file ${CMAKE_SOURCE_DIR}/man/${_name}.md)
    set(_install_dir "share/man/man{$_section}")
    string(TOUPPER ${_name} _name_upcase)
    add_custom_command(OUTPUT ${_output_file}
        COMMAND ${PANDOC}
            ${PANDOC_MAN_OPTIONS}
            --variable "title=${_name_upcase}"
            --variable "section=${_section}"
            -o ${_output_file}
            ${_source_file}
        DEPENDS ${_source_file}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building manpage ${_name}.${_section}"
        VERBATIM)
    set(ALL_MAN_PAGES "${ALL_MAN_PAGES};${_output_file}" PARENT_SCOPE)
endfunction()


if(PANDOC)
    message(STATUS "Looking for pandoc - found")
    message(STATUS "  Manual pages will be built")
    execute_process(COMMAND date "+%Y-%m-%d" OUTPUT_VARIABLE PUBDATE OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(PANDOC_MAN_OPTIONS
        -s
        -t man
        --template ${CMAKE_CURRENT_SOURCE_DIR}/man/manpage.template
        --variable "description=eodb/${EODB_VERSION}"
        --variable "date=${PUBDATE}"
        --variable "author=${AUTHOR}"
    )
    set(PANDOC_HTML_OPTIONS -s -t html)

    add_man_page(1 osm2osr)
    add_man_page(1 osr2osm)

    install(DIRECTORY ${CMAKE_BINARY_DIR}/man DESTINATION share)

    add_custom_target(man ALL DEPENDS ${ALL_MAN_PAGES})
else()
    message(STATUS "Looking for pandoc - not found")
    message(STATUS "  Manual pages will not be built")
endif(PANDOC)


#----------------------------------------------------------------------
include_directories(${PROJECT_BINARY_DIR}/src)

add_subdirectory(src)


#----------------------------------------------------------------------
