# FindNIDAQmx.cmake: try to find NIDAQmx library
#
# Cache Variables: (probably not for direct use in your scripts)
#  NIDAQmx_INCLUDE_DIR
#  NIDAQmx_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  NIDAQmx_FOUND
#  NIDAQmx_INCLUDE_DIRS
#  NIDAQmx_LIBRARIES
#
# Variables used:
#  NIDAQmx_ROOT - path to "DAQmx ANSI C Dev" directory

if(NIDAQmx_ROOT)
	find_path(NIDAQmx_INCLUDE_DIR NIDAQmx.h PATHS ${NIDAQmx_ROOT}/include DOC "The NIDAQmx include directory")
	find_library(NIDAQmx_LIBRARY NAMES NIDAQmx PATHS ${NIDAQmx_ROOT}/lib/msvc ${NIDAQmx_ROOT}/lib64/msvc DOC "The NIDAQmx library")
else(NIDAQmx_ROOT)
	find_path(NIDAQmx_INCLUDE_DIR NIDAQmx.h PATHS "C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev/include" DOC "The NIDAQmx include directory")
	find_library(NIDAQmx_LIBRARY NAMES NIDAQmx PATHS "C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev/lib/msvc" DOC "The NIDAQmx library")
endif(NIDAQmx_ROOT)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NIDAQmx DEFAULT_MSG NIDAQmx_LIBRARY NIDAQmx_INCLUDE_DIR)

if(NIDAQmx_FOUND)
	set(NIDAQmx_LIBRARIES ${NIDAQmx_LIBRARY})
	set(NIDAQmx_INCLUDE_DIRS ${NIDAQmx_INCLUDE_DIR})

	add_library(NIDAQmx::nidaqmx SHARED IMPORTED)
	target_include_directories(NIDAQmx::nidaqmx INTERFACE ${NIDAQmx_INCLUDE_DIRS})
	set_property(TARGET NIDAQmx::nidaqmx PROPERTY IMPORTED_IMPLIB ${NIDAQmx_LIBRARY})
endif()

mark_as_advanced(NIDAQmx_INCLUDE_DIR NIDAQmx_LIBRARY)