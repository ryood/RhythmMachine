/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * 2015.10.27 シーケンサー基板からのデータを反映（kickのみ）
 * 2015.10.25 I2C Masterを2系統に分割 (動作OK)
 * 2015.10.25 I2C Masterを2系統に分割
 * 2015.10.25 DDSで波形出力（作成）
 * 2015.10.25 SPI Master ComponentでMCP4922へ出力
 *
 * ========================================
*/
#include <project.h>
#include <stdio.h>
#include <stdlib.h>

#include "fixedpoint.h"
#include "WaveTableFp32.h"
#include "ModTableFp32.h"

/*********************************************************************
waveLookupTable   : fp32 Q16 : -1.0 .. +1.0
decayLookupTable  : fp32 Q16 : -1.0 .. +1.0

wavePhaseRegister : 32bit
waveTunigWord     : 32bit
decayPhaseRegister: 32bit
decayTuningWord   : 32bit

waveFrequency     : double
ampAmount         : 8bit    // 未実装
toneAmount        : 8bit    // 未実装
decayAmount       : 8bit

bpmAmount         : 8bit
volumeAmount      : 8bit    // 未実装

*********************************************************************/

/***************************************
* Macros
****************************************/

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

// I2C Command valid status
//
#define I2C_TRANSFER_CMPLT    (0x00u)
#define I2C_TRANSFER_ERROR    (0xFFu)

// 波形生成
//
#define SAMPLE_CLOCK			(8000u)	// 8kHz

#define TRACK_N					(3u)		// トラックの個数
#define WAVE_LOOKUP_TABLE_SIZE	(1024u)		// Lookup Table の要素数
#define MOD_LOOKUP_TABLE_SIZE	(128u)
#define SEQUENCE_LEN		 	(16u)

#define POW_2_32			    (4294967296ull) // 2の32乗

// 波形変数の初期値
//
#define INITIAL_BPM				(120u)

/***************************************
* Macro関数
****************************************/
/* Set LED RED color */
#define RGB_LED_ON_RED  \
                do{     \
                    Pin_LED_RED_Write  (0u); \
                    Pin_LED_GREEN_Write(1u); \
                }while(0)

/* Set LED GREEN color */
#define RGB_LED_ON_GREEN \
                do{      \
                    Pin_LED_RED_Write  (1u); \
                    Pin_LED_GREEN_Write(0u); \
                }while(0) 

/***************************************
*               大域変数
****************************************/
// Sequencer                
//
uint8 sequencerRdBuffer[SEQUENCER_I2C_RD_BUFFER_SIZE];
uint8 sequencerWrBuffer[SEQUENCER_I2C_WR_BUFFER_SIZE] = {0};

// カウンター
int tick = -1;				// 初回に0にインクリメント
int noteCount = 0;

// BPM
uint8_t bpm;				// 1分あたりのbeat数 (beat=note*4)
uint32_t ticksPerNote;		// noteあたりのサンプリング数

struct track {
	const fp32 *waveLookupTable;
	const fp32 *decayLookupTable;
	double waveFrequency;
	uint8_t decayAmount;
	uint8_t ampAmount;
	uint8_t toneAmount;
	
	uint32_t wavePhaseRegister;
	uint32_t waveTuningWord;
	fp32 waveValue;
	
	uint32_t decayPhaseRegister;
	uint32_t decayTuningWord;
	uint32_t decayPeriod;
	uint8_t decayStop;
	fp32 decayValue;
	
	uint8_t sequence[SEQUENCE_LEN];	// Velocity
} tracks[TRACK_N];

/*======================================================
 * Sequencer Board 
 *
 *======================================================*/
