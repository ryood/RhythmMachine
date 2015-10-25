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
#include <stdio.h>

// Sequencer
//
#define SEQUENCER_I2C_SLAVE_ADDRESS   (0x7f)
#define SEQUENCER_I2C_RD_BUFFER_SIZE  (6u)
#define SEQUENCER_I2C_WR_BUFFER_SIZE  (1u)

// I2C LCD
//
#define LCD_I2C_SLAVE_ADDRESS   (0x3e)
#define LCD_I2C_BUFFER_SIZE     (2u)
#define LCD_I2C_PACKET_SIZE     (LCD_I2C_BUFFER_SIZE)

#define LCD_CONTRAST            (0b100000)

/* Command valid status */
#define I2C_TRANSFER_CMPLT    (0x00u)
#define I2C_TRANSFER_ERROR    (0xFFu)

/***************************************
*               Macros
****************************************/
/* Set LED RED color */
#define RGB_LED_ON_RED  \
                do{     \
                    LED_RED_Write  (0u); \
                    LED_GREEN_Write(1u); \
                }while(0)

/* Set LED GREEN color */
#define RGB_LED_ON_GREEN \
                do{      \
                    LED_RED_Write  (1u); \
                    LED_GREEN_Write(0u); \
                }while(0) 

/***************************************
*               大域変数
****************************************/
// Sequencer                
//
uint8 sequencerRdBuffer[SEQUENCER_I2C_RD_BUFFER_SIZE];
uint8 sequencerWrBuffer[SEQUENCER_I2C_WR_BUFFER_SIZE] = {0};

/*======================================================
 * Sequencer Board 
 *
 *======================================================*/
