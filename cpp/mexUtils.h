#pragma once

#include <mex.h>
#include <cstdint>
#include <string>
#include <cstring>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <iostream>
#include <complex>
#include <map>
#include <valarray>
#include <algorithm>


namespace mexUtils
{


	class Cast
	{
	public:
		//using StringMap = std::map<std::string, std::string>;
		//typedef std::map<std::string, std::string> strmap;

		template <typename T> static mxArray *toMxArray(const T& data);
		template <typename T> static T fromMxArray(const mxArray * data);

		template <typename T> static mxArray * toMxCellArray(const std::vector<T> & data);

		template <typename T, typename... Args> static mxArray *toMxStruct(const std::string& fieldName, const T& fieldData, Args... args);
	private:
		static std::vector<const char*> getConstCharPtrVectorFromStringVector(const std::vector<std::string> & vector) {
			std::vector<const char*> cstrPtrVector(vector.size());
			for(int i = 0; i < vector.size(); ++i) cstrPtrVector[i] = vector[i].data();

			return cstrPtrVector;
		}


		template <typename T, typename... Args> static void toMxStructHelper(std::vector<std::string> &fieldNameVector, std::vector<mxArray*> &fieldDataVector, const std::string& fieldName, const T& fieldData, Args... args);
		template <typename T> static void toMxStructHelper(std::vector<std::string> &fieldNameVector, std::vector<mxArray*> &fieldDataVector, const std::string& fieldName, const T& fieldData);
	};



	//stdout redirection: as long as an object of this class exists cout will be redirected to mexPrintf
	class redirectOstream2mexPrintf : public std::streambuf
	{
	public:
		redirectOstream2mexPrintf(std::ostream & stream = std::cout) : _stream(stream)
		{
			outbuf = _stream.rdbuf(&(*this));
		}

		~redirectOstream2mexPrintf()
		{
			_stream.rdbuf(outbuf);
		}

	private:
		std::ostream & _stream;
		std::streambuf *outbuf;

	protected:
		virtual std::streamsize xsputn(const char *s, std::streamsize n);
		virtual int overflow(int c = EOF);
	};

	// returns the path of the mex file
	std::string getMexFilePath(void);

	///////////////
	// toMxArray //
	///////////////

	template<typename T>
	inline mxArray * Cast::toMxArray(const T & data)
	{
		static_assert(false, "mexUtils::toMxArray: Conversion not defined for this type!");
		return NULL;
	}

	template<> inline mxArray * Cast::toMxArray<double>(const double & data) {
		return mxCreateDoubleScalar(data);
	}

