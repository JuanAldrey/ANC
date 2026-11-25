# ANC

An embedded feedforward active noise control prototype running on a
NUCLEO-F446RE. This project is being developed as preliminary work for a
thesis and as a way to take a DSP algorithm from theory to hardware.
Acoustic cancellation on the physical setup has not yet been validated.

The project implements FxLMS in the time domain. G uses the CMSIS FIR
implementation. W and F use the controller's time-domain loops, and W is
adapted through direct block correlation. Audio transport uses I2S and DMA
at 48 kHz, while the controller processes audio at 8 kHz.

## Import and build

Verified environment: STM32CubeIDE 2.2.0, GNU Tools for STM32 14.3.rel1.

Clone this repository and open STM32CubeIDE with your own workspace.
Select **File -> Import -> General -> Existing Projects into Workspace**.
Choose the cloned project folder without copying it to another location.

The project is named `ANC` and includes Debug and Release configurations.
Both use `-O3`, Cortex-M4, the single-precision FPU, and the hard-float ABI.
No paths to the previous project or the USB package are required.

## Audio pipeline

In ANC mode, `main` handles the two DMA buffer halves and calls
`processAudioBlock`. This function decimates REF and ERR from 48 to 8 kHz,
runs the time-domain FxLMS controller, and interpolates the output from
8 to 48 kHz before writing PCM24 samples to the TX buffer.

- `Src/main.c`: startup and DMA buffer handling.
- `Src/fxlms.c`: time-domain FxLMS filtering and adaptation.
- `Src/sampleRate.c`: low-pass filtering, decimation, interpolation, and PCM24 conversion.
- `Src/clock.c`, `Src/i2s.c`, `Src/dma.c`: clock and audio transport configuration through peripheral registers.
- `Drivers/`: CMSIS, the two FIR source files, and C runtime support.

Select the operating mode in `Inc/appConfig.h`: `ANC`, `PASSTHROUGH`, or
`DAC_TONE`. Filter lengths and model windows are configured in
`Inc/fxlms.h` and `Inc/fxlmsModelWindow.h`.

The official CMSIS headers are kept intact. Only `arm_fir_f32.c` and
`arm_fir_init_f32.c` from the DSP module are compiled.

## Removed components

The USB stack, USB Audio, PCD, and their callbacks and configuration have
been removed. FFT initialization, configuration, source files, and tables
have also been removed.

HAL was removed as well. It was only being used for startup and a system
tick that the audio processing does not require. The CPU still runs at a
nominal 168 MHz, and audio timing is driven by I2S and DMA.

USB interrupt names and register definitions in the startup file and
official device headers are chip definitions, rather than an active USB stack.

## Measured impulse responses

The model tables come from the REF and ERR measurements in
`post-downsampling.mdat`, acquired on November 24, 2026. They were converted
to 8 kHz, including both sample-rate conversion filters used by the firmware.
The time origin and polarity were preserved.

The active windows are `G[0:64]` and `F[0:512]`, with `W_LENGTH = 576`.
These are direct cuts without an additional end fade. The source tables
retain 1024 coefficients for G and 4096 for F, allowing the active windows
to be changed from CubeIDE. The complete original measurements are archived
separately.

The external gain correction uses an electrical loopback measurement taken
with the same Focusrite gain settings as the acoustic measurements.
Timing equivalence between measurement and controller ports, adaptation
polarity, and acoustic cancellation still need to be checked.
`FXLMS_MODELS_VALIDATED_FOR_CONTROL` remains `0`. The sample-rate check is
retained and now matches the 8 kHz models.

The G64/F512/W576 configuration, tested with synthetic coefficients,
measured an isolated maximum of **0.888 ms per call**, including sample-rate
conversion. Timing with live audio, active DMA, and the measured coefficients
still needs to be verified. See `docs/rirs-8k-2026-11-24.json` for provenance
and benchmark conditions.

The original REW measurements, NPZ files, exporters, and benchmark programs
are kept separately and are not included in this repository. The coefficient
tables required to build the firmware are included. The provenance file
records measurement names, hashes, and acquisition conditions.

## Dependencies

CMSIS Core, Device, and the CMSIS-DSP FIR implementations are included under
`Drivers/`. Their original notices and licenses are preserved in that folder.
