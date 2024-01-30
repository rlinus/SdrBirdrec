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
		TaskHandle aiTaskHandle = 0;
		TaskHandle ctrTaskHandle = 0;
		bool isStreamActiveFlag = false;
		unique_ptr<ObjectPool< vector<dsp_t> >> bufferPool = nullptr;
		size_t tmpBufferSize;
		vector<double> tmpBuffer;

		thread noChannelsThread;
		bool daqmxBackPressureReportedFlag = false;
		size_t output_frame_ctr = 0;

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
				DAQmxErrorCheck(DAQmxCreateTask("ADCtask", &aiTaskHandle), "DAQmxCreateTask");

				DAQmxErrorCheck(DAQmxCreateAIVoltageChan(aiTaskHandle, params.DAQmx_AIChannelList.c_str(), "", TerminalConfigurations.at(params.DAQmx_AITerminalConfig), -params.DAQmx_MaxVoltage, params.DAQmx_MaxVoltage, DAQmx_Val_Volts, NULL), "DAQmxCreateAIVoltageChan");

				uInt32 actualChannelCount;
				DAQmxErrorCheck(DAQmxGetTaskNumChans(aiTaskHandle, &actualChannelCount), "DAQmxGetTaskNumChans");
				if(params.DAQmx_ChannelCount != actualChannelCount)
				{
					DAQmxClearTask(aiTaskHandle);
					throw invalid_argument("NIDAQmxSourceActivitiy: params.DAQmx_ChannelCount doesn't match params.DAQmx_AIChannelList.");
				}

				DAQmxErrorCheck(DAQmxExportSignal(aiTaskHandle, DAQmx_Val_StartTrigger, params.DAQmx_TriggerOutputTerminal.c_str()), "DAQmxExportSignal");

				if(params.DAQmx_ExternalClock)
				{
					if(params.DAQmx_ClockInputTerminal.empty()) throw invalid_argument("NIDAQmxSourceActivitiy: DAQmx_ClockInputTerminal is empty");
					DAQmxErrorCheck(DAQmxSetRefClkSrc(aiTaskHandle, params.DAQmx_ClockInputTerminal.c_str()), "DAQmxSetRefClkSrc");
					DAQmxErrorCheck(DAQmxSetRefClkRate(aiTaskHandle, 10e6), "DAQmxSetRefClkRate");
				}

				DAQmxErrorCheck(DAQmxCfgSampClkTiming(aiTaskHandle, "", params.DAQmx_SampleRate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, params.DAQmx_FrameSize), "DAQmxCfgSampClkTiming");

				double SampClkRate;
				DAQmxErrorCheck(DAQmxGetSampClkRate(aiTaskHandle, &SampClkRate));
				if(SampClkRate - params.DAQmx_SampleRate != 0.0)
				{
					std::ostringstream msg;
					msg << "NIDAQmxSourceActivitiy: DAQmx_SampleRate=" << params.DAQmx_SampleRate << " is not supported by the DAQmx device. The nearest supported rate is " << SampClkRate << endl;
					DAQmxClearTask(aiTaskHandle);
					throw invalid_argument(msg.str());
				}

				DAQmxErrorCheck(DAQmxCfgInputBuffer(aiTaskHandle, params.DAQmx_FrameSize * 20), "DAQmxCfgInputBuffer");

				DAQmxErrorCheck(DAQmxSetAILowpassEnable(aiTaskHandle, "", params.DAQmx_AILowpassEnable), "DAQmxSetAILowpassEnable");
				if(params.DAQmx_AILowpassEnable) DAQmxErrorCheck(DAQmxSetAILowpassCutoffFreq(aiTaskHandle, "", params.DAQmx_AILowpassCutoffFreq), "DAQmxSetAILowpassCutoffFreq");

				DAQmxErrorCheck(DAQmxRegisterEveryNSamplesEvent(aiTaskHandle, DAQmx_Val_Acquired_Into_Buffer, params.DAQmx_FrameSize, 0, EveryNCallback, (void *)this), "DAQmxRegisterEveryNSamplesEvent");
				DAQmxErrorCheck(DAQmxRegisterDoneEvent(aiTaskHandle, 0, DoneCallback, (void*)this), "DAQmxRegisterDoneEvent");

				DAQmxErrorCheck(DAQmxTaskControl(aiTaskHandle, DAQmx_Val_Task_Verify), "DAQmxTaskControl");
				DAQmxErrorCheck(DAQmxTaskControl(aiTaskHandle, DAQmx_Val_Task_Commit), "DAQmxTaskControl");

				//camera trigger task
				if (!params.DAQmx_ClockOutputSignalCounter.empty())
				{
					char devName[100];
					DAQmxErrorCheck(DAQmxGetNthTaskDevice(aiTaskHandle, 1, devName, 100), "DAQmxGetNthTaskDevice");

					std::string ctrTriggerName = "/" + std::string(devName) + "/ai/StartTrigger"; //correct format: "/Dev1/ai/StartTrigger"
					//std::cout << "ctrTriggerName: " << ctrTriggerName << endl;

					double initialDelay = 0;
					double dutyCycle = 0.2;
					DAQmxErrorCheck(DAQmxCreateTask("CounterTask", &ctrTaskHandle), "DAQmxCreateTask");
					DAQmxErrorCheck(DAQmxCreateCOPulseChanFreq(ctrTaskHandle, params.DAQmx_ClockOutputSignalCounter.c_str(), "cameraTriggerChannel", DAQmx_Val_Hz, DAQmx_Val_Low, initialDelay, params.DAQmx_ClockOutputSignalFreq, dutyCycle), "DAQmxCreateCOPulseChanFreq");
					DAQmxErrorCheck(DAQmxCfgDigEdgeStartTrig(ctrTaskHandle, ctrTriggerName.c_str(), DAQmx_Val_Rising), "DAQmxCfgDigEdgeStartTrig");
					DAQmxErrorCheck(DAQmxCfgImplicitTiming(ctrTaskHandle, DAQmx_Val_ContSamps, 1000), "DAQmxCfgImplicitTiming");				
					//DAQmxErrorCheck(DAQmxSetExportedCtrOutEventOutputTerm(ctrTaskHandle, "/Dev1/PFI5"), "DAQmxSetExportedCtrOutEventOutputTerm");

					DAQmxErrorCheck(DAQmxTaskControl(ctrTaskHandle, DAQmx_Val_Task_Verify), "DAQmxTaskControl");
					DAQmxErrorCheck(DAQmxTaskControl(ctrTaskHandle, DAQmx_Val_Task_Commit), "DAQmxTaskControl");
					
				}

				
				
			}

			tmpBufferSize = params.DAQmx_FrameSize*params.DAQmx_ChannelCount;
			bufferPool = make_unique<ObjectPool<vector<dsp_t>>>(params.MonitorRate * 10, vector<dsp_t>(tmpBufferSize));
			tmpBuffer.resize(tmpBufferSize);

		}

		~NIDAQmxSourceActivitiy()
		{
			if(isStreamActiveFlag) deactivate();
			if(params.DAQmx_ChannelCount > 0)
			{
				DAQmxClearTask(aiTaskHandle);
				DAQmxClearTask(ctrTaskHandle);
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
				if(ctrTaskHandle) DAQmxErrorCheck(DAQmxStartTask(ctrTaskHandle), "DAQmxStartTask", false);
				DAQmxErrorCheck(DAQmxStartTask(aiTaskHandle), "DAQmxStartTask", false);
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
				DAQmxErrorCheck(DAQmxStopTask(aiTaskHandle), "DAQmxStopTask", false);
				if (ctrTaskHandle) DAQmxErrorCheck(DAQmxStopTask(ctrTaskHandle), "DAQmxStopTask", false);
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

				if(clearTask)
				{
					if (aiTaskHandle != 0)
					{
						DAQmxStopTask(aiTaskHandle);
						DAQmxClearTask(aiTaskHandle);
					}
					if (ctrTaskHandle != 0)
					{
						DAQmxStopTask(ctrTaskHandle);
						DAQmxClearTask(ctrTaskHandle);
					}
				}

				throw runtime_error(strstream.str());
			}
			else if(status > 0) cout << "NIDAQmxSourceActivitiy: " << fcnName << " warning. " << endl;
		}

		static int32 CVICALLBACK EveryNCallback(TaskHandle aiTaskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
		{
			NIDAQmxSourceActivitiy *h = (NIDAQmxSourceActivitiy*)callbackData;

			NIDAQmxSourceActivitiy::output_type buffer = h->bufferPool->acquire();
			while (!buffer)
			{
				if (!h->daqmxBackPressureReportedFlag)
				{
					stringstream strstream2;
					strstream2 << "NIDAQmxSourceActivity: back pressure at frame " << h->output_frame_ctr << endl;
					h->logger.write(strstream2.str());
					h->daqmxBackPressureReportedFlag = true;
				}
				this_thread::yield();
				buffer = h->bufferPool->acquire();
			}
			if(buffer->size() != h->tmpBufferSize) buffer->resize(h->tmpBufferSize);

			int32 read = 0;
			int status = DAQmxReadAnalogF64(aiTaskHandle, nSamples, 10.0, DAQmx_Val_GroupByScanNumber, h->tmpBuffer.data(), h->tmpBuffer.size(), &read, NULL);

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
			++(h->output_frame_ctr);
			return 0;
		}

		static int32 CVICALLBACK DoneCallback(TaskHandle aiTaskHandle, int32 status, void* callbackData)
		{
			NIDAQmxSourceActivitiy* h = (NIDAQmxSourceActivitiy*)callbackData;

			if (status != 0)
			{
				char errBuff[2048];
				DAQmxGetExtendedErrorInfo(errBuff, 2048);
				stringstream strstream;
				strstream << "NIDAQmxSourceActivitiy: DoneCallback:" << errBuff << endl;
				h->logger.write(strstream.str());
				return 0;
			}

			return 0;
		}
	};
}