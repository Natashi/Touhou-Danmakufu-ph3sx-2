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
		uint32_t version;
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

		ArchiveFileEntry() :
			compressionType(CT_NONE), 
			sizeFull(0), sizeStored(0), offsetPos(0), 
			keyBase(0), keyStep(0) { }
		ArchiveFileEntry(const std::wstring& path) :
			path(path),
			compressionType(CT_NONE), 
			sizeFull(0), sizeStored(0), offsetPos(0), 
			keyBase(0), keyStep(0) { }

		std::wstring fullPath;	// Without module dir

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
	class FileArchiver {
	public:
		using CbSetStatus = std::function<void(const std::wstring&)>;
		using CbSetProgress = std::function<void(float)>;
	private:
		std::list<unique_ptr<ArchiveFileEntry>> listEntry_;
	public:
		FileArchiver();
		virtual ~FileArchiver();

		void AddEntry(unique_ptr<ArchiveFileEntry>&& entry) { listEntry_.push_back(MOVE(entry)); }
		bool CreateArchiveFile(const std::wstring& baseDir, const std::wstring& pathArchive, 
			CbSetStatus cbStatus, CbSetProgress cbProgress);

		bool EncryptArchive(std::fstream& inSrc, const std::wstring& pathOut, 
			ArchiveFileHeader* header, byte keyBase, byte keyStep);
	};

	//*******************************************************************
	//ArchiveFile
	//*******************************************************************
	class ArchiveFile {
		std::wstring basePath_;
		std::wstring baseDir_;

		shared_ptr<File> file_;
		size_t globalReadOffset_;
		uint8_t keyBase_;
		uint8_t keyStep_;

		std::map<std::wstring, ArchiveFileEntry> mapEntry_;
	public:
		ArchiveFile(const std::wstring& path, size_t readOffset);
		virtual ~ArchiveFile();

		bool OpenFile();

		bool Open();
		void Close();

		shared_ptr<File> GetFile() { return file_; }
		const std::wstring& GetPath() { return basePath_; }
		const std::wstring& GetBaseDirectory() { return baseDir_; }

		auto& GetEntryMap() { return mapEntry_; }

		std::set<std::wstring> GetFileList();
		optional<ArchiveFileEntry*> GetEntryByPath(const std::wstring& name);
		
		unique_ptr<ByteBuffer> CreateEntryBuffer(ArchiveFileEntry* entry);
	};
}