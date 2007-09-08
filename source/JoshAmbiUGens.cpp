/*
 *  JoshUGens.cpp
 *  xSC3plugins
 *
 *  Created by Josh Parmenter on 2/4/05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */
/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "SC_PlugIn.h"

const double sqrt3 = sqrt(3.);
const double sqrt3div6 = sqrt(3.) * 0.1666666667;
const double sqrt3div2 = sqrt(3.) * 0.5;
const double rsqrt6 = 1. / sqrt(6.);
const double sqrt6div3 = sqrt(6.) * 0.3333333333;

static InterfaceTable *ft;

struct A2B : public Unit
{
};

struct B2A : public Unit
{
};

struct B2UHJ : public Unit
{
	float m_wy1[12];
	float m_xy1[12];
	float m_yy1[12];
	float m_coefs[12];
};

struct UHJ2B : public Unit
{
	float m_lsy1[12];
	float m_rsy1[12];
	float m_coefs[12];

};

struct BFEncode1 : public Unit
{
	float m_azimuth, m_elevation, m_rho, m_level, m_W_amp, m_X_amp, m_Y_amp, m_Z_amp;
};

struct BFEncodeSter : public Unit
{
	float m_azimuth, m_width, m_elevation, m_rho, m_level, m_W_ampL, m_X_ampL, m_Y_ampL, m_Z_ampL, m_W_ampR, m_X_ampR, m_Y_ampR, m_Z_ampR;
};

struct BFEncode2 : public Unit
{
	float m_point_x, m_point_y, m_elevation, m_level, m_W_amp, m_X_amp, m_Y_amp, m_Z_amp;
}; 

struct FMHEncode1 : public Unit
{
	float m_azimuth, m_elevation, m_rho, m_level, m_W_amp, m_X_amp, m_Y_amp, m_Z_amp, m_R_amp, m_S_amp, m_T_amp, m_U_amp, m_V_amp;
};

struct FMHEncode2 : public Unit
{
	float m_point_x, m_point_y, m_elevation, m_rho, m_level, m_W_amp, m_X_amp, m_Y_amp, m_Z_amp, m_R_amp, m_S_amp, m_T_amp, m_U_amp, m_V_amp;
};

struct BFDecode1 : public Unit
{
	float m_cosa, m_sina, m_sinb;
};

struct BFManipulate : public Unit
{
	float m_rotate, m_tilt, m_tumble;
};

struct B2Ster : public Unit
{
};

extern "C"
{
	void load(InterfaceTable *inTable);

	void A2B_next(A2B *unit, int inNumSamples);
	void A2B_Ctor(A2B* unit);
	
	// B2A and A2B are the same transformation... B2A will call A2B_next
	void B2A_Ctor(A2B* unit);

	void B2UHJ_next(B2UHJ *unit, int inNumSamples);
	void B2UHJ_Ctor(B2UHJ* unit);
	
	void UHJ2B_next(UHJ2B *unit, int inNumSamples);
	void UHJ2B_Ctor(UHJ2B* unit);
	
	void BFEncode1_next_kkk(BFEncode1 *unit, int inNumSamples);
	void BFEncode1_next_aaa(BFEncode1 *unit, int inNumSamples);
	void BFEncode1_next_akk(BFEncode1 *unit, int inNumSamples);
	void BFEncode1_Ctor(BFEncode1* unit);

	void BFEncode2_next(BFEncode2 *unit, int inNumSamples);
	void BFEncode2_Ctor(BFEncode2* unit); 

	void BFEncodeSter_next(BFEncodeSter *unit, int inNumSamples);
	void BFEncodeSter_Ctor(BFEncodeSter* unit);

	void FMHEncode1_next(FMHEncode1 *unit, int inNumSamples);
	void FMHEncode1_Ctor(FMHEncode1* unit);

	void FMHEncode2_next(FMHEncode2 *unit, int inNumSamples);
	void FMHEncode2_Ctor(FMHEncode2* unit);
	
	void BFDecode1_next(BFDecode1 *unit, int inNumSamples);
	void BFDecode1_Ctor(BFDecode1* unit);

	void BFManipulate_next(BFManipulate *unit, int inNumSamples);
	void BFManipulate_Ctor(BFManipulate* unit);

	void B2Ster_next(B2Ster *unit, int inNumSamples);
	void B2Ster_Ctor(B2Ster* unit);
	
}



//////////////////////////////////////////////////////////////////////////////////////////////////
///////	    Basic Ambisonic UGens   //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// A2B and B2A conversions
/* See Ambisonics Ugens (ATK) */
void A2B_Ctor(A2B *unit)
{
	SETCALC(A2B_next);
	A2B_next(unit, 1);
}

void B2A_Ctor(A2B *unit)
{
	SETCALC(A2B_next);
	A2B_next(unit, 1);
}

