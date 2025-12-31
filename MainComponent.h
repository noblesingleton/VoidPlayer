// MainComponent.h — Final version with working Wet Gain slider and soft clipping (December 31, 2025)

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
    juce::TextButton loadButton   { "Load Audio" };
    juce::TextButton loadIRButton { "Load IR" };
    juce::TextButton playButton   { "Play" };
    juce::TextButton stopButton   { "Stop" };
    juce::Label      statusLabel;

    juce::Slider     wetSlider;
    juce::Label      wetLabel;

    // Audio playback
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    // Convolution core
    VoidConvolutionEngine convolutionEngine;

    // Wet gain control
    float wetGain = 16.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};