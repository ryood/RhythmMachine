//=========================================================
// PSoC5LP Project
//=========================================================
// File Name : com.h
// Function  : COM Port Control Header
//---------------------------------------------------------
// Rev.01 2013.02.04 Munetomo Maruyama
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

#ifndef __COM_H__
#define __COM_H__

#include <project.h>

//==============
// Prototypes
//==============
//void  Init_COM(void);
void  COM_printf(const char *format, ...);
//void  COM_Wait_PC_Term(void);

#endif // __COM_H__

//=========================================================
// End of Program
//=========================================================