// four channels in, four channels out... 
// regardless... call in w, x, y, z and out a, b, c, d
void A2B_next(A2B *unit, int inNumSamples)
{       
	float *Aout = ZOUT(0);
	float *Bout = ZOUT(1);
	float *Cout = ZOUT(2);
	float *Dout = ZOUT(3);
	
	float *Win = ZIN(0);
	float *Xin = ZIN(1);
	float *Yin = ZIN(2);
	float *Zin = ZIN(3);

	float lf, rf, rr, lr;
	float oneHalf = 0.5;
	
	LOOP(inNumSamples,
	    lf = ZXP(Win) * oneHalf;
	    rf = ZXP(Xin) * oneHalf;
	    lr = ZXP(Yin) * oneHalf;
	    rr = ZXP(Zin) * oneHalf;
	    
	    ZXP(Aout) = lf + rf + lr + rr;
	    ZXP(Bout) = lf + rf - lr - rr;
	    ZXP(Cout) = lf - rf + lr - rr;
	    ZXP(Dout) = lf - rf - lr + rr;
	    );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// B to UHJ - transform a 3 channel 2d ambisonic signal into a 2 channel UHJ
//
//		L = 0.5 * (0.9397*W + 0.1856*X - j*0.342*W + j*0.5099*X + 0.655*Y)
//		R = 0.5 * (0.9397*W+ 0.1856*X + j*0.342*W - j*0.5099*X - 0.655*Y)
//
// Where j  is a 90 shift

void B2UHJ_Ctor(B2UHJ *unit)
{
    SETCALC(B2UHJ_next);
    float gamconst = (15.0 * pi) / SAMPLERATE;
    float gamma01 = gamconst * 0.3609;
    float gamma02 = gamconst * 2.7412;
    float gamma03 = gamconst * 11.1573;
    float gamma04 = gamconst * 44.7581;
    float gamma05 = gamconst * 179.6242;
    float gamma06 = gamconst * 798.4578;
    float gamma07 = gamconst * 1.2524;
    float gamma08 = gamconst * 5.5671;
    float gamma09 = gamconst * 22.3423;
    float gamma10 = gamconst * 89.6271;
    float gamma11 = gamconst * 364.7914;
    float gamma12 = gamconst * 2770.1114;
    unit->m_coefs[0] = (gamma01 - 1) / (gamma01 + 1);
    unit->m_coefs[1] = (gamma02 - 1) / (gamma02 + 1);
    unit->m_coefs[2] = (gamma03 - 1) / (gamma03 + 1);
    unit->m_coefs[3] = (gamma04 - 1) / (gamma04 + 1);
    unit->m_coefs[4] = (gamma05 - 1) / (gamma05 + 1);
    unit->m_coefs[5] = (gamma06 - 1) / (gamma06 + 1);
    unit->m_coefs[6] = (gamma07 - 1) / (gamma07 + 1);
    unit->m_coefs[7] = (gamma08 - 1) / (gamma08 + 1);
    unit->m_coefs[8] = (gamma09 - 1) / (gamma09 + 1);
    unit->m_coefs[9] = (gamma10 - 1) / (gamma10 + 1);
    unit->m_coefs[10] = (gamma11 - 1) / (gamma11 + 1);
    unit->m_coefs[11] = (gamma12 - 1) / (gamma12 + 1);
    // clear out the storage variables for the filters
    for(int i = 0; i < 12; ++i){
	unit->m_wy1[i] = 0.0;
	unit->m_xy1[i] = 0.0;
	unit->m_yy1[i] = 0.0;
	}
    ClearUnitOutputs(unit, 1);
}

void B2UHJ_next(B2UHJ *unit, int inNumSamples)
{
    float *Win = IN(0);
    float *Xin = IN(1);
    float *Yin = IN(2);
    float *lsout = OUT(0);
    float *rsout = OUT(1);

    float wy1[12];
    float xy1[12];
    float yy1[12];

    // each filter's last sample
    for(int i = 0; i < 12; ++i){
	wy1[i] = unit->m_wy1[i];
	xy1[i] = unit->m_xy1[i];
	yy1[i] = unit->m_yy1[i];
	}

    for (int i = 0; i < inNumSamples; ++i){
	float wsig, w, wj, xsig, x, xj, ysig, y;
	
	float way1, way2, way3, way4, way5, way6;
	float way7, way8, way9, way10, way11, way12;

	float xay1, xay2, xay3, xay4, xay5, xay6;
	float xay7, xay8, xay9, xay10, xay11, xay12;    

	float yay1, yay2, yay3, yay4, yay5, yay6;
	
	float wy0_1, wy0_2, wy0_3, wy0_4, wy0_5, wy0_6;
	float wy0_7, wy0_8, wy0_9, wy0_10, wy0_11, wy0_12;

	float xy0_1, xy0_2, xy0_3, xy0_4, xy0_5, xy0_6;
	float xy0_7, xy0_8, xy0_9, xy0_10, xy0_11, xy0_12;    

	float yy0_1, yy0_2, yy0_3, yy0_4, yy0_5, yy0_6;
	
	wsig = Win[i];
	xsig = Xin[i];
	ysig = Yin[i];
	
	// pull out some values with a Shift90 - 6 cascading First order sections
	wy0_1 = wsig - (unit->m_coefs[0]) * wy1[0];
	way1 = unit->m_coefs[0] * wy0_1 + 1 * wy1[0]; 
	wy1[0] = wy0_1;
	wy0_2 = way1 - (unit->m_coefs[1]) * wy1[1];
	way2 = unit->m_coefs[1] * wy0_2 + 1 * wy1[1]; 
	wy1[1] = wy0_2;
	wy0_3 = way2 - (unit->m_coefs[2]) * wy1[2];
	way3 = unit->m_coefs[2] * wy0_3 + 1 * wy1[2]; 
	wy1[2] = wy0_3;	
	wy0_4 = way3 - (unit->m_coefs[3]) * wy1[3];
	way4 = unit->m_coefs[3] *  wy0_4 + 1 * wy1[3]; 
	wy1[3] = wy0_4;
	wy0_5 = way4 - (unit->m_coefs[4]) * wy1[4];
	way5 = unit->m_coefs[4] * wy0_5 + 1 * wy1[4]; 
	wy1[4] = wy0_5;	
	wy0_6 = way5 - (unit->m_coefs[5]) * wy1[5];
	w = way6 = unit->m_coefs[5] * wy0_6 + 1 * wy1[5]; 
	wy1[5] = wy0_6;
	
	wy0_7 = wsig - (unit->m_coefs[6]) * wy1[6];
	way7 = unit->m_coefs[6] * wy0_7 + 1 * wy1[6]; 
	wy1[6] = wy0_7;
	wy0_8 = way7 - (unit->m_coefs[7]) * wy1[7];
	way8 = unit->m_coefs[7] * wy0_8 + 1 * wy1[7]; 
	wy1[7] = wy0_8;
	wy0_9 = way8 - (unit->m_coefs[8]) * wy1[8];
	way9 = unit->m_coefs[8] * wy0_9 + 1 * wy1[8]; 
	wy1[8] = wy0_9;
	wy0_10 = way9 - (unit->m_coefs[9]) * wy1[9];
	way10 = unit->m_coefs[9] * wy0_10 + 1 * wy1[9]; 
	wy1[9] = wy0_10;
	wy0_11 = way10 - (unit->m_coefs[10]) * wy1[10];
	way11 = unit->m_coefs[10] * wy0_11  + 1 * wy1[10]; 
	wy1[10] = wy0_11;
	wy0_12 = way11 - (unit->m_coefs[11]) * wy1[11];
	wj = way12 = unit->m_coefs[11] * wy0_12 + 1 * wy1[11]; 
	wy1[11] = wy0_12;
	
	xy0_1 = xsig - (unit->m_coefs[0]) * xy1[0];
	xay1 = unit->m_coefs[0] * xy0_1 + 1 * xy1[0]; 
	xy1[0] = xy0_1;
	xy0_2 = xay1 - (unit->m_coefs[1]) * xy1[1];
	xay2 = unit->m_coefs[1] * xy0_2 + 1 * xy1[1]; 
	xy1[1] = xy0_2;
	xy0_3 = xay2 - (unit->m_coefs[2]) * xy1[2];
	xay3 = unit->m_coefs[2] * xy0_3 + 1 * xy1[2]; 
	xy1[2] = xy0_3;	
	xy0_4 = xay3 - (unit->m_coefs[3]) * xy1[3];
	xay4 = unit->m_coefs[3] * xy0_4 + 1 * xy1[3]; 
	xy1[3] = xy0_4;
	xy0_5 = xay4 - (unit->m_coefs[4]) * xy1[4];
	xay5 = unit->m_coefs[4] * xy0_5 + 1 * xy1[4]; 
	xy1[4] = xy0_5;
	xy0_6 = xay5 - (unit->m_coefs[5]) * xy1[5];
	x = xay6 = unit->m_coefs[5] * xy0_6 + 1 * xy1[5]; 
	xy1[5] = xy0_6;
	
	xy0_7 = xsig - (unit->m_coefs[6]) * xy1[6];
	xay7 = unit->m_coefs[6] * xy0_7 + 1 * xy1[6]; 
	xy1[6] = xy0_7;
	xy0_8 = xay7 - (unit->m_coefs[7]) * xy1[7];
	xay8 = unit->m_coefs[7] * xy0_8 + 1 * xy1[7]; 
	xy1[7] = xy0_8;
	xy0_9 = xay8 - (unit->m_coefs[8]) * xy1[8];
	xay9 = unit->m_coefs[8] * xy0_9 + 1 * xy1[8]; 
	xy1[8] = xy0_9;
	xy0_10 = xay9 - (unit->m_coefs[9]) * xy1[9];
	xay10 = unit->m_coefs[9] * xy0_10  + 1 * xy1[9]; 
	xy1[9] = xy0_10;
	xy0_11 = xay10 - (unit->m_coefs[10]) * xy1[10];
	xay11 = unit->m_coefs[10] * xy0_11 + 1 * xy1[10]; 
	xy1[10] = xy0_11;
	xy0_12 = xay11 - (unit->m_coefs[11]) * xy1[11];
	xj = xay12 = unit->m_coefs[11] * xy0_12 + 1 * xy1[11]; 
	xy1[11] = xy0_12;
	
	yy0_1 = ysig - (unit->m_coefs[0]) * yy1[0];
	yay1 = unit->m_coefs[0] * yy0_1 + 1 * yy1[0]; 
	yy1[0] = yy0_1;
	yy0_2 = yay1 - (unit->m_coefs[1]) * yy1[1];
	yay2 = unit->m_coefs[1] * yy0_2 + 1 * yy1[1]; 
	yy1[1] = yy0_2;
	yy0_3 = yay2 - (unit->m_coefs[2]) * yy1[2];
	yay3 = unit->m_coefs[2] * yy0_3 + 1 * yy1[2]; 
	yy1[2] = yy0_3;
	yy0_4 = yay3 - (unit->m_coefs[3]) * yy1[3];
	yay4 = unit->m_coefs[3] * yy0_4 + 1 * yy1[3]; 
	yy1[3] = yy0_4;
	yy0_5 = yay4 - (unit->m_coefs[4]) * yy1[4];
	yay5 = unit->m_coefs[4] * yy0_5 + 1 * yy1[4]; 
	yy1[4] = yy0_5;
	yy0_6 = yay5 - (unit->m_coefs[5]) * yy1[5];
	y = yay6 = unit->m_coefs[5] * yy0_6 + 1 * yy1[5]; 
	yy1[5] = yy0_6;	
		
	w = 0.9397 * w;
	wj = 0.342 * wj;
	x = 0.1856 * x;
	xj = 0.5099 * xj;
	y = y * 0.655;
		
	lsout[i] = ((((w + x) - wj) + xj) + y) * 0.5;
	rsout[i] = ((((w + x) + wj) - xj) - y) * 0.5;
	}
	
    for(int i = 0; i < 12; ++i){
	unit->m_wy1[i] = zapgremlins(wy1[i]);
	unit->m_xy1[i] = zapgremlins(xy1[i]);
	unit->m_yy1[i] = zapgremlins(yy1[i]);
	}

}


void UHJ2B_Ctor(UHJ2B *unit)
{
    SETCALC(UHJ2B_next);
    float gamconst = (15.0 * pi) / SAMPLERATE;
    float gamma01 = gamconst * 0.3609;
    float gamma02 = gamconst * 2.7412;
    float gamma03 = gamconst * 11.1573;
    float gamma04 = gamconst * 44.7581;
    float gamma05 = gamconst * 179.6242;
    float gamma06 = gamconst * 798.4578;
    float gamma07 = gamconst * 1.2524;
    float gamma08 = gamconst * 5.5671;
    float gamma09 = gamconst * 22.3423;
    float gamma10 = gamconst * 89.6271;
    float gamma11 = gamconst * 364.7914;
    float gamma12 = gamconst * 2770.1114;
    unit->m_coefs[0] = (gamma01 - 1) / (gamma01 + 1);
    unit->m_coefs[1] = (gamma02 - 1) / (gamma02 + 1);
    unit->m_coefs[2] = (gamma03 - 1) / (gamma03 + 1);
    unit->m_coefs[3] = (gamma04 - 1) / (gamma04 + 1);
    unit->m_coefs[4] = (gamma05 - 1) / (gamma05 + 1);
    unit->m_coefs[5] = (gamma06 - 1) / (gamma06 + 1);
    unit->m_coefs[6] = (gamma07 - 1) / (gamma07 + 1);
    unit->m_coefs[7] = (gamma08 - 1) / (gamma08 + 1);
    unit->m_coefs[8] = (gamma09 - 1) / (gamma09 + 1);
    unit->m_coefs[9] = (gamma10 - 1) / (gamma10 + 1);
    unit->m_coefs[10] = (gamma11 - 1) / (gamma11 + 1);
    unit->m_coefs[11] = (gamma12 - 1) / (gamma12 + 1);
    // clear out the storage variables for the filters
    for(int i = 0; i < 12; ++i){
	unit->m_lsy1[i] = 0.0;
	unit->m_rsy1[i] = 0.0;}
    UHJ2B_next(unit, 1);

}

void UHJ2B_next(UHJ2B *unit, int inNumSamples)
{
    float *Lsin = IN(0);
    float *Rsin = IN(1);
    float *Wout = OUT(0);
    float *Xout = OUT(1);
    float *Yout = OUT(2);

    float lsy1[12];
    float rsy1[12];
    float lsig, l, lj, rsig, r, rj;
	
    float lay1, lay2, lay3, lay4, lay5, lay6;
    float lay7, lay8, lay9, lay10, lay11, lay12;

    float ray1, ray2, ray3, ray4, ray5, ray6;
    float ray7, ray8, ray9, ray10, ray11, ray12;    
	
    float ly0_1, ly0_2, ly0_3, ly0_4, ly0_5, ly0_6;
    float ly0_7, ly0_8, ly0_9, ly0_10, ly0_11, ly0_12;

    float ry0_1, ry0_2, ry0_3, ry0_4, ry0_5, ry0_6;
    float ry0_7, ry0_8, ry0_9, ry0_10, ry0_11, ry0_12;    

    // each filter's last sample
    for(int i = 0; i < 12; ++i){
	lsy1[i] = unit->m_lsy1[i];
	rsy1[i] = unit->m_rsy1[i];
	}

    for (int i = 0; i < inNumSamples; ++i){

	lsig = Lsin[i];
	rsig = Rsin[i];
	
	// pull out some values with a Shift90 - 6 cascading First order sections
	ly0_1 = lsig - (unit->m_coefs[0]) * lsy1[0];
	lay1 = unit->m_coefs[0] * ly0_1 + 1 * lsy1[0]; 
	lsy1[0] = ly0_1;
	ly0_2 = lay1 - (unit->m_coefs[1]) * lsy1[1];
	lay2 = unit->m_coefs[1] * ly0_2 + 1 * lsy1[1]; 
	lsy1[1] = ly0_2;
	ly0_3 = lay2 - (unit->m_coefs[2]) * lsy1[2];
	lay3 = unit->m_coefs[2] * ly0_3 + 1 * lsy1[2]; 
	lsy1[2] = ly0_3;	
	ly0_4 = lay3 - (unit->m_coefs[3]) * lsy1[3];
	lay4 = unit->m_coefs[3] *  ly0_4 + 1 * lsy1[3]; 
	lsy1[3] = ly0_4;
	ly0_5 = lay4 - (unit->m_coefs[4]) * lsy1[4];
	lay5 = unit->m_coefs[4] * ly0_5 + 1 * lsy1[4]; 
	lsy1[4] = ly0_5;	
	ly0_6 = lay5 - (unit->m_coefs[5]) * lsy1[5];
	l = lay6 = unit->m_coefs[5] * ly0_6 + 1 * lsy1[5]; 
	lsy1[5] = ly0_6;
	
	ly0_7 = lsig - (unit->m_coefs[6]) * lsy1[6];
	lay7 = unit->m_coefs[6] * ly0_7 + 1 * lsy1[6]; 
	lsy1[6] = ly0_7;
	ly0_8 = lay7 - (unit->m_coefs[7]) * lsy1[7];
	lay8 = unit->m_coefs[7] * ly0_8 + 1 * lsy1[7]; 
	lsy1[7] = ly0_8;
	ly0_9 = lay8 - (unit->m_coefs[8]) * lsy1[8];
	lay9 = unit->m_coefs[8] * ly0_9 + 1 * lsy1[8]; 
	lsy1[8] = ly0_9;
	ly0_10 = lay9 - (unit->m_coefs[9]) * lsy1[9];
	lay10 = unit->m_coefs[9] * ly0_10 + 1 * lsy1[9]; 
	lsy1[9] = ly0_10;
	ly0_11 = lay10 - (unit->m_coefs[10]) * lsy1[10];
	lay11 = unit->m_coefs[10] * ly0_11  + 1 * lsy1[10]; 
	lsy1[10] = ly0_11;
	ly0_12 = lay11 - (unit->m_coefs[11]) * lsy1[11];
	lj = lay12 = unit->m_coefs[11] * ly0_12 + 1 * lsy1[11]; 
	lsy1[11] = ly0_12;

	ry0_1 = rsig - (unit->m_coefs[0]) * rsy1[0];
	ray1 = unit->m_coefs[0] * ry0_1 + 1 * rsy1[0]; 
	rsy1[0] = ry0_1;
	ry0_2 = ray1 - (unit->m_coefs[1]) * rsy1[1];
	ray2 = unit->m_coefs[1] * ry0_2 + 1 * rsy1[1]; 
	rsy1[1] = ry0_2;
	ry0_3 = ray2 - (unit->m_coefs[2]) * rsy1[2];
	ray3 = unit->m_coefs[2] * ry0_3 + 1 * rsy1[2]; 
	rsy1[2] = ry0_3;	
	ry0_4 = ray3 - (unit->m_coefs[3]) * rsy1[3];
	ray4 = unit->m_coefs[3] *  ry0_4 + 1 * rsy1[3]; 
	rsy1[3] = ry0_4;
	ry0_5 = ray4 - (unit->m_coefs[4]) * rsy1[4];
	ray5 = unit->m_coefs[4] * ry0_5 + 1 * rsy1[4]; 
	rsy1[4] = ry0_5;	
	ry0_6 = ray5 - (unit->m_coefs[5]) * rsy1[5];
	r = ray6 = unit->m_coefs[5] * ry0_6 + 1 * rsy1[5]; 
	rsy1[5] = ry0_6;
	
	ry0_7 = rsig - (unit->m_coefs[6]) * rsy1[6];
	ray7 = unit->m_coefs[6] * ry0_7 + 1 * rsy1[6]; 
	rsy1[6] = ry0_7;
	ry0_8 = ray7 - (unit->m_coefs[7]) * rsy1[7];
	ray8 = unit->m_coefs[7] * ry0_8 + 1 * rsy1[7]; 
	rsy1[7] = ry0_8;
	ry0_9 = ray8 - (unit->m_coefs[8]) * rsy1[8];
	ray9 = unit->m_coefs[8] * ry0_9 + 1 * rsy1[8]; 
	rsy1[8] = ry0_9;
	ry0_10 = ray9 - (unit->m_coefs[9]) * rsy1[9];
	ray10 = unit->m_coefs[9] * ry0_10 + 1 * rsy1[9]; 
	rsy1[9] = ry0_10;
	ry0_11 = ray10 - (unit->m_coefs[10]) * rsy1[10];
	ray11 = unit->m_coefs[10] * ry0_11  + 1 * rsy1[10]; 
	rsy1[10] = ry0_11;
	ry0_12 = ray11 - (unit->m_coefs[11]) * rsy1[11];
	rj = ray12 = unit->m_coefs[11] * ry0_12 + 1 * rsy1[11]; 
	rsy1[11] = ry0_12;	
		
	Wout[i] = ((0.982 * l) + (0.982 * r) + (0.164 * lj) - (0.164 * rj)) * 0.5;
	Xout[i] = ((0.419 * l) + (0.419 * r) - (0.828 * lj) + (0.828 * rj)) * 0.5;
	Yout[i] = ((0.763 * l) - (0.763 * r) + (0.385 * lj) + (0.385 * rj)) * 0.5;
	}
	
    for(int i = 0; i < 12; ++i){
	unit->m_lsy1[i] = zapgremlins(lsy1[i]);
	unit->m_rsy1[i] = zapgremlins(rsy1[i]);
	}

}


////////////////////////////////////////////////////////////////////////

void BFManipulate_Ctor(BFManipulate *unit)
{
	SETCALC(BFManipulate_next);
	unit->m_rotate = ZIN0(4);
	unit->m_tilt = ZIN0(5);
	unit->m_tumble = ZIN0(6);
	BFManipulate_next(unit, 1);
}

void BFManipulate_next(BFManipulate *unit, int inNumSamples)
{       
	float *Wout = ZOUT(0);
	float *Xout = ZOUT(1);
	float *Yout = ZOUT(2);
	float *Zout = ZOUT(3);
	
	float *Win = ZIN(0);
	float *Xin = ZIN(1);
	float *Yin = ZIN(2);
	float *Zin = ZIN(3);
	
	float w, x, y, z;
	float sina, sinb, sinc, cosa, cosb, cosc;
	
	float rotate = unit->m_rotate;
	float tilt = unit->m_tilt;
	float tumble = unit->m_tumble;
	
	float endrotate = ZIN0(4);
	float endtilt = ZIN0(5);
	float endtumble = ZIN0(6);
	
	float rotate_slope = CALCSLOPE(endrotate, rotate);
	float tilt_slope = CALCSLOPE(endtilt, tilt);
	float tumble_slope = CALCSLOPE(endtumble, tumble);

	LOOP(inNumSamples,
	    	sina = sin(rotate);
		sinb = sin(tilt);
		sinc = sin(tumble);
	
		cosa = cos(rotate);
		cosb = cos(tilt);
		cosc = cos(tumble);
		
		w = ZXP(Win);
		x = ZXP(Xin);
		y = ZXP(Yin);
		z = ZXP(Zin);
		
		ZXP(Wout) = w;
		ZXP(Xout) = ((x * cosa * cosc) - (y * sina) - (z * sinc));
		ZXP(Yout) = ((x * sina) + (y * cosa * cosb) - (z * sinb));
		ZXP(Zout) = ((x * sinc) + (y * sinb) + (z * cosb * cosc));
		
		rotate += rotate_slope;
		tilt += tilt_slope;
		tumble += tumble_slope;
		);
		
	unit->m_rotate = endrotate;
	unit->m_tilt = endtilt;
	unit->m_tumble = endtumble;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void B2Ster_Ctor(B2Ster *unit)
{
    SETCALC(B2Ster_next);
    B2Ster_next(unit, 1);
}

// http://www.cyber.rdg.ac.uk/P.Sharkey/WWW/icdvrat/WWW96/Papers/keating.htm
// L = W + 0.35X + 0.61Y
// R = W + 0.35X - 0.61Y
		
void B2Ster_next(B2Ster *unit, int inNumSamples)
{
    float *Win = IN(0);
    float *Xin = IN(1);
    float *Yin = IN(2);
    float *lout = OUT(0);
    float *rout = OUT(1);

    float w, x, y;
    for(int i = 0; i < inNumSamples; ++i){
	w = Win[i];
	x = Xin[i] * 0.35;
	y = Yin[i] * 0.61;
	
	lout[i] = w + x + y;
	rout[i] = w + x - y;
    }
}

///////////////////////////////////////////////////////////////////////////////////

void BFEncode2_Ctor(BFEncode2 *unit)
{
	SETCALC(BFEncode2_next);
	float sinint, cosint, azimuth, rho;
	float point_x = unit->m_point_x = IN0(1);
	float point_y = unit->m_point_y = IN0(2);
	float elevation = unit->m_elevation = IN0(3);
	float level = unit->m_level = IN0(4);

	azimuth = atan2(point_x, point_y);
	rho = hypot(point_x, point_y);
	
	float sina = sin(azimuth);
	float sinb = sin(elevation);
	
	float cosa = cos(azimuth);
	float cosb = cos(elevation);
	
	if(rho >= 1) {
		float intrho = 1 / (pow(rho, 1.5));
		sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; // pow(rho, (float)1.5);
		cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho; // / pow(rho, (float)1.5);
		}
	    else 
		{
		sinint = rsqrt2 * (sin(0.78539816339745 * rho));
		cosint = rsqrt2 * (cos(0.78539816339745 * rho));
		};

	float levelsinint = level * sinint;

	unit->m_W_amp = level * cosint;
	unit->m_X_amp = cosa * cosb * levelsinint;
	unit->m_Y_amp = sina * cosb * levelsinint;
	unit->m_Z_amp = sinb * levelsinint;

	BFEncode2_next(unit, 1);
}

void BFEncode2_next(BFEncode2 *unit, int inNumSamples)
{       
	float sinint, cosint, azimuth, rho, z;
	float *Wout = OUT(0);
	float *Xout = OUT(1);
	float *Yout = OUT(2);
	float *Zout = OUT(3);
	
	float *in = IN(0);
	float point_x = IN0(1);
	float point_y = IN0(2);
	float elevation = IN0(3);
	float level = IN0(4);

	azimuth = atan2(point_x, point_y);
	rho = hypot(point_x, point_y);

	float W_amp = unit->m_W_amp;
	float X_amp = unit->m_X_amp;
	float Y_amp = unit->m_Y_amp;
	float Z_amp = unit->m_Z_amp;

	if (point_x != unit->m_point_x || point_y != unit->m_point_y ||elevation != unit->m_elevation || level != unit->m_level) {
		unit->m_point_x = point_x;
		unit->m_point_y = point_y;
		unit->m_elevation = elevation;
		unit->m_level = level;

		float sina =  sin(azimuth);
		float sinb =  sin(elevation);

		float cosa = cos(azimuth);
		float cosb = cos(elevation);
		if(rho >= 1) {
			float intrho = 1 / (pow(rho, 1.5));
			sinint = (rsqrt2 * (sin(0.78539816339745 * 1.0))) * intrho; // pow(rho, (float)1.5);
			cosint =  (rsqrt2 * (cos(0.78539816339745 * 1.0))) * intrho; // / pow(rho, (float)1.5);
		    }
		    else 
			{sinint = rsqrt2 * (sin(0.78539816339745 * rho));
			cosint = rsqrt2 * (cos(0.78539816339745 * rho));
			};
		
		float levelsinint = level * sinint;

		// vary w according to x, y and z
//		float next_W_amp = rsqrt2 * level * cosint;
		float next_W_amp = level * cosint;
		float next_X_amp = cosa * cosb * levelsinint;
		float next_Y_amp = sina * cosb * levelsinint;
		float next_Z_amp = sinb * levelsinint;
		
		float W_slope = CALCSLOPE(next_W_amp, W_amp);
		float X_slope = CALCSLOPE(next_X_amp, X_amp);
		float Y_slope = CALCSLOPE(next_Y_amp, Y_amp);
		float Z_slope = CALCSLOPE(next_Z_amp, Z_amp);
		
		for(int i = 0; i < inNumSamples; i++){ 
			z = in[i];
			// vary w according to x, y and z
			Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp)))));
			Xout[i] = z * X_amp;
			Yout[i] = z * Y_amp;
			Zout[i] = z * Z_amp;
			W_amp += W_slope;
			X_amp += X_slope;
			Y_amp += Y_slope;
			Z_amp += Z_slope;
		    }
		unit->m_W_amp = W_amp;
		unit->m_X_amp = X_amp;
		unit->m_Y_amp = Y_amp;
		unit->m_Z_amp = Z_amp;
	} else {
		for(int i = 0; i < inNumSamples; i++){ 
			z = in[i];
			// vary w according to x, y and z
			Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp)))));
			Xout[i] = z * X_amp;
			Yout[i] = z * Y_amp;
			Zout[i] = z * Z_amp;
		    };
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void BFEncode1_Ctor(BFEncode1 *unit)
{

    if (INRATE(1) == calc_FullRate) {
	if (INRATE(2) == calc_FullRate) {
	    if (INRATE(3) == calc_FullRate) {
		SETCALC(BFEncode1_next_aaa);
		} else {
		SETCALC(BFEncode1_next_akk); // aak
		}
	    } else {
	    if (INRATE(3) == calc_FullRate) {
		SETCALC(BFEncode1_next_akk); //aka
		} else {
		SETCALC(BFEncode1_next_akk); // akk
		}
	    }
	} else {
	if (INRATE(2) == calc_FullRate) {
	    if (INRATE(3) == calc_FullRate) {
		SETCALC(BFEncode1_next_kkk); // kaa
		} else {
		SETCALC(BFEncode1_next_kkk); // kak
		}
	    } else {
	    if (INRATE(3) == calc_FullRate) {
		SETCALC(BFEncode1_next_kkk); //kka
		} else {
		SETCALC(BFEncode1_next_kkk); // kkk
		}
	    }
	}
	
	float sinint, cosint;
	float azimuth = unit->m_azimuth = IN0(1);
	float elevation = unit->m_elevation = IN0(2);
 	float rho = unit->m_rho = IN0(3);
	float level = unit->m_level = IN0(4);

	float sina = sin(azimuth);
	float sinb = sin(elevation);
	
	float cosa = cos(azimuth);
	float cosb = cos(elevation);
	
	if(rho >= 1) {
		float intrho = 1 / pow(rho, 1.5);
		sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
		cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);
	    else 
		{
		sinint = rsqrt2 * (sin(0.78539816339745 * rho));
		cosint = rsqrt2 * (cos(0.78539816339745 * rho));
		};

	float levelsinint = level * sinint;
	
	unit->m_W_amp = level * cosint;
	unit->m_X_amp = cosa * cosb * levelsinint;
	unit->m_Y_amp = sina * cosb * levelsinint;
	unit->m_Z_amp = sinb * levelsinint;

	BFEncode1_next_kkk(unit, 1);
}

