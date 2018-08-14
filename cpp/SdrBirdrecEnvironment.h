#pragma once

#include <iostream>
#include <string>
#include <tbb/task_scheduler_init.h>
#include <SoapySDR/Logger.hpp>
#include <Windows.h>
#include <processenv.h>
#include <portaudio.h>
#include "mexUtils.h"

namespace SdrBirdrec
{
	using namespace std;

	struct SdrBirdrecEnvironment
	{
		SdrBirdrecEnvironment();
		~SdrBirdrecEnvironment();
		static void SoapySDRLogHandler(const SoapySDR::LogLevel logLevel, const char *message) { std::cout << message << std::endl; }

		tbb::task_scheduler_init init;
	};

	SdrBirdrecEnvironment::SdrBirdrecEnvironment()
	{
		if(!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
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
		if(err != paNoError) throw runtime_error("PortAudio error: " + string(Pa_GetErrorText(err)));
	}

	SdrBirdrecEnvironment::~SdrBirdrecEnvironment()
	{
		//terminate portaudio
		PaError err = Pa_Terminate();
		if(err != paNoError) cout << "PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
	}
}