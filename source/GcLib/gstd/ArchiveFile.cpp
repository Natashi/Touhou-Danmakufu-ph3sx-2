#include "source/GcLib/pch.h"
#include "ArchiveFile.hpp"

#include "Logger.hpp"

#include "CompressorStream.hpp"

using namespace gstd;

//*******************************************************************
//ArchiveFileEntry
//*******************************************************************
void ArchiveFileEntry::_WriteEntryRecord(std::stringstream& buf) {
	uint32_t nameSize = path.size();
	buf.write((char*)&nameSize, sizeof(uint32_t));
	buf.write((char*)path.c_str(), nameSize * sizeof(wchar_t));

	buf.write((char*)&compressionType, sizeof(TypeCompression));
	buf.write((char*)&sizeFull, sizeof(uint32_t));
	buf.write((char*)&sizeStored, sizeof(uint32_t));
	buf.write((char*)&offsetPos, sizeof(uint32_t));

	buf.write((char*)&keyBase, sizeof(uint8_t));
	buf.write((char*)&keyStep, sizeof(uint8_t));
}
void ArchiveFileEntry::_ReadEntryRecord(std::stringstream& buf) {
	uint32_t pathSize = 0U;
	buf.read((char*)&pathSize, sizeof(uint32_t));
	path.resize(pathSize);
	buf.read((char*)path.c_str(), pathSize * sizeof(wchar_t));

	buf.read((char*)&compressionType, sizeof(TypeCompression));
	buf.read((char*)&sizeFull, sizeof(uint32_t));
	buf.read((char*)&sizeStored, sizeof(uint32_t));
	buf.read((char*)&offsetPos, sizeof(uint32_t));

	buf.read((char*)&keyBase, sizeof(uint8_t));
	buf.read((char*)&keyStep, sizeof(uint8_t));
}

//*******************************************************************
//FileArchiver
//*******************************************************************
FileArchiver::FileArchiver() {
}
FileArchiver::~FileArchiver() {
}

