function sdr_recorder(settings, gui_handles)
%SDR_RECORDER 
%   receive radio data and demodulate channels

if strcmpi(settings.radio,'USRP')
    radio = 1;
else %RTL SDR
    radio = 2;
end

%radio = 3;
%rawfile = '8s.mat';
%rawfile = 'C:\2016-01-13 SDRBirdrec vs zhInstruments comparison\SDRBirdrec\recraw1301.mat';

%% variables

%parameters
fs_lf = 24000; %samplerate of output signal
bw_lf = 20000; %two sideded bandwith of output signal
bw_if = 200e3; %filter bandwith for decimater
%bw_if = 2*200e3; %filter bandwith for decimater

if radio==1 %usrp
    decf = [22,10]; %decimation factors
    
    
    frameSize = fs_lf*prod(decf)/5; %samples per frame
       
    nfft=2^12;
elseif radio==2 %rtl sdr 
    decf = [10,10]; %decimation factors
    frameSize = 2.56e5; %samples per frame

    nfft=2^12;
else %file
    decf = [22,10]; %decimation factors
    %decf = [11,20]; %decimation factors
    
    frameSize = fs_lf*prod(decf)/5; %samples per frame
       
    nfft=2^12;
end
fs_if= fs_lf*decf(2); %samplerate after freq. shifting
fs_hf = fs_if*decf(1); %sdr samplerate
frameRate = fs_hf/frameSize; %frames per second



%global vars (the global vars are updated in the gui)
global is_recording;
global monitor_settings;

%% initalization

%spectrogram settings
spec_stepsize = 300;
spec_timelength = 3;
spec_rate_div = frameRate;

% dsp related
fcs = ((settings.channel_list{:,4}-settings.channel_list{:,3})/2-settings.sdr_center_freq)*1e6;
fcs = round(fcs/frameRate)*frameRate;
fcs = fcs(:)';

nch = size(settings.channel_list,1);

% gui related
x_spec =fftshift(0:nfft-1); x_spec(1:floor(nfft/2))=x_spec(1:floor(nfft/2))-nfft; x_spec = (x_spec*fs_hf/nfft/1e6+settings.sdr_center_freq);
gui_handles.line_handle.Parent.XLim = [x_spec(1) x_spec(end)];
gui_handles.line_handle.XData = x_spec;
gui_handles.line_handle.YData = zeros(size(gui_handles.line_handle.XData));

gui_handles.line_handle2.YData = repmat([gui_handles.line_handle2.Parent.YLim NaN],1,nch);
gui_handles.line_handle2.XData = NaN(1,nch*3);

gui_handles.spectrogram_handle.Parent.XLim = [-spec_timelength 0];
gui_handles.spectrogram_handle.Parent.YLim = [0 fs_lf/2]/1000;
gui_handles.spectrogram_handle.XData = gui_handles.spectrogram_handle.Parent.XLim;
gui_handles.spectrogram_handle.YData = gui_handles.spectrogram_handle.Parent.YLim;


axes(gui_handles.patch_handle.Parent);
ch_lims_sorted = sortrows(settings.channel_list{:,[1 3 4]},2);
gui_handles.patch_handle.YData = [gui_handles.line_handle.Parent.YLim(1)*ones(2,nch);gui_handles.line_handle.Parent.YLim(2)*ones(2,nch)];
gui_handles.patch_handle.XData = [ch_lims_sorted(:,2)'; ch_lims_sorted(:,3)'; ch_lims_sorted(:,3)'; ch_lims_sorted(:,2)'];
gui_handles.patch_handle.CData = ones(nch,1); gui_handles.patch_handle.CData((1:2:nch)) = 0;

