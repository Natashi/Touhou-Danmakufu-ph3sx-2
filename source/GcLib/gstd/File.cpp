#include "source/GcLib/pch.h"

#include "File.hpp"
#include "GstdUtility.hpp"

#if defined(DNH_PROJ_EXECUTOR)
#include "Logger.hpp"
#endif

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
#include "ArchiveFile.hpp"
#endif

using namespace gstd;

/**********************************************************
//ByteBuffer
**********************************************************/
ByteBuffer::ByteBuffer() {
	data_ = nullptr;
	Clear();
}
ByteBuffer::ByteBuffer(ByteBuffer& buffer) {
	data_ = nullptr;
	Clear();
	Copy(buffer);
}
ByteBuffer::ByteBuffer(std::stringstream& stream) {
	data_ = nullptr;
	Clear();
	Copy(stream);
}
ByteBuffer::~ByteBuffer() {
	Clear();
	ptr_delete_scalar(data_);
}
size_t ByteBuffer::_GetReservedSize() {
	return reserve_;
}
void ByteBuffer::_Resize(size_t size) {
	char* oldData = data_;
	size_t oldSize = size_;

	data_ = new char[size];
	ZeroMemory(data_, size);

	//元のデータをコピー
	size_t sizeCopy = std::min(size, oldSize);
	memcpy(data_, oldData, sizeCopy);
	reserve_ = size;
	size_ = size;

	//古いデータを削除
	delete[] oldData;
}
void ByteBuffer::Copy(ByteBuffer& src) {
	if (data_ != nullptr && src.reserve_ != reserve_) {
		delete[] data_;
		data_ = new char[src.reserve_];
		ZeroMemory(data_, src.reserve_);
	}

	offset_ = src.offset_;
	reserve_ = src.reserve_;
	size_ = src.size_;

	memcpy(data_, src.data_, reserve_);
}
void ByteBuffer::Copy(std::stringstream& src) {
	std::streampos org = src.tellg();
	src.seekg(0, std::ios::end);
	size_t size = src.tellg();
	src.seekg(0, std::ios::beg);

	size_t newReserve = size > 0 ? 0x1u << (size_t)ceil(log2f(size)) : 1u;

	if (data_) {
		delete[] data_;
		data_ = new char[newReserve];
		ZeroMemory(data_, newReserve);
	}

	offset_ = org;
	reserve_ = newReserve;
	size_ = size;

	src.read(data_, size);
	src.clear();

	src.seekg(org, std::ios::beg);
}
void ByteBuffer::Clear() {
	ptr_delete_scalar(data_);
	data_ = new char[0];
	offset_ = 0;
	reserve_ = 0;
	size_ = 0;
}
void ByteBuffer::Seek(size_t pos) {
	offset_ = pos;
	if (offset_ > size_) offset_ = size_;
}
void ByteBuffer::SetSize(size_t size) {
	_Resize(size);
}
DWORD ByteBuffer::Write(LPVOID buf, DWORD size) {
	if (offset_ + size > reserve_) {
		size_t sizeNew = (offset_ + size) * 2;
		_Resize(sizeNew);
		size_ = 0;//あとで再計算
	}

	memcpy(&data_[offset_], buf, size);
	offset_ += size;
	size_ = std::max(size_, offset_);
	return size;
}
DWORD ByteBuffer::Read(LPVOID buf, DWORD size) {
	memcpy(buf, &data_[offset_], size);
	offset_ += size;
	return size;
}
_NODISCARD char* ByteBuffer::GetPointer(size_t offset) {
#if _DEBUG
	if (offset > size_)
		throw gstd::wexception(L"ByteBuffer: Index out of bounds.");
#endif
	return &data_[offset];
}
_NODISCARD char& ByteBuffer::operator[](size_t offset) {
#if _DEBUG
	if (offset > size_)
		throw gstd::wexception(L"ByteBuffer: Index out of bounds.");
#endif
	return data_[offset];
}
ByteBuffer& ByteBuffer::operator=(const ByteBuffer& other) noexcept {
	if (this != std::addressof(other)) {
		Clear();
		Copy(const_cast<ByteBuffer&>(other));
	}
	return (*this);
}

