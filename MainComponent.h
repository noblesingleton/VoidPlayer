#pragma once

#include <JuceHeader.h>
#include "VoidConvolutionEngine.h"  // Make sure this header exists

class MainComponent : public juce::AudioAppComponent,
                      public juce::ChangeListener,
                      public juce::Slider::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void sliderValueChanged (juce::Slider* slider) override;

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    VoidConvolutionEngine convolutionEngine;

    juce::TextButton loadButton       { "Load Audio" };
    juce::TextButton loadIRButton     { "Load IR" };
    juce::TextButton playStopButton   { "Play" };
    juce::Label statusLabel;
    juce::Label trackTitleLabel;
    juce::Slider wetSlider;
    juce::Label wetLabel              { "Reverb (%)" };
    juce::Slider volumeSlider;
    juce::Label volumeLabel           { "Master Volume (%)" };
    juce::Slider positionSlider;
    juce::Label positionLabel;
    juce::ToggleButton exclusiveToggle;
    juce::Label bufferSizeLabel       { "Buffer Size" };
    juce::ComboBox bufferSizeBox;

    float wetMix = 1.0f;
    float masterVolume = 1.0f;
    bool useExclusiveMode = false;
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Async IR loading
    std::atomic<VoidConvolutionEngine*> activeEngine { &convolutionEngine };
    std::unique_ptr<VoidConvolutionEngine> pendingEngine;
    juce::WaitableEvent loadingFinished;

    // Cached from prepareToPlay for async prepare
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    void applyDeviceType();
    void applyBufferSize();
    void loadFile();
    void loadImpulseResponse();
    void clearConvolutionHistory();
    void updatePositionSlider();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};