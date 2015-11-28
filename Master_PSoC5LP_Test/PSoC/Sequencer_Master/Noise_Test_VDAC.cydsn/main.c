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
 * 2015.11.28 PRSモジュールをテスト(rand()関数の方が周期性が少ない)
 * 2015.11.21 Timer割り込みではなくループでfilterd noiseをテスト
 * 2015.11.21 Created.
 *
 * ========================================
*/
#include <project.h>
#include <stdlib.h>
#include "dds.h"
#include "utility.h"

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
    uint16 r;
    
    //r = PRS_1_Read();
    r = rand() & UINT16_MAX;
    VDAC8_1_SetValue(r >> 8);
    
    //UART_printf("%u\r\n", r);
}

//=================================================
// Test generateFilteredNoise()
// 
//=================================================
void test_generateFilteredNoise()
{
    uint16 r;
    uint16 fr;
    
    //r = PRS_1_Read();    
    r = rand() % UINT16_MAX;
    
    /* Enable the interrupt register bit to poll
       Value 1 for Channel A, Value 2 for Channel B */
    Filter_INT_CTRL_REG |= (1 << Filter_CHANNEL_A);
          
    Filter_Write16(Filter_CHANNEL_A, r);
    
    /* Poll waiting for the holding register to have data to read */
    while (Filter_IsInterruptChannelA() == 0) ;
    
    fr = Filter_Read16(Filter_CHANNEL_A);
    
    VDAC8_1_SetValue(fr >> 8);
    
    //UART_printf("%u\r\n", fr);   
}

//=================================================
// Timer
// 
//=================================================
CY_ISR(timer_interrupt_handler)
{
     /* Read Status register in order to clear the sticky Terminal Count (TC) bit 
	 * in the status register. Note that the function is not called, but rather 
	 * the status is read directly.
	 */
   	Timer_1_STATUS;
    
    Pin_ISR_Check_Write(1u);
    //test_generateNoise();
    test_generateFilteredNoise();
    Pin_ISR_Check_Write(0u);
}

//=================================================
// Main routine
// 
//=================================================
int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    VDAC8_1_Start();
    Opamp_1_Start();

    Filter_Start();
    PRS_1_Start();
    UART_Start();
    
    UART_printf("Noise_Test_VDAC...\r\n");

    CyDelay(100);
    
    Timer_1_Start();
    ISR_Timer_1_StartEx(timer_interrupt_handler);
    
    for(;;)
    {
        Pin_UserLED_Write(1u);
        CyDelay(100);
        Pin_UserLED_Write(0u);
        CyDelay(100);
    }
}

/* [] END OF FILE */
