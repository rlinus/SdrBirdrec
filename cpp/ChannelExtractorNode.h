#pragma once
#include <string>
#include <memory>
#include <iostream>
#include <stdexcept>

#include <tbb/tbb.h>

#include "fftwcpp.h"
#include "Types.h"
#include "SdrDataFrame.h"
#include "InitParams.h"
#include "BiquadFilter.h"
#include "FIRFilter.h"

namespace SdrBirdrec
{
	using namespace std;
	using namespace tbb::flow;


	/**
	* This processing node extracts the backpack channels from the SDR signal with the method presented in Borgerding2006
	*
	* Reference for the overlap-save DDC method:
	*	Mark Borgerding, 2006, Turning Overlap-Save into a Multiband Mixing, Downsampling Filter Bank
	*/
	class ChannelExtractorNode : public function_node<shared_ptr<SdrDataFrame>, shared_ptr<SdrDataFrame>>
	{
	private:
		SyncedLogger & logger;
		const InitParams params;

		const size_t W1; //no. Decimator1 input frames per sdr frame
		const size_t W2; //no. Decimator2 input frames per sdr frame

		const size_t V1; //Overlap factor, the ratio of FFT length to filter transient length

		vector<complex<dsp_t>> H1; //N1 point FFT of h1
		vector<complex<dsp_t>> H2; //(N2/2+1) point positive half FFT of h2

		vector<vector<size_t>> ChannelBands_bins; //list of carrier frequencies ranges measured in bins
		const size_t FreqTreckingThreshold_bins;

		vector<fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::forward>> fft1;
		vector<fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::inverse>> ifft1;

		vector<fftwcpp::Fft<dsp_t, fftwcpp::forward>> fft2; //real to complex fft
		vector<fftwcpp::Fft<dsp_t, fftwcpp::inverse>> ifft2;

		vector<BiquadFilter<dsp_t>> hp_filters;
		FIRFilter<dsp_t> noise_level_filter;

		// state variables
		size_t frame_ctr = 0;
		vector<vector<dsp_t>> spectrums;
		vector<dsp_t> fm_demod_state;

		vector<long long> receive_frequencies_shifted_bins; //in range [0, N-1] and neg. freqs. coresponding to 0 - N/2
		vector<vector<size_t>> receive_frequencies_bins; //in range [0, N-1] and neg. freqs. coresponding to N/2+1 - N-1


		dsp_t get_freq_from_bin(const long long& bin) { return (bin * 2 / double(params.Decimator1_Nfft) - 1)*params.SDR_SampleRate / 2 + params.SDR_CenterFrequency; }

