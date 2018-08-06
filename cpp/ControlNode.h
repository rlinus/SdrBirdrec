#pragma once
#include <string>
#include <memory>

#include <tbb/flow_graph.h>

#include "Types.h"
#include "DataFrame.h"
#include "InitParams.h"
#include "NodeFunctors.h"

namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;

	class ControlNode
	{
	private:
		struct MonitorSettings
		{
		public:
			enum class ChannelType { sdr, daqmx };
			const map<string, ChannelType> channelTypeMap = { { "sdr"s, ChannelType::sdr }, { "daqmx"s, ChannelType::daqmx } };
			bool play_audio = false;
			ChannelType channel_type = ChannelType::sdr;
			size_t channel_number = 0;
			double squelch = -2000.0;

			void set(Kwargs options)
			{
				auto search = options.find("play_audio");
				if(search != options.end()) play_audio = bool(stoi(search->second));

				search = options.find("channel_number");
				if(search != options.end()) channel_number = stoull(search->second);

				search = options.find("channel_type");
				if(search != options.end() && channelTypeMap.find(search->second) != channelTypeMap.cend()) channel_type = channelTypeMap.at(search->second);

				search = options.find("squelch");
				if(search != options.end()) squelch = stod(search->second);
			}
		};

		using sdr_frame_type = shared_ptr<DataFrame>;
		using daqmx_frame_type = shared_ptr<vector<dsp_t>>;
		using monitor_settings_type = Kwargs;

		using mf_input_type = tuple< sdr_frame_type, daqmx_frame_type >;
		using mf_output_type = tuple< mf_input_type, shared_ptr<OutputFrame>>;
		using multifunction_node_type = multifunction_node< mf_input_type, mf_output_type >;

		join_node<  mf_input_type, queueing > j;
		multifunction_node_type mf;
		queue_node< monitor_settings_type > monitor_settings_queue;

		const InitParams params;
		AudioOutputActivitiy &audioOutputActivitiy;
		MonitorSettings monitorSettings;

		struct mf_body {
			ControlNode &h;
			shared_ptr<OutputFrame> outputFrame = nullptr;
			size_t ctr = 0;
			bool play_audio = false;
		public:
			mf_body(ControlNode &h) : h{ h } {}

			void operator()(const mf_input_type &input, multifunction_node_type::output_ports_type &op) {
				auto& frame = get<0>(input);
				auto& daqmx_frame = get<1>(input);

				Kwargs options;
				while(h.monitor_settings_queue.try_get(options)) h.monitorSettings.set(options);

				outputFrame = make_shared<OutputFrame>();

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

						play_audio = h.monitorSettings.play_audio &&
							h.monitorSettings.channel_number < h.params.SDR_ChannelCount &&
							frame->signal_strengths[h.monitorSettings.channel_number] > h.monitorSettings.squelch;
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

						play_audio = h.monitorSettings.play_audio && h.monitorSettings.channel_number < h.params.DAQmx_ChannelCount;
						break;
				}
				
				if(play_audio)
				{
					//auto audioFrame = make_shared<vector<dsp_t>>(outputFrame->output_signal);
					h.audioOutputActivitiy.try_put(outputFrame->output_signal, h.params.SDR_ChannelSampleRate);
				}

				get<1>(op).try_put(outputFrame);
				get<0>(op).try_put(input);
				++ctr;
				return;
			}
		};

	public:
		ControlNode(graph &g, const InitParams &params, AudioOutputActivitiy &audioOutputActivitiy) :
			j(g),
			mf(g, serial, mf_body(*this)),
			monitor_settings_queue{ g },
			params{ params },
			audioOutputActivitiy{ audioOutputActivitiy } 
		{
#ifdef VERBOSE
			std::cout << "ControlNode()" << endl;
#endif
			make_edge(j, mf);
		}

		receiver< sdr_frame_type > &sdr_frame_input_port{ input_port<0>(j) };
		receiver< daqmx_frame_type > &daqmx_frame_input_port{ input_port<1>(j) };
		receiver< monitor_settings_type > &monitor_settings_input_port{ monitor_settings_queue };

		sender< mf_input_type > &file_writer_output_port{ output_port<0>(mf) };
		sender< shared_ptr<OutputFrame> > &output_frame_output_port{ output_port<1>(mf) };
	};
}
