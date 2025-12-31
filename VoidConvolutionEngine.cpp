// VoidConvolutionEngine.cpp — MAX HEADROOM EDITION (December 31, 2025)

#include "VoidConvolutionEngine.h"

void VoidConvolutionEngine::prepare(double sr, int bs)
{
    sampleRate = sr;
    blockSize = bs;
    partitionSize = 256;  // Small partitions for real-time safety

    reset();
}

void VoidConvolutionEngine::reset()
{
    irQ31.clear();
    irPartitions.clear();
    inputHistoryL.assign(partitionSize, 0);
    inputHistoryR.assign(partitionSize, 0);
}

void VoidConvolutionEngine::loadIR(const juce::File& irFile)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));

    if (!reader)
    {
        DBG("VOID: Failed to read IR file");
        return;
    }

    double irSampleRate = reader->sampleRate;
    int numSamples = static_cast<int>(reader->lengthInSamples);
    int channels = juce::jmin(2, static_cast<int>(reader->numChannels));

    juce::AudioBuffer<float> floatBuffer(channels, numSamples);
    reader->read(&floatBuffer, 0, numSamples, 0, true, true);

    // Resample IR to current device sample rate if needed
    if (irSampleRate != sampleRate && irSampleRate > 0.0)
    {
        juce::LagrangeInterpolator resampler;
        int resampledLength = static_cast<int>(numSamples * (sampleRate / irSampleRate) + 1.0);
        juce::AudioBuffer<float> resampledBuffer(channels, resampledLength);

        for (int ch = 0; ch < channels; ++ch)
        {
            resampler.reset();
            int produced = resampler.process(irSampleRate / sampleRate,
                                             floatBuffer.getReadPointer(ch),
                                             resampledBuffer.getWritePointer(ch),
                                             resampledLength);
            resampledBuffer.setSize(channels, produced, false, true, true);
        }

        floatBuffer.makeCopyOf(resampledBuffer, true);
        numSamples = floatBuffer.getNumSamples();

        DBG("VOIDPLAYER_CLEAN: IR resampled from " << irSampleRate << " Hz to " << sampleRate << " Hz");
    }

    irQ31.resize(static_cast<size_t>(numSamples) * 2);

    // MAX HEADROOM — -60dB (1/1024) — the absolute limit of safety in Q31
    const float headroom = 0.0009765625f;  // -60dB

    for (int i = 0; i < numSamples; ++i)
    {
        float left = floatBuffer.getReadPointer(0)[i];
        float right = (channels > 1) ? floatBuffer.getReadPointer(1)[i] : left;

        int32_t q31Left = static_cast<int32_t>(juce::jlimit(-1.0f, 1.0f, left) * 2147483647.0f * headroom);
        int32_t q31Right = static_cast<int32_t>(juce::jlimit(-1.0f, 1.0f, right) * 2147483647.0f * headroom);

        irQ31[static_cast<size_t>(i) * 2] = q31Left;
        irQ31[static_cast<size_t>(i) * 2 + 1] = q31Right;
    }

    partitionIR();

    DBG("VOIDPLAYER_CLEAN: MAX HEADROOM (-60dB) ENGAGED – IR loaded – " << irFile.getFileName());
}

void VoidConvolutionEngine::partitionIR()
{
    irPartitions.clear();

    int numSamples = static_cast<int>(irQ31.size() / 2);
    int numPartitions = (numSamples + partitionSize - 1) / partitionSize;

    irPartitions.assign(static_cast<size_t>(numPartitions) * partitionSize * 2, 0);

    for (int p = 0; p < numPartitions; ++p)
    {
        size_t offset = static_cast<size_t>(p) * partitionSize * 2;
        size_t length = static_cast<size_t>(juce::jmin(partitionSize, numSamples - p * partitionSize)) * 2;

        std::memcpy(&irPartitions[offset], &irQ31[static_cast<size_t>(p) * partitionSize * 2], length * sizeof(int32_t));
    }

    inputHistoryL.assign(partitionSize, 0);
    inputHistoryR.assign(partitionSize, 0);
}

void VoidConvolutionEngine::processBlock(int32_t* left, int32_t* right, int numSamples)
{
    if (irQ31.empty())
        return;

    std::vector<int64_t> accL(static_cast<size_t>(numSamples), 0);
    std::vector<int64_t> accR(static_cast<size_t>(numSamples), 0);

    int numPartitions = static_cast<int>(irPartitions.size() / (partitionSize * 2));

    for (int p = 0; p < numPartitions; ++p)
    {
        const int32_t* part = &irPartitions[static_cast<size_t>(p) * partitionSize * 2];

        for (int i = 0; i < numSamples; ++i)
        {
            for (int j = 0; j < partitionSize; ++j)
            {
                int historyIdx = i - j;
                int32_t inL = (historyIdx >= 0) ? left[historyIdx] : inputHistoryL[static_cast<size_t>(historyIdx + partitionSize)];
                int32_t inR = (historyIdx >= 0) ? right[historyIdx] : inputHistoryR[static_cast<size_t>(historyIdx + partitionSize)];

                // True stereo convolution — proper cross-terms for balanced imaging
                accL[i] += static_cast<int64_t>(inL) * part[static_cast<size_t>(j) * 2] + static_cast<int64_t>(inR) * part[static_cast<size_t>(j) * 2 + 1];
                accR[i] += static_cast<int64_t>(inL) * part[static_cast<size_t>(j) * 2 + 1] + static_cast<int64_t>(inR) * part[static_cast<size_t>(j) * 2];
            }
        }
    }

    for (int i = 0; i < numSamples; ++i)
    {
        int64_t shiftedL = accL[i] >> 31;
        int64_t shiftedR = accR[i] >> 31;

        left[i] = static_cast<int32_t>(juce::jlimit(static_cast<int64_t>(-2147483648LL), static_cast<int64_t>(2147483647LL), shiftedL));
        right[i] = static_cast<int32_t>(juce::jlimit(static_cast<int64_t>(-2147483648LL), static_cast<int64_t>(2147483647LL), shiftedR));
    }

    int historyOffset = numSamples - partitionSize;
    if (historyOffset < 0) historyOffset = 0;

    std::memcpy(inputHistoryL.data(), left + historyOffset, static_cast<size_t>(partitionSize) * sizeof(int32_t));
    std::memcpy(inputHistoryR.data(), right + historyOffset, static_cast<size_t>(partitionSize) * sizeof(int32_t));
}