#pragma once

namespace SdrBirdrec
{
	class AudioInputActivitiy : public sender< shared_ptr<vector<int16_t>> >
	{
	private:
		const InitParams params;
		PaStream *stream;
		const double sampleRate;
		const size_t channelCount;
		const size_t outputFrameSize;
		ObjectPool< vector<int16_t> > bufferPool;
		shared_ptr< vector<int16_t> > currentBuffer = nullptr;
		size_t currentBufferPos;

		successor_type* successor = nullptr;
		PaTime refTime;
	public:
		AudioInputActivitiy(const InitParams &params) :
			params{ params },
			sampleRate{ double(params.AudioInput_SampleRate) },
			channelCount{ params.AudioInput_ChannelCount },
			outputFrameSize{ size_t(sampleRate / 10) },
			bufferPool(100, vector<int16_t>(outputFrameSize*channelCount))
		{
			if(channelCount == 0) return;
			int deviceIndex = params.AudioInput_DeviceIndex < 0 ? Pa_GetDefaultInputDevice() : params.AudioInput_DeviceIndex;
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);

			if(params.AudioInput_ChannelCount > deviceInfo->maxInputChannels) throw invalid_argument("AudioInputActivitiy: The selected Audio input device doesn't support the requested number of channels");


			PaError err;
			PaStreamParameters inputParameters;
			inputParameters.channelCount = channelCount;
			inputParameters.device = deviceIndex;
			inputParameters.hostApiSpecificStreamInfo = NULL;
			inputParameters.sampleFormat = paInt16;
			inputParameters.suggestedLatency = deviceInfo->defaultHighInputLatency;

			err = Pa_IsFormatSupported(&inputParameters, NULL, sampleRate);
			if(err != paFormatIsSupported) throw runtime_error("AudioInputActivitiy: PortAudio stream format error: " + string(Pa_GetErrorText(err)));

			err = Pa_OpenStream(&stream, &inputParameters, NULL, sampleRate, paFramesPerBufferUnspecified, paNoFlag, audioInputCallback, (void *)this);
			if(err != paNoError) throw runtime_error("AudioInputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));
			refTime = Pa_GetStreamTime(stream);
		}

		~AudioInputActivitiy()
		{
			PaError err;
			if(Pa_IsStreamActive(stream) == 1)
			{
				err = Pa_StopStream(stream);
				if(err != paNoError) cout << "AudioInputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
			}
			err = Pa_CloseStream(stream);
			if(err != paNoError) cout << "AudioInputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
		}

		bool register_successor(successor_type &r)
		{
			if(successor)
			{
				return false;
			}
			else
			{
				successor = &r;
				return true;
			}
		}

		bool remove_successor(successor_type &r)
		{
			if(Pa_IsStreamActive(stream) == 1 || successor != &r) return false;
			successor = nullptr;
			return true;
		}

		void activate()
		{
			if(Pa_IsStreamActive(stream) == 1) throw logic_error("AudioInputActivitiy: Can't activate stream, because it's already active.");
			currentBuffer = bufferPool.acquire();
			if(!currentBuffer) throw runtime_error("AudioInputActivitiy: Can't activate stream, because no buffer is available.");
			currentBufferPos = 0;

			PaError err = Pa_StartStream(stream);
			if(err != paNoError) throw runtime_error("AudioInputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)));
		}

		void deactivate()
		{
			if(Pa_IsStreamActive(stream) != 1) throw logic_error("AudioInputActivitiy: Can't deactivate stream, because it's not active.");
			PaError err = Pa_StopStream(stream);
			if(err != paNoError) cout << "AudioInputActivitiy: PortAudio error: " + string(Pa_GetErrorText(err)) << endl;
			currentBuffer = nullptr;
		}
	private:
		static int audioInputCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
		{
			AudioInputActivitiy *h = (AudioInputActivitiy*)userData;

			size_t inputBufferPos = 0;
			size_t inputBufferSize = framesPerBuffer*h->channelCount;

			while(inputBufferPos < inputBufferSize)
			{
				if(h->currentBuffer->size() - h->currentBufferPos <= inputBufferSize - inputBufferPos)
				{
					copy_n((int16_t*)inputBuffer + inputBufferPos, h->currentBuffer->size() - h->currentBufferPos, h->currentBuffer->begin() + h->currentBufferPos);
					inputBufferPos += h->currentBuffer->size() - h->currentBufferPos;

					if(!h->successor->try_put(h->currentBuffer)) throw runtime_error("AudioInputActivitiy: successor did not accept item");
					h->currentBufferPos = 0;
					h->currentBuffer = nullptr;
					while(!h->currentBuffer) h->currentBuffer = h->bufferPool.acquire();
					h->currentBuffer->resize(h->outputFrameSize*h->channelCount);
				}
				else
				{
					copy_n((int16_t*)inputBuffer + inputBufferPos, inputBufferSize - inputBufferPos, h->currentBuffer->begin() + h->currentBufferPos);
					h->currentBufferPos += inputBufferSize - inputBufferPos;
					inputBufferPos = inputBufferSize;
				}

			}

			return 0;
		}
	};

