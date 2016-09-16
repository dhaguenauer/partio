# - PartIO finder module
# This module searches for a valid PartIO build.

########################################################################## HDA
message(STATUS  "################## FindPartio.cmake ################## : PASS" )
########################################################################## HDA

MESSAGE( STATUS "HDA ####  FindPartio ENV{PARTIO_HOME}/lib :              " $ENV{PARTIO_HOME}/lib )


#find_path(PARTIO_LIBRARY_DIR libpartio.a
#    PATHS $ENV{PARTIO_HOME}/lib
#    DOC "PartIO library path")

find_path(PARTIO_LIBRARY_DIR libpartio.a
    PATHS $ENV{PARTIO_HOME}/lib/Linux-x86_64
    DOC "PartIO library path")

find_path(PARTIO_INCLUDE_DIR Partio.h
    PATHS $ENV{PARTIO_HOME}/include
    DOC "PartIO include path")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Partio DEFAULT_MSG
    PARTIO_LIBRARY_DIR PARTIO_INCLUDE_DIR)
