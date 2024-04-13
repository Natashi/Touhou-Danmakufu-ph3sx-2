#include "source/GcLib/pch.h"

#include "CpuInformation.hpp"

#include "GstdUtility.hpp"

using namespace gstd;

// https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
CpuInformation::CpuInformation() {
	std::array<int32_t, 4> cpui{};

	// Get highest valid value for processor information
	__cpuid(cpui.data(), 0);
	nIds = cpui[0];

	// Get highest valid value for extended processor information
	__cpuid(cpui.data(), 0x80000000);
	nExIds = cpui[0];

	// Read all basic information
	for (int i = 0; i <= nIds; ++i) {
		__cpuidex(cpui.data(), i, 0);
		data.push_back(cpui);
	}

	// Read all extended information
	for (int i = 0x80000000; i <= nExIds; ++i) {
		__cpuidex(cpui.data(), i, 0);
		extData.push_back(cpui);
	}

	// Interpret vendor string
	{
		char vendor[0x20];
		memset(vendor, 0, sizeof(vendor));

		memcpy(&vendor[0], &data[0][1], sizeof(int32_t));
		memcpy(&vendor[4], &data[0][3], sizeof(int32_t));
		memcpy(&vendor[8], &data[0][2], sizeof(int32_t));

		this->vendor = vendor;

		bIntel = this->vendor == "GenuineIntel";
		bAMD = this->vendor == "AuthenticAMD";
	}

	// Read family, model, type, stepping
	{
		auto eax = data[1][0];

		stepping = eax & 0xf;
		model = (eax >> 4) & 0xf;
		family = (eax >> 8) & 0xf;
		type = (eax >> 12) & 0x3;

		int extModel = (eax >> 16) & 0xf;
		int extFamily = (eax >> 20) & 0xff;

		if (family == 0x0f)
			family += extFamily;
		if (model == 0x06 || model == 0x0f)
			model += extModel;
	}

	// load bitset with flags for function 0x00000001
	if (nIds >= 1) {
		f1_ecx = data[1][2];
		f1_edx = data[1][3];
	}

	// load bitset with flags for function 0x00000007
	if (nIds >= 7) {
		f7_ebx = data[7][1];
		f7_ecx = data[7][2];
	}

	// load bitset with flags for function 0x80000001
	if (nExIds >= 0x80000001) {
		f81_ecx = extData[1][2];
		f81_edx = extData[1][3];
	}

	if (nExIds >= 0x80000004) {
		char brand[0x40];

		memcpy(brand + 0x00, extData[2].data(), sizeof(cpui));
		memcpy(brand + 0x10, extData[3].data(), sizeof(cpui));
		memcpy(brand + 0x20, extData[4].data(), sizeof(cpui));
		memset(brand + 0x30, 0, 0x10);

		this->brand = brand;
	}
}
