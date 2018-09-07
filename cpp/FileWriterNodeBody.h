#pragma once

#include <string>
#include <vector>
#include <memory>
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

		shared_ptr<SndfileHandle> SdrChannelsH;
		shared_ptr<SndfileHandle> SdrSignalStrengthH;
		shared_ptr<SndfileHandle> SdrCarrierFreqH;
		shared_ptr<SndfileHandle> SdrReceiveFreqH;
		shared_ptr<SndfileHandle> DAQmxChannelsH;

		bool firstFrame = true;
		size_t filterDelay;
	public:
		FileWriterNodeBody(const InitParams &params) :
			params{ params },
			filterDelay{ (size_t)round(params.Decimator1_FilterOrder / (2.0* params.Decimator1_Factor*params.Decimator2_Factor) + params.Decimator2_FilterOrder / (2.0 * params.Decimator2_Factor)) }
		{
			//if filenames are empty or zero channels are requested don't create the files
			if(!params.SdrChannelsFilename.empty() && params.SDR_ChannelCount != 0)
				SdrChannelsH = make_shared<SndfileHandle>(params.SdrChannelsFilename, SFM_WRITE, SF_FORMAT_W64 | params.DataFile_SamplePrecision_Map.at(params.DataFile_SamplePrecision), params.SDR_ChannelCount, params.SDR_ChannelSampleRate);

			if(!params.SdrSignalStrengthFilename.empty() && params.SDR_ChannelCount != 0)
				SdrSignalStrengthH = make_shared<SndfileHandle>(params.SdrSignalStrengthFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate);

			if(!params.SdrCarrierFreqFilename.empty() && params.SDR_ChannelCount != 0)
				SdrCarrierFreqH = make_shared<SndfileHandle>(params.SdrCarrierFreqFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate);

			if(!params.SdrReceiveFreqFilename.empty() && params.SDR_ChannelCount != 0)
				SdrReceiveFreqH = make_shared<SndfileHandle>(params.SdrReceiveFreqFilename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_FLOAT, params.SDR_ChannelCount, params.SDR_TrackingRate);

			if(!params.DAQmxChannelsFilename.empty() && params.DAQmx_ChannelCount != 0)
				DAQmxChannelsH = make_shared<SndfileHandle>(params.DAQmxChannelsFilename, SFM_WRITE, SF_FORMAT_W64 | params.DataFile_SamplePrecision_Map.at(params.DataFile_SamplePrecision), params.DAQmx_ChannelCount, params.DAQmx_SampleRate);
		}

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
					if(SdrChannelsH) SdrChannelsH->write(SdrChannelsBuffer.data() + filterDelay * params.SDR_ChannelCount, SdrChannelsBuffer.size() - filterDelay * params.SDR_ChannelCount);
					firstFrame = false;
				}
				else
				{
					if(SdrChannelsH) SdrChannelsH->write(SdrChannelsBuffer.data(), SdrChannelsBuffer.size());
				}

				//write the rest
				if(SdrSignalStrengthH) SdrSignalStrengthH->write(frame->signal_strengths.data(), frame->signal_strengths.size());
				if(SdrCarrierFreqH) SdrCarrierFreqH->write(frame->carrier_frequencies.data(), frame->carrier_frequencies.size());
				if(SdrReceiveFreqH) SdrReceiveFreqH->write(frame->receive_frequencies.data(), frame->receive_frequencies.size());
			}
			if(DAQmxChannelsH) DAQmxChannelsH->write(daqmxFrame->data(), daqmxFrame->size());

			return continue_msg();
		}
	};

}