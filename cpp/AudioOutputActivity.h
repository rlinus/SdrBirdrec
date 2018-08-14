#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>
#include <exception>
#include <stdexcept>
#include <tbb/tbb.h>
#include <portaudio.h>
#include <samplerate.h>

#include "InitParams.h"
#include "SyncedLogger.h"
#include "Types.h"

namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;

	class AudioOutputActivitiy
	{
	private:
		double outputSampleRate;
		size_t channelCount;
		int deviceIndex;
		const PaSampleFormat sampleFormat = paFloat32;
		PaStream *stream = nullptr;
		tbb::concurrent_queue<shared_ptr<vector<float>>> queue;
		shared_ptr<vector<float>> currentBuffer = nullptr;
		size_t currentBufferPos = 0;
		size_t currentBufferSize = 0;

		SRC_STATE *src_state;
		SRC_DATA src_data;
	public:
		AudioOutputActivitiy(size_t channelCount = 1, int deviceIndex = -1) :
			channelCount{ channelCount },
			deviceIndex{ deviceIndex }
		{
#ifdef VERBOSE
			std::cout << "AudioOutputActivitiy()" << endl;
#endif
			PaError err;

			err = Pa_Initialize();
			if(err != paNoError) throw runtime_error("AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));

			if(deviceIndex < 0)
			{
				deviceIndex = Pa_GetDefaultOutputDevice();
				if(deviceIndex == paNoDevice) throw runtime_error("AudioOutputActivitiy: No default audio output device available");
			}
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if(deviceInfo == NULL) throw runtime_error("AudioOutputActivitiy: deviceIndex out of range");

			outputSampleRate = deviceInfo->defaultSampleRate; //set output SR to default SR of device

			PaStreamParameters outputParameters;
			outputParameters.channelCount = channelCount;
			outputParameters.device = deviceIndex;
			outputParameters.hostApiSpecificStreamInfo = NULL;
			outputParameters.sampleFormat = sampleFormat;
			outputParameters.suggestedLatency = deviceInfo->defaultHighOutputLatency;

			err = Pa_IsFormatSupported(NULL, &outputParameters, outputSampleRate);
			if(err != paFormatIsSupported) throw runtime_error("AudioOutputActivitiy: PortAudio stream format error: " + string(Pa_GetErrorText(err)));

			err = Pa_OpenStream(&stream, NULL, &outputParameters, outputSampleRate, paFramesPerBufferUnspecified, paNoFlag, audioOutputCallback, (void *)this);
			if(err != paNoError) throw runtime_error("AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));

			err = Pa_StartStream(stream);
			if(err != paNoError) throw runtime_error("AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));

			//init libsamplerate resampler
			int error;
			src_state = src_new(SRC_SINC_MEDIUM_QUALITY, channelCount, &error);
			if(src_state == NULL) throw runtime_error("AudioOutputActivitiy: libsamplerate error: Error in libsamplerate object construction");
			src_data.end_of_input = 0;
		}

		~AudioOutputActivitiy()
		{
			PaError err;
			err = Pa_AbortStream(stream);
			if(err != paNoError) cout << "AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
			err = Pa_CloseStream(stream);
			if(err != paNoError) cout << "AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
			err = Pa_Terminate();
			if(err != paNoError) cout << "AudioOutputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;

			src_delete(src_state);
		}

		double getOutputSampleRate() const { return outputSampleRate; }

		bool isFormatSupported(double sampleRate, size_t channelCount = 1, int deviceIndex = -1) const
		{
			if(deviceIndex < 0)
			{
				deviceIndex = Pa_GetDefaultOutputDevice();
				if(deviceIndex == paNoDevice) throw runtime_error("AudioOutputActivitiy: No default audio output device available");
			}
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if(deviceInfo == NULL) throw runtime_error("AudioOutputActivitiy: deviceIndex out of range");

			PaStreamParameters outputParameters;
			outputParameters.channelCount = channelCount;
			outputParameters.device = deviceIndex;
			outputParameters.hostApiSpecificStreamInfo = NULL;
			outputParameters.sampleFormat = sampleFormat;
			outputParameters.suggestedLatency = deviceInfo->defaultHighOutputLatency;

			return Pa_IsFormatSupported(NULL, &outputParameters, sampleRate) == paFormatIsSupported;
		}

		bool try_put(const shared_ptr<vector<float>> &data)
		{
			queue.push(data);
			return true;
		}

		bool try_put(const shared_ptr<vector<double>> &data)
		{
			auto data2 = make_shared<vector<float>>(data->cbegin(), data->cend());
			queue.push(data2);
			return true;
		}

		bool try_put(const vector<float> &data, double sampleRate)
		{
			src_data.src_ratio = outputSampleRate / sampleRate;
			src_data.input_frames = data.size() / channelCount;
			src_data.data_in = data.data();

			src_data.output_frames = std::ceil(src_data.input_frames*src_data.src_ratio);
			auto dataOut = make_shared<vector<float>>(src_data.output_frames*channelCount);
			src_data.data_out = dataOut->data();

			int error = src_process(src_state, &src_data);
			if(error) throw runtime_error("AudioOutputActivitiy: libsamplerate error: Error in src_process()");
			dataOut->resize(src_data.output_frames_gen*channelCount);
			if(src_data.input_frames_used != src_data.input_frames) cout << "AudioOutputActivitiy: stream discontinuity" << endl;

			queue.push(dataOut);
			return true;
		}

		bool try_put(const vector<double> &data, double sampleRate)
		{
			vector<float> dataf(data.cbegin(), data.cend());
			return try_put(dataf, sampleRate);
		}



	private:
		static int audioOutputCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
		{
			AudioOutputActivitiy *h = (AudioOutputActivitiy*)userData;

			size_t outputBufferPos = 0;
			size_t outputBufferSize = framesPerBuffer * h->channelCount;

			while(outputBufferPos < outputBufferSize)
			{
				if(!h->currentBuffer)
				{
					if(!h->queue.try_pop(h->currentBuffer))
					{
						fill_n((float*)outputBuffer + outputBufferPos, outputBufferSize - outputBufferPos, float(0));
						return 0;
					}

					h->currentBufferPos = 0;
					h->currentBufferSize = h->currentBuffer->size();
				}

				if(h->currentBufferSize - h->currentBufferPos > outputBufferSize - outputBufferPos)
				{
					size_t n = outputBufferSize - outputBufferPos;
					copy_n(h->currentBuffer->begin() + h->currentBufferPos, n, (float*)outputBuffer + outputBufferPos);
					h->currentBufferPos += n;
					return 0;
				}
				else
				{
					size_t n = h->currentBufferSize - h->currentBufferPos;
					copy_n(h->currentBuffer->begin() + h->currentBufferPos, n, (float*)outputBuffer + outputBufferPos);
					outputBufferPos += n;
					h->currentBuffer = nullptr;
				}
			}

			return 0;
		}

	};

}