#pragma once

#include <JuceHeader.h>
#include "Fixed256.h"

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

    Fixed256 volumeLevel = Fixed256(0.8);

    double cpuUsage = 0.0;

    void loadTrack (size_t index);
    void updateTransportButtons();
    void updateProgress();
    void processBlock (juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};