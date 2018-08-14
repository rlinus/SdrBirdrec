#pragma once
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

	class SdrSourceActivity : public sender<shared_ptr<SdrDataFrame>>
	{
	private:
		template <typename T> struct RingBuffer
		{
		private:
			vector<T> data;

			size_t size_;
			atomic<size_t> count{ 0 };
			size_t front_{ 0 };
			size_t back_{ 0 };
		public:
			RingBuffer(size_t size, const T& element = T()) :
				size_{ size },
				data(size, element)
			{}

			T& back() { return data[back_]; };
			T& front() { return data[front_]; };

			void push_back()
			{
				back_ = back_ == size_ - 1 ? 0 : back_ + 1;
				++count;
			}

			void pop_front()
			{
				front_ = front_ == size_ - 1 ? 0 : front_ + 1;
				--count;
			}

			size_t read_available() { return count; }
			size_t write_available() { return size_ - count; }
			void flush() { front_ = 0; back_ = 0; count = 0; }
		};

		struct RingBufferElement
		{
			RingBufferElement(size_t n) :
				buf(n)
			{}

			vector<complex<dsp_t>> buf;
			int nread;
			int flags;
			long long timeNs;
		};

		SyncedLogger &logger;

		size_t poolSize;
		size_t ringBufferSize;
		size_t streamMTUsize;

		double actualSampleRate;

		atomic<bool> isStreamActiveFlag = false;
		atomic<bool> isStreamTerminatedFlag = true;
		const InitParams params;
		SoapyDevice sdr_device;
		unique_ptr< RingBuffer<RingBufferElement> > ringBuffer;
		ObjectPool< SdrDataFrame > bufferPool;
		std::thread receiverThread;
		std::thread serviceThread;
		long long activationTimeNs = 0;
		successor_type* successor = nullptr;

		const std::chrono::duration<double> externalClockLockTimeout{ 2.0 }; //how long to wait for pll lock
	public:

		SdrSourceActivity(const InitParams &params, SyncedLogger &logger) :
			logger{ logger },
			poolSize{ size_t(round(5 * params.SDR_SampleRate / params.SDR_FrameSize)) },
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
			if(abs(sdr_device.handle->getSampleRate(SOAPY_SDR_RX, 0) - params.SDR_SampleRate) > 1) throw invalid_argument("SdrSourceActivity: The requested sdr samplerate is not supported on this device");
			//cout << "SR diff:" << sdr_device.handle->getSampleRate(SOAPY_SDR_RX, 0) - params.SDR_SampleRate << endl;

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
			ringBufferSize = size_t(3 * actualSampleRate / streamMTUsize);
			ringBuffer = make_unique<RingBuffer<RingBufferElement>>(ringBufferSize, RingBufferElement(streamMTUsize));
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
			ringBuffer->flush();

			//timing
			int activateFlags = 0;
			sdr_device.handle->setHardwareTime(0);

			if(params.SDR_StartOnTrigger)
			{
				//sdr_device.handle->setHardwareTime(0);
				activateFlags = SOAPY_SDR_HAS_TIME;
				activationTimeNs = 24 * 3600 * 1e9;
				sdr_device.handle->setHardwareTime(activationTimeNs, "PPS"); //hack to activate stream on next PPS: set time to value far from now on next pps, activate stream at that time 
			}
			//else if(sdr_device.handle->hasHardwareTime())
			//{
			//	activationTimeNs = 500e6;
			//	sdr_device.handle->setHardwareTime(0);
			//}

			//activate sdr stream
			if(sdr_device.activateRxStream(activateFlags, activationTimeNs) != 0) throw std::runtime_error("A Problem occured. Sdr stream couldn't be activated.");

			//start threads
			isStreamTerminatedFlag = false;
			isStreamActiveFlag = true;
			receiverThread = thread(&SdrSourceActivity::receiverThreadFunc, this);
			serviceThread = thread(&SdrSourceActivity::serviceThreadFunc, this);
		}

		void deactivate()
		{
			if(!isStreamActiveFlag) throw logic_error("SdrSourceActivity: Can't deactivate stream. Stream is not active.");
			isStreamActiveFlag = false;
			receiverThread.join();

			//deactivate sdr stream
			if(sdr_device.deactivateRxStream() != 0) throw runtime_error("A Problem occured. Sdr stream couldn't be deactivated.");

			serviceThread.join();
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
			try
			{
				if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
				{
					cout << "SdrSourceActivity: Couldn't set thread priority" << endl;
				}

				size_t ctr = 0;
				const size_t streamMTUsize = h->streamMTUsize;
				//const size_t activationTimeNs = h->activationTimeNs;


				//sdr loop
				while(h->isStreamActiveFlag)
				{
					if(!h->ringBuffer->write_available()) continue;

					auto& ringBufferElement = h->ringBuffer->back();
					void* buf_ptr = (void*)ringBufferElement.buf.data();
					ringBufferElement.flags = 0;
					ringBufferElement.nread = h->sdr_device.readStream(&buf_ptr, streamMTUsize, ringBufferElement.flags, ringBufferElement.timeNs, 600000000);
					//ringBufferElement.timeNs -= activationTimeNs;
					h->ringBuffer->push_back();

					++ctr;
				}

			}
			catch(exception e)
			{
				cout << "SdrSourceActivity: receiver thread exception: " << e.what() << endl;
			}
		}

		static void serviceThreadFunc(SdrSourceActivity* h)
		{
			try
			{
				size_t output_frame_ctr = 0;
				size_t input_frame_ctr = 0;
				bool sdrBackPressureReportedFlag = false;

				bool had_an_overflow = false;
				long long lastTimeNS;

				size_t outputBufferPos = 0;
				const size_t outputBufferSize = h->params.SDR_FrameSize;
				shared_ptr<SdrDataFrame> frame = h->bufferPool.acquire();
				if(!frame) runtime_error("no buffer available");
				frame->index = 0;

				while(h->isStreamActiveFlag)
				{
					if(!h->ringBuffer->read_available())
					{
						this_thread::yield();
						continue;
					}

					auto& ringBufferElement = h->ringBuffer->front();

					if(ringBufferElement.nread == SOAPY_SDR_OVERFLOW)
					{
						if(!had_an_overflow)
						{
							had_an_overflow = true;
							lastTimeNS = ringBufferElement.timeNs;
						}
						h->ringBuffer->pop_front();
						++input_frame_ctr;
						continue;
					}
					else if(ringBufferElement.nread < 0)
					{
						throw runtime_error("SdrSourceActivity: Sdr receive error.");
					}

					if(had_an_overflow)
					{
						had_an_overflow = false;
						size_t numDroppedSamples = (size_t)SoapySDR::timeNsToTicks(ringBufferElement.timeNs - lastTimeNS, h->actualSampleRate);

						stringstream strstream;
						if(ringBufferElement.flags & SOAPY_SDR_HAS_TIME)
						{
							strstream << "SdrSourceActivity: dropped " << numDroppedSamples << " samples at time " << (ringBufferElement.timeNs - h->activationTimeNs) / 1e9 << "s." << endl;
						}
						else
						{
							strstream << "SdrSourceActivity: Sdr overflow." << endl;
						}
						h->logger.write(strstream.str());

						while(numDroppedSamples > 0)
						{
							if(numDroppedSamples < outputBufferSize - outputBufferPos)
							{
								fill_n(frame->sdr_signal.begin() + outputBufferPos, numDroppedSamples, complex<dsp_t>(0));
								outputBufferPos += numDroppedSamples;
								numDroppedSamples = 0;
							}
							else
							{
								fill_n(frame->sdr_signal.begin() + outputBufferPos, outputBufferSize - outputBufferPos, complex<dsp_t>(0));
								numDroppedSamples -= outputBufferSize - outputBufferPos;
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
					}

					size_t inputBufferPos = 0;
					const size_t inputBufferSize = ringBufferElement.nread;

					while(inputBufferPos < inputBufferSize)
					{
						if(inputBufferSize - inputBufferPos < outputBufferSize - outputBufferPos)
						{
							copy_n(ringBufferElement.buf.cbegin() + inputBufferPos, inputBufferSize - inputBufferPos, frame->sdr_signal.begin() + outputBufferPos);
							outputBufferPos += inputBufferSize - inputBufferPos;
							inputBufferPos = inputBufferSize;
						}
						else
						{
							copy_n(ringBufferElement.buf.cbegin() + inputBufferPos, outputBufferSize - outputBufferPos, frame->sdr_signal.begin() + outputBufferPos);
							inputBufferPos += outputBufferSize - outputBufferPos;
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

					h->ringBuffer->pop_front();
					++input_frame_ctr;
				}
				frame = nullptr;
			}
			catch(exception e)
			{
				stringstream strstream;
				strstream << "SdrSourceActivity: service thread exception: " << e.what() << endl;
				h->logger.write(strstream.str());
			}
		}
	};
}