#ifndef __GSTD_FILE__
#define __GSTD_FILE__

#include "../pch.h"

#include "GstdUtility.hpp"
#include "Thread.hpp"

namespace gstd {
	const std::string HEADER_RECORDFILE = "RecordBufferFile";

	class ByteBuffer;
	class FileManager;
	/**********************************************************
	//Writer
	**********************************************************/
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

	/**********************************************************
	//Reader
	**********************************************************/
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

	/**********************************************************
	//FileReader
	**********************************************************/
	class FileReader : public Reader {
		friend FileManager;
	protected:
		std::wstring pathOriginal_;
		void _SetOriginalPath(std::wstring path) { pathOriginal_ = path; }

	public:
		virtual bool Open() = 0;
		virtual void Close() = 0;
		virtual size_t GetFileSize() = 0;
		virtual BOOL SetFilePointerBegin() = 0;
		virtual BOOL SetFilePointerEnd() = 0;
		virtual BOOL Seek(LONG offset) = 0;
		virtual LONG GetFilePointer() = 0;
		virtual bool IsArchived() { return false; }
		virtual bool IsCompressed() { return false; }

		std::wstring GetOriginalPath() { return pathOriginal_; }
		std::string ReadAllString() {
			SetFilePointerBegin();
			return ReadString(GetFileSize());
		}
	};

	/**********************************************************
	//ByteBuffer
	**********************************************************/
	class ByteBuffer : public Writer, public Reader {
	protected:
		size_t reserve_;
		size_t size_;
		size_t offset_;
		char* data_;

		size_t _GetReservedSize();
		void _Resize(size_t size);
	public:
		ByteBuffer();
		ByteBuffer(ByteBuffer& buffer);
		ByteBuffer(std::stringstream& stream);
		virtual ~ByteBuffer();
		void Copy(ByteBuffer& src);
		void Copy(std::stringstream& src);
		void Clear();

		void Seek(size_t pos);
		void SetSize(size_t size);
		size_t GetSize() { return size_; }
		size_t size() { return size_; }
		size_t GetOffset() { return offset_; }

		virtual DWORD Write(LPVOID buf, DWORD size);
		virtual DWORD Read(LPVOID buf, DWORD size);

		_NODISCARD char* GetPointer(size_t offset = 0);
		_NODISCARD char& operator[](size_t offset);

		ByteBuffer& operator=(const ByteBuffer& other) noexcept;
	};

	/**********************************************************
	//File
	//ファイルは、x:\fffff.xxx
	//ディレクトリはx:\ddddd\
	**********************************************************/
	class File : public Writer, public Reader {
	public:
		enum AccessType : DWORD {
			READ = 0x1,
			WRITE = 0x2,
			WRITEONLY = 0x4,
			TEXTBASED = 0x8,
		};
	protected:
		std::fstream hFile_;
		std::wstring path_;
		DWORD perms_;
		bool bOpen_;
	public:
		File();
		File(std::wstring path);
		virtual ~File();

		static bool CreateFileDirectory(std::wstring& path);
		static bool IsExists(std::wstring& path);
		static bool IsDirectory(std::wstring& path);

		void Delete();
		bool IsExists();
		bool IsDirectory();

		size_t GetSize();
		std::wstring& GetPath() { return path_; }
		std::fstream& GetFileStream() { return hFile_; }

		virtual bool Open();
		bool Open(DWORD typeAccess);
		void Close();

		virtual DWORD Write(LPVOID buf, DWORD size);
		virtual DWORD Read(LPVOID buf, DWORD size);

		void SetFilePointerBegin() { hFile_.seekg(0, std::ios::beg); }
		void SetFilePointerEnd() { hFile_.seekg(0, std::ios::end); }
		void Seek(size_t offset, const std::ios::_Seekdir& seek = std::ios::beg) {
			SeekRead(offset, seek);
			SeekWrite(offset, seek);
		}
		void SeekRead(size_t offset, const std::ios::_Seekdir& seek) {
			hFile_.seekg(offset, seek);
		}
		void SeekWrite(size_t offset, const std::ios::_Seekdir& seek) {
			if (perms_ & AccessType::WRITE) hFile_.seekp(offset, seek);
		}
		size_t GetFilePointerRead() { return hFile_.tellg(); }
		size_t GetFilePointerWrite() { return hFile_.tellp(); }

