#pragma once
// Adapted from Airwindows kStation (MIT). Processing bodies are unchanged.
#include <cstdint>
#include <cmath>
#include <algorithm>
class kStationEngine {
public:
    kStationEngine() { reset(44100.); }
    void reset(double sampleRate);
    void setParameter(int index,float value);
    void processReplacing(float** inputs,float** outputs,int sampleFrames);
    void processDoubleReplacing(double** inputs,double** outputs,int sampleFrames);
    double getSampleRate() const { return rate; }
private:
    double rate=44100.;
inline static constexpr int d3A = 636; inline static constexpr int d3B = 787; inline static constexpr int d3C = 816;
inline static constexpr int d3D = 624; inline static constexpr int d3E = 650; inline static constexpr int d3F = 750;
inline static constexpr int d3G = 445; inline static constexpr int d3H = 517; inline static constexpr int d3I = 656;
inline static constexpr int d6A = 118; inline static constexpr int d6B = 27; inline static constexpr int d6C = 38; inline static constexpr int d6D = 139; inline static constexpr int d6E = 750; inline static constexpr int d6F = 32; inline static constexpr int d6G = 285; inline static constexpr int d6H = 36; inline static constexpr int d6I = 206; inline static constexpr int d6J = 517; inline static constexpr int d6K = 72; inline static constexpr int d6L = 650; inline static constexpr int d6M = 43; inline static constexpr int d6N = 59; inline static constexpr int d6O = 816; inline static constexpr int d6P = 787; inline static constexpr int d6Q = 73; inline static constexpr int d6R = 91; inline static constexpr int d6S = 34; inline static constexpr int d6T = 9; inline static constexpr int d6U = 656; inline static constexpr int d6V = 445; inline static constexpr int d6W = 6; inline static constexpr int d6X = 47; inline static constexpr int d6Y = 91; inline static constexpr int d6ZA = 48; inline static constexpr int d6ZB = 91; inline static constexpr int d6ZC = 8; inline static constexpr int d6ZD = 636; inline static constexpr int d6ZE = 41; inline static constexpr int d6ZF = 432; inline static constexpr int d6ZG = 30; inline static constexpr int d6ZH = 293; inline static constexpr int d6ZI = 7; inline static constexpr int d6ZJ = 115; inline static constexpr int d6ZK = 624; //1 to 93 ms, 272 seat club  
inline static constexpr int early[] = {6, 7, 8, 9, 27, 30, 32, 34, 36, 38, 41, 43, 47, 48, 59, 72, 73, 91, 91, 91, 115, 118, 139, 206, 285, 293, 432, 445, 517, 624, 636, 650, 656, 750, 787, 816};
    
    float A;
    float B;
    float C;
    float D;
    float E;
    float F;
	
	double a3AL[d3A+5];
	double a3BL[d3B+5];
	double a3CL[d3C+5];
	double a3DL[d3D+5];
	double a3EL[d3E+5];
	double a3FL[d3F+5];
	double a3GL[d3G+5];
	double a3HL[d3H+5];
	double a3IL[d3I+5];
	double a3AR[d3A+5];
	double a3BR[d3B+5];
	double a3CR[d3C+5];
	double a3DR[d3D+5];
	double a3ER[d3E+5];
	double a3FR[d3F+5];
	double a3GR[d3G+5];
	double a3HR[d3H+5];
	double a3IR[d3I+5];
	int c3AL,c3AR,c3BL,c3BR,c3CL,c3CR,c3DL,c3DR,c3EL,c3ER;
	int c3FL,c3FR,c3GL,c3GR,c3HL,c3HR,c3IL,c3IR;
	
