```
.   *      .         .      *       
                     .      .     *     .       .     .      .    
                .         .       .         .     .       .     
            .      .      .    .      .      .         .     .  
       .         .     .        .         .      .      .       
   .      .      .      .     .      .      .     .      .    . 
.         .     .      .      .     .      .      .     .       
      .      .      .     .      .      .      .     .      .    
   .     .      .      .      .      .      .      .     .     
.      .      .     .      .      .      .      .     .      . 
      .      .      .      .      .      .      .      .      .   
   .      .      .     .      .      .      .      .     .     
      .      .      .      .      .      .      .      .        
         .      .      .      .      .      .      .           
            .      .      .      .      .      .              
               .      .      .      .      .                 
                  .      .      .      .                     
                     .      .      .                        
                        .      .                           
                           .    .                            
                              .
                    ████████████████
                 ██████████████████████
               ██████████████████████████
             ██████████████████████████████
           ██████████████████████████████████
          ████████████████████████████████████
         ██████████████████████████████████████
        ████████████████████████████████████████
       ██████████████████████████████████████████
       ██████████████████████████████████████████ 
      █████   █████    ███████    █████ ██████████  
    ░░███   ░░███   ███░░░░░███ ░░███ ░░███░░░░███ 
     ░███    ░███  ███     ░░███ ░███  ░███   ░░███
     ░███    ░███ ░███      ░███ ░███  ░███    ░███
     ░░███   ███  ░███      ░███ ░███  ░███    ░███
      ░░░█████░   ░░███     ███  ░███  ░███    ███ 
        ░░███      ░░░███████░   █████ ██████████  
         ░░░         ░░░░░░░    ░░░░░ ░░░░░░░░░░   
       ██████████████████████████████████████████
       ██████████████████████████████████████████
       ██████████████████████████████████████████
        ████████████████████████████████████████
         ██████████████████████████████████████
          ████████████████████████████████████
           ██████████████████████████████████
             ██████████████████████████████
               ██████████████████████████
                 ██████████████████████
                    ████████████████                           
                           .    .                            
                        .      .                           
                     .      .      .                        
                  .      .      .      .                     
               .      .      .      .      .              
            .      .      .      .      .      .           
         .      .      .      .      .      .      .        
      .     .      .      .      .      .     .      .     
   .      .      .     .      .      .     .      .      . 
.     .      .      .      .      .      .      .      .   
      .      .      .     .      .      .      .     .     
         .      .      .      .      .      .      .        
            .      .      .      .      .      .           
               .      .      .      .      .              
                  .      .      .      .                 
                     .      .      .                     
                        .      .                        
                           .                           
                              .                            
                                 .
                                                                                                                                                                                         
```
                                          
# VOID PLAYER ETERNAL VOID CONVOLUTION REVERB

**THE DIGITAL CURSE IS DEAD. THE VEIL HAS BEEN RIPPED OPEN.**

Void Player is the final weapon of pure audiophile truth.  
Q64 fixed-point time-domain convolution — no FFT, no veil, no mercy.  
This is not just a reverb tool. This is a general music playback beast with eternal convolution power.  
Load any track, slap on your favorite IR, and hear the truth.

> "I loaded my old guitar cab IR... and cried."  
> – early void disciple

## FEATUREZ (CURRENT - JAN 10, 2026)

* General music playback (wav / mp3 / flac / aiff / ogg)
* Load any IR (wav) — enter the abyss
* Track title display — filename shown under IR status
* Play/Stop control
* Seek bar — click + drag scrubbing (smooth)
* Reverb Dry/Wet mix — 0-100 whole number slider with live readout
* Master Volume — 0-200 whole number slider with live readout
* Exclusive Mode (Bit Perfect) — enables WASAPI exclusive for direct hardware access (lowest latency, no system mixing)
* Buffer Size selector — 64 / 128 / 256 / 512 samples
* Progress bar + current/total time display
* Fully windowed & resizable — controls adapt and stay visible at any size
* Pure black void aesthetic — cyan glow, lime green status, minimalism

### Multithreading / Async IR Loading (NEW - JAN 2026)

* Large/high-res impulse responses now load in a background thread
* No UI freeze during loading
* Audio playback continues uninterrupted (dry signal until IR is ready)
* Seamless, click-free engine swap when loading completes
* Lock-free using std::atomic for thread-safe pointer exchange
* Status label shows real-time feedback ("Loading IR... (background)" → "IR loaded")
* Zero dropouts, zero clicks — even with massive IRs

### Upsampling – 2x / 4x / 8x / 16x (NEW - JAN 10, 2026)

* Real-time selectable upsampling factor (Off / 2x / 4x / 8x / 16x)
* Background IR resampling (manual linear interpolation — fast & safe)
* Real-time block upsampling of input audio → convolution at higher rate → downsampling of output
* Noticeably smoother reverb tails, reduced aliasing, deeper/infinite decay
* "Preparing upsampling ×N... (background)" status during prep (no freeze)
* Seamless swap to upsampled engine when ready
* Quality jump especially audible at 8x/16x with long tails

### STATUS - JAN 10, 2026  
**ALL IR LENGTHS: SQUEAKY CLEAN | NO DISTORTION | NO CLICKS | NO CRASHES | NO VEIL**  
**FADE-OUTS: ABSOLUTE VOID BLACKNESS — squeaky clean silence, no zipper/crackle artifacts at the end**  
**DRY PLAYBACK: Q2.30 FIXED-POINT VOLUME — pure integer precision on the direct signal, no float veil, full headroom**  
ASIO/WASAPI: VISIBLE AND FUNCTIONAL (Exclusive toggle clearly "Bit Perfect")  
SEEK: DRAGGABLE AND SMOOTH  
LAYOUT: RESPONSIVE — WORKS IN ANY WINDOW SIZE  
ASYNC LOADING: FULLY STABLE — LARGE IRs LOAD SMOOTHLY IN BACKGROUND  
UPSAMPLING: LIVE AND AUDIBLE — 16x TAILS ARE ETERNAL

## MORE COMING SOON

* Drag & Drop support (files & folders)
* Gapless album playback from folder selection
* DSD playback support — native DSD64/DSD128/DSD256/DSD512/DSD1024
* Fractal Decay — nature-inspired reverb tail shaping with self-similar fractal curves (forests, caves, cathedrals, infinite void decay)
* Custom Void Filters & Modulators — elite EQ, LFO, envelope, phase destruction
* Playlist/Queue system
* Keyboard shortcuts
* Generative Visual Engine – real-time void visuals synced to the audio
  * Hyper Real Dream Scenes — generative surreal landscapes and dream-like environments reacting to audio spectrum and convolution tail
    * procedural fractal generation
    * spectral analysis driven color/particle systems
    * infinite zoom, self-similar void aesthetics
    * optional fullscreen "meditation mode"
    * visual feedback for convolution depth / tail length
    * pure psychedelic void immersion
* Ultra-long IR support (beyond current truncate)
* Even darker, deeper void UI polish
* Preset save/load
* Bit-perfect indicator
* CPU Indicator
* Latency Indicator

## BUILD

Requires JUCE 7.0.9 + Visual Studio 2022  
Open `.jucer` → Export → Build in VS2022

ENTER THE VOID. HEAR THE TRUTH. SEE THE TRUTH.  
© 2026 Noble Singleton  
ORYAAAAA!!! 🧬🔊⚡️
