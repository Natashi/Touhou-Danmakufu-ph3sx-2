#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Thread.hpp"

namespace gstd {
	class ByteBuffer;
	class FileManager;
	//*******************************************************************
	//Writer
	//*******************************************************************
	class Writer {
	public:
		virtual ~Writer() {};

		virtual DWORD Write(LPVOID buf, DWORD size) = 0;

		template <typename T> DWORD Write(T& data) {
			return Write(&data, sizeof(T));
		}
		template <typename T> DWORD WriteValue(T data) {
			return Write(&data, sizeof(T));
		}

		void WriteBoolean(bool b) { Write(b); }
		void WriteCharacter(char ch) { Write(ch); }
		void WriteShort(short num) { Write(num); }
		void WriteInteger(int num) { Write(num); }
		void WriteInteger64(int64_t num) { Write(num); }
		void WriteFloat(float num) { Write(num); }
		void WriteDouble(double num) { Write(num); }

		template<typename E>
		void WriteString(std::basic_string<E, std::char_traits<E>, std::allocator<E>> str) {
			Write(str.data(), str.size() * sizeof(E));
		}
	};

	//*******************************************************************
	//Reader
	//*******************************************************************
	class Reader {
	public:
		virtual ~Reader() {};

		virtual DWORD Read(LPVOID buf, DWORD size) = 0;
		template <typename T> DWORD Read(T& data) {
			return Read(&data, sizeof(T));
		}
		bool ReadBoolean() { bool res; Read(res); return res; }
		char ReadCharacter() { char res; Read(res); return res; }
		short ReadShort() { short res; Read(res); return res; }
		int ReadInteger() { int num; Read(num); return num; }
		int64_t ReadInteger64() { int64_t num; Read(num); return num; }
		float ReadFloat() { float num; Read(num); return num; }
		double ReadDouble() { double num; Read(num); return num; }

		template<typename T> T ReadValue() {
			T tmp; Read(tmp); return tmp;
		}

		std::string ReadString(size_t size) {
			std::string res = "";
			res.resize(size);
			Read(&res[0], size);
			return res;
		}
	};

	//*******************************************************************
	//ByteBuffer
	//*******************************************************************
	class ByteBuffer : public Writer, public Reader {
	protected:
		std::vector<byte> data_;
		size_t offset_;
	public:
		ByteBuffer();
		ByteBuffer(size_t size);
		ByteBuffer(std::stringstream& src);
		virtual ~ByteBuffer() = default;

		void Clear();

		void SetSize(size_t size);
		void Reserve(size_t newReserve);

		_NODISCARD size_t GetSize() const { return data_.size(); }
		_NODISCARD size_t GetOffset() const { return offset_; }

		void Seek(size_t pos);
		virtual DWORD Write(LPVOID buf, DWORD size);
		virtual DWORD Read(LPVOID buf, DWORD size);

		_NODISCARD char* GetPointer(size_t offset = 0);
		_NODISCARD const char* GetPointer(size_t offset = 0) const;
	};

	//*******************************************************************
	//File
	//*******************************************************************
	class File : public Writer, public Reader {
		static std::wstring lastError_;
		static std::map<std::wstring, size_t> mapFileUseCount_;
	public:
		enum AccessType : DWORD {
			READ = 0x1,
			WRITE = 0x2,
			WRITEONLY = 0x4,
		};
	protected:
		std::fstream hFile_;
		std::wstring path_;
		DWORD perms_;
	public:
		File();
		File(const std::wstring& path);
		virtual ~File();

		static const std::wstring& GetLastError() { return lastError_; }

		static bool CreateFileDirectory(const std::wstring& path);
		static bool IsExists(const std::wstring& path);
		static bool IsDirectory(const std::wstring& path);

		static bool IsEqualsPath(const std::wstring& path1, const std::wstring& path2);
		static std::vector<std::wstring> GetFilePathList(const std::wstring& dir, bool bSearchArchive = false);
		static std::vector<std::wstring> GetDirectoryPathList(const std::wstring& dir, bool bSearchArchive = false);

		void Delete();
		bool IsExists();
		bool IsDirectory();

		bool IsOpen() { return hFile_.is_open(); }

		size_t GetSize();
		std::wstring& GetPath() { return path_; }

		const std::fstream& GetFileHandle() const { return hFile_; }
		std::fstream& GetFileHandle() { return hFile_; }

		virtual bool Open();
		bool Open(DWORD typeAccess);
		void Close();

		virtual DWORD Write(LPVOID buf, DWORD size);
		virtual DWORD Read(LPVOID buf, DWORD size);

		bool File::SetFilePointerBegin(AccessType type = READ) { return this->Seek(0, std::ios::beg, type); }
		bool File::SetFilePointerEnd(AccessType type = READ) { return this->Seek(0, std::ios::end, type); }
		bool Seek(size_t offset, DWORD way, AccessType type = READ);
		size_t GetFilePointer(AccessType type = READ);
	};

