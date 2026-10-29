function results = anc_dsp_visualizer(outputDir)
% UNIVERSAL ANC: reproducible DSP experiments, base MATLAB only.
% All attenuation values are simulated at an error sensor, not measured at an ear.
% Run: addpath('matlab'); results = anc_dsp_visualizer;
if nargin < 1
    outputDir = fullfile(fileparts(fileparts(mfilename('fullpath'))),'results','matlab');
end
if ~exist(outputDir,'dir'), mkdir(outputDir); end
fs=8000; N=48000; L=64; mu=0.02; leak=1e-6;
names={'broadband','hum_drone','fan','uncorrelated','mismatch','wired_delay','wireless_delay','path_change'};
attenuation=zeros(8,1); early=attenuation; peaks=attenuation; clips=attenuation;
allRuns=cell(8,1); allWeights=cell(8,1);
oldVisibility=get(groot,'defaultFigureVisible');
cleanup=onCleanup(@()set(groot,'defaultFigureVisible',oldVisibility)); %#ok<NASGU>
set(groot,'defaultFigureVisible','off');
for scenario=1:numel(names)
    name=names{scenario}; delay=8;
    if strcmp(name,'wired_delay'), delay=40; end
    if strcmp(name,'wireless_delay'), delay=400; end
    P=[zeros(24,1);0.65;0.15;-0.05]; S=[zeros(delay,1);0.8;-0.15;0.05]; Shat=S;
    if strcmp(name,'mismatch'), Shat(delay+1)=0.65;Shat(delay+2)=-0.05;end
    % Portable LCG: same generator and draw order as the C++ simulation.
    white=zeros(N,1); independent=white; state=42;
    for n=1:N
        state=mod(1664525*state+1013904223,2^32);white(n)=2*state/2^32-1;
        state=mod(1664525*state+1013904223,2^32);independent(n)=2*state/2^32-1;
    end
    colored=filter(0.15,[1 -0.85],white); t=(0:N-1)'/fs;
    x=0.08*white;
    if strcmp(name,'hum_drone'),x=0.055*sin(2*pi*120*t)+0.025*sin(2*pi*240*t)+0.015*colored;end
    if strcmp(name,'fan'), x=0.12*colored;end
    d=filter(P,1,x);if strcmp(name,'uncorrelated'),d=d+0.04*independent;end
    xf=filter(Shat,1,x); w=zeros(L,1); xbuf=w; fbuf=w; sbuf=zeros(length(S),1);
    y=zeros(N,1);e=y;wnorm=y;clipCount=0;
    for n=1:N
        if strcmp(name,'path_change') && n==N/2+1
            S(delay+1)=0.6;S(delay+2)=-0.08;sbuf(:)=0;
        end
        xbuf=[x(n);xbuf(1:end-1)];fbuf=[xf(n);fbuf(1:end-1)];
        raw=w'*xbuf;y(n)=max(-0.2,min(0.2,raw));
        sbuf=[y(n);sbuf(1:end-1)];e(n)=d(n)+S'*sbuf;
        if abs(raw)<0.2
            w=(1-leak)*w-mu*e(n)*fbuf/(1e-8+fbuf'*fbuf);
        else
            clipCount=clipCount+1;
        end
        wnorm(n)=norm(w);
    end
    assert(all(isfinite(e)) && all(isfinite(w)),'Nonfinite controller');
    tail=(floor(0.75*N)+1):N; head=1:fs/2;
    attenuation(scenario)=dbRatio(d(tail),e(tail));early(scenario)=dbRatio(d(head),e(head));
    peaks(scenario)=max(abs(y));clips(scenario)=clipCount;
    allRuns{scenario}=[x d y e wnorm];allWeights{scenario}=w;
    writetable(table((0:N-1)',x,d,y,e,'VariableNames',{'sample','reference','baseline','control','residual'}),fullfile(outputDir,[name '.csv']));
    fprintf('%-16s %8.3f dB simulated final-window attenuation\n',name,attenuation(scenario));
end
results=table(names',attenuation,early,peaks,clips,'VariableNames',{'scenario','attenuation_db','early_attenuation_db','output_peak','clipped_samples'});
writetable(results,fullfile(outputDir,'summary.csv'));
save(fullfile(outputDir,'experiments.mat'),'results','allRuns','allWeights','fs','L','mu','leak');
% Independent secondary path identification, training / held-out validation.
rng(7,'twister');probe=0.1*randn(N,1); S=[zeros(8,1);0.8;-0.15;0.05]; measured=filter(S,1,probe)+0.0001*randn(N,1);
sh=zeros(32,1);buf=sh;err=zeros(N,1); split=floor(0.75*N);
for n=1:split
    buf=[probe(n);buf(1:end-1)];err(n)=measured(n)-sh'*buf;sh=sh+0.3*err(n)*buf/(1e-8+buf'*buf);
end
predicted=filter(sh,1,probe); idNmse=10*log10(sum((measured(split+1:end)-predicted(split+1:end)).^2)/sum(measured(split+1:end).^2));
assert(idNmse < -30,'Secondary path identification failed');
writetable(table(probe,measured,'VariableNames',{'probe','response'}),fullfile(outputDir,'calibration_recording.csv'));
writetable(table(idNmse,'VariableNames',{'validation_nmse_db'}),fullfile(outputDir,'calibration_summary.csv'));
% 1: FIR window method, impulse response, magnitude and linear-phase group delay.
M=63;k=(-(M-1)/2:(M-1)/2)';fc=700;h=zeros(M,1);h(k==0)=2*fc/fs;idx=k~=0;h(idx)=sin(2*pi*fc/fs*k(idx))./(pi*k(idx));
h=h.*(0.54-0.46*cos(2*pi*(0:M-1)'/(M-1)));h=h/sum(h);[f,H]=response(h,1,fs);
fig=figure; tiledlayout(2,2);nexttile;stem(0:M-1,h,'.');title('63-tap windowed-sinc FIR');xlabel('Sample');nexttile;plot(f,20*log10(abs(H)+1e-12));xlim([0 2000]);ylim([-100 5]);title('Magnitude');ylabel('dB');xlabel('Hz');nexttile;plot(f,unwrap(angle(H)));title('Phase');xlabel('Hz');ylabel('rad');nexttile;plot(f(2:end),-diff(unwrap(angle(H)))/(2*pi*(f(2)-f(1))/fs));ylim([0 65]);xlim([0 700]);title('Passband group delay: 31 samples');xlabel('Hz');ylabel('samples');exportFig(fig,outputDir,'01_FIR_Filter');
% 2: DC blocker and IIR low-pass; these demonstrations are not hidden in S(z).
r=exp(-2*pi*10/fs);[f,Hdc]=response([1 -1],[1 -r],fs);[b,a]=lowpassCoefficients(fs,700);[~,Hiir]=response(b,a,fs);
fig=figure;tiledlayout(2,1);nexttile;plot(f,20*log10(abs(Hdc)+1e-12));xlim([0 300]);ylim([-60 5]);title('DC blocker: (1-z^{-1})/(1-rz^{-1})');ylabel('dB');xlabel('Hz');nexttile;plot(f,20*log10(abs(Hiir)+1e-12));xlim([0 3000]);ylim([-80 5]);title('Second-order Butterworth low-pass: 700 Hz');ylabel('dB');xlabel('Hz');exportFig(fig,outputDir,'02_DC_Blocker_and_Biquad');
% 3: Sub-band spectrum (non-overlapping diagnostic frequency bins).
run=allRuns{1};[fPsd,psdD]=welchPsd(run(tail,2),fs);[~,psdE]=welchPsd(run(tail,4),fs);
edges=[50 150 350 750 1500];bandDb=zeros(4,1);
for j=1:4,band=fPsd>=edges(j)&fPsd<edges(j+1);bandDb(j)=10*log10(sum(psdD(band))/max(sum(psdE(band)),1e-20));end
writetable(table(edges(1:4)',edges(2:5)',bandDb,'VariableNames',{'low_hz','high_hz','attenuation_db'}),fullfile(outputDir,'bands.csv'));
fig=figure;bar(categorical({'50-150','150-350','350-750','750-1500'}),bandDb);ylabel('Simulated attenuation (dB)');xlabel('Band (Hz)');title('Broadband: integrated PSD band power');exportFig(fig,outputDir,'03_SubBands');
fig=figure;tiledlayout(2,1);nexttile;plot((0:split-1)/fs,10*log10(movmean(err(1:split).^2,400)+1e-14));title('NLMS secondary-path training error');ylabel('dBFS power');xlabel('s');nexttile;stem(0:31,sh,'.');hold on;stem(0:length(S)-1,S,'x');legend('identified','true');title(sprintf('Held-out identification NMSE %.1f dB',idNmse));xlabel('Tap');exportFig(fig,outputDir,'04_NLMS_and_SecondaryPath');
fig=figure;[f,Hs]=response(S,1,fs);[~,Hsh]=response(sh,1,fs);tiledlayout(2,1);nexttile;plot(f,20*log10(abs([Hs Hsh])+1e-12));legend('S','S estimate');ylabel('dB');xlabel('Hz');title('Secondary path frequency response');nexttile;plot(f,unwrap(angle([Hs Hsh])));ylabel('rad');xlabel('Hz');exportFig(fig,outputDir,'05_SecondaryPath_Phase');
fig=figure;tiledlayout(2,1);nexttile;plot(t,10*log10(movmean(run(:,2).^2,800)+1e-14));hold on;plot(t,10*log10(movmean(run(:,4).^2,800)+1e-14));legend('ANC off baseline','ANC on residual');xlabel('s');ylabel('dBFS power');title('Normalized filtered-x LMS convergence');nexttile;plot(t,run(:,5));xlabel('s');ylabel('L2 norm');title('Controller coefficients norm');exportFig(fig,outputDir,'06_FxLMS_Convergence');
fig=figure;plot(fPsd,10*log10(psdD+1e-20));hold on;plot(fPsd,10*log10(psdE+1e-20));xlabel('Hz');ylabel('PSD (dBFS/Hz)');legend('baseline','residual');title('Final-window Welch PSD, 512 samples, 50% overlap');exportFig(fig,outputDir,'07_Spectral_Comparison');
fig=figure;bar(categorical(names,names),attenuation);xtickangle(30);ylabel('Simulated attenuation (dB)');title('Causality and robustness: negative values mean amplification');exportFig(fig,outputDir,'08_Scenarios_and_Latency');
fig=figure;tiledlayout(2,1);nexttile;plot(t,run(:,3));yline(0.2,'r--');yline(-0.2,'r--');xlabel('s');ylabel('FS');title('Output clamp and adaptation freeze at 0.2 FS');nexttile;bar(categorical(names,names),clips);xtickangle(30);ylabel('samples');title('Clipped samples (must be zero in nominal experiment)');exportFig(fig,outputDir,'09_Stability_Monitor');
% Audio files use the same gain, preserving the baseline/residual comparison.
audiowrite(fullfile(outputDir,'baseline.wav'),allRuns{2}(:,2),fs);audiowrite(fullfile(outputDir,'residual.wav'),allRuns{2}(:,4),fs);
assert(attenuation(1)>20 && attenuation(2)>15 && attenuation(6)<3 && attenuation(7)<3,'Convergence/causality acceptance criteria failed');
assert(all(clips==0),'Unexpected output clipping');
% Compare independent MATLAB and C++ calculations when C++ traces exist.
cppDir=fullfile(fileparts(fileparts(mfilename('fullpath'))),'results','cpp');
if exist(fullfile(cppDir,'broadband.csv'),'file')
    cpp=readtable(fullfile(cppDir,'broadband.csv')); delta=max(abs(cpp.residual-allRuns{1}(:,4)));
    assert(delta<1e-4,'C++/MATLAB sample agreement failed');
    writetable(table(delta,'VariableNames',{'max_residual_absolute_difference'}),fullfile(outputDir,'cpp_agreement.csv'));
end
anc_fundamentals(outputDir);
disp(results);fprintf('Secondary path validation NMSE: %.3f dB\nResults: %s\n',idNmse,outputDir);
end
function d=dbRatio(x,y)
d=10*log10(max(sum(x.^2),1e-20)/max(sum(y.^2),1e-20));
end
function [f,H]=response(b,a,fs)
n=8192;H=fft(b(:),n,1)./fft(a(:),n,1);H=H(1:n/2+1);f=(0:n/2)'*fs/n;
end
function [b,a]=lowpassCoefficients(fs,fc)
w=2*pi*fc/fs;c=cos(w);alpha=sin(w)/(2/sqrt(2));b=[(1-c)/2 1-c (1-c)/2]/(1+alpha);a=[1 -2*c/(1+alpha) (1-alpha)/(1+alpha)];
end
function [f,p]=welchPsd(x,fs)
nfft=512;win=0.5-0.5*cos(2*pi*(0:nfft-1)'/nfft);p=zeros(nfft/2+1,1);count=0;
for n=1:nfft/2:length(x)-nfft+1
    segment=x(n:n+nfft-1);segment=segment-mean(segment);spec=fft(segment.*win,nfft);q=abs(spec(1:nfft/2+1)).^2/(fs*sum(win.^2));q(2:end-1)=2*q(2:end-1);p=p+q;count=count+1;
end
p=p/count;f=(0:nfft/2)'*fs/nfft;
end
function exportFig(fig,directory,name)
if exist('theme','file'), theme(fig,'light'); end
set(fig,'Position',[100 100 1200 750],'Color','w');exportgraphics(fig,fullfile(directory,[name '.png']),'Resolution',130);savefig(fig,fullfile(directory,[name '.fig']));close(fig);
end
