#pragma once

#include <JuceHeader.h>
#include "VoidConvolutionEngine.h"

class MainComponent : public juce::AudioAppComponent,
                      private juce::ChangeListener,
                      public juce::Slider::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void loadFile();
    void loadImpulseResponse();
    void updatePositionSlider();
    void clearConvolutionHistory();
    void applyDeviceType();
    void applyBufferSize();

    // UI
    juce::TextButton loadButton { "Load Audio" };
    juce::TextButton loadIRButton { "Load IR" };
    juce::TextButton playStopButton { "Play" };
    juce::Label statusLabel;
    juce::Slider wetSlider;
    juce::Label wetLabel;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    juce::ToggleButton exclusiveToggle;
    juce::Slider positionSlider;
    juce::Label positionLabel;
    juce::ComboBox bufferSizeBox;
    juce::Label bufferSizeLabel;
    juce::Label trackTitleLabel;  // NEW: Track title display

    // Audio
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    VoidConvolutionEngine convolutionEngine;

    // Controls
    float wetMix = 1.0f;
    float masterVolume = 1.0f;

    // Async FileChooser
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Audio device manager
    juce::AudioDeviceManager deviceManager;
    bool useExclusiveMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};