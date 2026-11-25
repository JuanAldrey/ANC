#include "fxlms.h"
#include "dsp/filtering_functions.h"
#include <string.h>

#include "fxlmsMeasuredModels.h"

// Filters
static float w[W_LENGTH] = {0};
static float gHat[GHAT_LENGTH] = {0};
static float fHat[FHAT_LENGTH] = {0};

volatile FxLMSInitStatus fxlmsInitStatus = FXLMS_INIT_NOT_RUN;

// Filter states
static float wBuffer[W_LENGTH + BLOCK_LENGTH - 1] = {0};
static float gHatBuffer[GHAT_LENGTH + BLOCK_LENGTH - 1] = {0};
static float fHatBuffer[FHAT_LENGTH + BLOCK_LENGTH - 1] = {0};

// Indices for circular buffers
static uint32_t wBufferWriteIndex = 0;
static uint32_t fHatBufferWriteIndex = 0;

// Audio blocks
static float xBlock[BLOCK_LENGTH] = {0};
static float xCompBlock[BLOCK_LENGTH] = {0};
static float eBlock[BLOCK_LENGTH] = {0};
static float uBlock[BLOCK_LENGTH] = {0};
static float fxBlock[BLOCK_LENGTH] = {0};
static float fbEstBlock[BLOCK_LENGTH] = {0};

// Filtered-X history
static float fxHistory[W_LENGTH - 1] = {0};
static float fxFullBlock[W_LENGTH - 1 + BLOCK_LENGTH] = {0};

// FIR G instance
static arm_fir_instance_f32 gHatFir;

// Reversed copy of G for ARM convolution
static float gFirCoefficients[GHAT_LENGTH];

static void adaptW(
    float mu
);

FxLMSInitStatus initFxLMS(void)
{
    fxlmsInitStatus = FXLMS_INIT_NOT_RUN;

    // The stored 48 kHz models cannot describe the new 8 kHz controller.
    if (FXLMS_MODEL_SAMPLE_RATE_HZ != SAMPLE_RATE_CONTROL_HZ)
    {
        fxlmsInitStatus = FXLMS_INIT_MODEL_RATE_ERROR;
        return fxlmsInitStatus;
    }

    initSampleRateConversion();

    // Keep source sample indices: FIRST masks the prefix, never shifts arrival.
    // The constant source tables remain complete; only working arrays shrink.
    memset(gHat, 0, sizeof(gHat));
    memset(fHat, 0, sizeof(fHat));
    memcpy(&gHat[GHAT_FIRST_TAP], &fxlmsMeasuredGHat[GHAT_FIRST_TAP],
        (GHAT_END_TAP - GHAT_FIRST_TAP) * sizeof(float));
    memcpy(&fHat[FHAT_FIRST_TAP], &fxlmsMeasuredFHat[FHAT_FIRST_TAP],
        (FHAT_END_TAP - FHAT_FIRST_TAP) * sizeof(float));

    // Prepare gHat time domain convolution
    for (uint32_t i = 0; i < GHAT_LENGTH; i++)
    {
        gFirCoefficients[i] = gHat[GHAT_LENGTH - 1 - i];
    }

    arm_fir_init_f32(
        &gHatFir,
        GHAT_LENGTH,
        gFirCoefficients,
        gHatBuffer,
        BLOCK_LENGTH
    );

    // FxLMS ready
    fxlmsInitStatus = FXLMS_INIT_READY;
    return fxlmsInitStatus;
}

void processAudioBlock(
    const volatile uint16_t *input,
    volatile uint16_t *output
)
{
    // PCM24 at 48 kHz -> filtered float REF/ERR at 8 kHz.
    decimateMicrophones(input, xBlock, eBlock);

	// Compensate feedback path and apply wFilter
	// This has to be done sample by sample due to the feedback loop
	for (uint32_t n = 0; n < BLOCK_LENGTH; n++)
	{
		// Calculate sample that compensates the feedback path
		float fbEstSample = 0.0f;

		for (uint32_t tap = 1; tap < FHAT_LENGTH; tap++)
		{
		    uint32_t bufferIndex;

		    if (fHatBufferWriteIndex >= tap)
		    {
		        bufferIndex = fHatBufferWriteIndex - tap;
		    }
		    else
		    {
		        bufferIndex = fHatBufferWriteIndex + (FHAT_LENGTH - 1) - tap;
		    }

		    fbEstSample += fHat[tap] * fHatBuffer[bufferIndex];
		}

		// Compensate the reference sample with the artificial sample
		float xCompSample = xBlock[n] - fbEstSample;

		// Calculate actuator sample
		float uSample = w[0] * xCompSample;
		for (uint32_t tap  = 1; tap  < W_LENGTH; tap ++)
		{
			uint32_t bufferIndex;
			if (wBufferWriteIndex >= tap)
			{
				bufferIndex = wBufferWriteIndex - tap;
			}
			else
			{
				bufferIndex = wBufferWriteIndex + (W_LENGTH - 1) - tap;
			}
			uSample += w[tap] * wBuffer[bufferIndex];

		}

		// Keep the returned output and feedback history at the same limited value.
		float uLimited = uSample;

		if (uLimited > 0.99999988f)
		{
		    uLimited = 0.99999988f;
		}
		else if (uLimited < -1.0f)
		{
		    uLimited = -1.0f;
		}

		// Save samples into signal blocks
		fbEstBlock[n] = fbEstSample;
		xCompBlock[n] = xCompSample;
		uBlock[n] = uLimited;

		// Update fHat circular buffer with actuator sample
		fHatBuffer[fHatBufferWriteIndex] = uLimited;
		fHatBufferWriteIndex++;
		if (fHatBufferWriteIndex == FHAT_LENGTH - 1)
		{
		    fHatBufferWriteIndex = 0;
		}

		// Update W circular buffer with compensated reference sample
		wBuffer[wBufferWriteIndex] = xCompSample;
		wBufferWriteIndex++;
		if (wBufferWriteIndex == W_LENGTH - 1)
		{
		    wBufferWriteIndex = 0;
		}
	}

	// Calculate the filtered reference
	arm_fir_f32(
	    &gHatFir,
	    xCompBlock,
	    fxBlock,
	    BLOCK_LENGTH
	);

	// Adapt w
	adaptW(MU);

    // Controller output at 8 kHz -> reconstructed PCM24 at 48 kHz.
    interpolateOutput(uBlock, output, 1.0f);
}

static void adaptW(
    float mu
)
{
	// Concatenate fxHistory with new fxBlock
	memcpy(fxFullBlock, fxHistory,  ((W_LENGTH - 1))* sizeof(float));
	memcpy(&fxFullBlock[W_LENGTH - 1], fxBlock, (BLOCK_LENGTH) * sizeof(float));

	// Update wFilter
	for(uint32_t k = 0; k < (W_LENGTH); k++)
	{
		float sum = 0.0f;

		for(uint32_t n = 0; n < BLOCK_LENGTH; n++)
		{
			sum += eBlock[n] * fxFullBlock[W_LENGTH - 1 + n - k];
		}

		w[k] += (mu / (float)BLOCK_LENGTH) * sum;
	}

	// Update history
	memcpy(fxHistory, &fxFullBlock[BLOCK_LENGTH], (W_LENGTH - 1) * sizeof(float));

}
