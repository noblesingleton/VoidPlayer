#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class VoidConvolutionEngine
{
public:
    VoidConvolutionEngine();
    ~VoidConvolutionEngine();

    void prepare(double sampleRate, int blockSize);
    void loadIR(const juce::AudioBuffer<float>& ir);
    void processBlock(float* left, float* right, int numSamples);
    bool isReady() const;
    void reset();

    // Declaration only — no body here!
    const juce::AudioBuffer<float>& getCurrentIR() const;

private:
    juce::AudioBuffer<float> currentIR;
    double currentSampleRate{44100.0};
    int currentBlockSize{512};
    bool ready{false};
    // Add your other private members (convolution state, kernels, etc.) here
};