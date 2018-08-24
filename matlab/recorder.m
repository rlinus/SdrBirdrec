function recorder(sdrBirdrecBackend, settings, gui_handles)
%recorder this function sets the init params for the sdrBirdrecBackend
%object and then starts the recording.

%% variables
sdrSpectrumPlotRate = 5; %how often to plot the sdr spectrum per second
spectrogramPlotRate = 1; %how often to plot the spectrogram per second

fs_lf = 24000; %samplerate of the demodulated sdr channels
bw_lf = 20000; %two sideded bandwidth of the demodulated sdr channels (filter parameter)

Decimator2_Factor = 10;
fs_if= fs_lf*Decimator2_Factor; %samplerate after first decimation
bw_if = 200e3; %bandwidth after first decimation (filter parameter)

switch settings.sdr_sample_rate_index
    case 1 %2.4 MHz
        Decimator1_Factor = 10;
        trackingSampleRate = 20; %frames per second
        Decimator1FramesPerTrackingFrame = 20;    
    case 2 %4.8 MHz
        Decimator1_Factor = 20;
        trackingSampleRate = 20; %frames per second
        Decimator1FramesPerTrackingFrame = 40;
    case 3 %6.0 MHz
        Decimator1_Factor = 25;
        trackingSampleRate = 20; %frames per second
        Decimator1FramesPerTrackingFrame = 50;
    case 4 %7.2 MHz
        Decimator1_Factor = 30;
        trackingSampleRate = 20; %frames per second
        Decimator1FramesPerTrackingFrame = 100;
    case 5 %9.6 MHz
        Decimator1_Factor = 40;
        trackingSampleRate = 20; %frames per second
        Decimator1FramesPerTrackingFrame = 80;
    otherwise
        error('No profile for sdr_sample_rate_index=%i',settings.sdr_sample_rate_index);
end

fs_hf = fs_if*Decimator1_Factor; %sdr samplerate


