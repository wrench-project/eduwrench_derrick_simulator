cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(%s)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CmakeModules/")

find_package(SimGrid REQUIRED)

# include directories for dependencies and WRENCH libraries
include_directories(src/ ${SimGrid_INCLUDE_DIR}/include /usr/local/include /opt/local/include /usr/local/include/wrench)

# source files
set(SOURCE_FILES
        src/ThrustDWMS.h
        src/ThrustDWMS.cpp
        src/ThrustDJobScheduler.h
        src/ThrustDJobScheduler.cpp
        src/ThrustDSimulator.cpp
       )

# test files
set(TEST_FILES
       )

# wrench library and dependencies
find_library(WRENCH_LIBRARY NAMES wrench)
find_library(WRENCH_PEGASUS_WORKFLOW_PARSER_LIBRARY NAMES wrenchpegasusworkflowparser)
find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(GTEST_LIBRARY NAMES gtest)

# generating the executable
add_executable(thrustd ${SOURCE_FILES})

if (ENABLE_BATSCHED)
target_link_libraries(thrustd
                       ${WRENCH_LIBRARY}
                       ${WRENCH_PEGASUS_WORKFLOW_PARSER_LIBRARY}
                       ${SimGrid_LIBRARY}
                       ${PUGIXML_LIBRARY}
                      -lzmq )
else()
target_link_libraries(thrustd
                       ${WRENCH_LIBRARY}
                       ${WRENCH_PEGASUS_WORKFLOW_PARSER_LIBRARY}
                       ${SimGrid_LIBRARY}
                       ${PUGIXML_LIBRARY}
                      )
endif()

install(TARGETS thrustd DESTINATION bin)

# generating unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL
                   ${SOURCE_FILES}
                   ${TEST_FILES}
               )
target_link_libraries(unit_tests
                       ${GTEST_LIBRARY} wrench -lpthread -lm
                      )
