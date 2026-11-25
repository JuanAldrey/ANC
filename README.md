# ANC

Prototipo de control activo de ruido feedforward en una NUCLEO-F446RE.
Proyecto en desarrollo, como preparacion para una tesis y para llevar un
algoritmo de DSP desde la teoria al hardware. La cancelacion acustica en
hardware todavia no esta validada.

Este proyecto ejecuta FxLMS en el dominio del tiempo. G usa el FIR de CMSIS,
W y F usan los bucles temporales del controlador, y la adaptacion de W calcula
la correlacion directamente por bloques. El transporte de audio usa I2S y DMA a 48 kHz, con procesamiento a 8 kHz.

## Abrir y compilar

Entorno verificado: STM32CubeIDE 2.2.0, GNU Tools for STM32 14.3.rel1.
Clonar este repositorio y abrir STM32CubeIDE con un workspace propio.
En CubeIDE: File -> Import -> General -> Existing Projects into Workspace.
Seleccionar esta carpeta, sin copiar el proyecto a otra ubicacion.
El proyecto se llama ANC y tiene configuraciones Debug y Release, ambas
con -O3, Cortex-M4, FPU de precision simple y ABI hard-float.
No necesita rutas al proyecto anterior ni al paquete USB.

## Recorrido del audio

`main` atiende las dos mitades DMA y llama a `processAudioBlock`.
Esa funcion decima REF/ERR de 48 a 8 kHz, ejecuta el FxLMS temporal e
interpola la salida de 8 a 48 kHz antes de escribir PCM24 en el buffer TX.

- `Src/main.c`: arranque y atencion de DMA.
- `Src/fxlms.c`: filtrado y adaptacion FxLMS en tiempo.
- `Src/sampleRate.c`: pasabajos, decimacion, interpolacion y PCM24.
- `Src/clock.c`, `Src/i2s.c`, `Src/dma.c`: relojes y transporte por registros.
- `Drivers/`: CMSIS, las dos fuentes FIR y soporte estandar de C.

Los modos se eligen en `Inc/appConfig.h`: ANC, PASSTHROUGH o DAC_TONE.
Las longitudes y ventanas siguen en `Inc/fxlms.h` y `Inc/fxlmsModelWindow.h`.
Los headers oficiales de CMSIS se conservan completos. Solo se compilan
`arm_fir_f32.c` y `arm_fir_init_f32.c` del modulo DSP.

## Lo que se retiro

Se retiraron el stack USB, USB Audio, PCD, sus callbacks y sus configuraciones.
Tambien se retiraron la inicializacion, configuracion, fuentes y tablas de FFT.
Tambien se retiro HAL: solo quedaba usado para el arranque y un tick que
el procesamiento de audio no necesita. La CPU conserva 168 MHz nominales,
y la temporizacion del audio sigue dependiendo de I2S/DMA.

Los nombres de interrupciones y registros USB que aparecen en el startup o
en los headers oficiales del micro son definiciones del chip, no un stack USB.

## Estado de las RIR

Las bases son las mediciones REF/ERR de `post-downsampling.mdat` del
24 de noviembre, convertidas a 8 kHz e incluyendo los dos filtros de
conversion del firmware. Se conservaron el origen temporal y la polaridad.
Las ventanas activas son G[0:64], F[0:512] y W_LENGTH=576, con corte directo.
Las tablas fuente conservan 1024 coeficientes de G y 4096 de F para permitir
cambiar la ventana desde CubeIDE. Los originales completos siguen archivados.

La escala externa usa el loopback electrico confirmado, conservando las ganancias de Focusrite.
La equivalencia temporal entre puertos, la polaridad de adaptacion y la
cancelacion acustica todavia deben comprobarse. VALIDATED_FOR_CONTROL sigue
en 0. El chequeo de Fs permanece y ahora coincide con los modelos a 8 kHz.
El modo de appConfig y el algoritmo FxLMS se conservaron.

La configuracion sintetica G64/F512/W576 midio un maximo aislado de 0,888 ms
por llamada, incluyendo la conversion. Falta medir con audio/DMA activos y
estos coeficientes reales. Ver `docs/rirs-8k-2026-11-24.json` para la procedencia.

Las mediciones REW originales, los NPZ, los exportadores y los programas
usados para el benchmark se conservan por separado y no estan incluidos
en este repositorio. Las tablas de coeficientes necesarias para compilar
si estan incluidas. El archivo de procedencia registra nombres, hashes y
condiciones de las mediciones.

## Dependencias

CMSIS Core, Device y los FIR de CMSIS-DSP estan incluidos en Drivers.
Sus avisos y licencias originales se conservan en esa carpeta.
