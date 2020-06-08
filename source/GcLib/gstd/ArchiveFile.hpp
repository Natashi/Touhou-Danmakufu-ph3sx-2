#pragma once

#include "../pch.h"

#include "File.hpp"
#include "ArchiveEncryption.hpp"

namespace gstd {
	/**********************************************************
	//ArchiveFileEntry
	**********************************************************/
	class FileArchiver;
	class ArchiveFile;

#pragma pack(push, 1)
	struct ArchiveFileHeader {
		enum {
			MAGIC_LENGTH = 0x8,
		};

		char magic[MAGIC_LENGTH];
		uint32_t entryCount;
		//uint8_t headerCompressed;
		uint32_t headerOffset;
		uint32_t headerSize;
	};

	class ArchiveFileEntry {
	public:
		using ptr = std::shared_ptr<ArchiveFileEntry>;

		enum TypeCompression : uint8_t {
			CT_NONE,
			CT_ZLIB,
		};

		//uint32_t directorySize;
		std::wstring directory;
		//uint32_t nameSize;
		std::wstring name;
		TypeCompression compressionType;
		uint32_t sizeFull;
		uint32_t sizeStored;
		uint32_t offsetPos;
		byte keyBase;
		byte keyStep;

		std::wstring archiveParent;

		const size_t GetRecordSize() {
			return static_cast<size_t>(directory.size() * sizeof(wchar_t) + name.size() * sizeof(wchar_t) +
				sizeof(uint32_t) * 5 + sizeof(TypeCompression) + sizeof(byte) * 2);
		}

		void _WriteEntryRecord(std::stringstream& buf);
		void _ReadEntryRecord(std::stringstream& buf);
	};
#pragma pack(pop)

	/**********************************************************
	//FileArchiver
	**********************************************************/
	class FileArchiver {
	private:
		std::list<ArchiveFileEntry::ptr> listEntry_;
	public:
		FileArchiver();
		virtual ~FileArchiver();

		void AddEntry(ArchiveFileEntry::ptr entry) { listEntry_.push_back(entry); }
		bool CreateArchiveFile(std::wstring path);

		bool EncryptArchive(std::wstring path, ArchiveFileHeader* header, byte keyBase, byte keyStep);
	};

	/**********************************************************
	//ArchiveFile
	**********************************************************/
	class ArchiveFile {
	private:
		std::wstring basePath_;
		std::ifstream file_;
		std::multimap<std::wstring, ArchiveFileEntry::ptr> mapEntry_;
		uint8_t keyBase_;
		uint8_t keyStep_;
	public:
		ArchiveFile(std::wstring path);
		virtual ~ArchiveFile();
		bool Open();
		void Close();

		std::set<std::wstring> GetKeyList();
		std::multimap<std::wstring, ArchiveFileEntry::ptr> GetEntryMap() { return mapEntry_; }
		std::vector<ArchiveFileEntry::ptr> GetEntryList(std::wstring name);
		bool IsExists(std::wstring name);
		static ref_count_ptr<ByteBuffer> CreateEntryBuffer(ArchiveFileEntry::ptr entry);
		//ref_count_ptr<ByteBuffer> GetBuffer(std::string name);
	};

	/**********************************************************
	//Compressor
	**********************************************************/
	class Compressor {
	public:
		template<typename TIN, typename TOUT>
		static bool Deflate(TIN& bufIn, TOUT& bufOut, size_t count, size_t* res) {
			bool ret = true;

			const size_t CHUNK = 65536U;
			char* in = new char[CHUNK];
			char* out = new char[CHUNK];

			int returnState = 0;
			size_t countBytes = 0U;

			z_stream stream;
			stream.zalloc = Z_NULL;
			stream.zfree = Z_NULL;
			stream.opaque = Z_NULL;
			returnState = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
			if (returnState != Z_OK) return false;

			try {
				int flushType = Z_NO_FLUSH;

				do {
					bufIn.read(in, CHUNK);
					size_t read = bufIn.gcount();
					if (read > count) {
						flushType = Z_FINISH;
						read = count;
					}
					else if (read < CHUNK) {
						flushType = Z_FINISH;
					}

					if (read > 0) {
						stream.next_in = (Bytef*)in;
						stream.avail_in = bufIn.gcount();

						do {
							stream.next_out = (Bytef*)out;
							stream.avail_out = CHUNK;

							returnState = deflate(&stream, flushType);

							size_t availWrite = CHUNK - stream.avail_out;
							countBytes += availWrite;
							if (returnState != Z_STREAM_ERROR)
								bufOut.write(out, availWrite);
							else throw returnState;
						} while (stream.avail_out == 0);
					}
					count -= read;
				} while (count > 0U && flushType != Z_FINISH);
			}
			catch (int&) {
				ret = false;
			}

			delete[] in;
			delete[] out;

			deflateEnd(&stream);
			if (res) *res = countBytes;
			return ret;
		}

		template<typename TIN, typename TOUT>
		static bool Inflate(TIN& bufIn, TOUT& bufOut, size_t count, size_t* res) {
			bool ret = true;

			const size_t CHUNK = 65536U;
			char* in = new char[CHUNK];
			char* out = new char[CHUNK];

			int returnState = 0;
			size_t countBytes = 0U;

			z_stream stream;
			stream.zalloc = Z_NULL;
			stream.zfree = Z_NULL;
			stream.opaque = Z_NULL;
			stream.avail_in = 0;
			stream.next_in = Z_NULL;
			returnState = inflateInit(&stream);
			if (returnState != Z_OK) return false;

			try {
				size_t read = 0U;

				do {
					bufIn.read(in, CHUNK);
					read = bufIn.gcount();
					if (read > count) read = count;

					if (read > 0U) {
						stream.avail_in = read;
						stream.next_in = (Bytef*)in;

						do {
							stream.next_out = (Bytef*)out;
							stream.avail_out = CHUNK;

							returnState = inflate(&stream, Z_NO_FLUSH);
							switch (returnState) {
							case Z_NEED_DICT:
							case Z_DATA_ERROR:
							case Z_MEM_ERROR:
							case Z_STREAM_ERROR:
								throw returnState;
							}

							size_t availWrite = CHUNK - stream.avail_out;
							countBytes += availWrite;
							bufOut.write(out, availWrite);
						} while (stream.avail_out == 0);
					}
					count -= read;
				} while (count > 0U && read > 0U);
			}
			catch (int&) {
				ret = false;
			}

			delete[] in;
			delete[] out;

			inflateEnd(&stream);
			if (res) *res = countBytes;
			return ret;
		}
	};
}