#include "MainComponent.h"
#include <cmath>

MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);

    // Dark VOID theme
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0a1a2a));
    setColour(juce::TextButton::textColourOffId, juce::Colours::cyan);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff0a1a2a));
    setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    setColour(juce::ToggleButton::textColourId, juce::Colours::cyan);
    setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);

    addAndMakeVisible(loadButton);
    loadButton.onClick = [this] { loadFile(); };

    addAndMakeVisible(loadIRButton);
    loadIRButton.onClick = [this] { loadImpulseResponse(); };

    addAndMakeVisible(playStopButton);
    playStopButton.onClick = [this]
    {
        if (transportSource.isPlaying())
        {
            transportSource.stop();
            clearConvolutionHistory();
        }
        else
        {
            if (transportSource.hasStreamFinished())
            {
                transportSource.setPosition(0.0);
                clearConvolutionHistory();
            }
            transportSource.start();
        }
    };
    playStopButton.setEnabled(false);

    addAndMakeVisible(statusLabel);
    statusLabel.setText("No IR loaded – Dry path active", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff00));  // Lime green

    addAndMakeVisible(trackTitleLabel);
    trackTitleLabel.setText("", juce::dontSendNotification);
    trackTitleLabel.setJustificationType(juce::Justification::centred);
    trackTitleLabel.setFont(juce::Font(14.0f));
    trackTitleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Reverb Slider
    addAndMakeVisible(wetSlider);
    wetSlider.setRange(0.0, 1.0, 0.01);
    wetSlider.setValue(1.0);
    wetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    wetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 26);
    wetSlider.setTextValueSuffix("");
    wetSlider.textFromValueFunction = [](double v) { return juce::String(static_cast<int>(v * 100)); };
    wetSlider.addListener(this);

    addAndMakeVisible(wetLabel);
    wetLabel.setText("Reverb", juce::dontSendNotification);
    wetLabel.attachToComponent(&wetSlider, true);

    // Master Volume Slider
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 2.0, 0.01);
    volumeSlider.setValue(1.0);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 26);
    volumeSlider.setTextValueSuffix("");
    volumeSlider.textFromValueFunction = [](double v) { return juce::String(static_cast<int>(v * 100)); };
    volumeSlider.addListener(this);

    addAndMakeVisible(volumeLabel);
    volumeLabel.setText("Master Volume", juce::dontSendNotification);
    volumeLabel.attachToComponent(&volumeSlider, true);

    // Seek bar
    addAndMakeVisible(positionSlider);
    positionSlider.setRange(0.0, 1.0);
    positionSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    positionSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    positionSlider.setMouseDragSensitivity(400);
    positionSlider.setVelocityBasedMode(false);
    positionSlider.addListener(this);

    addAndMakeVisible(positionLabel);
    positionLabel.setText("0:00 / 0:00", juce::dontSendNotification);
    positionLabel.setJustificationType(juce::Justification::right);

    // ASIO/WASAPI — PRIORITY #1 — LARGE, VISIBLE, FIRST
    addAndMakeVisible(exclusiveToggle);
    exclusiveToggle.setButtonText("Exclusive Mode (ASIO/WASAPI)");
    exclusiveToggle.setToggleState(false, juce::dontSendNotification);
    exclusiveToggle.changeWidthToFitText();
    exclusiveToggle.setSize(exclusiveToggle.getWidth() + 250, 90);  // HUGE, DOMINANT
    exclusiveToggle.onClick = [this] { applyDeviceType(); };

    // Buffer size selector — larger, readable
    addAndMakeVisible(bufferSizeLabel);
    bufferSizeLabel.setText("Buffer Size", juce::dontSendNotification);
    bufferSizeLabel.setJustificationType(juce::Justification::right);
    bufferSizeLabel.setFont(juce::Font(16.0f));

    addAndMakeVisible(bufferSizeBox);
    bufferSizeBox.addItem("64 samples (ultra-low latency)", 64);
    bufferSizeBox.addItem("128 samples (low latency)", 128);
    bufferSizeBox.addItem("256 samples (balanced)", 256);
    bufferSizeBox.addItem("512 samples (stable)", 512);
    bufferSizeBox.setSelectedId(256);
    bufferSizeBox.onChange = [this] { applyBufferSize(); };
    bufferSizeBox.setSize(300, 50);  // Larger, readable

    setSize(1024, 900);  // Perfect startup size

    setAudioChannels(0, 2);
    deviceManager.initialise(0, 2, nullptr, false);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    convolutionEngine.prepare(sampleRate, samplesPerBlockExpected);

    applyBufferSize();
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

        convolutionEngine.processBlock(floatL, floatR, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            float wetL = floatL[i];
            float wetR = floatR[i];

            float mixedL = (floatL[i] * (1.0f - wetMix)) + (wetL * wetMix);
            float mixedR = (floatR[i] * (1.0f - wetMix)) + (wetR * wetMix);

            mixedL *= masterVolume;
            mixedR *= masterVolume;

            mixedL = std::tanh(mixedL);
            mixedR = std::tanh(mixedR);

            floatL[i] = mixedL;
            floatR[i] = mixedR;
        }
    }
    else
    {
        float* floatL = bufferToFill.buffer->getWritePointer(0);
        float* floatR = bufferToFill.buffer->getWritePointer(1);

        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            floatL[i] *= masterVolume;
            floatR[i] *= masterVolume;
            floatL[i] = std::tanh(floatL[i]);
            floatR[i] = std::tanh(floatR[i]);
        }
    }

    if (transportSource.isPlaying())
    {
        updatePositionSlider();
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
    auto area = getLocalBounds().reduced(30);

    // Top — Status + Track title
    auto topRow = area.removeFromTop(60);
    statusLabel.setBounds(topRow);
    auto titleRow = area.removeFromTop(40);
    trackTitleLabel.setBounds(titleRow);

    area.removeFromTop(20);

    // Middle open space — 35% height — sacrificed for bottom
    auto mainArea = area.removeFromTop(static_cast<int>(area.getHeight() * 0.35));

    // Bottom control bar — 65% height — MAX ROOM
    auto controlArea = area.reduced(0, 30);

    // Row 1 — Buttons
    auto row1 = controlArea.removeFromTop(70);
    auto buttonWidth = 150;
    loadButton.setBounds(row1.removeFromLeft(buttonWidth).reduced(10));
    row1.removeFromLeft(30);
    loadIRButton.setBounds(row1.removeFromLeft(buttonWidth).reduced(10));
    row1.removeFromLeft(30);
    playStopButton.setBounds(row1.removeFromLeft(buttonWidth).reduced(10));

    controlArea.removeFromTop(30);

    // Row 2 — Progress bar
    auto progressRow = controlArea.removeFromTop(60);
    positionSlider.setBounds(progressRow.reduced(10, 0));
    positionLabel.setBounds(progressRow.withWidth(200).withRightX(progressRow.getRight()));

    controlArea.removeFromTop(40);

    // Row 3 — Reverb + Volume
    auto sliderRow = controlArea.removeFromTop(90);
    auto wetArea = sliderRow.removeFromLeft(460);
    wetLabel.setBounds(wetArea.removeFromLeft(140).withTrimmedTop(10));
    wetSlider.setBounds(wetArea.reduced(10, 0));

    sliderRow.removeFromLeft(100);

    auto volumeArea = sliderRow;
    volumeLabel.setBounds(volumeArea.removeFromLeft(160).withTrimmedTop(10));
    volumeSlider.setBounds(volumeArea.reduced(10, 0));

    controlArea.removeFromTop(60);

    // Row 4 — ASIO/WASAPI FIRST, LARGE, VISIBLE
    auto optionsRow = controlArea.removeFromTop(130);  // TALL ROW
    exclusiveToggle.setBounds(optionsRow.removeFromLeft(550).reduced(15));  // LEFT, DOMINANT
    optionsRow.removeFromLeft(60);
    bufferSizeLabel.setBounds(optionsRow.removeFromLeft(160));
    bufferSizeBox.setBounds(optionsRow.removeFromLeft(300));
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        playStopButton.setButtonText(transportSource.isPlaying() ? "Stop" : "Play");

        if (transportSource.hasStreamFinished())
        {
            updatePositionSlider();
            clearConvolutionHistory();
        }
    }
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &wetSlider)
    {
        wetMix = static_cast<float>(wetSlider.getValue());
    }
    else if (slider == &volumeSlider)
    {
        masterVolume = static_cast<float>(volumeSlider.getValue());
    }
    else if (slider == &positionSlider)
    {
        if (transportSource.getLengthInSeconds() > 0.0)
        {
            double position = positionSlider.getValue() * transportSource.getLengthInSeconds();
            transportSource.setPosition(position);
            updatePositionSlider();
        }
    }
}

