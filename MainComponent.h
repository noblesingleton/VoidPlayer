#pragma once

#include <JuceHeader.h>
#include "Fixed256.h"

class ConvolutionEngine
{
public:
    ConvolutionEngine() = default;

    void loadIR(const juce::File& irFile, juce::AudioFormatManager& manager);

    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    std::vector<Fixed256> irKernel;
};

class Modulator
{
public:
    Modulator() = default;

    void prepare(double sampleRate);

    void setSchumann(bool on) { schumannOn = on; }

    void set432Hz(bool on) { four32On = on; }

    void setBinaural(bool on) { binauralOn = on; }

    void setIntensity(double newIntensity) { intensity = newIntensity; }

    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    bool schumannOn = false;
    bool four32On = false;
    bool binauralOn = false;
    double intensity = 0.0;
    double phase = 0.0;
    double currentSampleRate = 44100.0;
    int mode = 0;
};

class MainComponent : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      public juce::Slider::Listener,
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    void sliderValueChanged (juce::Slider* slider) override;
    void timerCallback() override;

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioSourcePlayer> player;
    std::unique_ptr<juce::AudioTransportSource> transport;

    std::vector<std::unique_ptr<juce::AudioFormatReaderSource>> playlist;
    std::vector<juce::String> trackNames;
    size_t currentTrackIndex = 0;

    juce::TextButton playPauseButton {"Play"};
    juce::TextButton stopButton {"Stop"};
    juce::TextButton previousButton {"<< Previous"};
    juce::TextButton nextButton {"Next >>"};
    juce::Slider volumeSlider;
    juce::Label trackLabel;
    juce::Slider progressSlider;
    juce::Label timeLabel;
    juce::Label cpuLabel;

    juce::TextButton loadIRButton {"Load Impulse Response"};
    juce::ToggleButton schumannToggle {"Schumann 7.83Hz"};
    juce::ToggleButton four32Toggle {"432Hz Tuning"};
    juce::ToggleButton binauralToggle {"Binaural Beats"};
    juce::Slider modulatorIntensity {"Modulation Intensity"};

    std::unique_ptr<juce::FileChooser> fileChooser;

    Fixed256 volumeLevel = Fixed256(0.8);

    ConvolutionEngine convolution;
    Modulator modulator;

    double cpuUsage = 0.0;

    void loadTrack (size_t index);
    void updateTransportButtons();
    void updateProgress();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};