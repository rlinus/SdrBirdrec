#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <thread>
#include <memory>
#include <exception>
#include <stdexcept>

#include <tbb/tbb.h>

#include <NIDAQmx.h>


#include "InitParams.h"
#include "ObjectPool.h"
#include "SyncedLogger.h"

#include "Types.h"

/*!
* This is the NIDAQmx source streaming node.
*/
namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;

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

				if(params.DAQmx_ExternalClock)
				{
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
					DAQmxClearTask(taskHandle);
					throw invalid_argument(msg.str());
				}

				DAQmxErrorCheck(DAQmxCfgInputBuffer(taskHandle, params.DAQmx_FrameSize * 20), "DAQmxCfgInputBuffer");

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
				//if no DAQmx channels are recorded we just stream empty buffers instead.
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
				strstream << "NIDAQmxSourceActivitiy: read only " << read << " samples instead of " << nSamples << endl;
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
}