#pragma once
#include <complex>
#include <vector>
#include "Types.h"
#include "InitParams.h"

namespace SdrBirdrec
{
	using namespace std;

	struct SdrDataFrame
	{
	public:
		SdrDataFrame(const InitParams &params) :
			sdr_signal(params.SDR_FrameSize),
			sdr_spectrum(params.Decimator1_Nfft),
			demodulated_signals(params.SDR_ChannelCount, vector<dsp_t>(params.SDR_FrameSize / (params.Decimator1_Factor*params.Decimator2_Factor))),
			signal_strengths(params.SDR_ChannelCount*params.MonitorRateDivisor),
			carrier_frequencies(params.SDR_ChannelCount*params.MonitorRateDivisor),
			receive_frequencies(params.SDR_ChannelCount*params.MonitorRateDivisor),
			noise_level(params.MonitorRateDivisor)
		{
		}

		size_t index = 0;
		vector<complex<dsp_t>> sdr_signal;
		vector<dsp_t> sdr_spectrum;
		vector < vector<dsp_t> > demodulated_signals;
		vector < dsp_t > signal_strengths;
		vector < dsp_t > carrier_frequencies;
		vector < dsp_t > receive_frequencies;
		vector < dsp_t > noise_level;
	};

	
}
