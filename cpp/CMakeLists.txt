cmake_minimum_required (VERSION 3.8)

# find packages
find_package(SoapySDR REQUIRED)
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY MAT_LIBRARY)
find_package(TBB REQUIRED)
find_package(FFTW REQUIRED)
find_package(Libsndfile REQUIRED)
find_package(Libsamplerate REQUIRED)
find_package(PortAudio REQUIRED)
find_package(NIDAQmx REQUIRED)

# define mex target
set(SRC_FILES SdrBirdrecMex.cpp mexUtils.cpp Topology.cpp SdrBirdrecBackend.cpp SdrBirdrecMex.mexw64.manifest)  
#Note: The manifest file is necessary for that the right instances of the Intel TBB libraries are loaded. Matlab itself uses Intel TBB, this seems to cause trouble, if there is no manifest file.

matlab_add_mex(NAME SdrBirdrecMex SRC ${SRC_FILES})
target_include_directories(SdrBirdrecMex PRIVATE ${SoapySDR_INCLUDE_DIRS})
target_link_libraries (SdrBirdrecMex ${SoapySDR_LIBRARIES} TBB::tbb TBB::tbbmalloc TBB::tbbmalloc_proxy NIDAQmx::nidaqmx Libsndfile::libsndfile Libsamplerate::libsamplerate PortAudio::portaudio FFTW::single FFTW::double)
install(TARGETS SdrBirdrecMex DESTINATION bin)

#get location of .dll files from imported targets
get_property(Libsndfile_LOCATION TARGET Libsndfile::libsndfile PROPERTY LOCATION)
get_property(Libsamplerate_LOCATION TARGET Libsamplerate::libsamplerate PROPERTY LOCATION)
get_property(PortAudio_LOCATION TARGET PortAudio::portaudio PROPERTY LOCATION)
get_property(FFTW_single_LOCATION TARGET FFTW::single PROPERTY LOCATION)
get_property(FFTW_double_LOCATION TARGET FFTW::double PROPERTY LOCATION)
get_property(TBB_tbb_LOCATION TARGET TBB::tbb PROPERTY LOCATION)
get_property(TBB_tbbmalloc_LOCATION TARGET TBB::tbbmalloc PROPERTY LOCATION)
get_property(TBB_tbbmalloc_proxy_LOCATION TARGET TBB::tbbmalloc_proxy PROPERTY LOCATION)

#install .dll files
install(PROGRAMS ${Libsndfile_LOCATION} ${Libsamplerate_LOCATION} ${PortAudio_LOCATION} ${FFTW_single_LOCATION} ${FFTW_double_LOCATION} ${TBB_tbb_LOCATION} ${TBB_tbbmalloc_LOCATION} ${TBB_tbbmalloc_proxy_LOCATION} DESTINATION bin)

#install the manifest file to ensure that the right instance of the TBB library is loaded
install(FILES SdrBirdrecMex.tbb.manifest DESTINATION bin)