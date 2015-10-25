/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>

void DACSetVoltage(uint16 value)  
{  
    uint16 txDataH, txDataL;  

    // Highバイト(0x30=OUTA/BUFなし/1x/シャットダウンなし)  
    txDataH = (value >> 8) | 0x30;
    txDataL = (value & 0xff);

    Pin_LDAC_Write(1u);  
    SPIM_SpiUartWriteTxData(txDataH);
    SPIM_SpiUartWriteTxData(txDataL);   // bufferが空くまでblockするので連続出力可
    
    while(0u == (SPIM_GetMasterInterruptSource() & SPIM_INTR_MASTER_SPI_DONE))  
    {  
        /* Wait while Master completes transfer */  
    }
    /* Clear interrupt source after transfer completion */  
    SPIM_ClearMasterInterruptSource(SPIM_INTR_MASTER_SPI_DONE); 
    Pin_LDAC_Write(0u);
}  

void DACSetVoltage16bit(uint16 value)
{
    // Highバイト(0x30=OUTA/BUFなし/1x/シャットダウンなし)
    value = (value & ~0xF000) | 0x3000;  
    
    Pin_LDAC_Write(1u);
    
    SPIM_SpiUartWriteTxData(value);
    
    while(0u == (SPIM_GetMasterInterruptSource() & SPIM_INTR_MASTER_SPI_DONE))  
    {  
        /* Wait while Master completes transfer */  
    }
    /* Clear interrupt source after transfer completion */  
    SPIM_ClearMasterInterruptSource(SPIM_INTR_MASTER_SPI_DONE);
    
    Pin_LDAC_Write(0u);
}

int main()
{    
    CyGlobalIntEnable; /* Enable global interrupts. */

    SPIM_Start();
    
    uint16 i = 0;
    for(;;)
    {
        DACSetVoltage16bit(i);
        //DACSetVoltage(i);
        i += 8;
        if (i == 4096) 
            i = 0;
    }
}
/* [] END OF FILE */
 
