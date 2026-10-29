# Research finding: a bounded acoustic experiment is worth attempting

The user's goal remains enabling acoustic ANC across ordinary headphones, with simulation and real-time measurements. Available equipment is a laptop, a phone and boAt Nirvana Crystl earbuds. This research does not establish universal feasibility or supply measured attenuation values.

## What the research supports

**Commodity-device acoustic control has been demonstrated for a controlled alarm.** KAIST's Virtual Earplugs work reports up to 19 dB reduction and about 2.1 s processing/convergence time. The paper uses calibrated amplitude and phase for a steady sinusoidal alarm; that time is not a claim of 2.1-second sample-by-sample broadband ANC latency. Reduction is optimized at a microphone and declines away from it. This is evidence for trying a narrowband, fixed-geometry experiment, not for ambient cancellation in arbitrary headphones. [Authors' project](https://nmsl.kaist.ac.kr/projects/virtual_earplugs/), [authors' paper](https://taesikgong.com/paper/ICCCN17_Enjoy.pdf).

**A second reference microphone is not mandatory in every ANC architecture.** Feedback/internal-model headphone ANC can use an internal residual microphone and a modeled speaker path. Guldenschuh and Hoeldrich demonstrate predictive feedback control with simulations and DSP measurements. Prediction uses assumptions about the noise and headphone acoustics; the design still needs a suitable sensing location and secondary-path information. It is not evidence that a remote phone microphone equals an ear-canal sensor. [Prediction filter design for active noise cancellation headphones](https://ietresearch.onlinelibrary.wiley.com/doi/10.1049/iet-spr.2012.0161).

**Wireless ANC research exists, but wireless links alone are insufficient.** Studied designs use deliberately placed reference sensors, error sensing and local control or specialized wireless hardware. This does not establish that any commercial Bluetooth media endpoint exposes the required signals or timing. [Survey of Integrating Wireless Technology into ANC](https://arxiv.org/html/2405.12496v1).

## Proposed experiment using the existing equipment

This is an engineering proposal, not a result reproduced from those papers.

1. Use the phone speaker as an independent, steady single-tone noise source. Select a frequency only after checking both speakers' usable response. Start with a moderate audible low-frequency tone; no promised operating frequency or attenuation is assigned.
2. Use the laptop microphone as the error sensor. Position an earbud's acoustic outlet near that microphone in a fixed, repeatable bench arrangement. Do not wear it during this initial experiment. Any reduction will apply to the microphone position, not automatically to a listener's ear.
3. Turn the earbuds' manufacturer ANC and transparency processing OFF; otherwise their behavior confounds evaluation of our own controller. Verify that the earbud remains active outside the ear using the manufacturer's supported settings.
4. Keep capture on the laptop microphone and playback on the earbuds. If this cannot be maintained, the experiment fails its route requirement. Recording AGC/noise suppression, level compression or Bluetooth state changes can invalidate calibration.
5. Measure ANC-off tone power, then estimate the complex control path by playing a small calibration tone at several phases. Use long stationary windows with measured settling, rather than assuming the Bluetooth stream is low latency.
6. Calculate a candidate anti-tone amplitude/phase; test it using alternating OFF/ON/OFF blocks and repeat. Track the exact frequency, residual tone power, total power, clipping, source drift and route state. Stop if the source is not sufficiently stationary or the candidate increases the measured residual.
7. Compare the physical measurement to a simulation using the estimated acoustic path. Report microphone-point narrowband reduction separately from broadband power and from perceptual/ear-canal attenuation.

A stable tone can be phase-corrected across a fixed large delay. Random future noise cannot generally be supplied by the same method. Changing geometry, noisy sensors, unknown signal components, drifting oscillator frequency and variable delay can defeat the experiment. If the phone cannot produce a sufficiently stable tone or the sensor cannot observe both source contributions reliably, there is no valid result to report.

## Proposed estimator and an independently checked synthetic example

For one frequency, let D be the unknown disturbance phasor at the error microphone, H the control-output-to-microphone response, and c the control phasor. With a constant noise floor F:

    P(c) = |D + H c|^2 / 2 + F

Measure P_off and four powers with c=a*exp(j*phi), phi=0,pi/2,pi,3pi/2. Define:

    B = (mean(P_probe) - P_off) / a^2
    G = (P_0 - P_pi)/(4a) + j*(P_3pi/2 - P_pi/2)/(4a)
    c_opt = -conj(G) / B

These are proposed stationary phasor-identification equations, not copied implementation details from the cited smartphone system. They require constant disturbance phase relative to the control oscillator across calibration, constant noise floor and gains, and a resolvable positive B. Use repeated observations and fit-quality checks in a physical implementation. A low measured power alone cannot establish phase stability or identify whether AGC changed.

`matlab/anc_tonal_feasibility.m` checks the equations on an explicitly synthetic 120 Hz tone with a modeled 200 ms fixed path delay, then varies unmodeled timing error. It exports its own CSV/MAT/FIG/PNG files under `results/tonal_feasibility/`. The chosen frequency, path, delay and noise floor are illustrative; they are not measurements of the Nirvana Crystl or predictions of its performance.

## Research decision

Proceed toward **a measured, narrowband acoustic feasibility experiment on one supported route**, then evaluate transfer to other headphones. Retain universal compatibility as the research objective, not an already established property. Do not replace acoustic cancellation with microphone denoising or claim success from boAt's own ANC. Successful cancellation at a laptop microphone would be a legitimate physical proof of interference, but would still not demonstrate ambient ANC at a user's ear.

The current two-sensor FxLMS fixture implementation does not execute this proposed single-sensor phase-calibration experiment. A separate experimental audio mode and real recordings are still required. No physical sound output or microphone recording was performed during this research review.

## Executed MATLAB check

The standalone test completed successfully in MATLAB R2025b. All complex-amplitude recovery and sensitivity assertions passed. With the illustrative model's fixed 200 ms delay, modeled attenuation was 29.036 dB with no timing change, 21.597 dB with 0.1 ms timing change, 8.492 dB with 0.5 ms, 2.655 dB with 1 ms, and -2.726 dB with 2 ms. These results quantify a synthetic timing-sensitivity example, not the Bluetooth jitter or attainable attenuation of the actual headset. No measured physical attenuation is available yet.

[Executed model table](../results/tonal_feasibility/timing_sensitivity.csv) · [Figure](../results/tonal_feasibility/feasibility.png) · [MATLAB source](../matlab/anc_tonal_feasibility.m)
