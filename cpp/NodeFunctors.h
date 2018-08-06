#pragma once

#include <tbb/tbbmalloc_proxy.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <memory>
#include <atomic>

#include <tbb/tbb.h>

#include <NIDAQmx.h>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Time.hpp>
#include <portaudio.h>
#include <sndfile.hh>
#include <samplerate.h>


#include "fftwcpp.h"
#include "DspUtils.h"
#include "DataFrame.h"
#include "InitParams.h"
#include "SoapyDevice.h"
#include "ObjectPool.h"
#include "SyncedLogger.h"

#include "Types.h"

namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;

	class SdrSourceActivity : public sender<shared_ptr<DataFrame>>
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
			RingBuffer(size_t size, const T& element = T()):
				size_{size},
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
		SoapyDevice &sdr_device;	
		unique_ptr< RingBuffer<RingBufferElement> > ringBuffer;
		ObjectPool< DataFrame > bufferPool;
		std::thread receiverThread;
		std::thread serviceThread;
		long long activationTimeNs = 0;
		successor_type* successor = nullptr;
	public:

		SdrSourceActivity(const InitParams &params, SoapyDevice &sdr_device, SyncedLogger &logger) :
			logger{ logger },
			poolSize{ size_t(round(5 * params.SDR_SampleRate / params.SDR_FrameSize)) },
			params{ params },
			sdr_device{ sdr_device },
			bufferPool{ poolSize, DataFrame(params)}
		{
#ifdef VERBOSE
			std::cout << "SdrSourceActivity()" << endl;
#endif

			//setup sdr device
			if(params.SDR_AGC && !sdr_device.handle->hasGainMode(SOAPY_SDR_RX, 0)) throw invalid_argument("SdrSourceActivity: AGC is not supported on this device"); //hasGainMode() always returns false for UHD devices, even though they support AGC
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
			}
			else if(find(clockSources.cbegin(), clockSources.cend(), "internal"s) != clockSources.cend())
			{
				sdr_device.handle->setClockSource("internal"s);
			}
			//std::cout << "ClockSource: " << sdr_device.handle->getClockSource() << std::endl;
			//std::vector<std::string> sensor_list = sdr_device.handle->listSensors();
			//for(auto&& i : sensor_list)
			//{
			//	std::cout << "Sensor: " << i << std::endl;
			//}
			/*SoapySDR::ArgInfo ref_locked_args = sdr_device.handle->getSensorInfo("ref_locked");
			std::cout << "ref_locked: " << ref_locked_args.value << std::endl;*/

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
			if (isStreamActiveFlag) deactivate();
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

			if(params.SDR_StartOnTrigger)
			{
				sdr_device.handle->setHardwareTime(0);
				activateFlags = SOAPY_SDR_HAS_TIME;
				activationTimeNs = 24 * 3600 * 1e9;
				sdr_device.handle->setHardwareTime(activationTimeNs, "PPS"); //hack to activate stream on next PPS: set time to value far from now on next pps, activate stream at that time 
			}
			else if(sdr_device.handle->hasHardwareTime())
			{
				activationTimeNs = 500e6;
				sdr_device.handle->setHardwareTime(0);
			}

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

	private:
		static void receiverThreadFunc(SdrSourceActivity* h)
		{
			try
			{
				if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
				{
					cout << "SdrSourceActivity: Couldn't set thread priority" << endl;
				}

				size_t ctr = 0;
				const size_t streamMTUsize = h->streamMTUsize;
				const size_t activationTimeNs = h->activationTimeNs;
				

				//sdr loop
				while(h->isStreamActiveFlag)
				{
					if(!h->ringBuffer->write_available()) continue;

					auto& ringBufferElement = h->ringBuffer->back();
					void* buf_ptr = (void*)ringBufferElement.buf.data();
					ringBufferElement.flags = 0;
					ringBufferElement.nread = h->sdr_device.readStream(&buf_ptr, streamMTUsize, ringBufferElement.flags, ringBufferElement.timeNs, 600000000);
					ringBufferElement.timeNs -= activationTimeNs;
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
				shared_ptr<DataFrame> frame = h->bufferPool.acquire();
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
						size_t numDroppedSamples = (size_t) SoapySDR::timeNsToTicks(ringBufferElement.timeNs - lastTimeNS, h->actualSampleRate);

						stringstream strstream;
						if(ringBufferElement.flags & SOAPY_SDR_HAS_TIME)
						{
							strstream << "SdrSourceActivity: dropped " << numDroppedSamples << " samples at time " << ringBufferElement.timeNs / 1e9 << "s." << endl;
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

	class NIDAQmxSourceActivitiy : public sender< shared_ptr<vector<dsp_t>> >
	{
	private:
		const InitParams params;

		SyncedLogger &logger;
		successor_type* successor = nullptr;
		TaskHandle taskHandle = 0;
		bool isStreamActiveFlag = false;
		unique_ptr<ObjectPool< vector<dsp_t> >> bufferPool = nullptr;
		size_t tmpBufferSize;
		vector<double> tmpBuffer;

		thread noChannelsThread;

		const map<string, int32> TerminalConfigurations = { { "DAQmx_Val_Cfg_Default"s, DAQmx_Val_Cfg_Default }, { "DAQmx_Val_RSE"s, DAQmx_Val_RSE }, { "DAQmx_Val_NRSE"s, DAQmx_Val_NRSE }, { "DAQmx_Val_Diff"s, DAQmx_Val_Diff }, { "DAQmx_Val_PseudoDiff"s, DAQmx_Val_PseudoDiff } };
	public:
		NIDAQmxSourceActivitiy(const InitParams &params, SyncedLogger &logger) :
			params{ params },
			logger{ logger }
		{
#ifdef VERBOSE
			std::cout << "NIDAQmxSourceActivitiy()" << endl;
#endif
			if(params.DAQmx_ChannelCount > 0)
			{
				if(params.DAQmx_MaxVoltage <= 0) throw invalid_argument("NIDAQmxSourceActivitiy: DAQmx_MaxVoltage must be a positive value");
				if(TerminalConfigurations.find(params.DAQmx_AITerminalConfig) == TerminalConfigurations.cend()) throw invalid_argument("NIDAQmxSourceActivitiy: invalid DAQmx_AITerminalConfig");

				//config ADC task
				DAQmxErrorCheck(DAQmxCreateTask("ADC task", &taskHandle), "DAQmxCreateTask");

				DAQmxErrorCheck(DAQmxCreateAIVoltageChan(taskHandle, params.DAQmx_AIChannelList.c_str(), "", TerminalConfigurations.at(params.DAQmx_AITerminalConfig), -params.DAQmx_MaxVoltage, params.DAQmx_MaxVoltage, DAQmx_Val_Volts, NULL), "DAQmxCreateAIVoltageChan");

				uInt32 actualChannelCount;
				DAQmxErrorCheck(DAQmxGetTaskNumChans(taskHandle, &actualChannelCount), "DAQmxGetTaskNumChans");
				if(params.DAQmx_ChannelCount != actualChannelCount)
				{
					DAQmxClearTask(taskHandle);
					throw invalid_argument("NIDAQmxSourceActivitiy: params.DAQmx_ChannelCount doesn't match params.DAQmx_AIChannelList.");
				}

				DAQmxErrorCheck(DAQmxExportSignal(taskHandle, DAQmx_Val_StartTrigger, params.DAQmx_TriggerOutputTerminal.c_str()), "DAQmxExportSignal");

				//const int refClkSrc_bS = 128;
				//char refClkSrc[refClkSrc_bS];
				//double refClkRate;
				//DAQmxErrorCheck(DAQmxGetRefClkSrc(taskHandle, refClkSrc, refClkSrc_bS), "DAQmxGetRefClkSrc");
				//DAQmxErrorCheck(DAQmxGetRefClkRate(taskHandle, &refClkRate), "DAQmxGetRefClkRate");
				//std::cout << "DAQmxGetRefClkSrc: " << refClkSrc << ", DAQmxGetRefClkRate: " << refClkRate << std::endl;

				if(params.DAQmx_ExternalClock)
				{
					////DAQmxErrorCheck(DAQmxConnectTerms("/Dev1/10MHzRefClock", params.DAQmx_ClockInputTerminal.c_str(), DAQmx_Val_DoNotInvertPolarity), "DAQmxConnectTerms");
					////DAQmxErrorCheck(DAQmxExportSignal(taskHandle, DAQmx_Val_10MHzRefClock, params.DAQmx_ClockInputTerminal.c_str()), "DAQmxExportSignal");

					//DAQmxErrorCheck(DAQmxExportSignal(taskHandle, DAQmx_Val_10MHzRefClock, params.DAQmx_ClockInputTerminal.c_str()), "DAQmxExportSignal");


					if(params.DAQmx_ClockInputTerminal.empty()) throw invalid_argument("NIDAQmxSourceActivitiy: DAQmx_ClockInputTerminal is empty");
					DAQmxErrorCheck(DAQmxSetRefClkSrc(taskHandle, params.DAQmx_ClockInputTerminal.c_str()), "DAQmxSetRefClkSrc");
					DAQmxErrorCheck(DAQmxSetRefClkRate(taskHandle, 10e6), "DAQmxSetRefClkRate");
				}

				DAQmxErrorCheck(DAQmxCfgSampClkTiming(taskHandle, "", params.DAQmx_SampleRate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, params.DAQmx_FrameSize), "DAQmxCfgSampClkTiming");

				double SampClkRate;
				DAQmxErrorCheck(DAQmxGetSampClkRate(taskHandle, &SampClkRate));
				if(SampClkRate - params.DAQmx_SampleRate != 0.0)
				{
					std::ostringstream msg;
					msg << "NIDAQmxSourceActivitiy: DAQmx_SampleRate=" << params.DAQmx_SampleRate << " is not supported by the DAQmx device. The nearest supported rate is " << SampClkRate << endl;
					throw invalid_argument(msg.str());
				}

				DAQmxErrorCheck(DAQmxCfgInputBuffer(taskHandle, params.DAQmx_FrameSize*20), "DAQmxCfgInputBuffer");

				DAQmxErrorCheck(DAQmxSetAILowpassEnable(taskHandle, "", params.DAQmx_AILowpassEnable), "DAQmxSetAILowpassEnable");
				if(params.DAQmx_AILowpassEnable) DAQmxErrorCheck(DAQmxSetAILowpassCutoffFreq(taskHandle, "", params.DAQmx_AILowpassCutoffFreq), "DAQmxSetAILowpassCutoffFreq");

				DAQmxErrorCheck(DAQmxRegisterEveryNSamplesEvent(taskHandle, DAQmx_Val_Acquired_Into_Buffer, params.DAQmx_FrameSize, 0, EveryNCallback, (void *)this), "DAQmxRegisterEveryNSamplesEvent");


				DAQmxErrorCheck(DAQmxTaskControl(taskHandle, DAQmx_Val_Task_Verify), "DAQmxTaskControl");
				DAQmxErrorCheck(DAQmxTaskControl(taskHandle, DAQmx_Val_Task_Commit), "DAQmxTaskControl");
			}

			tmpBufferSize = params.DAQmx_FrameSize*params.DAQmx_ChannelCount;
			bufferPool = make_unique<ObjectPool<vector<dsp_t>>>(100, vector<dsp_t>(tmpBufferSize));
			tmpBuffer.resize(tmpBufferSize);

		}

		~NIDAQmxSourceActivitiy()
		{
			if(isStreamActiveFlag) deactivate();
			if(params.DAQmx_ChannelCount > 0)
			{
				DAQmxClearTask(taskHandle);
			}
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
			if(isStreamActiveFlag || successor != &r) return false;
			successor = nullptr;
			return true;
		}

		void activate()
		{
			if(isStreamActiveFlag) throw logic_error("NIDAQmxSourceActivitiy: Can't activate stream, because it is already active.");
			if(!successor) throw logic_error("NIDAQmxSourceActivitiy: Can't activate stream, because no successor is registered.");

			isStreamActiveFlag = true;
			if(params.DAQmx_ChannelCount > 0)
			{
				DAQmxErrorCheck(DAQmxStartTask(taskHandle), "DAQmxStartTask", false);
			}
			else
			{
				noChannelsThread = thread([&]() {
					while(isStreamActiveFlag)
					{
						NIDAQmxSourceActivitiy::output_type buffer = bufferPool->acquire();
						while(!buffer)
						{
							if(!isStreamActiveFlag) return;
							this_thread::yield();
							buffer = bufferPool->acquire();
						}
						successor->try_put(buffer);
						this_thread::yield();
					}
				});
			}

		}

		void deactivate()
		{
			if(!isStreamActiveFlag) throw logic_error("SdrSourceActivity: Can't deactivate stream. Stream is not active.");
			isStreamActiveFlag = false;
			if(params.DAQmx_ChannelCount > 0)
			{
				DAQmxErrorCheck(DAQmxStopTask(taskHandle), "DAQmxStopTask", false);
			}
			else
			{
				noChannelsThread.join();
			}
		}

		bool isStreamActive() { return isStreamActiveFlag; }

	private:
		void DAQmxErrorCheck(int32 status, const string& fcnName = ""s, bool clearTask = true)
		{
			if(status < 0) 
			{
				char errBuff[2048];
				DAQmxGetExtendedErrorInfo(errBuff, 2048);
				stringstream strstream;
				strstream << "NIDAQmxSourceActivitiy: " << fcnName << ": " << errBuff << endl;

				if(clearTask && taskHandle != 0)
				{
					DAQmxStopTask(taskHandle);
					DAQmxClearTask(taskHandle);
				}

				throw runtime_error(strstream.str());
			}
			else if(status > 0) cout << "NIDAQmxSourceActivitiy: " << fcnName << " warning. " << endl;
		}

		static int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
		{
			NIDAQmxSourceActivitiy *h = (NIDAQmxSourceActivitiy*)callbackData;

			NIDAQmxSourceActivitiy::output_type buffer = nullptr;
			while(!buffer) buffer = h->bufferPool->acquire();
			if(buffer->size() != h->tmpBufferSize) buffer->resize(h->tmpBufferSize);

			int32 read = 0;
			int status = DAQmxReadAnalogF64(taskHandle, nSamples, 10.0, DAQmx_Val_GroupByScanNumber, h->tmpBuffer.data(), h->tmpBuffer.size(), &read, NULL);

			if(status != 0)
			{
				char errBuff[2048];
				DAQmxGetExtendedErrorInfo(errBuff, 2048);
				stringstream strstream;
				strstream << "NIDAQmxSourceActivitiy: " << errBuff << "\nread=" << read << endl;
				h->logger.write(strstream.str());
				return 0;
			}

			if(read != h->params.DAQmx_FrameSize)
			{
				stringstream strstream;
				strstream << "NIDAQmxSourceActivitiy: read only " << read << " samples, instead of " << nSamples << endl;
				h->logger.write(strstream.str());
			}

			for(size_t i = 0; i < h->tmpBuffer.size(); ++i)
			{
				(*buffer)[i] = dsp_t(h->tmpBuffer[i] / h->params.DAQmx_MaxVoltage);
			}

			h->successor->try_put(buffer);
			return 0;
		}
	};

	class AudioOutputActivitiy
	{
	private:
		double outputSampleRate;
		size_t channelCount;
		int deviceIndex;
		const PaSampleFormat sampleFormat = paFloat32;
		PaStream *stream = nullptr;
		tbb::concurrent_queue<shared_ptr<vector<float>>> queue;
		shared_ptr<vector<float>> currentBuffer = nullptr;
		size_t currentBufferPos = 0;
		size_t currentBufferSize = 0;

		SRC_STATE *src_state;
		SRC_DATA src_data;
	public:
		AudioOutputActivitiy(size_t channelCount = 1, int deviceIndex = -1) :
			channelCount{ channelCount },
			deviceIndex{ deviceIndex }
		{
#ifdef VERBOSE
			std::cout << "AudioOutputActivitiy()" << endl;
#endif
			PaError err;

			err = Pa_Initialize();
			if(err != paNoError) throw runtime_error("AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));

			if(deviceIndex < 0)
			{
				deviceIndex = Pa_GetDefaultOutputDevice();
				if(deviceIndex == paNoDevice) throw runtime_error("AudioOutputActivitiy: No default audio output device available");
			}
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if(deviceInfo == NULL) throw runtime_error("AudioOutputActivitiy: deviceIndex out of range");

			outputSampleRate = deviceInfo->defaultSampleRate; //set output SR to default SR of device

			PaStreamParameters outputParameters;
			outputParameters.channelCount = channelCount;
			outputParameters.device = deviceIndex;
			outputParameters.hostApiSpecificStreamInfo = NULL;
			outputParameters.sampleFormat = sampleFormat;
			outputParameters.suggestedLatency = deviceInfo->defaultHighOutputLatency;

			err = Pa_IsFormatSupported(NULL, &outputParameters, outputSampleRate);
			if(err != paFormatIsSupported) throw runtime_error("AudioOutputActivitiy: PortAudio stream format error: " + string(Pa_GetErrorText(err)));

			err = Pa_OpenStream(&stream, NULL, &outputParameters, outputSampleRate, paFramesPerBufferUnspecified, paNoFlag, audioOutputCallback, (void *)this);
			if(err != paNoError) throw runtime_error("AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));

			err = Pa_StartStream(stream);
			if(err != paNoError) throw runtime_error("AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));

			//init libsamplerate resampler
			int error;
			src_state = src_new(SRC_SINC_MEDIUM_QUALITY, channelCount, &error);
			if(src_state == NULL) throw runtime_error("AudioOutputActivitiy: libsamplerate error: Error in libsamplerate object construction");
			src_data.end_of_input = 0;
		}

		~AudioOutputActivitiy()
		{
			PaError err;
			err = Pa_AbortStream(stream);
			if (err != paNoError) cout << "AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
			err = Pa_CloseStream(stream);
			if (err != paNoError) cout << "AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
			err = Pa_Terminate();
			if(err != paNoError) cout << "AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;

			src_delete(src_state);
		}

		double getOutputSampleRate() const { return outputSampleRate; }

		bool isFormatSupported(double sampleRate, size_t channelCount = 1, int deviceIndex = -1) const
		{
			if(deviceIndex < 0)
			{
				deviceIndex = Pa_GetDefaultOutputDevice();
				if(deviceIndex == paNoDevice) throw runtime_error("AudioOutputActivitiy: No default audio output device available");
			}
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if(deviceInfo == NULL) throw runtime_error("AudioOutputActivitiy: deviceIndex out of range");

			PaStreamParameters outputParameters;
			outputParameters.channelCount = channelCount;
			outputParameters.device = deviceIndex;
			outputParameters.hostApiSpecificStreamInfo = NULL;
			outputParameters.sampleFormat = sampleFormat;
			outputParameters.suggestedLatency = deviceInfo->defaultHighOutputLatency;

			return Pa_IsFormatSupported(NULL, &outputParameters, sampleRate) == paFormatIsSupported;
		}

		bool try_put(const shared_ptr<vector<float>> &data) 
		{
			queue.push(data); 
			return true;
		}

		bool try_put(const shared_ptr<vector<double>> &data)
		{
			auto data2 = make_shared<vector<float>>(data->cbegin(), data->cend());
			queue.push(data2);
			return true;
		}

		bool try_put(const vector<float> &data, double sampleRate)
		{
			src_data.src_ratio = outputSampleRate / sampleRate;
			src_data.input_frames = data.size() / channelCount;
			src_data.data_in = data.data();

			src_data.output_frames = std::ceil(src_data.input_frames*src_data.src_ratio);
			auto dataOut = make_shared<vector<float>>(src_data.output_frames*channelCount);
			src_data.data_out = dataOut->data();

			int error = src_process(src_state, &src_data);
			if(error) throw runtime_error("AudioOutputActivitiy: libsamplerate error: Error in src_process()");
			dataOut->resize(src_data.output_frames_gen*channelCount);
			if(src_data.input_frames_used != src_data.input_frames) cout << "AudioOutputActivitiy: stream discontinuity" << endl;

			queue.push(dataOut);
			return true;
		}

		

	private:
		static int audioOutputCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
		{
			AudioOutputActivitiy *h = (AudioOutputActivitiy*)userData;

			size_t outputBufferPos = 0;
			size_t outputBufferSize = framesPerBuffer*h->channelCount;

			while(outputBufferPos < outputBufferSize)
			{
				if(!h->currentBuffer)
				{
					if(!h->queue.try_pop(h->currentBuffer))
					{
						fill_n((float*)outputBuffer + outputBufferPos, outputBufferSize - outputBufferPos, float(0));
						return 0;
					}

					h->currentBufferPos = 0;
					h->currentBufferSize = h->currentBuffer->size();
				}

				if(h->currentBufferSize - h->currentBufferPos > outputBufferSize - outputBufferPos)
				{
					size_t n = outputBufferSize - outputBufferPos;
					copy_n(h->currentBuffer->begin() + h->currentBufferPos, n, (float*)outputBuffer + outputBufferPos);
					h->currentBufferPos += n;
					return 0;
				}
				else
				{
					size_t n = h->currentBufferSize - h->currentBufferPos;
					copy_n(h->currentBuffer->begin() + h->currentBufferPos, n, (float*)outputBuffer + outputBufferPos);
					outputBufferPos += n;
					h->currentBuffer = nullptr;
				}
			}

			return 0;
		}

	};

	
	

	namespace NodeFunctors
	{

		struct file_writer
		{
		private:
			const InitParams params;

			SndfileHandle SdrChannelsH;
			SndfileHandle SdrSignalStrengthH;
			SndfileHandle SdrCarrierFreqH;
			SndfileHandle SdrReceiveFreqH;
			SndfileHandle DAQmxChannelsH;

			bool firstFrame = true;
			size_t filterDelay;
		public:
			file_writer(const InitParams &params) :
				params{ params },
				SdrChannelsH{ params.SdrChannelsFilename, SFM_WRITE, SF_FORMAT_W64 | params.DataFile_SamplePrecision_Map.at(params.DataFile_SamplePrecision), params.SDR_ChannelCount, params.SDR_ChannelSampleRate },
				SdrSignalStrengthH{ params.SdrSignalStrengthFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate },
				SdrCarrierFreqH{ params.SdrCarrierFreqFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate },
				SdrReceiveFreqH{ params.SdrReceiveFreqFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate },
				DAQmxChannelsH{ params.DAQmxChannelsFilename, SFM_WRITE, SF_FORMAT_W64 | params.DataFile_SamplePrecision_Map.at(params.DataFile_SamplePrecision), params.DAQmx_ChannelCount, params.DAQmx_SampleRate },
				filterDelay{ params.Decimator1_FilterOrder/(2* params.Decimator1_Factor*params.Decimator2_Factor)+ params.Decimator2_FilterOrder / (2 * params.Decimator2_Factor)
			}
			{}

			using input_type = tuple<shared_ptr<DataFrame>, shared_ptr<vector<dsp_t>>>;

			continue_msg operator()(const input_type &input)
			{
				auto& frame = get<0>(input);
				auto& daqmxFrame = get<1>(input);

				if (params.SDR_ChannelCount != 0)
				{
					//interleave sdr channels
					auto n = frame->demodulated_signals[0].size();
					vector<dsp_t> SdrChannelsBuffer;
					SdrChannelsBuffer.reserve(params.SDR_ChannelCount*n);
					for (size_t i = 0; i < n; ++i)
					{
						for (size_t k = 0; k < params.SDR_ChannelCount; ++k)
						{
							SdrChannelsBuffer.push_back(frame->demodulated_signals[k][i]);
						}
					}

					if(firstFrame)
					{
						//compensate for group delay of decimation filters
						SdrChannelsH.write(SdrChannelsBuffer.data() + filterDelay*params.SDR_ChannelCount, SdrChannelsBuffer.size()- filterDelay*params.SDR_ChannelCount);
						firstFrame = false;
					}
					else
					{
						SdrChannelsH.write(SdrChannelsBuffer.data(), SdrChannelsBuffer.size());
					}

					//write the rest
					SdrSignalStrengthH.write(frame->signal_strengths.data(), frame->signal_strengths.size());
					SdrCarrierFreqH.write(frame->carrier_frequencies.data(), frame->carrier_frequencies.size());
					SdrReceiveFreqH.write(frame->receive_frequencies.data(), frame->receive_frequencies.size());	
				}
				DAQmxChannelsH.write(daqmxFrame->data(), daqmxFrame->size());

				return continue_msg();
			}
		};

		
	}

}