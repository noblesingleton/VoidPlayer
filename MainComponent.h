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

    // UI
    juce::TextButton loadButton { "Load Audio" };
    juce::TextButton loadIRButton { "Load IR" };
    juce::TextButton playStopButton { "Play" };
    juce::Label statusLabel;
    juce::Slider wetSlider;
    juce::Label wetLabel;
    juce::Label wetValueLabel;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    juce::Label volumeValueLabel;

    // Audio
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    VoidConvolutionEngine convolutionEngine;

    // Controls
    float wetMix = 1.0f;
    float masterVolume = 1.0f;

    // Async FileChooser
    std::unique_ptr<juce::FileChooser> fileChooser;  // <-- Added here

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};