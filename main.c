#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

// --- SELECCIÓN CENTRALIZADA DEL MODO DE CONTROL ---
// Cambiar este valor para alternar entre modos:
//   0 = Control Bipolar
//   1 = Control Unipolar
#define CONTROL_MODE 0

// Constantes para la configuración del periodo (1 kHz)
// Asumiendo SYSCLKOUT = 150 MHz
// CLKDIV = 1 (/2), HSPCLKDIV = 2 (/4) -> TBCLK = 150MHz / 8 = 18.75 MHz
// Para contador Up-Down a 1 kHz: TBPRD = 18.75 MHz / (2 * 1kHz) = 9375
#define EPWM_TIMER_TBPRD 9375

// Tiempo muerto máximo (10 bits), como lo requiere la consigna 2
#define MAX_DEADBAND 1023

// Prototipos de funciones
void InitEPwm_Bipolar(void);
void InitEPwm_Unipolar(void);

/**
 * @brief Función principal
 */
void main(void)
{
    // Variables para el bucle infinito
    Uint16 duty_direction = 1; // 1 = subiendo, 0 = bajando
    Uint16 duty_value = EPWM_TIMER_TBPRD / 2; // Inicia en 50%
    Uint16 step = 10; // Paso de incremento/decremento
    
    // Límites del duty cycle (10% al 90%) - aritmética entera
    Uint16 max_duty = (Uint16)((EPWM_TIMER_TBPRD * 9UL) / 10);  // = 8437
    Uint16 min_duty = (Uint16)(EPWM_TIMER_TBPRD / 10);           // = 937

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
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    // --- Inicialización según CONTROL_MODE ---
    #if CONTROL_MODE == 0
        InitEPwm_Bipolar();
    #else
        InitEPwm_Unipolar();
    #endif

    // 5. Iniciar sincronizadamente los relojes de los ePWM
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    // 6. Bucle Infinito: Variación continua del Ciclo de Trabajo para pruebas
    for(;;)
    {
        // Alternar dirección al llegar a los límites
        if(duty_direction == 1) {
            duty_value += step;
            if(duty_value >= max_duty) {
                duty_direction = 0;
            }
        } else {
            duty_value -= step;
            if(duty_value <= min_duty) {
                duty_direction = 1;
            }
        }

        // Actualizar los registros CMPA (Duty Cycle) según CONTROL_MODE
        #if CONTROL_MODE == 0
            // Bipolar: ambas ramas llevan el mismo CMPA
            EPwm1Regs.CMPA.half.CMPA = duty_value;
            EPwm2Regs.CMPA.half.CMPA = duty_value;
        #else
            // Unipolar: Rama B lleva el complemento
            EPwm1Regs.CMPA.half.CMPA = duty_value;
            EPwm2Regs.CMPA.half.CMPA = EPWM_TIMER_TBPRD - duty_value;
        #endif

        // Retardo simulado para que se pueda ver en el osciloscopio la modulación 
        DELAY_US(1000); 
    }
}

/**
 * @brief Configuración ePWM para Convertidor DC-DC (Control Bipolar a 1kHz)
 */
void InitEPwm_Bipolar(void)
{
    // Configuración ePWM1 (Rama A)
    EPwm1Regs.TBPRD = EPWM_TIMER_TBPRD;           
    EPwm1Regs.TBPHS.half.TBPHS = 0x0000;          
    EPwm1Regs.TBCTR = 0x0000;                     

    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;   
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;       

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 2;            
    EPwm1Regs.TBCTL.bit.CLKDIV = 1;               

    EPwm1Regs.CMPA.half.CMPA = EPWM_TIMER_TBPRD / 2; 

    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;

    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;     
    EPwm1Regs.DBCTL.bit.IN_MODE = DBA_ALL;
    EPwm1Regs.DBRED = MAX_DEADBAND; 
    EPwm1Regs.DBFED = MAX_DEADBAND; 

    // Configuración ePWM2 (Rama B)
    EPwm2Regs.TBPRD = EPWM_TIMER_TBPRD;
    EPwm2Regs.TBPHS.half.TBPHS = 0x0000;
    EPwm2Regs.TBCTR = 0x0000;

    EPwm2Regs.TBCTL.bit.PHSEN = TB_ENABLE;        
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;    

    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = 2;
    EPwm2Regs.TBCTL.bit.CLKDIV = 1;

    EPwm2Regs.CMPA.half.CMPA = EPWM_TIMER_TBPRD / 2; 

    // Acciones OPUESTAS a ePWM1
    EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR;          
    EPwm2Regs.AQCTLA.bit.CAD = AQ_SET;            

    EPwm2Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm2Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm2Regs.DBCTL.bit.IN_MODE = DBA_ALL;
    EPwm2Regs.DBRED = MAX_DEADBAND;
    EPwm2Regs.DBFED = MAX_DEADBAND;
}

/**
 * @brief Configuración ePWM para Convertidor DC-DC (Control Unipolar a 1kHz)
 */
void InitEPwm_Unipolar(void)
{
    Uint16 duty_cmp = EPWM_TIMER_TBPRD / 2; // Duty inicial 50%

    // Configuración ePWM1 (Rama A)
    EPwm1Regs.TBPRD = EPWM_TIMER_TBPRD;           
    EPwm1Regs.TBPHS.half.TBPHS = 0x0000;          
    EPwm1Regs.TBCTR = 0x0000;                     

    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;   
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;       

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 2;            
    EPwm1Regs.TBCTL.bit.CLKDIV = 1;               

    EPwm1Regs.CMPA.half.CMPA = duty_cmp; 

    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;

    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;     
    EPwm1Regs.DBCTL.bit.IN_MODE = DBA_ALL;
    EPwm1Regs.DBRED = MAX_DEADBAND; 
    EPwm1Regs.DBFED = MAX_DEADBAND; 


    // Configuración ePWM2 (Rama B)
    EPwm2Regs.TBPRD = EPWM_TIMER_TBPRD;
    EPwm2Regs.TBPHS.half.TBPHS = 0x0000;
    EPwm2Regs.TBCTR = 0x0000;

    EPwm2Regs.TBCTL.bit.PHSEN = TB_ENABLE;        
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;    

    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = 2;
    EPwm2Regs.TBCTL.bit.CLKDIV = 1;

    // Duty de la rama B es el complementario
    EPwm2Regs.CMPA.half.CMPA = EPWM_TIMER_TBPRD - duty_cmp; 

    // Acciones IDÉNTICAS a la Rama A
    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;

    EPwm2Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm2Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm2Regs.DBCTL.bit.IN_MODE = DBA_ALL;
    EPwm2Regs.DBRED = MAX_DEADBAND;
    EPwm2Regs.DBFED = MAX_DEADBAND;
}
