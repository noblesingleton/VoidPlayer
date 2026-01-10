#include "MainComponent.h"
#include <cmath>
#include <thread>

// Simple linear interpolation for block resampling (used in audio thread)
void linearResample(const juce::AudioBuffer<float>& src, juce::AudioBuffer<float>& dst, double ratio)
{
    const int srcLen = src.getNumSamples();
    const int dstLen = dst.getNumSamples();
    const int numCh = src.getNumChannels();

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* srcPtr = src.getReadPointer(ch);
        auto* dstPtr = dst.getWritePointer(ch);

        for (int i = 0; i < dstLen; ++i)
        {
            double pos = static_cast<double>(i) / ratio;
            int idx = static_cast<int>(pos);
            double frac = pos - idx;

            if (idx + 1 < srcLen)
            {
                dstPtr[i] = static_cast<float>(srcPtr[idx] * (1.0 - frac) + srcPtr[idx + 1] * frac);
            }
            else
            {
                dstPtr[i] = srcPtr[idx];
            }
        }
    }
}

// Manual IR resampling using linear interpolation (background thread)
// Defined at the top so it's visible in startUpsamplingPrep()
juce::AudioBuffer<float> resampleIR(const juce::AudioBuffer<float>& originalIR,
                                    double originalRate,
                                    double targetRate)
{
    if (originalIR.getNumSamples() == 0 || originalRate == targetRate)
        return originalIR;

    juce::AudioBuffer<float> resampled;
    int newNumSamples = static_cast<int>(originalIR.getNumSamples() * (targetRate / originalRate) + 0.5);
    resampled.setSize(originalIR.getNumChannels(), newNumSamples);

    linearResample(originalIR, resampled, targetRate / originalRate);

    return resampled;
}

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
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff00));

    addAndMakeVisible(trackTitleLabel);
    trackTitleLabel.setText("", juce::dontSendNotification);
    trackTitleLabel.setJustificationType(juce::Justification::centred);
    trackTitleLabel.setFont(juce::Font(14.0f));
    trackTitleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(wetSlider);
    wetSlider.setRange(0.0, 1.0, 0.01);
    wetSlider.setValue(1.0);
    wetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    wetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 26);
    wetSlider.setTextValueSuffix("");
    wetSlider.textFromValueFunction = [](double v) { return juce::String(static_cast<int>(v * 100)); };
    wetSlider.addListener(this);

    addAndMakeVisible(wetLabel);
    wetLabel.setText("Reverb (%)", juce::dontSendNotification);
    wetLabel.setJustificationType(juce::Justification::centred);
    wetLabel.setFont(juce::Font(16.0f, juce::Font::bold));

    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 2.0, 0.01);
    volumeSlider.setValue(1.0);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 26);
    volumeSlider.setTextValueSuffix("");
    volumeSlider.textFromValueFunction = [](double v) { return juce::String(static_cast<int>(v * 100)); };
    volumeSlider.addListener(this);

    addAndMakeVisible(volumeLabel);
    volumeLabel.setText("Master Volume (%)", juce::dontSendNotification);
    volumeLabel.setJustificationType(juce::Justification::centred);
    volumeLabel.setFont(juce::Font(16.0f, juce::Font::bold));

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

    addAndMakeVisible(exclusiveToggle);
    exclusiveToggle.setButtonText("Exclusive Mode (Bit Perfect)");
    exclusiveToggle.setToggleState(false, juce::dontSendNotification);
    exclusiveToggle.changeWidthToFitText();
    exclusiveToggle.setSize(exclusiveToggle.getWidth() + 250, 90);
    exclusiveToggle.onClick = [this] { applyDeviceType(); };

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
    bufferSizeBox.setSize(300, 50);

    // Upsampling selector
    addAndMakeVisible(upsamplingLabel);
    upsamplingLabel.setText("Upsampling", juce::dontSendNotification);
    upsamplingLabel.setJustificationType(juce::Justification::right);

    addAndMakeVisible(upsamplingBox);
    upsamplingBox.addItem("Off", 1);
    upsamplingBox.addItem("2x", 2);
    upsamplingBox.addItem("4x", 3);
    upsamplingBox.addItem("8x", 4);
    upsamplingBox.addItem("16x", 5);
    upsamplingBox.setSelectedId(1); // Off by default
    upsamplingBox.onChange = [this] { startUpsamplingPrep(); };

    // Force full volume at start
    masterVolume = 1.0f;
    volumeSlider.setValue(1.0, juce::dontSendNotification);

    // Critical: Open audio device and register callback
    setAudioChannels(0, 2);

    // Initialize active engines
    activeEngine.store(&convolutionEngine, std::memory_order_release);
    upsampledEngine.store(nullptr, std::memory_order_release);

    setSize(1024, 900);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    applyBufferSize();

    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Async IR swap (existing)
    if (pendingEngine && loadingFinished.wait(0))
    {
        auto* old = activeEngine.exchange(pendingEngine.release(), std::memory_order_release);
        if (old != &convolutionEngine) delete old;
        pendingEngine.reset();
    }

    // Upsampling engine swap
    if (pendingUpsampledEngine && upsamplingFinished.wait(0))
    {
        auto* old = upsampledEngine.exchange(pendingUpsampledEngine.release(), std::memory_order_release);
        delete old;
        pendingUpsampledEngine.reset();
    }

    if (readerSource == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);

    // Apply master volume
    for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
    {
        auto* data = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample);
        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            data[i] *= masterVolume;
        }
    }

    // Convolution engine selection with upsampling
    auto* engine = upsampledEngine.load(std::memory_order_acquire);
    if (engine && engine->isReady() && currentUpsamplingFactor > 1)
    {
        // 1. Upsample input block (simple linear)
        juce::AudioBuffer<float> upBlock(bufferToFill.buffer->getNumChannels(),
                                         bufferToFill.numSamples * currentUpsamplingFactor);
        linearResample(*bufferToFill.buffer, upBlock, static_cast<double>(currentUpsamplingFactor));

        // 2. Process with upsampled engine
        engine->processBlock(upBlock.getWritePointer(0, 0),
                             upBlock.getWritePointer(1, 0),
                             upBlock.getNumSamples());

        // 3. Downsample output block (simple linear)
        juce::AudioBuffer<float> downBlock(bufferToFill.buffer->getNumChannels(), bufferToFill.numSamples);
        linearResample(upBlock, downBlock, 1.0 / static_cast<double>(currentUpsamplingFactor));

        // Copy downsampled result back to output buffer
        for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
        {
            auto* src = downBlock.getReadPointer(ch);
            auto* dst = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample);
            std::copy(src, src + bufferToFill.numSamples, dst);
        }
    }
    else
    {
        // Normal path fallback
        auto* normalEngine = activeEngine.load(std::memory_order_acquire);
        if (normalEngine && normalEngine->isReady())
        {
            normalEngine->processBlock(bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample),
                                       bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample),
                                       bufferToFill.numSamples);
        }
    }

    if (transportSource.isPlaying())
    {
        updatePositionSlider();
    }
}

