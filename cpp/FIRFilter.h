#pragma once
#include <vector>
#include <stdexcept>

namespace SdrBirdrec
{
	/*!
	* FIR filter
	*/
	template<typename T>
	class FIRFilter
	{
	private:
		vector<T> coeffs;
		vector<T> state;
		size_t index;
	public:
		const size_t order;

		FIRFilter(const vector<T> &coeffs) :
			order(coeffs.size()-1),
			coeffs(coeffs),
			state(coeffs.size()),
			index(coeffs.size()-1)
		{
			if(coeffs.size() == 0) throw std::invalid_argument("FIRFilter: size of coeffs is 0");
		}

		T process(T x)
		{
			state[index] = x;
			
			size_t j = (index + 1) % coeffs.size();
			T sum = 0;
			for(size_t i = 0; i < coeffs.size(); ++i)
			{
				sum += coeffs[i] * state[j];
				j = (j + 1) % coeffs.size();
			}

			index = (index + 1) % coeffs.size();
			return sum;
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