/**********************************************************
//File
**********************************************************/
File::File() {
	path_ = L"";
	perms_ = 0;
	bOpen_ = false;
}
File::File(const std::wstring& path) {
	path_ = path;
	perms_ = 0;
	bOpen_ = false;
}
File::~File() {
	Close();
}
bool File::CreateFileDirectory(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	stdfs::path dir = stdfs::path(path).parent_path();
	if (stdfs::exists(dir)) return true;

	return stdfs::create_directories(dir);
#else
	std::wstring dir = PathProperty::GetFileDirectory(path_);
	if (File::IsExists(dir)) return true;

	std::vector<std::wstring> str = StringUtility::Split(dir, L"\\/");
	std::wstring tPath = L"";
	for (int iDir = 0; iDir < str.size(); iDir++) {
		tPath += str[iDir] + L"\\";
		WIN32_FIND_DATA fData;
		HANDLE hFile = ::FindFirstFile(tPath.c_str(), &fData);
		if (hFile == INVALID_HANDLE_VALUE) {
			SECURITY_ATTRIBUTES attr;
			attr.nLength = sizeof(SECURITY_ATTRIBUTES);
			attr.lpSecurityDescriptor = nullptr;
			attr.bInheritHandle = FALSE;
			::CreateDirectory(tPath.c_str(), &attr);
		}
		::FindClose(hFile);
	}
#endif
	return true;
}
bool File::IsExists(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	path_t _p = path;
	bool res = stdfs::exists(_p) && (stdfs::status(_p).type() != stdfs::file_type::directory);
#else
	bool res = PathFileExists(path.c_str()) == TRUE;
#endif
	return res;
}
bool File::IsDirectory(const std::wstring& path) {
#ifdef __L_STD_FILESYSTEM
	path_t _p = path;
	bool res = stdfs::exists(_p) && stdfs::is_directory(_p);
#else
	WIN32_FIND_DATA fData;
	HANDLE hFile = ::FindFirstFile(path.c_str(), &fData);
	bool res = hFile != INVALID_HANDLE_VALUE ? true : false;
	if (res) res = (FILE_ATTRIBUTE_DIRECTORY & fData.dwFileAttributes) > 0;

	::FindClose(hFile);
#endif
	return res;
}
void File::Delete() {
	Close();
	::DeleteFile(path_.c_str());
}
bool File::IsExists() {
	if (bOpen_) return true;
	return IsExists(path_);
}
bool File::IsDirectory() {
	return IsDirectory(path_);
}
size_t File::GetSize() {
	size_t res = 0;
	if (bOpen_) {
		std::streampos org_off = hFile_.tellg();
		hFile_.seekg(0, std::ios::end);
		res = hFile_.tellg();
		hFile_.seekg(org_off, std::ios::beg);
		return res;
	}

	try {
		res = stdfs::file_size(path_);
	}
	catch (stdfs::filesystem_error&) {
		res = 0;
	}
	return res;
}

bool File::Open() {
	return this->Open(AccessType::READ);
}
bool File::Open(DWORD typeAccess) {
	this->Close();
	
	std::ios_base::openmode modeOpen = 0;

	if (typeAccess == AccessType::READ) modeOpen = std::ios::in;
	else if (typeAccess == AccessType::WRITE) modeOpen = std::ios::in | std::ios::out;
	else if (typeAccess == AccessType::WRITEONLY) modeOpen = std::ios::out | std::ios::trunc;

	if ((typeAccess & TEXTBASED) == 0) modeOpen |= std::ios::binary;

	if (modeOpen == 0) return false;
	perms_ = typeAccess;

	hFile_.open(path_, modeOpen);

	bOpen_ = hFile_.is_open();
	return bOpen_;
}
void File::Close() {
	if (bOpen_) hFile_.close();
	bOpen_ = false;
	perms_ = 0;
}

DWORD File::Read(LPVOID buf, DWORD size) {
	hFile_.read((char*)buf, size);
	hFile_.clear();
	return hFile_.gcount();
}
DWORD File::Write(LPVOID buf, DWORD size) {
	hFile_.write((const char*)buf, size);
	hFile_.clear();
	return size;
}
bool File::IsEqualsPath(const std::wstring& path1, const std::wstring& path2) {
#ifdef __L_STD_FILESYSTEM
	bool res = (path_t(path1) == path_t(path2));
#else
	path1 = PathProperty::GetUnique(path1);
	path2 = PathProperty::GetUnique(path2);
	bool res = (wcscmp(path1.c_str(), path2.c_str()) == 0);
#endif
	return res;
}
std::vector<std::wstring> File::GetFilePathList(const std::wstring& dir) {
	std::vector<std::wstring> res;

#ifdef __L_STD_FILESYSTEM
	path_t p = dir;
	if (stdfs::exists(p) && stdfs::is_directory(p)) {
		for (auto itr : stdfs::directory_iterator(p)) {
			if (!itr.is_directory())
				res.push_back(PathProperty::ReplaceYenToSlash(itr.path()));
		}
	}
#else
	WIN32_FIND_DATA data;
	HANDLE hFind;
	std::wstring findDir = dir + L"*.*";
	hFind = FindFirstFile(findDir.c_str(), &data);
	do {
		std::wstring name = data.cFileName;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(name != L".." && name != L".")) {
			//ディレクトリ
			std::wstring tDir = dir + name;
			tDir += L"\\";

			continue;
		}
		if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0)
			continue;

		//ファイル
		std::wstring path = dir + name;

		res.push_back(path);

	} while (FindNextFile(hFind, &data));
	FindClose(hFind);