	struct ddc
	{
	private:
		const InitParams params;
		const size_t nch; //number of channels
		const vector<dsp_t> h1; //DDC FIR filter impulse response
		const vector<dsp_t> h2;
		const size_t P1; //length of h1
		const size_t P2; //length of h2
		const size_t D1; //Decimation factor
		const size_t D2;
		const size_t F1; //input frameSize
		const size_t F2; //intermediate frameSize
		const size_t F3; //output frameSize
		const size_t L1; //Decimator1_InputFrameSize
		const size_t L2; //Decimator1_InputFrameSize
		const size_t W1; //no. Decimator1 input frames per sdr frame
		const size_t W2; //no. Decimator2 input frames per sdr frame
		const size_t N1; //FFT1 size
		const size_t N2; //FFT2 size
		const size_t V1; //Overlap factor, the ration of FFT length to filter transient length

		vector<complex<dsp_t>> H1; //N1 point FFT of h1
		vector<complex<dsp_t>> H2; //(N2/2+1) point positive half FFT of h2

		vector<vector<size_t>> ChannelBands_bins; //list of carrier frequencies ranges measured in bins
		const size_t CarrierTolerance_bins;

		//using aligned_cvec = fftwcpp::aligned_vector<complex<dsp_t>>;
		//using aligned_rvec = fftwcpp::aligned_vector<dsp_t>;
		//aligned_cvec fft1_ibufr;
		//vector<aligned_cvec> fft1_obufr;
		//aligned_cvec ifft1_obufr;
		vector<fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::forward>> fft1;
		vector<fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::inverse>> ifft1;

		//aligned_rvec fft2_ibufr;
		//aligned_cvec fft2_obufr;
		//aligned_rvec ifft2_obufr;
		vector<fftwcpp::Fft<dsp_t, fftwcpp::forward>> fft2; //real to complex fft
		vector<fftwcpp::Fft<dsp_t, fftwcpp::inverse>> ifft2;

		vector<DspUtils::Filters::Biquad_IIR<dsp_t>> hp_filters;

		// state variables
		vector<long long> receive_frequencies_shifted_bins; //in range [0, N-1] and neg. freqs. coresponding to 0 - N/2
		vector<dsp_t> fm_demod_state;
		vector< complex<dsp_t> > fir_filter1_state;
		vector < vector<dsp_t> > fir_filter2_state;

