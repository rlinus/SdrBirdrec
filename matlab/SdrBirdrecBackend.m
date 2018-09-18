%% SdrBirdrecBackend multichannel sdr and NI-DAQmx recorder
%

classdef SdrBirdrecBackend < handle
    properties (SetAccess = private, Hidden = true)
        objectHandle; % Handle to the underlying C++ class instance
    end
    methods
        %% Constructor
        % constructs a new SdrBirdrecBackend handle object.
        %
        % sdr_device:   a struct that identifies an sdr device. Use one of
        %               the elements returned by 
        %               SdrBirdrecBackend.findSdrDevices()
        function this = SdrBirdrecBackend()
            this.objectHandle = SdrBirdrecMex('new');
        end
        
        %% Destructor
        function delete(this)
            SdrBirdrecMex('delete', this.objectHandle);
        end

        %% initRec
        % initializes a recording. Params must be a struct whith the
        % following fields:
        %   
        % SDR_SampleRate:
        %   the sdr sample rate in Hz [default: 1e6]
        % SDR_CenterFrequency:
        %   the frequency which the sdr is tuned to [defualt: 300e6]
        % SDR_AGC:
        %   enable automatic gain control of sdr [defualt: false]
        % SDR_Gain:
        %   the tuner gain of the sdr in dB if SDR_AGC is off [default: 50]
        % SDR_StartOnTrigger:   
        %   Set to true if the sdr recording shall be started by a hardware trigger 
        %   (rising flank) applied at the PPS port of the sdr device.
        %   This can be used to sync the sdr signal with the daqmx channels 
        %   or other recorders. [default: false]
		% SDR_ExternalClock:
		%   Set this properety to true to enable the external 10MHz clock
        %   input on the SDR device. [default: false]
        % SDR_TrackingRate:
        %   the rate of the frequency tracking of the sdr channels in Hz
        %   [default: 20]
        % MonitorRate:
        %   Specifies of often per second getData() returns monitor data.
        %   [default: 5]
        % Decimator1_Factor:
        %   the decimation factor of the first decimation stage of the
        %   sdr signal [default: 20]
        % Decimator2_Factor:
        %   the decimation factor of the second decimation stage of the
        %   sdr signal [default: 10]. The sample rate of the demodulated
        %   sdr channels is
        %   SDR_SampleRate/(Decimator1_Factor*Decimator2_Factor)
        % FreqTreckingThreshold:
        %   If abs(CarrierFreq - ReceiveFreq) > FreqTreckingThreshold the
        %   ReceiveFreq will be updated.
        % Decimator1_FirFilterCoeffs: a vector containing the FIR filter
        %   coefficients of the first decimation stage.
        % Decimator2_FirFilterCoeffs: a vector containing the FIR filter
        %   coefficients of the second decimation stage.
        % Decimator1_InputFrameSize: the input frame size for the first
        %   decimator. 
        % IirFilterCoeffs: 
        %   Biquad filter coefficients (Mx6 matrix) of a filter applied to   
        %   the sdr channels after the demodulation and downsampling.
        %   [default: empty (no filter applied)].
        % SdrChannels_AudioGain:
        %   A gain applied to the sdr channels at the end of the dsp chain.
        %   Can be used to fit the dynamic range, if data is saved as
        %   fixedpoint values.
        % SDR_ChannelBands:
        %   a matrix of size Nx2 (where N is the number of sdr channels that
        %   are recorded), containing the frequency bands, in which
        %   the n-th transmitter is found. The bands are not allowed to
        %   overlaped an must be in the range of
        %   [SDR_CenterFrequency-SDR_SampleRate/2,SDR_CenterFrequency+SDR_SampleRate/2)
        % DAQmx_AIChannelList:
        %   A list of analog DAQmx channels in the format described here:
        %   http://zone.ni.com/reference/en-XX/help/370466AC-01/mxcncpts/physchannames/
        % DAQmx_ChannelCount:
        %   The number of DAQmx channels you want to record. This number
        %   must match the DAQmx_AIChannelList setting. [default: 0]
        % DAQmx_ExternalClock:
        %   Set this properety to true to enable the external 10MHz clock
        %   input on the DAQ device. [default: false]
		% DAQmx_ClockInputTerminal:
		%   The input terminal on the DAQ device to which the external
        %   10MHz clock signal is connected (e.g: '/Dev1/PFI3'). 
        % DAQmx_TriggerOutputTerminal:
        %   An output terminal at which a physical start trigger of the DAQmx 
        %   recording is outputed (e.g: '/Dev1/PFI4').
        % DAQmx_AITerminalConfig:
        %   The terminal configuration of the analog DAQmx channels.
        %   Possible values are:
        %       * 'DAQmx_Val_Cfg_Default' [default]
        %       * 'DAQmx_Val_RSE'
        %       * 'DAQmx_Val_NRSE'
        %       * 'DAQmx_Val_Diff'
        %       * 'DAQmx_Val_PseudoDiff'
        % DAQmx_SampleRate:
        %   The sample rate of the DAQmx channels.
        % DAQmx_MaxVoltage:
        %   The ADC is configured to the range
        %   [-DAQmx_MaxVoltage,DAQmx_MaxVoltage]. [default: 10]
        % DAQmx_AILowpassEnable:
        %   Enables the analog lowpass filter in front of the ADC.
        %   [default: false]
        % DAQmx_AILowpassCutoffFreq:
        %   The -3dB cutoff frequency of the analog lowpass filter.
        % AudioOutput_DeviceIndex:
        %   The device index of the audio output device. The value -1
        %   selects the default device. Use the findAudioOutputDevices()
        %   method to query device indexes. [default: -1]
        % DataFile_SamplePrecision:
        %   The sample precision of the datafiles. Possible values:
        %       *float32 [default]
        %       *int16 
        % SdrChannelsFilename:
        %   The name of the sdr channels datafile. It's format is Wave64
        %   (.w64)
        % SdrSignalStrengthFilename:
        %   The name of the Wave64 file that logs the sdr channels signal
        %   strengths.
        % SdrReceiveFreqFilename:
        %   The name of the Wave64 file that logs the demodulation frequencies
        %   of the sdr channels.
        % SdrCarrierFreqFilename:
        %   The name of the Wave64 file that logs the carrier frequencies
        %   of the sdr channels.
        % DAQmxChannelsFilename:
        %   The name of the DAQmx channels datafile. Its format is Wave64
        %   (.w64)
        % LogFilename:
        %   The name of the text file that logs buffer overflows. 
        
        function initRec(this, params)
            SdrBirdrecMex('initRec', this.objectHandle, params);
        end
        
        %% startRec
        % starts a new recording. Must be preceeded by a call to
        % initStream(params).
        function startRec(this)
            SdrBirdrecMex('startRec', this.objectHandle);
        end
        
        %% stopRec
        % stops the recording
        function stopRec(this)
            SdrBirdrecMex('stopRec', this.objectHandle);
        end
        
        %% getMonitorDataFrame
        % a blocking call that returns a struct of monitor data.
        % The return value has the following fields:
        % sdr_spectrum: a log power spectrum of the sdr data input at the
        %               current time, centered around SDR_CenterFrequency.
        %               length(sdr_spectrum) =
        %               length(Decimator1_InputFrameSize) +
        %               length(Decimator1_FirFilterCoeffs) - 1
        % receive_frequencies:  a vector containing the current demodulation
        %                       frequency of each sdr channel.
        % carrier_frequencies:  a vector containing the carrier frequency
        %                       for each channel (argmax of the of the
        %                       spectrum in the channel band)
        % signal_strengths: the signal strength in db at the carrier
        %                   frequencies.
        % output_signal: a vector containing the current frame of one recorded
        %                channel (sdr or daqmx). 
        function x = getMonitorDataFrame(this)
            x = SdrBirdrecMex('getMonitorDataFrame', this.objectHandle);
        end
        
        %% isRecording
        % returns if a recording is started
        function x = isRecording(this)
            x = SdrBirdrecMex('isRecording', this.objectHandle);
        end
        
        %% setSquelch
        % set the squelch for the audio output of an sdr channel. If the
        % current signal strength is below squelch, the audio output is
        % interrupted.
        function setSquelch(this, squelch)
            SdrBirdrecMex('setSquelch', this.objectHandle, squelch);
        end
        
        %% setChannel
        % selects the channel type and channel number of the channel which is streamed to audio 
        % output and which is returned by getData.
        % channel_type:     possible values are:
        %                       'sdr': to stream an sdr channel
        %                       'daqmx': to stream an NI-DAQmx channel
        % channel_number:   channel number starting from 0
        function setChannel(this, channel_type, channel_number)
            SdrBirdrecMex('setChannel', this.objectHandle, channel_type, channel_number);
        end
        
        %% setChannelNumber
        % selects the channel which is streamed to audio output and which
        % is returned by getData.
        function setChannelNumber(this, channel_number)
            SdrBirdrecMex('setChannelNumber', this.objectHandle, channel_number);
        end
        
        %% setChannelType
        % selects the channel type of the channel which is streamed to audio 
        % output and which is returned by getData. Possible values are:
        %   'sdr': to stream an sdr channel
        %   'daqmx': to stream an NI-DAQmx channel
        function setChannelType(this, channel_type)
            SdrBirdrecMex('setChannelType', this.objectHandle, channel_type);
        end
        
        %% setPlayAudio
        % setPlayAudio(true): enables audio output
        % setPlayAudio(false): disables audio output
        function setPlayAudio(this, play_audio)
            SdrBirdrecMex('setPlayAudio', this.objectHandle, play_audio);
        end
        
    end
	methods(Static)
        %% findSdrDevices
        % returns a cell array of structs. Each struct represents a
        % detected sdr device and contains properties that identify the
        % device.
		function devices = findSdrDevices()
            devices = SdrBirdrecMex('findSdrDevices');
        end
        
        %% findAudioOutputDevices
        % returns a cell array of structs. Each struct represents a
        % detected audio output device and contains properties that identify the
        % device.
        function devices = findAudioOutputDevices()
            devices = SdrBirdrecMex('findAudioOutputDevices');
        end
        
        
	end
        
    
end