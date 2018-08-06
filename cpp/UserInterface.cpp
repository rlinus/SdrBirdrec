#include "stdafx.h"
#include "UserInterface.h"

namespace SdrBirdrec
{
	SdrBirdrecEnvironment::SdrBirdrecEnvironment()
	{
		if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		{
			cout << "Process priority couldn't be set" << endl;
		}

		//HMODULE hModule = GetModuleHandle(NULL);
		//char path[MAX_PATH];
		//GetModuleFileNameA(hModule, path, MAX_PATH);
		//std::string ws(path);
		//std::cout << "name: " << ws << endl;
		//std::cout << "name: " << std::string(ws.begin(), ws.end()) << endl;

		//GetEnvironmentVariableA("SOAPY_SDR_ROOT", path, MAX_PATH);
		//cout << string(path) << endl;

		string mex_dir = mexUtils::getMexFilePath();
		string uhd_images_dir = mex_dir + "\\SoapySdr\\images"s;
		string soapy_sdr_modules_dir = mex_dir + "\\SoapySdr\\modules"s;

		SetEnvironmentVariableA("UHD_IMAGES_DIR", uhd_images_dir.c_str());
		//SetEnvironmentVariableA("SOAPY_SDR_ROOT", "");
		SetEnvironmentVariableA("SOAPY_SDR_ROOT", mex_dir.c_str());
		SetEnvironmentVariableA("SOAPY_SDR_PLUGIN_PATH", soapy_sdr_modules_dir.c_str());

		//SetEnvironmentVariableA("UHD_PKG_PATH", mex_dir.c_str());
		SetEnvironmentVariableA("UHD_PKG_PATH", "");

		SetEnvironmentVariableA("PATH", mex_dir.c_str());

		////set UHD_IMAGES_DIR as user environment variable
		//LSTATUS result;
		//HKEY hkey;
		//result = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hkey);
		//result = RegSetValueExA(hkey, "UHD_IMAGES_DIR", 0, REG_SZ, (BYTE*)uhd_images_dir.c_str(), DWORD(uhd_images_dir.length()+1));
		//result = RegCloseKey(hkey);
		//const char msg[] = "Environment";
		//SendMessageA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)msg);

		SoapySDR::registerLogHandler(SoapySDRLogHandler);

		//vector<string> list = SoapySDR::listModules(mex_dir);
		//for(auto& mod : list)
		//{
		//	cout << "list: " << mod << endl;
		//}

		//vector<string> modules = { mex_dir + "\\rtlsdrSupport.dll", mex_dir + "\\uhdSupport.dll" };
		//for(auto& mod : modules)
		//{
		//	cout << "load: " << mod << endl;
		//	auto output = SoapySDR::loadModule(mod);
		//	if(!output.empty()) cout << output << endl;
		//}

		//SoapySDR::loadModules();

		//init portaudio
		PaError err = Pa_Initialize();
		if (err != paNoError) throw runtime_error("PortAudio error: " + string(Pa_GetErrorText(err)));
	}

	SdrBirdrecEnvironment::~SdrBirdrecEnvironment()
	{
		//terminate portaudio
		PaError err = Pa_Terminate();
		if (err != paNoError) cout << "PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
	}


	std::vector<Kwargs> UserInterface::findAudioInputDevices()
	{
		PaError err = Pa_Initialize();
		if(err != paNoError) throw runtime_error("PortAudio error: " + string(Pa_GetErrorText(err)));

		std::vector<Kwargs> list;

		int numDevices = Pa_GetDeviceCount();
		int defaultDevice = Pa_GetDefaultInputDevice();
		const PaDeviceInfo *deviceInfo;
		const PaHostApiInfo* hostApiInfo;
		for (int i = 0; i<numDevices; i++)
		{
			deviceInfo = Pa_GetDeviceInfo(i);
			hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
			if (deviceInfo->maxInputChannels == 0) continue;

			Kwargs device;
			device["deviceIndex"s] = to_string(i);
			device["name"s] = deviceInfo->name;
			device["hostApiName"s] = hostApiInfo->name;
			device["maxInputChannels"s] = to_string(deviceInfo->maxInputChannels);
			device["defaultLowInputLatency"s] = to_string(deviceInfo->defaultLowInputLatency*1e3) + "ms"s;
			device["defaultHighInputLatency"s] = to_string(deviceInfo->defaultHighInputLatency*1e3) + "ms"s;
			device["defaultSampleRate"s] = to_string(deviceInfo->defaultSampleRate) + "Hz"s;
			device["defaultDevice"s] = to_string(i == defaultDevice);
			list.push_back(device);
		}

		err = Pa_Terminate();
		if(err != paNoError) cout << "PortAudio error: " + string(Pa_GetErrorText(err)) << endl;

		return list;
	}

	std::vector<Kwargs> UserInterface::findAudioOutputDevices()
	{
		PaError err = Pa_Initialize();
		if(err != paNoError) throw runtime_error("PortAudio error: " + string(Pa_GetErrorText(err)));

		std::vector<Kwargs> list;

		int numDevices = Pa_GetDeviceCount();
		int defaultDevice = Pa_GetDefaultOutputDevice();
		const PaDeviceInfo *deviceInfo;
		const PaHostApiInfo* hostApiInfo;
		for (int i = 0; i<numDevices; i++)
		{
			deviceInfo = Pa_GetDeviceInfo(i);
			hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
			if (deviceInfo->maxOutputChannels == 0) continue;

			Kwargs device;
			device["deviceIndex"s] = to_string(i);
			device["name"s] = deviceInfo->name;
			device["hostApiName"s] = hostApiInfo->name;
			device["maxOutputChannels"s] = to_string(deviceInfo->maxOutputChannels);
			device["defaultLowOutputLatency"s] = to_string(deviceInfo->defaultLowOutputLatency*1e3) + "ms"s;
			device["defaultHighOutputLatency"s] = to_string(deviceInfo->defaultHighOutputLatency*1e3) + "ms"s;
			device["defaultSampleRate"s] = to_string(deviceInfo->defaultSampleRate) + "Hz"s;
			device["defaultDevice"s] = to_string(i == defaultDevice);
			list.push_back(device);
		}

		err = Pa_Terminate();
		if(err != paNoError) cout << "PortAudio error: " + string(Pa_GetErrorText(err)) << endl;

		return list;
	}

	std::vector<Kwargs> UserInterface::findNIDAQmxDevices()
	{
		std::vector<Kwargs> list;
		int bufferSize = DAQmxGetSysDevNames(NULL, 0);
		char *names = new char[bufferSize];
		DAQmxGetSysDevNames(names, bufferSize);
		list.push_back({ { "names"s, string(names) } });
		return list;
	}
}
