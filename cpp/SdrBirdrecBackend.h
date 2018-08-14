#pragma once
#include <memory>
#include <string>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <portaudio.h>

#include "Topology.h"
#include "SdrDataFrame.h"
#include "MonitorDataFrame.h"
#include "InitParams.h"

namespace SdrBirdrec
{
	using namespace std;

	class SdrBirdrecBackend
	{
	public:
		//SdrBirdrecBackend()
		//{
		//	//sdr_device.setupRxStream(std::is_same<SdrBirdrec::dsp_t, double>::value ? std::string(SOAPY_SDR_CF64) : std::string(SOAPY_SDR_CF32));
		//}

		static std::vector<Kwargs> findSdrDevices(const Kwargs & args = Kwargs());
		static std::vector<Kwargs> findAudioInputDevices();
		static std::vector<Kwargs> findAudioOutputDevices();
		static std::vector<Kwargs> findNIDAQmxDevices();


		void initStream(const InitParams &params)
		{
#ifdef VERBOSE
			std::cout << "SdrBirdrecBackend::initStream()" << endl;
#endif
			if(topology && topology->isStreamActive()) throw runtime_error("SdrBirdrec: Stream is active. Call stopStream before reinitalisation.");
			topology = std::make_unique<Topology>(params);
		}

		void startStream()
		{ 
			if(!topology) throw runtime_error("SdrBirdrec: Stream is not initalized.");
			if(topology->isStreamActive()) throw runtime_error("SdrBirdrec: Stream is already active.");
#ifdef VERBOSE
			std::cout << "SdrBirdrecBackend::startStream()" << endl;
#endif
			topology->activate(); 
		}

		void stopStream()
		{
			if(!topology) throw runtime_error("Stream is not running");
			topology = nullptr;
		}

		shared_ptr<MonitorDataFrame> getMonitorDataFrame()
		{
			if(!topology || !topology->isStreamActive()) throw runtime_error("Stream is not active. Can't get data.");
			return topology->getMonitorDataFrame();
		}

		void setSquelch(double squelch)
		{
			if (!topology) throw runtime_error("Stream is not initialised. Can't set option.");
			Kwargs args = { {"squelch"s, to_string(squelch)} };
			topology->setMonitorOptions(args);
		}

		void setChannel(string channel_type, size_t channel_number)
		{
			if(!topology) throw runtime_error("Stream is not initialised. Can't set option.");
			Kwargs args = { { "channel_number"s, to_string(channel_number) }, { "channel_type"s, channel_type } };
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

		std::vector<std::complex<dsp_t>> test(const Kwargs &args = Kwargs())
		{
			int flags = 0;
			long long timeNs = 0;
			int nread = 0;

			SoapyDevice sdr_device(args);
			sdr_device.setupRxStream(std::is_same<SdrBirdrec::dsp_t, double>::value ? std::string(SOAPY_SDR_CF64) : std::string(SOAPY_SDR_CF32));
			size_t streamMTUsize = sdr_device.getRxStreamMTU();
			std::vector<std::complex<dsp_t>> buffer(streamMTUsize);
			void* buf_ptr = (void*)buffer.data();

			sdr_device.activateRxStream();
			nread = sdr_device.readStream(&buf_ptr, streamMTUsize, flags, timeNs);
			cout << "nread: " << nread << endl;

			nread = sdr_device.readStream(&buf_ptr, streamMTUsize, flags, timeNs);
			cout << "nread: " << nread << endl;
			sdr_device.deactivateRxStream();
			sdr_device.closeRxStream();
			sdr_device.setupRxStream(std::is_same<SdrBirdrec::dsp_t, double>::value ? std::string(SOAPY_SDR_CF64) : std::string(SOAPY_SDR_CF32));

			sdr_device.activateRxStream();
			nread = sdr_device.readStream(&buf_ptr, streamMTUsize, flags, timeNs);
			cout << "nread: " << nread << endl;

			nread = sdr_device.readStream(&buf_ptr, streamMTUsize, flags, timeNs);
			cout << "nread: " << nread << endl;
			sdr_device.deactivateRxStream();
			//return sdr_device.handle->listSampleRates(SOAPY_SDR_RX, 0);	
			return buffer;
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
		std::unique_ptr<Topology> topology;
	};
}

