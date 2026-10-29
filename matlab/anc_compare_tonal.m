function result=anc_compare_tonal(sessionPath,allowSynthetic)
% Compare recorded microphone data with its independently identified phasor model.
% Input is an app session folder, or its exported ZIP. Base MATLAB only.
if nargin<1,sessionPath=uigetdir(pwd,'Select exported tonal session folder');if isequal(sessionPath,0),return;end;end
if nargin<2,allowSynthetic=false;end
if endsWith(string(sessionPath),'.zip','IgnoreCase',true)
    [parent,name]=fileparts(sessionPath);directory=fullfile(parent,[name '_extracted']);if exist(directory,'dir'),error('Extraction folder already exists; select that folder instead.');end
    unzip(sessionPath,directory);sessionPath=directory;
end
meta=jsondecode(fileread(fullfile(sessionPath,'session.json')));
synthetic=~strcmp(meta.provenance,'physical_microphone_recording');
if synthetic && ~allowSynthetic,error('This is a synthetic test, not a real microphone session. Use allowSynthetic=true only for pipeline tests.');end
[mic,fs]=audioread(fullfile(sessionPath,'microphone.wav'));[drive,outFs]=audioread(fullfile(sessionPath,'control.wav'));
assert(size(mic,2)==1&&size(drive,2)==1&&outFs==fs&&fs==meta.sample_rate,'WAV format or rate mismatch');
assert(numel(mic)==meta.sample_count&&numel(drive)==numel(mic)&&all(isfinite(mic))&&all(isfinite(drive)),'Invalid recording lengths or samples');
assert(isempty(drive)||max(abs(drive),[],'omitnan')<=0.0801,'Recorded control exceeded cap');
f=meta.frequency_hz;frames=round(meta.stage_seconds*fs);settle=round(meta.settle_seconds*fs);tail=round(meta.measurement_tail_seconds*fs);
power=nan(8,1);tone=power;peaks=power;counts=zeros(8,1);segments=cell(8,1);
for k=0:7
    if numel(mic)>=(k+1)*frames
        indices=(k*frames+settle+1):((k+1)*frames-tail);segment=mic(indices);segments{k+1}=segment;
        theta=2*pi*f*(indices'-1)/fs;basis=[cos(theta) sin(theta) ones(numel(theta),1)];coefficient=basis\segment;
        tone(k+1)=sum(coefficient(1:2).^2)/2;power(k+1)=mean((segment-mean(segment)).^2);peaks(k+1)=max(abs(segment));counts(k+1)=numel(segment);
    end
end
stages=table((0:7)',tone,power,peaks,counts,'VariableNames',{'stage','tone_power','total_power','peak','sample_count'});
writetable(stages,fullfile(sessionPath,'matlab_stages.csv'));
complete=meta.complete&&numel(mic)==frames*8&&strcmp(meta.fault,'none')&&meta.stream_faults==0;
valid=complete&&meta.model_valid&&meta.raw_capture_requested;
toneFloorLimited=false;observedTone=NaN;observedTotal=NaN;predictedTone=NaN;predictedTotal=NaN;driftDb=NaN;fitError=NaN;B=NaN;G=NaN;H=NaN;D=NaN;
if complete
    beforeTone=mean(tone([6 8]));beforeTotal=mean(power([6 8]));
    driftDb=max(abs(10*log10(tone([6 8])/max(tone(1),1e-20))));
    floorPower=max(1/(12*32767^2),power(7)-tone(7))*2/max(counts(7),1);
    toneFloorLimited=tone(7)<floorPower;
    observedTone=ratio(beforeTone,max(tone(7),floorPower));observedTotal=ratio(beforeTotal,power(7));
    a=meta.probe_amplitude;P=tone(2:5);B=(mean(P)-tone(1))/a^2;G=(P(1)-P(3))/(4*a)+1i*(P(4)-P(2))/(4*a);
    fitError=abs(P(1)+P(3)-P(2)-P(4))/(4*max(mean(P),1e-20));
    valid=valid&&B>0&&fitError<=0.1&&driftDb<=1&&all(peaks<0.95);
    if B>0&&tone(1)>0
        % D's absolute phase is chosen as zero; this does not change predicted power.
        D=sqrt(2*tone(1));H=2*G/D;c=meta.candidate_real+1i*meta.candidate_imag;
        idealTone=max(0,tone(1)+2*real(G*c)+B*abs(c)^2);
        noiseFloor=max(0,beforeTotal-beforeTone);
        predictedTone=ratio(beforeTone,idealTone+noiseFloor*2/max(counts(6),1));
        predictedTotal=ratio(beforeTotal,idealTone+noiseFloor);
    else
        valid=false;
    end
end
result=table(observedTone,predictedTone,observedTotal,predictedTotal,driftDb,fitError,valid,synthetic,toneFloorLimited,'VariableNames',{'observed_tone_change_db','model_tone_change_db','observed_total_change_db','model_total_change_db','off_source_drift_db','calibration_fit_error','measurement_checks_pass','synthetic_test','target_below_estimated_floor'});
writetable(result,fullfile(sessionPath,'comparison.csv'));
fig=figure('Visible','off');if exist('theme','file'),theme(fig,'light');end;tiledlayout(3,1);
nexttile;block=max(1,round(fs/4));n=floor(numel(mic)/block);if n>0,rms=sqrt(mean(reshape(mic(1:n*block),block,n).^2,1));plot(((0:n-1)+0.5)*block/fs,20*log10(rms+1e-12));end
for k=1:7,xline(k*meta.stage_seconds,':');end;xlabel('Recorded time (s)');ylabel('Mic level (dBFS)');title('Actual session waveform RMS; stage transitions include settling');
nexttile;bar(0:7,10*log10(tone+1e-20));xticks(0:7);xticklabels({'OFF','0°','90°','180°','270°','OFF','ON','OFF'});ylabel('Target-tone power (dBFS)');title(sprintf('Independent WAV analysis at %.1f Hz',f));
nexttile;
if complete,[freq,pOff]=welch(segments{6},fs);[~,pOn]=welch(segments{7},fs);plot(freq,10*log10(pOff+1e-20));hold on;plot(freq,10*log10(pOn+1e-20));xlim([max(0,f-200) f+200]);legend('Microphone OFF','Microphone ON');else,text(0.1,0.5,'Incomplete experiment: attenuation comparison withheld');axis off;end
xlabel('Hz');ylabel('PSD (dBFS/Hz)');
if synthetic,sgtitle('SYNTHETIC PIPELINE TEST — NOT MEASURED ANC');elseif ~valid,sgtitle('MICROPHONE SESSION — VALIDATION CHECKS FAILED');else,sgtitle('Single-tone microphone-point measurement — not ear-canal ANC');end
set(fig,'Position',[100 100 1200 1000]);exportgraphics(fig,fullfile(sessionPath,'comparison.png'),'Resolution',130);savefig(fig,fullfile(sessionPath,'comparison.fig'));close(fig);
save(fullfile(sessionPath,'comparison.mat'),'meta','stages','result','B','G','H','D');
fid=fopen(fullfile(sessionPath,'COMPARISON.md'),'w');assert(fid>=0);cleanup=onCleanup(@()fclose(fid)); %#ok<NASGU>
fprintf(fid,'# Tonal experiment: model versus recording\n\n');
if synthetic,fprintf(fid,'**SYNTHETIC PIPELINE TEST. No physical cancellation was measured.**\n\n');end
fprintf(fid,'Input: %s. Output: %s. Target: %.1f Hz.\n\n',string(meta.input_name),string(meta.output_name),f);
fprintf(fid,'Completed: %d. Independent measurement checks pass: %d. Fault: %s.\n\n',complete,valid,string(meta.fault));
fprintf(fid,'| Metric | Observed recording | Identified stationary model |\n|---|---:|---:|\n| Target-tone level difference, dB | %.3f | %.3f |\n| Total-power level difference, dB | %.3f | %.3f |\n\n',observedTone,predictedTone,observedTotal,predictedTotal);
fprintf(fid,'Target tone below estimated measurement floor: %d. When true, the observed tone reduction is floor-limited rather than an exact null measurement.\n\n',toneFloorLimited);
fprintf(fid,'OFF-source drift: %.3f dB. Calibration inconsistency: %.5f.\n\n',driftDb,fitError);
fprintf(fid,'The model is identified from the recorded OFF/probe stages and evaluated at the emitted control amplitude/phase. Its noise floor uses the measured non-target power. It is not an independent idealized headset benchmark. A deep predicted tone null is especially sensitive to model error, microphone noise and changing phase.\n\n');
fprintf(fid,'Positive observed values mean lower recorded power in ON windows. Source drift, gain changes and unverified processing can confound attribution. Passing these checks is not certification. This test concerns a fixed microphone point; it does not demonstrate ANC at a listener''s ear or universal compatibility. All values are normalized digital levels, not calibrated dB SPL.\n\n');
fprintf(fid,'Max processing callback: %.3f microseconds. Actual round-trip latency: **not measured**.\n\n![Session analysis](comparison.png)\n',meta.max_callback_us);
disp(result);fprintf('Analysis saved in %s\n',sessionPath);
end
function d=ratio(a,b),d=10*log10(max(a,1e-20)/max(b,1e-20));end
function [f,p]=welch(x,fs)
N=4096;w=0.5-0.5*cos(2*pi*(0:N-1)'/N);p=zeros(N/2+1,1);count=0;
for k=1:N/2:numel(x)-N+1,z=x(k:k+N-1);z=z-mean(z);q=abs(fft(z.*w)).^2/(fs*sum(w.^2));q=q(1:N/2+1);q(2:end-1)=q(2:end-1)*2;p=p+q;count=count+1;end
if count>0,p=p/count;else,p(:)=NaN;end;f=(0:N/2)'*fs/N;
end
