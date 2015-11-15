/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * 波形の生成
 * 2015.11.15 Created
 *
 * ========================================
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dds.h"
#include "WaveTableFP32.h"
#include "ModTableFP32.h"

//-------------------------------------------------
// BPM
//
static uint8_t bpm;				// 1分あたりのbeat数 (beat=note*4)
static uint32_t ticksPerNote;	// noteあたりのサンプリング数

//-------------------------------------------------
// カウンタ
static int tick = -1;  // 初回に0にインクリメント
static int noteCount = 0;

//=================================================
// getter / setter
// 
//=================================================
int getNoteCount()
{
    return noteCount;
}

//=================================================
// DDSパラメータ
// 
//=================================================
//-------------------------------------------------
// BPMの設定
//
inline void setBPM(uint8_t _bpm)
{
    bpm = _bpm;
    ticksPerNote = SAMPLE_CLOCK * 60 / (bpm * 4);
    // ↑整数演算のため丸めているので注意
}

//-------------------------------------------------
// モジュレーション波形のDDSパラメータの計算
// 
// parameter: track: 計算するトラックデータ
//
inline void setModDDSParameter(struct track *track)
{
    // Decay
	//decayPeriod = (SAMPLE_CLOCK / (((double)bpm / 60) * 4)) * ((double)decayAmount / 256);
	track->decayPeriod = ((uint64_t)SAMPLE_CLOCK * 60 * track->decayAmount) / ((uint64_t)bpm * 4 * 256);
	//decayTuningWord = ((((double)bpm / 60) * 4) / ((double)decayAmount / 256)) * (double)POW_2_32 / SAMPLE_CLOCK;
	track->decayTuningWord = (bpm * ((uint64_t)POW_2_32 / 60) * 4 * 256 / track->decayAmount) / SAMPLE_CLOCK;
}

//-------------------------------------------------
// メイン波形のDDSパラメータの計算
// 
// parameter: track: 計算するトラックデータ
//
inline void setWaveDDSParameter(struct track *track)
{
    //tracks[n].waveTuningWord = tracks[n].waveFrequency * POW_2_32 / SAMPLE_CLOCK;
    track->waveTuningWord = (track->waveFrequency
        + (track->waveFrequency * (((double)track->toneAmount/* - 128.0f*/) / 128.0f)))
        * POW_2_32 / SAMPLE_CLOCK;
}

//-------------------------------------------------
// DDSパラメータの初期化
// 
// parameter: track: 計算するトラックデータ
//
void initDDSParameter(struct track* tracks)
{
    uint8_t i;
    
    setBPM(INITIAL_BPM); 
    
    for (i = 0; i < TRACK_N; i++) {
        // 波形
		//tracks[i].waveTuningWord = tracks[i].waveFrequency * POW_2_32 / SAMPLE_CLOCK;
        setWaveDDSParameter(&tracks[i]);
		tracks[i].wavePhaseRegister = 0;
        
        // モジュレーション
        setModDDSParameter(&tracks[i]);    
        tracks[i].decayPhaseRegister = 0;
		tracks[i].decayStop = 0;
	}
}    

//=================================================
// 波形の初期化
// parameter: tracks: トラックデータの配列
//
//================================================= 
void initTracks(struct track *tracks)
{
 	const uint8_t kickSequence[]  = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0 };
	const uint8_t snareSequence[] = { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
	const uint8_t hihatSequnce[]  = { 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 };
    
	// Kick
	tracks[0].waveLookupTable = waveTableSine;
	tracks[0].decayLookupTable = modTableLinerDown01;
	tracks[0].waveFrequency = 50.0f;
	tracks[0].decayAmount = 200;
	tracks[0].levelAmount = 200;
	tracks[0].toneAmount = 0;
	memcpy(tracks[0].sequence, kickSequence, SEQUENCE_LEN);

	// Snare
	tracks[1].waveLookupTable = waveTableSine;
	tracks[1].decayLookupTable = modTableRampDown01;
	tracks[1].waveFrequency = 120.0f;
	tracks[1].decayAmount = 200;
	tracks[1].levelAmount = 200;
	tracks[1].toneAmount = 0;
	memcpy(tracks[1].sequence, snareSequence, SEQUENCE_LEN);

	// HiHat
	tracks[2].waveLookupTable = waveTableSine;	// unused
	tracks[2].decayLookupTable = modTableSustainBeforeRampDown01;
	tracks[2].waveFrequency = 2500.0f;			// unused
	tracks[2].decayAmount = 16;
	tracks[2].levelAmount = 8;
	tracks[2].toneAmount = 0;
	memcpy(tracks[2].sequence, hihatSequnce, SEQUENCE_LEN);
}

//=================================================
// シーケンサー基板からのパラメーターの設定
// parameter: track: トラックデータの配列
//            track_n: 設定するトラック番号
//            pram: シーケンサーパラメータ
//
//=================================================
void setTrack(struct track *tracks, int track_n, struct sequencer_parameter *param)
{
    int i;
    
    if (param->update & (UPDATE_POT1 | UPDATE_POT2)) {
        setBPM((param->pot2 << 4) | param->pot1);
        for (i = 0; i < TRACK_N; i++) {
            setModDDSParameter(&tracks[i]);
        } 
    }
    if (param->update & UPDATE_SEQUENCE1) {
        for (i = 0; i < 8; i++) {
            tracks[track_n].sequence[i] = (param->sequence1 & (1 << i)) >> i;
        }
    }
    if (param->update & UPDATE_SEQUENCE2) {
        for (i = 0; i < 8; i++) {
            tracks[track_n].sequence[i + 8] = (param->sequence2 & (1 << i)) >> i;
        }
    }
}

//=================================================
// 波形の生成
//
//=================================================
//-------------------------------------------------
// DDS波形の生成
// retrun: fp32型の波形値
// parameter: phaseRegister: 波形のPhase Register
//          : tuningWord: 波形のTuning Word
//          : lookupTable: 波形データのテーブル
//
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

//-------------------------------------------------
// 乱数生成
// return: fp32型の-1.0 .. 1.0の乱数
//
inline fp32 generateNoise()
{
    int32_t r, v;
	fp32 fv;
	
    //r = my_rand();
    r = rand() >> 15;
    v = (r & 0x8000) ? (0xffff0000 | (r << 1)) : (r << 1);
	fv = (fp32)v;

    return fv;
}

//-------------------------------------------------
// 波形生成ルーチン
// return: fp32型の波形値 
// parameter: tracks: トラックデータの配列
//
fp32 generateWave(struct track *tracks)
{
    int i;
    
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
		fv = fp32_mul(fv, int_to_fp32(tracks[i].levelAmount));
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
	//fp32 fp32_12bit = fp32_mul(synthWaveValue + int_to_fp32(1), int_to_fp32(2047));
	//int16_t i12v = fp32_to_int(fp32_12bit);

    //DACSetVoltage16bit(i12v);
        
    // for 8bit output (0..255)
	// 256で乗算すると8bit幅を超えるため255で乗算
	//
	//fp32 fp32_8bit = fp32_mul(synthWaveValue + int_to_fp32(1), int_to_fp32(255));
	//int16_t i8v = fp32_to_int(fp32_8bit);    
    //VDAC8_Wave_SetValue(i8v);
        
    return synthWaveValue;
}    

/* [] END OF FILE */
