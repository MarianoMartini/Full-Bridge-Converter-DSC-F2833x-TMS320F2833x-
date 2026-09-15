#if 1  // <-- Poner en '1' solo si se desea compilar este archivo en lugar de main.c

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

// --- SELECCIÓN CENTRALIZADA DEL MODO DE CONTROL ---
//   0b0 = Control Bipolar
//   0b1 = Control Unipolar
#define CONTROL_MODE 0b0

// Periodo del temporizador para 1 kHz: 37500 en decimal
// En binario (16 bits): 0b1001001001111100 (0x927C)
#define EPWM_TIMER_TBPRD 37500

// Tiempo muerto máximo: 1023 en decimal
// En binario (10 bits): 0b1111111111 (0x03FF)
#define MAX_DEADBAND 1023

// Ciclo de trabajo fijo en estado estacionario (50% = 18750 cuentas)
#define STEADY_DUTY (EPWM_TIMER_TBPRD / 2)

// Prototipos de funciones
void InitEPwm_Bipolar(void);
void InitEPwm_Unipolar(void);

/**
 * @brief Función principal
 */
void main(void)
{
    // 1. Inicialización de los relojes y hardware de control del sistema
    InitSysCtrl();

    // 2. Inicializar los pines GPIO para ePWM1 y ePWM2
    InitEPwm1Gpio();
    InitEPwm2Gpio();

    // 3. Limpiar interrupciones y establecer tabla de vectores PIE
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // 4. Detener el reloj base de los ePWM antes de configurarlos
    //    bit TBCLKSYNC = 0b0 (detener sincronización)
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    // --- Inicialización según CONTROL_MODE ---
    #if CONTROL_MODE == 0
        InitEPwm_Bipolar();
    #else
        InitEPwm_Unipolar();
    #endif

    // --- 5. Inicializar ePWM según CONTROL_MODE ---                               
        #if CONTROL_MODE == 0                                                           
            InitEPwm_Bipolar();                                                         
        #else                                                                           
            InitEPwm_Unipolar();                                                        
        #endif  

    // 6. Iniciar sincronizadamente los relojes de los ePWM
    //    bit TBCLKSYNC = 0b1 (iniciar sincronización)
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0b1;
    EDIS;

    // 7. Bucle infinito
    for(;;){
    }
}

/**
 * @brief Configuración ePWM para Convertidor DC-DC (Control Bipolar a 1kHz)
 * 
 * En esta función se han reemplazado todas las constantes por su valor BINARIO exacto:
 */
