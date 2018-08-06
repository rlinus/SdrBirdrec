cmake_minimum_required (VERSION 3.8)

project ("SdrBirdrec")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PROJECT_SOURCE_DIR}/dependencies)

# Chache variables
set(Matlab_ROOT_DIR "C:/Program Files/MATLAB/R2016a"
	CACHE PATH "Path to MATLAB root folder" FORCE)
set(SoapySDR_DIR "C:/Program Files/PothosSDR/cmake"
	CACHE PATH "Path to the folder in the PothosSDR installation that contains the PothosSDRConfig.cmake file" FORCE)
set(NIDAQmx_ROOT "C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev"
	CACHE PATH "Path to the \"DAQmx ANSI C Dev\" folder in the NIDAQmx installation" FORCE)

# Include sub-projects.
add_subdirectory ("cpp")

# install additional files
install(DIRECTORY matlab/ DESTINATION .)
install(FILES misc/SetUHD_IMAGES_DIR.ps1 DESTINATION .)
