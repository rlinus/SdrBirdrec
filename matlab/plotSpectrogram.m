fn = 'C:\Users\linus\Google Drive\SdrBirdrec Recordings\rec0702\rec0702_SdrChannels.0.w64';

[sdrChans,fs] = audioread(fn);

iChan = 3;

startTime = 60*50+120;
frameDuration = 60;

window = 256;
noverlap = [];
nfft = 256;

frame = sdrChans(startTime*fs+1:(startTime+frameDuration)*fs,iChan);

soundsc(frame,fs);
spectrogram(frame,window,noverlap,nfft,fs,'yaxis');