#include <stdint.h>

#include "main.h"
#include "hardwareTest.h"
#include "dma.h"

// One sine period: 48 frames, 10 percent of full scale.
static const int32_t dacTestTone[48] = {
    0, 109493, 217113, 321018, 419430, 510666, 593164, 665512, 726474, 775006, 810277, 831683, 838860, 831683, 810277, 775006, 726474, 665512, 593164, 510666, 419430, 321018, 217113, 109493, 0, -109493, -217113, -321018, -419430, -510666, -593164, -665512, -726474, -775006, -810277, -831683, -838860, -831683, -810277, -775006, -726474, -665512, -593164, -510666, -419430, -321018, -217113, -109493
};

#if (I2S_TX_BUFFER_LENGTH % (4 * 48)) != 0
#error "DAC test buffer must contain complete sine periods"
#endif

void fillDacTestBuffer(void)
{
    for (uint32_t frameIndex = 0;
         frameIndex < I2S_TX_BUFFER_LENGTH / 4;
         frameIndex++)
    {
        // Convert through a signed integer before packing negative samples.
        int32_t testSample = (int32_t)(
            (float)dacTestTone[frameIndex % 48] * DAC_TEST_GAIN
        );
        uint32_t pcmSample = ((uint32_t)testSample) << 8;
        uint32_t wordIndex = 4 * frameIndex;
        uint16_t pcmSampleHigh = (uint16_t)(pcmSample >> 16);
        uint16_t pcmSampleLow = (uint16_t)pcmSample;

        i2sTxBuffer[wordIndex] = pcmSampleHigh;
        i2sTxBuffer[wordIndex + 1] = pcmSampleLow;
        i2sTxBuffer[wordIndex + 2] = pcmSampleHigh;
        i2sTxBuffer[wordIndex + 3] = pcmSampleLow;
    }
}

void copyPassthroughBlock(
    const volatile uint16_t *input,
    volatile uint16_t *output,
    uint32_t wordCount,
    uint32_t micChannel,
    float gain
)
{
    uint32_t channelOffset = 2U * micChannel;

    for (uint32_t wordIndex = 0; wordIndex < wordCount; wordIndex += 4U)
    {
        uint32_t sampleIndex = wordIndex + channelOffset;

        // Discard the eight padding bits and sign-extend the 24-bit sample.
        uint32_t pcmBits = ((uint32_t)input[sampleIndex] << 8)
                         | ((uint32_t)input[sampleIndex + 1] >> 8);
        int32_t micSample = (int32_t)pcmBits;
        if (pcmBits & 0x800000U)
        {
            micSample -= 0x1000000;
        }

        float scaledSample = (float)micSample * gain;
        if (scaledSample > 8388607.0f)
        {
            scaledSample = 8388607.0f;
        }
        else if (scaledSample < -8388608.0f)
        {
            scaledSample = -8388608.0f;
        }

        int32_t outputSample = (int32_t)scaledSample;

        // Pack before duplicating; the unused mic is not mixed into the output.
        uint32_t pcmSample = ((uint32_t)outputSample) << 8;
        uint16_t pcmSampleHigh = (uint16_t)(pcmSample >> 16);
        uint16_t pcmSampleLow = (uint16_t)pcmSample;

        output[wordIndex] = pcmSampleHigh;
        output[wordIndex + 1] = pcmSampleLow;
        output[wordIndex + 2] = pcmSampleHigh;
        output[wordIndex + 3] = pcmSampleLow;
    }
}
