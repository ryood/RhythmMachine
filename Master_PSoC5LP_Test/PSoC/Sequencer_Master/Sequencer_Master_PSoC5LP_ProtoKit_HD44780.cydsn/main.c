/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * 2015.11.13 Master Clockを72MHzに変更
 * 2015.11.13 PSoC 5LP Prototyping Kitに移植
 * 2015.11.13 Sampling Timerをインプリメント
 * 2015.11.13 Sequencer Board, Char LCDをインプリメント
 * 2015.11.12 新規作成
 *
 * ========================================
*/
#include <project.h>
#include <stdio.h>

#include "utility.h"
#include "dds.h"

#define TITLE_STR   ("Rhythm Machine")
#define VERSION_STR ("2015.11.13")

// Sequencer
//
#define SEQUENCER_I2C_SLAVE_ADDRESS   (0x7f)
#define SEQUENCER_I2C_WR_BUFFER_SIZE  (1u)

#define SEQUENCER_TRANSFER_CMPLT    (0x00u)
#define SEQUENCER_RX_TRANSFER_ERROR (0xFEu)
#define SEQUENCER_TX_TRANSFER_ERROR (0xFFu)

#define TRACK_N (3)

// Error
//
#define ERR_SEQUENCER_READ  (0x01)
#define ERR_SEQUENCER_WRITE (0x02)

/**************************************************
waveLookupTable   : fp32 Q16 : -1.0 .. +1.0
decayLookupTable  : fp32 Q16 : -1.0 .. +1.0

wavePhaseRegister : 32bit
waveTunigWord     : 32bit
decayPhaseRegister: 32bit
decayTuningWord   : 32bit

waveFrequency     : double
levelAmount       : 8bit
toneAmount        : 8bit    // 未実装
decayAmount       : 8bit

bpmAmount         : 8bit
volumeAmount      : 8bit

***************************************************/

//=================================================
// マクロ関数
// 
//=================================================
#define RGB_LED_ON_RED  \
                do{     \
                    LED_RED_Write  (1u); \
                    LED_GREEN_Write(0u); \
                    LED_BLUE_Write (0u); \
                }while(0)

/* Set LED GREEN color */
#define RGB_LED_ON_GREEN \
                do{      \
                    LED_RED_Write  (0u); \
                    LED_GREEN_Write(1u); \
                    LED_BLUE_Write (0u); \
                }while(0)
/* Set LED BLUE color */
#define RGB_LED_ON_BLUE  \
                do{      \
                    LED_RED_Write  (0u); \
                    LED_GREEN_Write(0u); \
                    LED_BLUE_Write (1u); \
                }while(0)
/* Set LED BLUE color */
#define RGB_LED_OFF      \
                do{      \
                    LED_RED_Write  (0u); \
                    LED_GREEN_Write(0u); \
                    LED_BLUE_Write (0u); \
                }while(0)

//=================================================
// 大域変数
// 
//=================================================
//-------------------------------------------------
// Sequencer                
//
struct sequencer_parameter sequencerRdBuffer;
uint8 sequencerWrBuffer[SEQUENCER_I2C_WR_BUFFER_SIZE] = {0};

uint8 sequencerRdStatus;
uint8 sequencerWrStatus;

//struct track tracks[TRACK_N];

//=================================================
// LCD
// 
//=================================================                
//-------------------------------------------------
// LCD printf
// Parameter: line:   表示する桁 (0..1)
//            format: 書式
//            ...:    可変引数リスト
void LCD_printf(int line, const char *format, ...)
{
    va_list ap;
    uint8 buf[256];
    
    va_start(ap, format);
    xvsnprintf(buf, 256, format, ap);
    va_end(ap);

    //LCD_Char_ClearDisplay();
    LCD_Char_Position(line, 0);
    LCD_Char_PrintString((char8 *)buf);
}

//-------------------------------------------------
// エラー処理
// parameter: code: エラーコード
//            ext:  付帯数値
void error(uint32 code, uint32 ext)
{
    char8 error_str[][17] = {
        "",
        "Sequencer Rd Err",   // 0x01
        "Sequencre Wt Err",   // 0x02
    };
    
    LCD_Char_ClearDisplay();
    LCD_printf(0, "%s", error_str[code]);
    LCD_printf(1, "%ld", ext);
    
    // loop for ever
    for (;;);
}

//-------------------------------------------------
// シーケンサーパラメータ表示
//
//-------------------------------------------------
// シーケンス表示用文字列生成
// parameter: buffer: 文字列格納バッファ
//            sequence1: シーケンス1
//            sequence2: シーケンス2
void sequenceString(char *buffer, uint8 sequence1, uint8 sequence2)
{
    const char charOnOff[2] = { '.', 'o' };
    int i;
    
    for (i = 0; i < 8; i++) {
        buffer[i] = charOnOff[(sequence1 & (1 << i)) >> i];
    }
    for (i = 0; i < 8; i++) {
        buffer[i + 8] = charOnOff[(sequence2 & (1 << i)) >> i];
    }
}

