#pragma once
#include <vector>
#include "Types.h"

namespace SdrBirdrec
{
	using namespace std;

	struct MonitorDataFrame
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