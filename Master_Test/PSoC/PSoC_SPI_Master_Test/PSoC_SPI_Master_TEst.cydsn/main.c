/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * 2015.10.19 転送クロックを24MHzまでテスト
 * 2015.10.19 SPIM_ReadTxStatus()でSPIの完了待ち
 *
 * ========================================
*/

#include <project.h>

void DACSetVoltage(uint16 value)
{
    uint8 txDataH, txDataL;
    
    // Highバイト(0x30=OUTA/BUFなし/1x/シャットダウンなし)
    txDataH = 0x30 | (value >> 8);
    txDataL = (value & 0x00ff); 
    
    Pin_LDAC_Write(1u);
    
    SPIM_WriteTxData(txDataH);
    SPIM_WriteTxData(txDataL);
    
    while(0u == (SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE))  {
         // Wait while Master completes transfer
    }
        
    Pin_LDAC_Write(0u);
}

void DACSetVoltage16bit(uint16 value)
{
    // Highバイト(0x30=OUTA/BUFなし/1x/シャットダウンなし)
    value = (value & ~0xF000) | 0x3000;  
    
    Pin_LDAC_Write(1u);
    
    SPIM_WriteTxData(value);
    
    while(0u == (SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE))  {
         // Wait while Master completes transfer
    }
        
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
