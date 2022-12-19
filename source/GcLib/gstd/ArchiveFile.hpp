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

		std::wstring path;
		TypeCompression compressionType;
		uint32_t sizeFull;
		uint32_t sizeStored;
		uint32_t offsetPos;
		byte keyBase;
		byte keyStep;

		//Not stored into archive files, only for engine usage
		ArchiveFile* archiveParent;
		std::wstring fullPath;	//Without module dir

		const size_t GetRecordSize() {
			return (path.size() * sizeof(wchar_t) + sizeof(uint32_t)	//string + length
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
	public:
		using CbSetStatus = std::function<void(const std::wstring&)>;
		using CbSetProgress = std::function<void(float)>;
	private:
		std::list<shared_ptr<ArchiveFileEntry>> listEntry_;
	public:
		FileArchiver();
		virtual ~FileArchiver();

		void AddEntry(shared_ptr<ArchiveFileEntry> entry) { listEntry_.push_back(entry); }
		bool CreateArchiveFile(const std::wstring& baseDir, const std::wstring& pathArchive, 
			CbSetStatus cbStatus, CbSetProgress cbProgress);

		bool EncryptArchive(std::fstream& inSrc, const std::wstring& pathOut, 
			ArchiveFileHeader* header, byte keyBase, byte keyStep);
	};

	//*******************************************************************
	//ArchiveFile
	//*******************************************************************
	class ArchiveFile {
	public:
		using EntryMap = std::multimap<std::wstring, ArchiveFileEntry>;
		using EntryMapIterator = EntryMap::iterator;
	private:
		std::wstring basePath_;
		std::wstring baseDir_;

		shared_ptr<File> file_;
		size_t globalReadOffset_;
		uint8_t keyBase_;
		uint8_t keyStep_;

		EntryMap mapEntry_;
	public:
		ArchiveFile(const std::wstring& path, size_t readOffset);
		virtual ~ArchiveFile();

		bool OpenFile();

		bool Open();
		void Close();

		shared_ptr<File> GetFile() { return file_; }
		const std::wstring& GetPath() { return basePath_; }
		const std::wstring& GetBaseDirectory() { return baseDir_; }

		EntryMap& GetEntryMap() { return mapEntry_; }

		bool IsExists(const std::wstring& name, EntryMapIterator* out = nullptr);
		std::set<std::wstring> GetFileList();
		ArchiveFileEntry* GetEntryByPath(const std::wstring& name);
		
		static shared_ptr<ByteBuffer> CreateEntryBuffer(ArchiveFileEntry* entry);
	};

	//*******************************************************************
	//Compressor
	//*******************************************************************
	class Compressor {
		using in_stream_t = std::basic_istream<char, std::char_traits<char>>;
		using out_stream_t = std::basic_ostream<char, std::char_traits<char>>;
		
		static constexpr const size_t BASIC_CHUNK = (size_t)(1 << 16);
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