	template<> inline mxArray * Cast::toMxArray<float>(const float & data)
	{
		mxArray * array = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
		*(float*)mxGetData(array) = data;
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<int32_t>(const int32_t & data)
	{
		mxArray * array = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
		*(int32_t*)mxGetData(array) = data;
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<int64_t>(const int64_t & data)
	{
		mxArray * array = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
		*(int64_t*)mxGetData(array) = data;
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<uint32_t>(const uint32_t & data)
	{
		mxArray * array = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, mxREAL);
		*(uint32_t*)mxGetData(array) = data;
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<uint64_t>(const uint64_t & data)
	{
		mxArray * array = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
		*(uint64_t*)mxGetData(array) = data;
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<bool>(const bool & data)
	{
		mxArray * array = mxCreateNumericMatrix(1, 1, mxLOGICAL_CLASS, mxREAL);
		*(bool*)mxGetData(array) = data;
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::string>(const std::string & data)
	{
		mxArray * array = mxCreateString(data.data());
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<int16_t>>(const std::vector<int16_t> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxINT16_CLASS, mxREAL);
		int16_t* real = (int16_t*)mxGetData(array);
		copy(data.cbegin(), data.cend(), real);
		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<std::vector<int16_t>>>(const std::vector<std::vector<int16_t>> & data)
	{
		if(data.size() == 0 || data[0].size() == 0)
		{
			return mxCreateUninitNumericMatrix(0, 0, mxINT16_CLASS, mxREAL);
		}
		size_t n = data.size();
		size_t m = data[0].size();
		mxArray * array = mxCreateUninitNumericMatrix(m, n, mxINT16_CLASS, mxREAL);

		int16_t* real = (int16_t*)mxGetData(array);

		for(int i = 0; i<n; ++i)
		{
			if(data[i].size() != m) throw("All columns must have the same length");
			copy_n(data[i].begin(), m, &real[m*i]);
		}

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<std::complex<double>>>(const std::vector<std::complex<double>> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxDOUBLE_CLASS, mxCOMPLEX);

		#if MX_HAS_INTERLEAVED_COMPLEX
			/* add interleaved complex API code here */
			mxComplexDouble* array_ptr = mxGetComplexDoubles(array);
			for(int i = 0; i<data.size(); ++i) {
				array_ptr[i].real = data[i].real();
				array_ptr[i].imag = data[i].imag();
			}
		#else
			double* real = (double*)mxGetData(array);
			double* imag = (double*)mxGetImagData(array);

			for(int i = 0; i<data.size(); ++i) {
				real[i] = data[i].real();
				imag[i] = data[i].imag();
			}
		#endif	

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<std::complex<float>>>(const std::vector<std::complex<float>> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxSINGLE_CLASS, mxCOMPLEX);

		#if MX_HAS_INTERLEAVED_COMPLEX
			mxComplexSingle* array_ptr = mxGetComplexSingles(array);
			for(int i = 0; i<data.size(); ++i) {
				array_ptr[i].real = data[i].real();
				array_ptr[i].imag = data[i].imag();
			}
		#else
			float* real = (float*)mxGetData(array);
			float* imag = (float*)mxGetImagData(array);

			for(int i = 0; i<data.size(); ++i) {
				real[i] = data[i].real();
				imag[i] = data[i].imag();
			}
		#endif

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<double>>(const std::vector<double> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxDOUBLE_CLASS, mxREAL);

		double* real = (double*)mxGetData(array);

		for(int i = 0; i<data.size(); ++i) {
			real[i] = data[i];
		}

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<float>>(const std::vector<float> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxSINGLE_CLASS, mxREAL);

		float* real = (float*)mxGetData(array);

		for(int i = 0; i<data.size(); ++i)
		{
			real[i] = data[i];
		}

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<std::vector<double>>>(const std::vector<std::vector<double>> & data)
	{
		if(data.size() == 0 || data[0].size() == 0)
		{
			return mxCreateUninitNumericMatrix(0, 0, mxDOUBLE_CLASS, mxREAL);
		}
		size_t n = data.size();
		size_t m = data[0].size();
		mxArray * array = mxCreateUninitNumericMatrix(m, n, mxDOUBLE_CLASS, mxREAL);

		double* real = (double*)mxGetData(array);

		for(int i = 0; i<n; ++i)
		{
			if(data[i].size() != m) throw("All columns must have the same length");
			copy_n(data[i].begin(), m, &real[m*i]);
		}

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<std::vector<float>>>(const std::vector<std::vector<float>> & data)
	{
		if(data.size() == 0 || data[0].size() == 0)
		{
			return mxCreateUninitNumericMatrix(0, 0, mxSINGLE_CLASS, mxREAL);
		}
		size_t n = data.size();
		size_t m = data[0].size();
		mxArray * array = mxCreateUninitNumericMatrix(m, n, mxSINGLE_CLASS, mxREAL);

		float* real = (float*)mxGetData(array);

		for(int i = 0; i<n; ++i)
		{
			if(data[i].size() != m) throw("All columns must have the same length");
			copy_n(data[i].begin(), m, &real[m*i]);
		}

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::valarray<std::complex<double>>>(const std::valarray<std::complex<double>> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxDOUBLE_CLASS, mxCOMPLEX);


#if MX_HAS_INTERLEAVED_COMPLEX
		/* add interleaved complex API code here */
		mxComplexDouble* array_ptr = mxGetComplexDoubles(array);
		for(int i = 0; i < data.size(); ++i) {
			array_ptr[i].real = data[i].real();
			array_ptr[i].imag = data[i].imag();
		}
#else
		double* real = (double*)mxGetData(array);
		double* imag = (double*)mxGetImagData(array);

		for(int i = 0; i < data.size(); ++i) {
			real[i] = data[i].real();
			imag[i] = data[i].imag();
		}
#endif	

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::valarray<std::complex<float>>>(const std::valarray<std::complex<float>> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxSINGLE_CLASS, mxCOMPLEX);

#if MX_HAS_INTERLEAVED_COMPLEX
		mxComplexSingle* array_ptr = mxGetComplexSingles(array);
		for(int i = 0; i < data.size(); ++i) {
			array_ptr[i].real = data[i].real();
			array_ptr[i].imag = data[i].imag();
		}
#else
		float* real = (float*)mxGetData(array);
		float* imag = (float*)mxGetImagData(array);

		for(int i = 0; i < data.size(); ++i) {
			real[i] = data[i].real();
			imag[i] = data[i].imag();
		}
#endif

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::valarray<double>>(const std::valarray<double> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxDOUBLE_CLASS, mxREAL);

		double* real = (double*)mxGetData(array);

		for(int i = 0; i<data.size(); ++i)
		{
			real[i] = data[i];
		}

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::valarray<float>>(const std::valarray<float> & data)
	{
		mxArray * array = mxCreateUninitNumericMatrix(data.size(), 1, mxSINGLE_CLASS, mxREAL);

		float* real = (float*)mxGetData(array);

		for(int i = 0; i<data.size(); ++i)
		{
			real[i] = data[i];
		}

		return array;
	}

	template<> inline mxArray * Cast::toMxArray<std::map<std::string, std::string>>(const std::map<std::string, std::string> & data)
	{
		std::vector<std::string> keys;
		for(auto it = data.begin(); it != data.end(); ++it) {
			keys.push_back(it->first);
		}

		std::vector<const char*> cnames = getConstCharPtrVectorFromStringVector(keys);
		mxArray * r = mxCreateStructMatrix(1, 1, int(data.size()), &cnames[0]);

		for(size_t i = 0; i < keys.size(); i++)
		{
			mxSetField(r, 0, cnames[i], toMxArray<std::string>(data.at(keys[i])));
		}
		return r;
	}

	template<> inline mxArray * Cast::toMxArray<std::vector<std::map<std::string, std::string>>>(const std::vector<std::map<std::string, std::string>> & data)
	{
		return toMxCellArray(data);
	}

	template <typename T, typename... Args> static mxArray *Cast::toMxStruct(const std::string& fieldName, const T& fieldData, Args... args)
	{
		std::vector<std::string> fieldNameVector;
		std::vector<mxArray*> fieldDataVector;

		fieldNameVector.push_back(fieldName);
		fieldDataVector.push_back(Cast::toMxArray(fieldData));
		Cast::toMxStructHelper(fieldNameVector, fieldDataVector, args...);

		std::vector<const char*> cnames = getConstCharPtrVectorFromStringVector(fieldNameVector);
		mxArray * r = mxCreateStructMatrix(1, 1, int(fieldNameVector.size()), &cnames[0]);


		for(size_t i = 0; i < fieldNameVector.size(); i++)
		{
			mxSetField(r, 0, cnames[i], fieldDataVector[i]);
		}
		return r;
	}

	template <typename T, typename... Args> static void Cast::toMxStructHelper(std::vector<std::string> &fieldNameVector, std::vector<mxArray*> &fieldDataVector, const std::string& fieldName, const T& fieldData, Args... args)
	{
		fieldNameVector.push_back(fieldName);
		fieldDataVector.push_back(Cast::toMxArray(fieldData));
		Cast::toMxStructHelper(fieldNameVector, fieldDataVector, args...);
		return;
	}

	template <typename T> static void Cast::toMxStructHelper(std::vector<std::string> &fieldNameVector, std::vector<mxArray*> &fieldDataVector, const std::string& fieldName, const T& fieldData)
	{
		fieldNameVector.push_back(fieldName);
		fieldDataVector.push_back(Cast::toMxArray(fieldData));
		return;
	}


	template <typename T> static mxArray * Cast::toMxCellArray(const std::vector<T> & data)
	{
		mxArray * r = mxCreateCellMatrix(data.size(), 1);

		for(size_t i = 0; i < data.size(); i++) {
			mxSetCell(r, i, toMxArray(data[i]));
		}

		return r;
	}

	///////////////
	//fromMxArray//
	///////////////
	template<typename T>
	inline T Cast::fromMxArray(const mxArray * data)
	{
		static_assert(false, "mexUtils::fromMxArray: Conversion not defined for this type!");
		return NULL;
	}

	template<> inline bool Cast::fromMxArray<bool>(const mxArray * data)
	{
		switch(mxGetClassID(data))
		{
			case mxLOGICAL_CLASS:
				return mxIsLogicalScalarTrue(data);
			case mxDOUBLE_CLASS:
			case mxSINGLE_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
				return mxGetScalar(data) != 0;
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline std::string Cast::fromMxArray<std::string>(const mxArray * data)
	{
		switch(mxGetClassID(data))
		{
			case mxCHAR_CLASS:
			{
				char *cstr = mxArrayToString(data);
				std::string str(cstr);
				mxFree(cstr);
				return str;
			}
			case mxDOUBLE_CLASS:
				return std::to_string(mxGetScalar(data));
			case mxSINGLE_CLASS:
				return std::to_string(float(mxGetScalar(data)));
			case mxLOGICAL_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
				return std::to_string(int(mxGetScalar(data)));
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline double Cast::fromMxArray<double>(const mxArray * data)
	{
		return mxGetScalar(data);
	}

	template<> inline float Cast::fromMxArray<float>(const mxArray * data)
	{
		return float(mxGetScalar(data));
	}

	template<> inline int Cast::fromMxArray<int>(const mxArray * data)
	{
		return int(mxGetScalar(data));
	}

	template<> inline size_t Cast::fromMxArray<size_t>(const mxArray * data)
	{
		return size_t(mxGetScalar(data));
	}

	template<> inline std::valarray<double> Cast::fromMxArray<std::valarray<double>>(const mxArray * data)
	{
		size_t N = mxGetNumberOfElements(data);
		switch(mxGetClassID(data))
		{
			case mxDOUBLE_CLASS:
				return std::valarray<double>(mxGetPr(data), N);
			case mxSINGLE_CLASS:
			{
				std::valarray<double> tensor(N);
				float* a = static_cast<float*>(mxGetData(data));
				for(int i = 0; i < N; ++i)
				{
					tensor[i] = double(a[i]);
				}
				return tensor;
			}
			case mxLOGICAL_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline std::valarray<float> Cast::fromMxArray<std::valarray<float>>(const mxArray * data)
	{
		size_t N = mxGetNumberOfElements(data);
		switch(mxGetClassID(data))
		{
			case mxDOUBLE_CLASS:
			{
				std::valarray<float> tensor(N);
				double* a = static_cast<double*>(mxGetData(data));
				for(int i = 0; i < N; ++i)
				{
					tensor[i] = float(a[i]);
				}
				return tensor;
			}
			case mxSINGLE_CLASS:
			{
				return std::valarray<float>(static_cast<float*>(mxGetData(data)), N);
			}
			case mxLOGICAL_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline std::vector<double> Cast::fromMxArray<std::vector<double>>(const mxArray * data)
	{
		size_t N = mxGetNumberOfElements(data);
		switch(mxGetClassID(data))
		{
			case mxDOUBLE_CLASS:
				return std::vector<double>(mxGetPr(data), mxGetPr(data) + N);
			case mxSINGLE_CLASS:
			{
				std::vector<double> tensor(N);
				float* a = static_cast<float*>(mxGetData(data));
				for(int i = 0; i < N; ++i)
				{
					tensor[i] = double(a[i]);
				}
				return tensor;
			}
			case mxLOGICAL_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline std::vector<std::vector<double>> Cast::fromMxArray<std::vector<std::vector<double>>>(const mxArray * data)
	{
		if(mxGetNumberOfElements(data) == 0) return std::vector<std::vector<double>>();

		size_t M = mxGetM(data);
		size_t N = mxGetNumberOfElements(data) / M;
		switch(mxGetClassID(data))
		{
			case mxDOUBLE_CLASS:
			{
				double *data_ptr = (double*)mxGetData(data);
				std::vector<std::vector<double>> matrix(N, std::vector<double>(M));
				for(int i = 0; i < N; ++i)
				{
					copy(&data_ptr[i*M], &data_ptr[i*M] + M, matrix[i].begin());
				}
				return matrix;
			}
			case mxSINGLE_CLASS:
			{
				float *data_ptr = (float*)mxGetData(data);
				std::vector<std::vector<double>> matrix(N, std::vector<double>(M));
				for(int i = 0; i < N; ++i)
				{
					transform(&data_ptr[i*M], &data_ptr[i*M] + M, matrix[i].begin(), [](const float &x) { return (double)x; });
				}
				return matrix;
			}
			case mxLOGICAL_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline std::vector<std::vector<float>> Cast::fromMxArray<std::vector<std::vector<float>>>(const mxArray * data)
	{
		if(mxGetNumberOfElements(data) == 0) return std::vector<std::vector<float>>();

		size_t M = mxGetM(data);
		size_t N = mxGetNumberOfElements(data) / M;
		switch(mxGetClassID(data))
		{
			case mxDOUBLE_CLASS:
			{
				double *data_ptr = (double*)mxGetData(data);
				std::vector<std::vector<float>> matrix(N, std::vector<float>(M));
				for(int i = 0; i < N; ++i)
				{
					transform(&data_ptr[i*M], &data_ptr[i*M] + M, matrix[i].begin(), [](const double &x) { return (float)x; });
				}
				return matrix;
			}
			case mxSINGLE_CLASS:
			{
				float *data_ptr = (float*)mxGetData(data);
				std::vector<std::vector<float>> matrix(N, std::vector<float>(M));
				for(int i = 0; i < N; ++i)
				{
					copy(&data_ptr[i*M], &data_ptr[i*M] + M, matrix[i].begin());
				}
				return matrix;
			}
			case mxLOGICAL_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline std::vector<float> Cast::fromMxArray<std::vector<float>>(const mxArray * data)
	{
		size_t N = mxGetNumberOfElements(data);
		switch(mxGetClassID(data))
		{
			case mxDOUBLE_CLASS:
			{
				std::vector<float> tensor(N);
				double* a = static_cast<double*>(mxGetData(data));
				for(int i = 0; i < N; ++i)
				{
					tensor[i] = float(a[i]);
				}
				return tensor;
			}
			case mxSINGLE_CLASS:
			{
				return std::vector<float>(static_cast<float*>(mxGetData(data)), static_cast<float*>(mxGetData(data)) + N);
			}
			case mxLOGICAL_CLASS:
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
			default:
				throw std::runtime_error("illegal datatype");
		}
	}

	template<> inline std::map<std::string, std::string> Cast::fromMxArray<std::map<std::string, std::string>>(const mxArray * data)
	{
		if(!mxIsStruct(data) || !mxIsScalar(data)) throw std::runtime_error("invalid datatype: input must be a scalar struct");
		std::map<std::string, std::string> args;
		int nfields = mxGetNumberOfFields(data);

		for(int i = 0; i < nfields; ++i)
		{
			const char* fieldName = mxGetFieldNameByNumber(data, i);
			mxArray *field = mxGetFieldByNumber(data, 0, i);
			if(!mxIsChar(field)) throw std::runtime_error("all fields must be of type char");
			std::string fieldstr = fromMxArray<std::string>(field);
			args.emplace(std::string(fieldName), fieldstr);
		}

		return args;
	}
};

