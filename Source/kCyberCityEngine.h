#pragma once
// Adapted from Airwindows kCyberCity (MIT). Processing bodies are unchanged.
#include <cstdint>
#include <cmath>
#include <algorithm>
class kCyberCityEngine {
public:
    kCyberCityEngine() { reset(44100.); }
    void reset(double sampleRate);
    void setParameter(int index,float value);
    void processReplacing(float** inputs,float** outputs,int sampleFrames);
    void processDoubleReplacing(double** inputs,double** outputs,int sampleFrames);
    double getSampleRate() const { return rate; }
private:
    double rate=44100.;
inline static constexpr int d3A = 2114; inline static constexpr int d3B = 3309; inline static constexpr int d3C = 3425;
inline static constexpr int d3D = 2083; inline static constexpr int d3E = 2178; inline static constexpr int d3F = 2990;
inline static constexpr int d3G = 1880; inline static constexpr int d3H = 1903; inline static constexpr int d3I = 2801;
inline static constexpr int d6A = 1903; inline static constexpr int d6B = 19; inline static constexpr int d6C = 23; inline static constexpr int d6D = 29; inline static constexpr int d6E = 830; inline static constexpr int d6F = 1449; inline static constexpr int d6G = 41; inline static constexpr int d6H = 43; inline static constexpr int d6I = 47; inline static constexpr int d6J = 1327; inline static constexpr int d6K = 2178; inline static constexpr int d6L = 1657; inline static constexpr int d6M = 2114; inline static constexpr int d6N = 384; inline static constexpr int d6O = 73; inline static constexpr int d6P = 79; inline static constexpr int d6Q = 3309; inline static constexpr int d6R = 2083; inline static constexpr int d6S = 632; inline static constexpr int d6T = 302; inline static constexpr int d6U = 239; inline static constexpr int d6V = 445; inline static constexpr int d6W = 635; inline static constexpr int d6X = 113; inline static constexpr int d6Y = 127; inline static constexpr int d6ZA = 131; inline static constexpr int d6ZB = 2990; inline static constexpr int d6ZC = 70; inline static constexpr int d6ZD = 1880; inline static constexpr int d6ZE = 151; inline static constexpr int d6ZF = 222; inline static constexpr int d6ZG = 24; inline static constexpr int d6ZH = 2801; inline static constexpr int d6ZI = 460; inline static constexpr int d6ZJ = 3425; inline static constexpr int d6ZK = 181; //3 to 327 ms, 3264 seat arena  
inline static constexpr int early[] = {19, 23, 24, 29, 41, 43, 47, 70, 73, 79, 113, 127, 131, 151, 181, 222, 239, 302, 384, 445, 460, 632, 635, 830, 1327, 1449, 1657, 1880, 1903, 2083, 2114, 2178, 2801, 2990, 3309, 3425};

    float A;
    float B;
    float C;

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
	int ld3A,ld3B,ld3C,ld3D,ld3E,ld3F,ld3G,ld3H,ld3I;
	int earlyZero;
	
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
		bez_SampL,
		bez_SampR,
		bez_cycle,
		bez_total
	}; //the new undersampling. bez signifies the bezier curve reconstruction
	double bez[bez_total];
	
	double avg32L[33];
	double avg32R[33];
	double avg16L[17];
	double avg16R[17];
	double avg8L[9];
	double avg8R[9];
	double avg4L[5];
	double avg4R[5];
	double avg2L[3];
	double avg2R[3];
	double post32L[33];
	double post32R[33];
	double post16L[17];
	double post16R[17];
	double post8L[9];
	double post8R[9];
	double post4L[5];
	double post4R[5];
	double post2L[3];
	double post2R[3];
	double lastDarkL;
	double lastDarkR;
	int avgPos;
	//preTapeHack
	double roomNoiseL;
	double roomNoiseR;
	int roomTimerL;
	int roomTimerR;
	
	uint32_t fpdL;
	uint32_t fpdR;
};
