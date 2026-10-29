function results=anc_tonal_feasibility
% Research feasibility only: a stationary tone and a measured scalar acoustic path.
% This experiment does not play or record sound and is NOT hardware validation.
out=fullfile(fileparts(fileparts(mfilename('fullpath'))),'results','tonal_feasibility');
if ~exist(out,'dir'),mkdir(out);end
frequency=120; transportDelay=0.2;
D=0.04*exp(1i*0.7); H=0.7*exp(-1i*2*pi*frequency*transportDelay);
probe=0.02; floorPower=1e-6; phase=(0:3)'*pi/2;
baseline=abs(D)^2/2+floorPower;
observations=abs(D+H*probe*exp(1i*phase)).^2/2+floorPower;
% Four phase measurements and an ANC-off baseline identify the optimal
% complex control amplitude without streaming unpredictable future samples.
B=(mean(observations)-baseline)/probe^2;
G=(observations(1)-observations(3))/(4*probe) ...
  +1i*(observations(4)-observations(2))/(4*probe);
assert(B>0,'Unidentifiable control path');
control=-conj(G)/B;
assert(abs(control+D/H)<1e-10,'Complex-amplitude estimate failed');
assert(abs(control)<0.1,'Proposed output amplitude exceeds research cap');
% A fixed mean delay can be included in H. Time variation and noise changes
% between calibration and playback remain uncontrolled errors.
timingErrorMs=[0;0.1;0.5;1;2];
residualPower=abs(D+H*control*exp(-1i*2*pi*frequency*timingErrorMs/1000)).^2/2+floorPower;
attenuation=10*log10(baseline./residualPower);
results=table(timingErrorMs,attenuation,'VariableNames',{'timing_error_ms','simulated_attenuation_db'});
writetable(results,fullfile(out,'timing_sensitivity.csv'));
writetable(table(phase,observations,'VariableNames',{'probe_phase_rad','synthetic_sensor_power'}),fullfile(out,'phase_observations.csv'));
save(fullfile(out,'experiment.mat'),'D','H','control','probe','baseline','floorPower','frequency','transportDelay','results');
fig=figure('Visible','off');if exist('theme','file'),theme(fig,'light');end
tiledlayout(2,1);nexttile;plot(phase*180/pi,observations,'o-');xlabel('Probe phase (degrees)');ylabel('Synthetic sensor power (FS^2)');title('Stationary-tone phase calibration: four synthetic measurements');
nexttile;plot(timingErrorMs,attenuation,'o-');yline(0,'k--');xlabel('Unmodeled timing change (ms)');ylabel('Simulated attenuation (dB)');title('120 Hz tone, 200 ms fixed model delay: timing stability still matters');
set(fig,'Position',[100 100 1100 750]);exportgraphics(fig,fullfile(out,'feasibility.png'),'Resolution',130);savefig(fig,fullfile(out,'feasibility.fig'));close(fig);
assert(attenuation(1)>25 && attenuation(4)<5,'Sensitivity expectations failed');
disp(results);fprintf('SIMULATION ONLY. Required modeled control amplitude %.5f FS.\n',abs(control));
end
