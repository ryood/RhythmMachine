/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * 2015.11.05 Decay、Levelをロータリーエンコーダで変更
 * 2015.11.04 my_rand()関数を自作(遅い？ので保留)
 * 2015.11.03 モジュレーション波形のDDSパラメータの計算の仕方を分離（ノイズあり）
 * 2015.11.03 LCD制御をファイル分割
 * 2015.11.03 ロータリーエンコーダの読み取り
 * 2015.10.31 BPMの変更の反映(ノイズあり)
 * 2015.10.29 sequencerRdBufferを構造体に変更
 * 2015.10.29 シーケンサー基板からのデータを反映（2015.10.27版データフォーマット)
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

#include "SPLC792-I2C.h"
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

#define SEQUENCER_I2C_WR_BUFFER_SIZE  (1u)

#define UPDATE_TRACK        (0x02)
#define UPDATE_PLAY         (0x04)
#define UPDATE_SEQUENCE1    (0x08)
#define UPDATE_SEQUENCE2    (0x10)
#define UPDATE_POT1         (0x20)
#define UPDATE_POT2         (0x40)

// I2C LCD
//
#define LCD_I2C_SLAVE_ADDRESS   (0x3E)
#define LCD_CONTRAST            (0b110000)

// Sequencer Board I2C Command valid status
//
#define SEQUENCER_I2C_TRANSFER_CMPLT    (0x00u)
#define SEQUENCER_I2C_RX_ERROR          (0xFEu)    
#define SEQUENCER_I2C_TX_ERROR          (0xFFu)

// 波形生成
//
#define SAMPLE_CLOCK			(8000u)	    // 8kHz

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
* 大域変数
****************************************/
// Sequencer                
//
struct sequencer_parameter {
    uint8_t update;
	uint8_t track;
	uint8_t play;
	uint8_t sequence1;
	uint8_t sequence2;
	uint8_t pot1;
	uint8_t pot2;
} sequencerRdBuffer;
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

char8 lcdLine[17];

// デバッグ用
//uint8 RECount1, RECount2;
uint8 isREDirty;

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

#if 0
// 乱数生成
// Return: uint32_t: 0..0xFFFFの乱数
//
#define MY_RAND_MAX (0xFFFF)
static uint32_t next = 1;
inline uint32_t my_rand(void)
{
	next = next * 1103515245 + 12345;
	return (uint32_t)(next >> 16) & MY_RAND_MAX;
}
#endif

// 乱数生成
// Return: fp32: -1.0 .. 1.0の乱数
//
inline fp32 generateNoise()
{
    int32 r, v;
	fp32 fv;
	
    //r = my_rand();
    r = rand() >> 15;
    v = (r & 0x8000) ? (0xffff0000 | (r << 1)) : (r << 1);
	fv = (fp32)v;

    return fv;
}

/*======================================================
 * DDSパラメータ
 *
 *======================================================*/
// BPMの設定
//
// parameter: bpm 設定するbpm
//
inline void setBPM()
{
    ticksPerNote = SAMPLE_CLOCK * 60 / (bpm * 4);
    // ↑整数演算のため丸めているので注意
}

// モジュレーション波形のDDSパラメータの計算
// 
// parameter: n: 計算するトラック番号
//
inline void setModDDSParameter(uint8 n)
{
    // Decay
	//decayPeriod = (SAMPLE_CLOCK / (((double)bpm / 60) * 4)) * ((double)decayAmount / 256);
	tracks[n].decayPeriod = ((uint64_t)SAMPLE_CLOCK * 60 * tracks[n].decayAmount) / ((uint64_t)bpm * 4 * 256);
	//decayTuningWord = ((((double)bpm / 60) * 4) / ((double)decayAmount / 256)) * (double)POW_2_32 / SAMPLE_CLOCK;
	tracks[n].decayTuningWord = (bpm * ((uint64_t)POW_2_32 / 60) * 4 * 256 / tracks[n].decayAmount) / SAMPLE_CLOCK;
}

