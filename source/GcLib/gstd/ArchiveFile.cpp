#include "source/GcLib/pch.h"
#include "ArchiveFile.hpp"

#include "Logger.hpp"

using namespace gstd;

/**********************************************************
//ArchiveFileEntry
**********************************************************/
void ArchiveFileEntry::_WriteEntryRecord(std::stringstream& buf) {
	uint32_t directorySize = directory.size();
	buf.write((char*)&directorySize, sizeof(uint32_t));
	buf.write((char*)directory.c_str(), directorySize * sizeof(wchar_t));

	uint32_t nameSize = name.size();
	buf.write((char*)&nameSize, sizeof(uint32_t));
	buf.write((char*)name.c_str(), nameSize * sizeof(wchar_t));

	buf.write((char*)&compressionType, sizeof(TypeCompression));
	buf.write((char*)&sizeFull, sizeof(uint32_t));
	buf.write((char*)&sizeStored, sizeof(uint32_t));
	buf.write((char*)&offsetPos, sizeof(uint32_t));

	buf.write((char*)&keyBase, sizeof(uint8_t));
	buf.write((char*)&keyStep, sizeof(uint8_t));
}
void ArchiveFileEntry::_ReadEntryRecord(std::stringstream& buf) {
	uint32_t directorySize = 0U;
	buf.read((char*)&directorySize, sizeof(uint32_t));
	directory.resize(directorySize);
	buf.read((char*)directory.c_str(), directorySize * sizeof(wchar_t));

	uint32_t nameSize = 0U;
	buf.read((char*)&nameSize, sizeof(uint32_t));
	name.resize(nameSize);
	buf.read((char*)name.c_str(), nameSize * sizeof(wchar_t));

	buf.read((char*)&compressionType, sizeof(TypeCompression));
	buf.read((char*)&sizeFull, sizeof(uint32_t));
	buf.read((char*)&sizeStored, sizeof(uint32_t));
	buf.read((char*)&offsetPos, sizeof(uint32_t));

	buf.read((char*)&keyBase, sizeof(uint8_t));
	buf.read((char*)&keyStep, sizeof(uint8_t));
}

/**********************************************************
//FileArchiver
**********************************************************/
FileArchiver::FileArchiver() {

}
FileArchiver::~FileArchiver() {

}

//-------------------------------------------------------------------------------------------------------------

