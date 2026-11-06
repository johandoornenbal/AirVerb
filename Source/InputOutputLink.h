#pragma once
#include <algorithm>
// Preserve the sum of wrapper trim gains in dB during editor gestures.
class InputOutputLink {
public:
    void reset(float input,float output) { sum=input+output; }
    float outputFor(float input) const { return std::clamp(sum-input,-24.f,24.f); }
private: float sum=0;
};
