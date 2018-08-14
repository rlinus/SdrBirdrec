#pragma once

#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <stdexcept>
#include <tbb/tbb.h>
#include <sndfile.hh>

#include "SdrDataFrame.h"
#include "InitParams.h"
#include "SyncedLogger.h"
#include "Types.h"

namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;


	struct FileWriterNodeBody
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
		FileWriterNodeBody(const InitParams &params) :
			params{ params },
			SdrChannelsH{ params.SdrChannelsFilename, SFM_WRITE, SF_FORMAT_W64 | params.DataFile_SamplePrecision_Map.at(params.DataFile_SamplePrecision), params.SDR_ChannelCount, params.SDR_ChannelSampleRate },
			SdrSignalStrengthH{ params.SdrSignalStrengthFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate },
			SdrCarrierFreqH{ params.SdrCarrierFreqFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate },
			SdrReceiveFreqH{ params.SdrReceiveFreqFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate },
			DAQmxChannelsH{ params.DAQmxChannelsFilename, SFM_WRITE, SF_FORMAT_W64 | params.DataFile_SamplePrecision_Map.at(params.DataFile_SamplePrecision), params.DAQmx_ChannelCount, params.DAQmx_SampleRate },
			filterDelay{ (size_t)round(params.Decimator1_FilterOrder / (2.0* params.Decimator1_Factor*params.Decimator2_Factor) + params.Decimator2_FilterOrder / (2.0 * params.Decimator2_Factor)) }
		{}

		using input_type = tuple<shared_ptr<SdrDataFrame>, shared_ptr<vector<dsp_t>>>;

		continue_msg operator()(const input_type &input)
		{
			auto& frame = get<0>(input);
			auto& daqmxFrame = get<1>(input);

			if(params.SDR_ChannelCount != 0)
			{
				//interleave sdr channels
				auto n = frame->demodulated_signals[0].size();
				vector<dsp_t> SdrChannelsBuffer;
				SdrChannelsBuffer.reserve(params.SDR_ChannelCount*n);
				for(size_t i = 0; i < n; ++i)
				{
					for(size_t k = 0; k < params.SDR_ChannelCount; ++k)
					{
						SdrChannelsBuffer.push_back(frame->demodulated_signals[k][i]);
					}
				}

				if(firstFrame)
				{
					//compensate for group delay of decimation filters
					SdrChannelsH.write(SdrChannelsBuffer.data() + filterDelay * params.SDR_ChannelCount, SdrChannelsBuffer.size() - filterDelay * params.SDR_ChannelCount);
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