#endif

	return res;
}
std::vector<std::wstring> File::GetDirectoryPathList(const std::wstring& dir) {
	std::vector<std::wstring> res;

#ifdef __L_STD_FILESYSTEM
	path_t p = dir;
	if (stdfs::exists(p) && stdfs::is_directory(p)) {
		for (auto itr : stdfs::directory_iterator(dir)) {
			if (itr.is_directory()) {
				std::wstring str = PathProperty::ReplaceYenToSlash(itr.path());
				str += L'/';
				res.push_back(str);
			}
		}
	}
#else
	WIN32_FIND_DATA data;
	HANDLE hFind;
	std::wstring findDir = dir + L"*.*";
	hFind = FindFirstFile(findDir.c_str(), &data);
	do {
		std::wstring name = data.cFileName;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(name != L".." && name != L".")) {
			//ディレクトリ
			std::wstring tDir = dir + name;
			tDir += L"\\";
			res.push_back(tDir);
			continue;
		}
		if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0)
			continue;

		//ファイル

	} while (FindNextFile(hFind, &data));
	FindClose(hFind);
#endif

	return res;
}

/**********************************************************
//FileManager
**********************************************************/
FileManager* FileManager::thisBase_ = nullptr;
FileManager::FileManager() {}
FileManager::~FileManager() {
	EndLoadThread();
}
bool FileManager::Initialize() {
	if (thisBase_) return false;
	thisBase_ = this;
	threadLoad_ = new LoadThread();
	threadLoad_->Start();
	return true;
}
void FileManager::EndLoadThread() {
	{
		Lock lock(lock_);
		if (threadLoad_ == nullptr) return;
		threadLoad_->Stop();
		threadLoad_->Join();
		threadLoad_ = nullptr;
	}
}

#if defined(DNH_PROJ_EXECUTOR)
bool FileManager::AddArchiveFile(const std::wstring& path) {
	if (mapArchiveFile_.find(path) != mapArchiveFile_.end())
		return true;

	ref_count_ptr<ArchiveFile> file = new ArchiveFile(path);
	if (!file->Open()) return false;

	std::set<std::wstring> listKeyCurrent;
	for (auto itrFile = mapArchiveFile_.begin(); itrFile != mapArchiveFile_.end(); ++itrFile) {
		ref_count_ptr<ArchiveFile> tFile = itrFile->second;
		std::set<std::wstring> tList = tFile->GetKeyList();
		for (auto itrList = tList.begin(); itrList != tList.end(); ++itrList) {
			listKeyCurrent.insert(*itrList);
		}
	}

	std::set<std::wstring> listKeyIn = file->GetKeyList();
	for (auto itrKey = listKeyIn.begin(); itrKey != listKeyIn.end(); ++itrKey) {
		const std::wstring& key = *itrKey;
		if (listKeyCurrent.find(key) == listKeyCurrent.end()) continue;

		std::string log = StringUtility::Format("archive file entry already exists [%s]", 
			StringUtility::ConvertWideToMulti(key).c_str());
		Logger::WriteTop(log);
		throw wexception(log.c_str());
	}

	mapArchiveFile_[path] = file;
	return true;
}
bool FileManager::RemoveArchiveFile(const std::wstring& path) {
	mapArchiveFile_.erase(path);
	return true;
}
ref_count_ptr<ArchiveFile> FileManager::GetArchiveFile(const std::wstring& name) {
	ref_count_ptr<ArchiveFile> res = nullptr;
	auto itrFind = mapArchiveFile_.find(name);
	if (itrFind != mapArchiveFile_.end())
		res = itrFind->second;
	return res;
}
bool FileManager::ClearArchiveFileCache() {
	mapArchiveFile_.clear();
	return true;
}
#endif

