# SDR Birdrec
SDR Birdrec is an open-source software to record FM radio signals transmitted from miniature backpacks attached to songbirds like zebra finches. It is programmed in MATLAB (GUI/frontend) and C++ (backend). Binaries are provided for 64-bit Windows.

The software, in conjunction with the backpacks, serves as a tool for studying the social behavior of songbirds. Each backpack contains a microphone or an accelerometer (acting as a contact microphone), along with a battery and an FM radio transmitter.

SDR Birdrec enables the demodulation and recording of radio signals from multiple backpack channels using a single SDR (software-defined radio) device. Since the carrier frequency of these radio signals tends to fluctuate, the software includes carrier frequency tracking capabilities. SDR Birdrec employs an multiband mixing and downsampling filterbank and a data-flow programming scheme for efficient channel extraction on multiple processor cores. Additionally, it supports the recording of one or more analog channels and the output of video frame trigger signals from an NI DAQ device, with clock synchronization between the SDR and NI DAQ. This feature allows for simultaneous recording from both the backpacks, stationary microphones and industrial cameras.

## Documentation
* https://www.authorea.com/321045/Ge2ReNOERhUSYq7S_GMYRw

## Binaries
Get the binaries from the release page:
* https://github.com/rlinus/SdrBirdrec/releases

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
* Open this repository with Microsoft Visual Studio (>= 2017).
* Open the top-level CMakeLists.txt and change the path of the dependencies on line 9ff. 
* Right-click on CMakeLists.txt in the Solution Explorer and hit `Install`.

## Licensing information
Use, modification and distribution of SDR Birdrec is subject to the GNU General Public License v3.0. See the accompanying `LICENSE.md` file.

© 2016-​2024 ETH Zurich, Linus Rüttimann