bool FileArchiver::CreateArchiveFile(std::wstring path) {
	bool res = true;

	uint8_t headerKeyBase = 0;
	uint8_t headerKeyStep = 0;
	ArchiveEncryption::GetKeyHashHeader(ArchiveEncryption::ARCHIVE_ENCRYPTION_KEY, headerKeyBase, headerKeyStep);

	DeleteFile(path.c_str());

	std::wstring pathTmp = StringUtility::Format(L"%s_tmp", path.c_str());
	std::ofstream fileArchive;
	fileArchive.open(pathTmp, std::ios::binary);

	if (!fileArchive.is_open())
		throw gstd::wexception(StringUtility::Format(L"Cannot create an archive at [%s].", path.c_str()).c_str());

	ArchiveFileHeader header;
	memcpy(header.magic, ArchiveEncryption::HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH);
	header.entryCount = listEntry_.size();
	//header.headerCompressed = true;
	header.headerOffset = 0U;
	header.headerSize = 0U;

	fileArchive.write((char*)&header, sizeof(ArchiveFileHeader));

	std::streampos sDataBegin = fileArchive.tellp();

	//Write the files and record their information.
	for (auto itr = listEntry_.begin(); itr != listEntry_.end(); ++itr) {
		ArchiveFileEntry::ptr entry = *itr;

		std::ifstream file;
		file.open(entry->name, std::ios::binary);
		if (!file.is_open())
			throw gstd::wexception(StringUtility::Format(L"Cannot open file for reading. [%s]", (entry->name).c_str()).c_str());

		file.seekg(0, std::ios::end);
		entry->sizeFull = file.tellg();
		entry->sizeStored = entry->sizeFull;
		entry->offsetPos = fileArchive.tellp();
		file.seekg(0, std::ios::beg);

		byte localKeyBase = 0;
		byte localKeyStep = 0;
		{
			std::wstring strHash = StringUtility::Format(L"%s%s", entry->directory.c_str(), entry->name.c_str());
			ArchiveEncryption::GetKeyHashFile(StringUtility::ConvertWideToMulti(strHash).c_str(), 
				headerKeyBase, headerKeyStep, localKeyBase, localKeyStep);
		}

		entry->keyBase = localKeyBase;
		entry->keyStep = localKeyStep;

		if (entry->sizeFull > 0) {
			//Small files actually get bigger upon compression.
			if (entry->sizeFull < 0x80) entry->compressionType = ArchiveFileEntry::CT_NONE;

			switch (entry->compressionType) {
			case ArchiveFileEntry::CT_NONE:
			{
				fileArchive << file.rdbuf();
				break;
			}
			case ArchiveFileEntry::CT_ZLIB:
			{
				size_t countByte = 0U;
				Compressor::Deflate(file, fileArchive, entry->sizeFull, &countByte);
				entry->sizeStored = countByte;
				break;
			}
			}
		}

		file.close();
	}

	std::streampos sOffsetInfoBegin = fileArchive.tellp();

	fileArchive.flush();

	//Write the info header at the end, always compressed.
	{
		fileArchive.seekp(sOffsetInfoBegin, std::ios::beg);

		std::stringstream buf;
		size_t totalSize = 0U;

		for (auto itr = listEntry_.begin(); itr != listEntry_.end(); ++itr) {
			ArchiveFileEntry::ptr entry = *itr;

			std::wstring name = entry->name;
			entry->name = PathProperty::GetFileName(name);

			uint32_t sz = entry->GetRecordSize();
			buf.write((char*)&sz, sizeof(uint32_t));
			entry->_WriteEntryRecord(buf);

			entry->name = name;
			totalSize += sz + sizeof(uint32_t);
		}

		size_t countByte = totalSize;
		buf.seekg(0, std::ios::beg);
		if (!Compressor::Deflate(buf, fileArchive, totalSize, &countByte))
			throw gstd::wexception("Failed to compress archive header.");
		//fileArchive << buf.rdbuf();

		header.headerSize = countByte;
		//header.headerSize = totalSize;
		header.headerOffset = sOffsetInfoBegin;
	}

	fileArchive.seekp(ArchiveFileHeader::MAGIC_LENGTH + 4);
	fileArchive.write((char*)&header.headerOffset, sizeof(uint32_t));
	fileArchive.write((char*)&header.headerSize, sizeof(uint32_t));

	fileArchive.close();

	/*
	if (true) {
		std::ofstream fileTestOutput(StringUtility::Format(L"%s_head.txt", path.c_str()), std::ios::trunc);

		fileTestOutput << StringUtility::Format("File count:     %d\n", header.entryCount);
		fileTestOutput << StringUtility::Format("Header offset:  %d\n", header.headerOffset);
		fileTestOutput << StringUtility::Format("Header size:    %d\n\n", header.headerSize);

		size_t i = 0;
		for (auto itr = listEntry_.begin(); itr != listEntry_.end(); ++itr, ++i) {
			ArchiveFileEntry::ptr entry = *itr;
			std::string pathCom = StringUtility::ConvertWideToMulti(entry->directory + entry->name);
			fileTestOutput << StringUtility::Format("%d: [%s] [%s]\n", i, 
				StringUtility::ConvertWideToMulti(entry->directory).c_str(),
				StringUtility::ConvertWideToMulti(PathProperty::GetFileName(entry->name)).c_str());
			fileTestOutput << StringUtility::Format("\tCompression:  %d\n", entry->compressionType);
			fileTestOutput << StringUtility::Format("\tFull size:    %d\n", entry->sizeFull);
			fileTestOutput << StringUtility::Format("\tStored size:  %d\n", entry->sizeStored);
			fileTestOutput << StringUtility::Format("\tOffset:       %d\n", entry->offsetPos);
		}

		fileTestOutput.close();
	}
	*/

	res = EncryptArchive(path, &header, headerKeyBase, headerKeyStep);
	return res;
}