	double a6AL[d6A+5];
	double a6BL[d6B+5];
	double a6CL[d6C+5];
	double a6DL[d6D+5];
	double a6EL[d6E+5];
	double a6FL[d6F+5];
	double a6GL[d6G+5];
	double a6HL[d6H+5];
	double a6IL[d6I+5];
	double a6JL[d6J+5];
	double a6KL[d6K+5];
	double a6LL[d6L+5];
	double a6ML[d6M+5];
	double a6NL[d6N+5];
	double a6OL[d6O+5];
	double a6PL[d6P+5];
	double a6QL[d6Q+5];
	double a6RL[d6R+5];
	double a6SL[d6S+5];
	double a6TL[d6T+5];
	double a6UL[d6U+5];
	double a6VL[d6V+5];
	double a6WL[d6W+5];
	double a6XL[d6X+5];
	double a6YL[d6Y+5];
	double a6ZAL[d6ZA+5];
	double a6ZBL[d6ZB+5];
	double a6ZCL[d6ZC+5];
	double a6ZDL[d6ZD+5];
	double a6ZEL[d6ZE+5];
	double a6ZFL[d6ZF+5];
	double a6ZGL[d6ZG+5];
	double a6ZHL[d6ZH+5];
	double a6ZIL[d6ZI+5];
	double a6ZJL[d6ZJ+5];
	double a6ZKL[d6ZK+5];
	double a6AR[d6A+5];
	double a6BR[d6B+5];
	double a6CR[d6C+5];
	double a6DR[d6D+5];
	double a6ER[d6E+5];
	double a6FR[d6F+5];
	double a6GR[d6G+5];
	double a6HR[d6H+5];
	double a6IR[d6I+5];
	double a6JR[d6J+5];
	double a6KR[d6K+5];
	double a6LR[d6L+5];
	double a6MR[d6M+5];
	double a6NR[d6N+5];
	double a6OR[d6O+5];
	double a6PR[d6P+5];
	double a6QR[d6Q+5];
	double a6RR[d6R+5];
	double a6SR[d6S+5];
	double a6TR[d6T+5];
	double a6UR[d6U+5];
	double a6VR[d6V+5];
	double a6WR[d6W+5];
	double a6XR[d6X+5];
	double a6YR[d6Y+5];
	double a6ZAR[d6ZA+5];
	double a6ZBR[d6ZB+5];
	double a6ZCR[d6ZC+5];
	double a6ZDR[d6ZD+5];
	double a6ZER[d6ZE+5];
	double a6ZFR[d6ZF+5];
	double a6ZGR[d6ZG+5];
	double a6ZHR[d6ZH+5];
	double a6ZIR[d6ZI+5];
	double a6ZJR[d6ZJ+5];
	double a6ZKR[d6ZK+5];
	int c6AL,c6BL,c6CL,c6DL,c6EL,c6FL,c6GL,c6HL,c6IL;
	int c6JL,c6KL,c6LL,c6ML,c6NL,c6OL,c6PL,c6QL,c6RL;
	int c6SL,c6TL,c6UL,c6VL,c6WL,c6XL,c6YL,c6ZAL,c6ZBL;	
	int c6ZCL,c6ZDL,c6ZEL,c6ZFL,c6ZGL,c6ZHL,c6ZIL,c6ZJL,c6ZKL;		
	int c6AR,c6BR,c6CR,c6DR,c6ER,c6FR,c6GR,c6HR,c6IR;
	int c6JR,c6KR,c6LR,c6MR,c6NR,c6OR,c6PR,c6QR,c6RR;
	int c6SR,c6TR,c6UR,c6VR,c6WR,c6XR,c6YR,c6ZAR,c6ZBR;	
	int c6ZCR,c6ZDR,c6ZER,c6ZFR,c6ZGR,c6ZHR,c6ZIR,c6ZJR,c6ZKR;
	double f6AL,f6BL,f6CL,f6DL,f6EL,f6FL;
	double f6FR,f6LR,f6RR,f6XR,f6ZER,f6ZKR;
	double avg6L,avg6R; 
	
	enum {
		bez_AL,
		bez_AR,
		bez_BL,
		bez_BR,
		bez_CL,
		bez_CR,	
		bez_InL,
		bez_InR,
		bez_UnInL,
		bez_UnInR,
		bez_SampL,
		bez_SampR,
		bez_AvgInSampL,
		bez_AvgInSampR,
		bez_AvgOutSampL,
		bez_AvgOutSampR,
		bez_cycle,
		bez_total
	}; //the new undersampling. bez signifies the bezier curve reconstruction
	double bez[bez_total];
	
	double bezF[bez_total];
	
	uint32_t fpdL;
	uint32_t fpdR;
	//default stuff
};
