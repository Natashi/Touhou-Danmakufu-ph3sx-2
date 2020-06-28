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
				Compressor::DeflateStream(file, fileArchive, entry->sizeFull, &countByte);
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
		if (!Compressor::DeflateStream(buf, fileArchive, totalSize, &countByte))
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
			ByteBuffer tmpBufInfo;
			tmpBufInfo.SetSize(header.headerSize);
			file_.read(tmpBufInfo.GetPointer(), header.headerSize);

			ArchiveEncryption::ShiftBlock((byte*)tmpBufInfo.GetPointer(), header.headerSize, keyBase_, keyStep_);
			Compressor::InflateStream(tmpBufInfo, bufInfo, header.headerSize, &headerSizeTrue);
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

			ByteBuffer rawBuf;
			rawBuf.SetSize(entry->sizeStored);
			file->read(rawBuf.GetPointer(), entry->sizeStored);

			byte keyBase = entry->keyBase;
			ArchiveEncryption::ShiftBlock((byte*)rawBuf.GetPointer(), entry->sizeStored,
				keyBase, entry->keyStep);

			{
				size_t sizeVerif = 0U;

				if (entry->sizeStored > 0)
					Compressor::InflateStream(rawBuf, *res, entry->sizeStored, &sizeVerif);

				if (sizeVerif != entry->sizeFull)
					Logger::WriteTop(StringUtility::Format(L"(Warning)CreateEntryBuffer: Archive entry not properly read. "
						"[expected %d bytes, got %d bytes -> %s]", entry->name.c_str()));
			}

			res->Seek(0);
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

bool Compressor::Deflate(const size_t chunk,
	std::function<size_t(char*, size_t, int*)>&& ReadFunction,
	std::function<void(char*, size_t)>&& WriteFunction,
	std::function<void(size_t)>&& AdvanceFunction,
	std::function<bool()>&& CheckFunction,
	size_t* res) 
{
	bool ret = true;

	char* in = new char[chunk];
	char* out = new char[chunk];

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
			size_t read = ReadFunction(in, chunk, &flushType);

			if (read > 0) {
				stream.next_in = (Bytef*)in;
				stream.avail_in = read;

				do {
					stream.next_out = (Bytef*)out;
					stream.avail_out = chunk;

					returnState = deflate(&stream, flushType);

					size_t availWrite = chunk - stream.avail_out;
					countBytes += availWrite;
					if (returnState != Z_STREAM_ERROR)
						WriteFunction(out, availWrite);
					else throw returnState;
				} while (stream.avail_out == 0);
			}

			AdvanceFunction(read);
		} while (CheckFunction() && flushType != Z_FINISH);
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
bool Compressor::Inflate(const size_t chunk,
	std::function<size_t(char*, size_t)>&& ReadFunction,
	std::function<void(char*, size_t)>&& WriteFunction,
	std::function<void(size_t)>&& AdvanceFunction,
	std::function<bool()>&& CheckFunction,
	size_t* res) 
{
	bool ret = true;

	char* in = new char[chunk];
	char* out = new char[chunk];

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
			read = ReadFunction(in, chunk);

			if (read > 0U) {
				stream.next_in = (Bytef*)in;
				stream.avail_in = read;

				do {
					stream.next_out = (Bytef*)out;
					stream.avail_out = chunk;

					returnState = inflate(&stream, Z_NO_FLUSH);
					switch (returnState) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
					case Z_STREAM_ERROR:
						throw returnState;
					}

					size_t availWrite = chunk - stream.avail_out;
					countBytes += availWrite;
					WriteFunction(out, availWrite);
				} while (stream.avail_out == 0);
			}

			AdvanceFunction(read);
		} while (CheckFunction() && read > 0U);
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

#define DEF_COMP_ADVANCE_CHECK_FUNCS \
	auto _AdvanceFunc = [&](size_t advancing) { \
		count -= advancing; \
	}; \
	auto _StreamEndCheckFunc = [&]() -> bool { \
		return count > 0U; \
	};
