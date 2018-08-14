#pragma once
#include <memory>
#include <tbb/flow_graph.h>
#include "Types.h"
#include "SdrDataFrame.h"
#include "MonitorDataFrame.h"
#include "InitParams.h"
#include "SdrSourceActivity.h"
#include "NIDAQmxSourceActivitiy.h"
#include "AudioOutputActivity.h"
#include "FileWriterNodeBody.h"
#include "ChannelExtractorNode.h"
#include "ControlNode.h"
#include "SyncedLogger.h"

namespace SdrBirdrec
{
	using namespace tbb::flow;
	using namespace std;

	class Topology
	{
	public:
		Topology(const InitParams &params) :
			params{ params },
			isStreamActiveFlag{ false },
			logger{ params.LogFilename }
		{
#ifdef VERBOSE
			std::cout << "Topology()" << endl;
#endif

			make_edges();
		}

		~Topology()
		{
			try
			{
				if(isStreamActiveFlag)
				{
					sdrSourceActivity.deactivate();
					nIDAQmxSourceActivitiy.deactivate();
				}
				g.wait_for_all();	
			}
			catch (...)
			{
				cout << "Topology destructor: catched exception." << endl;
			}
		}

		void activate() 
		{ 
			if(isStreamActiveFlag) throw logic_error("Topology: Can't activate stream, because it's already active.");

			sdrSourceActivity.activate();
			nIDAQmxSourceActivitiy.activate();
			isStreamActiveFlag = true;
		}

		shared_ptr<MonitorDataFrame> getMonitorDataFrame(void)
		{
			shared_ptr<MonitorDataFrame> frame{ nullptr };
			outOverwriteNode.try_get(frame);
			while (frame == lastOutputFrame)
			{
				this_thread::yield();
				outOverwriteNode.try_get(frame);
			}

			lastOutputFrame = frame;
			return frame;
		}

		bool isRefPLLlocked(void) { return sdrSourceActivity.isRefPLLlocked(); }

		void setMonitorOptions(const Kwargs &args) { controlNode.monitorSettingsInputPort.try_put(args); }
		bool isStreamActive() { return isStreamActiveFlag; }

	private:
		atomic<bool> isStreamActiveFlag;

		//SoapyDevice sdr_device;
		const InitParams params;
		SyncedLogger logger;
		shared_ptr<MonitorDataFrame> lastOutputFrame = nullptr;
		graph g;

		NIDAQmxSourceActivitiy nIDAQmxSourceActivitiy{ params, logger };
		SdrSourceActivity sdrSourceActivity{ params, logger };	
		AudioOutputActivitiy audioOutputActivitiy{ 1, params.AudioOutput_DeviceIndex };

		ChannelExtractorNode channelExtractorNode{ g, params };
		ControlNode controlNode{ g, params, audioOutputActivitiy };
		function_node< typename FileWriterNodeBody::input_type > fileWriterNode{ g, serial,  FileWriterNodeBody(params) };
		overwrite_node< shared_ptr<MonitorDataFrame> > outOverwriteNode{ g };

		void make_edges()
		{
			make_edge(sdrSourceActivity, channelExtractorNode);
			make_edge(channelExtractorNode, controlNode.sdrFrameInputPort);
			make_edge(nIDAQmxSourceActivitiy, controlNode.daqmxFrameInputPort);
			make_edge(controlNode.fileWriterOutputPort, fileWriterNode);
			make_edge(controlNode.outputFrameOutputPort, outOverwriteNode);
		}
	};

}

