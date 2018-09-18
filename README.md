# SDR Birdrec
SDR Birdrec is an open-source software to record FM radio signals transmitted from miniature backpacks attached to songbirds like zebra finches. It is programmed in MATLAB (GUI/frontend) and C++ (backend). Binaries are provided for 64-bit Windows.

## Documentation
* https://www.authorea.com/321045/Ge2ReNOERhUSYq7S_GMYRw

## Dependencies
* CMake: as a build system
* MATLAB: for the frontend and the mex interface to the backend
* [Intel TBB](https://github.com/01org/tbb): for the parallelization of the data acquisation and signal processing with a data flow graph
* [Soapy SDR](https://github.com/pothosware/SoapySDR): to interface with SDR devices
* [NI-DAQmx](https://www.ni.com/dataacquisition/nidaqmx.htm): to interface with DAQ devices
* [FFTW](https://github.com/FFTW/fftw3): for the FFT in the fast convolution filter-bank
* [PortAudio](http://www.portaudio.com/): for audio output
* [libsndfile](http://www.mega-nerd.com/libsndfile/): to write sound files
* [libsamplerate](http://www.mega-nerd.com/SRC/): for sample rate conversions

This repository contains pre-compiled shared libraries (.dll) for FFTW, PortAudio, libsndfile and libsamplerate to simplify compilation.

## How to Compile
* Open this repository with Ms Visual Studio (>= 2017).
* Open the top-level CMakeLists.txt and change the path of the dependencies on line 9ff. 
* Right click on CMakeLists.txt in the Solution Explorer and hit `Install`.

## Licensing information
Use, modification and distribution is subject to the GNU General Public License v3.0. See accompanying file `LICENSE.md`.