void BFEncode1_next_kkk(BFEncode1 *unit, int inNumSamples)
{       
	float sinint, cosint, z = 0.0;
	float *Wout = OUT(0);
	float *Xout = OUT(1);
	float *Yout = OUT(2);
	float *Zout = OUT(3);
	
	float *in = IN(0);
	float azimuth = IN0(1);
	float elevation = IN0(2);
	float rho = IN0(3);
	float level = IN0(4);

	float W_amp = unit->m_W_amp;
	float X_amp = unit->m_X_amp;
	float Y_amp = unit->m_Y_amp;
	float Z_amp = unit->m_Z_amp;

	if (azimuth != unit->m_azimuth || rho != unit->m_rho || elevation != unit->m_elevation || level != unit->m_level) {
		unit->m_azimuth = azimuth;
		unit->m_elevation = elevation;
		unit->m_level = level;
		unit->m_rho = rho;
		
		float sina = sin(azimuth);
		float sinb = sin(elevation);
		
		float cosa = cos(azimuth);
		float cosb = cos(elevation);
		
		if(rho >= 1) {
			float intrho = 1 / pow(rho, 1.5);
			sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
			cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
		    else { 
			sinint = rsqrt2 * (sin(0.78539816339745 * rho));
			cosint = rsqrt2 * (cos(0.78539816339745 * rho));
			};
		float levelsinint = level * sinint;
		
		float next_W_amp = level * cosint;
		float next_X_amp = cosa * cosb * levelsinint;
		float next_Y_amp = sina * cosb * levelsinint;
		float next_Z_amp = sinb * levelsinint;
		
		float W_slope = CALCSLOPE(next_W_amp, W_amp);
		float X_slope = CALCSLOPE(next_X_amp, X_amp);
		float Y_slope = CALCSLOPE(next_Y_amp, Y_amp);
		float Z_slope = CALCSLOPE(next_Z_amp, Z_amp);
		
		for(int i = 0; i < inNumSamples; i++){
			z = in[i];
			// vary w according to x, y and z
			Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp)))));			
			Xout[i] = z * X_amp;
			Yout[i] = z * Y_amp;
			Zout[i] = z * Z_amp;
			W_amp += W_slope;
			X_amp += X_slope;
			Y_amp += Y_slope;
			Z_amp += Z_slope;
		    }
		unit->m_W_amp = W_amp;
		unit->m_X_amp = X_amp;
		unit->m_Y_amp = Y_amp;
		unit->m_Z_amp = Z_amp;
	} else {
	    for(int i = 0; i < inNumSamples; i++)
		{ 
		    z = in[i];
		    // vary w according to x, y and z
		    Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp)))));
		    Xout[i] = z * X_amp;
		    Yout[i] = z * Y_amp;
		    Zout[i] = z * Z_amp;
		}
	}
}


