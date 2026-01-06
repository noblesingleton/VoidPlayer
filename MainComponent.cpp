#include "MainComponent.h"
#include <cmath>

MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);

    // Void Aesthetic
    setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    setColour(juce::Slider::backgroundColourId, juce::Colours::black);

    addAndMakeVisible(loadButton);
    loadButton.setButtonText("Load Audio");
    loadButton.onClick = [this] { loadFile(); };

    addAndMakeVisible(loadIRButton);
    loadIRButton.setButtonText("Load IR");
    loadIRButton.onClick = [this] { loadImpulseResponse(); };

    addAndMakeVisible(playStopButton);
    playStopButton.setButtonText("Play");
    playStopButton.onClick = [this]
    {
        if (transportSource.isPlaying())
        {
            transportSource.stop();
            convolutionEngine.reset();  // Clear tail on stop
        }
        else
        {
            transportSource.start();
        }
    };
    playStopButton.setEnabled(false);

    addAndMakeVisible(statusLabel);
    statusLabel.setText("No IR loaded – Enter the void", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(18.0f, juce::Font::bold));

    // Wet Slider (horizontal)
    addAndMakeVisible(wetSlider);
    wetSlider.setRange(0.0, 1.0, 0.01);
    wetSlider.setValue(1.0);
    wetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    wetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    wetSlider.addListener(this);

    addAndMakeVisible(wetLabel);
    wetLabel.setText("Dry / Wet", juce::dontSendNotification);
    wetLabel.attachToComponent(&wetSlider, true);

    addAndMakeVisible(wetValueLabel);
    wetValueLabel.setText("100% Wet", juce::dontSendNotification);

    // Volume Slider (horizontal)
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 2.0, 0.01);
    volumeSlider.setValue(1.0);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    volumeSlider.addListener(this);

    addAndMakeVisible(volumeLabel);
    volumeLabel.setText("Master Volume", juce::dontSendNotification);
    volumeLabel.attachToComponent(&volumeSlider, true);

    addAndMakeVisible(volumeValueLabel);
    volumeValueLabel.setText("100%", juce::dontSendNotification);

    setSize(800, 500);
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

        // Preserve dry signal
        std::vector<float> dryL(floatL, floatL + numSamples);
        std::vector<float> dryR(floatR, floatR + numSamples);

        // Apply convolution to wet copy
        std::vector<float> wetL(numSamples);
        std::vector<float> wetR(numSamples);
        std::memcpy(wetL.data(), floatL, numSamples * sizeof(float));
        std::memcpy(wetR.data(), floatR, numSamples * sizeof(float));

        convolutionEngine.processBlock(wetL.data(), wetR.data(), numSamples);

        // Mix dry + wet + clamp + tanh
        for (int i = 0; i < numSamples; ++i)
        {
            float mixedL = dryL[i] * (1.0f - wetMix) + wetL[i] * wetMix;
            float mixedR = dryR[i] * (1.0f - wetMix) + wetR[i] * wetMix;

            mixedL *= masterVolume;
            mixedR *= masterVolume;

            // Hard clamp before tanh
            mixedL = juce::jlimit(-1.0f, 1.0f, mixedL);
            mixedR = juce::jlimit(-1.0f, 1.0f, mixedR);

            mixedL = std::tanh(mixedL);
            mixedR = std::tanh(mixedR);

            floatL[i] = mixedL;
            floatR[i] = mixedR;
        }
    }
    else
    {
        // Dry path
        float* floatL = bufferToFill.buffer->getWritePointer(0);
        float* floatR = bufferToFill.buffer->getWritePointer(1);

        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            floatL[i] *= masterVolume;
            floatR[i] *= masterVolume;

            floatL[i] = juce::jlimit(-1.0f, 1.0f, floatL[i]);
            floatR[i] = juce::jlimit(-1.0f, 1.0f, floatR[i]);

            floatL[i] = std::tanh(floatL[i]);
            floatR[i] = std::tanh(floatR[i]);
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

    auto buttonArea = area.removeFromLeft(200);
    auto buttonHeight = 60;

    loadButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(20);
    loadIRButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(20);
    playStopButton.setBounds(buttonArea.removeFromTop(buttonHeight));

    area.removeFromLeft(40);

    statusLabel.setBounds(area.removeFromTop(60));

    area.removeFromTop(40);

    auto wetRow = area.removeFromTop(60);
    wetLabel.setBounds(wetRow.removeFromLeft(150));
    wetSlider.setBounds(wetRow.removeFromLeft(400));
    wetValueLabel.setBounds(wetRow);

    area.removeFromTop(30);

    auto volumeRow = area.removeFromTop(60);
    volumeLabel.setBounds(volumeRow.removeFromLeft(150));
    volumeSlider.setBounds(volumeRow.removeFromLeft(400));
    volumeValueLabel.setBounds(volumeRow);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        playStopButton.setButtonText(transportSource.isPlaying() ? "Stop" : "Play");
    }
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &wetSlider)
    {
        wetMix = static_cast<float>(wetSlider.getValue());
        wetValueLabel.setText(juce::String(static_cast<int>(wetMix * 100)) + "% Wet", juce::dontSendNotification);
    }
    else if (slider == &volumeSlider)
    {
        masterVolume = static_cast<float>(volumeSlider.getValue());
        volumeValueLabel.setText(juce::String(static_cast<int>(masterVolume * 100)) + "%", juce::dontSendNotification);
    }
}

void MainComponent::loadFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select audio file...", juce::File{}, "*.wav;*.mp3;*.flac;*.aiff;*.ogg");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file != juce::File{})
                                 {
                                     auto* reader = formatManager.createReaderFor(file);
                                     if (reader != nullptr)
                                     {
                                         auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                                         transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                                         readerSource = std::move(newSource);
                                         playStopButton.setEnabled(true);
                                         playStopButton.setButtonText("Play");
                                     }
                                 }
                             });
}

void MainComponent::loadImpulseResponse()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Impulse Response – Enter the void", juce::File{}, "*.wav");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file != juce::File{})
                                 {
                                     convolutionEngine.loadIR(file);
                                     statusLabel.setText("IR: " + file.getFileName() + " – VOID Eternal Active", juce::dontSendNotification);
                                 }
                             });
}