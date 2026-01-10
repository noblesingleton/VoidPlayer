#include "VoidConvolutionEngine.h"

VoidConvolutionEngine::VoidConvolutionEngine()
{
    ready = false;
}

VoidConvolutionEngine::~VoidConvolutionEngine()
{
}

void VoidConvolutionEngine::prepare(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;
    // Your prepare logic (allocate buffers, init convolution, etc.)
    ready = true;
}

void VoidConvolutionEngine::loadIR(const juce::AudioBuffer<float>& ir)
{
    currentIR = ir;
    // Your IR loading logic (normalize, pad, init convolution kernels, etc.)
    ready = true;
}

void VoidConvolutionEngine::processBlock(float* left, float* right, int numSamples)
{
    if (!ready) return;
    // Your actual convolution processing (fixed-point time-domain, etc.)
    // Example placeholder:
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] *= 0.5f;   // dummy processing
        right[i] *= 0.5f;
    }
}

bool VoidConvolutionEngine::isReady() const
{
    return ready;
}

void VoidConvolutionEngine::reset()
{
    ready = false;
    currentIR.clear();
    // Reset convolution state
}

const juce::AudioBuffer<float>& VoidConvolutionEngine::getCurrentIR() const
{
    return currentIR;
}