		struct f_body
		{
		private:
			ChannelExtractorNode &h;
			const dsp_t pi = dsp_t(3.1415926535897932385);
		public:
			f_body(ChannelExtractorNode &h) : h{ h } {}
			shared_ptr<SdrDataFrame> operator()(const shared_ptr<SdrDataFrame> &frame)
			{
				tbb::parallel_for(size_t(0), h.W1, [&](size_t k)
				{
					if(k == 0)
					{
						copy_n(frame->sdr_signal.cbegin(), h.params.Decimator1_InputFrameSize, h.fft1[k].inBufr + h.params.Decimator1_FilterOrder);
						h.fft1[k].execute();
						copy_n(frame->sdr_signal.cend() - h.params.Decimator1_FilterOrder, h.params.Decimator1_FilterOrder, h.fft1[k].inBufr);
					}
					else
					{
						copy_n(frame->sdr_signal.begin() + k*h.params.Decimator1_InputFrameSize - h.params.Decimator1_FilterOrder, h.params.Decimator1_Nfft, h.fft1[k].inBufr);
						h.fft1[k].execute();
					}

					if(k % (h.W1 / h.params.MonitorRateDivisor) == 0)
					{
						auto j = k / (h.W1 / h.params.MonitorRateDivisor);

						// get spectrum
						for(int i = 0; i < h.params.Decimator1_Nfft; ++i)
						{
							h.spectrums[j][i] = 20 * log10(abs(h.fft1[k].outBufr[i]) / sqrt(h.params.Decimator1_Nfft));
						}
						rotate(h.spectrums[j].begin(), h.spectrums[j].begin() + (h.spectrums[j].size() + 1) / 2, h.spectrums[j].end());

						// carrier tracking
						for(size_t ch = 0; ch < h.params.SDR_ChannelCount; ++ch)
						{
							auto argmax_it = max_element(h.spectrums[j].cbegin() + h.ChannelBands_bins[ch][0], h.spectrums[j].cbegin() + h.ChannelBands_bins[ch][1]);
							long long argmax = distance(h.spectrums[j].cbegin(), argmax_it);
							frame->signal_strengths[j*h.params.SDR_ChannelCount + ch] = *argmax_it;
							frame->carrier_frequencies[j*h.params.SDR_ChannelCount + ch] = h.get_freq_from_bin(argmax);

							if(abs(argmax - h.receive_frequencies_shifted_bins[ch]) > long long(h.FreqTreckingThreshold_bins))
							{
								h.receive_frequencies_shifted_bins[ch] = long long(round((double(argmax) - h.params.Decimator1_Nfft / 2) / h.V1)*h.V1 + h.params.Decimator1_Nfft / 2);
								if(h.receive_frequencies_shifted_bins[ch] >= long long(h.params.Decimator1_Nfft)) h.receive_frequencies_shifted_bins[ch] = h.params.Decimator1_Nfft - 1;
								frame->receive_frequencies[j*h.params.SDR_ChannelCount + ch] = h.get_freq_from_bin(h.receive_frequencies_shifted_bins[ch]);
							}
							else
							{
								frame->receive_frequencies[j*h.params.SDR_ChannelCount + ch] = h.get_freq_from_bin(h.receive_frequencies_shifted_bins[ch]);
							}

							h.receive_frequencies_bins[ch][j] = (h.receive_frequencies_shifted_bins[ch] - h.params.Decimator1_Nfft / 2 + h.params.Decimator1_Nfft) % h.params.Decimator1_Nfft;
						}

						if(k == 0) frame->sdr_spectrum = h.spectrums[k];

						//calc noise level
						std::nth_element(h.spectrums[j].begin(), h.spectrums[j].begin() + h.spectrums[j].size() / 2, h.spectrums[j].end());
						frame->noise_level[j] = h.spectrums[j][h.spectrums[j].size() / 2];
					}

					
				});

				//filter noise level
				for (int j = 0; j < h.params.MonitorRateDivisor; ++j)
				{
					frame->noise_level[j] = h.noise_level_filter.process(frame->noise_level[j]);
					for (size_t ch = 0; ch < h.params.SDR_ChannelCount; ++ch)
					{
						frame->signal_strengths[j * h.params.SDR_ChannelCount + ch] = frame->signal_strengths[j * h.params.SDR_ChannelCount + ch] - frame->noise_level[j];
					}
				}
				

				tbb::parallel_for(size_t(0), h.params.SDR_ChannelCount, [&](size_t ch)
				{
					// freq. shift and downsampling
					vector<complex<dsp_t>> decimator1_output_frame;
					decimator1_output_frame.reserve(h.params.SDR_IntermediateFrameSize);
					for(int k = 0; k < h.W1; ++k)
					{
						auto j = k / (h.W1 / h.params.MonitorRateDivisor);

						// rotate (frequency shift) and aliase (downsample) spectrum
						fill(h.ifft1[ch].inBufr, h.ifft1[ch].inBufr + h.params.Decimator1_Nifft, complex<dsp_t>(0.0));

						for(int m = 0; m < h.params.Decimator1_Nfft; ++m)
							h.ifft1[ch].inBufr[m % h.params.Decimator1_Nifft] += h.fft1[k].outBufr[(m + h.receive_frequencies_bins[ch][j]) % h.params.Decimator1_Nfft] * h.H1[m];

						h.ifft1[ch].execute();
						copy_n(h.ifft1[ch].outBufr + h.params.Decimator1_FilterOrder / h.params.Decimator1_Factor, h.params.Decimator1_InputFrameSize / h.params.Decimator1_Factor, back_inserter(decimator1_output_frame));
					}


					// fm demodulation
					vector<dsp_t> intermediate_frame(h.params.SDR_IntermediateFrameSize);
					for(size_t j = 0; j < h.params.SDR_IntermediateFrameSize; ++j)
					{
						dsp_t p = arg(decimator1_output_frame[j]) / pi;
						intermediate_frame[j] = p - h.fm_demod_state[ch];
						h.fm_demod_state[ch] = p;

						// phase unwrap
						if(intermediate_frame[j] > 1)
							intermediate_frame[j] -= 2;
						else if(intermediate_frame[j] < -1)
							intermediate_frame[j] += 2;
					}

					for(int k = 0; k < h.W2; ++k)
					{
						if(k == 0)
						{
							copy_n(intermediate_frame.cbegin(), h.params.Decimator2_InputFrameSize, h.fft2[ch].inBufr + h.params.Decimator2_FilterOrder);
						}
						else
						{
							copy_n(intermediate_frame.cbegin() + k*h.params.Decimator2_InputFrameSize - h.params.Decimator2_FilterOrder, h.params.Decimator2_Nfft, h.fft2[ch].inBufr);
						}
						h.fft2[ch].execute();

						//multiply with H2
						vector<complex<dsp_t>> full_spectrum(h.params.Decimator2_Nfft);
						for(int j = 0; j < h.fft2[ch].outBufr_size; ++j)
						{
							full_spectrum[j] = h.fft2[ch].outBufr[j] * h.H2[j];
						}

						//get full spectrum of size N2
						for(int j = 1; j < (h.params.Decimator2_Nfft + 1) / 2; ++j)
						{
							full_spectrum[h.params.Decimator2_Nfft - j] = conj(full_spectrum[j]);
						}
						//aliase spectrum
						for(int j = h.params.Decimator2_Nifft; j < h.params.Decimator2_Nfft; ++j)
						{
							full_spectrum[j % h.params.Decimator2_Nifft] += full_spectrum[j];
						}

						copy_n(full_spectrum.begin(), h.ifft2[ch].inBufr_size, h.ifft2[ch].inBufr);
						h.ifft2[ch].execute();
						auto output_frame = &h.ifft2[ch].outBufr[h.params.Decimator2_FilterOrder / h.params.Decimator2_Factor];

						// highpass filter signal
						const size_t Decimator2_OutputFrameSize = h.params.Decimator2_InputFrameSize / h.params.Decimator2_Factor;
						for(int j = 0; j < Decimator2_OutputFrameSize; ++j)
						{
							frame->demodulated_signals[ch][k*Decimator2_OutputFrameSize + j] = h.hp_filters[ch].process(output_frame[j] / h.params.Decimator2_Nfft) * dsp_t(h.params.SdrChannels_AudioGain); //hp filter and fft normalisation
						}
					}

					copy_n(intermediate_frame.cend() - h.params.Decimator2_FilterOrder, h.params.Decimator2_FilterOrder, h.fft2[ch].inBufr);
				});

				++h.frame_ctr;
				return frame;
			}
		};
	public:
		ChannelExtractorNode(graph &g, const InitParams &params, SyncedLogger &logger) :
			function_node<shared_ptr<SdrDataFrame>, shared_ptr<SdrDataFrame>>(g, serial, f_body(*this)),
			params{ params },
			logger{ logger },
			W1{ params.SDR_FrameSize / params.Decimator1_InputFrameSize },
			W2{ params.SDR_IntermediateFrameSize / params.Decimator2_InputFrameSize },
			//V1{ params.Decimator1_Nfft / params.Decimator1_Factor },
			V1{ params.Decimator1_FirFilterCoeffs.size() != 1 ? params.Decimator1_Nfft / (params.Decimator1_FirFilterCoeffs.size()-1) : 1},
			H1(params.Decimator1_Nfft),
			H2(params.Decimator2_Nfft / 2 + 1),
			//FreqTreckingThreshold_bins{ params.Decimator1_Nfft / params.Decimator1_Factor / 4 },
			FreqTreckingThreshold_bins{ static_cast<size_t>(std::floor(params.Decimator1_Nfft * params.FreqTreckingThreshold / params.SDR_SampleRate)) },
			spectrums{ params.MonitorRateDivisor, vector<dsp_t>(params.Decimator1_Nfft) },
			receive_frequencies_shifted_bins(params.SDR_ChannelCount),
			receive_frequencies_bins(params.SDR_ChannelCount, vector<size_t>(params.MonitorRateDivisor)),
			fft1(W1, fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::forward>(params.Decimator1_Nfft)),
			ifft1(params.SDR_ChannelCount, fftwcpp::Fft<std::complex<dsp_t>, fftwcpp::inverse>(params.Decimator1_Nifft)),
			fft2(params.SDR_ChannelCount, fftwcpp::Fft<dsp_t, fftwcpp::forward>(params.Decimator2_Nfft)),
			ifft2(params.SDR_ChannelCount, fftwcpp::Fft<dsp_t, fftwcpp::inverse>(params.Decimator2_Nifft)),
			hp_filters(params.SDR_ChannelCount, BiquadFilter<dsp_t>(params.IirFilterCoeffs)),
			fm_demod_state(params.SDR_ChannelCount),
			noise_level_filter(std::vector<dsp_t>(5*params.SDR_TrackingRate, 1.0 / dsp_t(5*params.SDR_TrackingRate)))
		{
			#ifdef VERBOSE
			std::cout << "ChannelExtractorNode()" << endl;
			#endif

			//get H1
			for(int j = 0; j < params.Decimator1_FirFilterCoeffs.size(); ++j) fft1[0].inBufr[j] = complex<dsp_t>(params.Decimator1_FirFilterCoeffs[j]);
			fill(fft1[0].inBufr + params.Decimator1_FirFilterCoeffs.size(), fft1[0].inBufr + params.Decimator1_Nfft, complex<dsp_t>(0.0)); //zero pad h
			fft1[0].execute();
			copy_n(fft1[0].outBufr, params.Decimator1_Nfft, H1.begin());

			fill(fft1[0].inBufr, fft1[0].inBufr + params.Decimator1_Nfft, complex<dsp_t>(0.0));

			//get H2
			copy_n(params.Decimator2_FirFilterCoeffs.begin(), params.Decimator2_FirFilterCoeffs.size(), fft2[0].inBufr);
			fill(fft2[0].inBufr + params.Decimator2_FirFilterCoeffs.size(), fft2[0].inBufr + params.Decimator2_Nfft, dsp_t(0.0)); //zero pad h

			fft2[0].execute(); 
			copy_n(fft2[0].outBufr, fft2[0].outBufr_size, H2.begin());

			fill(fft2[0].inBufr, fft2[0].inBufr + params.Decimator2_Nfft, dsp_t(0.0));

			//get carrier bands
			ChannelBands_bins.reserve(params.SDR_ChannelCount);
			for(const auto& i : params.SDR_ChannelBands)
			{
				auto minFreq = (i.minFreq - params.SDR_CenterFrequency) / (params.SDR_SampleRate / 2);
				auto maxFreq = (i.maxFreq - params.SDR_CenterFrequency) / (params.SDR_SampleRate / 2);
				if(minFreq < -1 || maxFreq >= 1 || minFreq > maxFreq) throw invalid_argument("Invalid channel_bands parameter. At least one channel is out of bandwith.");
				ChannelBands_bins.push_back({ size_t((minFreq + 1)*params.Decimator1_Nfft / 2.0), size_t((maxFreq + 1)*params.Decimator1_Nfft / 2.0) + 1 });
			}

		}
	};
}