ref_count_ptr<FileReader> FileManager::GetFileReader(const std::wstring& path) {
	std::wstring pathAsUnique = PathProperty::GetUnique(path);

	ref_count_ptr<FileReader> res = nullptr;
	ref_count_ptr<File> fileRaw = new File(pathAsUnique);
	if (fileRaw->IsExists()) {
		res = new ManagedFileReader(fileRaw, nullptr);
	}
#if defined(DNH_PROJ_EXECUTOR)
	else {
		//Cannot find a physical file, search in the archive entries.

		std::vector<ArchiveFileEntry::ptr> listEntry;

		std::unordered_map<int, std::wstring> mapArchivePath;

		std::wstring key = PathProperty::GetFileName(pathAsUnique);
		for (auto itr = mapArchiveFile_.begin(); itr != mapArchiveFile_.end(); ++itr) {
			const std::wstring& pathArchive = itr->first;
			ref_count_ptr<ArchiveFile> fileArchive = itr->second;
			if (!fileArchive->IsExists(key)) continue;

			std::vector<ArchiveFileEntry::ptr> list = fileArchive->GetEntryList(key);
			listEntry.insert(listEntry.end(), list.begin(), list.end());
			for (auto itrEntry = list.begin(); itrEntry != list.end(); ++itrEntry) {
				int addr = (int)(itrEntry->get());
				mapArchivePath[addr] = pathArchive;
			}
		}

		if (listEntry.size() == 1) {	//No duplicate paths
			ArchiveFileEntry::ptr entry = listEntry[0];

			int addr = (int)entry.get();
			const std::wstring& pathArchive = mapArchivePath[addr];
			ref_count_ptr<File> file = new File(pathArchive);
			res = new ManagedFileReader(file, entry);
		}
		else {
			const std::wstring& module = PathProperty::GetModuleDirectory();

			std::wstring target = StringUtility::ReplaceAll(pathAsUnique, module, L"");
			for (auto itrEntry = listEntry.begin(); itrEntry != listEntry.end(); ++itrEntry) {
				ArchiveFileEntry::ptr entry = *itrEntry;

				const std::wstring& dir = entry->directory;
				if (target.find(dir) == std::wstring::npos) continue;

				int addr = (int)entry.get();
				std::wstring pathArchive = mapArchivePath[addr];
				ref_count_ptr<File> file = new File(pathArchive);
				res = new ManagedFileReader(file, entry);
				break;
			}
		}
	}
#endif

	if (res) res->_SetOriginalPath(path);
	return res;
}

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
ref_count_ptr<ByteBuffer> FileManager::_GetByteBuffer(shared_ptr<ArchiveFileEntry> entry) {
	ref_count_ptr<ByteBuffer> res = nullptr;
	try {
		Lock lock(lock_);

		std::wstring key = entry->directory + entry->name;
		auto itr = mapByteBuffer_.find(key);

		if (itr != mapByteBuffer_.end()) {
			res = itr->second;
		}
		else {
			res = ArchiveFile::CreateEntryBuffer(entry);
			if (res) mapByteBuffer_[key] = res;
		}
	}
	catch (...) {}

	return res;
}
void FileManager::_ReleaseByteBuffer(shared_ptr<ArchiveFileEntry> entry) {
	{
		Lock lock(lock_);

		std::wstring key = entry->directory + entry->name;
		auto itr = mapByteBuffer_.find(key);

		if (itr == mapByteBuffer_.end()) return;
		ref_count_ptr<ByteBuffer> buffer = itr->second;
		if (buffer.GetReferenceCount() == 2) {
			mapByteBuffer_.erase(itr);
		}
	}
}
#endif

void FileManager::AddLoadThreadEvent(shared_ptr<FileManager::LoadThreadEvent> event) {
	{
		Lock lock(lock_);
		if (threadLoad_)
			threadLoad_->AddEvent(event);
	}
}
void FileManager::AddLoadThreadListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lock_);
		if (threadLoad_)
			threadLoad_->AddListener(listener);
	}
}
void FileManager::RemoveLoadThreadListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lock_);
		if (threadLoad_)
			threadLoad_->RemoveListener(listener);
	}
}
void FileManager::WaitForThreadLoadComplete() {
	while (!threadLoad_->IsThreadLoadComplete())
		Sleep(1);
}