	//*******************************************************************
	//FileReader
	//*******************************************************************
	class FileReader : public Reader {
		friend FileManager;
	protected:
		std::wstring pathOriginal_;

		void _SetOriginalPath(const std::wstring& path) { pathOriginal_ = path; }
	public:
		virtual bool Open() = 0;
		virtual void Close() = 0;

		virtual size_t GetFileSize() = 0;

		virtual bool SetFilePointerBegin(File::AccessType type = File::READ) = 0;
		virtual bool SetFilePointerEnd(File::AccessType type = File::READ) = 0;
		virtual bool Seek(size_t offset, File::AccessType type = File::READ) = 0;
		virtual size_t GetFilePointer(File::AccessType type = File::READ) = 0;

		virtual bool IsArchived() { return false; }
		virtual bool IsCompressed() { return false; }

		std::wstring& GetOriginalPath() { return pathOriginal_; }
		std::string ReadAllString() {
			SetFilePointerBegin(File::READ);
			return ReadString(GetFileSize());
		}
	};

	class ArchiveFileEntry;
	class ArchiveFile;
#if defined(DNH_PROJ_CONFIG)
	class ArchiveFileEntry {
	};
	class ArchiveFile {
	};
#endif

	//*******************************************************************
	//FileManager
	//*******************************************************************
	class ManagedFileReader;
	class FileManager {
	public:
#if defined(DNH_PROJ_EXECUTOR)
		class LoadObject;
		class LoadThread;
		class LoadThreadListener;
		class LoadThreadEvent;
#endif
		friend ManagedFileReader;
	private:
		static FileManager* thisBase_;
	protected:
		gstd::CriticalSection lock_;
#if defined(DNH_PROJ_EXECUTOR)
		shared_ptr<LoadThread> threadLoad_;
#endif

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
		struct ArchiveEntryStore {
			ArchiveFile* archive;
			ArchiveFileEntry* entry;
			unique_ptr<ByteBuffer> dataBuffer;
		};

		std::unordered_map<std::wstring, unique_ptr<ArchiveFile>> mapArchiveFile_;
		std::map<std::wstring, ArchiveEntryStore> mapArchiveEntries_;

		ByteBuffer* _GetByteBuffer(ArchiveFileEntry* entry);
		void _ReleaseByteBuffer(ArchiveFileEntry* entry);
#endif
	public:
		FileManager();
		virtual ~FileManager();

		static FileManager* GetBase() { return thisBase_; }

		virtual bool Initialize();

#if defined(DNH_PROJ_EXECUTOR)
		void EndLoadThread();
		void AddLoadThreadEvent(shared_ptr<LoadThreadEvent> event);
		void AddLoadThreadListener(FileManager::LoadThreadListener* listener);
		void RemoveLoadThreadListener(FileManager::LoadThreadListener* listener);
		void WaitForThreadLoadComplete();

		bool AddArchiveFile(const std::wstring& archivePath, size_t readOff);
		bool RemoveArchiveFile(const std::wstring& archivePath);

		ArchiveFile* GetArchiveFile(const std::wstring& archivePath);
		ArchiveEntryStore* GetArchiveFileEntry(const std::wstring& path);

		std::vector<ArchiveFileEntry*> GetArchiveFilesInDirectory(const std::wstring& dir, bool bSubDirectory);
		std::set<std::wstring> GetArchiveSubDirectoriesInDirectory(const std::wstring& dir);

		bool IsArchiveFileExists(const std::wstring& path);
		bool IsArchiveDirectoryExists(const std::wstring& dir);

		bool ClearArchiveFileCache();
#endif

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
		shared_ptr<FileReader> GetFileReader(const std::wstring& path);
#endif
	};

#if defined(DNH_PROJ_EXECUTOR)
	class FileManager::LoadObject {
	public:
		virtual ~LoadObject() {};
	};

	class FileManager::LoadThread : public Thread {
		gstd::CriticalSection lockEvent_;
		gstd::CriticalSection lockListener_;
		gstd::ThreadSignal signal_;
	protected:
		virtual void _Run();
		//std::set<std::string> listPath_;
		std::list<shared_ptr<FileManager::LoadThreadEvent>> listEvent_;
		std::list<FileManager::LoadThreadListener*> listListener_;
	public:
		LoadThread();
		virtual ~LoadThread();

		virtual void Stop();

		bool IsThreadLoadComplete();
		bool IsThreadLoadExists(std::wstring path);
		
		void AddEvent(shared_ptr<FileManager::LoadThreadEvent> event);
		void AddListener(FileManager::LoadThreadListener* listener);
		void RemoveListener(FileManager::LoadThreadListener* listener);
	};

