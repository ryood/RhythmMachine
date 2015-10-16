/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * 2015.10.15 Timerを使って実行時間計測
 * 2015.10.13 Created. 実行時間計測 (途中)
 *
 * ========================================
 */

#include <project.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "fixedpoint.h"
#include "WaveTableFp32.h"
#include "ModTableFp32.h"

#define _UART_OUT_HEADER_ 0
#define _UART_OUT_12BIT_ 1
#define _UART_OUT_RUNTIME_ 0

#define _TRACE_ 0
#define _TRACE_TRACK_ 0
#define _TRACE_GENERATE_WAVE_DDS_ 0
#define _TRACE_GENERATE_WAVE_NOISE_ 0

#define LOOP_N 48000

/*********************************************************
waveLookupTable   : fp32 Q16 : -1.0 .. +1.0
decayLookupTable  : fp32 Q16 : -1.0 .. +1.0

wavePhaseRegister : 32bit
waveTunigWord     : 32bit
decayPhaseRegister: 32bit
decayTuningWord   : 32bit

waveFrequency     : double
ampAmount         : 8bit
toneAmount        : 8bit
decayAmount       : 8bit

bpmAmount         : 8bit
**********************************************************/

#define SAMPLE_CLOCK			(48000u)	// 48kHz

#define TRACK_N					(3u)		// トラックの個数
#define WAVE_LOOKUP_TABLE_SIZE	(1024u)		// Lookup Table の要素数
#define MOD_LOOKUP_TABLE_SIZE	(128u)
#define SEQUENCE_LEN		 	(16u)

#define POW_2_32				(4294967296ull) // 2の32乗

// 変数の初期値
#define INITIAL_BPM				(120u)

//*******************************************************************
// 大域変数
//

// カウンター
int tick = -1;				// 初回に0にインクリメント
int noteCount = 0;

// BPM
uint8_t bpm;				// 1分あたりのbeat数 (beat=note*4)
uint32_t ticksPerNote;		// noteあたりのサンプリング数

// UART出力用
static char lineBuffer[80];

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

//*******************************************************************
// PSoC用
// 固定小数点数を1000倍してintに変換
//
int32 fp32_to_int_1000(fp32 fv)
{
    return (int32)(fp32_to_double(fv) * 1000);    
}   

//*******************************************************************
// トラックの初期化
//
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
	tracks[2].waveFrequency = 1000.0f;			// unused
	tracks[2].decayAmount = 24;
	tracks[2].ampAmount = 24;
	tracks[2].toneAmount = 127;
	memcpy(tracks[2].sequence, hihatSequnce, SEQUENCE_LEN);
   
}

//*******************************************************************
// DDSパラメーターの初期化
//
void initDDSParameter()
{
    uint i;
    
    for (i = 0; i < TRACK_N; i++) {
		// 波形
		tracks[i].waveTuningWord = tracks[i].waveFrequency * POW_2_32 / SAMPLE_CLOCK;
		tracks[i].wavePhaseRegister = 0;

#if _TRACE_
		sprintf(lineBuffer, "track:\t%d\r\n waveFrequency:\t%d\r\n waveTunigWord:\t%u\r\n",
            i, (int)tracks[i].waveFrequency, tracks[i].waveTuningWord);
        UART_UartPutString(lineBuffer);
#endif
		// Decay
		//decayPeriod = (SAMPLE_CLOCK / (((double)bpm / 60) * 4)) * ((double)decayAmount / 256);
		tracks[i].decayPeriod = ((uint64_t)SAMPLE_CLOCK * 60 * tracks[i].decayAmount) / ((uint64_t)bpm * 4 * 256);
		//decayTuningWord = ((((double)bpm / 60) * 4) / ((double)decayAmount / 256)) * (double)POW_2_32 / SAMPLE_CLOCK;
		tracks[i].decayTuningWord = (bpm * ((uint64_t)POW_2_32 / 60) * 4 * 256 / tracks[i].decayAmount) / SAMPLE_CLOCK;
		tracks[i].decayPhaseRegister = 0;
		tracks[i].decayStop = 0;
		
#if _TRACE_        
		sprintf(lineBuffer, " decayAmount:\t%u\r\n decayPeriod:\t%u\r\n decayTunigWord:\t%u\r\n",
            tracks[i].decayAmount, tracks[i].decayPeriod, tracks[i].decayTuningWord);
        UART_UartPutString(lineBuffer);
#endif		
	}
}

//*******************************************************************
// 波形の生成
//
inline fp32 generateDDSWave(uint32_t *phaseRegister, uint32_t tuningWord, const fp32 *lookupTable)
{
	*phaseRegister += tuningWord;

	// lookupTableの要素数に丸める
	// 32bit -> 10bit
	uint16_t index = (*phaseRegister) >> 22;
	fp32 waveValue = *(lookupTable + index);

#if _TRACE_GENERATE_WAVE_DDS_    
    sprintf(lineBuffer, "%u\t%d\t%f\t", *phaseRegister, index, fp32_to_double(waveValue));
    UART_UartPutString(lineBuffer);
#endif
    
	return waveValue;
}

