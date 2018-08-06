# LibsndfileConfig.cmake:
#
# Imported Targets defined:
#	Libsndfile::libsndfile - shared libsndfile library		
#
# Variables defined:
#	Libsndfile_INCLUDE_DIRS - path to include directory
#	Libsndfile_LIBRARIES - path to .lib file
#	Libsndfile_SHARED_LIBRARIES - path to .dll file

get_filename_component(Libsndfile_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../include" ABSOLUTE)
get_filename_component(Libsndfile_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/../lib/libsndfile-1.lib" ABSOLUTE)
get_filename_component(Libsndfile_SHARED_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/../bin/libsndfile-1.dll" ABSOLUTE)

add_library(Libsndfile::libsndfile SHARED IMPORTED)
target_include_directories(Libsndfile::libsndfile INTERFACE ${Libsndfile_INCLUDE_DIRS})
set_property(TARGET Libsndfile::libsndfile PROPERTY IMPORTED_IMPLIB ${Libsndfile_LIBRARIES})
set_property(TARGET Libsndfile::libsndfile PROPERTY IMPORTED_LOCATION ${Libsndfile_SHARED_LIBRARIES})