		bool Fail() { return bOpen_ ? hFile_.fail() : false; }

		static bool IsEqualsPath(std::wstring path1, std::wstring path2);
		static std::vector<std::wstring> GetFilePathList(std::wstring dir);
		static std::vector<std::wstring> GetDirectoryPathList(std::wstring dir);
	};


	class ArchiveFileEntry;
	class ArchiveFile;
#if defined(DNH_PROJ_CONFIG)
	class ArchiveFileEntry {
	};
	class ArchiveFile {
	};
#endif

	/**********************************************************
	//FileManager
	**********************************************************/
	class ManagedFileReader;
	class FileManager {
	public:
		class LoadObject;
		class LoadThread;
		class LoadThreadListener;
		class LoadThreadEvent;
		friend ManagedFileReader;
	private:
		static FileManager* thisBase_;
	protected:
		gstd::CriticalSection lock_;
		gstd::ref_count_ptr<LoadThread> threadLoad_;
		std::map<std::wstring, ref_count_ptr<ArchiveFile>> mapArchiveFile_;
		std::map<std::wstring, ref_count_ptr<ByteBuffer>> mapByteBuffer_;

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
		ref_count_ptr<ByteBuffer> _GetByteBuffer(std::shared_ptr<ArchiveFileEntry> entry);
		void _ReleaseByteBuffer(std::shared_ptr<ArchiveFileEntry> entry);
#endif
	public:
		static FileManager* GetBase() { return thisBase_; }
		FileManager();
		virtual ~FileManager();
		virtual bool Initialize();
		void EndLoadThread();

#if defined(DNH_PROJ_EXECUTOR)
		bool AddArchiveFile(std::wstring path);
		bool RemoveArchiveFile(std::wstring path);
#endif

		ref_count_ptr<FileReader> GetFileReader(std::wstring path);