//FileManager::LoadThread
FileManager::LoadThread::LoadThread() {}
FileManager::LoadThread::~LoadThread() {}
void FileManager::LoadThread::_Run() {
	while (this->GetStatus() == RUN) {
		signal_.Wait(10);

		while (this->GetStatus() == RUN) {
			//Logger::WriteTop(StringUtility::Format("ロードイベント取り出し開始"));
			shared_ptr<FileManager::LoadThreadEvent> event = nullptr;
			{
				Lock lock(lockEvent_);
				if (listEvent_.size() == 0) break;
				event = listEvent_.front();
				//listPath_.erase(event->GetPath());
				listEvent_.pop_front();
			}
			//Logger::WriteTop(StringUtility::Format("ロードイベント取り出し完了：%s", event->GetPath().c_str()));

			//Logger::WriteTop(StringUtility::Format("ロード開始：%s", event->GetPath().c_str()));
			{
				Lock lock(lockListener_);

				for (auto itr = listListener_.begin(); itr != listListener_.end(); itr++) {
					FileManager::LoadThreadListener* listener = (*itr);
					if (event->GetListener() == listener)
						listener->CallFromLoadThread(event);
				}
			}
			//Logger::WriteTop(StringUtility::Format("ロード完了：%s", event->GetPath().c_str()));

		}
		Sleep(1);//TODO なぜか待機入れると落ちづらい？
	}

	{
		Lock lock(lockListener_);
		listListener_.clear();
	}
}
void FileManager::LoadThread::Stop() {
	Thread::Stop();
	signal_.SetSignal();
}
bool FileManager::LoadThread::IsThreadLoadComplete() {
	bool res = false;
	{
		Lock lock(lockEvent_);
		res = listEvent_.size() == 0;
	}
	return res;
}
bool FileManager::LoadThread::IsThreadLoadExists(std::wstring path) {
	bool res = false;
	{
		Lock lock(lockEvent_);
		//res = listPath_.find(path) != listPath_.end();
	}
	return res;
}
void FileManager::LoadThread::AddEvent(shared_ptr<FileManager::LoadThreadEvent> event) {
	{
		Lock lock(lockEvent_);
		std::wstring path = event->GetPath();
		if (IsThreadLoadExists(path)) return;
		listEvent_.push_back(event);
		//listPath_.insert(path);
		signal_.SetSignal();
		signal_.SetSignal(false);
	}
}
void FileManager::LoadThread::AddListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lockListener_);

		for (auto itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
			if (*itr == listener) return;
		}
		listListener_.push_back(listener);
	}
}
void FileManager::LoadThread::RemoveListener(FileManager::LoadThreadListener* listener) {
	{
		Lock lock(lockListener_);

		for (auto itr = listListener_.begin(); itr != listListener_.end(); ++itr) {
			if (*itr != listener) continue;
			listListener_.erase(itr);
			break;
		}
	}
}

/**********************************************************
//ManagedFileReader
**********************************************************/
ManagedFileReader::ManagedFileReader(ref_count_ptr<File> file, std::shared_ptr<ArchiveFileEntry> entry) {
	offset_ = 0;
	file_ = file;

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	entry_ = entry;

	if (entry_ == nullptr) {
		type_ = TYPE_NORMAL;
	}
	else if (entry_->compressionType == ArchiveFileEntry::CT_NONE && entry_ != nullptr) {
		type_ = TYPE_ARCHIVED;
	}
	else if (entry_->compressionType != ArchiveFileEntry::CT_NONE && entry_ != nullptr) {
		type_ = TYPE_ARCHIVED_COMPRESSED;
	}
#else
	type_ = TYPE_NORMAL;
#endif
}
ManagedFileReader::~ManagedFileReader() {
	Close();
}
bool ManagedFileReader::Open() {
	bool res = false;
	offset_ = 0;
	if (type_ == TYPE_NORMAL) {
		res = file_->Open();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		buffer_ = FileManager::GetBase()->_GetByteBuffer(entry_);
		res = buffer_ != nullptr;
	}
#endif
	return res;
}
void ManagedFileReader::Close() {
	if (file_) file_->Close();
	if (buffer_) {
		buffer_ = nullptr;
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
		FileManager::GetBase()->_ReleaseByteBuffer(entry_);
#endif
	}
}
size_t ManagedFileReader::GetFileSize() {
	size_t res = 0;
	if (type_ == TYPE_NORMAL) res = file_->GetSize();
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	else if ((type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) && buffer_ != nullptr)
		res = entry_->sizeFull;
#endif
	return res;
}
DWORD ManagedFileReader::Read(LPVOID buf, DWORD size) {
	DWORD res = 0;
	if (type_ == TYPE_NORMAL) {
		res = file_->Read(buf, size);
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		size_t read = size;
		if (buffer_->GetSize() < offset_ + size) {
			read = buffer_->GetSize() - offset_;
		}
		memcpy(buf, &buffer_->GetPointer()[offset_], read);
		res = read;
	}
#endif
	offset_ += res;
	return res;
}
BOOL ManagedFileReader::SetFilePointerBegin() {
	BOOL res = FALSE;
	offset_ = 0;
	if (type_ == TYPE_NORMAL) {
		file_->SetFilePointerBegin();
		res = !file_->Fail();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_) {
			offset_ = 0;
			res = TRUE;
		}
	}
