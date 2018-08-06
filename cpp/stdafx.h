#pragma once
#define NOMINMAX
#include <tbb/tbbmalloc_proxy.h>

#include <cstdint>
#include <string>
#include <cstring>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <complex>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>
#include <numeric>
#include <utility>
#include <algorithm>

#include <Windows.h>
#include <processenv.h>

#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Device.hpp>
#include <NIDAQmx.h>


#include <mex.h>

#include <tbb/tbb.h>
#include <sndfile.hh>
