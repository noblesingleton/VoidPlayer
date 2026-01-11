#pragma once

#include <JuceHeader.h>
#include "VoidConvolutionEngine.h"

class MainComponent  : public juce::AudioAppComponent,
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

    void applyDeviceType();
    void applyBufferSize();
    void loadFile();
    void loadImpulseResponse();
    void startUpsamplingPrep();
    void updatePositionSlider();
    void clearConvolutionHistory();

private:
    // UI Components
    juce::TextButton loadButton { "Load Track" };
    juce::TextButton loadIRButton { "Load IR" };
    juce::TextButton playStopButton { "Play" };
    juce::Label statusLabel;
    juce::Label trackTitleLabel;
    juce::Slider wetSlider;
    juce::Label wetLabel;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    juce::Slider positionSlider;
    juce::Label positionLabel;
    juce::ToggleButton exclusiveToggle;
    juce::Label bufferSizeLabel;
    juce::ComboBox bufferSizeBox;
    juce::Label upsamplingLabel;
    juce::ComboBox upsamplingBox;

    // Audio objects
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioDeviceManager deviceManager;

    // Convolution engines
    VoidConvolutionEngine convolutionEngine;
    std::atomic<VoidConvolutionEngine*> activeEngine{nullptr};
    std::atomic<VoidConvolutionEngine*> upsampledEngine{nullptr};
    std::unique_ptr<VoidConvolutionEngine> pendingEngine;
    std::unique_ptr<VoidConvolutionEngine> pendingUpsampledEngine;
    juce::WaitableEvent loadingFinished;
    juce::WaitableEvent upsamplingFinished;

    // State variables
    double currentSampleRate{44100.0};
    int currentBlockSize{512};
    int currentUpsamplingFactor{1};
    float wetMix{1.0f};
    float masterVolume{1.0f};
    bool useExclusiveMode{false};

    // Random for TPDF dither (used in volume loop)
    juce::Random rand;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};