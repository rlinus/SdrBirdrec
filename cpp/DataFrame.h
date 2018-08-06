#pragma once
#include <complex>
#include <vector>
#include "Types.h"
#include "InitParams.h"

namespace SdrBirdrec
{
	using namespace std;

	struct DataFrame
	{
	public:
		DataFrame(const InitParams &params) :
			sdr_signal(params.SDR_FrameSize),
			sdr_spectrum(params.Decimator1_Nfft),
			demodulated_signals(params.SDR_ChannelCount, vector<dsp_t>(params.SDR_FrameSize / (params.Decimator1_Factor*params.Decimator2_Factor))),
			signal_strengths(params.SDR_ChannelCount*params.MonitorRateDivisor),
			carrier_frequencies(params.SDR_ChannelCount*params.MonitorRateDivisor),
			receive_frequencies(params.SDR_ChannelCount*params.MonitorRateDivisor)
		{
		}

		size_t index = 0;
		vector<complex<dsp_t>> sdr_signal;
		vector<dsp_t> sdr_spectrum;
		vector < vector<dsp_t> > demodulated_signals;
		vector < dsp_t > signal_strengths;
		vector < dsp_t > carrier_frequencies;
		vector < dsp_t > receive_frequencies;
	};

	struct OutputFrame
	{
	public:
		std::vector<dsp_t> sdr_spectrum;
		std::vector < dsp_t > signal_strengths;
		std::vector < dsp_t > carrier_frequencies;
		std::vector < dsp_t > receive_frequencies;
		std::vector<dsp_t> output_signal;
		std::string channel_type;
		size_t channel_number;
	};
}
