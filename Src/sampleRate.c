#include <string.h>

#include "sampleRate.h"

#define FILTER_SECTIONS 4U

typedef struct
{
    float b0, b1, b2;
    float a1, a2;
} BiquadCoefficients;

typedef struct
{
    float d1, d2;
} BiquadState;

// Chebyshev II, order 8, Fs=48000, passband 0->3000 Hz (loss <=1 dB),
// stopband 4000->24000 Hz (attenuation >=40 dB).
// SciPy: cheb2ord(3000, 4000, 1, 40, fs=48000) -> (8, 3843.4290818740164).
// Then cheby2(8, 40, Wn, fs=48000, output="sos"). Each a0 is one.
// Feedback coefficients below use the standard SUBTRACT convention.
static const BiquadCoefficients coefficients[FILTER_SECTIONS] = {
    {0.0112136733f, 0.00602902053f, 0.0112136733f, -0.987342f, 0.25982675f},
    {1.0f, -1.29494679f, 1.0f, -1.26397896f, 0.486823916f},
    {1.0f, -1.65118694f, 1.0f, -1.54289567f, 0.722949624f},
    {1.0f, -1.74300539f, 1.0f, -1.74661446f, 0.911116064f}
};

// REF, ERR and output share coefficients, but never share their histories.
static BiquadState referenceState[FILTER_SECTIONS];
static BiquadState errorState[FILTER_SECTIONS];
static BiquadState outputState[FILTER_SECTIONS];

static float filterSample(float input, BiquadState *state)
{
    // Cascade of four transposed direct-form II sections.
    for (uint32_t section = 0; section < FILTER_SECTIONS; section++)
    {
        const BiquadCoefficients *c = &coefficients[section];
        BiquadState *s = &state[section];
        float output = c->b0 * input + s->d1;
        s->d1 = c->b1 * input - c->a1 * output + s->d2;
        s->d2 = c->b2 * input - c->a2 * output;
        input = output;
    }
    return input;
}

static float readPcm24(const volatile uint16_t *input)
{
    // One channel occupies two halfwords, with eight padding bits at the end.
    uint32_t bits = ((uint32_t)input[0] << 8) | ((uint32_t)input[1] >> 8);
    int32_t sample = (int32_t)bits;
    if (bits & 0x800000U)
    {
        sample -= 0x1000000;
    }
    return (float)sample * (1.0f / 8388608.0f);
}

void initSampleRateConversion(void)
{
    memset(referenceState, 0, sizeof(referenceState));
    memset(errorState, 0, sizeof(errorState));
    memset(outputState, 0, sizeof(outputState));
}

void decimateMicrophones(
    const volatile uint16_t *input,
    float *reference,
    float *error
)
{
    uint32_t slowIndex = 0;
    for (uint32_t frame = 0; frame < SAMPLE_RATE_IO_FRAMES; frame++)
    {
        uint32_t wordIndex = 4U * frame;
        float ref = filterSample(readPcm24(&input[wordIndex]), referenceState);
        float err = filterSample(readPcm24(&input[wordIndex + 2U]), errorState);

        // Filter EVERY input sample before discarding five out of six.
        // DMA halves contain a multiple of six frames, so phase is continuous.
        if ((frame % SAMPLE_RATE_FACTOR) == 0U)
        {
            reference[slowIndex] = ref;
            error[slowIndex] = err;
            slowIndex++;
        }
    }
}

void interpolateOutput(const float *control, volatile uint16_t *output, float gain)
{
    for (uint32_t frame = 0; frame < SAMPLE_RATE_IO_FRAMES; frame++)
    {
        float inserted = 0.0f;
        if ((frame % SAMPLE_RATE_FACTOR) == 0U)
        {
            // Compensate the gain lost by inserting five zeros per sample.
            inserted = control[frame / SAMPLE_RATE_FACTOR] * SAMPLE_RATE_FACTOR;
        }
        float sample = filterSample(inserted, outputState) * gain;

        if (sample > 0.9999998807907104f)
        {
            sample = 0.9999998807907104f;
        }
        else if (sample < -1.0f)
        {
            sample = -1.0f;
        }

        int32_t pcm = (int32_t)(sample * 8388608.0f);
        uint32_t packed = ((uint32_t)pcm) << 8;
        uint32_t wordIndex = 4U * frame;
        output[wordIndex] = (uint16_t)(packed >> 16);
        output[wordIndex + 1U] = (uint16_t)packed;
        output[wordIndex + 2U] = (uint16_t)(packed >> 16);
        output[wordIndex + 3U] = (uint16_t)packed;
    }
}