void displaySequencerParameter()
{
    const char *strTracks[] = { "KIK", "SNR", "HHC" };
    char lineBuffer[17];
    
    LCD_Char_Position(0, 0);
    LCD_printf(0, "%03d %s %2u %2u %2u",
        (sequencerRdBuffer.pot2 << 4) | sequencerRdBuffer.pot1,
        strTracks[sequencerRdBuffer.track],
        //tracks[sequencerRdBuffer.track].ampAmount >> 2,
        //tracks[sequencerRdBuffer.track].toneAmount >> 2,
        //tracks[sequencerRdBuffer.track].decayAmount >> 2
        0, 0, 0
    );
    
    sequenceString(lineBuffer, sequencerRdBuffer.sequence1, sequencerRdBuffer.sequence2);
    LCD_Char_Position(1, 0);
    LCD_Char_PrintString(lineBuffer);
}

//=================================================
// Sequencer
//
//=================================================
//-------------------------------------------------
// Sequencerから受信
// return: Error code
//
uint32 readSequencerBoard(void)
{
    uint32 status = SEQUENCER_RX_TRANSFER_ERROR;
    
    I2CM_Sequencer_MasterReadBuf(
        SEQUENCER_I2C_SLAVE_ADDRESS, 
        (uint8 *)&sequencerRdBuffer, 
        sizeof(sequencerRdBuffer), 
        I2CM_Sequencer_MODE_COMPLETE_XFER
    );
    
    while (0u == (I2CM_Sequencer_MasterStatus() & I2CM_Sequencer_MSTAT_RD_CMPLT))
    {
        /* Waits until master completes read transfer */
    }
    
    /* Displays transfer status */
    if (0u == (I2CM_Sequencer_MSTAT_ERR_XFER & I2CM_Sequencer_MasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if (I2CM_Sequencer_MasterGetReadBufSize() == sizeof(sequencerRdBuffer))
        {
            status = SEQUENCER_TRANSFER_CMPLT;
        }
    }
    else
    {
        RGB_LED_ON_RED;
    }
    
    sequencerRdStatus = I2CM_Sequencer_MasterStatus();
    (void) I2CM_Sequencer_MasterClearStatus();
    
    return status;
}

//-------------------------------------------------
// Sequencerに送信
// return: Error code
//
uint32 writeSequencerBoard(void)
{
    uint32 status = SEQUENCER_TX_TRANSFER_ERROR;
    
    I2CM_Sequencer_MasterWriteBuf(SEQUENCER_I2C_SLAVE_ADDRESS,
        sequencerWrBuffer,
        SEQUENCER_I2C_WR_BUFFER_SIZE,
        I2CM_Sequencer_MODE_COMPLETE_XFER
    );
    
    while (0u == (I2CM_Sequencer_MasterStatus() & I2CM_Sequencer_MSTAT_WR_CMPLT))
    {
        /* Waits until master completes read transfer */
    }
    
    /* Displays transfer status */
    if (0u == (I2CM_Sequencer_MSTAT_ERR_XFER & I2CM_Sequencer_MasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if (I2CM_Sequencer_MasterGetWriteBufSize() == SEQUENCER_I2C_WR_BUFFER_SIZE)
        {
            status = SEQUENCER_TRANSFER_CMPLT;
        }
    }
    else
    {
        RGB_LED_ON_RED;
    }

    sequencerWrStatus = I2CM_Sequencer_MasterStatus();
    (void) I2CM_Sequencer_MasterClearStatus();

    return status;
}

//=================================================
// Sampling Timer
//
//=================================================
//-------------------------------------------------
// 割り込み処理ルーチン
//
CY_ISR(Timer_Sampling_interrupt_handler)
{
    // デバッグ用
    Pin_ISR_Check_Write(1u);
    Pin_ISR_Check_Write(0u);
    
    /* Read Status register in order to clear the sticky Terminal Count (TC) bit 
	 * in the status register. Note that the function is not called, but rather 
	 * the status is read directly.
	 */
   	Timer_Sampling_STATUS;
}

//=================================================
// メインルーチン
//
//=================================================
int main()
{
    // LCDを初期化
    LCD_Char_Start();  
    LCD_printf(0, TITLE_STR);
    LCD_printf(1, VERSION_STR);
    
    // LED Check
    RGB_LED_ON_RED;
    CyDelay(500);
    RGB_LED_ON_GREEN;
    CyDelay(500);
    RGB_LED_ON_BLUE;
    CyDelay(500);
    RGB_LED_OFF;
  
    // Sequencerをリセット
    Pin_Sequencer_Reset_Write(0u);
    CyDelay(1);
    Pin_Sequencer_Reset_Write(1u);
    
    // Sequencerを初期化
    I2CM_Sequencer_Start();
    
    // Sampling Timerを初期化
    Timer_Sampling_Start();
    ISR_Timer_Sampling_StartEx(Timer_Sampling_interrupt_handler);
    
    CyGlobalIntEnable;
    
    // Sequencerの初期化待ち
    CyDelay(2000);
    
    for(;;)
    {
        // Read from sequencer board
        //
        if (readSequencerBoard() != SEQUENCER_TRANSFER_CMPLT) {
            error(ERR_SEQUENCER_READ, sequencerRdStatus);
        };
        
        // Write to sequencer board
        //
        if (writeSequencerBoard() != SEQUENCER_TRANSFER_CMPLT) {
            error(ERR_SEQUENCER_WRITE, sequencerWrStatus);
        }            
        
        displaySequencerParameter();
        
        sequencerWrBuffer[0]++;
        if (sequencerWrBuffer[0] == 16)
            sequencerWrBuffer[0] = 0;
        
        CyDelay(125);
    }
}

/* [] END OF FILE */
