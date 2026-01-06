#include "VoidConvolutionEngine.h"

constexpr double q64Max = 9223372036854775807.0;

void VoidConvolutionEngine::prepare(double sr, int bs)
{
    sampleRate = sr;
    blockSize = bs;
    partitionSize = 512;
    reset();
}

void VoidConvolutionEngine::reset()
{
    irQ64.clear();
    irPartitions.clear();
    inputHistoryL.assign(partitionSize, 0);
    inputHistoryR.assign(partitionSize, 0);
}

void VoidConvolutionEngine::loadIR(const juce::File& irFile)
{
    // (same as before — no change needed)
    // ... (keep your existing loadIR code)
}

void VoidConvolutionEngine::partitionIR()
{
    // (same as before — no change needed)
    // ... (keep your existing partitionIR code)
}

void VoidConvolutionEngine::processBlock(float* left, float* right, int numSamples)
{
    if (irQ64.empty())
        return;

    std::vector<int64_t> accL(static_cast<size_t>(numSamples), 0);
    std::vector<int64_t> accR(static_cast<size_t>(numSamples), 0);

    const int numPartitions = static_cast<int>(irPartitions.size() / (partitionSize * 2));

    for (int p = 0; p < numPartitions; ++p)
    {
        const int64_t* part = &irPartitions[static_cast<size_t>(p) * partitionSize * 2];

        for (int i = 0; i < numSamples; ++i)
        {
            for (int j = 0; j < partitionSize; ++j)
            {
                int historyIdx = i - j;
                int64_t inL = (historyIdx >= 0) ? static_cast<int64_t>(left[historyIdx] * q64Max) : inputHistoryL[static_cast<size_t>(historyIdx + partitionSize)];
                int64_t inR = (historyIdx >= 0) ? static_cast<int64_t>(right[historyIdx] * q64Max) : inputHistoryR[static_cast<size_t>(historyIdx + partitionSize)];

                accL[i] += inL * part[static_cast<size_t>(j) * 2 + 0];
                accL[i] += inR * part[static_cast<size_t>(j) * 2 + 1];

                accR[i] += inL * part[static_cast<size_t>(j) * 2 + 1];
                accR[i] += inR * part[static_cast<size_t>(j) * 2 + 0];

                accL[i] = juce::jlimit<int64_t>(-9223372036854775807LL, 9223372036854775807LL, accL[i]);
                accR[i] = juce::jlimit<int64_t>(-9223372036854775807LL, 9223372036854775807LL, accR[i]);
            }
        }
    }

    for (int i = 0; i < numSamples; ++i)
    {
        left[i] += static_cast<float>(accL[i] / q64Max);
        right[i] += static_cast<float>(accR[i] / q64Max);
    }

    size_t copySize = static_cast<size_t>(juce::jmin(partitionSize, numSamples));
    std::memcpy(inputHistoryL.data(), left + numSamples - copySize, copySize * sizeof(int64_t));
    std::memcpy(inputHistoryR.data(), right + numSamples - copySize, copySize * sizeof(int64_t));

    if (numSamples < partitionSize)
    {
        std::memset(inputHistoryL.data() + copySize, 0, (partitionSize - copySize) * sizeof(int64_t));
        std::memset(inputHistoryR.data() + copySize, 0, (partitionSize - copySize) * sizeof(int64_t));
    }
}