# LibsamplerateConfig.cmake:
#
# Imported Targets defined:
#	Libsamplerate::libsamplerate - shared libsamplerate library		
#
# Variables defined:
#	Libsamplerate_INCLUDE_DIRS - path to include directory
#	Libsamplerate_LIBRARIES - path to .lib file
#	Libsamplerate_SHARED_LIBRARIES - path to .dll file

get_filename_component(Libsamplerate_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../include" ABSOLUTE)
get_filename_component(Libsamplerate_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/../lib/libsamplerate-0.lib" ABSOLUTE)
get_filename_component(Libsamplerate_SHARED_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/../bin/libsamplerate-0.dll" ABSOLUTE)

add_library(Libsamplerate::libsamplerate SHARED IMPORTED)
target_include_directories(Libsamplerate::libsamplerate INTERFACE ${Libsamplerate_INCLUDE_DIRS})
set_property(TARGET Libsamplerate::libsamplerate PROPERTY IMPORTED_IMPLIB ${Libsamplerate_LIBRARIES})
set_property(TARGET Libsamplerate::libsamplerate PROPERTY IMPORTED_LOCATION ${Libsamplerate_SHARED_LIBRARIES})