void initDDSParameter()
{
    uint8 i;
    
    setBPM(); 
    
    for (i = 0; i < TRACK_N; i++) {
        // 波形
		tracks[i].waveTuningWord = tracks[i].waveFrequency * POW_2_32 / SAMPLE_CLOCK;
		tracks[i].wavePhaseRegister = 0;
#if 0
		// Decay
		//decayPeriod = (SAMPLE_CLOCK / (((double)bpm / 60) * 4)) * ((double)decayAmount / 256);
		tracks[i].decayPeriod = ((uint64_t)SAMPLE_CLOCK * 60 * tracks[i].decayAmount) / ((uint64_t)bpm * 4 * 256);
		//decayTuningWord = ((((double)bpm / 60) * 4) / ((double)decayAmount / 256)) * (double)POW_2_32 / SAMPLE_CLOCK;
		tracks[i].decayTuningWord = (bpm * ((uint64_t)POW_2_32 / 60) * 4 * 256 / tracks[i].decayAmount) / SAMPLE_CLOCK;
#endif
        
        // モジュレーション
        setModDDSParameter(i);
    
        tracks[i].decayPhaseRegister = 0;
		tracks[i].decayStop = 0;
	}
}    


/*======================================================
 * Sequencer Board 
 *
 *======================================================*/
