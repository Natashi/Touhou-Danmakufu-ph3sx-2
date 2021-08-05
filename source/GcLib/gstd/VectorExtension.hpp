#pragma once

#include "../pch.h"

#if defined(DNH_PROJ_EXECUTOR)

namespace gstd {
#define _MM_SHUFFLE_R(fp0, fp1, fp2, fp3) _MM_SHUFFLE(fp3, fp2, fp1, fp0)

	//================================================================
	//Vectorize
	class Vectorize {
	public:
		//Loads vector from D3DXVECTOR4, alignment ignored
		static __forceinline __m128 Load(const D3DXVECTOR4& vec);
		//Loads vector from float pointer, alignment ignored
		static __forceinline __m128 Load(float* const ptr);
		//Loads double vector from double pointer, alignment ignored
		static __forceinline __m128d Load(double* const ptr);
		//Stores the data of vector "dst" into float array "ptr" (size=4)
		static __forceinline void Store(float* const ptr, const __m128& dst);

		//Creates vector (a, b, c, d)
		static __forceinline __m128 Set(float a, float b, float c, float d);
		//Creates double vector (a, b)
		static __forceinline __m128d Set(double a, double b);
		//Creates int vector (a, b, c, d)
		static __forceinline __m128i Set(int a, int b, int c, int d);
		//Creates uint vector (a, b, c, d)
		static __forceinline __m128i Set(unsigned int a, unsigned int b, unsigned int c, unsigned int d);

		//Set wrappers----------------------------------------------------------------
		static __forceinline __m128 SetF(float a, float b, float c, float d) { return Set(a, b, c, d); }
		static __forceinline __m128d SetD(double a, double b) { return Set(a, b); }
		static __forceinline __m128i SetI(int a, int b, int c, int d) { return Set(a, b, c, d); }
		static __forceinline __m128i SetU(unsigned int a, unsigned int b, unsigned int c, unsigned int d) { return Set(a, b, c, d); }
		//----------------------------------------------------------------------------

		//Generates vector (x, x, x, x)
		static __forceinline __m128 Replicate(float x);
		//Generates double vector (x, x)
		static __forceinline __m128d Replicate(double x);
		//Generates int vector (x, x, x, x)
		static __forceinline __m128i Replicate(int x);

		//Creates new vector from _MM_SHUFFLE
		template<byte SHUF>
		static __forceinline __m128 Shuffle(const __m128& a, const __m128& b);

		//Returns x[0]
		static __forceinline float ToF32(const __m128& x);

		//From vectors a and b, generate a new vector (b[2], b[3], a[2], a[3])
		static __forceinline __m128 DuplicateHL(const __m128& a, const __m128& b);
		//From vectors a and b, generate a new vector (a[0], a[1], b[0], b[1])
		static __forceinline __m128 DuplicateLH(const __m128& a, const __m128& b);
		//From vector x, generate a new vector (x[0], x[0], x[2], x[2])
		static __forceinline __m128 DuplicateEven(const __m128& x);
		//From vector x, generate a new vector (x[1], x[1], x[3], x[3])
		static __forceinline __m128 DuplicateOdd(const __m128& x);

		//[XOR] vector a and b
		static __forceinline __m128 XOR(const __m128& a, const __m128& b);

		//[add] vector a and b
		static __forceinline __m128 Add(const __m128& a, const __m128& b);
		//[subtract] vector a and b
		static __forceinline __m128 Sub(const __m128& a, const __m128& b);
		//[multiply] vector a and b
		static __forceinline __m128 Mul(const __m128& a, const __m128& b);
		//[divide] vector a and b
		static __forceinline __m128 Div(const __m128& a, const __m128& b);

		//Computes reciprocal of vector
		static __forceinline __m128 Rcp(const __m128& x);

