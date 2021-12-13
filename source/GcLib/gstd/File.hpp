#pragma once

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Thread.hpp"

namespace gstd {
	const std::string HEADER_RECORDFILE = "RecordBufferFile";

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
		size_t reserve_;
		size_t size_;
		size_t offset_;
		char* data_;

		size_t _GetReservedSize() { return reserve_; }
	public:
		ByteBuffer();
		ByteBuffer(ByteBuffer& buffer);
		ByteBuffer(std::stringstream& stream);
		virtual ~ByteBuffer();

		void Copy(ByteBuffer& src);
		void Copy(std::stringstream& src);
		void Clear();

		void SetSize(size_t size);
		void Reserve(size_t newReserve);

		size_t GetSize() { return size_; }
		size_t size() { return size_; }
		size_t GetOffset() { return offset_; }

		void Seek(size_t pos);
		virtual DWORD Write(LPVOID buf, DWORD size);
		virtual DWORD Read(LPVOID buf, DWORD size);

		_NODISCARD char* GetPointer(size_t offset = 0);

		ByteBuffer& operator=(const ByteBuffer& other) noexcept;
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
		std::fstream* hFile_;
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
		static std::vector<std::wstring> GetFilePathList(const std::wstring& dir);
		static std::vector<std::wstring> GetDirectoryPathList(const std::wstring& dir);

		void Delete();
		bool IsExists();
		bool IsDirectory();

		bool IsOpen() { return hFile_ && hFile_->is_open(); }

		size_t GetSize();
		std::wstring& GetPath() { return path_; }
		const std::fstream* GetFileHandle() const { return hFile_; }
		std::fstream* GetFileHandle() { return hFile_; }

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
		std::map<std::wstring, shared_ptr<ArchiveFile>> mapArchiveFile_;
		std::map<std::wstring, shared_ptr<ByteBuffer>> mapByteBuffer_;

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
		shared_ptr<ByteBuffer> _GetByteBuffer(shared_ptr<ArchiveFileEntry> entry);
		void _ReleaseByteBuffer(shared_ptr<ArchiveFileEntry> entry);
#endif
	public:
		FileManager();
		virtual ~FileManager();

		static FileManager* GetBase() { return thisBase_; }
#if defined(DNH_PROJ_EXECUTOR)
		shared_ptr<LoadThread> GetLoadThread() { return threadLoad_;  }
#endif

		virtual bool Initialize();

#if defined(DNH_PROJ_EXECUTOR)
		void EndLoadThread();
		void AddLoadThreadEvent(shared_ptr<LoadThreadEvent> event);
		void AddLoadThreadListener(FileManager::LoadThreadListener* listener);
		void RemoveLoadThreadListener(FileManager::LoadThreadListener* listener);
		void WaitForThreadLoadComplete();

		bool AddArchiveFile(const std::wstring& path, size_t readOff);
		bool RemoveArchiveFile(const std::wstring& path);
		shared_ptr<ArchiveFile> GetArchiveFile(const std::wstring& name);
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
		shared_ptr<ArchiveFileEntry> entry_;

		shared_ptr<ByteBuffer> buffer_;
		size_t offset_;
	public:
		ManagedFileReader(shared_ptr<File> file, shared_ptr<ArchiveFileEntry> entry);
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

		virtual shared_ptr<ByteBuffer> GetBuffer() { return buffer_; }
	};
#endif

	//*******************************************************************
	//Recordable
	//*******************************************************************
	class Recordable;
	class RecordEntry;
	class RecordBuffer;
	class Recordable {
	public:
		virtual ~Recordable() {}

		virtual void Read(RecordBuffer& record) = 0;
		virtual void Write(RecordBuffer& record) = 0;
	};

	//*******************************************************************
	//RecordEntry
	//*******************************************************************
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
	class RecordBuffer : public Recordable {
	private:
		std::unordered_map<std::string, shared_ptr<RecordEntry>> mapEntry_;
	public:
		RecordBuffer();
		virtual ~RecordBuffer();

		void Clear();
		size_t GetEntryCount() { return mapEntry_.size(); }
		bool IsExists(const std::string& key);
		std::vector<std::string> GetKeyList();
		shared_ptr<RecordEntry> GetEntry(const std::string& key);

		void Write(Writer& writer);
		void Read(Reader& reader);
		bool WriteToFile(const std::wstring& path, uint64_t version, const std::string& header) {
			return WriteToFile(path, version, header.c_str(), header.size());
		}
		bool WriteToFile(const std::wstring& path, uint64_t version, const char* header, size_t headerSize);
		bool ReadFromFile(const std::wstring& path, uint64_t version, const std::string& header) {
			return ReadFromFile(path, version, header.c_str(), header.size());
		}
		bool ReadFromFile(const std::wstring& path, uint64_t version, const char* header, size_t headerSize);

		size_t GetEntrySize(const std::string& key);

		//Record get
		bool GetRecord(const std::string& key, LPVOID buf, DWORD size);
		template <typename T> bool GetRecord(const std::string& key, T& data) {
			return GetRecord(key, (LPVOID)&data, sizeof(T));
		}
		template <typename T> T GetRecordAs(const std::string& key, T def = T()) {
			T res = def;
			GetRecord(key, res);
			return res;
		}
		bool GetRecordAsBoolean(const std::string& key, bool def = false) { return GetRecordAs<bool>(key, false); }
		int GetRecordAsInteger(const std::string& key, int def = 0) { return GetRecordAs<int>(key, 0); }
		float GetRecordAsFloat(const std::string& key, float def = 0.0f) { return GetRecordAs<float>(key, 0.0f); }
		double GetRecordAsDouble(const std::string& key, double def = 0.0) { return GetRecordAs<double>(key, 0.0); }
		std::string GetRecordAsStringA(const std::string& key);
		std::wstring GetRecordAsStringW(const std::string& key);
		bool GetRecordAsRecordBuffer(const std::string& key, RecordBuffer& record);

		//Record set
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

		//Recordable
		virtual void Read(RecordBuffer& record);
		virtual void Write(RecordBuffer& record);
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
		std::map<std::string, shared_ptr<RecordBuffer>> mapRecord_;
	public:
		SystemValueManager();
		virtual ~SystemValueManager();

		static SystemValueManager* GetBase() { return thisBase_; }
		
		virtual bool Initialize();

		virtual void ClearRecordBuffer(const std::string& key);
		bool IsExists(const std::string& key);
		bool IsExists(const std::string& keyRecord, const std::string& keyValue);
		shared_ptr<RecordBuffer> GetRecordBuffer(const std::string& key);
	};
#endif
}