	class FileManager::LoadThreadListener {
	public:
		virtual ~LoadThreadListener() {}
		virtual void CallFromLoadThread(shared_ptr<FileManager::LoadThreadEvent> event) = 0;

		virtual void CancelLoad() {}
		virtual bool CancelLoadComplete() { return true; }

		inline void WaitForCancel() {
			this->CancelLoad();
			while (!this->CancelLoadComplete())
				::Sleep(1);
		}
	};

	class FileManager::LoadThreadEvent {
	protected:
		FileManager::LoadThreadListener* listener_;
		std::wstring path_;
		shared_ptr<FileManager::LoadObject> source_;
	public:
		LoadThreadEvent(FileManager::LoadThreadListener* listener, const std::wstring& path, 
			shared_ptr<FileManager::LoadObject> source) 
		{
			listener_ = listener;
			path_ = path;
			source_ = source;
		};
		virtual ~LoadThreadEvent() {}

		FileManager::LoadThreadListener* GetListener() { return listener_; }
		std::wstring& GetPath() { return path_; }
		shared_ptr<FileManager::LoadObject> GetSource() { return source_; }
	};
#endif

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
	//*******************************************************************
	//ManagedFileReader
	//*******************************************************************
	class ManagedFileReader : public FileReader {
	private:
		enum FILETYPE {
			TYPE_NORMAL,
			TYPE_ARCHIVED,
			TYPE_ARCHIVED_COMPRESSED,
		};

		FILETYPE type_;
		shared_ptr<File> file_;
		ArchiveFileEntry* entry_;

		ByteBuffer* buffer_;
		size_t offset_;
	public:
		ManagedFileReader(shared_ptr<File> file, ArchiveFileEntry* entry);
		~ManagedFileReader();

		virtual bool Open();
		virtual void Close();
		virtual size_t GetFileSize();
		virtual DWORD Read(LPVOID buf, DWORD size);

		virtual bool SetFilePointerBegin(File::AccessType type = File::READ);
		virtual bool SetFilePointerEnd(File::AccessType type = File::READ);
		virtual bool Seek(size_t offset, File::AccessType type = File::READ);
		virtual size_t GetFilePointer(File::AccessType type = File::READ);

		virtual bool IsArchived();
		virtual bool IsCompressed();

		virtual ByteBuffer* GetBuffer() { return buffer_; }
	};
#endif

	//*******************************************************************
	//RecordEntry
	//*******************************************************************
	class RecordBuffer;
	class RecordEntry {
		friend RecordBuffer;
	private:
		std::string key_;
		ByteBuffer buffer_;

		size_t _GetEntryRecordSize();
		void _WriteEntryRecord(Writer& writer);
		void _ReadEntryRecord(Reader& reader);
	public:
		RecordEntry();
		virtual ~RecordEntry();

		void SetKey(const std::string& key) { key_ = key; }
		std::string& GetKey() { return key_; }

