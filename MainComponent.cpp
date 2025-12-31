// MainComponent.cpp — Final version with working Wet Gain slider and soft clipping (December 31, 2025)

#include "MainComponent.h"
#include <cmath>  // for std::tanh

MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);

    addAndMakeVisible(loadButton);
    loadButton.onClick = [this] { loadFile(); };

    addAndMakeVisible(loadIRButton);
    loadIRButton.onClick = [this] { loadImpulseResponse(); };

    addAndMakeVisible(playButton);
    playButton.onClick = [this]
    {
        if (transportSource.isPlaying())
            transportSource.stop();
        else
            transportSource.start();
    };
    playButton.setEnabled(false);

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this]
    {
        transportSource.stop();
        transportSource.setPosition(0.0);
    };

    loadButton.setButtonText("Load Audio");
    loadIRButton.setButtonText("Load IR");
    playButton.setButtonText("Play");
    stopButton.setButtonText("Stop");

    addAndMakeVisible(statusLabel);
    statusLabel.setText("No IR loaded – Dry path active", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    statusLabel.setFont(juce::Font(juce::FontOptions(15.0f)));

    // Wet Gain slider — visible and fully responsive
    addAndMakeVisible(wetSlider);
    wetSlider.setRange(0.0, 64.0, 0.1);
    wetSlider.setValue(16.0);  // Start with +24dB wet boost
    wetSlider.setSliderStyle(juce::Slider::LinearVertical);
    wetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    wetSlider.addListener(this);

    addAndMakeVisible(wetLabel);
    wetLabel.setText("Wet Gain", juce::dontSendNotification);
    wetLabel.attachToComponent(&wetSlider, false);
    wetLabel.setJustificationType(juce::Justification::centred);

    setSize(700, 400);
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    convolutionEngine.prepare(sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);

    if (convolutionEngine.isReady())
    {
        const int numSamples = bufferToFill.numSamples;
        const int start = bufferToFill.startSample;

        float* floatL = bufferToFill.buffer->getWritePointer(0, start);
        float* floatR = bufferToFill.buffer->getWritePointer(1, start);

        // Thread-local Q31 buffers
        static thread_local std::vector<int32_t> q31L(numSamples);
        static thread_local std::vector<int32_t> q31R(numSamples);
        q31L.resize(numSamples);
        q31R.resize(numSamples);

        // Float → Q31
        for (int i = 0; i < numSamples; ++i)
        {
            q31L[i] = static_cast<int32_t>(floatL[i] * 2147483647.0f);
            q31R[i] = static_cast<int32_t>(floatR[i] * 2147483647.0f);
        }

        // Pure fixed-point convolution
        convolutionEngine.processBlock(q31L.data(), q31R.data(), numSamples);

        // Q31 → Float with wet gain, 50/50 mix, and soft clipping (tanh)
        for (int i = 0; i < numSamples; ++i)
        {
            float wetL = q31L[i] / 2147483648.0f * wetGain;
            float wetR = q31R[i] / 2147483648.0f * wetGain;

            float mixedL = (floatL[i] * 0.5f) + (wetL * 0.5f);
            float mixedR = (floatR[i] * 0.5f) + (wetR * 0.5f);

            // Soft clipping — tanh prevents harsh distortion at high gain
            mixedL = std::tanh(mixedL);
            mixedR = std::tanh(mixedR);

            floatL[i] = mixedL;
            floatR[i] = mixedR;
        }
    }
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
    convolutionEngine.reset();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(40);

    auto topRow = area.removeFromTop(80);
    loadButton.setBounds(topRow.removeFromLeft(150));
    loadIRButton.setBounds(topRow.removeFromLeft(150).translated(20, 0));

    area.removeFromTop(20);

    auto playRow = area.removeFromTop(80);
    playButton.setBounds(playRow.removeFromLeft(150));
    stopButton.setBounds(playRow.removeFromLeft(150).translated(20, 0));

    area.removeFromTop(20);

    statusLabel.setBounds(area.removeFromTop(60));

    auto sliderArea = getLocalBounds().reduced(20).removeFromRight(120);
    wetSlider.setBounds(sliderArea.removeFromBottom(250));
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
        playButton.setButtonText(transportSource.isPlaying() ? "Stop" : "Play");
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &wetSlider)
        wetGain = static_cast<float>(wetSlider.getValue());
}

void MainComponent::loadFile()
{
    juce::FileChooser chooser("Select audio file to convolve...", {}, "*.wav;*.mp3;*.flac;*.aiff;*.ogg");

    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        auto* reader = formatManager.createReaderFor(file);

        if (reader != nullptr)
        {
            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
            readerSource = std::move(newSource);
            playButton.setEnabled(true);
            playButton.setButtonText("Play");
        }
    }
}

void MainComponent::loadImpulseResponse()
{
    juce::FileChooser chooser("Load Impulse Response – Enter the void", {}, "*.wav");

    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        convolutionEngine.loadIR(file);

        statusLabel.setText("IR: " + file.getFileName() + " – Void breathing pure Q31", juce::dontSendNotification);
    }
}