		void AddLoadThreadEvent(shared_ptr<LoadThreadEvent> event);
		void AddLoadThreadListener(FileManager::LoadThreadListener* listener);
		void RemoveLoadThreadListener(FileManager::LoadThreadListener* listener);
		void WaitForThreadLoadComplete();
	};

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
	};

	class FileManager::LoadThreadEvent {
	protected:
		FileManager::LoadThreadListener* listener_;
		std::wstring path_;
		shared_ptr<FileManager::LoadObject> source_;
	public:
		LoadThreadEvent(FileManager::LoadThreadListener* listener, std::wstring path, shared_ptr<FileManager::LoadObject> source) {
			listener_ = listener;
			path_ = path;
			source_ = source;
		};
		virtual ~LoadThreadEvent() {}

		FileManager::LoadThreadListener* GetListener() { return listener_; }
		std::wstring GetPath() { return path_; }
		shared_ptr<FileManager::LoadObject> GetSource() { return source_; }
	};

	/**********************************************************
	//ManagedFileReader
	**********************************************************/
	class ManagedFileReader : public FileReader {
	private:
		enum FILETYPE {
			TYPE_NORMAL,
			TYPE_ARCHIVED,
			TYPE_ARCHIVED_COMPRESSED,
		};

		FILETYPE type_;
		ref_count_ptr<File> file_;
#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
		std::shared_ptr<ArchiveFileEntry> entry_;
#endif
		ref_count_ptr<ByteBuffer> buffer_;
		size_t offset_;
	public:
		ManagedFileReader(ref_count_ptr<File> file, std::shared_ptr<ArchiveFileEntry> entry);
		~ManagedFileReader();

		virtual bool Open();
		virtual void Close();
		virtual size_t GetFileSize();
		virtual DWORD Read(LPVOID buf, DWORD size);
		virtual BOOL SetFilePointerBegin();
		virtual BOOL SetFilePointerEnd();
		virtual BOOL Seek(LONG offset);
		virtual LONG GetFilePointer();

#if defined(DNH_PROJ_EXECUTOR) || defined(DNH_PROJ_FILEARCHIVER)
		virtual bool IsArchived();
		virtual bool IsCompressed();
#endif

		virtual ref_count_ptr<ByteBuffer> GetBuffer() { return buffer_; }
	};

	/**********************************************************
	//Recordable
	**********************************************************/
	class Recordable;
	class RecordEntry;
	class RecordBuffer;
	class Recordable {
	public:
		virtual ~Recordable() {}
		virtual void Read(RecordBuffer& record) = 0;
		virtual void Write(RecordBuffer& record) = 0;
	};

	/**********************************************************
	//RecordEntry
	**********************************************************/
	class RecordEntry {
		friend RecordBuffer;
	public:
		enum {
			TYPE_NOENTRY = -2,
			TYPE_UNKNOWN = -1,
			TYPE_BOOLEAN = 1,
			TYPE_INTEGER = 2,
			TYPE_FLOAT = 3,
			TYPE_DOUBLE = 4,
			TYPE_STRING_A = 5,	//char string
			TYPE_RECORD = 6,
			TYPE_STRING_W = 7,	//wchar_t string
		};

	private:
		char type_;
		std::string key_;
		ByteBuffer buffer_;

		size_t _GetEntryRecordSize();
		void _WriteEntryRecord(Writer &writer);
		void _ReadEntryRecord(Reader &reader);
	public:
		RecordEntry();
		virtual ~RecordEntry();
		char GetType() { return type_; }

		void SetKey(std::string key) { key_ = key; }
		void SetType(char type) { type_ = type; }
		std::string& GetKey() { return key_; }
		ByteBuffer& GetBufferRef() { return buffer_; }
	};

	/**********************************************************
	//RecordBuffer
	**********************************************************/
	class RecordBuffer : public Recordable {
	private:
		std::map<std::string, ref_count_ptr<RecordEntry>> mapEntry_;
	public:
		RecordBuffer();
		virtual ~RecordBuffer();
		void Clear();//保持データクリア
		size_t GetEntryCount() { return mapEntry_.size(); }
		bool IsExists(std::string key);
		std::vector<std::string> GetKeyList();
		ref_count_ptr<RecordEntry> GetEntry(std::string key);

		void Write(Writer& writer);
		void Read(Reader& reader);
		bool WriteToFile(std::wstring path, std::string header = HEADER_RECORDFILE);
		bool ReadFromFile(std::wstring path, std::string header = HEADER_RECORDFILE);

		//エントリ
		int GetEntryType(std::string key);
		size_t GetEntrySize(std::string key);

		//エントリ取得(文字列キー)
		bool GetRecord(std::string& key, LPVOID buf, DWORD size);
		template <typename T> bool GetRecord(std::string key, T& data) {
			return GetRecord(key, &data, sizeof(T));
		}
		bool GetRecordAsBoolean(std::string key, bool def = false);
		int GetRecordAsInteger(std::string key, int def = 0);
		float GetRecordAsFloat(std::string key, float def = 0.0f);
		double GetRecordAsDouble(std::string key, double def = 0.0);
		std::string GetRecordAsStringA(std::string key);
		std::wstring GetRecordAsStringW(std::string key);
		bool GetRecordAsRecordBuffer(std::string key, RecordBuffer& record);

		//エントリ取得(数値キー)
		bool GetRecord(int key, LPVOID buf, DWORD size) { return GetRecord(StringUtility::Format("%d", key), buf, size); }
		template <typename T> bool GetRecord(int key, T& data) { return GetRecord(StringUtility::Format("%d", key), data); }
		bool GetRecordAsBoolean(int key) { return GetRecordAsBoolean(StringUtility::Format("%d", key)); };
		int GetRecordAsInteger(int key) { return GetRecordAsInteger(StringUtility::Format("%d", key)); }
		float GetRecordAsFloat(int key) { return GetRecordAsFloat(StringUtility::Format("%d", key)); }
		double GetRecordAsDouble(int key) { return GetRecordAsDouble(StringUtility::Format("%d", key)); }
		std::string GetRecordAsStringA(int key) { return GetRecordAsStringA(StringUtility::Format("%d", key)); }
		std::wstring GetRecordAsStringW(int key) { return GetRecordAsStringW(StringUtility::Format("%d", key)); }
		bool GetRecordAsRecordBuffer(int key, RecordBuffer& record) { return GetRecordAsRecordBuffer(StringUtility::Format("%d", key), record); }


		//エントリ設定(文字列キー)
		void SetRecord(std::string& key, LPVOID buf, DWORD size) { SetRecord(RecordEntry::TYPE_UNKNOWN, key, buf, size); }
		template <typename T> void SetRecord(std::string key, T& data) {
			SetRecord(RecordEntry::TYPE_UNKNOWN, key, &data, sizeof(T));
		}
		void SetRecord(int type, std::string& key, LPVOID buf, DWORD size);
		template <typename T> void SetRecord(int type, std::string& key, T& data) {
			SetRecord(type, key, &data, sizeof(T));
		}
		void SetRecordAsBoolean(std::string key, bool data) { SetRecord(RecordEntry::TYPE_BOOLEAN, key, data); }
		void SetRecordAsInteger(std::string key, int data) { SetRecord(RecordEntry::TYPE_INTEGER, key, data); }
		void SetRecordAsFloat(std::string key, float data) { SetRecord(RecordEntry::TYPE_FLOAT, key, data); }
		void SetRecordAsDouble(std::string key, double data) { SetRecord(RecordEntry::TYPE_DOUBLE, key, data); }
		void SetRecordAsStringA(std::string key, std::string data) { SetRecord(RecordEntry::TYPE_STRING_A, key, &data[0], data.size()); }
		void SetRecordAsStringW(std::string key, std::wstring data) { SetRecord(RecordEntry::TYPE_STRING_W, key, &data[0], data.size() * sizeof(wchar_t)); }
		void SetRecordAsRecordBuffer(std::string key, RecordBuffer& record);

		//エントリ設定(数値キー)
		void SetRecord(int key, LPVOID buf, DWORD size) { SetRecord(StringUtility::Format("%d", key), buf, size); }
		template <typename T> void SetRecord(int key, T& data) { SetRecord(StringUtility::Format("%d", key), data); }
		void SetRecordAsBoolean(int key, bool data) { SetRecordAsInteger(StringUtility::Format("%d", key), data); }
		void SetRecordAsInteger(int key, int data) { SetRecordAsInteger(StringUtility::Format("%d", key), data); }
		void SetRecordAsFloat(int key, float data) { SetRecordAsFloat(StringUtility::Format("%d", key), data); }
		void SetRecordAsDouble(int key, double data) { SetRecordAsDouble(StringUtility::Format("%d", key), data); }
		void SetRecordAsStringA(int key, std::string data) { SetRecordAsStringA(StringUtility::Format("%d", key), data); }
		void SetRecordAsStringW(int key, std::wstring data) { SetRecordAsStringW(StringUtility::Format("%d", key), data); }
		void SetRecordAsRecordBuffer(int key, RecordBuffer& record) { SetRecordAsRecordBuffer(StringUtility::Format("%d", key), record); }

		//Recoedable
		virtual void Read(RecordBuffer& record);
		virtual void Write(RecordBuffer& record);
	};

