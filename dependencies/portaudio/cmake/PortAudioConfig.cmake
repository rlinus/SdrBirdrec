# PortAudioConfig.cmake:
#
# Imported Targets defined:
#	PortAudio::portaudio - shared PortAudio library		
#
# Variables defined:
#	PortAudio_INCLUDE_DIRS - path to include directory
#	PortAudio_LIBRARIES - path to .lib file
#	PortAudio_SHARED_LIBRARIES - path to .dll file

get_filename_component(PortAudio_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../include" ABSOLUTE)
get_filename_component(PortAudio_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/../lib/portaudio_x64.lib" ABSOLUTE)
get_filename_component(PortAudio_SHARED_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/../bin/portaudio_x64.dll" ABSOLUTE)

add_library(PortAudio::portaudio SHARED IMPORTED)
target_include_directories(PortAudio::portaudio INTERFACE ${PortAudio_INCLUDE_DIRS})
set_property(TARGET PortAudio::portaudio PROPERTY IMPORTED_IMPLIB ${PortAudio_LIBRARIES})
set_property(TARGET PortAudio::portaudio PROPERTY IMPORTED_LOCATION ${PortAudio_SHARED_LIBRARIES})

