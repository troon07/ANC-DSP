#pragma once
#include <stdexcept>
#include <random>
#include <cmath>
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
inline float noise() { static std::mt19937 g(42); static std::uniform_real_distribution<float> d(-0.2f,0.2f); return d(g); }
