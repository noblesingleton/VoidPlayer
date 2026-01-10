// MainComponent.h
#pragma once

#include <JuceHeader.h>
#include "VoidConvolutionEngine.h"

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

    // ───────────────────────────────────────────────────────────────
    //  Existing async IR loading
    // ───────────────────────────────────────────────────────────────
    std::atomic<VoidConvolutionEngine*> activeEngine { &convolutionEngine };
    std::unique_ptr<VoidConvolutionEngine> pendingEngine;
    juce::WaitableEvent loadingFinished;

    // ───────────────────────────────────────────────────────────────
    //  NEW: Upsampling support
    // ───────────────────────────────────────────────────────────────
    std::atomic<VoidConvolutionEngine*> upsampledEngine { nullptr };
    std::unique_ptr<VoidConvolutionEngine> pendingUpsampledEngine;
    juce::WaitableEvent upsamplingFinished;
    int currentUpsamplingFactor = 1; // 1 = off, 2/4/8/16 = active

    // Cached audio settings for background preparation
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // UI for upsampling
    juce::Label upsamplingLabel { "Upsampling" };
    juce::ComboBox upsamplingBox;

    float wetMix = 1.0f;
    float masterVolume = 1.0f;
    bool useExclusiveMode = false;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void applyDeviceType();
    void applyBufferSize();
    void loadFile();
    void loadImpulseResponse();
    void clearConvolutionHistory();
    void updatePositionSlider();

    // ───────────────────────────────────────────────────────────────
    //  NEW: Upsampling preparation
    // ───────────────────────────────────────────────────────────────
    void startUpsamplingPrep();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};