		//[add] double vector a and b
		static __forceinline __m128d Add(const __m128d& a, const __m128d& b);
		//[subtract] double vector a and b
		static __forceinline __m128d Sub(const __m128d& a, const __m128d& b);
		//[multiply] double vector a and b
		static __forceinline __m128d Mul(const __m128d& a, const __m128d& b);
		//[divide] double vector a and b
		static __forceinline __m128d Div(const __m128d& a, const __m128d& b);

		//Alternating [add] and [subtract]-> (-, +, -, +)
		static __forceinline __m128 AddSub(const __m128& a, const __m128& b);
		//Fused [multiply]+[add]
		static __forceinline __m128 MulAdd(const __m128& a, const __m128& b, const __m128& c);
		//Fused [multiply]+[add] for double vector
		static __forceinline __m128d MulAdd(const __m128d& a, const __m128d& b, const __m128d& c);

		//Performs [max] on double vector a and b
		static __forceinline __m128d Max(const __m128d& a, const __m128d& b);
		//Performs [min] on double vector a and b
		static __forceinline __m128d Min(const __m128d& a, const __m128d& b);
		//Performs [clamp] on double vector a and b (Fused [min]+[max])
		static __forceinline __m128d Clamp(const __m128d& a, const __m128d& min, const __m128d& max);

		//Performs [max] on int vector a and b
		static __forceinline __m128i Max(const __m128i& a, const __m128i& b);
		//Performs [min] on int vector a and b
		static __forceinline __m128i Min(const __m128i& a, const __m128i& b);
		//Performs [clamp] on int vector a and b (Fused [min]+[max])
		static __forceinline __m128i Clamp(const __m128i& a, const __m128i& min, const __m128i& max);
	};

	//---------------------------------------------------------------------
	//Implementation
	//---------------------------------------------------------------------

	__m128 Vectorize::Load(const D3DXVECTOR4& vec) {
		return Load((float*)&vec);
	}
	__m128 Vectorize::Load(float* const ptr) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		memcpy(res.m128_f32, ptr, sizeof(__m128));
#else
		//SSE
		res = _mm_loadu_ps(ptr);
#endif
		return res;
	}
	__m128d Vectorize::Load(double* const ptr) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		memcpy(res.m128d_f64, ptr, sizeof(__m128d));
#else
		//SSE2
		res = _mm_loadu_pd(ptr);
#endif
		return res;
	}
	void Vectorize::Store(float* const ptr, const __m128& dst) {
#ifndef __L_MATH_VECTORIZE
		memcpy(ptr, &dst, sizeof(__m128));
#else
		//SSE
		_mm_storeu_ps(ptr, dst);
#endif
	}

	//---------------------------------------------------------------------

	__m128 Vectorize::Set(float a, float b, float c, float d) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = a;
		res.m128_f32[1] = b;
		res.m128_f32[2] = c;
		res.m128_f32[3] = d;
#else
		//SSE
		res = _mm_set_ps(d, c, b, a);
#endif
		return res;
	}
	__m128d Vectorize::Set(double a, double b) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		res.m128d_f64[0] = a;
		res.m128d_f64[1] = b;
#else
		//SSE
		res = _mm_set_pd(b, a);
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
		//SSE2
		res = _mm_set_epi32(d, c, b, a);
#endif
		return res;
	}
	__m128i Vectorize::Set(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
		__m128i res;
		res.m128i_u32[0] = a;
		res.m128i_u32[1] = b;
		res.m128i_u32[2] = c;
		res.m128i_u32[3] = d;
		return res;
	}

	//---------------------------------------------------------------------

	__m128 Vectorize::Replicate(float x) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = x;
		res.m128_f32[1] = x;
		res.m128_f32[2] = x;
		res.m128_f32[3] = x;
#else
		//SSE
		res = _mm_set_ps1(x);
#endif
		return res;
	}
	__m128d Vectorize::Replicate(double x) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		res.m128d_f64[0] = x;
		res.m128d_f64[1] = x;