		bool sdrBackPressureReportedFlag = false;
	public:
		ddc(const InitParams &params) :
			params{ params },
			nch{ params.SDR_ChannelBands.size() },
			h1(params.Decimator1_FirFilterCoeffs),
			h2(params.Decimator2_FirFilterCoeffs),
			P1{ h1.size() },
			P2{ h2.size() },
			D1{ params.Decimator1_Factor },
			D2{ params.Decimator2_Factor },
			F1{ params.SDR_FrameSize },
			F2{ params.SDR_FrameSize / D1 },
			F3{ params.SDR_FrameSize / (D1*D2) },
			L1{ params.Decimator1_InputFrameSize },
			L2{ params.Decimator2_InputFrameSize },
			W1{ F1 / L1 },
			W2{ F2 / L2 },
			N1{ L1 + P1 - 1 },
			N2{ L2 + P2 - 1 },
			V1{ P1 != 1 ? N1 / (P1 - 1) : 1 },
			//fft1_ibufr(P1 - 1 + params.SDR_FrameSize),
			//fft1_obufr(W1, aligned_cvec(N1)),
			//ifft1_obufr((P1 - 1 + params.SDR_FrameSize) / D1),
			//fft2_ibufr(P2 - 1 + params.SDR_FrameSize / D1),
			//fft2_obufr(N2),
			//ifft2_obufr(N2 / D2),
			fft1(W1, fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::forward>(N1)),
			ifft1(nch, fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::inverse>(N1 / D1)),
			fft2(nch, fftwcpp::Fft<dsp_t, fftwcpp::forward>(N2)),
			ifft2(nch, fftwcpp::Fft<dsp_t, fftwcpp::inverse>(N2 / D2)),
			hp_filters(nch, DspUtils::Filters::Biquad_IIR<dsp_t>(params.IirFilterCoeffs)),
			H1(N1),
			H2(N2 / 2 + 1),
			CarrierTolerance_bins{ N1 / D1 / 4 },
			receive_frequencies_shifted_bins(nch),
			fm_demod_state(nch),
			fir_filter1_state(P1 - 1),
			fir_filter2_state(nch, vector<dsp_t>(P2 - 1))
		{
			if(params.SDR_FrameSize % params.Decimator1_InputFrameSize != 0)
				throw invalid_argument("SDR_FrameSize must be an integer multiple of Decimator1_InputFrameSize");
			if((params.SDR_FrameSize / D1) % params.Decimator2_InputFrameSize != 0)
				throw invalid_argument("SDR_FrameSize/Decimator1_Factor must be an integer multiple of Decimator2_InputFrameSize");
			if((P1 - 1) % D1 != 0) //condition 1 (Borgerding 2006)
				throw invalid_argument("the filter order (h1.size()-1) must be an integer multiple of Decimator1_Factor");
			if(P1 != 1 && N1 % (P1 - 1) != 0) //V1 is integer --> implies condition 2
				throw invalid_argument("Decimator1_InputFrameSize must be an integer multiple of the Decimator1_FirFilter order");

			if((P2 - 1) % D2 != 0) //condition 1 (Borgerding 2006)
				throw exception("the Decimator2_FirFilter order must be an integer multiple of Decimator2_Factor");
			if(N2 % D2 != 0) //condition 2 
				throw exception("Decimator2_InputFrameSize must be an integer multiple of the filter order Decimator2_Factor");

			//get H1
			for(int j = 0; j < P1; ++j) fft1[0].inBufr[j] = complex<dsp_t>(h1[j]);
			fill(fft1[0].inBufr + P1, fft1[0].inBufr + N1, complex<dsp_t>(0.0)); //zero pad h
			fft1[0].execute();
			copy_n(fft1[0].outBufr, N1, begin(H1));

			fill(fft1[0].inBufr, fft1[0].inBufr + N1, complex<dsp_t>(0.0));

			//get H2
			copy_n(begin(h2), P2, fft2[0].inBufr);
			fill(fft2[0].inBufr + P2, fft2[0].inBufr + N2, dsp_t(0.0)); //zero pad h
			fft2[0].execute();
			copy_n(fft2[0].outBufr, fft2[0].outBufr_size, begin(H2));

			fill(fft2[0].inBufr, fft2[0].inBufr + N2, dsp_t(0.0));

			//get carrier bands
			ChannelBands_bins.reserve(nch);
			for(const auto& i : params.SDR_ChannelBands)
			{
				auto minFreq = (i.minFreq - params.SDR_CenterFrequency) / (params.SDR_SampleRate / 2);
				auto maxFreq = (i.maxFreq - params.SDR_CenterFrequency) / (params.SDR_SampleRate / 2);
				if(minFreq < -1 || maxFreq >= 1 || minFreq > maxFreq) throw invalid_argument("Invalid channel_bands parameter. At least one channel is out of bandwith.");
				ChannelBands_bins.push_back({ size_t((minFreq + 1)*N1 / 2.0), size_t((maxFreq + 1)*N1 / 2.0) + 1 });
			}
		}

