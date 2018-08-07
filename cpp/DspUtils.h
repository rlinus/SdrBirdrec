#pragma once

#include <utility>
#include <cmath>
#include <complex>
#include <vector>
#include <valarray>
#include <limits>

namespace DspUtils
{
	using namespace std;

	// Math constants
	template<typename T>
	constexpr T pi = T(3.1415926535897932385);

	template<typename T>
	constexpr T e = T(2.71828182845904523536);

	// Types
	template<typename T>
	using cvec = std::valarray < std::complex<T> >;

	template<typename T>
	using rvec = std::valarray < T >;

	// Cooley–Tukey FFT (in-place)
	template<typename T>
	void fft_inplace(cvec<T>& x)
	{
		const size_t N = x.size();
		if(N <= 1) return;

		// divide
		cvec<T> even = x[std::slice(0, N / 2, 2)];
		cvec<T>  odd = x[std::slice(1, N / 2, 2)];

		// conquer
		fft_inplace(even);
		fft_inplace(odd);

		// combine
		for(size_t k = 0; k < N / 2; ++k)
		{
			complex<T> t = std::polar(1.0f, -2 * pi<T> * k / N) * odd[k];
			x[k] = even[k] + t;
			x[k + N / 2] = even[k] - t;
		}
	}

	// inverse fft (in-place)
	template<typename T>
	void ifft_inplace(cvec<T>& x)
	{
		// conjugate the complex numbers
		x = x.apply(std::conj);

		// forward fft
		fft_inplace(x);

		// conjugate the complex numbers again
		x = x.apply(std::conj);

		// scale the numbers
		x /= x.size();
	}

	// out of place wrappers
	template<typename T>
	const cvec<T>& fft(cvec<T> x)
	{
		fft_inplace(x);
		return x;
	}

	template<typename T>
	const cvec<T>& fft(const rvec<T> &x)
	{
		cvec<T> y(x.size());
		transform(x.cbegin(), x.cend(), y.begin, [](const T& xk) { return complex<T>(xk); });
		fft_inplace(y);
		return y;
	}

	template<typename T>
	const cvec<T>& ifft(cvec<T> x)
	{
		ifft_inplace(x);
		return x;
	}

	enum struct WindowType
	{
		hanning,
		hamming
	};

	/**
	* Window Style:
	*
	* equivalent to MATLAB's sflag parameter
	*/
	enum struct WindowStyle
	{
		symmetric,
		periodic
	};

	template<typename T>
	class Window
	{
	public:
		Window(size_t length, WindowType type = WindowType::hanning, WindowStyle style = WindowStyle::periodic) :
			length(length),
			type(type),
			style(style),
			data(length)
		{
			size_t N = style == WindowStyle::symmetric ? length - 1 : length;

			switch(type)
			{
				case WindowType::hanning:
					for(int i = 0; i < length; ++i)
					{
						data[i] = 0.5*(1 - std::cos(i*2.0*pi<T> / N));
					}
					break;
				case WindowType::hamming:
					for(int i = 0; i < length; ++i)
					{
						data[i] = 0.54 - 0.46*std::cos(i*2.0*pi<T> / N);
					}
					break;
			}
		}

		template<typename param>
		const T& operator[] (param&& pos) const
		{
			return data[std::forward<param>(pos)];
		}

		const rvec<T>& get_data() { return data };

		/**
		* check if the window fullfiels the Constant OverLap-Add (COLA) condition.
		*/
		static bool isCOLA(const rvec<T>& window, const size_t& hopsize)
		{
			auto&& N = window.size();

			if(hopsize > N) return false;

			rvec<T> y(3 * N);
			for(size_t k = 0; k < 2 * N; k = k + hopsize)
			{
				y[slice(k, N, 1)] += window;
			}
			rvec<T> e = pow<T>(rvec<T>(y[slice(N, N, 1)]) - y[N], 2);
			rvec<T> we = pow<T>(window, 2);
			return e.sum() / we.sum() < std::numeric_limits<T>::epsilon();
		}

		bool isCOLA(const size_t& hopsize) { return isCOLA(data, hopsize); }

		static double gain(const rvec<T>& window, const size_t& hopsize)
		{
			return window.sum() / hopsize;
		}

		double gain(const size_t& hopsize) { return data.sum() / hopsize; }

		const size_t length;
		const WindowType type;
		const WindowStyle style;

	private:
		rvec<T> data;
	};


	namespace Filters
	{
		template<typename T>
		class Biquad_IIR
		{
		private:
			size_t nstages;
			vector<vector<T>> coeffs; // {b_0, b_1, b_2, a_0, a_1, a_2}
			vector<vector<T>> state;
		public:
			Biquad_IIR(const vector<vector<T>> &coeffs) :
				nstages(coeffs.size()),
				coeffs(coeffs),
				state(nstages, vector<T>(2))
			{
				for(const auto& i : coeffs)
				{
					if(i.size() != 6) throw std::invalid_argument("Biquad_IIR: each element of coeffs must be a vector of size 6 of the form {b_0, b_1, b_2, a_0, a_1, a_2}");
				}
			}

			T process(T x)
			{
				//w[n] = a_0*x[n] - a_1*w[n-1] - a_2*w[n-2]
				//y[n] = b_0*w[n] + b_1*w[n-1] + b_2*w[n-2]
				//see: https://en.wikipedia.org/wiki/Digital_biquad_filter

				for(int i = 0; i < nstages; ++i)
				{
					T w = coeffs[i][3] * x - coeffs[i][4] * state[i][0] - coeffs[i][5] * state[i][1];
					x = coeffs[i][0] * w + coeffs[i][1] * state[i][0] + coeffs[i][2] * state[i][1];
					state[i][1] = state[i][0];
					state[i][0] = w;
				}

				return x;
			}

			vector<T> process(const vector<T> &x)
			{
				vector<T> y;
				y.reserve(x.size());
				for(const auto& i : x)
				{
					y.emplace_back(process(i));
				}
			}
		};
	};
};