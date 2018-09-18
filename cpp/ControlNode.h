#pragma once
#include <string>
#include <memory>

#include <tbb/flow_graph.h>

#include "Types.h"
#include "SdrDataFrame.h"
#include "MonitorDataFrame.h"
#include "InitParams.h"
#include "AudioOutputActivity.h"
#include "MonitorSettings.h"

namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;

	/**
	* This composite processing node joins the SDR and NIDAQmx dataframe with a join node and forwards the joint data to the FileWriterNode. Additionally it processes MonitorSetting inputs and outputs MonitorDataFrames and calls AudioOutputActivity.try_put() to output the monitor audio stream.
	*/
	class ControlNode
	{
	private:
		using SdrFrameType = shared_ptr<SdrDataFrame>;
		using NIDAQmxFrameType = shared_ptr<vector<dsp_t>>;
		using MonitorSettingsType = Kwargs;

		using mf_input_type = tuple< SdrFrameType, NIDAQmxFrameType >;
		using mf_output_type = tuple< mf_input_type, shared_ptr<MonitorDataFrame>>;
		using multifunction_node_type = multifunction_node< mf_input_type, mf_output_type >;

		join_node<  mf_input_type, queueing > j;
		multifunction_node_type mf;
		queue_node< MonitorSettingsType > monitor_settings_queue;

		SyncedLogger & logger;
		const InitParams params;
		AudioOutputActivitiy &audioOutputActivitiy;
		MonitorSettings monitorSettings;

		struct mf_body {
			ControlNode &h;
			shared_ptr<MonitorDataFrame> outputFrame = nullptr;
			size_t ctr = 0;
		public:
			mf_body(ControlNode &h) : h{ h } {}

			void operator()(const mf_input_type &input, multifunction_node_type::output_ports_type &op) {
				auto& frame = get<0>(input);
				auto& daqmx_frame = get<1>(input);

				Kwargs options;
				while(h.monitor_settings_queue.try_get(options)) h.monitorSettings.set(options);

				outputFrame = make_shared<MonitorDataFrame>();

				outputFrame->sdr_spectrum = frame->sdr_spectrum;
				outputFrame->signal_strengths.assign(frame->signal_strengths.begin(), frame->signal_strengths.begin() + h.params.SDR_ChannelCount);
				outputFrame->carrier_frequencies.assign(frame->carrier_frequencies.begin(), frame->carrier_frequencies.begin() + h.params.SDR_ChannelCount);
				outputFrame->receive_frequencies.assign(frame->receive_frequencies.begin(), frame->receive_frequencies.begin() + h.params.SDR_ChannelCount);

				outputFrame->channel_number = h.monitorSettings.channel_number;

				switch(h.monitorSettings.channel_type)
				{
					case MonitorSettings::ChannelType::sdr:
						outputFrame->channel_type = "sdr";

						if(h.monitorSettings.channel_number < h.params.SDR_ChannelCount)
							outputFrame->output_signal = frame->demodulated_signals[h.monitorSettings.channel_number];

						if(h.monitorSettings.play_audio &&
							h.monitorSettings.channel_number < h.params.SDR_ChannelCount &&
							frame->signal_strengths[h.monitorSettings.channel_number] > h.monitorSettings.squelch)
						{
							h.audioOutputActivitiy.try_put(outputFrame->output_signal, h.params.SDR_ChannelSampleRate);
						}
						break;
					case MonitorSettings::ChannelType::daqmx:
						outputFrame->channel_type = "daqmx";

						if(h.monitorSettings.channel_number < h.params.DAQmx_ChannelCount)
						{
							outputFrame->output_signal.reserve(h.params.DAQmx_FrameSize);
							auto oit = back_inserter(outputFrame->output_signal);
							for(auto it = daqmx_frame->cbegin() + h.monitorSettings.channel_number; it < daqmx_frame->cend(); advance(it, h.params.DAQmx_ChannelCount))
							{
								*oit = *it;
							}
						}

						if(h.monitorSettings.play_audio && h.monitorSettings.channel_number < h.params.DAQmx_ChannelCount)
						{
							h.audioOutputActivitiy.try_put(outputFrame->output_signal, h.params.DAQmx_SampleRate);
						}
						break;
				}
				

				get<1>(op).try_put(outputFrame);
				get<0>(op).try_put(input);
				++ctr;
				return;
			}
		};

	public:
		ControlNode(graph &g, const InitParams &params, AudioOutputActivitiy &audioOutputActivitiy, SyncedLogger &logger) :
			j(g),
			mf(g, serial, mf_body(*this)),
			monitor_settings_queue{ g },
			params{ params },
			logger{ logger },
			audioOutputActivitiy{ audioOutputActivitiy } 
		{
#ifdef VERBOSE
			std::cout << "ControlNode()" << endl;
#endif
			make_edge(j, mf);
		}

		receiver< SdrFrameType > &sdrFrameInputPort{ input_port<0>(j) };
		receiver< NIDAQmxFrameType > &daqmxFrameInputPort{ input_port<1>(j) };
		receiver< MonitorSettingsType > &monitorSettingsInputPort{ monitor_settings_queue };

		sender< mf_input_type > &fileWriterOutputPort{ output_port<0>(mf) };
		sender< shared_ptr<MonitorDataFrame> > &outputFrameOutputPort{ output_port<1>(mf) };
	};
}