void MainComponent::updatePositionSlider()
{
    double current = transportSource.getCurrentPosition();
    double total = transportSource.getLengthInSeconds();

    if (total > 0.0)
    {
        positionSlider.setValue(current / total, juce::dontSendNotification);

        int currentMins = (int)(current / 60);
        int currentSecs = (int)current % 60;
        int totalMins = (int)(total / 60);
        int totalSecs = (int)total % 60;

        positionLabel.setText(juce::String::formatted("%02d:%02d / %02d:%02d",
                                                      currentMins, currentSecs, totalMins, totalSecs),
                              juce::dontSendNotification);
    }
    else
    {
        positionLabel.setText("0:00 / 0:00", juce::dontSendNotification);
    }
}

void MainComponent::clearConvolutionHistory()
{
    convolutionEngine.reset();
}

void MainComponent::applyDeviceType()
{
    useExclusiveMode = exclusiveToggle.getToggleState();

    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager.getAudioDeviceSetup();
    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = true;

    if (useExclusiveMode)
    {
        bool hasASIO = false;
        for (auto* type : deviceManager.getAvailableDeviceTypes())
        {
            if (type->getTypeName() == "ASIO")
            {
                hasASIO = true;
                break;
            }
        }

        if (hasASIO)
        {
            deviceManager.setCurrentAudioDeviceType("ASIO", true);
        }
        else
        {
            deviceManager.setCurrentAudioDeviceType("Windows Audio", true);
        }

        deviceManager.initialise(0, 2, nullptr, true, juce::String(), &setup);
    }
    else
    {
        deviceManager.initialise(0, 2, nullptr, false);
    }
}

void MainComponent::applyBufferSize()
{
    int selectedSize = bufferSizeBox.getSelectedId();

    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager.getAudioDeviceSetup();
    setup.bufferSize = selectedSize;
    deviceManager.setAudioDeviceSetup(setup, true);
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
                                         updatePositionSlider();

                                         trackTitleLabel.setText("Track: " + file.getFileNameWithoutExtension(), juce::dontSendNotification);
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
                                     statusLabel.setText("IR: " + file.getFileName() + " – Q64 Eternal Void Active", juce::dontSendNotification);
                                 }
                             });
}