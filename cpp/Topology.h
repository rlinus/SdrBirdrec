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
	class Topology
	{
	public:
		Topology(const InitParams &params);

		~Topology();

		void activate();

		shared_ptr<MonitorDataFrame> getMonitorDataFrame(void);

		bool isRefPLLlocked(void) { return sdrSourceActivity.isRefPLLlocked(); }

		void setMonitorOptions(const Kwargs &args) { controlNode.monitorSettingsInputPort.try_put(args); }
		bool isStreamActive() const { return isStreamActiveFlag; }

	private:
		std::atomic<bool> isStreamActiveFlag;

		const InitParams params;
		SyncedLogger logger;
		shared_ptr<MonitorDataFrame> lastOutputFrame = nullptr;
		tbb::flow::graph g;

		NIDAQmxSourceActivitiy nIDAQmxSourceActivitiy{ params, logger };
		SdrSourceActivity sdrSourceActivity{ params, logger };	
		AudioOutputActivitiy audioOutputActivitiy{ 1, params.AudioOutput_DeviceIndex };

		ChannelExtractorNode channelExtractorNode{ g, params };
		ControlNode controlNode{ g, params, audioOutputActivitiy };
		tbb::flow::function_node< typename FileWriterNodeBody::input_type > fileWriterNode{ g, serial,  FileWriterNodeBody(params) };
		tbb::flow::overwrite_node< std::shared_ptr<MonitorDataFrame> > outOverwriteNode{ g };
	};

}

