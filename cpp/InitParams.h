#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <sndfile.hh>
#include "Types.h"
#include "UserParams.h"

namespace SdrBirdrec
{
	using namespace std;

	/*!
	* This struct defines some additional settings derived from the UserParams. It also checks the UserParams on construction.
	*/
	struct InitParams : public UserParams
	{
		size_t SDR_FrameSize;
		size_t MonitorRateDivisor;
		size_t NumSdrSamplesPerTrackingSample;
		size_t SDR_ChannelCount;
		size_t DAQmx_FrameSize;

		size_t Decimator1_FilterOrder;
		size_t Decimator2_FilterOrder;

		double SDR_IntermediateSampleRate;
		double SDR_ChannelSampleRate;

		size_t SDR_IntermediateFrameSize;
		size_t SDR_ChannelFrameSize;

		size_t Decimator2_InputFrameSize;

		size_t Decimator1_Nfft;
		size_t Decimator2_Nfft;
		size_t Decimator1_Nifft;
		size_t Decimator2_Nifft;

		const map<string, int> DataFile_SamplePrecision_Map = { { "int16", SF_FORMAT_PCM_16 }, { "float32", SF_FORMAT_FLOAT } };
		const map<string, int> DataFile_Format_Map = { { "w64", SF_FORMAT_W64 }, { "rf64", SF_FORMAT_RF64 }, { "wav", SF_FORMAT_WAV } };

		InitParams(const UserParams &userParams) : UserParams(userParams),
			SDR_FrameSize{ size_t(SDR_SampleRate / MonitorRate) },
			MonitorRateDivisor{ size_t(SDR_TrackingRate / MonitorRate) },
			NumSdrSamplesPerTrackingSample{ size_t(SDR_SampleRate / SDR_TrackingRate) },
			SDR_ChannelCount{ SDR_ChannelBands.size() },
			DAQmx_FrameSize{ size_t(double(DAQmx_SampleRate) / MonitorRate) },
			Decimator1_FilterOrder{ Decimator1_FirFilterCoeffs.size() - 1 },
			Decimator2_FilterOrder{ Decimator2_FirFilterCoeffs.size() - 1 },
			SDR_IntermediateSampleRate{ SDR_SampleRate / double(Decimator1_Factor) },
			SDR_ChannelSampleRate{ SDR_SampleRate / double(Decimator1_Factor * Decimator2_Factor) },
			SDR_IntermediateFrameSize{ size_t(SDR_IntermediateSampleRate / MonitorRate) },
			SDR_ChannelFrameSize{ size_t(SDR_ChannelSampleRate / MonitorRate) },
			Decimator1_Nfft{ Decimator1_InputFrameSize + Decimator1_FilterOrder },
			Decimator1_Nifft{ Decimator1_Nfft / Decimator1_Factor }
		{
			// check userParams
			if(SDR_ChannelCount == 0)
				throw invalid_argument("Please specify at least one SDR channel");
			if(double(SDR_FrameSize) * MonitorRate != SDR_SampleRate)
				throw invalid_argument("SDR_SampleRate must be an integer multiple of MonitorRate");
			if(double(MonitorRateDivisor) * MonitorRate != SDR_TrackingRate)
				throw invalid_argument("SDR_TrackingRate must be an integer multiple of MonitorRate");
			if(double(NumSdrSamplesPerTrackingSample) * SDR_TrackingRate != SDR_SampleRate)
				throw invalid_argument("SDR_SampleRate must be an integer multiple of SDR_TrackingRate");
			if(Decimator1_FirFilterCoeffs.size() == 0)
				throw invalid_argument("Decimator1_FirFilterCoeffs is empty");
			if(Decimator2_FirFilterCoeffs.size() == 0)
				throw invalid_argument("Decimator2_FirFilterCoeffs is empty");
			if(Decimator1_Factor == 0)
				throw invalid_argument("Decimator1_Factor must be positive");
			if(Decimator2_Factor == 0)
				throw invalid_argument("Decimator2_Factor must be positive");
			if(double(DAQmx_FrameSize) * MonitorRate != double(DAQmx_SampleRate))
				throw invalid_argument("DAQmx_SampleRate must be an integer multiple of MonitorRate");
			if(SDR_IntermediateSampleRate * double(Decimator1_Factor) != SDR_SampleRate)
				throw invalid_argument("SDR_SampleRate must be dividable by Decimator1_Factor");
			if(SDR_ChannelSampleRate * double(Decimator1_Factor * Decimator2_Factor) != SDR_SampleRate)
				throw invalid_argument("SDR_SampleRate must be dividable by (Decimator1_Factor * Decimator2_Factor)");
			if(SDR_ChannelFrameSize * (double(Decimator1_Factor * Decimator2_Factor)*MonitorRate) != SDR_SampleRate)
				throw invalid_argument("SDR_SampleRate must be dividable by (Decimator1_Factor * Decimator2_Factor * MonitorRate)");
			if(FreqTreckingThreshold >= SDR_IntermediateSampleRate / 2)
				throw invalid_argument("FreqTreckingThreshold must be smaller then SDR_SampleRate/Decimator1_Factor/2");
			if(FreqTreckingThreshold < 0)
				throw invalid_argument("FreqTreckingThreshold must be positive");

			if(NumSdrSamplesPerTrackingSample % Decimator1_InputFrameSize != 0)
				throw invalid_argument("Decimator1_InputFrameSize must be a divisor of (SDR_SampleRate / SDR_TrackingRate)");
			if(Decimator1_FilterOrder % Decimator1_Factor != 0) //condition 1 (Borgerding 2006)
				throw invalid_argument("the filter order (Decimator1_FirFilterCoeffs.size()-1) must be an integer multiple of Decimator1_Factor");
			if(Decimator1_InputFrameSize % Decimator1_FilterOrder != 0) //V1 is integer --> implies condition 2
				throw invalid_argument("Decimator1_InputFrameSize must be an integer multiple of the Decimator1_FirFilter order");

			if(Decimator2_FilterOrder % Decimator2_Factor != 0) //condition 1 (Borgerding 2006)
				throw invalid_argument("the Decimator2_FirFilter order must be an integer multiple of Decimator2_Factor");


			// find Decimator2_InputFrameSize ~= 4 * Decimator2_FilterOrder && and which is dividable by Decimator2_Factor (Borgerding 2006)
			Decimator2_InputFrameSize = 4 * Decimator2_FilterOrder;
			if(Decimator2_InputFrameSize > SDR_ChannelFrameSize * Decimator2_Factor) Decimator2_InputFrameSize = SDR_ChannelFrameSize * Decimator2_Factor;
			while(SDR_ChannelFrameSize * Decimator2_Factor % Decimator2_InputFrameSize != 0) Decimator2_InputFrameSize += Decimator2_Factor;
			Decimator2_Nfft = Decimator2_InputFrameSize + Decimator2_FilterOrder;
			Decimator2_Nifft = Decimator2_Nfft / Decimator2_Factor;

			// check for valid DataFile_SamplePrecision
			if(DataFile_SamplePrecision_Map.find(DataFile_SamplePrecision) == DataFile_SamplePrecision_Map.cend()) throw invalid_argument("invalid DataFile_SamplePrecision");

			// check for valid DataFile_Format
			if (DataFile_SamplePrecision_Map.find(DataFile_Format) == DataFile_Format_Map.cend()) throw invalid_argument("invalid DataFile_Format");
		}

