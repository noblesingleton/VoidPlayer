#include "MainComponent.h"

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

    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8);
    volumeSlider.addListener(this);
    progressSlider.setRange(0.0, 1.0);
    progressSlider.setEnabled(false);

    trackLabel.setText("No track loaded", juce::dontSendNotification);
    trackLabel.setJustificationType(juce::Justification::centred);
    trackLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    timeLabel.setText("00:00 / 00:00", juce::dontSendNotification);
    cpuLabel.setJustificationType(juce::Justification::centredRight);

    // Sexy button styling
    auto styleButton = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        b.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::limegreen);
    };

    styleButton(playPauseButton);
    styleButton(stopButton);
    styleButton(previousButton);
    styleButton(nextButton);

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

    startTimer(500);

    setSize(1000, 700);
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
    g.drawFittedText("Void Software Player v2.∞\nPhase 2 – Final Build", getLocalBounds().removeFromTop(80), juce::Justification::centred, 1);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(50);

    trackLabel.setBounds(bounds.removeFromTop(100));

    auto controlArea = bounds.removeFromBottom(120);
    auto buttonArea = controlArea.removeFromLeft(controlArea.getWidth() - 250);

    auto buttonWidth = 140;
    auto spacing = (buttonArea.getWidth() - buttonWidth * 4) / 5;

    previousButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    stopButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    playPauseButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(spacing);
    nextButton.setBounds(buttonArea.removeFromLeft(buttonWidth));

    volumeSlider.setBounds(controlArea);

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

void MainComponent::processBlock(juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            Fixed256 sample(channelData[i]);

            sample = sample * volumeLevel;

            channelData[i] = sample.toFloat();
        }
    }
}