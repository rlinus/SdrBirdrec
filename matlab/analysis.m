fs = 240000;
afc_filter = designfilt('lowpassiir', ...
                        'PassbandFrequency', 1, ...
                        'StopbandFrequency', 10, ...
                        'PassbandRipple', 1, ...
                        'StopbandAttenuation', 80, ...
                        'SampleRate', fs);

[z,p,k] = sos2zp(afc_filter.Coefficients);

loopfilter = zpk(z,p,k,fs^-1);

z = tf('z');

%gl = c('loopgain',5*10^-6);
gl = 1.0199997809501*10^-6;
gl = 0.906*10^-6;
gl = 5*10^-6;
d=floor(456/22);
%d=0;


ramp = z/(z-1)^2;


% for i =6:6
%     %gl = 10^-i;
%     H = (z^(d)*(z-1))/(z^(d)*(z-1)+gl*loopfilter);
%     p=bodeplot(H);
%     setoptions(p,'FreqUnits','Hz','FreqScale','log');
%     %figure(1);
%     %stepplot(H,20);
%     %figure(2);
%     %impulse((z^(d+1)/(z-1))/(z^(d)*(z-1)+gl*loopfilter)/fs,20);
%     hold on;
% end


gl = 10^-5;
H = (z^(d)*(z-1))/(z^(d)*(z-1)+gl*loopfilter);

figure(1); hold on;
p=bodeplot(H);
setoptions(p,'FreqUnits','Hz','FreqScale','log','Xlim',[0.01 fs/2]);

figure(2); hold on;
stepplot(H,10);

gl = 5*10^-6;
H = (z^(d)*(z-1))/(z^(d)*(z-1)+gl*loopfilter);

figure(1);
p=bodeplot(H);
setoptions(p,'FreqUnits','Hz','FreqScale','log','Xlim',[0.01 fs/2]);

figure(2);
stepplot(H,10);

gl = 10^-6;
H = (z^(d)*(z-1))/(z^(d)*(z-1)+gl*loopfilter);

figure(1);
p=bodeplot(H);
setoptions(p,'FreqUnits','Hz','FreqScale','log','Xlim',[0.01 fs/2]);

figure(2);
stepplot(H,10);

figure(1);
I=legend('g_L = 10^{-5}','g_L = 5*10^{-6}','g_L = 10^{-6}');
set(I,'FontSize',12);

figure(2);
I=legend('g_L = 10^{-5}','g_L = 5*10^{-6}','g_L = 10^{-6}');
set(I,'FontSize',12);
 