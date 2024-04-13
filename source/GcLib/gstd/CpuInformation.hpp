#pragma once

#include "../pch.h"

namespace gstd {
	class CpuInformation {
		int nIds, nExIds;

		std::string vendor, brand;
		bool bIntel, bAMD;

		int family, model, type, stepping;

		std::bitset<32> f1_ecx, f1_edx;
		std::bitset<32> f7_ebx, f7_ecx;
		std::bitset<32> f81_ecx, f81_edx;
		std::vector<std::array<int32_t, 4>> data, extData;
	public:
		CpuInformation();

		std::string_view Vendor() const { return vendor; }
		std::string_view Brand() const { return brand; }
		
		bool IsIntel() const { return bIntel; }
		bool IsAMD() const { return bAMD; }

		int Family() const { return family; }
		int Model() const { return model; }
		int Type() const { return type; }
		int Stepping() const { return stepping; }

#define _D_FLAG(_fn, _cond) bool Flag##_fn() const { return _cond; }
		_D_FLAG(SSE3,		f1_ecx[0]);
		_D_FLAG(PCLMULQDQ,	f1_ecx[1]);
		_D_FLAG(MONITOR,	f1_ecx[3]);
		_D_FLAG(SSSE3,		f1_ecx[9]);
		_D_FLAG(FMA,		f1_ecx[12]);
		_D_FLAG(CMPXCHG16B,	f1_ecx[13]);
		_D_FLAG(SSE41,		f1_ecx[19]);
		_D_FLAG(SSE42,		f1_ecx[20]);
		_D_FLAG(MOVBE,		f1_ecx[22]);
		_D_FLAG(POPCNT,		f1_ecx[23]);
		_D_FLAG(AES,		f1_ecx[25]);
		_D_FLAG(XSAVE,		f1_ecx[26]);
		_D_FLAG(OSXSAVE,	f1_ecx[27]);
		_D_FLAG(AVX,		f1_ecx[28]);
		_D_FLAG(F16C,		f1_ecx[29]);
		_D_FLAG(RDRAND,		f1_ecx[30]);

		_D_FLAG(MSR,		f1_edx[5]);
		_D_FLAG(CX8,		f1_edx[8]);
		_D_FLAG(SEP,		f1_edx[11]);
		_D_FLAG(CMOV,		f1_edx[15]);
		_D_FLAG(CLFSH,		f1_edx[19]);
		_D_FLAG(MMX,		f1_edx[23]);
		_D_FLAG(FXSR,		f1_edx[24]);
		_D_FLAG(SSE,		f1_edx[25]);
		_D_FLAG(SSE2,		f1_edx[26]);

		_D_FLAG(FSGSBASE,	f7_ebx[0]);
		_D_FLAG(BMI1,		f7_ebx[3]);
		_D_FLAG(HLE,		f7_ebx[4] && bIntel);
		_D_FLAG(AVX2,		f7_ebx[5]);
		_D_FLAG(BMI2,		f7_ebx[8]);
		_D_FLAG(ERMS,		f7_ebx[9]);
		_D_FLAG(INVPCID,	f7_ebx[10]);
		_D_FLAG(RTM,		f7_ebx[11] && bIntel);
		_D_FLAG(AVX512F,	f7_ebx[16]);
		_D_FLAG(RDSEED,		f7_ebx[18]);
		_D_FLAG(ADX,		f7_ebx[19]);
		_D_FLAG(AVX512PF,	f7_ebx[26]);
		_D_FLAG(AVX512ER,	f7_ebx[27]);
		_D_FLAG(AVX512CD,	f7_ebx[28]);
		_D_FLAG(SHA,		f7_ebx[29]);

		_D_FLAG(PREFETCHWT1,f7_ecx[0]);

		_D_FLAG(LAHF,		f81_ecx[0]);
		_D_FLAG(LZCNT,		f81_ecx[5] && bIntel);
		_D_FLAG(ABM,		f81_ecx[5] && bAMD);
		_D_FLAG(SSE4a,		f81_ecx[6] && bAMD);
		_D_FLAG(XOP,		f81_ecx[11] && bAMD);
		_D_FLAG(TBM,		f81_ecx[21] && bAMD);

		_D_FLAG(SYSCALL,	f81_edx[11] && bIntel);
		_D_FLAG(MMXEXT,		f81_edx[22] && bAMD);
		_D_FLAG(RDTSCP,		f81_edx[27] && bIntel);
		_D_FLAG(_3DNOWEXT,	f81_edx[30] && bAMD);
		_D_FLAG(_3DNOW,		f81_edx[31] && bAMD);
#undef _D_FLAG
	};
}