bool Compressor::DeflateStream(in_stream_t& bufIn, out_stream_t& bufOut, size_t count, size_t* res) {
	auto _ReadFunc = [&](char* _bIn, size_t reading, int* _flushType) -> size_t {
		bufIn.read(_bIn, reading);
		size_t read = bufIn.gcount();
		if (read > count) {
			*_flushType = Z_FINISH;
			read = count;
		}
		else if (read < reading) {
			*_flushType = Z_FINISH;
		}
		return read;
	};
	auto _WriteFunc = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	DEF_COMP_ADVANCE_CHECK_FUNCS
	return Deflate(BASIC_CHUNK, _ReadFunc, _WriteFunc, _AdvanceFunc, _StreamEndCheckFunc, res);
}
bool Compressor::DeflateStream(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count, size_t* res) {
	size_t readPos = 0;
	auto _ReadFunc = [&](char* _bIn, size_t reading, int* _flushType) -> size_t {
		size_t read = std::min(reading, count);
		if (read > count) {
			*_flushType = Z_FINISH;
			read = count;
		}
		else if (read < reading) {
			*_flushType = Z_FINISH;
		}
		memcpy(_bIn, bufIn.GetPointer(readPos), read);
		readPos += read;
		return read;
	};
	auto _WriteFunc = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	DEF_COMP_ADVANCE_CHECK_FUNCS
	return Deflate(BASIC_CHUNK, _ReadFunc, _WriteFunc, _AdvanceFunc, _StreamEndCheckFunc, res);
}
bool Compressor::InflateStream(in_stream_t& bufIn, out_stream_t& bufOut, size_t count, size_t* res) {
	auto _ReadFunc = [&](char* _bIn, size_t reading) -> size_t {
		bufIn.read(_bIn, reading);
		size_t read = bufIn.gcount();
		return read > count ? count : read;
	};
	auto _WriteFunc = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	DEF_COMP_ADVANCE_CHECK_FUNCS
	return Inflate(BASIC_CHUNK, _ReadFunc, _WriteFunc, _AdvanceFunc, _StreamEndCheckFunc, res);
}
bool Compressor::InflateStream(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count, size_t* res) {
	size_t readPos = 0;
	auto _ReadFunc = [&](char* _bIn, size_t reading) -> size_t {
		size_t read = std::min(reading, count);
		memcpy(_bIn, bufIn.GetPointer(readPos), read);
		readPos += read;
		return read;
	};
	auto _WriteFunc = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	DEF_COMP_ADVANCE_CHECK_FUNCS
	return Inflate(BASIC_CHUNK, _ReadFunc, _WriteFunc, _AdvanceFunc, _StreamEndCheckFunc, res);
}
bool Compressor::InflateStream(in_stream_t& bufIn, ByteBuffer& bufOut, size_t count, size_t* res) {
	size_t readPos = 0;
	auto _ReadFunc = [&](char* _bIn, size_t reading) -> size_t {
		bufIn.read(_bIn, reading);
		size_t read = bufIn.gcount();
		return read > count ? count : read;
	};
	auto _WriteFunc = [&](char* _bOut, size_t writing) {
		bufOut.Write(_bOut, writing);
	};
	DEF_COMP_ADVANCE_CHECK_FUNCS
		return Inflate(BASIC_CHUNK, _ReadFunc, _WriteFunc, _AdvanceFunc, _StreamEndCheckFunc, res);
}
bool Compressor::InflateStream(ByteBuffer& bufIn, ByteBuffer& bufOut, size_t count, size_t* res) {
	size_t readPos = 0;
	auto _ReadFunc = [&](char* _bIn, size_t reading) -> size_t {
		size_t read = std::min(reading, count);
		memcpy(_bIn, bufIn.GetPointer(readPos), read);
		readPos += read;
		return read;
	};
	auto _WriteFunc = [&](char* _bOut, size_t writing) {
		bufOut.Write(_bOut, writing);
	};
	DEF_COMP_ADVANCE_CHECK_FUNCS
	return Inflate(BASIC_CHUNK, _ReadFunc, _WriteFunc, _AdvanceFunc, _StreamEndCheckFunc, res);
}