#ifndef __GSTD_RANDPROVIDER__
#define __GSTD_RANDPROVIDER__

#include "../pch.h"

namespace gstd {
	/*
	class RandProvider {
		enum {
			MT_W = 32,
			MT_N = 624,
			MT_M = 397,
			MT_R = 31,

			MT_A = 0x9908B0DF,

			MT_U = 11,
			MT_D = 0xFFFFFFFF,

			MT_S = 7,
			MT_B = 0x9D2C5680,

			MT_T = 15,
			MT_C = 0xEFC60000,
			MT_L = 18,

			MT_F = 0x6C078965,

			LOWER_MASK = 0x7FFFFFFF,
			UPPER_MASK = 0x80000000,
		};

		uint32_t mtStates_[MT_N];
		int mtIndex_;

		int seed_;
		uint32_t _GenrandInt32();
	public:
		RandProvider();
		RandProvider(unsigned long s);
		virtual ~RandProvider() {}
		void Initialize(unsigned long s);

		int GetSeed() { return seed_; }
		long GetInt();
		long GetInt(long min, long max);
		int64_t GetInt64();
		int64_t GetInt64(int64_t min, int64_t max);
		float GetReal();
		float GetReal(float min, float max);
	};
	*/

	//Xoroshiro256**
	class RandProvider {
	private:
		uint64_t states_[4];

		uint32_t seed_;
		uint64_t _GenrandInt64();

		inline uint64_t rotl(uint64_t u, size_t x);
		inline uint64_t rotr(uint64_t u, size_t x);
	public:
		RandProvider();
		RandProvider(uint32_t s);
		virtual ~RandProvider() {}
		void Initialize(uint32_t s);

		uint32_t GetSeed() { return seed_; }
		int GetInt();
		int GetInt(int min, int max);
		int64_t GetInt64();
		int64_t GetInt64(int64_t min, int64_t max);
		double GetReal();
		double GetReal(double min, double max);
	};
}

#endif