#endif
	return res;
}
BOOL ManagedFileReader::SetFilePointerEnd() {
	BOOL res = FALSE;
	if (type_ == TYPE_NORMAL) {
		file_->SetFilePointerEnd();
		res = !file_->Fail();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_) {
			offset_ = buffer_->GetSize();
			res = TRUE;
		}
	}
#endif
	return res;
}
BOOL ManagedFileReader::Seek(LONG offset) {
	BOOL res = FALSE;
	if (type_ == TYPE_NORMAL) {
		file_->Seek(offset);
		res = !file_->Fail();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_) {
			res = TRUE;
		}
	}
#endif
	if (res == TRUE)
		offset_ = offset;
	return res;
}
LONG ManagedFileReader::GetFilePointer() {
	LONG res = 0;
	if (type_ == TYPE_NORMAL) {
		res = file_->GetFilePointerRead();
	}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	else if (type_ == TYPE_ARCHIVED || type_ == TYPE_ARCHIVED_COMPRESSED) {
		if (buffer_) {
			res = offset_;
		}
	}
#endif
	return res;
}
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
bool ManagedFileReader::IsArchived() {
	return type_ != TYPE_NORMAL;
}
bool ManagedFileReader::IsCompressed() {
	return type_ == TYPE_ARCHIVED_COMPRESSED;
}
#endif

/**********************************************************
//RecordEntry
**********************************************************/
RecordEntry::RecordEntry() {
	type_ = TYPE_UNKNOWN;
}
RecordEntry::~RecordEntry() {

}
size_t RecordEntry::_GetEntryRecordSize() {
	size_t res = 0;
	res += sizeof(char);
	res += sizeof(int);
	res += key_.size();
	res += sizeof(int);
	res += buffer_.GetSize();
	return res;
}
void RecordEntry::_WriteEntryRecord(Writer &writer) {
	writer.WriteCharacter(type_);
	writer.WriteInteger(key_.size());
	writer.Write(&key_[0], key_.size());

	writer.WriteInteger(buffer_.GetSize());
	writer.Write(buffer_.GetPointer(), buffer_.GetSize());
}
void RecordEntry::_ReadEntryRecord(Reader &reader) {
	type_ = reader.ReadCharacter();
	key_.resize(reader.ReadInteger());
	reader.Read(&key_[0], key_.size());

	buffer_.Clear();
	buffer_.SetSize(reader.ReadInteger());
	reader.Read(buffer_.GetPointer(), buffer_.GetSize());
}

/**********************************************************
//RecordBuffer
**********************************************************/
RecordBuffer::RecordBuffer() {

}
RecordBuffer::~RecordBuffer() {
	this->Clear();
}
void RecordBuffer::Clear() {
	mapEntry_.clear();
}
ref_count_ptr<RecordEntry> RecordBuffer::GetEntry(const std::string& key) {
	ref_count_ptr<RecordEntry> res;

	auto itr = mapEntry_.find(key);
	if (itr != mapEntry_.end()) res = itr->second;

	return res;
}
bool RecordBuffer::IsExists(const std::string& key) {
	return mapEntry_.find(key) != mapEntry_.end();
}
std::vector<std::string> RecordBuffer::GetKeyList() {
	std::vector<std::string> res;
	for (auto itr = mapEntry_.begin(); itr != mapEntry_.end(); ++itr)
		res.push_back(itr->first);
	return res;
}