void InitEPwm_Bipolar(void)
{
    // ========================================================================
    // Configuración ePWM1 (Rama A - MASTER)
    // ========================================================================
    EPwm1Regs.TBPRD = EPWM_TIMER_TBPRD;           
    EPwm1Regs.TBPHS.half.TBPHS = 0x0000;          
    EPwm1Regs.TBCTR = 0x0000;                     

    // TBCTL.SYNCOSEL (bits 5-4): 
    //   0b01 = Sincronizar en CTR == 0 (antes: TB_CTR_ZERO)
    EPwm1Regs.TBCTL.bit.SYNCOSEL = 0b01;   

    // TBCTL.PHSEN (bit 2): 
    //   0b0 = Deshabilitar carga de fase por ser Master (antes: TB_DISABLE)
    EPwm1Regs.TBCTL.bit.PHSEN = 0b0;       

    // TBCTL.CTRMODE (bits 1-0): 
    //   0b10 = Contador Up-Down simétrico / triangular (antes: TB_COUNT_UPDOWN)
    EPwm1Regs.TBCTL.bit.CTRMODE = 0b10;

    // TBCTL.HSPCLKDIV (bits 9-7): 
    //   0b001 = Preescalador /2 (valor por defecto de reset) -> TBCLK = 150 MHz / 2 = 75 MHz
    // EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0b001;            

    // TBCTL.CLKDIV (bits 12-10): 
    //   0b000 = Preescalador /1 (valor por defecto de reset)
    // EPwm1Regs.TBCTL.bit.CLKDIV = 0b000;               

    EPwm1Regs.CMPA.half.CMPA = STEADY_DUTY; 

    // AQCTLA.CAU (bits 5-4): 
    //   0b10 = Salida en ALTO (SET) al comparar en subida (antes: AQ_SET)
    EPwm1Regs.AQCTLA.bit.CAU = 0b10;

    // AQCTLA.CAD (bits 7-6): 
    //   0b01 = Salida en BAJO (CLEAR) al comparar en bajada (antes: AQ_CLEAR)
    EPwm1Regs.AQCTLA.bit.CAD = 0b01;

    // DBCTL.OUT_MODE (bits 1-0): 
    //   0b11 = Habilitar retardos RED en EPWMxA y FED en EPWMxB (antes: DB_FULL_ENABLE)
    EPwm1Regs.DBCTL.bit.OUT_MODE = 0b11;

    // DBCTL.POLSEL (bits 3-2): 
    //   0b10 = Active High Complementary: EPWMxA directo, EPWMxB invertido (antes: DB_ACTV_HIC)
    EPwm1Regs.DBCTL.bit.POLSEL = 0b10;     

    // DBCTL.IN_MODE (bits 5-4): 
    //   0b00 = EPWMxA es la fuente de entrada para ambos retardos (antes: DBA_ALL)
    EPwm1Regs.DBCTL.bit.IN_MODE = 0b00;

    // DBRED / DBFED: 10 bits de resolución
    EPwm1Regs.DBRED = MAX_DEADBAND; 
    EPwm1Regs.DBFED = MAX_DEADBAND; 

    // ========================================================================
    // Configuración ePWM2 (Rama B - SLAVE)
    // ========================================================================
    EPwm2Regs.TBPRD = EPWM_TIMER_TBPRD;
    EPwm2Regs.TBPHS.half.TBPHS = 0x0000;
    EPwm2Regs.TBCTR = 0x0000;

    // TBCTL.PHSEN (bit 2): 
    //   0b1 = Habilitar carga de fase al recibir pulso de sincronismo (antes: TB_ENABLE)
    EPwm2Regs.TBCTL.bit.PHSEN = 0b1;        

    // TBCTL.SYNCOSEL (bits 5-4): 
    //   0b00 = Pasar señal de sincronización (EPWMxSYNCI -> EPWMxSYNCO) (antes: TB_SYNC_IN)
    EPwm2Regs.TBCTL.bit.SYNCOSEL = 0b00;    

    // TBCTL.CTRMODE (bits 1-0): 
    //   0b10 = Contador Up-Down simétrico (antes: TB_COUNT_UPDOWN)
    EPwm2Regs.TBCTL.bit.CTRMODE = 0b10;

    // TBCTL preescaladores idénticos a ePWM1
    // EPwm2Regs.TBCTL.bit.HSPCLKDIV = 0b001;  // Preescalador /2 (por defecto)
    // EPwm2Regs.TBCTL.bit.CLKDIV = 0b000;     // Preescalador /1 (por defecto)

    EPwm2Regs.CMPA.half.CMPA = STEADY_DUTY; 

    // Acciones OPUESTAS a ePWM1 para puente completo bipolar:
    // AQCTLA.CAU (bits 5-4): 
    //   0b01 = Salida en BAJO al comparar en subida (antes: AQ_CLEAR)
    EPwm2Regs.AQCTLA.bit.CAU = 0b01;

    // AQCTLA.CAD (bits 7-6): 
    //   0b10 = Salida en ALTO al comparar en bajada (antes: AQ_SET)
    EPwm2Regs.AQCTLA.bit.CAD = 0b10;            

    // DBCTL configurado igual que ePWM1
    EPwm2Regs.DBCTL.bit.OUT_MODE = 0b11;    // 0b11: RED y FED activos
    EPwm2Regs.DBCTL.bit.POLSEL = 0b10;      // 0b10: Active High Complementary
    EPwm2Regs.DBCTL.bit.IN_MODE = 0b00;     // 0b00: Fuente EPWMxA
    EPwm2Regs.DBRED = MAX_DEADBAND;         // 1023 cuentas
    EPwm2Regs.DBFED = MAX_DEADBAND;         // 1023 cuentas
}

