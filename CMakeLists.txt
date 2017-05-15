cmake_minimum_required(VERSION 3.0)
project(by.pixie)

if(NOT WIN32)
 return()
endif()

set(CMAKE_SUPPRESS_REGENERATION ON)
set(CMAKE_MACOSX_RPATH OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(C74SUPPORT ${CMAKE_CURRENT_SOURCE_DIR}/max-sdk/source/c74support)
set(MAX_ROOT ${C74SUPPORT}/max-includes)

add_library(MaxAPI INTERFACE IMPORTED)

set_target_properties(MaxAPI PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${MAX_ROOT})
set_target_properties(MaxAPI PROPERTIES INTERFACE_LINK_LIBRARIES ${MAX_ROOT}/MaxAPI.lib)

set_target_properties(MaxAPI PROPERTIES 
    INTERFACE_COMPILE_DEFINITIONS 
    "WIN_VERSION;_CRT_SECURE_NO_WARNINGS;VER_TARGETNAME=\"by.pixie\"")

add_library(by.pixie MODULE 
    pixie-driver.cpp 
    pixie/usb_driver.cpp
)

target_include_directories(by.pixie PRIVATE pixie)

set_target_properties(by.pixie PROPERTIES CXX_STANDARD 17)
target_link_libraries(by.pixie PRIVATE
        MaxAPI
        ${CMAKE_CURRENT_SOURCE_DIR}/pixie/ftd2xx.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".mxe"
)