void RecordBuffer::Write(Writer& writer) {
	size_t countEntry = mapEntry_.size();
	writer.Write<size_t>(countEntry);

	for (auto itr = mapEntry_.begin(); itr != mapEntry_.end(); ++itr) {
		ref_count_ptr<RecordEntry> entry = itr->second;
		entry->_WriteEntryRecord(writer);
	}
}
void RecordBuffer::Read(Reader& reader) {
	this->Clear();
	size_t countEntry = 0;
	reader.Read<size_t>(countEntry);
	for (size_t iEntry = 0; iEntry < countEntry; ++iEntry) {
		ref_count_ptr<RecordEntry> entry = new RecordEntry();
		entry->_ReadEntryRecord(reader);

		std::string& key = entry->GetKey();
		mapEntry_[key] = entry;
	}
}
bool RecordBuffer::WriteToFile(const std::wstring& path, std::string header) {
	File file(path);
	if (!file.Open(File::AccessType::WRITEONLY)) return false;

	file.Write((char*)&header[0], header.size());
	Write(file);
	file.Close();

	return true;
}
bool RecordBuffer::ReadFromFile(const std::wstring& path, std::string header) {
	File file(path);
	if (!file.Open()) return false;

	std::string fHead;
	fHead.resize(header.size());
	file.Read(&fHead[0], fHead.size());
	if (fHead != header) return false;

	Read(file);
	file.Close();

	return true;
}
int RecordBuffer::GetEntryType(const std::string& key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return RecordEntry::TYPE_NOENTRY;
	return itr->second->GetType();
}
size_t RecordBuffer::GetEntrySize(const std::string& key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return 0;
	ByteBuffer& buffer = itr->second->GetBufferRef();
	return buffer.GetSize();
}
bool RecordBuffer::GetRecord(const std::string& key, LPVOID buf, DWORD size) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return false;
	ByteBuffer& buffer = itr->second->GetBufferRef();
	buffer.Seek(0);
	buffer.Read(buf, size);
	return true;
}
bool RecordBuffer::GetRecordAsBoolean(const std::string& key, bool def) {
	bool res = def;
	GetRecord(key, res);
	return res;
}
int RecordBuffer::GetRecordAsInteger(const std::string& key, int def) {
	int res = def;
	GetRecord(key, res);
	return res;
}
float RecordBuffer::GetRecordAsFloat(const std::string& key, float def) {
	float res = def;
	GetRecord(key, res);
	return res;
}
double RecordBuffer::GetRecordAsDouble(const std::string& key, double def) {
	double res = def;
	GetRecord(key, res);
	return res;
}
std::string RecordBuffer::GetRecordAsStringA(const std::string& key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return "";

	std::string res;
	ref_count_ptr<RecordEntry> entry = itr->second;
	int type = entry->GetType();
	ByteBuffer& buffer = entry->GetBufferRef();
	buffer.Seek(0);
	if (type == RecordEntry::TYPE_STRING_A) {
		res.resize(buffer.GetSize());
		buffer.Read(&res[0], buffer.GetSize());
	}
	else if (type == RecordEntry::TYPE_STRING_W) {
		std::wstring wstr;
		wstr.resize(buffer.GetSize() / sizeof(wchar_t));
		buffer.Read(&wstr[0], buffer.GetSize());
		res = StringUtility::ConvertWideToMulti(wstr);
	}

	return res;
}
std::wstring RecordBuffer::GetRecordAsStringW(const std::string& key) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return L"";

	std::wstring res;
	ref_count_ptr<RecordEntry> entry = itr->second;
	int type = entry->GetType();
	ByteBuffer& buffer = entry->GetBufferRef();
	buffer.Seek(0);
	if (type == RecordEntry::TYPE_STRING_A) {
		std::string str;
		str.resize(buffer.GetSize());
		buffer.Read(&str[0], buffer.GetSize());

		res = StringUtility::ConvertMultiToWide(str);
	}
	else if (type == RecordEntry::TYPE_STRING_W) {
		res.resize(buffer.GetSize() / sizeof(wchar_t));
		buffer.Read(&res[0], buffer.GetSize());
	}

	return res;
}
bool RecordBuffer::GetRecordAsRecordBuffer(const std::string& key, RecordBuffer& record) {
	auto itr = mapEntry_.find(key);
	if (itr == mapEntry_.end()) return false;
	ByteBuffer& buffer = itr->second->GetBufferRef();
	buffer.Seek(0);
	record.Read(buffer);
	return true;
}
void RecordBuffer::SetRecord(int type, const std::string& key, LPVOID buf, DWORD size) {
	ref_count_ptr<RecordEntry> entry = new RecordEntry();
	entry->SetType((char)type);
	entry->SetKey(key);
	ByteBuffer& buffer = entry->GetBufferRef();
	buffer.SetSize(size);
	buffer.Write(buf, size);
	mapEntry_[key] = entry;
}
void RecordBuffer::SetRecordAsRecordBuffer(const std::string& key, RecordBuffer& record) {
	ref_count_ptr<RecordEntry> entry = new RecordEntry();
	entry->SetType((char)RecordEntry::TYPE_RECORD);
	entry->SetKey(key);
	ByteBuffer& buffer = entry->GetBufferRef();
	record.Write(buffer);
	mapEntry_[key] = entry;
}

