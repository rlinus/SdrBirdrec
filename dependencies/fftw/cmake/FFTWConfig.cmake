# FFTWConfig.cmake:
#
# Imported Targets defined:
#	FFTW::double - shared FFTW library (double precision)
#	FFTW::single - shared FFTW library (single precision)
#
# Variables defined:
#	FFTW_INCLUDE_DIRS - path to include directory
#	FFTW_LIBRARIES - path to .lib files
#	FFTW_SHARED_LIBRARIES - path to .dll files

get_filename_component(FFTW_LIBRARY_DOUBLE ${CMAKE_CURRENT_LIST_DIR}/../lib/libfftw3-3.lib ABSOLUTE)
get_filename_component(FFTW_LIBRARY_SINGLE ${CMAKE_CURRENT_LIST_DIR}/../lib/libfftw3f-3.lib ABSOLUTE)

get_filename_component(FFTW_SHARED_LIBRARY_DOUBLE ${CMAKE_CURRENT_LIST_DIR}/../bin/libfftw3-3.dll ABSOLUTE)
get_filename_component(FFTW_SHARED_LIBRARY_SINGLE ${CMAKE_CURRENT_LIST_DIR}/../bin/libfftw3f-3.dll ABSOLUTE)

get_filename_component(FFTW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../include ABSOLUTE)
set(FFTW_LIBRARIES ${FFTW_LIBRARY_DOUBLE} ${FFTW_LIBRARY_SINGLE})
set(FFTW_SHARED_LIBRARIES ${FFTW_SHARED_LIBRARY_DOUBLE} ${FFTW_SHARED_LIBRARY_SINGLE})

add_library(FFTW::double SHARED IMPORTED)
target_include_directories(FFTW::double INTERFACE ${FFTW_INCLUDE_DIRS})
set_property(TARGET FFTW::double PROPERTY IMPORTED_IMPLIB ${FFTW_LIBRARY_DOUBLE})
set_property(TARGET FFTW::double PROPERTY IMPORTED_LOCATION ${FFTW_SHARED_LIBRARY_DOUBLE})

add_library(FFTW::single SHARED IMPORTED)
target_include_directories(FFTW::single INTERFACE ${FFTW_INCLUDE_DIRS})
set_property(TARGET FFTW::single PROPERTY IMPORTED_IMPLIB ${FFTW_LIBRARY_SINGLE})
set_property(TARGET FFTW::single PROPERTY IMPORTED_LOCATION ${FFTW_SHARED_LIBRARY_SINGLE})

