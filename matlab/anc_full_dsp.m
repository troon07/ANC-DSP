function results=anc_full_dsp(cppDirectory,outputDirectory)
% Independent base-MATLAB LMS, NLMS and RLS implementation and verification.
% Digital reference-based cancellation: residual=primary-estimated_noise.
% Acoustic FxNLMS and real microphone comparisons use separate entrypoints.
root=fileparts(fileparts(mfilename('fullpath')));
if nargin<1,cppDirectory=fullfile(root,'results','full_dsp','cpp');end
if nargin<2,outputDirectory=fullfile(root,'results','full_dsp','matlab');end
if ~exist(outputDirectory,'dir'),mkdir(outputDirectory);end
scenarios={'stationary','path_change','uncorrelated','reference_leakage','reference_dropout'};algorithms={'lms','nlms','rls'};
labels={'LMS','NLMS','RLS'};fs=8000;L=16;epsilon=1e-8;lambda=0.999;maxNorm=20;index=0;
scenarioColumn=cell(15,1);algorithmColumn=scenarioColumn;inputSnr=zeros(15,1);outputSnr=inputSnr;improvement=inputSnr;mse=inputSnr;gain=inputSnr;agreement=inputSnr;
stationary=cell(3,1);changing=cell(3,1);allErrors=cell(5,3);
for s=1:5
    for method=1:3
        index=index+1;scenario=scenarios{s};name=algorithms{method};
        data=readtable(fullfile(cppDirectory,[scenario '_' name '.csv']));
        x=condition(data.reference,fs);d=condition(data.primary,fs);truth=condition(data.clean,fs);N=numel(x);tail=(N-7999):N;
        w=zeros(L,1);history=w;P=10*eye(L);estimate=zeros(N,1);residual=estimate;normTrace=estimate;
        mu=0.05;if method==1,mu=0.1;end
        for n=1:N
            history=[x(n);history(1:end-1)];estimate(n)=w'*history;residual(n)=d(n)-estimate(n);energy=history'*history;
            if energy/L>=1e-10
                if method==3
                    u=P*history;denominator=lambda+history'*u;k=u/denominator;
                    w=w+k*residual(n);P=(P-u*u'/denominator)/lambda;
                else
                    scale=mu*residual(n);if method==2,scale=scale/(epsilon+energy);end
                    w=w+scale*history;
                end
                lengthW=norm(w);if lengthW>maxNorm,w=w*maxNorm/lengthW;end
            end
            normTrace(n)=norm(w);
        end
        assert(all(isfinite(residual))&&all(isfinite(w)),'Adaptive algorithm failed');
        agreement(index)=max(abs(residual-data.residual));assert(agreement(index)<2e-4,'C++/MATLAB sample agreement failed');
        cleanEnergy=sum(truth(tail).^2);inputError=sum((d(tail)-truth(tail)).^2);outputError=sum((residual(tail)-truth(tail)).^2);
        scenarioColumn{index}=scenario;algorithmColumn{index}=name;inputSnr(index)=ratio(cleanEnergy,inputError);outputSnr(index)=ratio(cleanEnergy,outputError);improvement(index)=outputSnr(index)-inputSnr(index);mse(index)=outputError/numel(tail);gain(index)=sum(residual(tail).*truth(tail))/cleanEnergy;
        allErrors{s,method}=(residual-truth).^2;
        if s==1,stationary{method}=[truth d estimate residual normTrace];end
        if s==2,changing{method}=[truth d estimate residual normTrace];end
        fprintf('%-18s %-4s input %7.3f, output %7.3f dB, C++ delta %.3g FS\n',scenario,name,inputSnr(index),outputSnr(index),agreement(index));
    end
end
results=table(scenarioColumn,algorithmColumn,inputSnr,outputSnr,improvement,mse,gain,agreement,'VariableNames',{'scenario','algorithm','input_snr_db','output_snr_db','snr_improvement_db','late_mse','desired_signal_gain','cpp_max_absolute_difference'});
writetable(results,fullfile(outputDirectory,'summary.csv'));save(fullfile(outputDirectory,'evidence.mat'),'results','stationary','changing','allErrors','fs','L','lambda');
assert(all(improvement(1:6)>3),'Nominal or changed-path convergence failed');
assert(all(gain(10:12)<0.8),'Leakage experiment did not demonstrate desired-signal loss');
t=(0:size(stationary{1},1)-1)'/fs;
fig=figure('Visible','off');tiledlayout(2,1);
nexttile;for j=1:3,plot(t,10*log10(movmean(allErrors{1,j},400)+1e-16));hold on;end;xlabel('s');ylabel('Error MSE (dBFS power)');legend(labels);title('Digital ANC learning curves: known clean target, stationary plant');
nexttile;for j=1:3,plot(t,10*log10(movmean(allErrors{2,j},400)+1e-16));hold on;end;xline(2,'k--','Noise path changes');xlabel('s');ylabel('Error MSE (dBFS power)');legend(labels);title('Tracking a changed noise path');exportFigure(fig,outputDirectory,'01_Convergence_and_Tracking');
fig=figure('Visible','off');bar(categorical(scenarios,scenarios),reshape(improvement,3,5)');xtickangle(20);legend(labels);ylabel('SNR improvement (dB)');title('Synthetic digital cancellation: no acoustic attenuation claim');exportFigure(fig,outputDirectory,'02_SNR_Comparison');
fig=figure('Visible','off');tiledlayout(2,1);nexttile;segment=(28001:28400);plot(t(segment),stationary{2}(segment,[1 2 4]));legend('conditioned clean target','conditioned noisy primary','NLMS residual');xlabel('s');ylabel('FS');title('Noise estimate subtracted from primary; desired audio retained');
nexttile;for j=1:3,plot(t,stationary{j}(:,5));hold on;end;xlabel('s');ylabel('Coefficient L2 norm');legend(labels);title('Bounded adaptive coefficients');exportFigure(fig,outputDirectory,'03_Signals_and_Weights');
fig=figure('Visible','off');tiledlayout(2,1);nexttile;[f,p]=welch(stationary{2}(24001:end,2),fs);[~,q]=welch(stationary{2}(24001:end,4),fs);[~,r]=welch(stationary{2}(24001:end,1),fs);plot(f,10*log10([p q r]+1e-20));xlim([0 2000]);xlabel('Hz');ylabel('PSD (dBFS/Hz)');legend('primary','NLMS residual','clean target');title('Known clean signal separates noise removal from signal distortion');
nexttile;bar(categorical(algorithms,algorithms),[gain(1:3) gain(10:12)]);legend('independent reference','reference contains desired audio');ylabel('Desired-signal projection gain');yline(1,'k--');title('Reference leakage can cancel wanted audio');exportFigure(fig,outputDirectory,'04_Spectra_and_Reference_Leakage');
fig=figure('Visible','off');for j=1:3,plot(t,10*log10(movmean(allErrors{5,j},400)+1e-16));hold on;end;xline(1.5,'k--','Reference lost');xline(2.5,'k--','Reference returns');xlabel('s');ylabel('Error MSE (dBFS power)');legend(labels);title('Missing reference prevents cancellation; adaptation freezes once history is silent');exportFigure(fig,outputDirectory,'05_Reference_Dropout');
audiowrite(fullfile(outputDirectory,'clean.wav'),stationary{2}(:,1),fs);audiowrite(fullfile(outputDirectory,'noisy.wav'),stationary{2}(:,2),fs);audiowrite(fullfile(outputDirectory,'nlms_residual.wav'),stationary{2}(:,4),fs);
fid=fopen(fullfile(outputDirectory,'REPORT.md'),'w');assert(fid>=0);cleanup=onCleanup(@()fclose(fid)); %#ok<NASGU>
fprintf(fid,'# Full adaptive DSP implementation: executed results\n\nDigital reference-based noise cancellation. All signals in this experiment are synthetic; SNR uses the known conditioned clean target. These values are not acoustic headphone attenuation.\n\n');
fprintf(fid,'C++17 and independent base-MATLAB implementations: 8 kHz, 4 seconds per case, 16 taps; DC cutoff 10 Hz and second-order low-pass 1500 Hz. LMS step 0.1; NLMS step 0.05; RLS forgetting factor 0.999, initial inverse covariance 10I. Values below use the last second. Algorithm ranking depends on these parameters and inputs.\n\n');
fprintf(fid,'| Scenario | Algorithm | Input SNR dB | Output SNR dB | Improvement dB | Desired gain | C++/MATLAB max delta FS |\n|---|---|---:|---:|---:|---:|---:|\n');
for k=1:height(results),fprintf(fid,'| %s | %s | %.3f | %.3f | %.3f | %.3f | %.3g |\n',scenarioColumn{k},upper(algorithmColumn{k}),inputSnr(k),outputSnr(k),improvement(k),gain(k),agreement(k));end
fprintf(fid,'\nThe reference-leakage case demonstrates loss of desired audio. An uncorrelated disturbance cannot be predicted from this reference. The final-window metric does not show the dropout interval, so its full time trace is plotted separately. No speech intelligibility score or measured microphone SNR is inferred.\n\n');
names={'01_Convergence_and_Tracking','02_SNR_Comparison','03_Signals_and_Weights','04_Spectra_and_Reference_Leakage','05_Reference_Dropout'};for k=1:numel(names),fprintf(fid,'![%s](%s.png)\n\n',names{k},names{k});end
fprintf(fid,'See docs/FULL_ADAPTIVE_DSP.md for equations, C++ APIs, guards, calibration and acoustic/digital distinctions. C++ CSV traces are under ../cpp/. Source is matlab/anc_full_dsp.m. Playback WAVs use identical gain. Actual headphone measurements remain separate.\n');
fprintf('All 15 algorithm/scenario checks passed. Results: %s\n',outputDirectory);
end
function y=condition(x,fs)
r=exp(-2*pi*10/fs);w=2*pi*1500/fs;c=cos(w);alpha=sin(w)/sqrt(2);b=[(1-c)/2 1-c (1-c)/2]/(1+alpha);a=[1 -2*c/(1+alpha) (1-alpha)/(1+alpha)];y=filter(b,a,filter([1 -1],[1 -r],x));
end
function d=ratio(a,b),d=10*log10(max(a,1e-20)/max(b,1e-20));end
function [f,p]=welch(x,fs)
N=512;w=0.5-0.5*cos(2*pi*(0:N-1)'/N);p=zeros(N/2+1,1);count=0;
for k=1:N/2:numel(x)-N+1,z=x(k:k+N-1);z=z-mean(z);q=abs(fft(z.*w)).^2/(fs*sum(w.^2));q=q(1:N/2+1);q(2:end-1)=2*q(2:end-1);p=p+q;count=count+1;end;p=p/count;f=(0:N/2)'*fs/N;
end
function exportFigure(fig,directory,name)
if exist('theme','file'),theme(fig,'light');end
set(findall(fig,'Type','axes'),'TickLabelInterpreter','none');set(fig,'Position',[100 100 1200 850]);exportgraphics(fig,fullfile(directory,[name '.png']),'Resolution',130);savefig(fig,fullfile(directory,[name '.fig']));close(fig);
end
