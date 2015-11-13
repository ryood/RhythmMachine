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

#include <stdint.h>
#include "dds.h"

#if 0
//=================================================
// 波形の初期化
// 
//================================================= 
void initTracks(struct track *tracks)
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
	tracks[0].ampAmount = 200;
	tracks[0].toneAmount = 128;
	//memcpy(tracks[0].sequence, kickSequence, SEQUENCE_LEN);

	// Snare
	tracks[1].waveLookupTable = waveTableSine;
	tracks[1].decayLookupTable = modTableRampDown01;
	tracks[1].waveFrequency = 120.0f;
	tracks[1].decayAmount = 200;
	tracks[1].ampAmount = 200;
	tracks[1].toneAmount = 128;
	//memcpy(tracks[1].sequence, snareSequence, SEQUENCE_LEN);

	// HiHat
	tracks[2].waveLookupTable = waveTableSine;	// unused
	tracks[2].decayLookupTable = modTableSustainBeforeRampDown01;
	tracks[2].waveFrequency = 2500.0f;			// unused
	tracks[2].decayAmount = 16;
	tracks[2].ampAmount = 8;
	tracks[2].toneAmount = 128;
	//memcpy(tracks[2].sequence, hihatSequnce, SEQUENCE_LEN);
}
#endif
/* [] END OF FILE */