void BFEncode1_next_aaa(BFEncode1 *unit, int inNumSamples)
{       
	float sinint, cosint, z = 0.0;
	float *Wout = OUT(0);
	float *Xout = OUT(1);
	float *Yout = OUT(2);
	float *Zout = OUT(3);
	
	float *in = IN(0);
	float *azimuth = IN(1);
	float *elevation = IN(2);
	float *rho = IN(3);
	float newlevel = IN0(4);
	float level = unit->m_level;
	float levelslope = 0.f;
	
	if(newlevel != unit->m_level){
	    levelslope = CALCSLOPE(newlevel, unit->m_level);
	    }
	    
	for(int i = 0; i < inNumSamples; i++){
	    float thisaz = azimuth[i];
	    float thiselevation = elevation[i];
	    float thisrho = rho[i];
	    float sina = sin(thisaz);
	    float sinb = sin(thiselevation);
	    
	    float cosa = cos(thisaz);
	    float cosb = cos(thiselevation);
	    
	    if(thisrho >= 1) {
		    float intrho = 1 / pow(thisrho, 1.5);
		    sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
		    cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
		else { 
		    sinint = rsqrt2 * (sin(0.78539816339745 * thisrho));
		    cosint = rsqrt2 * (cos(0.78539816339745 * thisrho));
		    };
	    float levelsinint = level * sinint;
	    
	    float W_amp = level * cosint;
	    float X_amp = cosa * cosb * levelsinint;
	    float Y_amp = sina * cosb * levelsinint;
	    float Z_amp = sinb * levelsinint;
	
	    z = in[i];
	    // vary w according to x, y and z
	    Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp)))));			
	    Xout[i] = z * X_amp;
	    Yout[i] = z * Y_amp;
	    Zout[i] = z * Z_amp;
	    level += levelslope;

	}
	unit->m_level = level;
    }


