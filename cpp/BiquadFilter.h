#pragma once
#include <vector>
#include <stdexcept>

namespace SdrBirdrec
{
	/*!
	* Biquad IIR filter
	*/
	template<typename T>
	class BiquadFilter
	{
	private:
		const size_t nstages;
		vector<vector<T>> coeffs; // {b_0, b_1, b_2, a_0, a_1, a_2}
		vector<vector<T>> state;
	public:
		BiquadFilter(const vector<vector<T>> &coeffs) :
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

			for(size_t i = 0; i < nstages; ++i)
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
			return y;
		}
	};
}