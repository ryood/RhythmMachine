/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * DDS モジュール テストプログラム
 *
 * 2015.11.28 ノイズ生成をDDSモジュールから外出し
 * 2015.11.21 Created.
 *
 * ========================================
*/
#include <project.h>
#include "dds.h"
#include "utility.h"

#define LOOP_N_GENERATE_NOISE           30000
#define LOOP_N_FILTERED_GENERATE_NOISE  30000

fp32 generateNoiseWhite();
fp32 generateNoiseBlue();

void UART_printf(const char *format, ...)
{
    va_list ap;
    uint8 buf[256];
    
    va_start(ap, format);
    xvsnprintf(buf, 256, format, ap);
    va_end(ap);

    UART_PutString((char8 *)buf);
}

//=================================================
// Test generateNoise()
// 
//=================================================
void test_generateNoise()
{
    uint16 r;
    
    PRS_Start();
    UART_printf("test generateNoise()\r\n");
    
    for (int i = 0; i < LOOP_N_GENERATE_NOISE; i++) {
        Pin_Check0_Write(1u);
        r = PRS_Read();
        Pin_Check0_Write(0u);
        UART_printf("%d\r\n", (int32)r - 32768);
    }
    
    PRS_Stop();
}

//=================================================
// Test generateFilteredNoise()
// 
//=================================================
void test_generateFilteredNoise()
{
    uint16 r, fr;
    
    PRS_Start();
    Filter_Start();
    UART_printf("test generateFilteredNoise()\r\n");
    
    /* Enable the interrupt register bit to poll
    Value 1 for Channel A, Value 2 for Channel B */
    Filter_INT_CTRL_REG |= (1 << Filter_CHANNEL_A);
    
    for (int i = 0; i < LOOP_N_GENERATE_NOISE; i++) {
        Pin_Check0_Write(1u);
        r = PRS_Read();
        Filter_Write16(Filter_CHANNEL_A, r);
        /* Poll waiting for the holding register to have data to read */
        while (Filter_IsInterruptChannelA() == 0) ;
        fr = Filter_Read16(Filter_CHANNEL_A);
        Pin_Check0_Write(0u);
        UART_printf("%d\r\n", (int32)fr - 32768);
    }
    
    PRS_Stop();
    Filter_Stop();
}

//=================================================
// Main routine
// 
//=================================================
int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    UART_Start();
    
    //test_generateNoise();
    test_generateFilteredNoise();
    
    for(;;)
    {
        Pin_UserLED_Write(1u);
        CyDelay(500);
        Pin_UserLED_Write(0u);
        CyDelay(500);
    }
}

/* [] END OF FILE */
