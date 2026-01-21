cmake_minimum_required(VERSION 3.16)

set(PRODUCT "prod_esp32_s3" CACHE STRING "Product directory under products/")

set(_PRODUCT_DIR "${CMAKE_SOURCE_DIR}/products/${PRODUCT}")
if(NOT EXISTS "${_PRODUCT_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Selected PRODUCT='${PRODUCT}' but '${_PRODUCT_DIR}' does not contain CMakeLists.txt")
endif()

set(EXTRA_COMPONENT_DIRS
  ${CMAKE_SOURCE_DIR}/main
  ${CMAKE_SOURCE_DIR}/utils

  # core
  ${CMAKE_SOURCE_DIR}/core/wifi_nat_router_app
  ${CMAKE_SOURCE_DIR}/core/webserver
  ${CMAKE_SOURCE_DIR}/core/user_credential_manager

  # third-party
  ${CMAKE_SOURCE_DIR}/third-party/mongoose

  # product as a component
  ${CMAKE_SOURCE_DIR}/products/${PRODUCT}
)

# search for interfaces
file(GLOB _if_dirs   RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/components/*/*_if")

# search for implementations
file(GLOB _impl_dirs RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/components/*/*_impl")

# create auto_components lists with ifs and impls
set(_auto_components "")
foreach(d IN LISTS _if_dirs _impl_dirs)
  if(EXISTS "${CMAKE_SOURCE_DIR}/${d}/CMakeLists.txt")
    list(APPEND _auto_components "${CMAKE_SOURCE_DIR}/${d}")
  endif()
endforeach()

# debug message
message(STATUS "AUTO COMPONENTS = ${_auto_components}")

list(APPEND EXTRA_COMPONENT_DIRS ${_auto_components})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wifi_nat_router)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