#else
		//SSE
		res = _mm_set1_pd(x);
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
		//SSE2
		res = _mm_set1_epi32(x);
#endif
		return res;
	}

	template<byte SHUF>
	__m128 Shuffle(const __m128& a, const __m128& b) {
#ifndef __L_MATH_VECTORIZE
#define _SELECT(x, ctrl) (x).m128_f32[(ctrl) & 4];
		res.m128_f32[0] = _SELECT(a, shuf);
		res.m128_f32[1] = _SELECT(a, shuf >> 2);
		res.m128_f32[2] = _SELECT(b, shuf >> 4);
		res.m128_f32[3] = _SELECT(b, shuf >> 6);
#undef _SELECT
#else
		//SSE
		return _mm_shuffle_ps(a, b, SHUF);
#endif
	}

	float Vectorize::ToF32(const __m128& x) {
#ifndef __L_MATH_VECTORIZE
		return x.m128_f32[0];
#else
		//SSE
		return _mm_cvtss_f32(x);
#endif
	}

	//---------------------------------------------------------------------

	__m128 Vectorize::DuplicateHL(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = b.m128_f32[2];
		res.m128_f32[1] = b.m128_f32[3];
		res.m128_f32[2] = a.m128_f32[2];
		res.m128_f32[3] = a.m128_f32[3];
#else
		//SSE
		res = _mm_movehl_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::DuplicateLH(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = a.m128_f32[0];
		res.m128_f32[1] = a.m128_f32[1];
		res.m128_f32[2] = b.m128_f32[0];
		res.m128_f32[3] = b.m128_f32[1];
#else
		//SSE
		res = _mm_movelh_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::DuplicateEven(const __m128& x) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = x.m128_f32[0];
		res.m128_f32[1] = x.m128_f32[0];
		res.m128_f32[2] = x.m128_f32[2];
		res.m128_f32[3] = x.m128_f32[2];
#else
		//SSE3
		res = _mm_moveldup_ps(x);
#endif
		return res;
	}
	__m128 Vectorize::DuplicateOdd(const __m128& x) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		res.m128_f32[0] = x.m128_f32[1];
		res.m128_f32[1] = x.m128_f32[1];
		res.m128_f32[2] = x.m128_f32[3];
		res.m128_f32[3] = x.m128_f32[3];
#else
		//SSE3
		res = _mm_movehdup_ps(x);
#endif
		return res;
	}

	//---------------------------------------------------------------------

	__m128 Vectorize::XOR(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i) {
			uint32_t s = (uint32_t&)a.m128_f32[i] ^ (uint32_t&)b.m128_f32[i];
			res.m128_f32[i] = (float&)s;
		}
#else
		//SSE
		res = _mm_xor_ps(a, b);
#endif
		return res;
	}

	//---------------------------------------------------------------------

	__m128 Vectorize::Add(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] = a.m128_f32[i] + b.m128_f32[i];
#else
		//SSE
		res = _mm_add_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::Sub(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] = a.m128_f32[i] - b.m128_f32[i];
#else
		//SSE
		res = _mm_sub_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::Mul(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] = a.m128_f32[i] * b.m128_f32[i];
#else
		//SSE
		res = _mm_mul_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::Div(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] = a.m128_f32[i] / b.m128_f32[i];
#else
		//SSE
		res = _mm_div_ps(a, b);
#endif
		return res;
	}

	__m128 Vectorize::Rcp(const __m128& x) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] = 1.0f / x.m128_f32[i];
#else
		//SSE
		res = _mm_rcp_ps(x);
#endif
		return res;
	}

	//---------------------------------------------------------------------

	__m128d Vectorize::Add(const __m128d& a, const __m128d& b) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = a.m128d_f64[i] + b.m128d_f64[i];
#else
		//SSE
		res = _mm_add_pd(a, b);
#endif
		return res;
	}
	__m128d Vectorize::Sub(const __m128d& a, const __m128d& b) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = a.m128d_f64[i] - b.m128d_f64[i];
