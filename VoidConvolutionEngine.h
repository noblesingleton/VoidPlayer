#pragma once

#include <JuceHeader.h>

class VoidConvolutionEngine
{
public:
    VoidConvolutionEngine() { reset(); }

    void prepare(double sampleRate, int blockSize);
    void reset();
    void loadIR(const juce::File& irFile);
    bool isReady() const { return !irQ31.empty(); }
    void processBlock(int32_t* left, int32_t* right, int numSamples);

private:
    void partitionIR();

    double sampleRate = 48000.0;
    int blockSize = 512;
    int partitionSize = 512;

    std::vector<int32_t> irQ31;
    std::vector<int32_t> irPartitions;
    std::vector<int32_t> inputHistoryL;
    std::vector<int32_t> inputHistoryR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidConvolutionEngine)
};