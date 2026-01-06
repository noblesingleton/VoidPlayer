#pragma once

#include <JuceHeader.h>

class VoidConvolutionEngine
{
public:
    VoidConvolutionEngine() { reset(); }

    void prepare(double sampleRate, int blockSize);
    void reset();
    void loadIR(const juce::File& irFile);
    bool isReady() const { return !irQ64.empty(); }
    void processBlock(float* left, float* right, int numSamples);  // <-- Changed to float*

private:
    void partitionIR();

    double sampleRate = 48000.0;
    int blockSize = 512;
    int partitionSize = 512;

    std::vector<int64_t> irQ64;
    std::vector<int64_t> irPartitions;
    std::vector<int64_t> inputHistoryL;
    std::vector<int64_t> inputHistoryR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidConvolutionEngine)
};