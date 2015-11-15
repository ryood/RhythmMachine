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
#ifndef _DDS_H_
#define _DDS_H_
    
#include "fixedpoint.h"
#include <stdint.h>
    
//-------------------------------------------------
// 波形生成
//
#define SAMPLE_CLOCK			(8000u)	    // 8kHz

#define TRACK_N					(8)		    // トラックの個数
#define WAVE_LOOKUP_TABLE_SIZE	(1024u)		// Lookup Table の要素数
#define MOD_LOOKUP_TABLE_SIZE	(128u)
#define SEQUENCE_LEN		 	(16u)
#define INITIAL_BPM				(120u)

#define POW_2_32			    (4294967296ull) // 2の32乗

//-------------------------------------------------
// シーケンサーパラメータ
//
#define UPDATE_TRACK        (0x02)
#define UPDATE_PLAY         (0x04)
#define UPDATE_SEQUENCE1    (0x08)
#define UPDATE_SEQUENCE2    (0x10)
#define UPDATE_POT1         (0x20)
#define UPDATE_POT2         (0x40)
    
struct sequencer_parameter {
    uint8_t update;
	uint8_t track;
	uint8_t play;
	uint8_t sequence1;
	uint8_t sequence2;
	uint8_t pot1;
	uint8_t pot2;
};

struct track {
	const fp32 *waveLookupTable;
	const fp32 *decayLookupTable;
	double waveFrequency;
	uint8_t decayAmount;
	uint8_t levelAmount;
	int8_t toneAmount;
	
	uint32_t wavePhaseRegister;
	uint32_t waveTuningWord;
	fp32 waveValue;
	
	uint32_t decayPhaseRegister;
	uint32_t decayTuningWord;
	uint32_t decayPeriod;
	uint8_t decayStop;
	fp32 decayValue;
	
	uint8_t sequence[SEQUENCE_LEN];	// Velocity
};

int getNoteCount();
void initTracks(struct track *tracks);
void initDDSParameter(struct track* tracks);
inline void setModDDSParameter(struct track *track);
void setTrack(struct track *tracks, int track_n, struct sequencer_parameter *param);
fp32 generateWave(struct track *tracks);

#endif // _DDS_H_

/* [] END OF FILE */
