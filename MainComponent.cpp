#include "MainComponent.h"

void ConvolutionEngine::loadIR(const juce::File& irFile, juce::AudioFormatManager& manager)
{
    auto* reader = manager.createReaderFor(irFile);
    if (reader)
    {
        irKernel.resize(static_cast<size_t>(reader->lengthInSamples));
        juce::AudioBuffer<float> temp(reader->numChannels, static_cast<int>(reader->lengthInSamples));
        reader->read(&temp, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        for (int i = 0; i < irKernel.size(); ++i)
            irKernel[i] = Fixed256(temp.getReadPointer(0)[i]);
    }
}

void ConvolutionEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (irKernel.empty()) return;

    // Simple convolution — audible effect
    int irLength = static_cast<int>(irKernel.size());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            Fixed256 sum = Fixed256(data[i]);

            for (int j = 1; j < irLength; ++j)
            {
                if (i - j >= 0)
                    sum = sum + Fixed256(data[i - j]) * irKernel[j];
            }

            data[i] = std::clamp(sum.toFloat(), -1.0f, 1.0f);
        }
    }
}

void Modulator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    phase = 0.0;
}

void Modulator::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (intensity <= 0.0 || (!schumannOn && !four32On && !binauralOn)) return;

    double freq = 7.83;
    if (mode == 2) freq = -8.0;
    if (mode == 3) freq = 10.0;

    double increment = 2.0 * juce::MathConstants<double>::pi * freq / currentSampleRate;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            double mod = std::sin(phase) * intensity * 0.05;
            data[i] = std::clamp(data[i] * (1.0f + (float)mod), -1.0f, 1.0f);
            phase += increment;
        }
    }
}

MainComponent::MainComponent()
{
    deviceManager.initialiseWithDefaultDevices(0, 2);
    formatManager.registerBasicFormats();

    transport = std::make_unique<juce::AudioTransportSource>();
    player = std::make_unique<juce::AudioSourcePlayer>();
    player->setSource(transport.get());
    deviceManager.addAudioCallback(player.get());

    addAndMakeVisible(playPauseButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(previousButton);
    addAndMakeVisible(nextButton);
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(trackLabel);
    addAndMakeVisible(progressSlider);
    addAndMakeVisible(timeLabel);
    addAndMakeVisible(cpuLabel);

    addAndMakeVisible(loadIRButton);
    addAndMakeVisible(schumannToggle);
    addAndMakeVisible(four32Toggle);
    addAndMakeVisible(binauralToggle);
    addAndMakeVisible(modulatorIntensity);

    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8);
    volumeSlider.addListener(this);
    progressSlider.setRange(0.0, 1.0);
    progressSlider.setEnabled(false);

    trackLabel.setText("No track loaded", juce::dontSendNotification);
    trackLabel.setJustificationType(juce::Justification::centred);
    trackLabel.setFont(juce::Font(juce::FontOptions().withHeight(32.0f).withStyle("Bold")));
    timeLabel.setText("00:00 / 00:00", juce::dontSendNotification);
    cpuLabel.setJustificationType(juce::Justification::centredRight);

    // Sexy button styling
    auto styleButton = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        b.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::limegreen);
    };

    auto styleToggle = [](juce::ToggleButton& t)
    {
        t.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        t.setColour(juce::ToggleButton::tickColourId, juce::Colours::limegreen);
    };

    styleButton(playPauseButton);
    styleButton(stopButton);
    styleButton(previousButton);
    styleButton(nextButton);
    styleButton(loadIRButton);
    styleToggle(schumannToggle);
    styleToggle(four32Toggle);
    styleToggle(binauralToggle);

    playPauseButton.onClick = [this] {
        if (transport->isPlaying())
            transport->stop();
        else
            transport->start();
        updateTransportButtons();
    };

    stopButton.onClick = [this] {
        transport->stop();
        transport->setPosition(0.0);
        updateTransportButtons();
        updateProgress();
    };

    previousButton.onClick = [this] {
        if (currentTrackIndex > 0)
            loadTrack(--currentTrackIndex);
    };

    nextButton.onClick = [this] {
        if (currentTrackIndex + 1 < playlist.size())
            loadTrack(++currentTrackIndex);
    };

    loadIRButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>("Load Impulse Response WAV", juce::File(), "*.wav");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                 [this](const juce::FileChooser& fc)
                                 {
                                     if (fc.getResult().existsAsFile())
                                     {
                                         convolution.loadIR(fc.getResult(), formatManager);
                                     }
                                     fileChooser.reset();
                                 });
    };

    schumannToggle.onClick = [this] {
        modulator.setSchumann(schumannToggle.getToggleState());
    };

    four32Toggle.onClick = [this] {
        modulator.set432Hz(four32Toggle.getToggleState());
    };

    binauralToggle.onClick = [this] {
        modulator.setBinaural(binauralToggle.getToggleState());
    };

    modulatorIntensity.onValueChange = [this] {
        modulator.setIntensity(modulatorIntensity.getValue());
    };

    startTimer(500);

    setSize(1200, 700);
}