void BFEncode1_next_akk(BFEncode1 *unit, int inNumSamples)
{       
	float sinint, cosint, z = 0.0;
	float *Wout = OUT(0);
	float *Xout = OUT(1);
	float *Yout = OUT(2);
	float *Zout = OUT(3);
	
	float *in = IN(0);
	float *azimuth = IN(1);
	float newelevation = IN0(2);
	float newrho = IN0(3);
	float newlevel = IN0(4);
	float elevation = unit->m_elevation;
	float rho = unit->m_rho;
	float level = unit->m_level;
	float elslope = 0.f;
	float rhoslope = 0.f;
	float levelslope = 0.f;
	
	if(newelevation != unit->m_elevation || newrho != unit->m_rho || newlevel != unit->m_level){
	    elslope = CALCSLOPE(newelevation, unit->m_elevation);
	    rhoslope = CALCSLOPE(newrho, unit->m_rho);
	    levelslope = CALCSLOPE(newlevel, unit->m_level);
	    }
	    
	for(int i = 0; i < inNumSamples; i++){
	    float thisaz = azimuth[i];
	    float sina = sin(thisaz);
	    float sinb = sin(elevation);
	    
	    float cosa = cos(thisaz);
	    float cosb = cos(elevation);
	    
	    if(rho >= 1) {
		    float intrho = 1 / pow(rho, 1.5);
		    sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
		    cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
		else { 
		    sinint = rsqrt2 * (sin(0.78539816339745 * rho));
		    cosint = rsqrt2 * (cos(0.78539816339745 * rho));
		    };
	    float levelsinint = level * sinint;
	    
	    float W_amp = level * cosint;
	    float X_amp = cosa * cosb * levelsinint;
	    float Y_amp = sina * cosb * levelsinint;
	    float Z_amp = sinb * levelsinint;
	
	    z = in[i];
	    // vary w according to x, y and z
	    Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp)))));			
	    Xout[i] = z * X_amp;
	    Yout[i] = z * Y_amp;
	    Zout[i] = z * Z_amp;
	    
	    elevation += elevation;
	    rho += rhoslope;
	    level += levelslope;

	}
	unit->m_elevation = elevation;
	unit->m_level = level;
	unit->m_rho = rho;
    }