		void print()
		{
			cout << "----InitParams:" << endl;
			cout << "SDR_ChannelCount: " << SDR_ChannelCount << endl;
			cout << "SDR_SampleRate: " << SDR_SampleRate << endl;
			cout << "SDR_IntermediateSampleRate: " << SDR_IntermediateSampleRate << endl;
			cout << "SDR_ChannelSampleRate: " << SDR_ChannelSampleRate << endl;
			cout << "SDR_FrameSize: " << SDR_FrameSize << endl;
			cout << "SDR_ChannelFrameSize: " << SDR_ChannelFrameSize << endl;
			cout << "Decimator1_Factor: " << Decimator1_Factor << endl;
			cout << "Decimator2_Factor: " << Decimator2_Factor << endl;
			cout << "MonitorRate: " << MonitorRate << endl;
			cout << "Decimator1_InputFrameSize: " << Decimator1_InputFrameSize << endl;
			cout << "Decimator2_InputFrameSize: " << Decimator2_InputFrameSize << endl;
			cout << "Decimator1_FilterOrder: " << Decimator1_FilterOrder << endl;
			cout << "Decimator2_FilterOrder: " << Decimator2_FilterOrder << endl;
			cout << "Decimator1_Nfft: " << Decimator1_Nfft << endl;
			cout << "Decimator2_Nfft: " << Decimator2_Nfft << endl;
			cout << "---------------" << endl;
		}
	};
}