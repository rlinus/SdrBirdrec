#include "Topology.h"


namespace SdrBirdrec
{
	using namespace tbb::flow;
	using namespace std;

	Topology::Topology(const InitParams &params) :
		params{ params },
		isStreamActiveFlag{ false },
		logger{ params.LogFilename }
	{
		#ifdef VERBOSE
		std::cout << "Topology()" << endl;
		#endif

		//make edges between graph nodes
		make_edge(sdrSourceActivity, channelExtractorNode);
		make_edge(channelExtractorNode, controlNode.sdrFrameInputPort);
		make_edge(nIDAQmxSourceActivitiy, controlNode.daqmxFrameInputPort);
		make_edge(controlNode.fileWriterOutputPort, fileWriterNode);
		make_edge(controlNode.outputFrameOutputPort, outOverwriteNode);
	}

	Topology::~Topology()
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
		catch(...)
		{
			cout << "Topology destructor: catched exception." << endl;
		}
	}

	void Topology::activate()
	{
		if(isStreamActiveFlag) throw logic_error("Topology: Can't activate stream, because it's already active.");

		sdrSourceActivity.activate();
		nIDAQmxSourceActivitiy.activate();
		isStreamActiveFlag = true;
	}

	shared_ptr<MonitorDataFrame> Topology::getMonitorDataFrame(void)
	{
		shared_ptr<MonitorDataFrame> frame{ nullptr };
		outOverwriteNode.try_get(frame);
		while(frame == lastOutputFrame)
		{
			this_thread::yield();
			outOverwriteNode.try_get(frame);
		}

		lastOutputFrame = frame;
		return frame;
	}
}


