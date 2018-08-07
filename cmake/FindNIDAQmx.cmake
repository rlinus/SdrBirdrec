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
#  NIDAQmx_ExternalCompilerSupport_DIR - path to "ExternalCompilerSupport" directory

if(NIDAQmx_ROOT)
	file(TO_CMAKE_PATH ${NIDAQmx_ExternalCompilerSupport_DIR} NIDAQmx_ROOT)

	find_path(NIDAQmx_INCLUDE_DIR NIDAQmx.h PATHS ${NIDAQmx_ExternalCompilerSupport_DIR}/C/include DOC "The NIDAQmx include directory")
	find_library(NIDAQmx_LIBRARY NAMES NIDAQmx PATHS ${NIDAQmx_ExternalCompilerSupport_DIR}/C/lib64/msvc DOC "The NIDAQmx library")
else(NIDAQmx_ROOT)
	find_path(NIDAQmx_INCLUDE_DIR NIDAQmx.h PATHS "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C/include" DOC "The NIDAQmx include directory")
	find_library(NIDAQmx_LIBRARY NAMES NIDAQmx PATHS "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C/lib64/msvc" DOC "The NIDAQmx library")
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