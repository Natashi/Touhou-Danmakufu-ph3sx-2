#pragma once
#include "../pch.h"

namespace gstd {
	class ArchiveEncryption {
	public:
		static constexpr const char* HEADER_ARCHIVEFILE = "DNHARC\0\0";

		//Change these to however you want.
		static const std::string ARCHIVE_ENCRYPTION_KEY;
		static void GetKeyHashHeader(const char* key, size_t keySize, byte& keyBase, byte& keyStep) {
			uint32_t hash = std::_Hash_array_representation(key, keySize);
			keyBase = (hash & 0x000000ff) ^ 0x55;
			keyStep = ((hash & 0x0000ff00) >> 8) ^ 0xc8;
		}
		static inline void GetKeyHashHeader(const std::string& str, byte& keyBase, byte& keyStep) {
			GetKeyHashHeader(str.c_str(), str.size(), keyBase, keyStep);
		}

		static void GetKeyHashFile(const char* key, size_t keySize, byte headerBase, byte headerStep,
			byte& keyBase, byte& keyStep) {
			uint32_t hash = std::_Hash_array_representation(key, keySize);
			keyBase = ((hash & 0xff000000) >> 24) ^ 0x4a;
			keyStep = ((hash & 0x00ff0000) >> 16) ^ 0xeb;
		}
		static inline void GetKeyHashFile(const std::string& str, byte headerBase, byte headerStep, byte& keyBase, byte& keyStep) {
			GetKeyHashFile(str.c_str(), str.size(), headerBase, headerStep, keyBase, keyStep);
		}

		static void ShiftBlock(byte* data, size_t count, byte& base, byte step) {
			for (size_t i = 0; i < count; ++i) {
				data[i] ^= base;
				base = (byte)(((uint32_t)base + (uint32_t)step) % 0x100);
			}
		}
	};
	inline const std::string ArchiveEncryption::ARCHIVE_ENCRYPTION_KEY = "Mima for Touhou 18";
}