bool FileArchiver::EncryptArchive(std::wstring path, ArchiveFileHeader* header, byte keyBase, byte keyStep) {
	std::wstring pathTmp = StringUtility::Format(L"%s_tmp", path.c_str());

	std::ifstream src;
	src.open(pathTmp, std::ios::binary);

	std::ofstream dest;
	dest.open(path, std::ios::binary | std::ios::trunc);

	constexpr size_t CHUNK = 16384U;
	char buf[CHUNK];

	size_t read = 0U;
	byte headerBase = keyBase;

	{
		src.read(buf, sizeof(ArchiveFileHeader));
		ArchiveEncryption::ShiftBlock((byte*)buf, sizeof(ArchiveFileHeader), headerBase, keyStep);
		dest.write(buf, sizeof(ArchiveFileHeader));
	}

	{
		for (auto itr = listEntry_.begin(); itr != listEntry_.end(); ++itr) {
			auto entry = *itr;
			size_t count = entry->sizeStored;

			byte localBase = entry->keyBase;

			src.clear();
			src.seekg(entry->offsetPos, std::ios::beg);
			dest.seekp(entry->offsetPos, std::ios::beg);

			do {
				src.read(buf, CHUNK);
				read = src.gcount();
				if (read > count) read = count;

				ArchiveEncryption::ShiftBlock((byte*)buf, read, localBase, entry->keyStep);

				dest.write(buf, read);
				count -= read;
			} while (count > 0U && read > 0U);
		}
	}

	src.clear();

	{
		size_t infoSize = header->headerSize;

		src.seekg(header->headerOffset, std::ios::beg);
		dest.seekp(header->headerOffset, std::ios::beg);

		do {
			src.read(buf, CHUNK);
			read = src.gcount();
			if (read > infoSize) read = infoSize;

			ArchiveEncryption::ShiftBlock((byte*)buf, read, headerBase, keyStep);

			dest.write(buf, read);
			infoSize -= read;
		} while (infoSize > 0U && read > 0U);
	}

	src.close();
	dest.close();

	DeleteFile(pathTmp.c_str());

	return true;
}