bool FileArchiver::CreateArchiveFile(const std::wstring& baseDir, const std::wstring& pathArchive,
	CbSetStatus cbStatus, CbSetProgress cbProgress)
{
	bool res = true;

	if (cbStatus)
		cbStatus(L"");
	if (cbProgress)
		cbProgress(0.0f);

	if (cbStatus)
		cbStatus(L"Writing header");

	uint8_t headerKeyBase = 0;
	uint8_t headerKeyStep = 0;
	ArchiveEncryption::GetKeyHashHeader(ArchiveEncryption::ARCHIVE_ENCRYPTION_KEY, headerKeyBase, headerKeyStep);

	std::wstring pathTmp = StringUtility::Format(L"%s_tmp", pathArchive.c_str());
	std::fstream fileArchiveTmp;
	fileArchiveTmp.open(pathTmp, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

	if (!fileArchiveTmp.is_open()) {
		throw gstd::wexception(StringUtility::Format(L"Cannot create an archive at [%s].",
			pathArchive.c_str()).c_str());
	}

	try {
		ArchiveFileHeader header{};

		memcpy(header.magic, ArchiveEncryption::HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH);
		header.version = DATA_VERSION_ARCHIVE;
		header.entryCount = listEntry_.size();
		//header.headerCompressed = true;
		header.headerOffset = 0U;
		header.headerSize = 0U;

		fileArchiveTmp.write((char*)&header, sizeof(ArchiveFileHeader));

		if (cbProgress)
			cbProgress(0.1f);
		float progressStep = (0.75f - 0.10f) / (float)listEntry_.size();

		std::streampos sDataBegin = fileArchiveTmp.tellp();

		size_t iEntry = 0;
		//Write the files and record their information.
		for (auto& entry : listEntry_) {
			if (cbStatus) {
				std::wstring name = entry->path;
				cbStatus(StringUtility::Format(L"Processing [%s]", name.c_str()));
			}

			std::wstring filePath = baseDir + entry->path;

			std::ifstream file;
			file.open(filePath, std::ios::binary);
			if (!file.is_open())
				throw gstd::wexception(StringUtility::Format(L"Cannot open file for reading. [%s]", entry->path.c_str()));

			file.seekg(0, std::ios::end);
			entry->sizeFull = file.tellg();
			entry->sizeStored = entry->sizeFull;
			entry->offsetPos = fileArchiveTmp.tellp();
			file.seekg(0, std::ios::beg);

			byte localKeyBase = 0;
			byte localKeyStep = 0;
			{
				std::wstring strHash = StringUtility::Format(L"%s%u",
					filePath.c_str(), entry->sizeFull ^ 0xe54f077a);
				ArchiveEncryption::GetKeyHashFile(StringUtility::ConvertWideToMulti(strHash).c_str(),
					headerKeyBase, headerKeyStep, localKeyBase, localKeyStep);
			}

			entry->keyBase = localKeyBase;
			entry->keyStep = localKeyStep;

			if (entry->sizeFull > 0) {
				//Small files actually get bigger upon compression.
				if (entry->sizeFull < 0x100) entry->compressionType = ArchiveFileEntry::CT_NONE;

				switch (entry->compressionType) {
				case ArchiveFileEntry::CT_NONE:
				{
					fileArchiveTmp << file.rdbuf();
					break;
				}
				case ArchiveFileEntry::CT_ZLIB:
				{
					size_t countByte = 0U;
					if (auto oRes = CompressorStream::Deflate(file, fileArchiveTmp, entry->sizeFull)) {
						countByte = *oRes;
					}
					else {
						throw gstd::wexception("CompressorStream::Deflate failed");
					}

					entry->sizeStored = countByte;
					break;
				}
				}
			}

			file.close();

			if (cbProgress)
				cbProgress(0.1f + progressStep * iEntry);

			++iEntry;
		}

		std::streampos sOffsetInfoBegin = fileArchiveTmp.tellp();
		fileArchiveTmp.flush();

		if (cbStatus)
			cbStatus(L"Writing entries info");
		progressStep = (0.95f - 0.75f) / (float)listEntry_.size();

		//Write the info header at the end, always compressed.
		{
			fileArchiveTmp.seekp(sOffsetInfoBegin, std::ios::beg);

			std::stringstream buf;
			size_t totalSize = 0U;

			iEntry = 0;
			for (auto& entry : listEntry_) {
				uint32_t sz = entry->GetRecordSize();
				buf.write((char*)&sz, sizeof(uint32_t));	//Write the size of the entry
				entry->_WriteEntryRecord(buf);

				totalSize += sz + sizeof(uint32_t);

				if (cbProgress)
					cbProgress(0.75f + progressStep * iEntry);

				++iEntry;
			}

			size_t countByte = totalSize;
			buf.seekg(0, std::ios::beg);

			if (!CompressorStream::Deflate(buf, fileArchiveTmp, totalSize))
				throw gstd::wexception("Failed to compress archive header.");
			//fileArchiveTmp << buf.rdbuf();

			header.headerSize = countByte;
			//header.headerSize = totalSize;
			header.headerOffset = sOffsetInfoBegin;
		}

		if (cbStatus)
			cbStatus(L"Encrypting archive");
		if (cbProgress)
			cbProgress(0.95f);

		fileArchiveTmp.seekp(offsetof(ArchiveFileHeader, headerOffset));
		fileArchiveTmp.write((char*)&header.headerOffset, sizeof(uint32_t));
		fileArchiveTmp.write((char*)&header.headerSize, sizeof(uint32_t));

		/*
		if (true) {
			std::ofstream fileTestOutput(StringUtility::Format(L"%s_head.txt", pathArchive.c_str()), std::ios::trunc);

			fileTestOutput << StringUtility::Format("File count:     %d\n", header.entryCount);
			fileTestOutput << StringUtility::Format("Header offset:  %d\n", header.headerOffset);
			fileTestOutput << StringUtility::Format("Header size:    %d\n\n", header.headerSize);

			size_t i = 0;
			for (auto itr = listEntry_.begin(); itr != listEntry_.end(); ++itr, ++i) {
				shared_ptr<ArchiveFileEntry> entry = *itr;
				fileTestOutput << StringUtility::Format("%d: %s\n", i,
					StringUtility::ConvertWideToMulti(entry->path).c_str());
				fileTestOutput << StringUtility::Format("\tCompression:  %d\n", entry->compressionType);
				fileTestOutput << StringUtility::Format("\tFull size:    %d\n", entry->sizeFull);
				fileTestOutput << StringUtility::Format("\tStored size:  %d\n", entry->sizeStored);
				fileTestOutput << StringUtility::Format("\tOffset:       %d\n", entry->offsetPos);
			}

			fileTestOutput.close();
		}
		*/

		res = EncryptArchive(fileArchiveTmp, pathArchive, &header, headerKeyBase, headerKeyStep);
		fileArchiveTmp.close();
	}
	catch (std::exception& e) {
		::DeleteFileW(pathTmp.c_str());
		throw e;
	}

	//::DeleteFileW(pathTmp.c_str());

	if (cbStatus)
		cbStatus(L"Done");
	if (cbProgress)
		cbProgress(1.0f);

	return res;
}

bool FileArchiver::EncryptArchive(std::fstream& inSrc, const std::wstring& pathOut, 
	ArchiveFileHeader* header,
	byte keyBase, byte keyStep) 
{
	if (!inSrc.is_open()) return false;
	inSrc.clear();
	inSrc.seekg(0);

	std::ofstream dest;
	dest.open(pathOut, std::ios::binary | std::ios::trunc);

	constexpr size_t CHUNK = 16384U;
	auto buf = make_unique<char[]>(CHUNK);
	auto pBuf = buf.get();

	size_t read = 0U;
	byte headerBase = keyBase;

	{
		inSrc.read(pBuf, sizeof(ArchiveFileHeader));
		ArchiveEncryption::ShiftBlock((byte*)pBuf, sizeof(ArchiveFileHeader), headerBase, keyStep);
		dest.write(pBuf, sizeof(ArchiveFileHeader));
	}

	{
		for (auto& entry : listEntry_) {
			size_t count = entry->sizeStored;

			byte localBase = entry->keyBase;

			inSrc.clear();
			inSrc.seekg(entry->offsetPos, std::ios::beg);
			dest.seekp(entry->offsetPos, std::ios::beg);

			do {
				inSrc.read(pBuf, CHUNK);
				read = inSrc.gcount();
				if (read > count) read = count;

				ArchiveEncryption::ShiftBlock((byte*)pBuf, read, localBase, entry->keyStep);

				dest.write(pBuf, read);
				count -= read;
			} while (count > 0U && read > 0U);
		}
	}

	inSrc.clear();

	{
		size_t infoSize = header->headerSize;

		inSrc.seekg(header->headerOffset, std::ios::beg);
		dest.seekp(header->headerOffset, std::ios::beg);

		do {
			inSrc.read(pBuf, CHUNK);
			read = inSrc.gcount();
			if (read > infoSize) read = infoSize;

			ArchiveEncryption::ShiftBlock((byte*)pBuf, read, headerBase, keyStep);

			dest.write(pBuf, read);
			infoSize -= read;
		} while (infoSize > 0U && read > 0U);
	}

	dest.close();

	return true;
}

//*******************************************************************
//ArchiveFile
//*******************************************************************
ArchiveFile::ArchiveFile(const std::wstring& path, size_t readOffset) {
	file_.reset(new File(path));

	basePath_ = path;
	baseDir_ = PathProperty::GetDirectoryWithoutModuleDirectory(path);
	baseDir_ = PathProperty::ReplaceYenToSlash(baseDir_);
	baseDir_ = PathProperty::AppendSlash(baseDir_);

	globalReadOffset_ = readOffset;
}
ArchiveFile::~ArchiveFile() {
	Close();
}

bool ArchiveFile::OpenFile() {
	if (!file_->IsOpen()) {
		bool res = file_->Open(File::AccessType::READ);
		return res;
	}
	return true;
}
bool ArchiveFile::Open() {
	if (!this->OpenFile())
		return false;
	if (mapEntry_.size() != 0)
		return true;

	std::fstream& stream = file_->GetFileHandle();

	/*
	std::wstring fileTestPath = StringUtility::Format(L"temp/%s_head_out.txt", PathProperty::GetFileName(basePath_));
	File::CreateFileDirectory(fileTestPath);
	std::ofstream fileTestOutput(fileTestPath, std::ios::trunc);
	*/

	bool res = true;
	try {
		ArchiveEncryption::GetKeyHashHeader(ArchiveEncryption::ARCHIVE_ENCRYPTION_KEY, keyBase_, keyStep_);

		ArchiveFileHeader header{};

		stream.seekg(globalReadOffset_, std::ios::beg);
		stream.read((char*)&header, sizeof(ArchiveFileHeader));
		ArchiveEncryption::ShiftBlock((byte*)&header, sizeof(ArchiveFileHeader), keyBase_, keyStep_);

		if (memcmp(header.magic, ArchiveEncryption::HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH) != 0) {
			Logger::WriteError("File is not a ph3sx data archive");
			throw wexception();
		}
		if (header.version != DATA_VERSION_ARCHIVE) {
			Logger::WriteError("Archive version not compatible with engine version");
			throw wexception();
		}

		uint32_t headerSizeTrue = 0U;

		std::stringstream bufInfo;
		stream.seekg(globalReadOffset_ + header.headerOffset, std::ios::beg);
		{
			ByteBuffer tmpBufInfo(header.headerSize);
			stream.read(tmpBufInfo.GetPointer(), header.headerSize);

			ArchiveEncryption::ShiftBlock((byte*)tmpBufInfo.GetPointer(), header.headerSize, keyBase_, keyStep_);

			if (auto oSize = CompressorStream::Inflate(tmpBufInfo, bufInfo, header.headerSize)) {
				headerSizeTrue = *oSize;
			}
			else {
				Logger::WriteError("Failed to decompress archive header");
				throw wexception();
			}
		}

		bufInfo.clear();
		bufInfo.seekg(0, std::ios::beg);

		/*
		{
			fileTestOutput << StringUtility::Format("File count:     %d\n", header.entryCount);
			fileTestOutput << StringUtility::Format("Header offset:  %d\n", header.headerOffset);
			fileTestOutput << StringUtility::Format("Header size:    %d\n\n", header.headerSize);
		}
		*/

		stream.clear();

		for (size_t iEntry = 0U; iEntry < header.entryCount; ++iEntry) {
			ArchiveFileEntry entry;

			uint32_t sizeEntry = 0U; 
			bufInfo.read((char*)&sizeEntry, sizeof(uint32_t));

			entry._ReadEntryRecord(bufInfo);

			/*
			{
				fileTestOutput << StringUtility::Format("%d: [%s] [%s]\n", iEntry,
					StringUtility::ConvertWideToMulti(entry->directory).c_str(),
					StringUtility::ConvertWideToMulti(entry->name).c_str());
				fileTestOutput << StringUtility::Format("\tCompression:  %d\n", entry->compressionType);
				fileTestOutput << StringUtility::Format("\tFull size:    %d\n", entry->sizeFull);
				fileTestOutput << StringUtility::Format("\tStored size:  %d\n", entry->sizeStored);
				fileTestOutput << StringUtility::Format("\tOffset:       %d\n", entry->offsetPos);
			}
			*/

			mapEntry_.insert(std::make_pair(baseDir_ + entry.path, entry));
		}

		res = true;
	}
	catch (...) {
		res = false;
	}

	//fileTestOutput.close();
	return res;
}
void ArchiveFile::Close() {
	file_->Close();
	mapEntry_.clear();
}

std::set<std::wstring> ArchiveFile::GetFileList() {
	std::set<std::wstring> res;
	for (auto& [path, entry] : mapEntry_)
		res.insert(entry.path);
	return res;
}
optional<ArchiveFileEntry*> ArchiveFile::GetEntryByPath(const std::wstring& name) {
	auto itr = mapEntry_.find(name);
	if (itr != mapEntry_.end())
		return &itr->second;
	return {};
}

unique_ptr<ByteBuffer> ArchiveFile::CreateEntryBuffer(ArchiveFileEntry* entry) {
	unique_ptr<ByteBuffer> res;

	// Archive file somehow closed, try to reopen
	if (!file_->IsOpen())
		OpenFile();

	std::fstream& stream = file_->GetFileHandle();
	if (stream.is_open()) {
		switch (entry->compressionType) {
		case ArchiveFileEntry::CT_NONE:
		{
			stream.seekg(globalReadOffset_ + entry->offsetPos, std::ios::beg);

			res.reset(new ByteBuffer(entry->sizeFull));

			stream.read(res->GetPointer(), entry->sizeFull);

			byte keyBase = entry->keyBase;
			ArchiveEncryption::ShiftBlock((byte*)res->GetPointer(), entry->sizeFull,
				keyBase, entry->keyStep);

			break;
		}
		case ArchiveFileEntry::CT_ZLIB:
		{
			stream.seekg(globalReadOffset_ + entry->offsetPos, std::ios::beg);

			res.reset(new ByteBuffer());

			ByteBuffer rawBuf(entry->sizeStored);
			stream.read(rawBuf.GetPointer(), entry->sizeStored);

			byte keyBase = entry->keyBase;
			ArchiveEncryption::ShiftBlock((byte*)rawBuf.GetPointer(), entry->sizeStored,
				keyBase, entry->keyStep);

			{
				size_t sizeVerif = 0U;

				if (entry->sizeStored > 0) {
					if (auto oSize = CompressorStream::Inflate(rawBuf, *res, entry->sizeStored)) {
						sizeVerif = *oSize;
					}
				}

				if (sizeVerif != entry->sizeFull) {
					Logger::WriteTop(StringUtility::Format(
						L"CreateEntryBuffer: Archive entry not properly read; entry might be corrupted\r\n"
						L"\t[%s] -> expected %d bytes, read %d bytes",
						entry->path.c_str(), entry->sizeFull, sizeVerif));
				}
			}

			res->Seek(0);
			break;
		}
		}
		stream.clear();

		if (false) {
			std::wstring nameTmp = entry->path;
			std::wstring pathTest = StringUtility::Format(L"temp/arch_buf_%d_%s", entry->sizeFull, nameTmp.c_str());
			File file(pathTest);
			File::CreateFileDirectory(pathTest);
			file.Open(File::WRITEONLY);
			file.Write(res->GetPointer(), entry->sizeFull);
			file.Close();
		}
	}
	else {
		Logger::WriteTop(StringUtility::Format(
			L"CreateEntryBuffer: Cannot open archive file for reading.\r\n"
			L"\t[%s] in [%s]", entry->fullPath.c_str(), 
			PathProperty::ReduceModuleDirectory(GetPath()).c_str()));
	}

	return res;
}
/*
ref_count_ptr<ByteBuffer> ArchiveFile::GetBuffer(std::string name)
{
	if(!IsExists(name)) return NULL;

	if(!file_->Open()) return NULL;

	ref_count_ptr<ByteBuffer> res = new ByteBuffer();
	ref_count_ptr<ArchiveFileEntry> entry = mapEntry_[name];
	int offset = entry->GetOffset();
	int size = entry->GetDataSize();

	res->SetSize(size);
	file_->Seek(offset);
	file_->Read(res->GetPointer(), size);

	file_->Close();
	return res;
}
*/