uint32 readSequencerBoard(void)
{
    uint32 status = I2C_TRANSFER_ERROR; 
    
    // Read from sequencer board
    //
    I2CM_I2CMasterReadBuf(SEQUENCER_I2C_SLAVE_ADDRESS, 
        sequencerRdBuffer,
        SEQUENCER_I2C_RD_BUFFER_SIZE,
        I2CM_I2C_MODE_COMPLETE_XFER
    );
    while (0u == (I2CM_I2CMasterStatus() & I2CM_I2C_MSTAT_RD_CMPLT))
    {
        /* Waits until master completes read transfer */
    }
    
    /* Displays transfer status */
    if (0u == (I2CM_I2C_MSTAT_ERR_XFER & I2CM_I2CMasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if (I2CM_I2CMasterGetReadBufSize() == SEQUENCER_I2C_RD_BUFFER_SIZE)
        {
            status = I2C_TRANSFER_CMPLT;
        }
    }
    else
    {
        RGB_LED_ON_RED;
    }

    (void) I2CM_I2CMasterClearStatus();
    
    return status;
}

uint32 writeSequencerBoard(void)
{
    uint32 status = I2C_TRANSFER_ERROR; 
    
    I2CM_I2CMasterWriteBuf(SEQUENCER_I2C_SLAVE_ADDRESS,
        sequencerWrBuffer,
        SEQUENCER_I2C_WR_BUFFER_SIZE,
        I2CM_I2C_MODE_COMPLETE_XFER
    );
    while (0u == (I2CM_I2CMasterStatus() & I2CM_I2C_MSTAT_WR_CMPLT))
    {
        /* Waits until master completes read transfer */
    }
    
    /* Displays transfer status */
    if (0u == (I2CM_I2C_MSTAT_ERR_XFER & I2CM_I2CMasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if (I2CM_I2CMasterGetWriteBufSize() == SEQUENCER_I2C_WR_BUFFER_SIZE)
        {
            status = I2C_TRANSFER_CMPLT;
        }
    }
    else
    {
        RGB_LED_ON_RED;
    }

    (void) I2CM_I2CMasterClearStatus();
    
    return status;   
}

/*======================================================
 * LCD制御
 *              
 *======================================================*/
uint32 LCD_Write(uint8 *buffer)
{
	uint32 status = I2C_TRANSFER_ERROR;
	
    I2CM_I2CMasterWriteBuf(LCD_I2C_SLAVE_ADDRESS,
        buffer,
        LCD_I2C_PACKET_SIZE,
        I2CM_I2C_MODE_COMPLETE_XFER
    );
    while (0u == (I2CM_I2CMasterStatus() & I2CM_I2C_MSTAT_WR_CMPLT))
    {
        /* Waits until master completes write transfer */
    }

    /* Displays transfer status */
    if (0u == (I2CM_I2C_MSTAT_ERR_XFER & I2CM_I2CMasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if(I2CM_I2CMasterGetWriteBufSize() == LCD_I2C_BUFFER_SIZE)
        {
            status = I2C_TRANSFER_CMPLT;
			
			// １命令ごとに余裕を見て50usウェイトします。
			CyDelayUs(50);	
        }
    }
    else
    {
        RGB_LED_ON_RED;
    }

    (void) I2CM_I2CMasterClearStatus();
	   
	return (status);
}

// コマンドを送信します。HD44780でいうRS=0に相当
void LCD_Cmd(uint8 cmd)
{
	uint8 buffer[LCD_I2C_BUFFER_SIZE];
	buffer[0] = 0b00000000;
	buffer[1] = cmd;
	(void) LCD_Write(buffer);
}

// データを送信します。HD44780でいうRS=1に相当
void LCD_Data(uint8 data)
{
	uint8 buffer[LCD_I2C_BUFFER_SIZE];
	buffer[0] = 0b01000000;
	buffer[1] = data;
	(void) LCD_Write(buffer);
}

void LCD_Init()
{
	CyDelay(40);
	LCD_Cmd(0b00111000);	// function set
	LCD_Cmd(0b00111001);	// function set
	LCD_Cmd(0b00010100);	// interval osc
	LCD_Cmd(0b01110000 | (LCD_CONTRAST & 0xF));	// contrast Low
	LCD_Cmd(0b01011100 | ((LCD_CONTRAST >> 4) & 0x3)); // contast High/icon/power
	LCD_Cmd(0b01101100); // follower control
	CyDelay(300);
	
	LCD_Cmd(0b00111000); // function set
	LCD_Cmd(0b00001100); // Display On
}

void LCD_Clear()
{
	LCD_Cmd(0b00000001); // Clear Display
	CyDelay(2);	// Clear Displayは追加ウェイトが必要
}

void LCD_SetPos(uint32 x, uint32 y)
{
	LCD_Cmd(0b10000000 | (x + y * 0x40));
}

// （主に）文字列を連続送信します。
void LCD_Puts(char8 *s)
{
	while(*s) {
		LCD_Data((uint8)*s++);
	}
}

/*======================================================
 * Display Parameter on Char LCD
 *
 *======================================================*/
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
    const char *strPlayStop[] = { "PLAY", "STOP" }; 
    char lcdBuffer[17];

    LCD_Clear();
    sprintf(lcdBuffer, "%s %3d %3d %3d",
        strPlayStop[sequencerRdBuffer[5]],
        sequencerRdBuffer[3],
        sequencerRdBuffer[2],
        sequencerRdBuffer[4]    
    );
    LCD_Puts(lcdBuffer);

    sequenceString(lcdBuffer, sequencerRdBuffer[0], sequencerRdBuffer[1]);
    LCD_SetPos(0, 1);
    LCD_Puts(lcdBuffer);
}

/*======================================================
 * Main Routine 
 *
 *======================================================*/
uint8 inc_within_uint8(uint8 x, uint8 h, uint8 l)
{
    x++;
    if (x == h)
        x = l;
    return x;
}

int main()
{
    char uartBuffer[80];
    
    UART_1_Start();    
    UART_1_UartPutString("Sequencer Board Test\r\n");
    
    // Sequence Boardをリセット
    Pin_I2C_Reset_Write(0u);
    CyDelay(1);
    Pin_I2C_Reset_Write(1u);
    
    /* Init I2C */
    I2CM_Start();
    CyDelay(1500);
    
    CyGlobalIntEnable;
    
    LCD_Init();
    LCD_Clear();
	LCD_Puts("Sequencer Board");
    
    CyDelay(1000);
        
    for(;;)
    {  
        if (readSequencerBoard() == I2C_TRANSFER_CMPLT) {
            sprintf(uartBuffer, "%d %d %d %d %d %d ",
                sequencerRdBuffer[0],
                sequencerRdBuffer[1],
                sequencerRdBuffer[2],
                sequencerRdBuffer[3],
                sequencerRdBuffer[4],
                sequencerRdBuffer[5]
            );
            UART_1_UartPutString(uartBuffer);
        }
        else {
            UART_1_UartPutString("I2C Master Sequencer Read Error.\r\n");
        }
        
        if (writeSequencerBoard() == I2C_TRANSFER_CMPLT) {
            sprintf(uartBuffer, "%d\r\n", sequencerWrBuffer[0]);
            UART_1_UartPutString(uartBuffer);
        }
        else {
            UART_1_UartPutString("I2C Master Sequencer Write Error.\r\n");
        }
        
        displaySequencerParameter();
        
        sequencerWrBuffer[0] = inc_within_uint8(sequencerWrBuffer[0], 16, 0);
        
        CyDelay(125);
    }
}

/* [] END OF FILE */
