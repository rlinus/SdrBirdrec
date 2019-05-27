cmake_minimum_required (VERSION 3.8)

project (SdrBirdrec VERSION 0.69.3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PROJECT_SOURCE_DIR}/dependencies)

# Chache variables
set(Matlab_ROOT_DIR "C:/Program Files/MATLAB/R2018b"
	CACHE PATH "Path to MATLAB root folder" FORCE)
set(SoapySDR_DIR "C:/Program Files/PothosSDR/cmake"
	CACHE PATH "Path to the folder in the PothosSDR installation that contains the PothosSDRConfig.cmake file" FORCE)
set(NIDAQmx_ExternalCompilerSupport_DIR "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport"
	CACHE PATH "Path to the ExternalCompilerSupport folder in the NIDAQmx installation" FORCE)
set(TBB_DIR "C:/CMake_Packages/tbb2018_20180618oss/cmake"
	CACHE PATH "Path to the folder in the Intel TBB installation that contains the PothosSDRConfig.cmake file" FORCE)

# Include sub-projects.
add_subdirectory ("cpp")

# install matlab files
install(FILES matlab/sdr_birdrec.fig matlab/sdr_birdrec.m matlab/SdrBirdrecBackend.m matlab/getChannels.m matlab/getSdrDevice.m matlab/recorder.m DESTINATION .)

# config cpack
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_GENERATOR "ZIP")
include(CPack)

