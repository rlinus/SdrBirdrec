#pragma once
#include <memory>
#include <tbb/flow_graph.h>
#include "Types.h"
#include "DataFrame.h"
#include "InitParams.h"
#include "SoapyDevice.h"
#include "SdrSourceActivity.h"
#include "NodeFunctors.h"
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
		Topology(const InitParams &params, Kwargs args) :
			sdr_device{ args },
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
			if(isStreamActiveFlag) throw logic_error("Topology: Can't activate stream, because it is already active.");

			sdrSourceActivity.activate();
			nIDAQmxSourceActivitiy.activate();
			isStreamActiveFlag = true;
		}

		shared_ptr<OutputFrame> getData(void)
		{
			shared_ptr<OutputFrame> frame{ nullptr };
			out_overwrite_node.try_get(frame);
			while (frame == lastOutputFrame)
			{
				this_thread::yield();
				out_overwrite_node.try_get(frame);
			}

			lastOutputFrame = frame;
			return frame;
		}

		bool isRefPLLlocked(void)
		{
			SoapySDR::ArgInfo ref_locked_args = sdr_device.handle->getSensorInfo("ref_locked");
			return ref_locked_args.value.compare("true") == 0;
		}

		void setMonitorOptions(const Kwargs &args) { controlNode.monitor_settings_input_port.try_put(args); }
		bool isStreamActive() { return isStreamActiveFlag; }

	private:
		atomic<bool> isStreamActiveFlag;

		SoapyDevice sdr_device;
		const InitParams params;
		SyncedLogger logger;
		shared_ptr<OutputFrame> lastOutputFrame = nullptr;
		graph g;

		NIDAQmxSourceActivitiy nIDAQmxSourceActivitiy{ params, logger };
		SdrSourceActivity sdrSourceActivity{ params, sdr_device, logger };	
		AudioOutputActivitiy audioOutputActivitiy{ 1, params.AudioOutput_DeviceIndex };

		ChannelExtractorNode channelExtractorNode{ g, params };
		ControlNode controlNode{ g, params, audioOutputActivitiy };
		function_node< typename NodeFunctors::file_writer::input_type > file_writer_node{ g, serial,  NodeFunctors::file_writer(params) };
		overwrite_node< shared_ptr<OutputFrame> > out_overwrite_node{ g };

		void make_edges()
		{
			make_edge(sdrSourceActivity, channelExtractorNode);
			make_edge(channelExtractorNode, controlNode.sdr_frame_input_port);
			make_edge(nIDAQmxSourceActivitiy, controlNode.daqmx_frame_input_port);
			make_edge(controlNode.file_writer_output_port, file_writer_node);
			make_edge(controlNode.output_frame_output_port, out_overwrite_node);
		}
	};

}