#else
		//SSE
		res = _mm_sub_pd(a, b);
#endif
		return res;
	}
	__m128d Vectorize::Mul(const __m128d& a, const __m128d& b) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = a.m128d_f64[i] * b.m128d_f64[i];
#else
		//SSE
		res = _mm_mul_pd(a, b);
#endif
		return res;
	}
	__m128d Vectorize::Div(const __m128d& a, const __m128d& b) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = a.m128d_f64[i] / b.m128d_f64[i];
#else
		//SSE
		res = _mm_div_pd(a, b);
#endif
		return res;
	}

	//---------------------------------------------------------------------

	__m128 Vectorize::AddSub(const __m128& a, const __m128& b) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i) {
			if ((i & 0b1) == 0)
				res.m128_f32[i] = a.m128_f32[i] - b.m128_f32[i];
			else
				res.m128_f32[i] = a.m128_f32[i] + b.m128_f32[i];
		}
#else
		//SSE3
		res = _mm_addsub_ps(a, b);
#endif
		return res;
	}
	__m128 Vectorize::MulAdd(const __m128& a, const __m128& b, const __m128& c) {
		__m128 res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128_f32[i] = fma(a.m128_f32[i], b.m128_f32[i], c.m128_f32[i]);
#else
		//SSE
		res = _mm_mul_ps(a, b);
		res = _mm_add_ps(res, c);
#endif
		return res;
	}
	__m128d Vectorize::MulAdd(const __m128d& a, const __m128d& b, const __m128d& c) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = fma(a.m128d_f64[i], b.m128d_f64[i], c.m128d_f64[i]);
#else
		//SSE2
		res = _mm_mul_pd(a, b);
		res = _mm_add_pd(res, c);
#endif
		return res;
	}

	//---------------------------------------------------------------------

	__m128d Vectorize::Max(const __m128d& a, const __m128d& b) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = std::max(a.m128d_f64[i], b.m128d_f64[i]);
#else
		//SSE2
		res = _mm_min_pd(a, b);
#endif
		return res;
	}
	__m128d Vectorize::Min(const __m128d& a, const __m128d& b) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = std::min(a.m128d_f64[i], b.m128d_f64[i]);
#else
		//SSE2
		res = _mm_max_pd(a, b);
#endif
		return res;
	}
	__m128d Vectorize::Clamp(const __m128d& a, const __m128d& min, const __m128d& max) {
		__m128d res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 2; ++i)
			res.m128d_f64[i] = std::clamp(a.m128d_f64[i], min.m128d_f64[i], max.m128d_f64[i]);
#else
		//SSE2
		res = _mm_max_pd(a, min);
		res = _mm_min_pd(res, max);
#endif
		return res;
	}

	//---------------------------------------------------------------------

	__m128i Vectorize::Max(const __m128i& a, const __m128i& b) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128i_i32[i] = std::max(a.m128i_i32[i], b.m128i_i32[i]);
#else
		//SSE4.1
		res = _mm_min_epi32(a, b);
#endif
		return res;
	}
	__m128i Vectorize::Min(const __m128i& a, const __m128i& b) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128i_i32[i] = std::min(a.m128i_i32[i], b.m128i_i32[i]);
#else
		//SSE4.1
		res = _mm_max_epi32(a, b);
#endif
		return res;
	}
	__m128i Vectorize::Clamp(const __m128i& a, const __m128i& min, const __m128i& max) {
		__m128i res;
#ifndef __L_MATH_VECTORIZE
		for (int i = 0; i < 4; ++i)
			res.m128i_i32[i] = std::clamp(a.m128i_i32[i], min.m128i_i32[i], max.m128i_i32[i]);
#else
		//SSE4.1
		res = _mm_max_epi32(a, min);
		res = _mm_min_epi32(res, max);
#endif
		return res;
	}
}

#endif