////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

void BFEncodeSter_Ctor(BFEncodeSter *unit)
{
	SETCALC(BFEncodeSter_next);
	float sinint, cosint;
	float azimuth = unit->m_azimuth = IN0(2);
	float width = unit->m_width = IN0(3);
	float elevation = unit->m_elevation = IN0(4);
 	float rho = unit->m_rho = IN0(5);
	float level = unit->m_level = IN0(6);
	float width2 = width * 0.5;
	float azplus = azimuth + width2;
	float azminus = azimuth - width2;
	
	float sinal = sin(azplus);
	float sinar = sin(azminus);
	float sinb= sin(elevation);
	
	
	float cosar = cos(azplus);
	float cosal = cos(azminus);
	float cosb = cos(elevation);
	
	if(rho >= 1) {
		float intrho = 1 / pow(rho, 1.5);
		sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
		cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
	    else 
		{
		sinint = rsqrt2 * (sin(0.78539816339745 * rho));
		cosint = rsqrt2 * (cos(0.78539816339745 * rho));
		};
	float levelsinint = level * sinint;
	
	unit->m_W_ampL = level * cosint;
	unit->m_X_ampL = cosal * cosb * levelsinint;
	unit->m_Y_ampL = sinal * cosb * levelsinint;
	unit->m_Z_ampL = sinb * levelsinint;

	unit->m_W_ampR = level * cosint;
	unit->m_X_ampR = cosar * cosb * levelsinint;
	unit->m_Y_ampR = sinar * cosb * levelsinint;
	unit->m_Z_ampR = sinb * levelsinint;
	
	BFEncodeSter_next(unit, 1);
}

void BFEncodeSter_next(BFEncodeSter *unit, int inNumSamples)
{       
	float sinint, cosint, z, y = 0.0;
	float *Wout = OUT(0);
	float *Xout = OUT(1);
	float *Yout = OUT(2);
	float *Zout = OUT(3);
	
	float *l = IN(0);
	float *r = IN(1);
	float azimuth = IN0(2);
	float width = IN0(3);
	float elevation = IN0(4);
	float rho = IN0(5);
	float level = IN0(6);

	float W_ampL = unit->m_W_ampL;
	float X_ampL = unit->m_X_ampL;
	float Y_ampL = unit->m_Y_ampL;
	float Z_ampL = unit->m_Z_ampL;
	float W_ampR = unit->m_W_ampR;
	float X_ampR = unit->m_X_ampR;
	float Y_ampR = unit->m_Y_ampR;
	float Z_ampR = unit->m_Z_ampR;

	if (azimuth != unit->m_azimuth || width != unit->m_width || rho != unit->m_rho || elevation != unit->m_elevation || level != unit->m_level) {
		unit->m_azimuth = azimuth;
		unit->m_width = width;
		unit->m_elevation = elevation;
		unit->m_level = level;
		unit->m_rho = rho;
		
		float width2 = width * 0.5;
		float azplus = azimuth + width2;
		float azminus = azimuth - width2;
		float sinal = sin(azplus);
		float sinar = sin(azminus);
		float sinb = sin(elevation);
		
		float cosal = cos(azplus);
		float cosar = cos(azminus);
		float cosb = cos(elevation);
		
		if(rho >= 1) {
			float intrho = 1 / pow(rho, 1.5);
			sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
			cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
		    else { 
			sinint = rsqrt2 * (sin(0.78539816339745 * rho));
			cosint = rsqrt2 * (cos(0.78539816339745 * rho));
			};
		
		float levelsinint = level * sinint;
		
		float next_W_ampL = level * cosint;
		float next_X_ampL = cosal * cosb * levelsinint;
		float next_Y_ampL = sinal * cosb * levelsinint;
		float next_Z_ampL = sinb * levelsinint;
		
		float next_W_ampR = level * cosint;
		float next_X_ampR = cosar * cosb * levelsinint;
		float next_Y_ampR = sinar * cosb * levelsinint;
		float next_Z_ampR = sinb * levelsinint;
				
		float W_slopeL = CALCSLOPE(next_W_ampL, W_ampL);
		float X_slopeL = CALCSLOPE(next_X_ampL, X_ampL);
		float Y_slopeL = CALCSLOPE(next_Y_ampL, Y_ampL);
		float Z_slopeL = CALCSLOPE(next_Z_ampL, Z_ampL);

		float W_slopeR = CALCSLOPE(next_W_ampR, W_ampR);
		float X_slopeR = CALCSLOPE(next_X_ampR, X_ampR);
		float Y_slopeR = CALCSLOPE(next_Y_ampR, Y_ampR);
		float Z_slopeR = CALCSLOPE(next_Z_ampR, Z_ampR);
				
		for(int i = 0; i < inNumSamples; i++){
			y = l[i];
			z = r[i];
			// vary w according to x, y and z
			Wout[i] = (y * (W_ampL * (1 - (0.293 * ((X_ampL * X_ampL) + (Y_ampL * Y_ampL) + (Z_ampL * Z_ampL)))))) +
			    (z * (W_ampR * (1 - (0.293 * ((X_ampR * X_ampR) + (Y_ampR * Y_ampR) + (Z_ampR * Z_ampR))))));			
			Xout[i] = (y * X_ampL) + (z * X_ampR);
			Yout[i] = (y * Y_ampL) + (z * Y_ampR);
			Zout[i] = (y * Z_ampL) + (z * Z_ampR);
			W_ampL += W_slopeL;
			X_ampL += X_slopeL;
			Y_ampL += Y_slopeL;
			Z_ampL += Z_slopeL;
			W_ampR += W_slopeR;
			X_ampR += X_slopeR;
			Y_ampR += Y_slopeR;
			Z_ampR += Z_slopeR;
		    }
		unit->m_W_ampL = W_ampL;
		unit->m_X_ampL = X_ampL;
		unit->m_Y_ampL = Y_ampL;
		unit->m_Z_ampL = Z_ampL;
		unit->m_W_ampR = W_ampR;
		unit->m_X_ampR = X_ampR;
		unit->m_Y_ampR = Y_ampR;
		unit->m_Z_ampR = Z_ampR;
	} else {
	    for(int i = 0; i < inNumSamples; i++)
		{ 
		    y = l[i];
		    z = r[i];
		    // vary w according to x, y and z
		    Wout[i] = (y * (W_ampL * (1 - (0.293 * ((X_ampL * X_ampL) + (Y_ampL * Y_ampL) + (Z_ampL * Z_ampL)))))) +
			(z * (W_ampR * (1 - (0.293 * ((X_ampR * X_ampR) + (Y_ampR * Y_ampR) + (Z_ampR * Z_ampR))))));			
		    Xout[i] = (y * X_ampL) + (z * X_ampR);
		    Yout[i] = (y * Y_ampL) + (z * Y_ampR);
		    Zout[i] = (y * Z_ampL) + (z * Z_ampR);
		}
	}
}

void FMHEncode1_Ctor(FMHEncode1 *unit)
{
	SETCALC(FMHEncode1_next);
	float sinint, cosint;
	float azimuth = unit->m_azimuth = IN0(1);
	float elevation = unit->m_elevation = IN0(2);
 	float rho = unit->m_rho = IN0(3);
	float level = unit->m_level = IN0(4);
	
	float azimuthtimes2 = azimuth * 2.;
	float sina = sin(azimuth);
	float sinb = sin(elevation);
	
	float cosa = cos(azimuth);
	float cosb = cos(elevation);
	
	float cosbsq = cosb * cosb;
	
	// the whole rho thing just may not work... let's try anyways!
	if(rho >= 1) {
		float intrho = 1 / pow(rho, 1.5);
		sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
		cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
	    else 
		{
		sinint = rsqrt2 * (sin(0.78539816339745 * rho));
		cosint = rsqrt2 * (cos(0.78539816339745 * rho));
		};
	
	float levelsinint = level * sinint;
	
	unit->m_W_amp = level * cosint;
	unit->m_X_amp = cosa * cosb * levelsinint;
	unit->m_Y_amp = sina * cosb * levelsinint;
	unit->m_Z_amp = sinb * level * sinint;
	unit->m_R_amp = sqrt3div2 * cosbsq * cos(azimuthtimes2) * levelsinint;
	unit->m_S_amp = sqrt3div2 * cosbsq * sin(azimuthtimes2) * levelsinint; 
	unit->m_T_amp = sqrt3 * cosb * sinb * cosa * levelsinint;
	unit->m_U_amp = sqrt3 * cosb * sinb * sina * levelsinint;
	unit->m_V_amp = 0.5 * (3. * (sinb * sinb) - 1.) * levelsinint;
	
	FMHEncode1_next(unit, 1);
}

void FMHEncode1_next(FMHEncode1 *unit, int inNumSamples)
{       
	float sinint, cosint, z = 0.0;
	float *Wout = OUT(0);
	float *Xout = OUT(1);
	float *Yout = OUT(2);
	float *Zout = OUT(3);
	float *Rout = OUT(4);
	float *Sout = OUT(5);
	float *Tout = OUT(6);
	float *Uout = OUT(7);
	float *Vout = OUT(8);
	
	float *in = IN(0);
	float azimuth = IN0(1);
	float elevation = IN0(2);
	float rho = IN0(3);
	float level = IN0(4);

	float W_amp = unit->m_W_amp;
	float X_amp = unit->m_X_amp;
	float Y_amp = unit->m_Y_amp;
	float Z_amp = unit->m_Z_amp;
	float R_amp = unit->m_R_amp;
	float S_amp = unit->m_S_amp;
	float T_amp = unit->m_T_amp;
	float U_amp = unit->m_U_amp;
	float V_amp = unit->m_V_amp;
	
	if (azimuth != unit->m_azimuth || rho != unit->m_rho || elevation != unit->m_elevation || level != unit->m_level) {
		unit->m_azimuth = azimuth;
		unit->m_elevation = elevation;
		unit->m_level = level;
		unit->m_rho = rho;
		
		float sina = sin(azimuth);
		float sinb = sin(elevation);
		
		float cosa = cos(azimuth);
		float cosb = cos(elevation);
		
		float azimuthtimes2 = azimuth * 2.;
		float cosbsq = cosb * cosb;
		
		if(rho >= 1) {
			float intrho = 1 / pow(rho, 1.5);
			sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
			cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
		    else { 
			sinint = rsqrt2 * (sin(0.78539816339745 * rho));
			cosint = rsqrt2 * (cos(0.78539816339745 * rho));
			};
		
		float levelsinint = level * sinint;
		
		float next_W_amp = level * cosint;
		float next_X_amp = cosa * cosb * levelsinint;
		float next_Y_amp = sina * cosb * levelsinint;
		float next_Z_amp = sinb * levelsinint;
		float next_R_amp = sqrt3div2 * cosbsq * cos(azimuthtimes2) * levelsinint;
		float next_S_amp = sqrt3div2 * cosbsq * sin(azimuthtimes2) * levelsinint; 
		float next_T_amp = sqrt3 * cosb * sinb * cosa * levelsinint;
		float next_U_amp = sqrt3 * cosb * sinb * sina * levelsinint;
		float next_V_amp = 0.5 * (3. * (sinb * sinb) - 1.) * levelsinint;
		
		float W_slope = CALCSLOPE(next_W_amp, W_amp);
		float X_slope = CALCSLOPE(next_X_amp, X_amp);
		float Y_slope = CALCSLOPE(next_Y_amp, Y_amp);
		float Z_slope = CALCSLOPE(next_Z_amp, Z_amp);
		float R_slope = CALCSLOPE(next_R_amp, R_amp);
		float S_slope = CALCSLOPE(next_S_amp, S_amp);
		float T_slope = CALCSLOPE(next_T_amp, T_amp);
		float U_slope = CALCSLOPE(next_U_amp, U_amp);
		float V_slope = CALCSLOPE(next_V_amp, V_amp);
				
		for(int i = 0; i < inNumSamples; i++){
			z = in[i];
			// vary w according to x, y and z
			Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp) + (R_amp * R_amp) + (S_amp * S_amp) +
			    (T_amp * T_amp) + (U_amp * U_amp) + (V_amp * V_amp)))));			
			Xout[i] = z * X_amp;
			Yout[i] = z * Y_amp;
			Zout[i] = z * Z_amp;
			Rout[i] = z * R_amp;
			Sout[i] = z * S_amp;
			Tout[i] = z * T_amp;
			Uout[i] = z * U_amp;
			Vout[i] = z * V_amp;			
						
			W_amp += W_slope;
			X_amp += X_slope;
			Y_amp += Y_slope;
			Z_amp += Z_slope;
			R_amp += R_slope;
			S_amp += S_slope;
			T_amp += T_slope;
			U_amp += U_slope;
			V_amp += V_slope;

		    }
		    
		unit->m_W_amp = W_amp;
		unit->m_X_amp = X_amp;
		unit->m_Y_amp = Y_amp;
		unit->m_Z_amp = Z_amp;
		unit->m_R_amp = R_amp;
		unit->m_S_amp = S_amp;
		unit->m_T_amp = T_amp;
		unit->m_U_amp = U_amp;
		unit->m_V_amp = V_amp;

	} else {
	    for(int i = 0; i < inNumSamples; i++)
		{ 
		    z = in[i];
		    Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp) + (R_amp * R_amp) + (S_amp * S_amp) +
			(T_amp * T_amp) + (U_amp * U_amp) + (V_amp * V_amp)))));			
		    Xout[i] = z * X_amp;
		    Yout[i] = z * Y_amp;
		    Zout[i] = z * Z_amp;
		    Rout[i] = z * R_amp;
		    Sout[i] = z * S_amp;
		    Tout[i] = z * T_amp;
		    Uout[i] = z * U_amp;
		    Vout[i] = z * V_amp;
		}
	}
}



