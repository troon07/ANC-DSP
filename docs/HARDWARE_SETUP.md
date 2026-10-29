# Bench hardware

Use one USB audio interface with two simultaneous independent microphone/line inputs and at least one output, sharing one clock. Input 0 is a reference microphone positioned toward the noise source. Input 1 is an error microphone in a fixed ear coupler or enclosed test fixture. Output 0 drives the control headphone/driver through a gain-controlled amplifier. Use a repeatable noise loudspeaker independent of the control driver. Separate microphones and correct gain staging are essential.

Keep reference-to-error propagation preview larger than the complete output/control/measurement delay for broadband cancellation. At sound speed roughly 343 m/s, even 1 m of acoustic preview is only about 2.9 ms. Laptop/phone IO can exceed that; a near-ear low-delay embedded controller is normally needed for consumer ANC. A favorable synthetic delay is not evidence that a host setup can meet it.

Validate channel assignment with separate physical stimuli. A stereo endpoint can still expose duplicated or beamformed mono; inspect captured independent channels. Disable AGC, enhancement and echo cancellation where possible. Keep all device gains and microphone geometry fixed after calibration. Start with a coupler and conservative hardware gain; the FS limiter does not specify SPL at an ear.

Wired headphones alone supply no independent error microphone. Most Bluetooth headsets expose a communications microphone and buffered playback, while internal ANC sensor channels are private to headset firmware. The detected Nirvana Crystl route on this computer exposes 16 kHz mono capture and 48 kHz stereo output; this is insufficient for this two-sensor host ANC architecture. Use the headphone's built-in ANC for normal wireless listening.
