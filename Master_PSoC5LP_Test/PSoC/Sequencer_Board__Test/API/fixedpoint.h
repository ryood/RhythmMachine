//=========================================================
// PSoC5LP Project
//=========================================================
// File Name : fixedpoint.h
// Function  : Fixed Point Library Header
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

#ifndef __FIXEDPOINT_H__
#define __FIXEDPOINT_H__

#include <project.h>

//#include <stdint.h>

//==================
// Define Types
//==================
typedef int32 fix32;
//typedef long long int64; // comment out this line if you use Creator 2.2SP1 or above.

//=====================
// Define Constants
//=====================
#define FIXQ 16
#define FIX_PI 0x0003243f // 3.14

//===================
// Define Operations
//===================
#define MIN(x, y) (((x) < (y))? x : y)
#define MAX(x, y) (((x) > (y))? x : y)

//==================
// Prototypes
//==================
fix32 FIX_INT(int32 ia);
int32 INT_FIX(fix32 fa);
fix32 FIX_DBL(double da);
double  DBL_FIX(fix32 fa);
//
fix32 FIX_Add(fix32 fa, fix32 fb);
fix32 FIX_Sub(fix32 fa, fix32 fb);
fix32 FIX_Abs(fix32 fa);
fix32 FIX_Mul(fix32 fa, fix32 fb);
fix32 FIX_Div(fix32 fa, fix32 fb);
fix32 FIX_Sqrt(fix32 fa);
//
fix32 FIX_Sin(fix32 frad);
fix32 FIX_Cos(fix32 frad);
fix32 FIX_Atan(fix32 fval);
fix32 FIX_Atan2(fix32 fnum, fix32 fden);
fix32 FIX_Deg_Rad(fix32 frad);
fix32 FIX_Rad_Deg(fix32 fdeg);
fix32 FIX_Sin_Deg(fix32 fdeg);
fix32 FIX_Cos_Deg(fix32 fdeg);
fix32 FIX_Atan_Deg(fix32 fval);
fix32 FIX_Atan2_Deg(fix32 fnum, fix32 fden);

#endif // __FIXEDPOINT_H__

//=========================================================
// End of Program
//=========================================================
