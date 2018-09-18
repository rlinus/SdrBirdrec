#pragma once

#include <utility>
#include <atomic>
#include <exception>
#include <SoapySDR/Device.hpp>

/*!
* RAII wrapper for soapy device.
*/
class SoapyDevice
{
private:
	SoapySDR::Stream *rxStream;
	SoapySDR::Stream *txStream;

	std::atomic<bool> rxIsStreaming = false;
	std::atomic<bool> txIsStreaming = false;
public:
	SoapySDR::Device *const handle;

	template<typename T>
	SoapyDevice(T&& args):
		handle{SoapySDR::Device::make(std::forward<T>(args))},
		rxStream{nullptr},
		txStream{nullptr}
	{
		if (handle == nullptr) throw std::runtime_error("SoapyDevice: Device initialisation error");
	}

	~SoapyDevice()
	{
		if(rxStream != nullptr) handle->closeStream(rxStream);
		if(txStream != nullptr) handle->closeStream(txStream);
		SoapySDR::Device::unmake(handle);
	}

	template<typename... Ts>
	void setupRxStream(Ts&&... ts)
	{
		if (rxStream != nullptr) handle->closeStream(rxStream);
		rxStream = handle->setupStream(SOAPY_SDR_RX, std::forward<Ts>(ts)...);
		if (rxStream == nullptr) throw std::runtime_error("SoapyDevice: rx stream could not be initialised");
	}

	template<typename... Ts>
	void setupTxStream(Ts&&... ts)
	{
		if (txStream != nullptr) handle->closeStream(txStream);
		txStream = handle->setupStream(SOAPY_SDR_TX, std::forward<Ts>(ts)...);
		if (txStream == nullptr) throw std::runtime_error("SoapyDevice: tx stream could not be initialised");
	}

	void closeRxStream()
	{
		if (rxStream != nullptr) handle->closeStream(rxStream);
		rxStream = nullptr;
	}

	void closeTxStream()
	{
		if (txStream != nullptr) handle->closeStream(txStream);
		txStream = nullptr;
	}

	template<typename... Ts>
	int activateRxStream(Ts&&... ts)
	{
		if(rxStream == nullptr) throw std::runtime_error("RX stream is not setup yet");
		if(rxIsStreaming) throw std::runtime_error("RX stream is already activated");
		
		int status = handle->activateStream(rxStream, std::forward<Ts>(ts)...);
		if(status == 0) rxIsStreaming = true;
		return status;
	}

	template<typename... Ts>
	int activateTxStream(Ts&&... ts)
	{
		if(txStream == nullptr) throw std::runtime_error("RX stream is not setup yet");
		if(txIsStreaming) throw std::runtime_error("RX stream is already activated");

		int status = handle->activateStream(txStream, std::forward<Ts>(ts)...);
		if(status == 0) txIsStreaming = true;
		return status;
	}

	template<typename... Ts>
	int deactivateRxStream(Ts&&... ts)
	{
		if(rxStream == nullptr) throw std::runtime_error("RX stream is not setup yet");
		if(!rxIsStreaming) throw std::runtime_error("RX stream is not activated");

		int status = handle->deactivateStream(rxStream, std::forward<Ts>(ts)...);
		if(status == 0) rxIsStreaming = false;
		return status;
	}

	template<typename... Ts>
	int deactivateTxStream(Ts&&... ts)
	{
		if(txStream == nullptr) throw std::runtime_error("TX stream is not setup yet");
		if(!txIsStreaming) throw std::runtime_error("TX stream is not activated");

		int status = handle->deactivateStream(txStream, std::forward<Ts>(ts)...);
		if(status == 0) txIsStreaming = false;
		return status;
	}

	bool rxStreamIsActivated() { return rxIsStreaming; }
	bool txStreamIsActivated() { return txIsStreaming; }

	size_t getRxStreamMTU()
	{ 
		if(rxStream == nullptr) throw std::runtime_error("getRxStreamMTU: RX stream is not setup yet");
		return handle->getStreamMTU(rxStream); 
	}
	size_t getTxStreamMTU() 
	{
		if(txStream == nullptr) throw std::runtime_error("getTxStreamMTU: TX stream is not setup yet");
		return handle->getStreamMTU(txStream);
	}

	template<typename... Ts>
	int readStream(Ts&&... ts) { return handle->readStream(rxStream, std::forward<Ts>(ts)...); }

	template<typename... Ts>
	int writeStream(Ts&&... ts) { return handle->writeStream(txStream, std::forward<Ts>(ts)...); }
};