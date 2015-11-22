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
 * 2015.11.21 Timer割り込みではなくループでfilterd noiseをテスト
 * 2015.11.21 Created.
 *
 * ========================================
*/
#include <project.h>
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
    fp32 fv, fv8;
    uint8 i8v;
    
    fv = generateNoise();
    fv8 = fp32_mul(fv + int_to_fp32(1), int_to_fp32(127));
    i8v = fp32_to_int(fv8);
    
    VDAC8_1_SetValue(i8v);
}

//=================================================
// Test generateFilteredNoise()
// 
//=================================================
uint16_t filterFunc(uint16_t sample)
{
    uint16_t f_sample;
    
    // Debug用
    Pin_Ext_Check_Write(1u);
    
    /* Enable the interrupt register bit to poll
     Value 1 for Channel A, Value 2 for Channel B */
    Filter_INT_CTRL_REG |= (1 << Filter_CHANNEL_A);
          
    Filter_Write16(Filter_CHANNEL_A, sample);
    
    /* Poll waiting for the holding register to have data to read */
    
    while (Filter_IsInterruptChannelA() == 0) ;
    
    f_sample = Filter_Read16(Filter_CHANNEL_A);
    
    // Debug用
    Pin_Ext_Check_Write(0u);
    
    return f_sample;
}

void test_generateDFBFilteredNoise()
{
    fp32 fv, fv8;
    uint8 i8v;
    
    fv = generateFilteredNoise();
    fv8 = fp32_mul(fv + int_to_fp32(1), int_to_fp32(127));
    i8v = fp32_to_int(fv8);
    
    VDAC8_1_SetValue(i8v);
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
    test_generateDFBFilteredNoise();
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
    //Timer_1_Start();
    //ISR_Timer_1_StartEx(timer_interrupt_handler);

    Filter_Start();
    setFilterRoutine(&filterFunc);

    for(;;)
    {
        //Pin_UserLED_Write(1u);
        Pin_ISR_Check_Write(1u);
        //test_generateFilteredNoise();
        test_generateDFBFilteredNoise();
        Pin_ISR_Check_Write(0u);
        //Pin_UserLED_Write(0u);
    }
}

/* [] END OF FILE */