// ───────────────────────────────────────────────────────────────
//  Remaining functions (unchanged)
// ───────────────────────────────────────────────────────────────

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(40);

    juce::FlexBox mainFlex;
    mainFlex.flexDirection   = juce::FlexBox::Direction::column;
    mainFlex.flexWrap        = juce::FlexBox::Wrap::noWrap;
    mainFlex.justifyContent  = juce::FlexBox::JustifyContent::flexStart;
    mainFlex.alignItems      = juce::FlexBox::AlignItems::stretch;

    juce::FlexBox topSection;
    topSection.flexDirection = juce::FlexBox::Direction::column;
    topSection.items.add(juce::FlexItem(statusLabel).withHeight(50));
    topSection.items.add(juce::FlexItem(trackTitleLabel).withHeight(40));
    mainFlex.items.add(juce::FlexItem(topSection).withHeight(110));

    mainFlex.items.add(juce::FlexItem().withFlex(1.0f));

    juce::FlexBox buttonRow;
    buttonRow.flexDirection   = juce::FlexBox::Direction::row;
    buttonRow.justifyContent  = juce::FlexBox::JustifyContent::center;
    buttonRow.alignItems      = juce::FlexBox::AlignItems::center;
    buttonRow.items.add(juce::FlexItem(loadButton).withWidth(160).withHeight(70));
    buttonRow.items.add(juce::FlexItem(loadIRButton).withWidth(160).withHeight(70));
    buttonRow.items.add(juce::FlexItem(playStopButton).withWidth(160).withHeight(70));
    mainFlex.items.add(juce::FlexItem(buttonRow).withHeight(100));

    juce::FlexBox seekRow;
    seekRow.flexDirection = juce::FlexBox::Direction::row;
    seekRow.alignItems    = juce::FlexBox::AlignItems::center;
    seekRow.items.add(juce::FlexItem(positionSlider).withFlex(1.0f).withMinHeight(60));
    seekRow.items.add(juce::FlexItem(positionLabel).withWidth(220).withMinHeight(60));
    mainFlex.items.add(juce::FlexItem(seekRow).withHeight(100));

    mainFlex.items.add(juce::FlexItem(wetLabel).withHeight(30));
    mainFlex.items.add(juce::FlexItem(wetSlider).withHeight(80).withMinHeight(80));

    mainFlex.items.add(juce::FlexItem(volumeLabel).withHeight(30));
    mainFlex.items.add(juce::FlexItem(volumeSlider).withHeight(80).withMinHeight(80));

    juce::FlexBox bottomRow;
    bottomRow.flexDirection   = juce::FlexBox::Direction::row;
    bottomRow.justifyContent  = juce::FlexBox::JustifyContent::spaceBetween;
    bottomRow.alignItems      = juce::FlexBox::AlignItems::center;
    bottomRow.items.add(juce::FlexItem(exclusiveToggle).withMinWidth(500).withHeight(120));
    bottomRow.items.add(juce::FlexItem(bufferSizeBox).withMinWidth(350).withHeight(70));
    mainFlex.items.add(juce::FlexItem(bottomRow).withHeight(160));

    // Upsampling row
    juce::FlexBox upsamplingRow;
    upsamplingRow.flexDirection = juce::FlexBox::Direction::row;
    upsamplingRow.alignItems    = juce::FlexBox::AlignItems::center;
    upsamplingRow.items.add(juce::FlexItem(upsamplingLabel).withWidth(150).withMinHeight(40));
    upsamplingRow.items.add(juce::FlexItem(upsamplingBox).withFlex(1.0f).withMaxWidth(300).withMinHeight(40));
    mainFlex.items.add(juce::FlexItem(upsamplingRow).withHeight(80).withMinHeight(80));

    mainFlex.performLayout(area.toFloat());
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
    auto* up = upsampledEngine.exchange(nullptr, std::memory_order_release);
    delete up;
    pendingUpsampledEngine.reset();
}

