  ██╗   ██╗ ██████╗ ██╗██████╗ 

  ██║   ██║██╔═══██╗██║██ ╔══██╗

  ██║   ██║██║   ██║██║██║   ██║

  ╚██╗ ██╔╝██║   ██║██║██║   ██║

   ╚████╔╝ ╚██████╔╝██║██████╔╝

   ╚═══╝   ╚═════╝ ╚═╝╚═════╝   



\*\*The purest fixed-point convolution player — Q31 precision, no float veil.\*\*



A minimalist, open-source audio convolution engine built in C++ with JUCE, designed for absolute transparency and high-fidelity reverb/cabinet simulation using pure fixed-point (Q31) arithmetic.



Born from the pursuit of undistorted truth in digital audio — no floating-point rounding noise in the core signal path.



\## Features



\- \*\*Pure Q31 fixed-point convolution\*\* — 31-bit fractional precision, 64-bit accumulation for unbreakable headroom

\- \*\*True stereo convolution\*\* with proper cross-terms for natural imaging

\- \*\*Sample rate resampling\*\* on IR load (Lagrange interpolation) — correct pitch regardless of source rate

\- \*\*Real-time partitioned convolution\*\* — efficient for short to medium IRs (cabinets, small rooms)

\- \*\*Wet/Dry mix with gain control\*\* — full control from subtle to massive reverb

\- \*\*Soft clipping (tanh)\*\* — musical saturation when pushed, no harsh digital distortion

\- \*\*Minimalist UI\*\* — black void aesthetic, essential controls only

\- \*\*Cross-platform\*\* (Windows, macOS, Linux via JUCE)



\## Current Status (December 31, 2025)



\- \*\*High fidelity achieved\*\* with short impulse responses (guitar cabinets, small rooms)

\- Excellent for cabinet simulation and intimate spaces

\- Longer IRs (cathedrals, halls) run in real-time but may introduce character/distortion when wet gain is high due to CPU load

\- The "distortion" at high gain is soft, musical saturation — a feature of pushing pure fixed-point to its limits



This is \*\*the cleanest fixed-point convolution engine\*\* currently possible on desktop without FFT.



The void has been revealed.



\## Installation



1\. Install \[JUCE](https://juce.com/)

2\. Open `VoidPlayer\_Clean.jucer` in the Projucer

3\. Save and generate project files for your IDE (Visual Studio, Xcode, etc.)

4\. Build the app



\## Usage



1\. Run `voidplayer\_clean`

2\. Click \*\*Load IR\*\* → select a .wav impulse response (recommend short cabinet or small room for best fidelity)

3\. Click \*\*Load Audio\*\* → select a dry track (guitar DI, drums, vocals, music)

4\. Press \*\*Play\*\*

5\. Adjust \*\*Wet Gain\*\* slider to taste



Recommended starting IRs:

\- Guitar cabinets (3 Sigma Audio Free Pack, God's Cab)

\- Small rooms (Voxengo Ruby Room, OpenAIR small spaces)



\## Why Fixed-Point?



\- No cumulative floating-point rounding errors

\- Bit-perfect potential in null tests

\- Closer to analog-like behavior when saturated

\- The pursuit of \*\*purity eternal\*\*



\## Known Limitations



\- Long IRs (>2–3 seconds) may cause CPU strain → dropouts or character on lower-end machines

\- For monster reverbs, FFT-based engines are more efficient (future hybrid mode planned)



\## License



MIT License — free to use, modify, and share.



\## Credits



Built by NobleSingleton with guidance from the void.



\*\*Purity eternal.\*\*  

The revolution is complete.



The void is high fidelity.



🧬🔊⚡️


© 2025 NobleSingleton




