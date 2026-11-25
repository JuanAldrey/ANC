#ifndef FXLMS_H
#define FXLMS_H

#include <stdint.h>
#include "sampleRate.h"

// One DMA half contains this many samples after decimation (currently 8).
#define BLOCK_LENGTH SAMPLE_RATE_CONTROL_FRAMES

#ifndef W_LENGTH
#define W_LENGTH       576
#endif

#include "fxlmsModelConfig.h"
#include "fxlmsModelWindow.h"
// Stored source capacities stay in fxlmsModelConfig; active sizes end at END.
#define GHAT_LENGTH GHAT_END_TAP
#define FHAT_LENGTH FHAT_END_TAP
#if GHAT_FIRST_TAP < 0 || GHAT_FIRST_TAP >= GHAT_END_TAP || GHAT_END_TAP > FXLMS_MODEL_GHAT_LENGTH
#error "ERR slice must satisfy 0 <= FIRST < END <= exported ERR capacity"
#endif
#if FHAT_FIRST_TAP < 0 || FHAT_FIRST_TAP >= FHAT_END_TAP || FHAT_END_TAP > FXLMS_MODEL_FHAT_LENGTH
#error "REF slice must satisfy 0 <= FIRST < END <= exported REF capacity"
#endif

#if W_LENGTH < 2 || GHAT_LENGTH < 2 || FHAT_LENGTH < 2
#error "FxLMS filters require at least two taps"
#endif

#define MU        0.001f

typedef enum
{
    FXLMS_INIT_NOT_RUN = 0,
    // Matching model rate and initialized filters, not convergence validation.
    FXLMS_INIT_READY = 1,
    FXLMS_INIT_MODEL_RATE_ERROR = 2
} FxLMSInitStatus;

extern volatile FxLMSInitStatus fxlmsInitStatus;
FxLMSInitStatus initFxLMS(void);

// One complete DMA half: PCM -> decimation -> FxLMS -> interpolation -> PCM.
void processAudioBlock(
    const volatile uint16_t *input,
    volatile uint16_t *output
);

#endif