		shared_ptr<DataFrame> operator()(const shared_ptr<DataFrame> &frame)
		{
			//cout << "ddc" << endl;
			const dsp_t pi = DspUtils::pi<dsp_t>;

			/*if(sdrBackPressureFlag && !sdrBackPressureReportedFlag)
			{
			cout << "ddc: back_ pressure at frame " << frame->index << endl;
			sdrBackPressureReportedFlag = true;
			}*/

			copy(fft1[W1 - 1].inBufr + N1 - (P1 - 1), fft1[W1 - 1].inBufr + N1, fft1[0].inBufr);
			copy(frame->sdr_signal.cbegin(), frame->sdr_signal.cbegin() + L1, fft1[0].inBufr + (P1 - 1));
			//for(int k = 0; k < W1; ++k)
			//{
			//	if(k != 0) copy_n(frame->sdr_signal.begin() + k*L1 - (P1 - 1), N1, fft1[k].inBufr);
			//	fft1[k].execute();
			//}

			tbb::parallel_for(size_t(0), W1, [&](size_t k)
			{
				if(k != 0) copy_n(frame->sdr_signal.begin() + k*L1 - (P1 - 1), N1, fft1[k].inBufr);
				fft1[k].execute();
			});

			auto&& spectrum = frame->sdr_spectrum;

			//get spectrum from first subframe
			transform(fft1[0].outBufr, fft1[0].outBufr + N1, spectrum.begin(),
				[N1 = N1](const complex<dsp_t> &x) ->dsp_t { return 20.0*log10(std::abs(x) / sqrt(N1)); });


			rotate(begin(spectrum),
				begin(spectrum) + (spectrum.size() + 1) / 2,
				end(spectrum));

			auto get_freq_from_bin = [N = N1, SDR_SampleRate = params.SDR_SampleRate, cf = params.SDR_CenterFrequency](const long long& bin) ->double
			{
				return (bin * 2 / double(N) - 1)*SDR_SampleRate / 2 + cf;
			};

			//for (size_t ch = 0; ch < nch; ++ch)
			tbb::parallel_for(size_t(0), nch, [&](size_t ch)
			{
				//carrier tracking
				auto argmax_it = max_element(spectrum.cbegin() + ChannelBands_bins[ch][0], spectrum.cbegin() + ChannelBands_bins[ch][1]);
				long long argmax = distance(spectrum.cbegin(), argmax_it);
				frame->signal_strengths[ch] = *argmax_it;
				frame->carrier_frequencies[ch] = get_freq_from_bin(argmax);

				if(abs(argmax - receive_frequencies_shifted_bins[ch]) > long long(CarrierTolerance_bins))
				{
					receive_frequencies_shifted_bins[ch] = long long(round((double(argmax) - N1 / 2) / V1)*V1 + N1 / 2);
					if(receive_frequencies_shifted_bins[ch] >= long long(N1))
						receive_frequencies_shifted_bins[ch] = N1 - 1;
					frame->receive_frequencies[ch] = get_freq_from_bin(receive_frequencies_shifted_bins[ch]);
				}
				else
				{
					frame->receive_frequencies[ch] = get_freq_from_bin(receive_frequencies_shifted_bins[ch]);
				}

				auto rec_freq = (receive_frequencies_shifted_bins[ch] - N1 / 2 + N1) % N1;

				// freq. shift and downsampling
				vector<complex<dsp_t>> decimator1_output_frame;
				decimator1_output_frame.reserve(F2);
				for(int k = 0; k < W1; ++k)
				{
					// rotate (frequency shift) spectrum
					vector<complex<dsp_t>> rotatated_spectrum;
					rotatated_spectrum.reserve(N1);
					rotate_copy(fft1[k].outBufr, fft1[k].outBufr + rec_freq, fft1[k].outBufr + N1, back_inserter(rotatated_spectrum));

					// aliase (downsample) spectrum
					fill(ifft1[ch].inBufr, ifft1[ch].inBufr + (N1 / D1), complex<dsp_t>(0.0));
					for(int j = 0; j < N1; ++j)
						ifft1[ch].inBufr[j % (N1 / D1)] += rotatated_spectrum[j] * H1[j];

					ifft1[ch].execute();
					copy_n(ifft1[ch].outBufr + (P1 - 1) / D1, L1 / D1, back_inserter(decimator1_output_frame));
				}

				// fm demodulation
				vector<dsp_t> phase(F2);
				//phase.reserve(F2);
				transform(decimator1_output_frame.begin(), decimator1_output_frame.end(), phase.begin(),
					[&pi](const complex<dsp_t> &x)->dsp_t { return arg(x) / pi; });

				vector<dsp_t> decimator2_input_frame(P2 - 1 + F2);
				copy_n(fir_filter2_state[ch].begin(), P2 - 1, decimator2_input_frame.begin());

				//differentiate
				adjacent_difference(phase.cbegin(), phase.cend(), decimator2_input_frame.begin() + (P2 - 1));
				auto newdata_begin = decimator2_input_frame.begin() + P2 - 1;
				*newdata_begin -= fm_demod_state[ch];
				fm_demod_state[ch] = phase[F2 - 1];

				//phase unwrap
				for_each(newdata_begin, decimator2_input_frame.end(), [](dsp_t &x) {
					if(x > 1)
						x -= 2;
					else if(x < -1)
						x += 2;
				});

				//// fm demodulation
				//vector<dsp_t> decimator2_input_frame(P2 - 1 + F2);
				//for (size_t j = 0; j < decimator1_output_frame.size_(); ++j)
				//{
				//	auto&& y = decimator2_input_frame[P2 - 1 + j];
				//	dsp_t p = arg(decimator1_output_frame[j]) / pi;
				//	y = p - fm_demod_state[ch];
				//	fm_demod_state[ch] = p;

				//	if (y > 1)
				//		y -= 2;
				//	else if (y < -1)
				//		y += 2;
				//}

				copy_n(decimator2_input_frame.end() - (P2 - 1), P2 - 1, fir_filter2_state[ch].begin());

				for(int k = 0; k < W2; ++k)
				{
					copy_n(decimator2_input_frame.begin() + k*L2, N2, fft2[ch].inBufr);
					fft2[ch].execute();

					//multiply with H2
					vector<complex<dsp_t>> full_spectrum(N2);
					for(int j = 0; j < fft2[ch].outBufr_size; ++j)
					{
						full_spectrum[j] = fft2[ch].outBufr[j] * H2[j];
					}

					//get full spectrum of size_ N2
					for(int j = 1; j < (N2 + 1) / 2; ++j)
					{
						full_spectrum[N2 - j] = conj(full_spectrum[j]);
					}
					//aliase spectrum
					for(int j = N2 / D2; j < N2; ++j)
					{
						full_spectrum[j % (N2 / D2)] += full_spectrum[j];
					}

					copy_n(full_spectrum.begin(), ifft2[ch].inBufr_size, ifft2[ch].inBufr);
					ifft2[ch].execute();
					auto output_frame = &ifft2[ch].outBufr[(P2 - 1) / D2];

					// highpass filter signal
					for(int j = 0; j < L2 / D2; ++j)
					{
						frame->demodulated_signals[ch][k*(L2 / D2) + j] = hp_filters[ch].process(output_frame[j] / N2) * dsp_t(params.SdrChannels_AudioGain); //hp filter and fft normalisation
					}
				}



			});
			//}

			return frame;
		}
	};

