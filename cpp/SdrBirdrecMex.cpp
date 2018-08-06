//#include <cstdint>
#include <string>
//#include <cstring>
//#include <typeinfo>
//#include <type_traits>
//#include <unordered_map>
#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <complex>
//#include <thread>
//#include <mutex>
//#include <memory>
//#include <atomic>
#include <vector>
//#include <map>
#include <exception>
//#include <stdexcept>
//#include <numeric>
//#include <utility>
//#include <algorithm>

//#include "stdafx.h"
#include "mexUtils/class_handle.h"
#include "mexUtils/mexUtils.h"

#include "Types.h"
#include "UserInterface.h"
#include "UserParams.h"


namespace SdrBirdrec
{
	mexUtils::redirectOstream2mexPrintf redirect;
	const char mexFileName[] = "SdrBirdrecMex";
	SdrBirdrecEnvironment environment;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	using namespace mexUtils;
	using namespace SdrBirdrec;

	char cmd[64];	

	try
	{
		// Get the command string
		if (nrhs < 1 || mxGetString(prhs[0], cmd, sizeof(cmd)))
		{
			strcpy_s(cmd, sizeof(cmd), "invalidCommandString");
			throw std::invalid_argument("First input should be a command string less than 64 characters long.");
		}

		// static class methods
		if (!strcmp("findSdrDevices", cmd)) {
			if (nlhs > 1) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(UserInterface::findSdrDevices());
			return;
		}

		if (!strcmp("findAudioInputDevices", cmd)) {
			if (nlhs > 1) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(UserInterface::findAudioInputDevices());
			return;
		}

		if (!strcmp("findAudioOutputDevices", cmd)) {
			if (nlhs > 1) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(UserInterface::findAudioOutputDevices());
			return;
		}

		if(!strcmp("findNIDAQmxDevices", cmd)) {
			if(nlhs > 1) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(UserInterface::findNIDAQmxDevices());
			return;
		}

		if (!strcmp("staticTest", cmd)) {
			if (nlhs < 0 || nrhs < 1) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(UserInterface::staticTest());
			return;
		}

		// New
		if (!strcmp("new", cmd)) {
			// Check parameters
			if (nlhs != 1)
				throw std::invalid_argument("One output expected.");

			UserInterface* obj;
			if (nrhs == 1) obj = new UserInterface();
			else if (nrhs == 2) obj = new UserInterface(Cast::fromMxArray<Kwargs>(prhs[1]));
			else throw std::invalid_argument("Unexpected arguments.");

			// Return a handle to a new C++ instance
			plhs[0] = convertPtr2Mat<UserInterface>(obj);
			return;
		}

		// Check there is a second input, which should be the class instance handle
		if (nrhs < 2)
			throw std::invalid_argument("Second input must be a class instance handle for a non-static method");

		// Delete
		if (!strcmp("delete", cmd)) {
			// Destroy the C++ object
			destroyObject<UserInterface>(prhs[1]);
			return;
		}

		// Get the class instance pointer from the second input
		UserInterface *sdrBirdrecMex = convertMat2Ptr<UserInterface>(prhs[1]);

		if (!strcmp("initStream", cmd)) {
			if (nlhs < 0 || nrhs < 3) throw std::invalid_argument("Unexpected arguments.");
			const mxArray *mxParams = prhs[2];
			if(!mxIsStruct(mxParams)) throw std::invalid_argument("params must be a struct.");

			UserParams params;
			mxArray * fieldptr;

			fieldptr = mxGetField(mxParams, 0, "DataFile_SamplePrecision");
			if(fieldptr) params.DataFile_SamplePrecision = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SdrChannelsFilename");
			if (fieldptr) params.SdrChannelsFilename = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SdrSignalStrengthFilename");
			if (fieldptr) params.SdrSignalStrengthFilename = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SdrCarrierFreqFilename");
			if (fieldptr) params.SdrCarrierFreqFilename = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SdrReceiveFreqFilename");
			if (fieldptr) params.SdrReceiveFreqFilename = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmxChannelsFilename");
			if(fieldptr) params.DAQmxChannelsFilename = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "AudioChannelsFilename");
			if (fieldptr) params.AudioChannelsFilename = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "LogFilename");
			if(fieldptr) params.LogFilename = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "AudioInput_SampleRate");
			if(fieldptr) params.AudioInput_SampleRate = Cast::fromMxArray<size_t>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "AudioInput_ChannelCount");
			if(fieldptr) params.AudioInput_ChannelCount = Cast::fromMxArray<size_t>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "AudioInput_DeviceIndex");
			if(fieldptr) params.AudioInput_DeviceIndex = Cast::fromMxArray<int>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "AudioOutput_DeviceIndex");
			if(fieldptr) params.AudioOutput_DeviceIndex = Cast::fromMxArray<int>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SDR_SampleRate");
			if(fieldptr) params.SDR_SampleRate = Cast::fromMxArray<double>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SDR_CenterFrequency");
			if(fieldptr) params.SDR_CenterFrequency = Cast::fromMxArray<double>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SDR_AGC");
			if(fieldptr) params.SDR_AGC = Cast::fromMxArray<bool>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SDR_Gain");
			if(fieldptr) params.SDR_Gain = Cast::fromMxArray<int>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "MonitorRate");
			if(fieldptr) params.MonitorRate = Cast::fromMxArray<double>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SdrChannels_AudioGain");
			if(fieldptr) params.SdrChannels_AudioGain = Cast::fromMxArray<double>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SDR_StartOnTrigger");
			if(fieldptr) params.SDR_StartOnTrigger = Cast::fromMxArray<bool>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SDR_ExternalClock");
			if(fieldptr) params.SDR_ExternalClock = Cast::fromMxArray<bool>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "SDR_TrackingRate");
			if(fieldptr) params.SDR_TrackingRate = Cast::fromMxArray<double>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "Decimator1_InputFrameSize");
			if(fieldptr) params.Decimator1_InputFrameSize = Cast::fromMxArray<size_t>(fieldptr);

			/*fieldptr = mxGetField(mxParams, 0, "Decimator2_InputFrameSize");
			if(fieldptr) params.Decimator2_InputFrameSize = Cast::fromMxArray<size_t>(fieldptr);*/

			fieldptr = mxGetField(mxParams, 0, "Decimator1_Factor");
			if(fieldptr) params.Decimator1_Factor = Cast::fromMxArray<size_t>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "Decimator2_Factor");
			if(fieldptr) params.Decimator2_Factor = Cast::fromMxArray<size_t>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "Decimator1_FirFilterCoeffs");
			if(fieldptr) params.Decimator1_FirFilterCoeffs = Cast::fromMxArray<vector<dsp_t>>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "Decimator2_FirFilterCoeffs");
			if(fieldptr) params.Decimator2_FirFilterCoeffs = Cast::fromMxArray<vector<dsp_t>>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "IirFilterCoeffs");
			if(fieldptr)
			{
				auto tmp = Cast::fromMxArray<vector<vector<dsp_t>>>(fieldptr);
				if(tmp.size() != 0)
				{
					auto n = 6;
					if(tmp.size() != n) throw std::invalid_argument("IirFilterCoeffs must be an array of size m x 6");
					auto nstages = tmp[0].size();
					params.IirFilterCoeffs = vector<vector<dsp_t>>(nstages, vector<dsp_t>(n));

					// transpose tmp
					for(int i = 0; i < nstages; ++i)
					{
						for(int j = 0; j < n; ++j)
						{
							params.IirFilterCoeffs[i][j] = tmp[j][i];
						}
					}
				}
				else
				{
					params.IirFilterCoeffs = vector<vector<dsp_t>>(0);
				}
			}

			fieldptr = mxGetField(mxParams, 0, "SDR_ChannelBands");
			if(fieldptr)
			{
				auto tmp = Cast::fromMxArray<vector<vector<double>>>(fieldptr);
				if(tmp.size() != 0)
				{
					if(tmp.size() != 2) throw std::invalid_argument("SDR_ChannelBands must be an array of size_ nch x 2");
					auto nch = tmp[0].size();
					params.SDR_ChannelBands = vector<UserParams::ChannelBand>(nch);
					for(int i = 0; i < nch; ++i)
					{
						params.SDR_ChannelBands[i].minFreq = tmp[0][i];
						params.SDR_ChannelBands[i].maxFreq = tmp[1][i];
					}
				}

			}

			fieldptr = mxGetField(mxParams, 0, "DAQmx_ChannelCount");
			if(fieldptr) params.DAQmx_ChannelCount = Cast::fromMxArray<size_t>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_AIChannelList");
			if(fieldptr) params.DAQmx_AIChannelList = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_TriggerOutputTerminal");
			if(fieldptr) params.DAQmx_TriggerOutputTerminal = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_ClockInputTerminal");
			if(fieldptr) params.DAQmx_ClockInputTerminal = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_ExternalClock");
			if(fieldptr) params.DAQmx_ExternalClock = Cast::fromMxArray<bool>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_AITerminalConfig");
			if(fieldptr) params.DAQmx_AITerminalConfig = Cast::fromMxArray<std::string>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_SampleRate");
			if(fieldptr) params.DAQmx_SampleRate = Cast::fromMxArray<double>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_MaxVoltage");
			if(fieldptr) params.DAQmx_MaxVoltage = Cast::fromMxArray<double>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_AILowpassEnable");
			if(fieldptr) params.DAQmx_AILowpassEnable = Cast::fromMxArray<bool>(fieldptr);

			fieldptr = mxGetField(mxParams, 0, "DAQmx_AILowpassCutoffFreq");
			if(fieldptr) params.DAQmx_AILowpassCutoffFreq = Cast::fromMxArray<double>(fieldptr);

			sdrBirdrecMex->initStream(params);
			return;
		}

		if(!strcmp("startStream", cmd)) {
			if(nlhs < 0 || nrhs < 2) throw std::exception("Unexpected arguments.");
			sdrBirdrecMex->startStream();
			return;
		}

		if (!strcmp("stopStream", cmd)) {
			if (nlhs < 0 || nrhs < 2) throw std::exception("Unexpected arguments.");
			sdrBirdrecMex->stopStream();
			return;
		}

		if (!strcmp("getData", cmd)) {
			if (nlhs < 0 || nrhs < 2) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = sdrBirdrecMex->getData();
			return;
		}

		if (!strcmp("isStreaming", cmd)) {
			if (nlhs < 0 || nrhs < 2) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(sdrBirdrecMex->isStreaming());
			return;
		}

		if (!strcmp("setSquelch", cmd)) {
			if (nlhs < 0 || nrhs < 3) throw std::invalid_argument("Unexpected arguments.");
			sdrBirdrecMex->setSquelch(Cast::fromMxArray<double>(prhs[2]));
			return;
		}

		if (!strcmp("setChannelNumber", cmd)) {
			if (nlhs < 0 || nrhs < 3) throw std::invalid_argument("Unexpected arguments.");
			sdrBirdrecMex->setChannelNumber(Cast::fromMxArray<size_t>(prhs[2]));
			return;
		}

		if(!strcmp("setChannelType", cmd)) {
			if(nlhs < 0 || nrhs < 3) throw std::invalid_argument("Unexpected arguments.");
			sdrBirdrecMex->setChannelType(Cast::fromMxArray<string>(prhs[2]));
			return;
		}

		if (!strcmp("setPlayAudio", cmd)) {
			if (nlhs < 0 || nrhs < 3) throw std::invalid_argument("Unexpected arguments.");
			sdrBirdrecMex->setPlayAudio(Cast::fromMxArray<bool>(prhs[2]));
			return;
		}

		if (!strcmp("isRefPLLlocked", cmd)) {
			if (nlhs < 0 || nrhs < 2) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(sdrBirdrecMex->isRefPLLlocked());
			return;
		}

		if(!strcmp("test", cmd)) {
			if(nlhs < 0 || nrhs < 2) throw std::invalid_argument("Unexpected arguments.");
			plhs[0] = Cast::toMxArray(sdrBirdrecMex->test());
			return;
		}

		// Got here, so command not recognized
		throw std::invalid_argument("Command not recognized.");
	}
	catch (std::exception& e)
	{
		std::stringstream error_id, error_txt;
		error_id << mexFileName << ":" << cmd;
		error_txt << cmd << ": " << e.what();
		mexErrMsgIdAndTxt(error_id.str().c_str(), error_txt.str().c_str());
	}
}


