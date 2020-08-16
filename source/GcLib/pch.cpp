#include "pch.h"

//SIMD mode
//#define __L_MATH_VECTORIZE	//<--- Macro definition moved to project
#ifdef __L_MATH_VECTORIZE
//SIMD checks
#if !defined(_XM_X64_) && !defined(_XM_X86_)
#if defined(_M_AMD64) || defined(_AMD64_)
#define _XM_X64_
#elif defined(_M_IX86) || defined(_X86_)
#define _XM_X86_
#endif
#endif

#if (!defined(_XM_X86_) && !defined(_XM_X64_))
#error Compilation on SIMD mode requires x86 intrinsics
#endif
#endif