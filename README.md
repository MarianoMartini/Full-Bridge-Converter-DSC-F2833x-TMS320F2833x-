# Convertidor Full-Bridge con TMS320F28335

Firmware para generar las señales PWM de un convertidor DC-DC de puente completo
(*full bridge*) usando el microcontrolador **Texas Instruments TMS320F28335** y
los módulos ePWM1/ePWM2.

> **Advertencia:** este proyecto controla electrónica de potencia. Verifique el
> aislamiento, la alimentación, las conexiones de tierra y el tiempo muerto
> antes de conectar una etapa de potencia. Realice las primeras pruebas con
> baja tensión y una carga limitada, usando la instrumentación y protección
> adecuadas.

## Características

- Frecuencia PWM nominal: **1 kHz**.
- Contador ePWM en modo **up-down** para obtener una modulación simétrica.
- Tiempo muerto configurado en **1023 cuentas** (`DBRED` y `DBFED`).
- Sincronización de ePWM2 con ePWM1.
- Selección de la rutina de modulación en tiempo de compilación mediante
  `CONTROL_MODE`.
- Configuración de pines para ePWM1 y ePWM2 mediante `InitEPwm1Gpio()` y
  `InitEPwm2Gpio()`.

## Hardware y herramientas

### Hardware

- Placa o sistema basado en **TMS320F28335**.
- Etapa de potencia de puente completo compatible con las salidas ePWM.
- Driver de compuertas con entradas complementarias y soporte para tiempo
  muerto.
- Osciloscopio y fuente de alimentación con limitación de corriente para la
  puesta en marcha.

### Software

El proyecto está configurado para:

- Code Composer Studio (CCS).
- Compilador TI C2000 **6.2.0**.
- Formato de salida **COFF**.
- Archivo de enlace: `28335_RAM_lnk.cmd`.
- Configuración de depuración: `targetConfigs/TMS320F28335.ccxml`.

## Estructura del repositorio

| Ruta | Descripción |
| --- | --- |
| `main.c` | Inicialización del dispositivo y configuración de ePWM1/ePWM2. |
| `main_bin.c` | Variante experimental con constantes expresadas en binario; no forma parte de la configuración de compilación. |
| `DSP2833x_common/` | Código fuente, cabeceras, librerías y archivos de enlace comunes de la familia F2833x. |
| `DSP2833x_headers/` | Definiciones de registros y cabeceras de periféricos. |
| `F28335.cmd` | Archivo de enlace alternativo presente en el repositorio. |
| `28335_RAM_lnk.cmd` | Archivo de enlace utilizado por las configuraciones Debug y Release de CCS. |
| `targetConfigs/` | Configuración del dispositivo y de la conexión de depuración. |

## Modos de control

La selección se realiza en `main.c`:

```c
#define CONTROL_MODE 1
```

El código actualmente ejecuta:

| Valor | Rutina invocada |
| ---: | --- |
| `1` | `InitEPwm_Bipolar()` |
| cualquier otro valor | `InitEPwm_Unipolar()` |

En el modo bipolar, ePWM1 y ePWM2 usan acciones de comparación opuestas. En el
modo unipolar, ambas ramas usan acciones iguales y ePWM2 utiliza el ciclo de
trabajo complementario. Cambie el valor, compile nuevamente y verifique las
formas de onda antes de conectar la carga.

> Los comentarios de selección de modo presentes en el código no coinciden
> completamente con la condición `#if` actual. La tabla anterior describe el
> comportamiento real de `main.c`; si se modifica esta lógica, actualice
> también sus comentarios y esta documentación.

## Parámetros principales

Con `SYSCLKOUT = 150 MHz`, `HSPCLKDIV = /2` y `CLKDIV = /1`, el código utiliza:

| Parámetro | Valor | Propósito |
| --- | ---: | --- |
| `EPWM_TIMER_TBPRD` | `37500` | Periodo para el contador up-down a 1 kHz. |
| `MAX_DEADBAND` | `1023` | Tiempo muerto máximo configurado. |
| `STEADY_DUTY` | `18750` | Ciclo de trabajo nominal del 50 %. |

Estos valores dependen del reloj real del sistema y de la etapa de potencia.
Recalcule el periodo si cambia la frecuencia de CPU o los divisores del reloj.

## Importar, compilar y cargar

1. Abra Code Composer Studio y seleccione un espacio de trabajo.
2. Importe el proyecto existente desde el directorio raíz del repositorio.
3. Confirme que el dispositivo sea `TMS320F28335` y que la configuración de
   conexión corresponda a la sonda de depuración disponible.
4. Seleccione la configuración `Debug` o `Release`.
5. Compile con **Project > Build Project**. La compilación utiliza
   `28335_RAM_lnk.cmd` y las rutas de `DSP2833x_common/include` y
   `DSP2833x_headers/include`.
6. Inicie una sesión de depuración con
   `targetConfigs/TMS320F28335.ccxml`, cargue el `.out` generado y ejecute el
   dispositivo.
7. Mida ePWM1A/ePWM1B y ePWM2A/ePWM2B antes de habilitar la etapa de potencia.

Los artefactos de compilación (`Debug/`, `Release/`, `*.out`, `*.map`, entre
otros) están excluidos por `.gitignore`.

## Notas de desarrollo

- `main.c` detiene `TBCLKSYNC` durante la configuración y lo habilita después
  de inicializar ambos módulos ePWM.
- El programa no implementa un lazo de control de tensión o corriente: después
  de la inicialización permanece en un bucle infinito.
- `main_bin.c` está excluido de la compilación por la configuración del
  proyecto; no lo compile junto con `main.c` sin revisar previamente sus
  diferencias.