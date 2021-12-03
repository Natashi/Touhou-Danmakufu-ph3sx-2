#pragma once

#include "../pch.h"

#include "File.hpp"
#include "ArchiveEncryption.hpp"

namespace gstd {
	//*******************************************************************
	//ArchiveFileEntry
	//*******************************************************************
	class FileArchiver;
	class ArchiveFile;

#pragma pack(push, 1)
	struct ArchiveFileHeader {
		enum {
			MAGIC_LENGTH = 0x8,
		};

		char magic[MAGIC_LENGTH];
		uint64_t version;
		uint32_t entryCount;
		//uint8_t headerCompressed;
		uint32_t headerOffset;
		uint32_t headerSize;
	};

	class ArchiveFileEntry {
	public:
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

		ArchiveFile* archiveParent;

		const size_t GetRecordSize() {
			return ((directory.size() + name.size()) * sizeof(wchar_t) + sizeof(uint32_t) * 2 
				+ sizeof(TypeCompression) + sizeof(uint32_t) * 3 + sizeof(byte) * 2);
		}

		void _WriteEntryRecord(std::stringstream& buf);
		void _ReadEntryRecord(std::stringstream& buf);
	};
#pragma pack(pop)

	//*******************************************************************
	//FileArchiver
	//*******************************************************************
	class WStatusBar;
	class WProgressBar;
	class FileArchiver {
	private:
		std::list<shared_ptr<ArchiveFileEntry>> listEntry_;
	public:
		FileArchiver();
		virtual ~FileArchiver();

		void AddEntry(shared_ptr<ArchiveFileEntry> entry) { listEntry_.push_back(entry); }
		bool CreateArchiveFile(const std::wstring& path, WStatusBar* pStatus, WProgressBar* pProgress);

		bool EncryptArchive(std::fstream& inSrc, const std::wstring& pathOut, ArchiveFileHeader* header, 
			byte keyBase, byte keyStep);
	};

	//*******************************************************************
	//ArchiveFile
	//*******************************************************************
	class ArchiveFile {
	private:
		std::wstring basePath_;
		std::ifstream file_;
		std::multimap<std::wstring, shared_ptr<ArchiveFileEntry>> mapEntry_;
		uint8_t keyBase_;
		uint8_t keyStep_;

		size_t globalReadOffset_;
	public:
		ArchiveFile(std::wstring path, size_t readOffset);
		virtual ~ArchiveFile();

		bool Open();
		void Close();

		std::ifstream& GetFile() { return file_; }
		std::wstring& GetPath() { return basePath_; }

		std::set<std::wstring> GetKeyList();
		std::multimap<std::wstring, shared_ptr<ArchiveFileEntry>>& GetEntryMap() { return mapEntry_; }
		std::vector<shared_ptr<ArchiveFileEntry>> GetEntryList(const std::wstring& name);
		bool IsExists(const std::wstring& name);
		static shared_ptr<ByteBuffer> CreateEntryBuffer(shared_ptr<ArchiveFileEntry> entry);
		//ref_count_ptr<ByteBuffer> GetBuffer(std::string name);
	};

	//*******************************************************************
	//Compressor
	//*******************************************************************
	class Compressor {
		using in_stream_t = std::basic_istream<char, std::char_traits<char>>;
		using out_stream_t = std::basic_ostream<char, std::char_traits<char>>;
		enum : size_t {
			BASIC_CHUNK = 65536U,
		};
	public:
		static bool Deflate(const size_t chunk, 
			std::function<size_t(char*, size_t, int*)>&& ReadFunction,
			std::function<void(char*, size_t)>&& WriteFunction,
			std::function<void(size_t)>&& AdvanceFunction,
			std::function<bool()>&& CheckFunction,
			size_t* res);
		static bool DeflateStream(in_stream_t& bufIn, out_stream_t& bufOut, size_t count, size_t* res);
		static bool DeflateStream(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count, size_t* res);

		static bool Inflate(const size_t chunk,
			std::function<size_t(char*, size_t)>&& ReadFunction,
			std::function<void(char*, size_t)>&& WriteFunction,
			std::function<void(size_t)>&& AdvanceFunction,
			std::function<bool()>&& CheckFunction,
			size_t* res);
		static bool InflateStream(in_stream_t& bufIn, out_stream_t& bufOut, size_t count, size_t* res);
		static bool InflateStream(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count, size_t* res);
		static bool InflateStream(in_stream_t& bufIn, ByteBuffer& bufOut, size_t count, size_t* res);
		static bool InflateStream(ByteBuffer& bufIn, ByteBuffer& bufOut, size_t count, size_t* res);
	};
}