ctxt = num2cell(int2str(ch_lims_sorted(:,1)));
tx = sum(ch_lims_sorted(:,2:3)')/2;
ty = 3*ones(1,nch);
texthandles = text(tx,ty,ctxt,'HorizontalAlignment','center');
    

gui_handles.status_txt.String = 'Initialization...';
drawnow;

  
%% init filters and dsp function
filterfile = [fileparts(mfilename('fullpath')) filesep 'filters.mat'];


if exist(filterfile,'file')
    load(filterfile);
else
    %ddc_filter     
    %ddc_filter = designfilt('lowpassfir', 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2, 'PassbandRipple', 10, 'StopbandAttenuation', 80, 'SampleRate', fs_hf);
    %ddc_filter = designfilt('lowpassfir', 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2, 'PassbandRipple', 0.1, 'StopbandAttenuation', 80, 'SampleRate', fs_hf);
    ddc_filter = designfilt('lowpassfir', 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2, 'PassbandRipple', 10, 'StopbandAttenuation', 60, 'SampleRate', fs_hf);
    
    %afc_filter
    %afc_filter = designfilt('lowpassiir', 'PassbandFrequency', 1, 'StopbandFrequency', 10, 'PassbandRipple', 1, 'StopbandAttenuation', 80, 'SampleRate', fs_if);

    %lf_filter                        
    lf_filter = designfilt('lowpassfir', 'PassbandFrequency', bw_lf/2, 'StopbandFrequency', fs_lf/2, 'PassbandRipple', 0.1, 'StopbandAttenuation', 80, 'SampleRate', fs_if);

    %highpass filter                    
    hp_filter = designfilt('highpassiir', 'StopbandFrequency', 15, 'PassbandFrequency', 50, 'StopbandAttenuation', 80, 'PassbandRipple', 0.5, 'SampleRate', fs_lf);
    
    %save(filterfile,'ddc_filter','afc_filter','lf_filter','hp_filter');
    save(filterfile,'ddc_filter','lf_filter','hp_filter');
end

% if settings.AFC
%     pllddcdemod(1,fcs,decf,frameSize,fs_hf,ddc_filter.Coefficients,settings.AudioGain*lf_filter.Coefficients,hp_filter.Coefficients,afc_filter.Coefficients,settings.LoopGain);
% else
%     pllddcdemod(1,fcs,decf,frameSize,fs_hf,ddc_filter.Coefficients,settings.AudioGain*lf_filter.Coefficients,hp_filter.Coefficients);
% end

pllddcdemod(1,fcs,decf,frameSize,fs_hf,ddc_filter.Coefficients,settings.AudioGain*lf_filter.Coefficients,hp_filter.Coefficients);
       
%init system objects
hAudioOut = dsp.AudioPlayer('SampleRate',fs_lf, 'BufferSizeSource', 'property','QueueDuration',0/frameRate);

hmfw = dsp.MatFileWriter('Filename', settings.full_fn, 'VariableName', 'data');
if exist(hmfw.Filename,'file'), delete(hmfw.Filename); end;

%temporary file for log
hmfw_log = dsp.MatFileWriter('Filename', [tempname '.mat'], 'VariableName', 'log');

if settings.wallmic
    hAudioIn = dsp.AudioRecorder('DeviceName', settings.wallmic_device, 'SampleRate', fs_lf,'OutputDataType', 'int16', 'NumChannels', 1, 'BufferSizeSource', 'property', 'SamplesPerFrame', fs_lf/frameRate, 'QueueDuration',0/frameRate);
    do_offset = sign(settings.wallmic_offset);
    hDelay = dsp.Delay('Length', abs(settings.wallmic_offset));
    %init hDelay
    if do_offset == -1
        data6 = hDelay.step(int16(zeros(fs_lf/frameRate,nch)));
    elseif do_offset == 1
        wallmicdata = hDelay.step(int16(zeros(fs_lf/frameRate,1)));
    end
end

if settings.udp
    udph = udp(settings.udp_ip, settings.udp_port);
    fopen(udph);
end

%buffer for spectrogram
hbuff = dsp.Buffer(spec_timelength*fs_lf,spec_timelength*fs_lf-fs_lf/frameRate);


%fft for spectrum
hFFT = dsp.FFT('Normalize',false,'FFTLengthSource','auto');
hFFT.step(complex(zeros(nfft,1),zeros(nfft,1)));



%init radio
if radio==1 %USRP
    if settings.AGC
        gain = -1;
    else
        gain = settings.TunerGain;
    end
    flag = usrp(2,fs_hf*10,10,frameSize,settings.sdr_center_freq*1e6,gain);
    if flag < 0 %couldn't init sdr
        release(hAudioOut);
        hmfw.release;
        hmfw_log.release;
        if settings.udp
            fclose(udph);
        end
        hFFT.release;
        
        delete(texthandles);
        throw(MException('E:InitError','Couldn''t init SDR Device.'));
    end

   
elseif radio==2 %RTL SDR
    sdrsetup;
    ds = comm.SDRRTLReceiver(...
          'CenterFrequency', settings.sdr_center_freq*1e6, ...
          'SampleRate',      fs_hf, ...
          'EnableTunerAGC',  settings.AGC, ...
          'TunerGain',  settings.TunerGain, ...
          'SamplesPerFrame', frameSize, ...
          'OutputDataType',  'int16');

else %from file
    hmfr = dsp.MatFileReader('Filename', rawfile, 'VariableName', 'data', 'SamplesPerFrame', frameSize);
end

%calc borders
%boarders=zeros(length(fcs),2);
% for k=1:length(fcs)
%     if k>1
%         boarders(k,1) = boarders(k-1,2)+1;
%     else
%         boarders(k,1)=1;
%     end
%     
%     if k<length(fcs)
%         boarders(k,2)=floor((fs_hf+fcs(k)+fcs(k+1))*nfft/(2*fs_hf));
%     else
%         boarders(k,2)=nfft;
%     end
% end
boarders = round(((settings.channel_list{:,3:4}-settings.sdr_center_freq)*1e6+fs_hf/2)*nfft/fs_hf+1);
signal_level = zeros(1,nch);
signal_level(nch+1) = Inf;
    
if settings.wallmic
    wallmicdata = hAudioIn.step();
end

rec_onset_time = clock;

gui_handles.status_txt.String = 'Recording...';
drawnow;
if settings.udp
    fprintf(udph, settings.udp_msg);
end
if settings.wallmic
    wallmicdata = hAudioIn.step();
end
if radio==1
    usrp(1); %start stream
elseif radio==2
    [data_s, ~, lost] = ds.step;
    data = double(data_s)/128;
end

%% dsp loop
i = int64(0);
try
while is_recording
    
    
    
    
    if radio==1 %USRP 
        [data,lost,error] = usrp(0);
        
        if error
            fprintf('Internal error because of overrun at %.2fs. Recording is aborted!\n',i/frameRate);
            is_recording = false;
            break;
        end
        
        if lost>0, fprintf('lost samples at %.2fs\n',i/frameRate); end;

        if length(data)~=frameSize
            fprintf('lost samples at %.2fs (length doesn''t match)\n',i/frameRate);
            drawnow
            continue;
        end
    elseif radio==2 %RTL SDR
        [data_s, ~, lost] = ds.step;
        data = double(data_s)/128;
        if lost>0, warning('lost samples at %.2fs',i/frameRate); end;
    else
        if hmfr.isDone
            is_recording = false;
            break;
        end
            
        d = hmfr.step;
        data = double(complex(d.real,d.imag))/2^15;
    end
    
    if settings.wallmic
        wallmicdata = hAudioIn.step();
    end
    
    spec = 20*log10(abs(fftshift(hFFT.step(data(1:nfft))))/nfft); 
    %spec = 20*log10(abs(fftshift(fft(data(1:nfft))))/nfft); 
    gui_handles.line_handle.YData = spec;
    
    nfcs = NaN(nch,1);
    for k=1:nch

        [signal_level(k),mindex]=max(spec(boarders(k,1):boarders(k,2)));
        
        mf = (boarders(k,1)+mindex-1)*fs_hf/nfft-fs_hf/2;
        if abs(mf-fcs(k))>bw_if/4
            fcs(k)= mf;
            nfcs(k) =mf;
            %fprintf('%s: carrier shift on channel %i to %.3f MHz at %fs\n',datestr(now),k,nfcs(k)*1e-6+settings.sdr_center_freq,i/frameRate);
        end
    end

    [data6, cfreqs] = pllddcdemod(0,data,nfcs);
    
    if settings.wallmic
        %apply offset
        if do_offset == -1
            data6 = hDelay.step(data6);
        elseif do_offset == 1
            wallmicdata = hDelay.step(wallmicdata);
        end
        %add wallmic data
        data7 = [data6,wallmicdata];
    else
        data7 = data6;
    end
    
    current_monitor_settings = monitor_settings;
    
    if current_monitor_settings.play && signal_level(current_monitor_settings.channel) >= current_monitor_settings.squelch_level
        hAudioOut.step(data7(:,current_monitor_settings.channel));
    end
    
    if current_monitor_settings.show_spectrogram
        x = hbuff.step(double(data7(:,current_monitor_settings.channel)));
        if mod(i,spec_rate_div)==0;
            s = log(abs(spectrogram(x,2*spec_stepsize,spec_stepsize)))*current_monitor_settings.spectrogram_scaling+current_monitor_settings.spectrogram_offset;
            gui_handles.spectrogram_handle.CData = s;
        end
    end
    
    
    cfreqs = cfreqs*1e-6+settings.sdr_center_freq;
    x = cfreqs([1;1;1;],:);
    gui_handles.line_handle2.XData = x(:);
    
    hmfw.step(data7);
    hmfw_log.step(single([lost,fcs,signal_level(1:nch)]));

    drawnow;
    
    %fprintf('%i: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n',i,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11); 
    i = i+1;
end

release(hAudioOut);

if radio==1
    usrp(-1);
elseif radio==2
    release(ds);
else
    hmfr.release;
end

if settings.wallmic
    hAudioIn.release;
    hDelay.release;
end
if settings.udp
    fclose(udph);
end
hFFT.release;

hmfw.release;
hmfw_log.release;

data_file = matfile(hmfw.Filename,'Writable',true);
log_file = matfile(hmfw_log.Filename,'Writable',false);

data_file.fs = fs_lf;
data_file.rec_onset_time = rec_onset_time;
data_file.center_freq = settings.sdr_center_freq;
data_file.AGC = settings.AGC;
data_file.log_fs = frameRate;
data_file.lost = log_file.log(:,1);
data_file.channel_freqs = log_file.log(:,2:nch+1)*single(1e-6)+single(settings.sdr_center_freq);
data_file.channel_levels = log_file.log(:,nch+2:2*nch+1);
data_file.bird_names = settings.channel_list{:,2};

delete(hmfw_log.Filename);

delete(texthandles);

catch ME
    ME
    
    release(hAudioOut);

    if radio==1
        usrp(-1);
    elseif radio==2
        release(ds);
    else
        hmfr.release;
    end
    
    if settings.wallmic
        hAudioIn.release;
        hDelay.release;
    end
    if settings.udp
        fclose(udph);
    end
    hFFT.release;

    hmfw.release;
    hmfw_log.release;
    delete(hmfw_log.Filename);
        
    delete(texthandles);
    
    ME.rethrow;
    
end


end

