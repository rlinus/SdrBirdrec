#pragma once
#include <algorithm>
#include <complex>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <memory>
#include <atomic>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <windows.h> //for thread priority

#include <tbb/tbb.h>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Time.hpp>

#include "SoapyDevice.h"
#include "ObjectPool.h"
#include "SyncedLogger.h"
#include "SdrDataFrame.h"
#include "Types.h"

namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;

	/*!
	* This is the SDR source streaming node.
	*/
	class SdrSourceActivity : public sender<shared_ptr<SdrDataFrame>>
	{
	private:
		SyncedLogger &logger;
		std::atomic<bool> &streamErrorFlag;

		size_t poolSize;
		size_t streamMTUsize;
		double actualSampleRate;

		atomic<bool> isStreamActiveFlag = false;
		atomic<bool> isStreamTerminatedFlag = true;
		const InitParams params;
		SoapyDevice sdr_device;
		ObjectPool< SdrDataFrame > bufferPool;
		std::thread receiverThread;
		long long activationTimeNs = 0;
		successor_type* successor = nullptr;

		const std::chrono::duration<double> externalClockLockTimeout{ 2.0 }; //how long to wait for pll lock
	public:

		SdrSourceActivity(const InitParams &params, SyncedLogger &logger, std::atomic<bool> &streamErrorFlag) :
			logger{ logger },
			streamErrorFlag{ streamErrorFlag },
			poolSize{ size_t(round(10 * params.SDR_SampleRate / params.SDR_FrameSize)) },
			params{ params },
			sdr_device{ params.SDR_DeviceArgs },
			bufferPool{ poolSize, SdrDataFrame(params) }
		{
#ifdef VERBOSE
			std::cout << "SdrSourceActivity()" << endl;
#endif

			//setup sdr device
			
			//if(params.SDR_AGC && !sdr_device.handle->hasGainMode(SOAPY_SDR_RX, 0)) throw invalid_argument("SdrSourceActivity: AGC is not supported on this device"); //hasGainMode() always returns false for UHD devices, even though they support AGC
			sdr_device.handle->setGainMode(SOAPY_SDR_RX, 0, params.SDR_AGC);

			if(!params.SDR_AGC)
			{
				SoapySDR::Range range = sdr_device.handle->getGainRange(SOAPY_SDR_RX, 0);
				auto gain = params.SDR_Gain;
				if(gain < range.minimum() || gain > range.maximum())
				{
					stringstream strstream;
					strstream << "SdrSourceActivity: The requested sdr gain is not supported on this device. It must be in the range of [" << range.minimum() << ", " << range.maximum() << "]." << endl;
					throw invalid_argument(strstream.str());
				}
				sdr_device.handle->setGain(SOAPY_SDR_RX, 0, gain);
			}

			sdr_device.handle->setSampleRate(SOAPY_SDR_RX, 0, params.SDR_SampleRate);
			if(abs(sdr_device.handle->getSampleRate(SOAPY_SDR_RX, 0) - params.SDR_SampleRate)/params.SDR_SampleRate > 0.1e-6) throw invalid_argument("SdrSourceActivity: The requested sdr samplerate is not supported on this device"); //tolearate a samplerate mismatch of 0.1ppm

			sdr_device.handle->setFrequency(SOAPY_SDR_RX, 0, params.SDR_CenterFrequency);
			if(abs(sdr_device.handle->getFrequency(SOAPY_SDR_RX, 0) - params.SDR_CenterFrequency) > 100) throw invalid_argument("SdrSourceActivity: The requested sdr center frequency is not supported on this device");

			std::vector<std::string> clockSources = sdr_device.handle->listClockSources();
			if(params.SDR_ExternalClock)
			{
				if(find(clockSources.cbegin(), clockSources.cend(), "external"s) == clockSources.cend()) throw invalid_argument("SdrSourceActivity: no external clock supported on this device");
				sdr_device.handle->setClockSource("external"s);

				//wait till pll is locked
				bool ref_locked = false;
				auto start = std::chrono::system_clock::now();
				std::chrono::duration<double> diff{ 0.0 };
				while(!ref_locked && diff < externalClockLockTimeout)
				{
					SoapySDR::ArgInfo ref_locked_args = sdr_device.handle->getSensorInfo("ref_locked");
					ref_locked = ref_locked_args.value.compare("true") == 0;
					diff = std::chrono::system_clock::now() - start;
				}
				if(!ref_locked) throw runtime_error("SdrSourceActivity: The PLL could not lock to the external clock (timeout).");
				
			}
			else if(find(clockSources.cbegin(), clockSources.cend(), "internal"s) != clockSources.cend())
			{
				sdr_device.handle->setClockSource("internal"s);
			}

			std::vector<std::string> timeSources = sdr_device.handle->listTimeSources();
			if(params.SDR_StartOnTrigger)
			{
				if(find(timeSources.cbegin(), timeSources.cend(), "external"s) == timeSources.cend() || !sdr_device.handle->hasHardwareTime("PPS")) throw invalid_argument("SdrSourceActivity: no trigger supported on this device");
				sdr_device.handle->setTimeSource("external"s);
			}
			else if(find(timeSources.cbegin(), timeSources.cend(), "internal"s) != timeSources.cend())
			{
				sdr_device.handle->setTimeSource("internal"s);
			}

			sdr_device.setupRxStream(std::is_same<SdrBirdrec::dsp_t, double>::value ? std::string(SOAPY_SDR_CF64) : std::string(SOAPY_SDR_CF32));
			streamMTUsize = sdr_device.getRxStreamMTU();
			actualSampleRate = sdr_device.handle->getSampleRate(SOAPY_SDR_RX, 0);
		}

		~SdrSourceActivity()
		{
			if(isStreamActiveFlag) deactivate();
			sdr_device.closeRxStream();
		}

		bool register_successor(successor_type &r)
		{
			if(successor)
			{
				return false;
			}
			else
			{
				successor = &r;
				return true;
			}
		}

		bool remove_successor(successor_type &r)
		{
			if(!isStreamTerminatedFlag || successor != &r) return false;
			successor = nullptr;
			return true;
		}

		void activate()
		{
			if(isStreamActiveFlag) throw logic_error("SdrSourceActivity: Can't activate stream, because it is already active.");
			if(!successor) throw logic_error("SdrSourceActivity: Can't activate stream, because no successor is registered.");
			while(receiverThread.joinable()) this_thread::yield();

			//start receiverThread (thread will wait till isStreamActiveFlag=true);
			receiverThread = thread(&SdrSourceActivity::receiverThreadFunc, this);

			//timing
			int activateFlags = 0;
			sdr_device.handle->setHardwareTime(0);

			if(params.SDR_StartOnTrigger)
			{
				activateFlags = SOAPY_SDR_HAS_TIME;
				activationTimeNs = 24 * 3600 * 1e9;
				sdr_device.handle->setHardwareTime(activationTimeNs, "PPS"); //hack to activate stream on next PPS: set time to value far from now on next pps, activate stream at that time 
			}

			//activate sdr stream
			if(sdr_device.activateRxStream(activateFlags, activationTimeNs) != 0) throw std::runtime_error("SdrSourceActivity: A Problem occured. SDR stream couldn't be activated.");

			isStreamTerminatedFlag = false;
			isStreamActiveFlag = true;
		}

		void deactivate()
		{
			if(!isStreamActiveFlag) throw logic_error("SdrSourceActivity: Can't deactivate stream. Stream is not active.");
			isStreamActiveFlag = false;
			receiverThread.join();

			//deactivate sdr stream
			if(sdr_device.deactivateRxStream() != 0) throw runtime_error("A Problem occured. Sdr stream couldn't be deactivated.");

			isStreamTerminatedFlag = true;
		}

		bool isStreamActive() { return isStreamActiveFlag; }
		bool isStreamTerminated() { return isStreamTerminatedFlag; }

		bool isRefPLLlocked(void)
		{
			SoapySDR::ArgInfo ref_locked_args = sdr_device.handle->getSensorInfo("ref_locked");
			return ref_locked_args.value.compare("true") == 0;
		}

	private:

		static void receiverThreadFunc(SdrSourceActivity* h)
		{
			if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
			{
				cout << "SdrSourceActivity: Couldn't set thread priority" << endl;
			}
			
			try
			{
				size_t output_frame_ctr = 0;
				bool sdrBackPressureReportedFlag = false;

				uint64_t sampleTickCtr = 0;
				size_t numDroppedSamples = 0;

				size_t outputBufferPos = 0;
				const size_t outputBufferSize = h->params.SDR_FrameSize;
				shared_ptr<SdrDataFrame> frame = h->bufferPool.acquire();
				if(!frame) runtime_error("no buffer available");
				frame->index = 0;

				while(!h->isStreamActiveFlag) this_thread::yield();

				while(h->isStreamActiveFlag)
				{
					
					if(numDroppedSamples == 0)
					{
						void* buf_ptr = (void*)(frame->sdr_signal.data() + outputBufferPos);
						int flags = 0;
						long long timeNs = 0;

						int nread = h->sdr_device.readStream(&buf_ptr, std::min(outputBufferSize - outputBufferPos, h->streamMTUsize), flags, timeNs, 10000000);


						if(sampleTickCtr == 0 && nread == SOAPY_SDR_TIMEOUT && h->params.SDR_StartOnTrigger) throw runtime_error("SdrSourceActivity: SOAPY_SDR_TIMEOUT error. The trigger signal has not been detected.");

						////initial non-blocking read for all available samples that can fit into the buffer
						//int nread = h->sdr_device.readStream(&buf_ptr, outputBufferSize - outputBufferPos, flags, timeNs, 0);

						////otherwise perform a blocking read on the single transfer unit size (in samples)
						//if(nread == 0 || nread == SOAPY_SDR_TIMEOUT)
						//{
						//	nread = h->sdr_device.readStream(&buf_ptr, std::min(outputBufferSize - outputBufferPos, h->streamMTUsize), flags, timeNs, 10000000);
						//}

						if(nread == SOAPY_SDR_OVERFLOW)
						{
							flags = 0;
							long long timeNs2 = 0;
							int nread2 = h->sdr_device.readStream(&buf_ptr, 0, flags, timeNs2, 10000000);
							if(nread2 < 0) throw runtime_error("SdrSourceActivity: readStream error.");
							numDroppedSamples = (size_t)SoapySDR::timeNsToTicks(timeNs2 - timeNs, h->actualSampleRate);

							stringstream strstream;
							strstream << "SdrSourceActivity: SDR overflow (" << numDroppedSamples << " samples have been lost at stream time " << sampleTickCtr/h->params.SDR_SampleRate << "s)" << endl;
							h->logger.write(strstream.str());

							continue;
						}
						else if(nread == SOAPY_SDR_TIMEOUT) throw runtime_error("SdrSourceActivity: SOAPY_SDR_TIMEOUT error.");
						else if(nread < 0) throw runtime_error("SdrSourceActivity: readStream error.");

						sampleTickCtr += nread;
						outputBufferPos += nread;
					}
					else
					{
						//if samples have been lost due to an overflow write a respective number of zeros in the output stream, so that everything stays in sync
						size_t numZeros = std::min(outputBufferSize - outputBufferPos, numDroppedSamples);
						std::fill_n(frame->sdr_signal.data() + outputBufferPos, numZeros, std::complex<dsp_t>(0.0));
						numDroppedSamples -= numZeros;
						sampleTickCtr += numZeros;
						outputBufferPos += numZeros;
					}

					if(outputBufferPos == outputBufferSize)
					{
						outputBufferPos = 0;

						//get a new empty frame
						if(!h->successor->try_put(frame)) throw runtime_error("SdrSourceActivity: successor did not accept item");
						frame = h->bufferPool.acquire();
						while(!frame)
						{
							if(!h->isStreamActiveFlag) return;
							if(!sdrBackPressureReportedFlag)
							{
								stringstream strstream2;
								strstream2 << "SdrSourceActivity: back pressure at frame " << output_frame_ctr << endl;
								h->logger.write(strstream2.str());
								sdrBackPressureReportedFlag = true;
							}
							this_thread::yield();
							frame = h->bufferPool.acquire();
						}
						frame->index = ++output_frame_ctr;
					}
				}

				frame = nullptr;
			}
			catch(exception e)
			{
				h->streamErrorFlag = true;
				stringstream strstream;
				strstream << "SdrSourceActivity: receiver thread exception: " << e.what() << endl;
				h->logger.write(strstream.str());
			}
		}
	};
}