/**********************************************************
//ArchiveFile
**********************************************************/
ArchiveFile::ArchiveFile(std::wstring path) {
	file_.open(path, std::ios::binary);
	basePath_ = path;
}
ArchiveFile::~ArchiveFile() {
	Close();
}
bool ArchiveFile::Open() {
	if (!file_.is_open()) return false;
	if (mapEntry_.size() != 0) return true;

	/*
	std::wstring fileTestPath = StringUtility::Format(L"temp/%s_head_out.txt", PathProperty::GetFileName(basePath_));
	File::CreateFileDirectory(fileTestPath);
	std::ofstream fileTestOutput(fileTestPath, std::ios::trunc);
	*/

	bool res = true;
	try {
		ArchiveEncryption::GetKeyHashHeader(ArchiveEncryption::ARCHIVE_ENCRYPTION_KEY, keyBase_, keyStep_);

		ArchiveFileHeader header;

		file_.read((char*)&header, sizeof(ArchiveFileHeader));
		ArchiveEncryption::ShiftBlock((byte*)&header, sizeof(ArchiveFileHeader), keyBase_, keyStep_);

		if (memcmp(header.magic, ArchiveEncryption::HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH) != 0) 
			throw gstd::wexception();

		uint32_t headerSizeTrue = 0U;

		std::stringstream bufInfo;
		file_.seekg(header.headerOffset, std::ios::beg);
		{
			char* tmpBufInfo = new char[header.headerSize];
			file_.read(tmpBufInfo, header.headerSize);

			ArchiveEncryption::ShiftBlock((byte*)tmpBufInfo, header.headerSize, keyBase_, keyStep_);

			std::stringstream sTmpBufInfo;
			sTmpBufInfo.write(tmpBufInfo, header.headerSize);
			Compressor::Inflate(sTmpBufInfo, bufInfo, header.headerSize, &headerSizeTrue);

			delete[] tmpBufInfo;
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

		file_.clear();

		for (size_t iEntry = 0U; iEntry < header.entryCount; ++iEntry) {
			ArchiveFileEntry::ptr entry = std::make_shared<ArchiveFileEntry>();

			uint32_t sizeEntry = 0U; 
			bufInfo.read((char*)&sizeEntry, sizeof(uint32_t));

			entry->_ReadEntryRecord(bufInfo);
			entry->archiveParent = this;

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

			mapEntry_.insert(std::pair<std::wstring, ArchiveFileEntry::ptr>(entry->name, entry));
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
	file_.close();
	mapEntry_.clear();
}
std::set<std::wstring> ArchiveFile::GetKeyList() {
	std::set<std::wstring> res;
	for (auto itr = mapEntry_.begin(); itr != mapEntry_.end(); itr++) {
		ArchiveFileEntry::ptr entry = itr->second;
		//std::wstring key = entry->GetDirectory() + entry->GetName();
		res.insert(entry->name);
	}
	return res;
}
std::vector<ArchiveFileEntry::ptr> ArchiveFile::GetEntryList(std::wstring name) {
	std::vector<ArchiveFileEntry::ptr> res;
	if (!IsExists(name))return res;

	for (auto itrPair = mapEntry_.equal_range(name); itrPair.first != itrPair.second; itrPair.first++) {
		ArchiveFileEntry::ptr entry = (itrPair.first)->second;
		res.push_back(entry);
	}

	return res;
}
bool ArchiveFile::IsExists(std::wstring name) {
	return mapEntry_.find(name) != mapEntry_.end();
}
ref_count_ptr<ByteBuffer> ArchiveFile::CreateEntryBuffer(ArchiveFileEntry::ptr entry) {
	ref_count_ptr<ByteBuffer> res;

	std::ifstream* file = &entry->archiveParent->GetFile();
	if (file->is_open()) {
		switch (entry->compressionType) {
		case ArchiveFileEntry::CT_NONE:
		{
			file->seekg(entry->offsetPos, std::ios::beg);
			res = new ByteBuffer();
			res->SetSize(entry->sizeFull);
			file->read(res->GetPointer(), entry->sizeFull);

			byte keyBase = entry->keyBase;
			ArchiveEncryption::ShiftBlock((byte*)res->GetPointer(), entry->sizeFull,
				keyBase, entry->keyStep);

			break;
		}
		case ArchiveFileEntry::CT_ZLIB:
		{
			file->seekg(entry->offsetPos, std::ios::beg);
			res = new ByteBuffer();
			//res->SetSize(entry->sizeFull);

			char* tmp = new char[entry->sizeStored];
			file->read(tmp, entry->sizeStored);

			byte keyBase = entry->keyBase;
			ArchiveEncryption::ShiftBlock((byte*)tmp, entry->sizeStored,
				keyBase, entry->keyStep);

			std::stringstream streamTmp;
			streamTmp.write(tmp, entry->sizeStored);
			delete[] tmp;

			size_t sizeVerif = 0U;

			std::stringstream stream;
			if (entry->sizeStored > 0)
				Compressor::Inflate(streamTmp, stream, entry->sizeStored, &sizeVerif);
			res->Copy(stream);

			if (sizeVerif != entry->sizeFull)
				Logger::WriteTop(StringUtility::Format(L"(Warning)CreateEntryBuffer: Archive entry not properly read. "
					"[expected %d bytes, got %d bytes -> %s]", entry->name.c_str()));

			break;
		}
		}
		file->clear();

		if (false) {
			std::wstring nameTmp = entry->name;
			std::wstring pathTest = StringUtility::Format(L"temp/arch_buf_%d_%s", entry->sizeFull, nameTmp.c_str());
			File file(pathTest);
			File::CreateFileDirectory(pathTest);
			file.Open(File::WRITEONLY);
			file.Write(res->GetPointer(), entry->sizeFull);
			file.Close();
		}
	}
	else {
		Logger::WriteTop(StringUtility::Format(L"(Error)CreateEntryBuffer: Archive file already unloaded. "
			"[%s in %s]", entry->name.c_str(), entry->archiveParent->GetPath().c_str()));
	}

	return res;
}
/*
ref_count_ptr<ByteBuffer> ArchiveFile::GetBuffer(std::string name)
{
	if(!IsExists(name))return NULL;

	if(!file_->Open())return NULL;

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