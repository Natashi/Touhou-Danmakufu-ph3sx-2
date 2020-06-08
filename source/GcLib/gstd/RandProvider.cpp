#include "source/GcLib/pch.h"

#include "RandProvider.hpp"

using namespace gstd;

inline uint64_t RandProvider::rotl(uint64_t u, size_t x) {
	return (u << x) | (u >> (64U - x));
}
inline uint64_t RandProvider::rotr(uint64_t u, size_t x) {
	return (u >> x) | (u << (64U - x));
}

RandProvider::RandProvider() {
	this->Initialize(0);
}
RandProvider::RandProvider(uint32_t s) {
	this->Initialize(s);
}

/*
uint32_t RandProvider::_GenrandInt32() {
	if (mtIndex_ >= MT_N) {
		for (int i = 0; i < MT_N; ++i) {
			int x = (mtStates_[i] & UPPER_MASK) + (mtStates_[(i + 1) % MT_N] & LOWER_MASK);
			int y = x >> 1;

			if ((x & 0x00000001) == 1)
				y = y ^ MT_A;
			mtStates_[i] = mtStates_[(i + MT_M) % MT_N] ^ y;
		}
		mtIndex_ = 0;
	}

	uint32_t result = mtStates_[mtIndex_++];

	result ^= (result >> MT_U);
	result ^= (result << MT_S) & MT_B;
	result ^= (result << MT_T) & MT_C;
	result ^= (result >> MT_L);

	return (result & MT_D);
}

void RandProvider::Initialize(unsigned long s) {
	mtIndex_ = MT_N;
	mtStates_[0] = s;

	for (int i = 1; i < MT_N; ++i) {
		mtStates_[i] = (MT_F * mtStates_[i - 1] ^ (mtStates_[i - 1] >> (MT_W - 2)) + i) & MT_D;
	}
	seed_ = s;
}
*/
void RandProvider::Initialize(uint32_t s) {
	memset(states_, 0x00, sizeof(uint64_t) * 4U);

	static const uint64_t JUMP[] = 
		{ 0x180ec6d33cfd0abaui64, 0xd5a61266f0c9392cui64, 0xa9582618e03fc9aaui64, 0x39abdc4529b1661cui64 };

	seed_ = s;

	states_[0] = s ^ JUMP[0];
	for (uint64_t i = 1ui64; i < 4ui64; ++i) {
		uint64_t s = 0x76e15d3efefdcbbfui64 * states_[i - 1] ^ (states_[i - 1] >> 37) + i;
		s ^= JUMP[i];
		states_[i] = s & 0xffffffffffffffffui64;
	}
}
uint64_t RandProvider::_GenrandInt64() {
	uint64_t result = rotl(states_[1] * 5ui64, 7U) * 9ui64;

	uint64_t t = states_[1] << 17U;

	states_[2] ^= states_[0];
	states_[3] ^= states_[1];
	states_[1] ^= states_[2];
	states_[0] ^= states_[3];

	states_[2] ^= t;

	states_[3] = rotl(states_[3], 45U);

	return result;
}

int RandProvider::GetInt() {
	return (int)(_GenrandInt64() >> 1u);
}
int RandProvider::GetInt(int min, int max) {
	return (int)this->GetReal(min, max);
}
int64_t RandProvider::GetInt64() {
	return (int64_t)this->GetReal();
}
int64_t RandProvider::GetInt64(int64_t min, int64_t max) {
	return (int64_t)this->GetReal(min, max);
}
double RandProvider::GetReal() {
	return (double)(_GenrandInt64() * (1.0 / (double)UINT64_MAX));
}
double RandProvider::GetReal(double a, double b) {
	if (a == b) return a;
	return (a + GetReal() * (b - a));
}