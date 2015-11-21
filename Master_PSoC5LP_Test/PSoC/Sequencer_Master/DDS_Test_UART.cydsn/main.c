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
 * 2015.11.21 Created.
 *
 * ========================================
*/
#include <project.h>
#include "dds.h"
#include "utility.h"

#define LOOP_N_GENERATE_NOISE           30000
#define LOOP_N_FILTERED_GENERATE_NOISE  30000

fp32 generateNoise();
fp32 generateFilteredNoise();

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
    fp32 fr;
    
    UART_printf("test generateNoise()\r\n");
    
    for (int i = 0; i < LOOP_N_GENERATE_NOISE; i++) {
        fr = generateNoise();
        UART_printf("%d\r\n", (int32)fr);
    }
}

//=================================================
// Test generateFilteredNoise()
// 
//=================================================
uint16_t filterFunc(uint16_t sample)
{
    uint16_t f_sample;
    
    // Debug用
    //Pin_Ext_Clock_Write(1u);
    
    /* Enable the interrupt register bit to poll
     Value 1 for Channel A, Value 2 for Channel B */
    Filter_INT_CTRL_REG |= (1 << Filter_CHANNEL_A);
    
    Filter_Write16(Filter_CHANNEL_A, sample);
    /* Poll waiting for the holding register to have data to read */
    while (Filter_IsInterruptChannelA() == 0) ;
    f_sample = Filter_Read16(Filter_CHANNEL_A);
    
    // Debug用
    //Pin_Ext_Clock_Write(0u);
    
    return f_sample;
}

void test_generateFilteredNoise()
{
    fp32 fr;
  
    UART_printf("test generateFilteredNoise()\r\n");
    
    Filter_Start();
    setFilterRoutine(&filterFunc);
    
    for (int i = 0; i < LOOP_N_GENERATE_NOISE; i++) {
        fr = generateFilteredNoise();
        UART_printf("%d\r\n", (int32)fr);
    }
    
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
    UART_PutString("dds module test..\r\n");

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
