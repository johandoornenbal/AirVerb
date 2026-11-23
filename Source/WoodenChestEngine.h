#pragma once
// Adapted from Airwindows WoodenChest (MIT). Processing bodies are unchanged.
#include <cstdint>
#include <cmath>
#include <algorithm>

class WoodenChestEngine {
public:
    WoodenChestEngine() { reset(44100.); }
    void reset(double sampleRate);
    void setParameter(int index,float value);
    void processReplacing(float** inputs,float** outputs,int sampleFrames);
    void processDoubleReplacing(double** inputs,double** outputs,int sampleFrames);
    double getSampleRate() const { return rate; }
private:
    double rate=44100.;
inline static constexpr int d4A = 380; inline static constexpr int d4B = 343; inline static constexpr int d4C = 370; inline static constexpr int d4D = 384;
inline static constexpr int d4E = 363; inline static constexpr int d4F = 414; inline static constexpr int d4G = 366; inline static constexpr int d4H = 378;
inline static constexpr int d4I = 401; inline static constexpr int d4J = 400; inline static constexpr int d4K = 360; inline static constexpr int d4L = 384;
inline static constexpr int d4M = 391; inline static constexpr int d4N = 353; inline static constexpr int d4O = 382; inline static constexpr int d4P = 345;
    float A;
    float B;
    float C;
	
	double lA[d4A+5];
	double lB[d4B+5];
	double lC[d4C+5];
	double lD[d4D+5];
	double lE[d4E+5];
	double lF[d4F+5];
	double lG[d4G+5];
	double lH[d4H+5];
	double lI[d4I+5];
	double lJ[d4J+5];
	double lK[d4K+5];
	double lL[d4L+5];
	double lM[d4M+5];
	double lN[d4N+5];
	double lO[d4O+5];
	double lP[d4P+5];
	
	double rA[d4A+5];
	double rB[d4B+5];
	double rC[d4C+5];
	double rD[d4D+5];
	double rE[d4E+5];
	double rF[d4F+5];
	double rG[d4G+5];
	double rH[d4H+5];
	double rI[d4I+5];
	double rJ[d4J+5];
	double rK[d4K+5];
	double rL[d4L+5];
	double rM[d4M+5];
	double rN[d4N+5];
	double rO[d4O+5];
	double rP[d4P+5];
	
	int cLA,cLB,cLC,cLD,cLE,cLF,cLG,cLH;
	int cLI,cLJ,cLK,cLL,cLM,cLN,cLO,cLP;
	
	int cRA,cRB,cRC,cRD,cRE,cRF,cRG,cRH;
	int cRI,cRJ,cRK,cRL,cRM,cRN,cRO,cRP;
	
	double fAL,fBL,fCL,fDL;
	double hAL, hBL, hCL, hDL, hEL; //do not allocate in audio code
	
	double fDR,fHR,fLR,fPR;
	double hAR, hBR, hCR, hDR, hER; //do not allocate in audio code
	
	enum {
		bez_AL,
		bez_AR,
		bez_BL,
		bez_BR,
		bez_CL,
		bez_CR,	
		bez_SampL,
		bez_SampR,
		bez_cycle,
		bez_total
	}; //the new undersampling. bez signifies the bezier curve reconstruction
	double bez[bez_total];
	
	int shortA;
	int shortB;
	int shortC;
	int shortD;
	int shortE;
	int shortF;
	int shortG;
	int shortH;
	int shortI;
	int shortJ;
	int shortK;
	int shortL;
	int shortM;
	int shortN;
	int shortO;
	int shortP;
	
	int prevclearcoat;	
	uint32_t fpdL;
	uint32_t fpdR;
	//default stuff
};