uint32 readSequencerBoard(void)
{
    uint32 status = SEQUENCER_I2C_RX_ERROR; 
    
    // Read from sequencer board
    //
    I2CM_Sequencer_Board_I2CMasterReadBuf(SEQUENCER_I2C_SLAVE_ADDRESS, 
        (uint8 *)&sequencerRdBuffer,
        sizeof(sequencerRdBuffer),
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
        if (I2CM_Sequencer_Board_I2CMasterGetReadBufSize() == sizeof(sequencerRdBuffer))
        {
            status = SEQUENCER_I2C_TRANSFER_CMPLT;
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
    uint32 status = SEQUENCER_I2C_TX_ERROR; 
    
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
            status = SEQUENCER_I2C_TRANSFER_CMPLT;
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
    const char *strPlayStop[] = { "S", "P" };
    const char *strTracks[] = { "BD", "SN", "HC" };
    char lcdBuffer[17];

    LCD_SetPos(0, 0);
    sprintf(lcdBuffer, "%s %3d %s %3u %3u",
        strPlayStop[sequencerRdBuffer.play],
        (sequencerRdBuffer.pot2 << 4) | sequencerRdBuffer.pot1,
        strTracks[sequencerRdBuffer.track],
        tracks[sequencerRdBuffer.track].decayAmount,
        tracks[sequencerRdBuffer.track].ampAmount
    );
    LCD_Puts(lcdBuffer);

    sequenceString(lcdBuffer, sequencerRdBuffer.sequence1, sequencerRdBuffer.sequence2);
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
 * Rotary Encoder
 *
 *======================================================*/
// 戻り値: ロータリーエンコーダーの回転方向
//        0:変化なし 1:時計回り -1:反時計回り
//
int readRE(int RE_n)
{
    static uint8_t index[2];
    uint8_t rd;
    int retval = 0;
    
    switch (RE_n) {
    case 0:
        rd = Pin_RE1_Read();
        break;
    case 1:
        rd = Pin_RE2_in_Read();
        break;
    default:
        displayError("RE_n OB", "");
    }

    index[RE_n] = (index[RE_n] << 2) | rd;
	index[RE_n] &= 0b1111;

	switch (index[RE_n]) {
	// 時計回り
	case 0b0001:	// 00 -> 01
	case 0b1110:	// 11 -> 10
	    retval = 1;
	    break;
	// 反時計回り
	case 0b0010:	// 00 -> 10
	case 0b1101:	// 11 -> 01
	    retval = -1;
	    break;
    }
    return retval;
}

void readDecayAndLevel()
{   
    int rv;
    int16 amt;
    
    rv = readRE(0);
    if (rv != 0) {
        amt = tracks[sequencerRdBuffer.track].decayAmount;
        amt += rv << 2;
        if (amt > 0 && amt <= UINT8_MAX) {
            isREDirty |= (1 << 0);
            tracks[sequencerRdBuffer.track].decayAmount = amt;
            setModDDSParameter(sequencerRdBuffer.track);
        }
    }
    rv = readRE(1);
    if (rv != 0) {
        amt = tracks[sequencerRdBuffer.track].ampAmount;
        amt += rv << 2;
        if (amt >= 0 && amt <= UINT8_MAX) { 
            isREDirty |= (1 << 1);
            tracks[sequencerRdBuffer.track].ampAmount = amt;
        }
    }
}

/*======================================================
 * 波形初期化
 *
 *======================================================*/
void initTracks()
{
    /*
	const uint8_t kickSequence[]  = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0 };
	const uint8_t snareSequence[] = { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
	const uint8_t hihatSequnce[]  = { 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 };
    */
    
	// Kick
	tracks[0].waveLookupTable = waveTableSine;
	tracks[0].decayLookupTable = modTableLinerDown01;
	tracks[0].waveFrequency = 50.0f;
	tracks[0].decayAmount = 200;
	tracks[0].ampAmount = 160;
	tracks[0].toneAmount = 127;
	//memcpy(tracks[0].sequence, kickSequence, SEQUENCE_LEN);

	// Snare
	tracks[1].waveLookupTable = waveTableSine;
	tracks[1].decayLookupTable = modTableRampDown01;
	tracks[1].waveFrequency = 120.0f;
	tracks[1].decayAmount = 200;
	tracks[1].ampAmount = 128;
	tracks[1].toneAmount = 127;
	//memcpy(tracks[1].sequence, snareSequence, SEQUENCE_LEN);

	// HiHat
	tracks[2].waveLookupTable = waveTableSine;	// unused
	tracks[2].decayLookupTable = modTableLinerDown01;
	tracks[2].waveFrequency = 2500.0f;			// unused
	tracks[2].decayAmount = 16;
	tracks[2].ampAmount = 12;
	tracks[2].toneAmount = 127;
	//memcpy(tracks[2].sequence, hihatSequnce, SEQUENCE_LEN);
}

/*======================================================
 * シーケンサー基板からのパラメーターの設定
 * parameter: track_n: 設定するtrack番号
 *
 *======================================================*/
void setTracks(uint8 track_n)
{
    int i;
    
    if (sequencerRdBuffer.update & (UPDATE_POT1 | UPDATE_POT2)) {
        bpm = (sequencerRdBuffer.pot2 << 4) | sequencerRdBuffer.pot1;
        setBPM();
        for (i = 0; i < TRACK_N; i++) {
            setModDDSParameter(i);
        } 
    }
    if (sequencerRdBuffer.update & UPDATE_SEQUENCE1) {
        for (i = 0; i < 8; i++) {
            tracks[track_n].sequence[i] = (sequencerRdBuffer.sequence1 & (1 << i)) >> i;
        }
    }
    if (sequencerRdBuffer.update & UPDATE_SEQUENCE2) {
        for (i = 0; i < 8; i++) {
            tracks[track_n].sequence[i + 8] = (sequencerRdBuffer.sequence2 & (1 << i)) >> i;
        }
    }
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
int main()
{
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
    
    /* Init SPI */
    SPIM_Start();
    
    /* Init Timer and ISR */
    Timer_Sampling_Start();
    ISR_Timer_Sampling_StartEx(Timer_Sampling_interrupt_handler);
    
    CyGlobalIntEnable;
    
    LCD_Init(LCD_I2C_SLAVE_ADDRESS, LCD_CONTRAST);
    LCD_Clear();
	LCD_Puts("Sequencer Board");
    
    CyDelay(500);
    
    int lcdWaitCount = 0;
    for(;;)
    {
        sequencerWrBuffer[0] = noteCount % 16;
        
        if (readSequencerBoard() != SEQUENCER_I2C_TRANSFER_CMPLT) {
            displayError("I2C Master", "Read Error");
        }
        
        if (writeSequencerBoard() != SEQUENCER_I2C_TRANSFER_CMPLT) {
            displayError("I2C Master", "Write Error");
        }
        
        if (sequencerRdBuffer.track >= TRACK_N)
            displayError("Sequencer Param", "TRACK NO OB");
            
        setTracks(sequencerRdBuffer.track);
        readDecayAndLevel();
        
        // パラメータに変更があった場合、LCD表示を更新
        lcdWaitCount++;
        if ((isREDirty || sequencerRdBuffer.update) && lcdWaitCount > 5) {
            lcdWaitCount = 0;
            displaySequencerParameter();
            isREDirty = 0;
        }
        
        // ロータリーエンコーダ
        // デバッグ用
        /*
        int rv = readRE(0);
        if (rv != 0) isREDirty = 1;        
        RECount1 += rv;
        rv = readRE(1);
        if (rv != 0) isREDirty = 1;
        RECount2 += rv;
        */
        
        CyDelay(2);
    }
}

/* [] END OF FILE */
