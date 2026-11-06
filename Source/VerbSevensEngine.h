#pragma once
// Adapted from Airwindows VerbSevens (MIT). Processing bodies are unchanged.
#include <cstdint>
#include <cmath>
#include <algorithm>
class VerbSevensEngine {
public:
    VerbSevensEngine() { reset(44100.); }
    void reset(double sampleRate);
    void setParameter(int index,float value);
    void processReplacing(float** inputs,float** outputs,int sampleFrames);
    void processDoubleReplacing(double** inputs,double** outputs,int sampleFrames);
    double getSampleRate() const { return rate; }
private:
    double rate=44100.;
inline static constexpr int d4A = 17; inline static constexpr int d4B = 19; inline static constexpr int d4C = 24; inline static constexpr int d4D = 6682; inline static constexpr int d4E = 31; inline static constexpr int d4F = 37; inline static constexpr int d4G = 41; inline static constexpr int d4H = 43; inline static constexpr int d4I = 47; inline static constexpr int d4J = 6312; inline static constexpr int d4K = 755; inline static constexpr int d4L = 6697; inline static constexpr int d4M = 67; inline static constexpr int d4N = 71; inline static constexpr int d4O = 73; inline static constexpr int d4P = 6638; //2 to 454 ms, 6214 seat arena  
inline static constexpr int d6A = 511; inline static constexpr int d6B = 1282; inline static constexpr int d6C = 772; inline static constexpr int d6D = 2663; inline static constexpr int d6E = 227; inline static constexpr int d6F = 763; inline static constexpr int d6G = 626; inline static constexpr int d6H = 265; inline static constexpr int d6I = 3699; inline static constexpr int d6J = 3489; inline static constexpr int d6K = 2923; inline static constexpr int d6L = 281; inline static constexpr int d6M = 592; inline static constexpr int d6N = 2551; inline static constexpr int d6O = 136; inline static constexpr int d6P = 2795; inline static constexpr int d6Q = 2000; inline static constexpr int d6R = 1652; inline static constexpr int d6S = 394; inline static constexpr int d6T = 535; inline static constexpr int d6U = 2114; inline static constexpr int d6V = 69; inline static constexpr int d6W = 1239; inline static constexpr int d6X = 2822; inline static constexpr int d6Y = 269; inline static constexpr int d6ZA = 508; inline static constexpr int d6ZB = 2962; inline static constexpr int d6ZC = 2959; inline static constexpr int d6ZD = 214; inline static constexpr int d6ZE = 189; inline static constexpr int d6ZF = 1120; inline static constexpr int d6ZG = 226; inline static constexpr int d6ZH = 530; inline static constexpr int d6ZI = 847; inline static constexpr int d6ZJ = 539; inline static constexpr int d6ZK = 1989; 
    
	double a4AL[d4A+5];
	double a4BL[d4B+5];
	double a4CL[d4C+5];
	double a4DL[d4D+5];
	double a4EL[d4E+5];
	double a4FL[d4F+5];
	double a4GL[d4G+5];
	double a4HL[d4H+5];
	double a4IL[d4I+5];
	double a4JL[d4J+5];
	double a4KL[d4K+5];
	double a4LL[d4L+5];
	double a4ML[d4M+5];
	double a4NL[d4N+5];
	double a4OL[d4O+5];
	double a4PL[d4P+5];
	double a4AR[d4A+5];
	double a4BR[d4B+5];
	double a4CR[d4C+5];
	double a4DR[d4D+5];
	double a4ER[d4E+5];
	double a4FR[d4F+5];
	double a4GR[d4G+5];
	double a4HR[d4H+5];
	double a4IR[d4I+5];
	double a4JR[d4J+5];
	double a4KR[d4K+5];
	double a4LR[d4L+5];
	double a4MR[d4M+5];
	double a4NR[d4N+5];
	double a4OR[d4O+5];
	double a4PR[d4P+5];
	int c4AL,c4BL,c4CL,c4DL,c4EL,c4FL,c4GL,c4HL;
	int c4IL,c4JL,c4KL,c4LL,c4ML,c4NL,c4OL,c4PL;		
	int c4AR,c4BR,c4CR,c4DR,c4ER,c4FR,c4GR,c4HR;
	int c4IR,c4JR,c4KR,c4LR,c4MR,c4NR,c4OR,c4PR;
	double f4AL,f4BL,f4CL,f4DL,f4DR,f4HR,f4LR,f4PR;
	double avg4L,avg4R; 
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
	
	double hA,hB,hC,hD,hE,hF;
	double hAL,hBL,hCL,hDL,hEL;
	double hAR,hBR,hCR,hDR,hER;
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
		bez_cycle,
		bez_total
	}; //the new undersampling. bez signifies the bezier curve reconstruction
	double bez[bez_total];
	
	uint32_t fpdL;
	uint32_t fpdR;
	//default stuff
};
