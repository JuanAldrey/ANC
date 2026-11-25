#include <stdint.h>

#include "main.h"
#include "clock.h"
#include "hardwareTest.h"
#include "i2s.h"
#include "dma.h"
#include "fxlms.h"

// Inspect these counters in the debugger while checking DMA timing.
volatile uint32_t firstUnsafeCount = 0;
volatile uint32_t secondUnsafeCount = 0;
volatile uint32_t firstTxNdtr = 0;
volatile uint32_t secondTxNdtr = 0;
volatile uint32_t txUnderrunSeen = 0;

int main(void)
{
    // Enable FPU.
    SCB->CPACR |= (0xFU << 20);
    __DSB();
    __ISB();

    // Audio scheduling is driven by DMA; no system tick is needed.
    configureSystemClock();

    // Load control models before starting the audio hardware.
    if (APP_MODE == APP_MODE_ANC && initFxLMS() != FXLMS_INIT_READY)
    {
        Error_Handler();
    }

    // Configure audio input, then output.
    configureI2S2();
    configureI2S2DMA();
    configureI2S3();

    if (APP_MODE == APP_MODE_DAC_TONE)
    {
        // Fill before enabling TX DMA. Circular playback needs no refills.
        fillDacTestBuffer();
    }
    configureI2S3DMA();

    // Start slave TX before master RX starts the shared clocks.
    SPI3->CR2 |= (1U << 1);       // TXDMAEN.
    SPI3->I2SCFGR |= (1U << 10);
    SPI2->CR2 |= (1U << 0);       // RXDMAEN.
    SPI2->I2SCFGR |= (1U << 10);

    while (1)
    {
        if (APP_MODE != APP_MODE_DAC_TONE)
        {
            if (i2sRxFirstHalfReady && i2sTxFirstHalfReady)
            {
                i2sRxFirstHalfReady = 0;
                i2sTxFirstHalfReady = 0;

                firstTxNdtr = DMA1_Stream5->NDTR;
                uint32_t firstRxNdtr = DMA1_Stream3->NDTR;
                if (firstTxNdtr == 0 || firstTxNdtr > I2S_TX_BUFFER_LENGTH / 2 ||
                    firstRxNdtr == 0 || firstRxNdtr > I2S_RX_BUFFER_LENGTH / 2)
                {
                    firstUnsafeCount++;
                }
                else
                {
                    // Process the completed RX half into the free TX half.
                    if (APP_MODE == APP_MODE_ANC)
                    {
                        processAudioBlock(i2sRxBuffer, i2sTxBuffer);
                    }
                    else
                    {
                        copyPassthroughBlock(i2sRxBuffer, i2sTxBuffer,
                            I2S_RX_BUFFER_LENGTH / 2, PASSTHROUGH_MIC, PASSTHROUGH_GAIN);
                    }
                }
            }

            if (i2sRxSecondHalfReady && i2sTxSecondHalfReady)
            {
                i2sRxSecondHalfReady = 0;
                i2sTxSecondHalfReady = 0;

                secondTxNdtr = DMA1_Stream5->NDTR;
                uint32_t secondRxNdtr = DMA1_Stream3->NDTR;
                if (secondTxNdtr <= I2S_TX_BUFFER_LENGTH / 2 ||
                    secondRxNdtr <= I2S_RX_BUFFER_LENGTH / 2)
                {
                    secondUnsafeCount++;
                }
                else
                {
                    if (APP_MODE == APP_MODE_ANC)
                    {
                        processAudioBlock(
                            &i2sRxBuffer[I2S_RX_BUFFER_LENGTH / 2],
                            &i2sTxBuffer[I2S_TX_BUFFER_LENGTH / 2]
                        );
                    }
                    else
                    {
                        copyPassthroughBlock(
                            &i2sRxBuffer[I2S_RX_BUFFER_LENGTH / 2],
                            &i2sTxBuffer[I2S_TX_BUFFER_LENGTH / 2],
                            I2S_RX_BUFFER_LENGTH / 2, PASSTHROUGH_MIC, PASSTHROUGH_GAIN
                        );
                    }
                }
            }
        }
        if (SPI3->SR & (1U << 3))
        {
            // I2S3 transmitter underrun.
            txUnderrunSeen = 1;
        }
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
