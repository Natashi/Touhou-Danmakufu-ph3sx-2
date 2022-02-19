#pragma once

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
	class DirectSoundManager;
	class SoundDivision;

	class SoundInfoPanel;

	class SoundSourceData;

	class SoundPlayer;
	class SoundStreamingPlayer;

	class SoundPlayerWave;
	class SoundStreamingPlayerWave;
	class SoundStreamingPlayerMp3;
	class SoundStreamingPlayerOgg;

	class AcmBase;
	class AcmMp3;
	class AcmMp3Wave;

	//*******************************************************************
	//DirectSoundManager
	//*******************************************************************
	class DirectSoundManager {
	public:
		class SoundManageThread;
		friend SoundManageThread;
		friend SoundInfoPanel;
	public:
		enum {
			SD_VOLUME_MIN = DSBVOLUME_MIN,
			SD_VOLUME_MAX = DSBVOLUME_MAX,
		};
	private:
		static DirectSoundManager* thisBase_;
	protected:
		DSCAPS dxSoundCaps_;

		IDirectSound8* pDirectSound_;
		IDirectSoundBuffer8* pDirectSoundPrimaryBuffer_;

		gstd::CriticalSection lock_;
		std::unique_ptr<SoundManageThread> threadManage_;

		std::list<shared_ptr<SoundPlayer>> listManagedPlayer_;
		std::map<std::wstring, shared_ptr<SoundSourceData>> mapSoundSource_;
		std::map<int, SoundDivision*> mapDivision_;

		shared_ptr<SoundInfoPanel> panelInfo_;

		shared_ptr<SoundSourceData> _GetSoundSource(const std::wstring& path);
		shared_ptr<SoundSourceData> _CreateSoundSource(std::wstring path);
	public:
		DirectSoundManager();
		virtual ~DirectSoundManager();

		static DirectSoundManager* GetBase() { return thisBase_; }

		virtual bool Initialize(HWND hWnd);
		void Clear();

		const DSCAPS* GetDeviceCaps() const { return &dxSoundCaps_; }

		IDirectSound8* GetDirectSound() { return pDirectSound_; }
		gstd::CriticalSection& GetLock() { return lock_; }

		shared_ptr<SoundSourceData> GetSoundSource(const std::wstring& path, bool bCreate = false);
		shared_ptr<SoundPlayer> CreatePlayer(shared_ptr<SoundSourceData> source);
		shared_ptr<SoundPlayer> GetPlayer(const std::wstring& path);

		SoundDivision* CreateSoundDivision(int index);
		SoundDivision* GetSoundDivision(int index);

		void SetInfoPanel(shared_ptr<SoundInfoPanel> panel) {
			gstd::Lock lock(lock_); 
			panelInfo_ = panel; 
		}

		void SetFadeDeleteAll();
	};

	class DirectSoundManager::SoundManageThread : public gstd::Thread, public gstd::InnerClass<DirectSoundManager> {
		friend DirectSoundManager;
	protected:
		int timeCurrent_;
		int timePrevious_;
	protected:
		SoundManageThread(DirectSoundManager* manager);

		void _Run();
		void _Arrange();
		void _Fade();
	};

	//*******************************************************************
	//SoundInfoPanel
	//*******************************************************************
	class SoundInfoPanel : public gstd::WindowLogger::Panel {
	protected:
		enum {
			ROW_ADDRESS,
			ROW_FILENAME,
			ROW_FULLPATH,
			ROW_COUNT_REFFRENCE,
		};
	protected:
		gstd::WListView wndListView_;
		int timeLastUpdate_;
		int timeUpdateInterval_;

		virtual bool _AddedLogger(HWND hTab);
	public:
		SoundInfoPanel();

		void SetUpdateInterval(int time) { timeUpdateInterval_ = time; }
		virtual void LocateParts();

		virtual void Update(DirectSoundManager* soundManager);
	};

	//*******************************************************************
	//SoundDivision
	//*******************************************************************
	class SoundDivision {
	public:
		enum {
			DIVISION_BGM = 0,
			DIVISION_SE,
			DIVISION_VOICE,
		};
	protected:
		double rateVolume_;		//0~100
	public:
		SoundDivision();
		virtual ~SoundDivision();

		void SetVolumeRate(double rate) { rateVolume_ = rate; }
		double GetVolumeRate() { return rateVolume_; }
	};

	//*******************************************************************
	//SoundSourceData
	//*******************************************************************
	class SoundSourceData {
	public:
		std::wstring path_;
		size_t pathHash_;

		SoundFileFormat format_;
		WAVEFORMATEX formatWave_;

		shared_ptr<gstd::FileReader> reader_;

		QWORD audioSizeTotal_;		//In bytes
	public:
		SoundSourceData();
		virtual ~SoundSourceData() {};

		virtual void Release();
		virtual bool Load(shared_ptr<gstd::FileReader> reader) = 0;
	};
	class SoundSourceDataWave : public SoundSourceData {
	public:
		QWORD posWaveStart_;		//In bytes
		QWORD posWaveEnd_;			//In bytes
		gstd::ByteBuffer bufWaveData_;
	public:
		SoundSourceDataWave();

		virtual void Release();
		virtual bool Load(shared_ptr<gstd::FileReader> reader);
	};
	class SoundSourceDataOgg : public SoundSourceData {
	public:
		static ov_callbacks oggCallBacks_;
		OggVorbis_File* fileOgg_;
	public:
		static size_t _ReadOgg(void* ptr, size_t size, size_t nmemb, void* source);
		static int _SeekOgg(void* source, ogg_int64_t offset, int whence);
		static int _CloseOgg(void* source);
		static long _TellOgg(void* source);
	public:
		SoundSourceDataOgg();
		~SoundSourceDataOgg();

		virtual void Release();
		virtual bool Load(shared_ptr<gstd::FileReader> reader);
	};
	class SoundSourceDataMp3 : public SoundSourceData {
	public:
		MPEGLAYER3WAVEFORMAT formatMp3_;

		HACMSTREAM hAcmStream_;
		ACMSTREAMHEADER acmStreamHeader_;

		QWORD posMp3DataStart_;
		QWORD posMp3DataEnd_;
	public:
		SoundSourceDataMp3();
		~SoundSourceDataMp3();

		virtual void Release();
		virtual bool Load(shared_ptr<gstd::FileReader> reader);
	};

	//*******************************************************************
	//SoundPlayer
	//*******************************************************************
	class SoundPlayer {
		friend DirectSoundManager;
		friend DirectSoundManager::SoundManageThread;
	public:
		struct PlayStyle {
			bool bLoop_;				//Loop enable
			double timeLoopStart_;		//Loop start (in seconds)
			double timeLoopEnd_;		//Loop end (in seconds)
			double timeStart_;			//Starting time (in seconds)
			bool bResume_;				//Resume playing after Stop and Play?
		};
	public:
		enum {
			FADE_DEFAULT = 20,

			INFO_FORMAT = 0,
			INFO_CHANNEL,
			INFO_SAMPLE_RATE,
			INFO_AVG_BYTE_PER_SEC,
			INFO_BLOCK_ALIGN,
			INFO_BIT_PER_SAMPLE,

			INFO_POSITION,
			INFO_POSITION_SAMPLE,
			INFO_LENGTH,
			INFO_LENGTH_SAMPLE,
		};
	protected:
		DirectSoundManager* manager_;
		gstd::CriticalSection lock_;

		SoundDivision* division_;

		std::wstring path_;
		size_t pathHash_;

		shared_ptr<SoundSourceData> soundSource_;
		IDirectSoundBuffer8* pDirectSoundBuffer_;

		PlayStyle playStyle_;

		bool bPause_;

		bool bDelete_;
		bool bFadeDelete_;
		bool bAutoDelete_;			//Allows deletion when the sound ends
		double rateVolume_;			//0~100
		double rateVolumeFadePerSec_;
		
		bool flgUpdateStreamOffset_;

		virtual bool _CreateBuffer(shared_ptr<SoundSourceData> source) = 0;
		static LONG _GetVolumeAsDirectSoundDecibel(float rate);

		void _LoadSamples(byte* pWaveData, size_t pSize, double* pRes);
		void _DoFFT(const std::vector<double>& bufIn, std::vector<double>& bufOut, double multiplier, bool bAutoLog);
	public:
		SoundPlayer();
		virtual ~SoundPlayer();

		gstd::CriticalSection& GetLock() { return lock_; }

		virtual void Restore() { pDirectSoundBuffer_->Restore(); }

		void SetSoundDivision(SoundDivision* div);
		void SetSoundDivision(int index);

		std::wstring& GetPath() { return path_; }
		size_t GetPathHash() { return pathHash_; }

		shared_ptr<SoundSourceData> GetSoundSource() { return soundSource_; }

		virtual bool Play();
		virtual bool Stop();
		virtual bool IsPlaying();
		void Delete() { bDelete_ = true; }

		virtual bool Seek(double time) = 0;
		virtual bool Seek(DWORD sample) = 0;
		virtual void ResetStreamForSeek() {}

		virtual bool SetVolumeRate(double rateVolume);
		double GetVolumeRate();

		bool SetPanRate(double ratePan);

		void SetFade(double rateVolumeFadePerSec);
		void SetFadeDelete(double rateVolumeFadePerSec);
		double GetFadeVolumeRate();

		void SetAutoDelete(bool bAuto = true) { bAutoDelete_ = bAuto; }

		PlayStyle* GetPlayStyle() { return &playStyle_; }

		virtual DWORD GetCurrentPosition();

		void SetFrequency(DWORD freq);

		virtual bool GetSamplesFFT(DWORD durationMs, size_t resolution, bool bAutoLog, std::vector<double>& res);
	};

	//*******************************************************************
	//SoundStreamPlayer
	//*******************************************************************
	class SoundStreamingPlayer : public SoundPlayer {
		class StreamingThread;
		friend StreamingThread;
	protected:
		HANDLE hEvent_[3];
		IDirectSoundNotify* pDirectSoundNotify_;
		unique_ptr<StreamingThread> thread_;

		bool bStreaming_;
		bool bStreamOver_;
		int streamOverIndex_;

		DWORD sizeCopy_;

		DWORD lastStreamCopyPos_[2];
		DWORD bufferPositionAtCopy_[2];

		DWORD lastReadPointer_;
	protected:
		void _CreateSoundEvent(WAVEFORMATEX& formatWave);

		virtual void _CopyStream(int indexCopy);
		virtual DWORD _CopyBuffer(LPVOID pMem, DWORD dwSize) = 0;

		void _SetStreamOver() { bStreamOver_ = true; }
	public:
		SoundStreamingPlayer();
		virtual ~SoundStreamingPlayer();

		virtual void ResetStreamForSeek();

		virtual bool Play();
		virtual bool Stop();
		virtual bool IsPlaying();

		virtual DWORD GetCurrentPosition();
		DWORD* DbgGetStreamCopyPos() { return lastStreamCopyPos_; }

		virtual bool GetSamplesFFT(DWORD durationMs, size_t resolution, bool bAutoLog, std::vector<double>& res);
	};
	class SoundStreamingPlayer::StreamingThread : public gstd::Thread, public gstd::InnerClass<SoundStreamingPlayer> {
	public:
		StreamingThread(SoundStreamingPlayer* player);

		virtual void _Run();

		void Notify(size_t index);
	};

	//*******************************************************************
	//SoundPlayerWave
	//*******************************************************************
	class SoundPlayerWave : public SoundPlayer {
	protected:
		virtual bool _CreateBuffer(shared_ptr<SoundSourceData> source);
	public:
		SoundPlayerWave();
		virtual ~SoundPlayerWave();

		virtual bool Play();
		virtual bool Stop();
		virtual bool IsPlaying();
		virtual bool Seek(double time);
		virtual bool Seek(DWORD sample);
	};

	//*******************************************************************
	//SoundStreamingPlayerWave
	//*******************************************************************
	class SoundStreamingPlayerWave : public SoundStreamingPlayer {
	protected:
		virtual bool _CreateBuffer(shared_ptr<SoundSourceData> source);
		virtual DWORD _CopyBuffer(LPVOID pMem, DWORD dwSize);
	public:
		SoundStreamingPlayerWave();

		virtual bool Seek(double time);
		virtual bool Seek(DWORD sample);
	};

	//*******************************************************************
	//SoundStreamingPlayerOgg
	//*******************************************************************
	class SoundStreamingPlayerOgg : public SoundStreamingPlayer {
	protected:
		virtual bool _CreateBuffer(shared_ptr<SoundSourceData> source);
		virtual DWORD _CopyBuffer(LPVOID pMem, DWORD dwSize);
	public:
		SoundStreamingPlayerOgg();
		~SoundStreamingPlayerOgg();

		virtual bool Seek(double time);
		virtual bool Seek(DWORD sample);
	};

	//*******************************************************************
	//SoundStreamingPlayerMp3
	//*******************************************************************
	class SoundStreamingPlayerMp3 : public SoundStreamingPlayer {
	protected:
		double timeCurrent_;
		gstd::ByteBuffer bufDecode_;	//Temp buffer containing decoded ACM stream data
	protected:
		virtual bool _CreateBuffer(shared_ptr<SoundSourceData> source);
		virtual DWORD _CopyBuffer(LPVOID pMem, DWORD dwSize);
		DWORD _DecodeAcmStream(char* pBuffer, DWORD size);
	public:
		SoundStreamingPlayerMp3();
		~SoundStreamingPlayerMp3();

		virtual bool Seek(double time);
		virtual bool Seek(DWORD sample);
	};
}