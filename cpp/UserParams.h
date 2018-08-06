#pragma once
#include <vector>
#include <string>
#include "Types.h"

namespace SdrBirdrec
{
	using namespace std;

	struct UserParams
	{
		struct ChannelBand
		{
			double minFreq;
			double maxFreq;
		};

		double SDR_SampleRate = 1e6;
		double SDR_CenterFrequency = 300e6;
		bool SDR_AGC = false;
		int SDR_Gain = 50;
		bool SDR_StartOnTrigger = false;
		bool SDR_ExternalClock = false;

		double SDR_TrackingRate = 20;
		double MonitorRate = 5;

		size_t Decimator1_InputFrameSize = 10000;

		size_t Decimator1_Factor = 20;
		size_t Decimator2_Factor = 10;

		vector<dsp_t> Decimator1_FirFilterCoeffs = { 1.0 };
		vector<dsp_t> Decimator2_FirFilterCoeffs = { 1.0 };
		vector<vector<dsp_t>> IirFilterCoeffs = { { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 } };
		double SdrChannels_AudioGain = 1;

		vector<ChannelBand> SDR_ChannelBands = {}; //list of carrier frequencies ranges (in range [-SDR_SampleRate,SDR_SampleRate)

		size_t DAQmx_ChannelCount = 0;
		string DAQmx_AIChannelList = ""s;
		string DAQmx_TriggerOutputTerminal = ""s;
		string DAQmx_ClockInputTerminal = ""s;
		bool DAQmx_ExternalClock = false;
		string DAQmx_AITerminalConfig = "DAQmx_Val_Cfg_Default"s;
		double DAQmx_SampleRate = 24000;
		double DAQmx_MaxVoltage = 10.0;
		bool DAQmx_AILowpassEnable = false;
		double DAQmx_AILowpassCutoffFreq = 12000.0;

		size_t AudioInput_SampleRate = 24000;
		size_t AudioInput_ChannelCount = 0;
		int AudioInput_DeviceIndex = -1;

		int AudioOutput_DeviceIndex = -1;


		string DataFile_SamplePrecision = "int16";

		string SdrChannelsFilename = "SdrChannels.w64"s;
		string SdrSignalStrengthFilename = "SdrSignalStrength.w64"s;
		string SdrCarrierFreqFilename = "SdrCarrierFreq.w64"s;
		string SdrReceiveFreqFilename = "SdrReceiveFreq.w64"s;
		string DAQmxChannelsFilename = "DAQmxChannels.w64"s;
		string AudioChannelsFilename = "AudioChannels.w64"s;
		string LogFilename = "Log.txt";
	};
}