		ByteBuffer& GetBufferRef() { return buffer_; }
	};

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_CONFIG)
	//*******************************************************************
	//RecordBuffer
	//*******************************************************************
	class RecordBuffer {
	public:
		static inline const std::string HEADER = "RecordBufferFile";
	private:
		std::unordered_map<std::string, RecordEntry> mapEntry_;
	public:
		RecordBuffer();
		virtual ~RecordBuffer();

		void Clear();

		size_t GetEntryCount() const { return mapEntry_.size(); }
		RecordEntry* GetEntry(const std::string& key);
		const RecordEntry* GetEntry(const std::string& key) const;

		bool IsExists(const std::string& key) const;
		std::vector<std::string> GetKeyList() const;

		void Write(Writer& writer);
		void Read(Reader& reader);

		bool WriteToFile(const std::wstring& path, uint32_t version, const char* header, size_t headerSize);
		bool ReadFromFile(const std::wstring& path, uint32_t version, const char* header, size_t headerSize);
		bool WriteToFile(const std::wstring& path, uint32_t version, const std::string& header) {
			return WriteToFile(path, version, header.c_str(), header.size());
		}
		bool ReadFromFile(const std::wstring& path, uint32_t version, const std::string& header) {
			return ReadFromFile(path, version, header.c_str(), header.size());
		}

		size_t GetEntrySize(const std::string& key);

		// --------------------------------------------------------------------------------
		// Record get
		
		bool GetRecord(const std::string& key, LPVOID buf, DWORD size);
		template <typename T> inline bool GetRecord(const std::string& key, T& data) {
			return GetRecord(key, (LPVOID)&data, sizeof(T));
		}
		template <typename T> inline optional<T> GetRecordAs(const std::string& key) {
			T res{};
			if (GetRecord(key, res))
				return res;
			return {};
		}
		template <typename T> inline T GetRecordOr(const std::string& key, T def) {
			GetRecord(key, def);
			return MOVE(def);
		}

		// --------------------------------------------------------------------------------

		inline optional<bool> GetRecordAsBoolean(const std::string& key) { return GetRecordAs<bool>(key); }
		inline optional<int> GetRecordAsInteger(const std::string& key) { return GetRecordAs<int>(key); }
		inline optional<float> GetRecordAsFloat(const std::string& key) { return GetRecordAs<float>(key); }
		inline optional<double> GetRecordAsDouble(const std::string& key) { return GetRecordAs<double>(key); }

		inline bool GetRecordAsBoolean(const std::string& key, bool def) { return GetRecordOr(key, def); }
		inline int GetRecordAsInteger(const std::string& key, int def) { return GetRecordOr(key, def); }
		inline float GetRecordAsFloat(const std::string& key, float def) { return GetRecordOr(key, def); }
		inline double GetRecordAsDouble(const std::string& key, double def) { return GetRecordOr(key, def); }

		optional<std::string> GetRecordAsStringA(const std::string& key);
		inline std::string GetRecordAsStringA(const std::string& key, std::string def) {
			if (auto res = GetRecordAsStringA(key))
				return *res;
			return MOVE(def);
		}

		inline optional<std::wstring> GetRecordAsStringW(const std::string& key) {
			if (auto res = GetRecordAsStringA(key))
				return STR_WIDE(*res);
			return {};
		}
		inline std::wstring GetRecordAsStringW(const std::string& key, std::wstring def) {
			if (auto res = GetRecordAsStringW(key))
				return *res;
			return MOVE(def);
		}

		optional<RecordBuffer> GetRecordAsRecordBuffer(const std::string& key);
		optional<ByteBuffer> GetRecordAsByteBuffer(const std::string& key);

		// --------------------------------------------------------------------------------
		// Record set

		void SetRecord(const std::string& key, LPVOID buf, DWORD size);
		template <typename T> void SetRecord(const std::string& key, const T& data) {
			SetRecord(key, (LPVOID)&data, sizeof(T));
		}

		void SetRecordAsBoolean(const std::string& key, bool data) { SetRecord(key, data); }
		void SetRecordAsInteger(const std::string& key, int data) { SetRecord(key, data); }
		void SetRecordAsFloat(const std::string& key, float data) { SetRecord(key, data); }
		void SetRecordAsDouble(const std::string& key, double data) { SetRecord(key, data); }
		void SetRecordAsStringA(const std::string& key, const std::string& data) { 
			SetRecord(key, (LPVOID)data.c_str(), data.size() * sizeof(char));
		}
		void SetRecordAsStringW(const std::string& key, const std::wstring& data) {
			std::string mbstr = StringUtility::ConvertWideToMulti(data);
			SetRecord(key, (LPVOID)mbstr.c_str(), mbstr.size() * sizeof(char));
		}

		void SetRecordAsRecordBuffer(const std::string& key, RecordBuffer& record);
		void SetRecordAsByteBuffer(const std::string& key, ByteBuffer&& buffer);
	};

	//*******************************************************************
	//PropertyFile
	//*******************************************************************
	class PropertyFile {
	protected:
		std::map<std::wstring, std::wstring> mapEntry_;
	public:
		PropertyFile();
		virtual ~PropertyFile();

		bool Load(const std::wstring& path);

		bool HasProperty(const std::wstring& key);
		std::wstring GetString(const std::wstring& key) { return GetString(key, L""); }
		std::wstring GetString(const std::wstring& key, const std::wstring& def);
		int GetInteger(const std::wstring& key) { return GetInteger(key, 0); }
		int GetInteger(const std::wstring& key, int def);
		double GetReal(const std::wstring& key) { return GetReal(key, 0.0); }
		double GetReal(const std::wstring& key, double def);
	};
#endif

#if defined(DNH_PROJ_EXECUTOR)
	//*******************************************************************
	//SystemValueManager
	//*******************************************************************
	class SystemValueManager {
	public:
		const static std::string RECORD_SYSTEM;
		const static std::string RECORD_SYSTEM_GLOBAL;
	private:
		static SystemValueManager* thisBase_;
	protected:
		std::map<std::string, unique_ptr<RecordBuffer>> mapRecord_;
	public:
		SystemValueManager();
		virtual ~SystemValueManager();

		static SystemValueManager* GetBase() { return thisBase_; }
		
		virtual bool Initialize();

		virtual void ClearRecordBuffer(const std::string& key);

		bool IsExists(const std::string& key) const;
		bool IsExists(const std::string& keyRecord, const std::string& keyValue) const;

		const RecordBuffer* GetRecordBuffer(const std::string& key) const;
	};
#endif
}