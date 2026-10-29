function anc_fundamentals(outputDir)
% Sampling, quantization, multirate filtering, and phase-delay sensitivity.
fs=8000; n=(0:7999)'; tone=0.1*sin(2*pi*123*n/fs);
q=round(tone*32768)/32768; quantizationSnr=10*log10(sum(tone.^2)/sum((tone-q).^2));
assert(quantizationSnr>70 && max(abs(tone-q))<=1/65536+eps);
fig=figure('Visible','off');tiledlayout(2,1);
nexttile;plot(n(1:100)/fs,tone(1:100));hold on;stairs(n(1:100)/fs,q(1:100));xlabel('s');ylabel('FS');title('PCM16 quantization, identical gain');legend('float','PCM16');
nexttile;histogram(tone-q,40);xlabel('Quantization error (FS)');ylabel('count');title(sprintf('Measured quantization SNR %.2f dB for 0.1 FS tone',quantizationSnr));savePlot(fig,outputDir,'10_Quantization');
% Sample a 5.5 kHz sine at 8 kHz: aliases to 2.5 kHz without analog anti-alias filtering.
time=(0:2047)'/fs;aliased=sin(2*pi*5500*time);Y=abs(fft(aliased));f=(0:1024)'*fs/2048;
fig=figure('Visible','off');plot(f,Y(1:1025));xlabel('Hz');ylabel('DFT magnitude');title('Aliasing: 5500 Hz sampled at 8000 Hz appears at 2500 Hz');savePlot(fig,outputDir,'11_Sampling_and_Aliasing');
% Offline 48 -> 8 kHz decimation with explicit FIR anti-alias low-pass.
highFs=48000;t=(0:2*highFs-1)'/highFs;input=0.1*sin(2*pi*600*t)+0.1*sin(2*pi*6000*t);
M=127;k=(-(M-1)/2:(M-1)/2)';cutoff=3000;h=zeros(M,1);h(k==0)=2*cutoff/highFs;idx=k~=0;h(idx)=sin(2*pi*cutoff/highFs*k(idx))./(pi*k(idx));h=h.*(0.54-0.46*cos(2*pi*(0:M-1)'/(M-1)));h=h/sum(h);
filtered=filter(h,1,input);decimated=filtered(end-highFs+1:6:end);naive=input(end-highFs+1:6:end);
F=(0:4000)'*fs/8000;A=abs(fft(naive,8000));B=abs(fft(decimated,8000));
fig=figure('Visible','off');plot(F,20*log10(A(1:4001)+1e-12));hold on;plot(F,20*log10(B(1:4001)+1e-12));xlabel('Hz');ylabel('DFT magnitude (dB)');legend('No anti-alias filter','127-tap FIR before decimation');title('48 to 8 kHz decimation: suppress 6 kHz alias at 2 kHz');savePlot(fig,outputDir,'12_Multirate_AntiAlias');
% Delay error for equal-amplitude sinusoidal cancellation: |1-exp(-j*2*pi*f*dt)|.
freq=(1:1500)';delayMs=[0.1 0.5 1 5];ratio=abs(1-exp(-1i*2*pi*freq*(delayMs/1000)));
fig=figure('Visible','off');plot(freq,-20*log10(max(ratio,1e-6)));ylim([-10 40]);xlabel('Hz');ylabel('Attenuation (dB)');legend('0.1 ms','0.5 ms','1 ms','5 ms');title('Sensitivity to unmodeled delay in fixed equal-amplitude anti-phase control');savePlot(fig,outputDir,'13_Phase_and_Latency');
writetable(table(quantizationSnr,(M-1)/2/highFs*1000,'VariableNames',{'pcm16_snr_db','decimation_fir_group_delay_ms'}),fullfile(outputDir,'fundamentals_summary.csv'));
end
function savePlot(fig,directory,name)
if exist('theme','file'), theme(fig,'light'); end
set(fig,'Position',[100 100 1200 750],'Color','w');exportgraphics(fig,fullfile(directory,[name '.png']),'Resolution',130);savefig(fig,fullfile(directory,[name '.fig']));close(fig);
end