#if defined(DNH_PROJ_EXECUTOR)
	/**********************************************************
	//PropertyFile
	**********************************************************/
	class PropertyFile {
	protected:
		std::map<std::wstring, std::wstring> mapEntry_;
	public:
		PropertyFile();
		virtual ~PropertyFile();

		bool Load(std::wstring path);

		bool HasProperty(std::wstring key);
		std::wstring GetString(std::wstring key) { return GetString(key, L""); }
		std::wstring GetString(std::wstring key, std::wstring def);
		int GetInteger(std::wstring key) { return GetInteger(key, 0); }
		int GetInteger(std::wstring key, int def);
		double GetReal(std::wstring key) { return GetReal(key, 0.0); }
		double GetReal(std::wstring key, double def);
	};

	/**********************************************************
	//SystemValueManager
	**********************************************************/
	class SystemValueManager {
	public:
		const static std::string RECORD_SYSTEM;
		const static std::string RECORD_SYSTEM_GLOBAL;
	private:
		static SystemValueManager* thisBase_;

	protected:
		std::map<std::string, gstd::ref_count_ptr<RecordBuffer> > mapRecord_;

	public:
		SystemValueManager();
		virtual ~SystemValueManager();
		static SystemValueManager* GetBase() { return thisBase_; }
		virtual bool Initialize();

		virtual void ClearRecordBuffer(std::string key);
		bool IsExists(std::string key);
		bool IsExists(std::string keyRecord, std::string keyValue);
		gstd::ref_count_ptr<RecordBuffer> GetRecordBuffer(std::string key);
	};
#endif
}


#endif
