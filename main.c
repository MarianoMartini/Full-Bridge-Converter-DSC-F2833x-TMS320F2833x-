#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

// --- SELECCIÓN CENTRALIZADA DEL MODO DE CONTROL ---
// Cambiar este valor para alternar entre modos:
//   0 = Control Bipolar
//   1 = Control Unipolar
#define CONTROL_MODE 1

// Constantes para la configuración del periodo (1 kHz)
// Asumiendo SYSCLKOUT = 150 MHz
// CLKDIV = 0 (/1), HSPCLKDIV = 1 (/2) -> TBCLK = 150MHz / 2 = 75 MHz
// Para contador Up-Down a 1 kHz: TBPRD = 75 MHz / (2 * 1kHz) = 37500
#define EPWM_TIMER_TBPRD 37500

// Tiempo muerto máximo (10 bits), como lo requiere la consigna 2
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
        EALLOW;                                                                         
        SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;                                          
        EDIS;                                                                           
                                                                                        
        // --- 5. Inicializar ePWM según CONTROL_MODE ---                               
        #if CONTROL_MODE == 1                                                           
            InitEPwm_Bipolar();                                                         
        #else                                                                           
            InitEPwm_Unipolar();                                                        
        #endif                                                                          
                                                                                        
        // 6. Iniciar sincronizadamente los relojes de los ePWM                         
        EALLOW;                                                                         
        SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;                                          
        EDIS;                                                                           

        for(;;){                                                                                                                                             
        }                                                                                   
     
    }               

/**
 * @brief Configuración ePWM para Convertidor DC-DC (Control Bipolar a 1kHz)
 */
void InitEPwm_Bipolar()
{
    // Configuración ePWM1 (Rama A)
    EPwm1Regs.TBPRD = EPWM_TIMER_TBPRD;           
    EPwm1Regs.TBPHS.half.TBPHS = 0x0000;          
    EPwm1Regs.TBCTR = 0x0000;                     

    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;   
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;       

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    //EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;            // /2 (valor por defecto)
    //EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;               // /1 (valor por defecto)

    EPwm1Regs.CMPA.half.CMPA = STEADY_DUTY; 

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
    //EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;            // /2 (valor por defecto)
    //EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;               // /1 (valor por defecto)

    EPwm2Regs.CMPA.half.CMPA = STEADY_DUTY; 

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
void InitEPwm_Unipolar()
{
    // Configuración ePWM1 (Rama A)
    EPwm1Regs.TBPRD = EPWM_TIMER_TBPRD;           
    EPwm1Regs.TBPHS.half.TBPHS = 0x0000;          
    EPwm1Regs.TBCTR = 0x0000;                     

    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;   
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;       

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    //EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;            // /2 (valor por defecto)
    //EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;               // /1 (valor por defecto)

    EPwm1Regs.CMPA.half.CMPA = STEADY_DUTY; 

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
    //EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV2;            // /2 (valor por defecto)
    //EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;               // /1 (valor por defecto)

    // Duty de la rama B es el complementario
    EPwm2Regs.CMPA.half.CMPA = EPWM_TIMER_TBPRD - STEADY_DUTY; 

    // Acciones IDÉNTICAS a la Rama A
    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;

    EPwm2Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm2Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm2Regs.DBCTL.bit.IN_MODE = DBA_ALL;
    EPwm2Regs.DBRED = MAX_DEADBAND;
    EPwm2Regs.DBFED = MAX_DEADBAND;
}
