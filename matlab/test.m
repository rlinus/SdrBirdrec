%cd C://Users/linus/Repositories/; addpath SdrBirdrec/SdrBirdrecMex; addpath SdrBirdrec/x64/Release; addpath SdrBirdrec/matlab;

%%
clear h; clear SdrBirdrecRecorder; clear SdrBirdrecMex;

list = SdrBirdrecRecorder.findSdrDevices();
devNumber = 3;
list{devNumber}
h= SdrBirdrecRecorder(list{devNumber});

%% parameters
fs_lf = 24000; %samplerate of output signal
bw_lf = 20000; %two sideded bandwith of output signal
bw_if = 200e3; %filter bandwith for decimater

sdrSpectrumPlotRate = 5; %how often to plot per second
Decimator1_Factor = 20;
Decimator2_Factor = 10;
trackingSampleRate = 20;
frameRate = 20; %frames per second

frameSize = fs_lf/frameRate; %output samples per frame
fs_if= fs_lf*Decimator2_Factor; %samplerate after freq. shifting
fs_hf = fs_if*Decimator1_Factor; %sdr samplerate

Decimator1FramesPerTrackingFrame = 40;

ddc_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 400, 'SampleRate', fs_hf, 'PassbandFrequency', bw_if/2, 'StopbandFrequency', fs_if/2);

lp_filter = designfilt('lowpassfir', 'DesignMethod', 'ls', 'FilterOrder', 250, 'SampleRate', fs_if, 'PassbandFrequency', bw_lf/2, 'StopbandFrequency', fs_lf/2);

% hp_filter = designfilt('highpassiir', ...
%     'SampleRate', fs_lf, ...
%     'StopbandFrequency', 15, ...
%     'PassbandFrequency', 50, ...
%     'StopbandAttenuation', 80, ...
%     'PassbandRipple', 0.5);

params.MonitorRate = sdrSpectrumPlotRate;
params.SDR_SampleRate = fs_hf;
params.SDR_CenterFrequency = 300 * 1e6;
params.SDR_AGC = false;
params.SDR_Gain = 30;
params.SDR_TrackingRate = trackingSampleRate;
params.Decimator1_InputFrameSize = (fs_hf/trackingSampleRate)/Decimator1FramesPerTrackingFrame;
params.Decimator1_Factor = Decimator1_Factor;
params.Decimator2_Factor = Decimator2_Factor;
params.Decimator1_FirFilterCoeffs = ddc_filter.Coefficients;
params.Decimator2_FirFilterCoeffs = lp_filter.Coefficients;
params.IirFilterCoeffs = []; %hp_filter.Coefficients;


params.SDR_ChannelBands = [299, 300; 300.1, 300.6]*1e6;
params.SdrChannelsFilename = ['E:\SDR_Birdrec_testrec' '\test.w64'];
%%
h.initStream(params);
h.startStream();
frame = h.getData();

%%
h.stopStream();