MainComponent::~MainComponent()
{
    deviceManager.removeAudioCallback(player.get());
    transport->setSource(nullptr);
    player->setSource(nullptr);
    stopTimer();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::limegreen);
    g.setFont(36.0f);
    g.drawFittedText("Void Software Player v2.∞\nPhase 3.1 – Revolution Unleashed", getLocalBounds().removeFromTop(80), juce::Justification::centred, 1);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(50);

    trackLabel.setBounds(bounds.removeFromTop(100));

    auto controlArea = bounds.removeFromBottom(280);  // Increased height

    auto transportArea = controlArea.removeFromTop(60);
    auto buttonWidth = 140;
    auto totalWidth = buttonWidth * 4 + 30 * 3;
    auto left = (transportArea.getWidth() - totalWidth) / 2;

    previousButton.setBounds(transportArea.withX(left).removeFromLeft(buttonWidth));
    stopButton.setBounds(transportArea.withX(left + buttonWidth + 30).removeFromLeft(buttonWidth));
    playPauseButton.setBounds(transportArea.withX(left + (buttonWidth + 30) * 2).removeFromLeft(buttonWidth));
    nextButton.setBounds(transportArea.withX(left + (buttonWidth + 30) * 3).removeFromLeft(buttonWidth));

    volumeSlider.setBounds(controlArea.removeFromTop(50));

    auto resonancePanel = controlArea.removeFromRight(450);

    loadIRButton.setBounds(resonancePanel.removeFromTop(50));

    auto toggleHeight = 45;
    schumannToggle.setBounds(resonancePanel.removeFromTop(toggleHeight));
    four32Toggle.setBounds(resonancePanel.removeFromTop(toggleHeight));
    binauralToggle.setBounds(resonancePanel.removeFromTop(toggleHeight));

    modulatorIntensity.setBounds(resonancePanel);

    progressSlider.setBounds(bounds.removeFromBottom(40));
    timeLabel.setBounds(bounds.removeFromBottom(40));

    cpuLabel.setBounds(getLocalBounds().removeFromTop(40).removeFromRight(200));
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray&)
{
    return true;
}

void MainComponent::filesDropped(const juce::StringArray& files, int, int)
{
    playlist.clear();
    trackNames.clear();

    for (const auto& path : files)
    {
        juce::File file(path);
        if (file.existsAsFile())
        {
            auto* reader = formatManager.createReaderFor(file);
            if (reader != nullptr)
            {
                playlist.push_back(std::make_unique<juce::AudioFormatReaderSource>(reader, true));
                trackNames.push_back(file.getFileName());
            }
        }
    }

    if (!playlist.empty())
        loadTrack(0);
}

void MainComponent::loadTrack(size_t index)
{
    if (index < playlist.size())
    {
        currentTrackIndex = index;
        transport->setSource(playlist[index].get(), 0, nullptr, playlist[index]->getAudioFormatReader()->sampleRate);
        trackLabel.setText(trackNames[index], juce::dontSendNotification);
        transport->setPosition(0.0);
        updateTransportButtons();
        updateProgress();
    }
}

void MainComponent::updateTransportButtons()
{
    playPauseButton.setButtonText(transport->isPlaying() ? "Pause" : "Play");
}

void MainComponent::updateProgress()
{
    if (transport->getLengthInSeconds() > 0)
    {
        double pos = transport->getCurrentPosition();
        double len = transport->getLengthInSeconds();
        progressSlider.setValue(pos / len, juce::dontSendNotification);

        int posMin = static_cast<int>(pos) / 60;
        int posSec = static_cast<int>(pos) % 60;
        int lenMin = static_cast<int>(len) / 60;
        int lenSec = static_cast<int>(len) % 60;

        timeLabel.setText(juce::String::formatted("%02d:%02d / %02d:%02d", posMin, posSec, lenMin, lenSec),
                          juce::dontSendNotification);
    }
}

void MainComponent::timerCallback()
{
    cpuUsage = deviceManager.getCpuUsage() * 100.0;
    cpuLabel.setText("CPU: " + juce::String(cpuUsage, 1) + "%", juce::dontSendNotification);
    updateProgress();
    if (transport->hasStreamFinished() && currentTrackIndex + 1 < playlist.size())
        loadTrack(currentTrackIndex + 1);
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &volumeSlider)
        volumeLevel = Fixed256(slider->getValue());
}