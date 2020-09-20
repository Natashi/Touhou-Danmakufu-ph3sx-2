#pragma once

#include "../pch.h"

namespace gstd {
	//================================================================
	//Vectorize
	class Vectorize {
	public:
		static __forceinline __m128 Load(const D3DXVECTOR4& vec);
		static __forceinline __m128 Load(float* const ptr);
		static __forceinline void Store(float* const ptr, const __m128& dst);

		static __forceinline __m128 Set(float a, float b, float c, float d);
		static __forceinline __m128i Set(int a, int b, int c, int d);
		static __forceinline __m256 Set(float a, float b, float c, float d, float e, float f, float g, float h);

		static __forceinline __m128 Replicate(float x);
		static __forceinline __m128i Replicate(int x);

		static __forceinline __m128 Add(const __m128& a, const __m128& b);
		static __forceinline __m128 Sub(const __m128& a, const __m128& b);
		static __forceinline __m128 Mul(const __m128& a, const __m128& b);
		static __forceinline __m128 Div(const __m128& a, const __m128& b);
		static __forceinline __m256 Mul(const __m256& a, const __m256& b);

		static __forceinline __m128 MulAdd(const __m128& a, const __m128& b, const __m128& c);
		static __forceinline __m128d MulAdd(const __m128d& a, const __m128d& b, const __m128d& c);

		static __forceinline __m128i MaxPacked(const __m128i& a, const __m128i& b);
		static __forceinline __m128i MinPacked(const __m128i& a, const __m128i& b);
		static __forceinline __m128i ClampPacked(const __m128i& a, const __m128i& min, const __m128i& max);
	};

	__m128 Vectorize::Load(const D3DXVECTOR4& vec) {
		return Load((float*)&vec);
	}
	__m128 Vectorize::Load(float* const ptr) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		memcpy(res.m128_f32, ptr, sizeof(__m128));
#else
		res = _mm_loadu_ps(ptr);
#endif
		return res;
	}
	void Vectorize::Store(float* const ptr, const __m128& dst) {
#ifndef __L_MATH_VECTORIZE
		memcpy(ptr, &dst, sizeof(__m128));
#else
		_mm_storeu_ps(ptr, dst);
#endif
	}
	__m128 Vectorize::Set(float a, float b, float c, float d) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = a;
		res.m128_f32[1] = b;
		res.m128_f32[2] = c;
		res.m128_f32[3] = d;
#else
		res = _mm_set_ps(d, c, b, a);
#endif
		return res;
	}
	__m128i Vectorize::Set(int a, int b, int c, int d) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		res.m128i_i32[0] = a;
		res.m128i_i32[1] = b;
		res.m128i_i32[2] = c;
		res.m128i_i32[3] = d;
#else
		res = _mm_set_epi32(d, c, b, a);
#endif
		return res;
	}
	__m256 Vectorize::Set(float a, float b, float c, float d, float e, float f, float g, float h) {
		__m256 res;
#ifndef __L_MATH_VECTORIZE
		res.m256_f32[0] = a; res.m256_f32[1] = b;
		res.m256_f32[2] = c; res.m256_f32[3] = d;
		res.m256_f32[4] = e; res.m256_f32[5] = f;
		res.m256_f32[6] = g; res.m256_f32[7] = h;
#else
		res = _mm256_set_ps(h, g, f, e, d, c, b, a);
#endif
		return res;
	}
	__m128 Vectorize::Replicate(float x) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = x;
		res.m128_f32[1] = x;
		res.m128_f32[2] = x;
		res.m128_f32[3] = x;
#else
		res = _mm_set_ps1(x);
#endif
		return res;
	}
	__m128i Vectorize::Replicate(int x) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		res.m128i_i32[0] = x;
		res.m128i_i32[1] = x;
		res.m128i_i32[2] = x;
		res.m128i_i32[3] = x;
#else
		res = _mm_set1_epi32(x);
#endif
		return res;
	}
	__m128 Vectorize::Add(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] += b.m128_f32[i];
#else
		res = _mm_add_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::Sub(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] -= b.m128_f32[i];
#else
		res = _mm_sub_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::Mul(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] *= b.m128_f32[i];
#else
		res = _mm_mul_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::Div(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] /= b.m128_f32[i];
#else
		res = _mm_div_ps(a, b);
#endif
		return res;
	}
	__m256 Vectorize::Mul(const __m256& a, const __m256& b) {
		__m256 res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 8; ++i)
			res.m256_f32[i] *= b.m256_f32[i];
#else
		res = _mm256_mul_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::MulAdd(const __m128& a, const __m128& b, const __m128& c) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] = fma(res.m128_f32[i], b.m128_f32[i], c.m128_f32[i]);
#else
		res = _mm_mul_ps(a, b);
		res = _mm_add_ps(res, c);
#endif
		return res;
	}
	__m128d Vectorize::MulAdd(const __m128d& a, const __m128d& b, const __m128d& c) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = fma(res.m128d_f64[i], b.m128d_f64[i], c.m128d_f64[i]);
#else
		res = _mm_mul_pd(a, b);
		res = _mm_add_pd(res, c);
#endif
		return res;
	}
	__m128i Vectorize::MaxPacked(const __m128i& a, const __m128i& b) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128i_i32[i] = std::max(res.m128i_i32[i], b.m128i_i32[i]);
#else
		res = _mm_maskz_min_epi32(0xff, a, b);
#endif
		return res;
	}
	__m128i Vectorize::MinPacked(const __m128i& a, const __m128i& b) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128i_i32[i] = std::min(res.m128i_i32[i], b.m128i_i32[i]);
#else
		res = _mm_maskz_max_epi32(0xff, a, b);
#endif
		return res;
	}
	__m128i Vectorize::ClampPacked(const __m128i& a, const __m128i& min, const __m128i& max) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		res = a;
		for (int i = 0; i < 4; ++i)
			res.m128i_i32[i] = std::clamp(res.m128i_i32[i], min.m128i_i32[i], max.m128i_i32[i]);
#else
		res = _mm_maskz_max_epi32(0xff, a, min);
		res = _mm_maskz_min_epi32(0xff, res, max);
#endif
		return res;
	}
}