void MainComponent::applyDeviceType()
{
    useExclusiveMode = exclusiveToggle.getToggleState();
    juce::AudioDeviceManager::AudioDeviceSetup setup = deviceManager.getAudioDeviceSetup();
    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = true;

    deviceManager.setCurrentAudioDeviceType("Windows Audio", true);

    if (useExclusiveMode)
    {
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
                                 if (file == juce::File{})
                                     return;

                                 statusLabel.setText("Loading IR... (background)", juce::dontSendNotification);

                                 std::thread([this, file]()
                                 {
                                     auto newEngine = std::make_unique<VoidConvolutionEngine>();
                                     newEngine->prepare(currentSampleRate, currentBlockSize);

                                     // Load the IR from file into buffer
                                     auto* reader = formatManager.createReaderFor(file);
                                     if (reader != nullptr)
                                     {
                                         juce::AudioBuffer<float> irBuffer(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
                                         reader->read(&irBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
                                         delete reader;

                                         newEngine->loadIR(irBuffer);
                                     }

                                     pendingEngine = std::move(newEngine);
                                     loadingFinished.signal();

                                     juce::MessageManager::callAsync([this, file]()
                                     {
                                         statusLabel.setText("IR: " + file.getFileName() + " – Q64 Eternal Void Active", juce::dontSendNotification);
                                     });
                                 }).detach();
                             });
}

void MainComponent::startUpsamplingPrep()
{
    int selectedId = upsamplingBox.getSelectedId();
    int factor = 1;

    switch (selectedId)
    {
        case 1: factor = 1;   break; // Off
        case 2: factor = 2;   break;
        case 3: factor = 4;   break;
        case 4: factor = 8;   break;
        case 5: factor = 16;  break;
        default: factor = 1;
    }

    currentUpsamplingFactor = factor;

    if (factor == 1)
    {
        auto* old = upsampledEngine.exchange(nullptr, std::memory_order_release);
        delete old;
        pendingUpsampledEngine.reset();
        statusLabel.setText("Upsampling Off", juce::dontSendNotification);
        return;
    }

    statusLabel.setText("Preparing upsampling ×" + juce::String(factor) + "... (background)", juce::dontSendNotification);

    std::thread([this, factor]()
    {
        auto newEngine = std::make_unique<VoidConvolutionEngine>();

        // Prepare at upsampled rate
        newEngine->prepare(currentSampleRate * factor, currentBlockSize * factor);

        // Resample current IR to factor × rate (manual linear)
        juce::AudioBuffer<float> upsampledIR = resampleIR(convolutionEngine.getCurrentIR(),
                                                          currentSampleRate,
                                                          currentSampleRate * factor);

        // Load the upsampled IR into the new engine
        newEngine->loadIR(upsampledIR);

        // Signal completion
        pendingUpsampledEngine = std::move(newEngine);
        upsamplingFinished.signal();

        juce::MessageManager::callAsync([this, factor]()
        {
            statusLabel.setText("Upsampling ×" + juce::String(factor) + " Active", juce::dontSendNotification);
        });
    }).detach();
}