void RecordBuffer::Read(RecordBuffer& record) {}
void RecordBuffer::Write(RecordBuffer& record) {}

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)
/**********************************************************
//PropertyFile
**********************************************************/
PropertyFile::PropertyFile() {}
PropertyFile::~PropertyFile() {}
bool PropertyFile::Load(const std::wstring& path) {
	mapEntry_.clear();

	std::vector<char> text;
	FileManager* fileManager = FileManager::GetBase();
	if (fileManager) {
		ref_count_ptr<FileReader> reader = fileManager->GetFileReader(path);

		if (reader == nullptr || !reader->Open()) {
#if defined(DNH_PROJ_EXECUTOR)
			Logger::WriteTop(ErrorUtility::GetFileNotFoundErrorMessage(path));
#endif
			return false;
		}

		size_t size = reader->GetFileSize();
		text.resize(size + 1);
		reader->Read(&text[0], size);
		text[size] = '\0';
	}
	else {
		File file(path);
		if (!file.Open()) return false;

		size_t size = file.GetSize();
		text.resize(size + 1);
		file.Read(&text[0], size);
		text[size] = '\0';
	}

	bool res = false;
	gstd::Scanner scanner(text);
	try {
		while (scanner.HasNext()) {
			gstd::Token& tok = scanner.Next();
			if (tok.GetType() != Token::Type::TK_ID) continue;
			std::wstring key = tok.GetElement();
			while (true) {
				tok = scanner.Next();
				if (tok.GetType() == Token::Type::TK_EQUAL) break;
				key += tok.GetElement();
			}

			std::wstring value;
			try {
				int posS = scanner.GetCurrentPointer();
				int posE = posS;
				while (true) {
					bool bEndLine = false;
					if (!scanner.HasNext()) {
						bEndLine = true;
					}
					else {
						tok = scanner.Next();
						bEndLine = tok.GetType() == Token::Type::TK_NEWLINE;
					}

					if (bEndLine) {
						posE = scanner.GetCurrentPointer();
						value = scanner.GetString(posS, posE);
						value = StringUtility::Trim(value);
						break;
					}
				}
			}
			catch (...) {}

			mapEntry_[key] = value;
		}

		res = true;
	}
	catch (gstd::wexception& e) {
		mapEntry_.clear();
#if defined(DNH_PROJ_EXECUTOR)
		Logger::WriteTop(
			ErrorUtility::GetParseErrorMessage(path, scanner.GetCurrentLine(), e.what()));
#endif
		res = false;
	}

	return res;
}
bool PropertyFile::HasProperty(const std::wstring& key) {
	return mapEntry_.find(key) != mapEntry_.end();
}
std::wstring PropertyFile::GetString(const std::wstring& key, const std::wstring& def) {
	if (!HasProperty(key)) return def;
	return mapEntry_[key];
}
int PropertyFile::GetInteger(const std::wstring& key, int def) {
	if (!HasProperty(key)) return def;
	const std::wstring& strValue = mapEntry_[key];
	return StringUtility::ToInteger(strValue);
}
double PropertyFile::GetReal(const std::wstring& key, double def) {
	if (!HasProperty(key)) return def;
	const std::wstring& strValue = mapEntry_[key];
	return StringUtility::ToDouble(strValue);
}
#endif

#if defined(DNH_PROJ_EXECUTOR)
/**********************************************************
//SystemValueManager
**********************************************************/
const std::string SystemValueManager::RECORD_SYSTEM = "__RECORD_SYSTEM__";
const std::string SystemValueManager::RECORD_SYSTEM_GLOBAL = "__RECORD_SYSTEM_GLOBAL__";
SystemValueManager* SystemValueManager::thisBase_ = nullptr;
SystemValueManager::SystemValueManager() {}
SystemValueManager::~SystemValueManager() {}
bool SystemValueManager::Initialize() {
	if (thisBase_) return false;

	mapRecord_[RECORD_SYSTEM] = new RecordBuffer();
	mapRecord_[RECORD_SYSTEM_GLOBAL] = new RecordBuffer();

	thisBase_ = this;
	return true;
}
void SystemValueManager::ClearRecordBuffer(std::string key) {
	if (!IsExists(key)) return;
	mapRecord_[key]->Clear();
}
bool SystemValueManager::IsExists(std::string key) {
	return mapRecord_.find(key) != mapRecord_.end();
}
bool SystemValueManager::IsExists(std::string keyRecord, std::string keyValue) {
	if (!IsExists(keyRecord)) return false;
	gstd::ref_count_ptr<RecordBuffer> record = GetRecordBuffer(keyRecord);
	return record->IsExists(keyValue);
}
gstd::ref_count_ptr<RecordBuffer> SystemValueManager::GetRecordBuffer(std::string key) {
	return IsExists(key) ? mapRecord_[key] : nullptr;
}
#endif