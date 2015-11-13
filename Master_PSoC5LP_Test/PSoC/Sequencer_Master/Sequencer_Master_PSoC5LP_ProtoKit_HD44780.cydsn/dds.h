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
    
#define SEQUENCE_LEN    (16)
    
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
};
    
#endif // _DDS_H_

/* [] END OF FILE */
