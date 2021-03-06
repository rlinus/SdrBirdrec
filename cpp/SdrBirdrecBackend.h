#pragma once
#include <memory>
#include <string>
#include <stdexcept>
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

	/*!
	* An object of this class represents a recording. It provides methods to start and control the recording.
	* Additionally, it provides static methods for device discovery. 
	*/
	class SdrBirdrecBackend
	{
	public:
		/*!
		* Enumerate a list of available SDR devices on the system.
		* \param args device construction key/value argument filters
		* \return a list of argument maps, each unique to a device
		*/
		static std::vector<Kwargs> findSdrDevices(const Kwargs & args = Kwargs());

		/*!
		* Enumerate a list of available audio input devices on the system.
		* \return a list of argument maps, each unique to a device
		*/
		static std::vector<Kwargs> findAudioInputDevices();

		/*!
		* Enumerate a list of available audio output devices on the system.
		* \return a list of argument maps, each unique to a device
		*/
		static std::vector<Kwargs> findAudioOutputDevices();

		/*!
		* Enumerate a list of available NIDAQmx devices on the system.
		* \return a list of argument maps, each unique to a device
		*/
		static std::vector<Kwargs> findNIDAQmxDevices();

		/*!
		* Initializes a recording.
		* \param params an InitParams object, which contains all the parameters needed to set up the recording
		*/
		void initRec(const InitParams &params)
		{
			#ifdef VERBOSE
			std::cout << "SdrBirdrecBackend::initRec()" << endl;
			#endif

			if(topology && topology->isStreamActive()) throw runtime_error("SdrBirdrec: A recording is ongoing. Call stopRec() before reinitalisation.");
			topology = std::make_unique<Topology>(params);
		}

		/*!
		* Starts a recording
		*/
		void startRec()
		{ 
			if(!topology) throw runtime_error("SdrBirdrec: Recording is not yet initalized.");
			if(topology->isStreamActive()) throw runtime_error("SdrBirdrec: Recording is already started.");
			#ifdef VERBOSE
			std::cout << "SdrBirdrecBackend::startRec()" << endl;
			#endif
			topology->activate(); 
		}

		/*!
		* Stops a recording
		*/
		void stopRec()
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

		bool isRecording() { return topology && topology->isStreamActive(); };

	private:
		std::unique_ptr<Topology> topology;
	};
}

