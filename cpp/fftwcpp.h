#pragma once
#include <fftw3.h>
#include <complex>

/**
* C++11 wrapper for fftw
*
* fft is not normalized, hence ifft(fft(x))=N*x
*/
namespace fftwcpp
{
	using namespace std;

	enum Direction : int
	{
		forward = FFTW_FORWARD,
		inverse = FFTW_BACKWARD
	};

	template <typename T, Direction direction>
	class Fft
	{
		Fft() { static_assert(false, "fftwcpp::Fft: Fft not defined for this type!"); }
	};

	// real double forward
	template <>
	class Fft<double, forward>
	{
	public:
		Fft(size_t numBins) :
			numBins(numBins),
			inBufr_size(numBins),
			outBufr_size(numBins / 2 + 1),
			inBufr((double *)fftw_malloc(numBins * sizeof(double))),
			outBufr((std::complex<double> *) fftw_malloc(outBufr_size * sizeof(std::complex<double>)))
		{
			if(inBufr == nullptr || outBufr == nullptr) throw("fftw_malloc: Bad allocation");
			fft_plan = fftw_plan_dft_r2c_1d((int)numBins, (double*)inBufr, (fftw_complex *)outBufr, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
		}

		Fft(const Fft<double, Direction::forward>& fft) : Fft(fft.numBins)
		{
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
		}

		Fft & operator=(const Fft<double, forward>& fft)
		{
			if(fft.numBins != numBins) throw("can not assign Fft objects of not matching numBins");
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
			return *this;
		}

		~Fft()
		{
			fftw_destroy_plan(fft_plan);
			fftw_free(inBufr);
			fftw_free(outBufr);
		}

		void inline execute(const double *in, std::complex<double> *out) const
		{
			fftw_plan tplan = fftw_plan_dft_r2c_1d((int)numBins, (double*)in, (fftw_complex *)out, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
			fftw_execute(tplan);
			fftw_destroy_plan(tplan);
		}

		void inline execute(void)
		{
			fftw_execute(fft_plan);
		}

		const size_t inBufr_size;
		const size_t outBufr_size;

		double *const inBufr;
		complex<double> *const outBufr;

	private:
		size_t numBins;
		fftw_plan fft_plan;
	};

	// real float forward
	template <>
	class Fft<float, forward>
	{
	public:
		Fft(size_t numBins) :
			numBins(numBins),
			inBufr_size(numBins),
			outBufr_size(numBins / 2 + 1),
			inBufr((float *)fftwf_malloc(numBins * sizeof(float))),
			outBufr((std::complex<float> *) fftwf_malloc(outBufr_size * sizeof(std::complex<float>)))
		{
			if(inBufr == nullptr || outBufr == nullptr) throw("fftwf_malloc: Bad allocation");
			fft_plan = fftwf_plan_dft_r2c_1d((int)numBins, (float*)inBufr, (fftwf_complex *)outBufr, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
		}

		Fft(const Fft<float, forward>& fft) : Fft(fft.numBins)
		{
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
		}

		Fft & operator=(const Fft<float, forward>& fft)
		{
			if(fft.numBins != numBins) throw("can not assign Fft objects of not matching numBins");
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
			return *this;
		}

		~Fft()
		{
			fftwf_destroy_plan(fft_plan);
			fftwf_free(inBufr);
			fftwf_free(outBufr);
		}

		void inline execute(const float *in, std::complex<float> *out) const
		{
			fftwf_plan tplan = fftwf_plan_dft_r2c_1d((int)numBins, (float*)in, (fftwf_complex *)out, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
			fftwf_execute(tplan);
			fftwf_destroy_plan(tplan);
		}

		void inline execute(void)
		{
			fftwf_execute(fft_plan);
		}

		const size_t inBufr_size;
		const size_t outBufr_size;

		float *const inBufr;
		complex<float> *const outBufr;

	private:
		size_t numBins;
		fftwf_plan fft_plan;
	};


	//real double backward
	template<>
	class Fft<double, inverse>
	{
	public:
		Fft(size_t numBins) :
			numBins(numBins),
			inBufr_size(numBins / 2 + 1),
			outBufr_size(numBins),
			inBufr((std::complex<double> *) fftw_malloc(inBufr_size * sizeof(std::complex<double>))),
			outBufr((double *)fftw_malloc(outBufr_size * sizeof(double)))
		{
			if(inBufr == nullptr || outBufr == nullptr) throw("fftw_malloc: Bad allocation");
			fft_plan = fftw_plan_dft_c2r_1d((int)numBins, (fftw_complex *)inBufr, (double *)outBufr, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
		}

		Fft(const Fft<double, inverse>& fft) : Fft(fft.numBins)
		{
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
		}

		Fft & operator=(const Fft<double, inverse>& fft)
		{
			if(fft.numBins != numBins) throw("can not assign Fft objects of not matching numBins");
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
			return *this;
		}

		~Fft()
		{
			fftw_destroy_plan(fft_plan);
			fftw_free(inBufr);
			fftw_free(outBufr);
		}

		void inline execute(const std::complex<double> *in, double *out) const
		{
			fftw_plan tplan = fftw_plan_dft_c2r_1d((int)numBins, (fftw_complex *)in, out, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
			fftw_execute(tplan);
			fftw_destroy_plan(tplan);
		}

		void inline execute(void)
		{
			fftw_execute(fft_plan);
		}

		const size_t inBufr_size;
		const size_t outBufr_size;

		complex<double> *const inBufr;
		double *const outBufr;

	private:
		size_t numBins;
		fftw_plan fft_plan;
	};

	//real float backward
	template<>
	class Fft<float, inverse>
	{
	public:
		Fft(size_t numBins) :
			numBins(numBins),
			inBufr_size(numBins / 2 + 1),
			outBufr_size(numBins),
			inBufr((std::complex<float> *) fftwf_malloc(inBufr_size * sizeof(std::complex<float>))),
			outBufr((float *)fftwf_malloc(outBufr_size * sizeof(float)))
		{
			if(inBufr == nullptr || outBufr == nullptr) throw("fftwf_malloc: Bad allocation");
			fft_plan = fftwf_plan_dft_c2r_1d((int)numBins, (fftwf_complex *)inBufr, (float *)outBufr, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
		}

		Fft(const Fft<float, inverse>& fft) : Fft(fft.numBins)
		{
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
		}

		Fft & operator=(const Fft<float, inverse>& fft)
		{
			if(fft.numBins != numBins) throw("can not assign Fft objects of not matching numBins");
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
			return *this;
		}

		~Fft()
		{
			fftwf_destroy_plan(fft_plan);
			fftwf_free(inBufr);
			fftwf_free(outBufr);
		}

		void inline execute(const std::complex<float> *in, float *out) const
		{
			fftwf_plan tplan = fftwf_plan_dft_c2r_1d((int)numBins, (fftwf_complex *)in, out, FFTW_MEASURE|FFTW_PRESERVE_INPUT);
			fftwf_execute(tplan);
			fftwf_destroy_plan(tplan);
		}

		void inline execute(void)
		{
			fftwf_execute(fft_plan);
		}

		const size_t inBufr_size;
		const size_t outBufr_size;

		std::complex<float> *const inBufr;
		float *const outBufr;

	private:
		size_t numBins;
		fftwf_plan fft_plan;
	};

	//complex double
	template <Direction direction>
	class Fft<std::complex<double>, direction>
	{
	public:
		Fft(size_t numBins) :
			numBins(numBins),
			inBufr_size(numBins),
			outBufr_size(numBins),
			inBufr((std::complex<double> *) fftw_malloc(numBins * sizeof(std::complex<double>))),
			outBufr((std::complex<double> *) fftw_malloc(numBins * sizeof(std::complex<double>)))
		{
			if(inBufr == nullptr || outBufr == nullptr) throw("fftw_malloc: Bad allocation");
			fft_plan = fftw_plan_dft_1d(static_cast<int>(numBins), (fftw_complex *)inBufr, (fftw_complex *)outBufr, static_cast<int>(direction), FFTW_MEASURE|FFTW_PRESERVE_INPUT);
		}

		Fft(const Fft<std::complex<double>, direction>& fft) : Fft(fft.numBins)
		{
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
		}

		Fft & operator=(const Fft<std::complex<double>, direction>& fft)
		{
			if(fft.numBins != numBins) throw("can not assign Fft objects of not matching numBins");
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
			return *this;
		}

		~Fft()
		{
			fftw_destroy_plan(fft_plan);
			fftw_free(inBufr);
			fftw_free(outBufr);
		}

		void inline execute(const complex<double> *in, complex<double> *out) const
		{
			fftw_plan tplan = fftw_plan_dft_1d(static_cast<int>(numBins), (fftw_complex *)in, (fftw_complex *)out, static_cast<int>(direction), FFTW_MEASURE|FFTW_PRESERVE_INPUT);
			fftw_execute(tplan);
			fftw_destroy_plan(tplan);
		}

		void inline execute(void)
		{
			fftw_execute(fft_plan);
		}

		const size_t inBufr_size;
		const size_t outBufr_size;

		complex<double> *const inBufr;
		complex<double> *const outBufr;

	private:
		fftw_plan fft_plan;
		size_t numBins;
	};

	//complex float
	template <Direction direction>
	class Fft<std::complex<float>, direction>
	{
	public:
		Fft(size_t numBins) :
			numBins(numBins),
			inBufr_size(numBins),
			outBufr_size(numBins),
			inBufr((std::complex<float> *) fftwf_malloc(numBins * sizeof(std::complex<float>))),
			outBufr((std::complex<float> *) fftwf_malloc(numBins * sizeof(std::complex<float>)))
		{
			if(inBufr == nullptr || outBufr == nullptr) throw("fftwf_malloc: Bad allocation");
			fft_plan = fftwf_plan_dft_1d(static_cast<int>(numBins), (fftwf_complex *)inBufr, (fftwf_complex *)outBufr, static_cast<int>(direction), FFTW_MEASURE|FFTW_PRESERVE_INPUT);
		}

		Fft(const Fft<std::complex<float>, direction>& fft) : Fft(fft.numBins)
		{
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
		}

		Fft & operator=(const Fft<std::complex<float>, direction>& fft)
		{
			if(fft.numBins != numBins) throw("can not assign Fft objects of not matching numBins");
			copy_n(fft.inBufr, inBufr_size, inBufr);
			copy_n(fft.outBufr, outBufr_size, outBufr);
			return *this;
		}

		~Fft()
		{
			fftwf_destroy_plan(fft_plan);
			fftwf_free(inBufr);
			fftwf_free(outBufr);
		}

		void inline execute(const std::complex<float> *in, std::complex<float> *out) const
		{
			fftwf_plan tplan = fftwf_plan_dft_1d(static_cast<int>(numBins), (fftwf_complex *)in, (fftwf_complex *)out, static_cast<int>(direction), FFTW_MEASURE|FFTW_PRESERVE_INPUT);
			fftwf_execute(tplan);
			fftwf_destroy_plan(tplan);
		}

		void inline execute(void)
		{
			fftwf_execute(fft_plan);
		}

		const size_t inBufr_size;
		const size_t outBufr_size;

		complex<float> *const inBufr;
		complex<float> *const outBufr;

	private:
		fftwf_plan fft_plan;
		size_t numBins;
	};
}