	struct audio_file_writer
	{
	private:
		int sampleRate;
		int channelCount;
		string filename;
		SndfileHandle fileHandle;

	public:
		audio_file_writer(int sampleRate, int channelCount, string filename) :
			sampleRate{ sampleRate },
			channelCount{ channelCount },
			filename{ filename },
			fileHandle{ filename, SFM_WRITE, SF_FORMAT_W64 | SF_FORMAT_PCM_16, channelCount, sampleRate }
		{
		}

		continue_msg operator()(const shared_ptr<const vector<double>> &input)
		{
			fileHandle.write(input->data(), input->size());
			return continue_msg();
		}

		continue_msg operator()(const shared_ptr<const vector<int16_t>> &input)
		{
			//cout << "s: " << input->size_() << endl;
			fileHandle.write(input->data(), input->size());
			return continue_msg();
		}
	};

	//template<typename T = int>
	//struct frame_resizer
	//{
	//public:
	//	//using T = int;
	//private:
	//	size_t outputFrameSize;
	//	ObjectPool<vector<T>> bufferPool;
	//public:
	//	using node_t = multifunction_node< shared_ptr<vector<T>>, tuple<shared_ptr<vector<T>>> >;

	//	frame_resizer(size_t outputFrameSize, size_t bufferCount) :
	//		outputFrameSize{ outputFrameSize },
	//		bufferPool{ bufferCount, vector<T>(outputFrameSize), false }
	//	{}

	//	void operator()(const shared_ptr<vector<T>> &input, typename node_t::output_ports_type &op)
	//	{
	//		shared_ptr<vector<T>> frame = bufferPool.acquire();


	//		get<0>(op).try_put(frame);
	//	}
	//};
}