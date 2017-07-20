cmake_minimum_required(VERSION 2.4.6)

if (BUILD_PARTIO_MTOA)
    find_package(MtoA)
endif ()


########################################################################## HDA
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : BUILD_PARTIO_MTOA  = ${BUILD_PARTIO_MTOA}")
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : MTOA_FOUND  = ${MTOA_FOUND}")
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : MTOA_ROOT  = ${MTOA_ROOT}")
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : ARNOLD_FOUND  = ${ARNOLD_FOUND}")
########################################################################## HDA

#if(MTOA_FOUND)
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : add_subdirectory  plugin")
add_subdirectory(plugin)


if (ARNOLD_VERSION_ARCH_NUM VERSION_GREATER "4")
    add_compile_options(-std=c++11)
endif ()

add_subdirectory(procedurals)
add_subdirectory(shaders)

if(ARNOLD_FOUND)
	add_subdirectory(procedurals)
endif()
########################################################################## HDA