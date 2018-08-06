#pragma once
#include <memory>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <tbb/task_scheduler_init.h>
#include <portaudio.h>

#include "Topology.h"
#include "DataFrame.h"
#include "mexUtils/mexUtils.h"
#include <mex.h>
#include "InitParams.h"
#include "SoapyDevice.h"

namespace SdrBirdrec
{
	using namespace std;

	struct SdrBirdrecEnvironment
	{
		SdrBirdrecEnvironment();
		~SdrBirdrecEnvironment();
		static void SoapySDRLogHandler(const SoapySDR::LogLevel logLevel, const char *message) { std::cout << message << std::endl;}

		tbb::task_scheduler_init init;
	};

	class UserInterface
	{
	public:
		UserInterface(const Kwargs & args = Kwargs()):
			args { args }
		{
			//sdr_device.setupRxStream(std::is_same<SdrBirdrec::dsp_t, double>::value ? std::string(SOAPY_SDR_CF64) : std::string(SOAPY_SDR_CF32));
		}

		static std::vector<Kwargs> findSdrDevices(const Kwargs & args = Kwargs()) { return SoapySDR::Device::enumerate(args); }
		static std::vector<Kwargs> findAudioInputDevices();
		static std::vector<Kwargs> findAudioOutputDevices();
		static std::vector<Kwargs> findNIDAQmxDevices();


		void initStream(const InitParams &params)
		{
#ifdef VERBOSE
			std::cout << "UserInterface::initStream()" << endl;
#endif
			if(topology && topology->isStreamActive()) throw runtime_error("SdrBirdrec: Stream is active. Call stopStream before reinitalisation.");
			topology = std::make_unique<Topology>(params, args);
		}

		void startStream()
		{ 
			if(!topology) throw runtime_error("SdrBirdrec: Stream is not initalized.");
			if(topology->isStreamActive()) throw runtime_error("SdrBirdrec: Stream is already active.");
#ifdef VERBOSE
			std::cout << "UserInterface::startStream()" << endl;
#endif
			topology->activate(); 
		}

		void stopStream()
		{
			if(!topology) throw runtime_error("Stream is not running");
			topology = nullptr;
		}

		mxArray* getData()
		{
			if(!topology || !topology->isStreamActive()) throw runtime_error("Stream is not active. Can't get data.");
			shared_ptr<OutputFrame> frame = topology->getData();

			mxArray* m = mexUtils::Cast::toMxStruct(
				"sdr_spectrum", frame->sdr_spectrum,
				"signal_strengths", frame->signal_strengths,
				"carrier_frequencies", frame->carrier_frequencies,
				"receive_frequencies", frame->receive_frequencies,
				"output_signal", frame->output_signal,
				"channel_type", frame->channel_type,
				"channel_number", frame->channel_number
			);

			return m;
		}

		void setSquelch(double squelch)
		{
			if (!topology) throw runtime_error("Stream is not initialised. Can't set option.");
			Kwargs args = { {"squelch"s, to_string(squelch)} };
			topology->setMonitorOptions(args);
		}

		void setChannelNumber(size_t channel_number)
		{
			if (!topology) throw runtime_error("Stream is not initialised. Can't set option.");
			Kwargs args = { { "channel_number"s, to_string(channel_number) } };
			topology->setMonitorOptions(args);
		}

		void setChannelType(string channel_type)
		{
			if(!topology) throw runtime_error("Stream is not initialised. Can't set option.");
			Kwargs args = { { "channel_type"s, channel_type } };
			topology->setMonitorOptions(args);
		}

		void setPlayAudio(bool play_audio)
		{
			if (!topology) throw runtime_error("Stream is not initialised. Can't set option.");
			Kwargs args = { { "play_audio"s, to_string(int(play_audio)) } };
			topology->setMonitorOptions(args);
		}

		bool isStreaming() { return topology && topology->isStreamActive(); };

		std::vector<double> test()
		{
			SoapyDevice sdr_device(args);
			sdr_device.setupRxStream(std::is_same<SdrBirdrec::dsp_t, double>::value ? std::string(SOAPY_SDR_CF64) : std::string(SOAPY_SDR_CF32));
			sdr_device.activateRxStream();
			sdr_device.deactivateRxStream();
			//return sdr_device.handle->listSampleRates(SOAPY_SDR_RX, 0);	
			return {};
		}

		bool isRefPLLlocked()
		{
			return topology->isRefPLLlocked();
		}

		static int staticTest()
		{
			return 0;
		}


	private:
		Kwargs args;
		//SoapyDevice sdr_device;

		std::unique_ptr<Topology> topology;
	};
}

