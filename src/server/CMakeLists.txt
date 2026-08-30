# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Enforce compileparameters for corebuilds under GCC
# This to stop a few silly crashes that could have been avoided IF people
# weren't doing some -O3 psychooptimizations etc.

include_directories(
${Boost_INCLUDE_DIR}
)

if(CMAKE_COMPILER_IS_GNUCXX AND NOT MINGW)
  add_definitions(-fno-delete-null-pointer-checks)
endif()

if(WIN32)
  list(APPEND sources_windows
    ${CMAKE_SOURCE_DIR}/src/common/Platform/ServiceWin32.cpp
    ${CMAKE_SOURCE_DIR}/src/common/Platform/ServiceWin32.h
  )
endif(WIN32)

add_subdirectory(database)
add_subdirectory(shared)
add_subdirectory(authserver)
add_subdirectory(game)

# The playerbot module lives outside this submodule, so it needs an explicit binary
# directory -- CMake cannot derive one for a source dir outside the source tree.
# A missing tree is a hard error: -DPRABOBOTS=1 quietly producing a core without bots
# is the worst failure mode available here.
if(PRABOBOTS)
  if(NOT EXISTS "${PRABOBOTS_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "PRABOBOTS=1 but no CMakeLists.txt found at ${PRABOBOTS_SOURCE_DIR}")
  endif()
  add_subdirectory("${PRABOBOTS_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/src/prabobots")
endif()

add_subdirectory(scripts)
add_subdirectory(worldserver)