uint32 readSequencerBoard(void)
{
    uint32 status = I2C_TRANSFER_ERROR; 
    
    // Read from sequencer board
    //
    I2CM_Sequencer_Board_I2CMasterReadBuf(SEQUENCER_I2C_SLAVE_ADDRESS, 
        sequencerRdBuffer,
        SEQUENCER_I2C_RD_BUFFER_SIZE,
        I2CM_Sequencer_Board_I2C_MODE_COMPLETE_XFER
    );
    while (0u == (I2CM_Sequencer_Board_I2CMasterStatus() & I2CM_Sequencer_Board_I2C_MSTAT_RD_CMPLT))
    {
        /* Waits until master completes read transfer */
    }
    
    /* Displays transfer status */
    if (0u == (I2CM_Sequencer_Board_I2C_MSTAT_ERR_XFER & I2CM_Sequencer_Board_I2CMasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if (I2CM_Sequencer_Board_I2CMasterGetReadBufSize() == SEQUENCER_I2C_RD_BUFFER_SIZE)
        {
            status = I2C_TRANSFER_CMPLT;
        }
    }
    else
    {
        RGB_LED_ON_RED;
    }

    (void) I2CM_Sequencer_Board_I2CMasterClearStatus();
    
    return status;
}

uint32 writeSequencerBoard(void)
{
    uint32 status = I2C_TRANSFER_ERROR; 
    
    I2CM_Sequencer_Board_I2CMasterWriteBuf(SEQUENCER_I2C_SLAVE_ADDRESS,
        sequencerWrBuffer,
        SEQUENCER_I2C_WR_BUFFER_SIZE,
        I2CM_Sequencer_Board_I2C_MODE_COMPLETE_XFER
    );
    while (0u == (I2CM_Sequencer_Board_I2CMasterStatus() & I2CM_Sequencer_Board_I2C_MSTAT_WR_CMPLT))
    {
        /* Waits until master completes read transfer */
    }
    
    /* Displays transfer status */
    if (0u == (I2CM_Sequencer_Board_I2C_MSTAT_ERR_XFER & I2CM_Sequencer_Board_I2CMasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if (I2CM_Sequencer_Board_I2CMasterGetWriteBufSize() == SEQUENCER_I2C_WR_BUFFER_SIZE)
        {
            status = I2C_TRANSFER_CMPLT;
        }
    }
    else
    {
        RGB_LED_ON_RED;
    }

    (void) I2CM_Sequencer_Board_I2CMasterClearStatus();
    
    return status;   
}

/*======================================================
 * LCD制御
 *              
 *======================================================*/
uint32 LCD_Write(uint8 *buffer)
{
	uint32 status = I2C_TRANSFER_ERROR;
	
    I2CM_LCD_I2CMasterWriteBuf(LCD_I2C_SLAVE_ADDRESS,
        buffer,
        LCD_I2C_PACKET_SIZE,
        I2CM_LCD_I2C_MODE_COMPLETE_XFER
    );
    while (0u == (I2CM_LCD_I2CMasterStatus() & I2CM_LCD_I2C_MSTAT_WR_CMPLT))
    {
        /* Waits until master completes write transfer */
    }

    /* Displays transfer status */
    if (0u == (I2CM_LCD_I2C_MSTAT_ERR_XFER & I2CM_LCD_I2CMasterStatus()))
    {
        RGB_LED_ON_GREEN;

        /* Check if all bytes was written */
        if(I2CM_LCD_I2CMasterGetWriteBufSize() == LCD_I2C_BUFFER_SIZE)
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

    (void) I2CM_LCD_I2CMasterClearStatus();
	   
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
    const char *strPlayStop[] = { "Play", "Stop" }; 
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

void displayStr(char8* line)
{
    LCD_Clear();
    LCD_Puts(line);
}

void displayError(char8* line1, char8* line2)
{
    LCD_Clear();
    LCD_Puts(line1);
    LCD_SetPos(0,1);
    LCD_Puts(line2);
    
    // 処理停止
    for (;;)
        ;
}

/*======================================================
 * DAC MCP4922(12bit)
 * Data Bits:16bitでSPI送信
 *
 *======================================================*/
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

/*======================================================
 * シーケンサー基板からのパラメーターの設定
 * parameter: track_n: 設定するtrack番号
 *
 *======================================================*/
void setTracks(uint8 track_n)
{
    int i;
      
    for (i = 0; i < 8; i++) {
        tracks[track_n].sequence[i] = (sequencerRdBuffer[0] & (1 << i)) >> i;
    }
    for (i = 0; i < 8; i++) {
        tracks[track_n].sequence[i + 8] = (sequencerRdBuffer[1] & (1 << i)) >> i;
    }
}

/*======================================================
 * 波形初期化
 *
 *======================================================*/
void initTracks()
{
	const uint8_t kickSequence[]  = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0 };
	const uint8_t snareSequence[] = { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
	const uint8_t hihatSequnce[]  = { 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 };

	// Kick
	tracks[0].waveLookupTable = waveTableSine;
	tracks[0].decayLookupTable = modTableDown;
	tracks[0].waveFrequency = 60.0f;
	tracks[0].decayAmount = 200;
	tracks[0].ampAmount = 255;
	tracks[0].toneAmount = 127;
	memcpy(tracks[0].sequence, kickSequence, SEQUENCE_LEN);

	// Snare
	tracks[1].waveLookupTable = waveTableSine;
	tracks[1].decayLookupTable = modTableDown;
	tracks[1].waveFrequency = 120.0f;
	tracks[1].decayAmount = 128;
	tracks[1].ampAmount = 255;
	tracks[1].toneAmount = 127;
	memcpy(tracks[1].sequence, snareSequence, SEQUENCE_LEN);

	// HiHat
	tracks[2].waveLookupTable = waveTableSine;	// unused
	tracks[2].decayLookupTable = modTableDown;
	tracks[2].waveFrequency = 2000.0f;			// unused
	tracks[2].decayAmount = 24;
	tracks[2].ampAmount = 24;
	tracks[2].toneAmount = 127;
	memcpy(tracks[2].sequence, hihatSequnce, SEQUENCE_LEN);
}

void initDDSParameter()
{
    uint i;
    
    for (i = 0; i < TRACK_N; i++) {
		// 波形
		tracks[i].waveTuningWord = tracks[i].waveFrequency * POW_2_32 / SAMPLE_CLOCK;
		tracks[i].wavePhaseRegister = 0;

		// Decay
		//decayPeriod = (SAMPLE_CLOCK / (((double)bpm / 60) * 4)) * ((double)decayAmount / 256);
		tracks[i].decayPeriod = ((uint64_t)SAMPLE_CLOCK * 60 * tracks[i].decayAmount) / ((uint64_t)bpm * 4 * 256);
		//decayTuningWord = ((((double)bpm / 60) * 4) / ((double)decayAmount / 256)) * (double)POW_2_32 / SAMPLE_CLOCK;
		tracks[i].decayTuningWord = (bpm * ((uint64_t)POW_2_32 / 60) * 4 * 256 / tracks[i].decayAmount) / SAMPLE_CLOCK;
		tracks[i].decayPhaseRegister = 0;
		tracks[i].decayStop = 0;
	}
}

/*======================================================
 * 波形の生成
 *
 *======================================================*/
inline fp32 generateDDSWave(uint32_t *phaseRegister, uint32_t tuningWord, const fp32 *lookupTable)
{
	*phaseRegister += tuningWord;

	// lookupTableの要素数に丸める
	// 32bit -> 10bit
	uint16_t index = (*phaseRegister) >> 22;
	fp32 waveValue = *(lookupTable + index);

	return waveValue;
}

// -1.0 .. 1.0の乱数(PSoC用)
inline fp32 generateNoise()
{
    int32 r, v;
	fp32 fv;
	
    r = rand() >> 15;
    v = (r & 0x8000) ? (0xffff0000 | (r << 1)) : (r << 1);
	fv = (fp32)v;

    return fv;
}

/*======================================================
 * Timer 割り込み
 *
 *======================================================*/
CY_ISR(Timer_Sampling_interrupt_handler)
{
    int i;
    
    Timer_Sampling_ClearInterrupt(Timer_Sampling_INTR_MASK_TC);
    
    // デバッグ用
    Pin_ISR_Check_Write(1u);
    Pin_ISR_Check_Write(0u);
    
    tick++;

	if (tick >= ticksPerNote) {
	    noteCount++;

		// noteの先頭でtickをリセット
		tick = 0;

		// noteの先頭でwavePhaseRegister, decayPhaserRegisterをリセット
		for (i = 0; i < TRACK_N; i++) {
			tracks[i].wavePhaseRegister = 0;
			tracks[i].decayPhaseRegister = 0;
			tracks[i].decayStop = 0;
		}
	}
	// トラックの処理
	//
	for (i = 0; i < TRACK_N; i++) {
        /*
    	if (tracks[i].sequence[noteCount % SEQUENCE_LEN] == 0) {
			tracks[i].waveValue = int_to_fp32(0);
			continue;
		}
        */
    	// Decayの処理 ***********************************************************
		//
		//***********************************************************************
		if (!tracks[i].decayStop) {
			tracks[i].decayPhaseRegister += tracks[i].decayTuningWord;
		}
		if (tick == tracks[i].decayPeriod - 1) {
			tracks[i].decayStop = 1;
		}
		// 32bitのphaseRegisterをテーブルの7bit(128個)に丸める
		int decayIndex = tracks[i].decayPhaseRegister >> 25;
		tracks[i].decayValue = *(tracks[i].decayLookupTable + decayIndex);
	
		// サンプル毎の振幅変調の合算 **********************************************
		//
		//************************************************************************ 
		fp32 amValue = tracks[i].decayValue;

		// Wave系の処理 ***********************************************************
		//
		//************************************************************************
		switch (i) {
		case 0:	// kick
			tracks[i].waveValue = generateDDSWave(
				&(tracks[i].wavePhaseRegister),
				tracks[i].waveTuningWord,
				tracks[i].waveLookupTable);
			break;
		case 1:	// snare
            tracks[i].waveValue = generateDDSWave(
				&(tracks[i].wavePhaseRegister),
				tracks[i].waveTuningWord,
				tracks[i].waveLookupTable);
            break;
		case 2:	// hihat
			tracks[i].waveValue = generateNoise();
            /*
            tracks[i].waveValue = generateDDSWave(
				&(tracks[i].wavePhaseRegister),
				tracks[i].waveTuningWord,
				tracks[i].waveLookupTable);
            */
            break;
		default:
                ;
			//ToDo: error処理
		}			

		// 振幅変調 ***************************************************************
		// waveValue: -1.0 .. 1.0
		// amValue:    0.0 .. 1.0
		//************************************************************************
		tracks[i].waveValue = fp32_mul(tracks[i].waveValue, amValue);
	}
	// トラックの合成 ***********************************************************
	//
	// ************************************************************************
	fp32 synthWaveValue = int_to_fp32(0);
	for (i = 0; i < TRACK_N; i++) {
		fp32 fv;
		// 各トラックの出力値： waveValue * sequence[note](Velocity) * ampAmount
		fv = fp32_mul(tracks[i].waveValue, int_to_fp32(tracks[i].sequence[noteCount % SEQUENCE_LEN]));
		fv = fp32_mul(fv, int_to_fp32(tracks[i].ampAmount));
		fv = fp32_div(fv, int_to_fp32(UINT8_MAX));
        // ToDo: ↑シフト演算で除算？

		synthWaveValue = fp32_add(synthWaveValue, fv); 
	}
	// 出力値の補正 ***********************************************************
	//
	// ************************************************************************
	// リミッター
	//
	if (synthWaveValue >= int_to_fp32(1))
		synthWaveValue = int_to_fp32(1);
	else if (synthWaveValue < int_to_fp32(-1))
		synthWaveValue = int_to_fp32(-1);

    //synthWaveValue = synthWaveValue >= int_to_fp32(1) ? int_to_fp32(1)
    //    : synthWaveValue < int_to_fp32(-1) ? int_to_fp32(-1) : synthWaveValue;
        
	// for 12bit output (0..4095)
	// 2048で乗算すると12bit幅を超えるため2047で乗算
	//
	fp32 fp32_12bit = fp32_mul(synthWaveValue + int_to_fp32(1), int_to_fp32(2047));
	int16_t i12v = fp32_to_int(fp32_12bit);

    DACSetVoltage16bit(i12v);
}    

/*======================================================
 * Main Routine 
 *
 *======================================================*/
/*
uint8 inc_within_uint8(uint8 x, uint8 h, uint8 l)
{
    x++;
    if (x == h)
        x = l;
    return x;
}
*/

int main()
{
    //char lcdLine[17];
    
    // 波形の初期化
    //
    bpm = INITIAL_BPM;
    ticksPerNote = SAMPLE_CLOCK * 60 / (bpm * 4);
	// ↑整数演算のため丸めているので注意

    initTracks();
    initDDSParameter();
    
    // Sequence Boardをリセット
    //
    Pin_I2C_Sequencer_Reset_Write(0u);
    CyDelay(1);
    Pin_I2C_Sequencer_Reset_Write(1u);
    
    /* Init I2C */
    I2CM_Sequencer_Board_Start();
    I2CM_LCD_Start();
    //CyDelay(100);
    
    /* Init SPI */
    SPIM_Start();
    
    /* Init Timer and ISR */
    Timer_Sampling_Start();
    ISR_Timer_Sampling_StartEx(Timer_Sampling_interrupt_handler);
    
    CyGlobalIntEnable;
    
    LCD_Init();
    LCD_Clear();
	LCD_Puts("Sequencer Board");
    
    CyDelay(500);
    
    int lcdCount = 0;
    for(;;)
    {
        sequencerWrBuffer[0] = noteCount % 16;
        
        if (readSequencerBoard() != I2C_TRANSFER_CMPLT) {
            displayError("I2C Master", "Read Error");
        }
        
        if (writeSequencerBoard() != I2C_TRANSFER_CMPLT) {
            displayError("I2C Master", "Write Error");
        }
        
        setTracks(sequencerRdBuffer[4] % TRACK_N);
        
        if (lcdCount++ == 500) {
            lcdCount = 0;
            displaySequencerParameter();
        }
        /*
        sprintf(lcdLine, "%d", sequencerWrBuffer[0]);            
        displayStr(lcdLine);
        */
        
        //sequencerWrBuffer[0] = inc_within_uint8(sequencerWrBuffer[0], 16, 0);
        
        //DACSetVoltage16bit(sequencerWrBuffer[0] << 8);
        
        //CyDelay(125);
    }
}

/* [] END OF FILE */