void FMHEncode2_Ctor(FMHEncode2 *unit)
{
	SETCALC(FMHEncode2_next);
	float sinint, cosint, azimuth, rho;
	float point_x = unit->m_point_x = IN0(1);
	float point_y = unit->m_point_y = IN0(2);
	float elevation = unit->m_elevation = IN0(3);
	float level = unit->m_level = IN0(4);

	azimuth = atan2(point_x, point_y);
	rho = hypot(point_x, point_y);
	
	float azimuthtimes2 = azimuth * 2.;
	float sina = sin(azimuth);
	float sinb = sin(elevation);
	
	float cosa = cos(azimuth);
	float cosb = cos(elevation);
	
	float cosbsq = cosb * cosb;
	
	// the whole rho thing just may not work... let's try anyways!
	if(rho >= 1) {
		float intrho = 1 / pow(rho, 1.5);
		sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
		cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
	    else 
		{
		sinint = rsqrt2 * (sin(0.78539816339745 * rho));
		cosint = rsqrt2 * (cos(0.78539816339745 * rho));
		};
	
	float levelsinint = level * sinint;
	
	unit->m_W_amp = level * cosint;
	unit->m_X_amp = cosa * cosb * levelsinint;
	unit->m_Y_amp = sina * cosb * levelsinint;
	unit->m_Z_amp = sinb * level * sinint;
	unit->m_R_amp = sqrt3div2 * cosbsq * cos(azimuthtimes2) * levelsinint;
	unit->m_S_amp = sqrt3div2 * cosbsq * sin(azimuthtimes2) * levelsinint; 
	unit->m_T_amp = sqrt3 * cosb * sinb * cosa * levelsinint;
	unit->m_U_amp = sqrt3 * cosb * sinb * sina * levelsinint;
	unit->m_V_amp = 0.5 * (3. * (sinb * sinb) - 1.) * levelsinint;
	
	FMHEncode2_next(unit, 1);
}

void FMHEncode2_next(FMHEncode2 *unit, int inNumSamples)
{       
	float sinint, cosint, z, azimuth, rho = 0.0;
	float *Wout = OUT(0);
	float *Xout = OUT(1);
	float *Yout = OUT(2);
	float *Zout = OUT(3);
	float *Rout = OUT(4);
	float *Sout = OUT(5);
	float *Tout = OUT(6);
	float *Uout = OUT(7);
	float *Vout = OUT(8);
	
	float *in = IN(0);
	float point_x = IN0(1);
	float point_y = IN0(2);
	float elevation = unit->m_elevation = IN0(3);
	float level = unit->m_level = IN0(4);


	float W_amp = unit->m_W_amp;
	float X_amp = unit->m_X_amp;
	float Y_amp = unit->m_Y_amp;
	float Z_amp = unit->m_Z_amp;
	float R_amp = unit->m_R_amp;
	float S_amp = unit->m_S_amp;
	float T_amp = unit->m_T_amp;
	float U_amp = unit->m_U_amp;
	float V_amp = unit->m_V_amp;
	
	if (point_x != unit->m_point_x || point_y != unit->m_point_y || rho != unit->m_rho || elevation != unit->m_elevation || level != unit->m_level) {
		unit->m_point_x = point_x;
		unit->m_point_y = point_y;
		unit->m_elevation = elevation;
		unit->m_level = level;
		unit->m_rho = rho;

		azimuth = atan2(point_x, point_y);
		rho = hypot(point_x, point_y);
				
		float sina = sin(azimuth);
		float sinb = sin(elevation);
		
		float cosa = cos(azimuth);
		float cosb = cos(elevation);
		
		float azimuthtimes2 = azimuth * 2.;
		float cosbsq = cosb * cosb;
		
		if(rho >= 1) {
			float intrho = 1 / pow(rho, 1.5);
			sinint = (rsqrt2 * (sin(0.78539816339745))) * intrho; //  pow(rho, (float)1.5);
			cosint =  (rsqrt2 * (cos(0.78539816339745))) * intrho;} //  pow(rho, (float)1.5);}
		    else { 
			sinint = rsqrt2 * (sin(0.78539816339745 * rho));
			cosint = rsqrt2 * (cos(0.78539816339745 * rho));
			};
		
		float levelsinint = level * sinint;
		
		float next_W_amp = level * cosint;
		float next_X_amp = cosa * cosb * levelsinint;
		float next_Y_amp = sina * cosb * levelsinint;
		float next_Z_amp = sinb * levelsinint;
		float next_R_amp = sqrt3div2 * cosbsq * cos(azimuthtimes2) * levelsinint;
		float next_S_amp = sqrt3div2 * cosbsq * sin(azimuthtimes2) * levelsinint; 
		float next_T_amp = sqrt3 * cosb * sinb * cosa * levelsinint;
		float next_U_amp = sqrt3 * cosb * sinb * sina * levelsinint;
		float next_V_amp = 0.5 * (3. * (sinb * sinb) - 1.) * levelsinint;
		
		float W_slope = CALCSLOPE(next_W_amp, W_amp);
		float X_slope = CALCSLOPE(next_X_amp, X_amp);
		float Y_slope = CALCSLOPE(next_Y_amp, Y_amp);
		float Z_slope = CALCSLOPE(next_Z_amp, Z_amp);
		float R_slope = CALCSLOPE(next_R_amp, R_amp);
		float S_slope = CALCSLOPE(next_S_amp, S_amp);
		float T_slope = CALCSLOPE(next_T_amp, T_amp);
		float U_slope = CALCSLOPE(next_U_amp, U_amp);
		float V_slope = CALCSLOPE(next_V_amp, V_amp);
				
		for(int i = 0; i < inNumSamples; i++){
			z = in[i];
			// vary w according to x, y and z
			Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp) + (R_amp * R_amp) + (S_amp * S_amp) +
			    (T_amp * T_amp) + (U_amp * U_amp) + (V_amp * V_amp)))));			
			Xout[i] = z * X_amp;
			Yout[i] = z * Y_amp;
			Zout[i] = z * Z_amp;
			Rout[i] = z * R_amp;
			Sout[i] = z * S_amp;
			Tout[i] = z * T_amp;
			Uout[i] = z * U_amp;
			Vout[i] = z * V_amp;			
						
			W_amp += W_slope;
			X_amp += X_slope;
			Y_amp += Y_slope;
			Z_amp += Z_slope;
			R_amp += R_slope;
			S_amp += S_slope;
			T_amp += T_slope;
			U_amp += U_slope;
			V_amp += V_slope;

		    }
		    
		unit->m_W_amp = W_amp;
		unit->m_X_amp = X_amp;
		unit->m_Y_amp = Y_amp;
		unit->m_Z_amp = Z_amp;
		unit->m_R_amp = R_amp;
		unit->m_S_amp = S_amp;
		unit->m_T_amp = T_amp;
		unit->m_U_amp = U_amp;
		unit->m_V_amp = V_amp;

	} else {
	    for(int i = 0; i < inNumSamples; i++)
		{ 
		    z = in[i];
		    Wout[i] = z * (W_amp * (1 - (0.293 * ((X_amp * X_amp) + (Y_amp * Y_amp) + (Z_amp * Z_amp) + (R_amp * R_amp) + (S_amp * S_amp) +
			(T_amp * T_amp) + (U_amp * U_amp) + (V_amp * V_amp)))));			
		    Xout[i] = z * X_amp;
		    Yout[i] = z * Y_amp;
		    Zout[i] = z * Z_amp;
		    Rout[i] = z * R_amp;
		    Sout[i] = z * S_amp;
		    Tout[i] = z * T_amp;
		    Uout[i] = z * U_amp;
		    Vout[i] = z * V_amp;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

void BFDecode1_Ctor(BFDecode1 *unit)
{
	SETCALC(BFDecode1_next);

	BFDecode1_next(unit, 1);
	
	float angle = IN0(4);
	float elevation = IN0(5);
	
	unit->m_cosa = cos(angle);
	unit->m_sina = sin(angle);
	unit->m_sinb = sin(elevation);
}

void BFDecode1_next(BFDecode1 *unit, int inNumSamples)
{
	float *Win0 = IN(0);
	float *Xin0 = IN(1);
	float *Yin0 = IN(2);
	float *Zin0 = IN(3);
	float *out = OUT(0);

	float cosa = unit->m_cosa;
	float sina = unit->m_sina;
	float sinb = unit->m_sinb;
	
	// scaling is done according to W in the Encoders
	float W_amp = 1.0; //0.7071067811865476;
	float X_amp = cosa; //0.5 * cosa;
	float Y_amp = sina; //0.5 * sina;
	float Z_amp = sinb; //0.5 * sinb;

	for(int i = 0; i < inNumSamples; i++)
	    { 
		out[i] = (Win0[i] * W_amp) +    
			(Xin0[i] * X_amp) + 
			(Yin0[i] * Y_amp) + 
			(Zin0[i] * Z_amp);
	    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

void load(InterfaceTable *inTable)
{
	ft = inTable;
	
	DefineSimpleUnit(BFEncode1);
	DefineSimpleUnit(BFEncode2);
	DefineSimpleUnit(BFEncodeSter);
	DefineSimpleUnit(FMHEncode1);
	DefineSimpleUnit(FMHEncode2);
	DefineSimpleCantAliasUnit(BFDecode1);
	DefineSimpleUnit(BFManipulate);
	DefineSimpleUnit(B2Ster);
	DefineSimpleUnit(A2B);
	DefineSimpleUnit(B2A);
	DefineSimpleUnit(UHJ2B);
	DefineSimpleUnit(B2UHJ);

	
}


	