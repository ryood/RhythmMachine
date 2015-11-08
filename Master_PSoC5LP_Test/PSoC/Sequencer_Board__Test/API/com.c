//=========================================================
// PSoC5LP Project
//=========================================================
// File Name : com.c
// Function  : COM Port Control Routime
//---------------------------------------------------------
// Rev.01 2013.02.04 Munetomo Maruyama
//---------------------------------------------------------
// Copyright (C) 2012-2013 Munetomo Maruyama
//=========================================================
// ---- License Information -------------------------------
// Anyone can FREELY use this code fully or partially
// under conditions shown below.
// 1. You may use this code only for individual purpose,
//    and educational purpose.
//    Do not use this code for business even if partially.
// 2. You can copy, modify and distribute this code.
// 3. You should remain this header text in your codes
//   including Copyright credit and License Information.
// 4. Your codes should inherit this license information.
//=========================================================
// ---- Patent Notice -------------------------------------
// I have not cared whether this system (hw + sw) causes
// infringement on the patent, copyright, trademark,
// or trade secret rights of others. You have all
// responsibilities for determining if your designs
// and products infringe on the intellectual property
// rights of others, when you use technical information
// included in this system for your business.
//=========================================================
// ---- Disclaimers ---------------------------------------
// The function and reliability of this system are not
// guaranteed. They may cause any damages to loss of
// properties, data, money, profits, life, or business.
// By adopting this system even partially, you assume
// all responsibility for its use.
//=========================================================

#include <project.h>
#include <stdarg.h>
#include "com.h"
#include "utility.h"

#if 0
//----------------
// Defines
//----------------
#define USBUART_TIMEOUT 2000000 // Timeout for USB Connection (adjustment might be required)

//----------------
// Globals
//----------------
volatile uint32 gUSBUART_Ready = 1;

//=====================
// Initialize COM
//=====================
void Init_COM(void)
{
    uint32 i;

    //USBUART_1_Start(0, USBUART_1_3V_OPERATION);
    USBUART_1_Start();

    //while(!USBUART_1_bGetConfiguration());
    for (i = 0; i < USBUART_TIMEOUT; i++)
    {
        gUSBUART_Ready = USBUART_1_bGetConfiguration();
        if (gUSBUART_Ready) break;
    }
    // Initiaize UABUART
    if (gUSBUART_Ready)
        USBUART_1_CDC_Init();
    else
        USBUART_1_Stop();
    
    // Dummy Tx to establish Terminal on PC
    COM_printf("This is a Dummy Tx.\r\n");
}
#endif
//=====================
// COM printf
//=====================
void COM_printf(const char *format, ...)
{
    va_list ap;
    uint8 buf[256];
    uint8 *pStr;
    
    //if (gUSBUART_Ready == 0) return;

    va_start(ap, format);
    xvsnprintf(buf, 256, format, ap);
    va_end(ap);

    pStr = buf;
    while(*pStr != '\0')
    {
        //while (USBUART_1_CDCIsReady() == 0);
        //USBUART_1_PutChar(*pStr++);
        UART_1_PutChar(*pStr++);
    }
}
#if 0
//===========================
// Wait for setup PC Terminal
//===========================
void COM_Wait_PC_Term(void)
{
    uint32 i;
    uint8  len;
    uint8  dummy;
    //
    if (gUSBUART_Ready == 0) return;
    //
    while(1)
    {
        COM_printf("Push any Key to Start...\r");
        len = USBUART_1_DataIsReady();
        if (len > 0) break;
    }
    for (i = 0; i < len; i++) USBUART_1_GetData(&dummy, 1);
    COM_printf("\r\nOK!\r\n");
}
#endif
//=========================================================
// End of Program
//=========================================================
