cmake_minimum_required(VERSION 3.24)

project(SEDAS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

file(GLOB_RECURSE SEDAS_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

set(ENABLE_SKYRIM_SE OFF CACHE BOOL "Build SE 1.5 support" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "Build AE/latest support" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "Build VR support" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build CommonLib tests" FORCE)

find_package(CommonLibSSE CONFIG QUIET)

if(NOT CommonLibSSE_FOUND AND DEFINED ENV{COMMONLIB_SSE_FOLDER})
  add_subdirectory("$ENV{COMMONLIB_SSE_FOLDER}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/CommonLibSSE")
  get_target_property(COMMONLIB_SRC_DIR CommonLibSSE SOURCE_DIR)
  include("${COMMONLIB_SRC_DIR}/cmake/CommonLibSSE.cmake")
endif()

if(COMMAND add_commonlibsse_plugin)
  add_commonlibsse_plugin(${PROJECT_NAME}
    SOURCES ${SEDAS_SOURCES}
  )
else()
  add_library(${PROJECT_NAME} SHARED ${SEDAS_SOURCES})
  if(TARGET CommonLibSSE::CommonLibSSE)
    target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE)
  else()
    message(FATAL_ERROR "CommonLibSSE-NG was not found. Install the vcpkg port or set COMMONLIB_SSE_FOLDER.")
  endif()
endif()

find_package(tomlplusplus CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE tomlplusplus::tomlplusplus)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/SEDAS/PCH.h)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  UNICODE
  _UNICODE
  SEDAS_PLUGIN_NAME="SEDAS"
  SEDAS_PRETTY_NAME="Survival Saving, Soul Economy, and Death Alternative System"
)

set(SEDAS_SMF_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/SKSEMenuFramework")
if(EXISTS "${SEDAS_SMF_INCLUDE_DIR}/SKSEMenuFramework.h")
  target_include_directories(${PROJECT_NAME} PRIVATE "${SEDAS_SMF_INCLUDE_DIR}")
  target_compile_definitions(${PROJECT_NAME} PRIVATE SEDAS_WITH_SKSE_MENU_FRAMEWORK=1)
else()
  target_compile_definitions(${PROJECT_NAME} PRIVATE SEDAS_WITH_SKSE_MENU_FRAMEWORK=0)
  message(WARNING "SKSEMenuFramework.h not found. SEDAS will build without the in-game menu.")
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SEDAS_SOURCES})

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION "SKSE/Plugins"
  LIBRARY DESTINATION "SKSE/Plugins"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/SKSE/Plugins/SEDAS.toml"
  DESTINATION "SKSE/Plugins"
)
