//=========================================================
// PSoC5LP Project
//=========================================================
// File Name : utiity.h
// Function  : Utility Routine Header
//---------------------------------------------------------
// Rev.01 2012.09.01 Munetomo Maruyama
//---------------------------------------------------------
// Copyright (C) 2012-2013 Munetomo Maruyama
//=========================================================
// ---- License Information -------------------------------
// Anyone can FREELY use this code fully or partially
// under conditions shown below.
// 1. You may use this code only for individual purpose,
//    and educational purpose.
//    Do not use this code for business even if partially.
// 2. You can copy, modify and distribute this code.
// 3. You should remain this header text in your codes
//   including Copyright credit and License Information.
// 4. Your codes should inherit this license information.
//=========================================================
// ---- Patent Notice -------------------------------------
// I have not cared whether this system (hw + sw) causes
// infringement on the patent, copyright, trademark,
// or trade secret rights of others. You have all
// responsibilities for determining if your designs
// and products infringe on the intellectual property
// rights of others, when you use technical information
// included in this system for your business.
//=========================================================
// ---- Disclaimers ---------------------------------------
// The function and reliability of this system are not
// guaranteed. They may cause any damages to loss of
// properties, data, money, profits, life, or business.
// By adopting this system even partially, you assume
// all responsibility for its use.
//=========================================================

#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <project.h>

#include <stdarg.h>
//#include "fixedpoint.h"

//=============
// Prototypes
//=============
//int32 power(int32 x, int32 n);
//uint8 BCD_INT(uint8 num);
//uint8 INT_BCD(uint8 bcd);
uint32 xatoi(uint8 **str, int32 *data);
uint8 *xitoa(uint8 *str, uint32 *pLength, int32 value, int8 radix, int8 width);
uint8 *xsnprintf(uint8 *str, uint32 length, const char *format, ...);
uint8 *xvsnprintf(uint8 *str, uint32 length, const char *format, va_list ap);

//uint8* Append_String_UI32 (uint32 value, uint8 *string, uint32 radix, uint32 length);
//uint8* Append_String_SI32 (int32 value, uint8 *string, uint32 radix, uint32 length);
//uint8* Append_String_Fixed(fix32 fvalue, uint8 *string, uint32 format);

//uint8* Append_String_UI08_Ndigit(uint32 value, uint8 *string, uint32 digit);
//uint8* Append_String_UI08_Ndigit(uint32 value, uint8 *string, uint32 digit);
//uint8* Append_String(uint8 *pDst, uint8 *pSrc);


#endif // __UTILITY_H__

//=========================================================
// End of Program
//=========================================================
