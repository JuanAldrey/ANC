#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// Elegir un solo modo y recompilar.
#define APP_MODE_PASSTHROUGH 2
#define APP_MODE_DAC_TONE    3
#define APP_MODE_ANC         4

#ifndef APP_MODE
#define APP_MODE APP_MODE_PASSTHROUGH
#endif

#if APP_MODE != APP_MODE_ANC && \
    APP_MODE != APP_MODE_PASSTHROUGH && \
    APP_MODE != APP_MODE_DAC_TONE
#error "Elegir ANC, PASSTHROUGH o DAC_TONE en APP_MODE"
#endif

// Microfono y ganancia para PASSTHROUGH.
#define PASSTHROUGH_REFERENCE 0U
#define PASSTHROUGH_ERROR     1U
#define PASSTHROUGH_MIC PASSTHROUGH_ERROR
#define PASSTHROUGH_GAIN 1.0f

// La tabla de tono tiene un pico de 0.1 FS. Esta ganancia solo afecta DAC_TONE.
#define DAC_TEST_GAIN 0.1f

#if PASSTHROUGH_MIC != PASSTHROUGH_REFERENCE && PASSTHROUGH_MIC != PASSTHROUGH_ERROR
#error "Elegir PASSTHROUGH_REFERENCE o PASSTHROUGH_ERROR"
#endif

#endif