// -1.0 .. 1.0の乱数
// PSoC用に変更
inline fp32 generateNoise()
{
    int32 r, v;
	fp32 fv;
	
    r = rand() >> 15;
    v = (r & 0x8000) ? (0xffff0000 | (r << 1)) : (r << 1);
	fv = (fp32)v;
    
#if _TRACE_GENERATE_WAVE_NOISE_    
	sprintf(lineBuffer, "%ld\t%ld\t%ld\r\n", r, v, fp32_to_int_1000(fv));
    UART_UartPutString(lineBuffer);
#endif   

	return fv;
}
    
//*******************************************************************
// メインルーチン
//
int main()
{
    uint32 startTime, endTime;
    uint32 i, loop;
    
    /* Start SCB (UART mode) operation */
    UART_Start();
    
#if _UART_OUT_HEADER_
    UART_UartPutString("\r\n***********************************************************************************\r\n");
    UART_UartPutString("Fixed Point DDS RhythmMachine Test.\r\n");
    UART_UartPutString("\r\n");

    sprintf(lineBuffer, "RAND_MAX: %d\r\n", RAND_MAX);
    UART_UartPutString(lineBuffer);
#endif

    Timer_1_Start();
    
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    initTracks();

	// BPMの計算
	//
	bpm = INITIAL_BPM;
    
#if _TRACE_    
	sprintf(lineBuffer, "bpm:\t%u\r\n", bpm);
    UART_UartPutString(lineBuffer);
#endif

	ticksPerNote = SAMPLE_CLOCK * 60 / (bpm * 4);
	// ↑整数演算のため丸めているので注意
    
#if _TRACE_   
	sprintf(lineBuffer, "ticksPerNote:\t%d\r\n", ticksPerNote);
    UART_UartPutString(lineBuffer);
#endif

    initDDSParameter();
    
    /**********************************************************
     * 実行時間計測スタート
     **********************************************************/
    startTime = Timer_1_ReadCounter();

    for (loop = 0; loop < LOOP_N; loop++)
    {        
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
            
    		if (tracks[i].sequence[noteCount % SEQUENCE_LEN] == 0) {
				tracks[i].waveValue = int_to_fp32(0);
				continue;
			}
            
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

#if _TRACE_TRACK_            
			sprintf(lineBuffer, "%d\t%d\t%d\t%d\t",
                tick, tracks[i].decayPhaseRegister, decayIndex, fp32_to_int(tracks[i].decayValue));
            UART_UartPutString(lineBuffer);
#endif
			
			// サンプル毎の振幅変調の合算 **********************************************
			//
			//************************************************************************ 
			fp32 amValue = tracks[i].decayValue;

#if _TRACE_TRACK_            
			sprintf(lineBuffer, "%d\t", fp32_to_int(amValue));
            UART_UartPutString(lineBuffer);
#endif            

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
				//tracks[i].waveValue = generateNoise();
                
                tracks[i].waveValue = generateDDSWave(
					&(tracks[i].wavePhaseRegister),
					tracks[i].waveTuningWord,
					tracks[i].waveLookupTable);
                
				break;
			default:
                ;
				//error("Track no. out of range: %d\n", i);
			}			

			// 振幅変調 ***************************************************************
			// waveValue: -1.0 .. 1.0
			// amValue:    0.0 .. 1.0
			//************************************************************************
			tracks[i].waveValue = fp32_mul(tracks[i].waveValue, amValue);

#if _TRACE_TRACK_            
			sprintf(lineBuffer, "%d\t", fp32_to_int(amValue));
            UART_UartPutString(lineBuffer);

            UART_UartPutString("\r\n");
#endif            
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

#if _TRACE_            
			sprintf(lineBuffer, "%d\r\n", fp32_to_int(fp32_mul(fv, int_to_fp32(1000))));
            UART_UartPutString(lineBuffer);
#endif

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

		// for 12bit output (0..4095)
		// 2048で乗算すると12bit幅を超えるため2047で乗算
		//
		fp32 fp32_12bit = fp32_mul(synthWaveValue + int_to_fp32(1), int_to_fp32(2047));
		int16_t i12v = fp32_to_int(fp32_12bit);
		
#if _UART_OUT_12BIT_        
        sprintf(lineBuffer, "%d\r\n", i12v);
        UART_UartPutString(lineBuffer);
#endif        

		// for 12bit output (0..4095) as 16bit RAW format
		//
		//printf("%d\t", (int)(i12v - 2048) << 4);

		// for 16bit output (-32768 .. 32767)
		//
		//fp32 fp32_16bit = fp32_mul(synthWaveValue, int_to_fp32(32767));
		//int16_t out_16bit = fp32_to_int(fp32_16bit);
		//printf("%d\t", out_16bit);
		//fwrite(&out_16bit, sizeof(out_16bit), 1, stdout);
	}
    
    /**********************************************************
     * 実行時間計測終了
     **********************************************************/
    endTime = Timer_1_ReadCounter();

#if _UART_OUT_RUNTIME_    
    sprintf(lineBuffer, "start: %lu, end: %lu, diff: %lu\r\n", startTime, endTime, endTime - startTime);
    UART_UartPutString(lineBuffer);
#endif

    UART_UartPutString("end\r\n");
    
    for(;;)
        ;
}

/* [] END OF FILE */