%% filenames
params.LogFilename = [settings.outputfolder '\' settings.fn '_log.txt'];
SdrChannelListFilename = [settings.outputfolder '\' settings.fn '_SdrChannelList.csv'];

if ~settings.split_files
    params.SdrChannelsFilename = [settings.outputfolder '\' settings.fn '_SdrChannels.w64'];
    params.SdrSignalStrengthFilename = [settings.outputfolder '\' settings.fn '_SdrSignalStrength.w64'];
    params.SdrCarrierFreqFilename = [settings.outputfolder '\' settings.fn '_SdrCarrierFreq.w64'];
    params.SdrReceiveFreqFilename = [settings.outputfolder '\' settings.fn '_SdrReceiveFreq.w64'];
    params.DAQmxChannelsFilename = [settings.outputfolder '\' settings.fn '_DAQmxChannels.w64'];
else
    params.SdrChannelsFilename = [settings.outputfolder '\' settings.fn '_SdrChannels.*.w64'];
    params.SdrSignalStrengthFilename = [settings.outputfolder '\' settings.fn '_SdrSignalStrength.*.w64'];
    params.SdrCarrierFreqFilename = [settings.outputfolder '\' settings.fn '_SdrCarrierFreq.*.w64'];
    params.SdrReceiveFreqFilename = [settings.outputfolder '\' settings.fn '_SdrReceiveFreq.*.w64'];
    params.DAQmxChannelsFilename = [settings.outputfolder '\' settings.fn '_DAQmxChannels.*.w64'];
end

%% udp trigger messages
udp_start_msg = ['sdrbirdrec::start::' params.SdrChannelsFilename];
udp_stop_msg = 'sdrbirdrec::stop';

%% Check for existing files
if ~isempty(dir(params.SdrChannelsFilename)) || ... 
        ~isempty(dir(params.SdrSignalStrengthFilename)) || ...
        ~isempty(dir(params.SdrCarrierFreqFilename)) || ...
        ~isempty(dir(params.SdrReceiveFreqFilename)) || ...
        ~isempty(dir(params.DAQmxChannelsFilename)) || ...
        exist(params.LogFilename,'file') || ...
        exist(SdrChannelListFilename,'file')
    
    choice = questdlg(sprintf('Some files already exist, they will be overwritten. Do you want to proceed?'),'Warning!', 'Yes', 'No', 'No');
    if strcmpi(choice,'No')
        gui_handles.status_txt.String = 'Ready.';
        return;
    end
end
if exist(params.LogFilename,'file'); delete(params.LogFilename); end;


%% global vars (the global vars are updated in the gui)
global is_recording;
global monitor_settings;

gui_handles.status_txt.String = 'Initialization...';
drawnow;

%% init filters
filterfile = [fileparts(mfilename('fullpath')) filesep 'filters' num2str(settings.sdr_sample_rate_index) '.mat'];

if exist(filterfile,'file')
    load(filterfile);
else   
    switch settings.sdr_sample_rate_index
        case 1 %2.4 MHz
            ddc_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 400, 'SampleRate', fs_hf, 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2);
        case 2 %4.8 MHz
            ddc_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 400, 'SampleRate', fs_hf, 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2);
        case 3 %6.0 MHz
            ddc_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 400, 'SampleRate', fs_hf, 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2);
        case 4 %7.2 MHz
            ddc_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 450, 'SampleRate', fs_hf, 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2);
        case 5 %9.6 MHz
            ddc_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 600, 'SampleRate', fs_hf, 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2);
        otherwise
            error('No profile for sdr_sample_rate_index=%i',settings.sdr_sample_rate_index);
    end
    lp_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 250, 'SampleRate', fs_if, 'PassbandFrequency', bw_lf/2, 'StopbandFrequency', fs_lf/2);
    save(filterfile, 'ddc_filter', 'lp_filter');
    
    %hp_filter = designfilt('highpassiir', 'SampleRate', fs_lf, 'StopbandFrequency', 15, 'PassbandFrequency', 50, 'StopbandAttenuation', 80, 'PassbandRipple', 0.5);
    %save(filterfile, 'ddc_filter', 'lp_filter', 'hp_filter');
end

%% set params

params.SDR_DeviceArgs = settings.sdr_args;
params.MonitorRate = sdrSpectrumPlotRate;
params.SDR_SampleRate = fs_hf;
params.SDR_CenterFrequency = settings.sdr_center_freq * 1e6;
params.SDR_AGC = settings.AGC;
params.SDR_Gain = settings.TunerGain;
params.SDR_TrackingRate = trackingSampleRate;
params.Decimator1_InputFrameSize = (fs_hf/trackingSampleRate)/Decimator1FramesPerTrackingFrame;
params.Decimator1_Factor = Decimator1_Factor;
params.Decimator2_Factor = Decimator2_Factor;
params.Decimator1_FirFilterCoeffs = ddc_filter.Coefficients;
params.Decimator2_FirFilterCoeffs = lp_filter.Coefficients;
params.IirFilterCoeffs = []; %hp_filter.Coefficients;
params.SDR_ChannelBands = settings.channel_list{:,3:4}*1e6;
params.SdrChannels_AudioGain = settings.AudioGain;
params.SDR_StartOnTrigger = settings.sdr_startontrigger;
params.DAQmx_ChannelCount = settings.daqmx_nch;
params.DAQmx_AIChannelList = settings.daqmx_channellist;
params.DAQmx_TriggerOutputTerminal = settings.daqmx_triggerterminal;
params.DAQmx_AITerminalConfig = settings.daqmx_terminalconfig;
params.DAQmx_SampleRate = 32e3;
params.DAQmx_MaxVoltage = settings.daqmx_maxvoltage;
params.DAQmx_AILowpassEnable = settings.daqmx_antialiasing;
params.DAQmx_AILowpassCutoffFreq = 0.45*params.DAQmx_SampleRate;
params.AudioOutput_DeviceIndex = -1;
params.DataFile_SamplePrecision = 'float32';

params.SDR_ExternalClock = settings.sdr_externalclock;
params.DAQmx_ExternalClock = settings.daqmx_externalclock;
params.DAQmx_ClockInputTerminal = settings.daqmx_clockterminal;



%% initalization

% save channel list
writetable(settings.channel_list, SdrChannelListFilename)

%spectrogram settings
spectrogram_window = 512;
spectrogram_noverlap = 0;
spectrogram_duration = 3; %in seconds
spectrogram_rate_div = round(sdrSpectrumPlotRate/spectrogramPlotRate);

nch = size(settings.channel_list,1);

% gui related

% sdr spectrum
nfft = params.Decimator1_InputFrameSize + ddc_filter.FilterOrder;
x_spec =fftshift(0:nfft-1); x_spec(1:floor(nfft/2))=x_spec(1:floor(nfft/2))-nfft; x_spec = (x_spec*fs_hf/nfft/1e6+settings.sdr_center_freq);
gui_handles.line_handle.Parent.XLim = [x_spec(1) x_spec(end)];
gui_handles.line_handle.XData = x_spec;
gui_handles.line_handle.YData = zeros(size(gui_handles.line_handle.XData));

% receive frequency (vertical lines)
gui_handles.line_handle2.YData = repmat([gui_handles.line_handle2.Parent.YLim NaN],1,nch);
gui_handles.line_handle2.XData = NaN(1,nch*3);


% audio spectrogram
gui_handles.spectrogram_handle.Parent.XLim = [-spectrogram_duration 0];
gui_handles.spectrogram_handle.Parent.YLim = [0 fs_lf/2]/1000;
gui_handles.spectrogram_handle.XData = gui_handles.spectrogram_handle.Parent.XLim;
gui_handles.spectrogram_handle.YData = gui_handles.spectrogram_handle.Parent.YLim;
gui_handles.spectrogram_handle.CData = abs(spectrogram(zeros(spectrogram_duration*fs_lf,1), spectrogram_window, spectrogram_noverlap));


% frequency tracking ranges
axes(gui_handles.patch_handle.Parent);
ch_lims_sorted = sortrows(settings.channel_list{:,[1 3 4]},2);
gui_handles.patch_handle.YData = [gui_handles.line_handle.Parent.YLim(1)*ones(2,nch);gui_handles.line_handle.Parent.YLim(2)*ones(2,nch)];
gui_handles.patch_handle.XData = [ch_lims_sorted(:,2)'; ch_lims_sorted(:,3)'; ch_lims_sorted(:,3)'; ch_lims_sorted(:,2)'];
gui_handles.patch_handle.CData = ones(nch,1); gui_handles.patch_handle.CData((1:2:nch)) = 0;

ctxt = num2cell(int2str(ch_lims_sorted(:,1)));
tx = sum(ch_lims_sorted(:,2:3)')/2;
ty = 3*ones(1,nch) + gui_handles.line_handle.Parent.YLim(2);
texthandles = text(tx,ty,ctxt,'HorizontalAlignment','center');

% udp
if settings.udp
    udph = udp(settings.udp_ip, settings.udp_port);
    fopen(udph);
end

%buffer for spectrogram
hbuff = dsp.Buffer(spectrogram_duration*fs_lf,spectrogram_duration*fs_lf-fs_lf/sdrSpectrumPlotRate);

%resampler for spectrogram
hSRC = dsp.SampleRateConverter('Bandwidth',bw_lf, 'InputSampleRate',params.DAQmx_SampleRate,'OutputSampleRate',fs_lf);
r = hSRC.step(single(zeros(params.DAQmx_SampleRate/sdrSpectrumPlotRate,1)));

if ~params.SDR_ExternalClock; gui_handles.pll_txt.String = '-'; end;

%% dsp loop
i_split = 0;
is_recording = true;
while is_recording
    %gui_handles.status_txt.String = 'Init...';
    
    if settings.split_files
        params.SdrChannelsFilename = [settings.outputfolder '\' settings.fn '_SdrChannels.' num2str(i_split) '.w64'];
        params.SdrSignalStrengthFilename = [settings.outputfolder '\' settings.fn '_SdrSignalStrength.' num2str(i_split) '.w64'];
        params.SdrCarrierFreqFilename = [settings.outputfolder '\' settings.fn '_SdrCarrierFreq.' num2str(i_split) '.w64'];
        params.SdrReceiveFreqFilename = [settings.outputfolder '\' settings.fn '_SdrReceiveFreq.' num2str(i_split) '.w64'];
        params.DAQmxChannelsFilename = [settings.outputfolder '\' settings.fn '_DAQmxChannels.' num2str(i_split) '.w64'];
    end
    
    % init stream
    sdrBirdrecBackend.initRec(params);
    hbuff.reset();
    
    sdrBirdrecBackend.setSquelch(monitor_settings.squelch_level);
    sdrBirdrecBackend.setChannel(monitor_settings.channel_type, monitor_settings.channel_number);
    sdrBirdrecBackend.setPlayAudio(monitor_settings.play_audio);

    gui_handles.status_txt.String = 'Recording...';
    drawnow;

    % send udp start trigger 
    if settings.udp; fprintf(udph, udp_start_msg); end


    try
        i_frame = int64(0);
        sdrBirdrecBackend.startRec();
        while is_recording && (~settings.split_files || i_frame < sdrSpectrumPlotRate * settings.file_duration * 60)

            frame = sdrBirdrecBackend.getMonitorDataFrame();

            %plotting
            gui_handles.line_handle.YData = frame.sdr_spectrum;
            x = frame.receive_frequencies(:,[1, 1, 1])'*1e-6;
            gui_handles.line_handle2.XData = x(:);
            current_monitor_settings = monitor_settings;

            if current_monitor_settings.show_spectrogram
                if strcmpi(frame.channel_type, 'sdr')
                    d = hbuff.step(frame.output_signal);
                else %daqmx
                    r = hSRC.step(frame.output_signal);
                    d = hbuff.step(r);
                end
                if mod(i_frame,spectrogram_rate_div)==0;
                    s = (log(abs(spectrogram(d,spectrogram_window,spectrogram_noverlap)))+current_monitor_settings.spectrogram_offset)*current_monitor_settings.spectrogram_scaling;
                    gui_handles.spectrogram_handle.CData = s;
                end
            end

            if params.SDR_ExternalClock
                if sdrBirdrecBackend.isRefPLLlocked()
                    gui_handles.pll_txt.String = 'locked';
                else
                    gui_handles.pll_txt.String = 'unlocked';
                end
            end

            drawnow;

            i_frame = i_frame+1;
        end


    catch ME
        sdrBirdrecBackend.stopStream();
        delete(texthandles);
        if settings.udp; fclose(udph); end
        ME.rethrow;
    end

    sdrBirdrecBackend.stopRec();
    
    if settings.udp; fprintf(udph, udp_stop_msg); end % send udp stop trigger 
    
    i_split = i_split+1;
end
gui_handles.status_txt.String = 'Ready.';
delete(texthandles);
if settings.udp; fclose(udph); end


end

