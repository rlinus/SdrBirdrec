#include "Topology.h"
#include "Version.h"


namespace SdrBirdrec
{
	using namespace tbb::flow;
	using namespace std;

	Topology::Topology(const InitParams &params) :
		params{ params },
		logger{ params.LogFilename }
	{
		#ifdef VERBOSE
		std::cout << "Topology()" << endl;
		#endif

		logger.write_line("SdrBirdrec Version: "s + PROJECT_VERSION_STRING);

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
				auto now = std::chrono::system_clock::now();
				logger.write_line("Stoptime: " + logger.time2string(now));
				logger.logbuffer2cout();
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

		auto now = std::chrono::system_clock::now();

		
		sdrSourceActivity.activate();
		nIDAQmxSourceActivitiy.activate();
		isStreamActiveFlag = true;

		logger.write_line("Starttime: " + logger.time2string(now));
	}

	shared_ptr<MonitorDataFrame> Topology::getMonitorDataFrame(void)
	{
		logger.logbuffer2cout();
		shared_ptr<MonitorDataFrame> frame{ nullptr };
		outOverwriteNode.try_get(frame);
		while(frame == lastOutputFrame)
		{
			if(streamErrorFlag) throw runtime_error("Topology: stream error");
			this_thread::yield();
			outOverwriteNode.try_get(frame);
		}

		lastOutputFrame = frame;
		return frame;
	}
}