/**
 * @brief Configuración ePWM para Convertidor DC-DC (Control Unipolar a 1kHz)
 */
void InitEPwm_Unipolar(void)
{
    Uint16 duty_cmp = EPWM_TIMER_TBPRD / 2; // Duty inicial 50%

    // ========================================================================
    // Configuración ePWM1 (Rama A)
    // ========================================================================
    EPwm1Regs.TBPRD = EPWM_TIMER_TBPRD;           
    EPwm1Regs.TBPHS.half.TBPHS = 0x0000;          
    EPwm1Regs.TBCTR = 0x0000;                     

    EPwm1Regs.TBCTL.bit.SYNCOSEL = 0b01;    // 0b01: Sincronizar en CTR == 0
    EPwm1Regs.TBCTL.bit.PHSEN = 0b0;        // 0b0: Deshabilitar carga de fase
    EPwm1Regs.TBCTL.bit.CTRMODE = 0b10;     // 0b10: Up-Down
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0b001;  // 0b001: Preescalador /2 (por defecto)
    EPwm1Regs.TBCTL.bit.CLKDIV = 0b000;     // 0b000: Preescalador /1 (por defecto)

    EPwm1Regs.CMPA.half.CMPA = STEADY_DUTY; 

    EPwm1Regs.AQCTLA.bit.CAU = 0b10;        // 0b10: Salida en ALTO en subida
    EPwm1Regs.AQCTLA.bit.CAD = 0b01;        // 0b01: Salida en BAJO en bajada

    EPwm1Regs.DBCTL.bit.OUT_MODE = 0b11;    // 0b11: RED y FED activos
    EPwm1Regs.DBCTL.bit.POLSEL = 0b10;      // 0b10: Active High Complementary
    EPwm1Regs.DBCTL.bit.IN_MODE = 0b00;     // 0b00: Fuente EPWMxA
    EPwm1Regs.DBRED = MAX_DEADBAND;         // 1023 cuentas
    EPwm1Regs.DBFED = MAX_DEADBAND;         // 1023 cuentas

    // ========================================================================
    // Configuración ePWM2 (Rama B)
    // ========================================================================
    EPwm2Regs.TBPRD = EPWM_TIMER_TBPRD;
    EPwm2Regs.TBPHS.half.TBPHS = 0x0000;
    EPwm2Regs.TBCTR = 0x0000;

    EPwm2Regs.TBCTL.bit.PHSEN = 0b1;        // 0b1: Habilitar fase
    EPwm2Regs.TBCTL.bit.SYNCOSEL = 0b00;    // 0b00: Pasar sync
    EPwm2Regs.TBCTL.bit.CTRMODE = 0b10;     // 0b10: Up-Down
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = 0b001;  // 0b001: Preescalador /2 (por defecto)
    EPwm2Regs.TBCTL.bit.CLKDIV = 0b000;     // 0b000: Preescalador /1 (por defecto)

    // En Unipolar, la rama B lleva el ciclo de trabajo complementario
    EPwm2Regs.CMPA.half.CMPA = EPWM_TIMER_TBPRD - STEADY_DUTY; 

    // En Unipolar, las acciones del Action-Qualifier son IDÉNTICAS a Rama A:
    EPwm2Regs.AQCTLA.bit.CAU = 0b10;        // 0b10: Salida en ALTO en subida
    EPwm2Regs.AQCTLA.bit.CAD = 0b01;        // 0b01: Salida en BAJO en bajada

    EPwm2Regs.DBCTL.bit.OUT_MODE = 0b11;    // 0b11: RED y FED activos
    EPwm2Regs.DBCTL.bit.POLSEL = 0b10;      // 0b10: Active High Complementary
    EPwm2Regs.DBCTL.bit.IN_MODE = 0b00;     // 0b00: Fuente EPWMxA
    EPwm2Regs.DBRED = MAX_DEADBAND;         // 1023 cuentas
    EPwm2Regs.DBFED = MAX_DEADBAND;         // 1023 cuentas
}

#endif
