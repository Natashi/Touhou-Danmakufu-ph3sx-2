#include "source/GcLib/pch.h"

#include "DirectSound.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//DirectSoundManager
//*******************************************************************
DirectSoundManager* DirectSoundManager::thisBase_ = nullptr;
DirectSoundManager::DirectSoundManager() {
	ZeroMemory(&dxSoundCaps_, sizeof(DSCAPS));

	pDirectSound_ = nullptr;
	pDirectSoundPrimaryBuffer_ = nullptr;

	CreateSoundDivision(SoundDivision::DIVISION_BGM);
	CreateSoundDivision(SoundDivision::DIVISION_SE);
	CreateSoundDivision(SoundDivision::DIVISION_VOICE);
}
DirectSoundManager::~DirectSoundManager() {
	Logger::WriteTop("DirectSound: Finalizing.");
	this->Clear();

	threadManage_->Stop();
	threadManage_->Join();
	delete threadManage_;

	for (auto itr = mapDivision_.begin(); itr != mapDivision_.end(); ++itr)
		ptr_delete(itr->second);

	ptr_release(pDirectSoundPrimaryBuffer_);
	ptr_release(pDirectSound_);

	panelInfo_ = nullptr;
	thisBase_ = nullptr;
	Logger::WriteTop("DirectSound: Finalized.");
}
bool DirectSoundManager::Initialize(HWND hWnd) {
	if (thisBase_) return false;

	Logger::WriteTop("DirectSound: Initializing.");

	auto WrapDX = [&](HRESULT hr, const std::wstring& routine) {
		if (SUCCEEDED(hr)) return;
		std::wstring err = StringUtility::Format(L"DirectSound: %s failure. [%s]\r\n  %s",
			routine.c_str(), DXGetErrorString(hr), DXGetErrorDescription(hr));
		Logger::WriteTop(err);
		throw wexception(err);
	};

	WrapDX(DirectSoundCreate8(nullptr, &pDirectSound_, nullptr), L"DirectSoundCreate8");

	WrapDX(pDirectSound_->SetCooperativeLevel(hWnd, DSSCL_PRIORITY), L"SetCooperativeLevel");

	//Get device caps
	dxSoundCaps_.dwSize = sizeof(DSCAPS);
	WrapDX(pDirectSound_->GetCaps(&dxSoundCaps_), L"GetCaps");

	//Create the primary buffer
	{
		DSBUFFERDESC desc;
		ZeroMemory(&desc, sizeof(DSBUFFERDESC));
		desc.dwSize = sizeof(DSBUFFERDESC);
		desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
		desc.dwBufferBytes = 0;
		desc.lpwfxFormat = nullptr;
		WrapDX(pDirectSound_->CreateSoundBuffer(&desc, (LPDIRECTSOUNDBUFFER*)&pDirectSoundPrimaryBuffer_, nullptr),
			L"CreateSoundBuffer(primary)");

		WAVEFORMATEX pcmwf;
		ZeroMemory(&pcmwf, sizeof(WAVEFORMATEX));
		pcmwf.wFormatTag = WAVE_FORMAT_PCM;
		pcmwf.nChannels = 2;
		pcmwf.nSamplesPerSec = 44100;
		pcmwf.nBlockAlign = 4;
		pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
		pcmwf.wBitsPerSample = 16;
		WrapDX(pDirectSoundPrimaryBuffer_->SetFormat(&pcmwf), L"SetFormat");
	}

	//Sound manager thread, this thread runs even when the window is unfocused,
	//and manages stuff like fade, deletion, and the LogWindow's Sound panel.
	threadManage_ = new SoundManageThread(this);
	threadManage_->Start();

	Logger::WriteTop("DirectSound: Initialized.");

	thisBase_ = this;
	return true;
}
void DirectSoundManager::Clear() {
	try {
		Lock lock(lock_);

		for (auto itrPlayer = listManagedPlayer_.begin(); itrPlayer != listManagedPlayer_.end(); ++itrPlayer) {
			shared_ptr<SoundPlayer> player = *itrPlayer;
			if (player == nullptr) continue;
			player->Stop();
		}

		mapSoundSource_.clear();
	}
	catch (...) {}
}
shared_ptr<SoundSourceData> DirectSoundManager::GetSoundSource(const std::wstring& path, bool bCreate) {
	shared_ptr<SoundSourceData> res;
	try {
		Lock lock(lock_);

		res = _GetSoundSource(path);
		if (res == nullptr && bCreate) {
			res = _CreateSoundSource(path);
		}
	}
	catch (...) {}

	return res;
}
shared_ptr<SoundSourceData> DirectSoundManager::_GetSoundSource(const std::wstring& path) {
	auto itr = mapSoundSource_.find(path);
	if (itr == mapSoundSource_.end()) return nullptr;
	return itr->second;
}
shared_ptr<SoundSourceData> DirectSoundManager::_CreateSoundSource(std::wstring path) {
	FileManager* fileManager = FileManager::GetBase();

	shared_ptr<SoundSourceData> res;

	try {
		path = PathProperty::GetUnique(path);
		shared_ptr<FileReader> reader = fileManager->GetFileReader(path);
		if (reader == nullptr || !reader->Open())
			throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path, true));

		size_t sizeFile = reader->GetFileSize();
		if (sizeFile <= 64)
			throw gstd::wexception(L"Audio file invalid.");

		ByteBuffer header;
		header.SetSize(0x100);
		reader->Read(header.GetPointer(), header.GetSize());

		SoundFileFormat format = SoundFileFormat::Unknown;
		if (!memcmp(header.GetPointer(), "RIFF", 4)) {		//WAVE
			format = SoundFileFormat::Wave;
			/*
			size_t ptr = 0xC;
			while (ptr <= 0x100) {
				size_t chkSize = *(uint32_t*)header.GetPointer(ptr + sizeof(uint32_t));
				if (memcmp(header.GetPointer(ptr), "fmt ", 4) == 0 && chkSize >= 0x10) {
					WAVEFORMATEX* wavefmt = (WAVEFORMATEX*)header.GetPointer(ptr + sizeof(uint32_t) * 2);
					if (wavefmt->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
						format = SoundFileFormat::AWave;	//Surprise! You thought it was .wav, BUT IT WAS ME, .MP3!!
					break;
				}
				ptr += chkSize + sizeof(uint32_t);
			}
			*/
		}
		else if (!memcmp(header.GetPointer(), "OggS", 4)) {	//Ogg Vorbis
			format = SoundFileFormat::Ogg;
		}
		else {		//Death sentence
			format = SoundFileFormat::Mp3;
		}

		bool bSuccess = false;

		//Create the sound player object
		switch (format) {
		case SoundFileFormat::Wave:
			res.reset(new SoundSourceDataWave());
			bSuccess = res->Load(reader);
			break;
		case SoundFileFormat::Ogg:
			res.reset(new SoundSourceDataOgg());
			bSuccess = res->Load(reader);
			break;
		case SoundFileFormat::Mp3:
			res.reset(new SoundSourceDataMp3());
			bSuccess = res->Load(reader);
			break;
		}

		if (res) {
			res->path_ = path;
			res->pathHash_ = std::hash<std::wstring>{}(path);
			res->format_ = format;
		}

		if (bSuccess) {
			mapSoundSource_[path] = res;
			std::wstring str = StringUtility::Format(L"DirectSound: Audio loaded [%s]", path.c_str());
			Logger::WriteTop(str);
		}
		else {
			res = nullptr;
			std::wstring str = StringUtility::Format(L"DirectSound: Audio load failed [%s]", path.c_str());
			Logger::WriteTop(str);
		}
	}
	catch (gstd::wexception& e) {
		res = nullptr;
		std::wstring str = StringUtility::Format(L"DirectSound：Audio load failed [%s]\r\n\t%s", path.c_str(), e.what());
		Logger::WriteTop(str);
	}
	return res;
}
shared_ptr<SoundPlayer> DirectSoundManager::CreatePlayer(shared_ptr<SoundSourceData> source) {
	if (source == nullptr) return nullptr;

	const std::wstring& path = source->path_;
	shared_ptr<SoundPlayer> res;

	try {
		//Create the sound player object
		switch (source->format_) {
		case SoundFileFormat::Wave:
			if (source->audioSizeTotal_ < 1024 * 1024) {
				//The audio is small enough (<1MB), just load the entire thing into memory
				//Max: ~23.78sec at 44100hz
				res = std::shared_ptr<SoundPlayerWave>(new SoundPlayerWave(), SoundPlayer::PtrDelete);
			}
			else {
				//File too bigg uwu owo, pleasm be gentwe and take it easwy owo *blushes*
				res = std::shared_ptr<SoundStreamingPlayerWave>(new SoundStreamingPlayerWave(), SoundPlayer::PtrDelete);
			}
			break;
		case SoundFileFormat::Ogg:
			res = std::shared_ptr<SoundStreamingPlayerOgg>(new SoundStreamingPlayerOgg(), SoundPlayer::PtrDelete);
			break;
		case SoundFileFormat::Mp3:
			res = std::shared_ptr<SoundStreamingPlayerMp3>(new SoundStreamingPlayerMp3(), SoundPlayer::PtrDelete);
			break;
		}

		bool bSuccess = false;
		if (res) {
			//Create a DirectSound buffer
			bSuccess = res->_CreateBuffer(source);
			if (bSuccess)
				res->Seek(0.0);
		}

		if (bSuccess) {
			res->manager_ = this;

			res->path_ = path;
			res->pathHash_ = std::hash<std::wstring>{}(path);

			listManagedPlayer_.push_back(res);
			/*
			std::wstring str = StringUtility::Format(L"DirectSound: Sound player created [%s]", path.c_str());
			Logger::WriteTop(str);
			*/
		}
		else {
			res = nullptr;
			std::wstring str = StringUtility::Format(L"DirectSound: Sound player create failed [%s]", path.c_str());
			Logger::WriteTop(str);
		}
	}
	catch (gstd::wexception& e) {
		res = nullptr;
		std::wstring str = StringUtility::Format(L"DirectSound：Sound player create failed [%s]\r\n\t%s", path.c_str(), e.what());
		Logger::WriteTop(str);
	}
	return res;
}
shared_ptr<SoundPlayer> DirectSoundManager::GetPlayer(const std::wstring& path) {
	shared_ptr<SoundPlayer> res;
	{
		Lock lock(lock_);

		size_t hash = std::hash<std::wstring>{}(path);

		for (auto itrPlayer = listManagedPlayer_.begin(); itrPlayer != listManagedPlayer_.end(); ++itrPlayer) {
			shared_ptr<SoundPlayer> player = *itrPlayer;
			if (hash == player->GetPathHash()) {
				res = player;
				break;
			}
		}
	}
	return res;
}
SoundDivision* DirectSoundManager::CreateSoundDivision(int index) {
	auto itrDiv = mapDivision_.find(index);
	if (itrDiv != mapDivision_.end())
		return itrDiv->second;

	SoundDivision* division = new SoundDivision();
	mapDivision_[index] = division;
	return division;
}
SoundDivision* DirectSoundManager::GetSoundDivision(int index) {
	auto itrDiv = mapDivision_.find(index);
	if (itrDiv == mapDivision_.end()) return nullptr;
	return itrDiv->second;
}
void DirectSoundManager::SetFadeDeleteAll() {
	try {
		Lock lock(lock_);

		for (auto itrPlayer = listManagedPlayer_.begin(); itrPlayer != listManagedPlayer_.end(); ++itrPlayer) {
			SoundPlayer* player = itrPlayer->get();
			if (player == nullptr) continue;
			player->SetFadeDelete(SoundPlayer::FADE_DEFAULT);
		}
	}
	catch (...) {}
}

//DirectSoundManager::SoundManageThread
DirectSoundManager::SoundManageThread::SoundManageThread(DirectSoundManager* manager) {
	_SetOuter(manager);
	timeCurrent_ = 0;
	timePrevious_ = 0;
}
void DirectSoundManager::SoundManageThread::_Run() {
	DirectSoundManager* manager = _GetOuter();
	while (this->GetStatus() == RUN) {
		timeCurrent_ = ::timeGetTime();

		{
			Lock lock(manager->GetLock());
			_Fade();
			_Arrange();
		}

		if (manager->panelInfo_ != nullptr && this->GetStatus() == RUN)
			manager->panelInfo_->Update(manager);
		
		timePrevious_ = timeCurrent_;
		::Sleep(100);
	}
}
void DirectSoundManager::SoundManageThread::_Arrange() {
	DirectSoundManager* manager = _GetOuter();

	{
		auto* listPlayer = &manager->listManagedPlayer_;
		for (auto itrPlayer = listPlayer->begin(); itrPlayer != listPlayer->end();) {
			SoundPlayer* player = itrPlayer->get();
			if (player == nullptr) {
				itrPlayer = listPlayer->erase(itrPlayer);
				continue;
			}

			bool bDelete = player->bDelete_ || (player->bAutoDelete_ && !player->IsPlaying());

			if (bDelete) {
				Logger::WriteTop(StringUtility::Format(L"DirectSound: Released player [%s]", player->GetPath().c_str()));
				player->Stop();
				itrPlayer = listPlayer->erase(itrPlayer);
			}
			else ++itrPlayer;
		}
	}
	{
		auto* mapSource = &manager->mapSoundSource_;
		for (auto itrSource = mapSource->begin(); itrSource != mapSource->end();) {
			shared_ptr<SoundSourceData> source = itrSource->second;
			if (source == nullptr) {
				itrSource = mapSource->erase(itrSource);
				continue;
			}

			//1 in here, and 1 in the sound manager = not preloaded or not owned by any sound player
			bool bDelete = source.use_count() == 2;

			if (bDelete) {
				Logger::WriteTop(StringUtility::Format(L"DirectSound: Released data [%s]", source->path_.c_str()));
				itrSource = mapSource->erase(itrSource);
			}
			else ++itrSource;
		}
	}
}
void DirectSoundManager::SoundManageThread::_Fade() {
	DirectSoundManager* manager = _GetOuter();
	int timeGap = timeCurrent_ - timePrevious_;

	auto* listPlayer = &manager->listManagedPlayer_;
	for (auto itrPlayer = listPlayer->begin(); itrPlayer != listPlayer->end(); ++itrPlayer) {
		SoundPlayer* player = itrPlayer->get();
		if (player == nullptr) continue;

		double rateFade = player->GetFadeVolumeRate();
		if (rateFade == 0) continue;

		double rateVolume = player->GetVolumeRate();
		rateFade *= (double)timeGap / (double)1000.0;
		rateVolume += rateFade;
		player->SetVolumeRate(rateVolume);

		if (rateVolume <= 0 && player->bFadeDelete_) {
			player->Stop();
			player->Delete();
		}
	}
}


//*******************************************************************
//SoundInfoPanel
//*******************************************************************
SoundInfoPanel::SoundInfoPanel() {
	timeLastUpdate_ = 0;
	timeUpdateInterval_ = 500;
}
bool SoundInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);

	wndListView_.AddColumn(64, ROW_ADDRESS, L"Address");
	wndListView_.AddColumn(96, ROW_FILENAME, L"Name");
	wndListView_.AddColumn(48, ROW_FULLPATH, L"Path");
	wndListView_.AddColumn(32, ROW_COUNT_REFFRENCE, L"Ref");

	return true;
}
void SoundInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	wndListView_.SetBounds(wx, wy, wWidth, wHeight);
}
void SoundInfoPanel::Update(DirectSoundManager* soundManager) {
	if (!IsWindowVisible()) return;

	{
		int time = timeGetTime();
		if (abs(time - timeLastUpdate_) < timeUpdateInterval_) return;
		timeLastUpdate_ = time;
	}

	struct _Info {
		uint32_t address;
		std::wstring path;
		int countRef;
	};

	std::vector<_Info> listInfo;
	{
		Lock lock(soundManager->GetLock());

		auto* mapSource = &soundManager->mapSoundSource_;
		for (auto itrSource = mapSource->begin(); itrSource != mapSource->end(); ++itrSource) {
			const std::wstring& path = itrSource->first;
			shared_ptr<SoundSourceData>& source = itrSource->second;

			{
				_Info info;
				info.address = (uint32_t)source.get();
				info.path = path;
				info.countRef = source.use_count();
				listInfo.push_back(info);
			}
		}
	}

	{
		size_t i = 0;
		for (auto itrInfo = listInfo.begin(); itrInfo != listInfo.end(); ++itrInfo, ++i) {
			_Info* pInfo = itrInfo._Ptr;

			wndListView_.SetText(i, ROW_ADDRESS, StringUtility::Format(L"%08x", pInfo->address));
			wndListView_.SetText(i, ROW_FILENAME, PathProperty::GetFileName(pInfo->path));
			wndListView_.SetText(i, ROW_FULLPATH, pInfo->path);
			wndListView_.SetText(i, ROW_COUNT_REFFRENCE, StringUtility::Format(L"%d", pInfo->countRef));
		}
		for (; i < wndListView_.GetRowCount(); ++i) {
			wndListView_.DeleteRow(i);
		}
	}

	/*
	{
		Lock lock(soundManager->GetLock());

		IDirectSound8* directSound = soundManager->GetDirectSound();
		directSound->GetCaps(const_cast<LPDSCAPS>(soundManager->GetDeviceCaps()));
		UINT sndMem = soundManager->GetDeviceCaps()->dwFreeHwMemBytes / (1024U * 1024U);

		WindowLogger* logger = WindowLogger::GetParent();
		if (logger) {
			ref_count_ptr<WStatusBar> statusBar = logger->GetStatusBar();
			statusBar->SetText(0, L"Available Sound Memory");
			statusBar->SetText(1, StringUtility::Format(L"%u MB", sndMem));
		}
	}
	*/
}
//*******************************************************************
//SoundDivision
//*******************************************************************
SoundDivision::SoundDivision() {
	rateVolume_ = 100;
}
SoundDivision::~SoundDivision() {
}

//*******************************************************************
//SoundSourceData
//*******************************************************************
SoundSourceData::SoundSourceData() {
	path_ = L"";
	pathHash_ = 0;

	format_ = SoundFileFormat::Unknown;
	ZeroMemory(&formatWave_, sizeof(WAVEFORMATEX));

	audioSizeTotal_ = 0;
}

SoundSourceDataWave::SoundSourceDataWave() {
	posWaveStart_ = 0;
	posWaveEnd_ = 0;
}
bool SoundSourceDataWave::Load(shared_ptr<gstd::FileReader> reader) {
	reader_ = reader;
	reader->SetFilePointerBegin();

	try {
		byte chunk[4];
		uint32_t sizeChunk = 0;
		uint32_t sizeRiff = 0;

		//First, check if we're actually reading a .wav
		reader->Read(&chunk, 4);
		if (memcmp(chunk, "RIFF", 4) != 0) throw false;
		reader->Read(&sizeRiff, sizeof(uint32_t));
		reader->Read(&chunk, 4);
		if (memcmp(chunk, "WAVE", 4) != 0) throw false;

		bool bReadValidFmtChunk = false;
		uint32_t fmtChunkOffset = 0;
		bool bFoundValidDataChunk = false;
		uint32_t dataChunkOffset = 0;

		//Scan chunks
		while (reader->Read(&chunk, 4)) {
			reader->Read(&sizeChunk, sizeof(uint32_t));

			if (!bReadValidFmtChunk && memcmp(chunk, "fmt ", 4) == 0 && sizeChunk >= 0x10) {
				bReadValidFmtChunk = true;
				fmtChunkOffset = reader->GetFilePointer() - sizeof(uint32_t);
			}
			else if (!bFoundValidDataChunk && memcmp(chunk, "data", 4) == 0) {
				bFoundValidDataChunk = true;
				dataChunkOffset = reader->GetFilePointer() - sizeof(uint32_t);
			}

			reader->Seek(reader->GetFilePointer() + sizeChunk);
			if (bReadValidFmtChunk && bFoundValidDataChunk) break;
		}

		if (!bReadValidFmtChunk) throw gstd::wexception("wave format not found");
		if (!bFoundValidDataChunk) throw gstd::wexception("wave data not found");

		reader->Seek(fmtChunkOffset);
		reader->Read(&sizeChunk, sizeof(uint32_t));
		reader->Read(&formatWave_, sizeChunk);

		switch (formatWave_.wFormatTag) {
		case WAVE_FORMAT_UNKNOWN:
		case WAVE_FORMAT_EXTENSIBLE:
		case WAVE_FORMAT_DEVELOPMENT:
			//Unsupported format type
			throw gstd::wexception("unsupported wave format");
		}

		reader->Seek(dataChunkOffset);
		reader->Read(&sizeChunk, sizeof(uint32_t));

		//sizeChunk is now the size of the wave data
		audioSizeTotal_ = sizeChunk;

		posWaveStart_ = dataChunkOffset + sizeof(uint32_t);
		posWaveEnd_ = posWaveStart_ + sizeChunk;
	}
	catch (bool) {
		return false;
	}
	catch (gstd::wexception& e) {
		throw e;
	}

	return true;
}

SoundSourceDataOgg::SoundSourceDataOgg() {
}
SoundSourceDataOgg::~SoundSourceDataOgg() {
	ov_clear(&fileOgg_);
}
bool SoundSourceDataOgg::Load(shared_ptr<gstd::FileReader> reader) {
	reader_ = reader;
	reader->SetFilePointerBegin();

	try {
		oggCallBacks_.read_func = SoundSourceDataOgg::_ReadOgg;
		oggCallBacks_.seek_func = SoundSourceDataOgg::_SeekOgg;
		oggCallBacks_.close_func = SoundSourceDataOgg::_CloseOgg;
		oggCallBacks_.tell_func = SoundSourceDataOgg::_TellOgg;

		if (ov_open_callbacks((void*)this, &fileOgg_, nullptr, 0, oggCallBacks_) < 0)
			throw false;

		vorbis_info* vi = ov_info(&fileOgg_, -1);
		if (vi == nullptr) {
			ov_clear(&fileOgg_);
			throw false;
		}

		formatWave_.cbSize = sizeof(WAVEFORMATEX);
		formatWave_.wFormatTag = WAVE_FORMAT_PCM;
		formatWave_.nChannels = vi->channels;
		formatWave_.nSamplesPerSec = vi->rate;
		formatWave_.nAvgBytesPerSec = vi->rate * vi->channels * 2;
		formatWave_.nBlockAlign = vi->channels * 2;		//Bytes per sample
		formatWave_.wBitsPerSample = 2 * 8;
		formatWave_.cbSize = 0;

		QWORD pcmTotal = ov_pcm_total(&fileOgg_, -1);
		audioSizeTotal_ = pcmTotal * formatWave_.nBlockAlign;
	}
	catch (bool) {
		return false;
	}
	catch (gstd::wexception& e) {
		throw e;
	}

	return true;
}
size_t SoundSourceDataOgg::_ReadOgg(void* ptr, size_t size, size_t nmemb, void* source) {
	SoundSourceDataOgg* parent = (SoundSourceDataOgg*)source;

	size_t sizeCopy = size * nmemb;
	sizeCopy = parent->reader_->Read(ptr, sizeCopy);

	return sizeCopy / size;
}
int SoundSourceDataOgg::_SeekOgg(void* source, ogg_int64_t offset, int whence) {
	SoundSourceDataOgg* parent = (SoundSourceDataOgg*)source;
	LONG high = (LONG)((offset & 0xFFFFFFFF00000000) >> 32);
	LONG low = (LONG)((offset & 0x00000000FFFFFFFF) >> 0);

	switch (whence) {
	case SEEK_CUR:
	{
		size_t current = parent->reader_->GetFilePointer() + low;
		parent->reader_->Seek(current);
		break;
	}
	case SEEK_END:
	{
		parent->reader_->SetFilePointerEnd();
		break;
	}
	case SEEK_SET:
	{
		parent->reader_->Seek(low);
		break;
	}
	}
	return 0;
}
int SoundSourceDataOgg::_CloseOgg(void* source) {
	return 0;
}
long SoundSourceDataOgg::_TellOgg(void* source) {
	SoundSourceDataOgg* parent = (SoundSourceDataOgg*)source;
	return parent->reader_->GetFilePointer();
}

SoundSourceDataMp3::SoundSourceDataMp3() {
	hAcmStream_ = nullptr;

	ZeroMemory(&formatMp3_, sizeof(MPEGLAYER3WAVEFORMAT));
	ZeroMemory(&acmStreamHeader_, sizeof(ACMSTREAMHEADER));
	
	posMp3DataStart_ = 0;
	posMp3DataEnd_ = 0;
}
SoundSourceDataMp3::~SoundSourceDataMp3() {
	if (hAcmStream_) {
		acmStreamUnprepareHeader(hAcmStream_, &acmStreamHeader_, 0);
		acmStreamClose(hAcmStream_, 0);

		ptr_delete_scalar(acmStreamHeader_.pbSrc);
		ptr_delete_scalar(acmStreamHeader_.pbDst);
	}
}
bool SoundSourceDataMp3::Load(shared_ptr<gstd::FileReader> reader) {
	reader_ = reader;
	reader->SetFilePointerBegin();

	try {
		size_t fileSize = reader->GetFileSize();
		if (fileSize < 64) return false;

		DWORD offsetDataStart = 0;
		DWORD dataSize = 0;

		{
			byte headerFile[10];
			reader->Read(headerFile, sizeof(headerFile));
			if (memcmp(headerFile, "ID3", 3) == 0) {
				auto UInt28ToInt = [](uint32_t src) -> uint32_t {
					uint32_t res = 0;
					byte* srcAsUInt8 = reinterpret_cast<byte*>(&src);
					for (size_t i = 0; i < sizeof(uint32_t); ++i) {
						res = (res << 7) | (srcAsUInt8[i] & 0b01111111);
					}
					return res;
				};

				uint32_t tagSize = UInt28ToInt(*reinterpret_cast<uint32_t*>(&headerFile[6])) + 0xA;

				//Skip ID3 tags
				offsetDataStart = tagSize;
				dataSize = fileSize - tagSize;
			}
			else if (fileSize > 128) {
				offsetDataStart = 0;

				byte tag[3];
				reader->Seek(fileSize - 128);
				reader->Read(tag, sizeof(tag));

				if (memcmp(tag, "TAG", 3) == 0)
					dataSize = fileSize - 128;
				else
					dataSize = fileSize;
			}
		}

		dataSize -= 4;	//mp3 header

		posMp3DataStart_ = offsetDataStart + 4;
		posMp3DataEnd_ = posMp3DataStart_ + dataSize;

		reader->Seek(offsetDataStart);

		byte headerData[4];
		reader->Read(headerData, sizeof(headerData));

		if (!(headerData[0] == 0xFF && (headerData[1] & 0xF0) == 0xF0))
			return false;

		static const short TABLE_BITRATE[][16] = {
			// MPEG1, Layer1
			{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 },
			// MPEG1, Layer2
			{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1 },
			// MPEG1, Layer3
			{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 },
			// MPEG2/2.5, Layer1,2
			{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1 },
			// MPEG2/2.5, Layer3
			{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1 }
		};
		static const int TABLE_SAMPLERATE[][4] = {
			{ 44100, 48000, 32000, -1 },	// MPEG1
			{ 22050, 24000, 16000, -1 },	// MPEG2
			{ 11025, 12000, 8000, -1 }		// MPEG2.5
		};

		byte version = (headerData[1] >> 3) & 0x03;
		byte layer = (headerData[1] >> 1) & 0x03;

		short bitRate = 0;
		{
			size_t indexBitrate = 0;
			if (version == 3) {
				indexBitrate = 3 - layer;
			}
			else {
				if (layer == 3) indexBitrate = 3;
				else indexBitrate = 4;
			}
			bitRate = TABLE_BITRATE[indexBitrate][headerData[2] >> 4];
		}

		size_t sampleRate = 0;
		{
			size_t indexSampleRate = 0;
			switch (version) {
			case 0: indexSampleRate = 2; break;
			case 2: indexSampleRate = 1; break;
			case 3: indexSampleRate = 0; break;
			default:
				throw false;
			}
			sampleRate = TABLE_SAMPLERATE[indexSampleRate][(headerData[2] >> 2) & 0x03];
		}

		byte padding = headerData[2] >> 1 & 0x01;
		byte channel = headerData[3] >> 6;

		DWORD mp3BlockSize = ((144000 * bitRate) / sampleRate) + padding;

		formatMp3_.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
		formatMp3_.wfx.nChannels = channel == 3 ? 1 : 2;
		formatMp3_.wfx.nSamplesPerSec = sampleRate;
		formatMp3_.wfx.nAvgBytesPerSec = (bitRate * 1000) / 8;
		formatMp3_.wfx.nBlockAlign = 1;
		formatMp3_.wfx.wBitsPerSample = 0;
		formatMp3_.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;

		formatMp3_.wID = MPEGLAYER3_ID_MPEG;
		formatMp3_.fdwFlags = padding ? MPEGLAYER3_FLAG_PADDING_ON : MPEGLAYER3_FLAG_PADDING_OFF;
		formatMp3_.nBlockSize = (WORD)mp3BlockSize;
		formatMp3_.nFramesPerBlock = 1;
		formatMp3_.nCodecDelay = 0;

		formatWave_.wFormatTag = WAVE_FORMAT_PCM;
		MMRESULT mmResSuggest = acmFormatSuggest(nullptr, &formatMp3_.wfx, &formatWave_,
			sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG);
		if (mmResSuggest != 0)
			throw false;

		MMRESULT mmResStreamOpen = acmStreamOpen(&hAcmStream_, nullptr, &formatMp3_.wfx,
			&formatWave_, nullptr, 0, 0, 0);
		if (mmResStreamOpen != 0)
			throw false;

		DWORD waveBlockSize = 0;
		acmStreamSize(hAcmStream_, mp3BlockSize, &waveBlockSize, ACM_STREAMSIZEF_SOURCE);

		DWORD totalSize = 0;
		acmStreamSize(hAcmStream_, dataSize, &totalSize, ACM_STREAMSIZEF_SOURCE);
		audioSizeTotal_ = totalSize;

		ZeroMemory(&acmStreamHeader_, sizeof(ACMSTREAMHEADER));
		acmStreamHeader_.cbStruct = sizeof(ACMSTREAMHEADER);
		acmStreamHeader_.pbSrc = new BYTE[mp3BlockSize];
		acmStreamHeader_.cbSrcLength = mp3BlockSize;
		acmStreamHeader_.pbDst = new BYTE[waveBlockSize];
		acmStreamHeader_.cbDstLength = waveBlockSize;

		MMRESULT mmResPrepareHeader = acmStreamPrepareHeader(hAcmStream_, &acmStreamHeader_, 0);
		if (mmResPrepareHeader != 0)
			return false;
	}
	catch (bool) {
		return false;
	}
	catch (gstd::wexception& e) {
		throw e;
	}

	return true;
}

//*******************************************************************
//SoundPlayer
//*******************************************************************
SoundPlayer::SoundPlayer() {
	pDirectSoundBuffer_ = nullptr;

	path_ = L"";
	pathHash_ = 0;

	playStyle_.bLoop_ = false;
	playStyle_.timeLoopStart_ = 0;
	playStyle_.timeLoopEnd_ = 0;
	playStyle_.timeStart_ = 0;
	playStyle_.bResume_ = false;

	bDelete_ = false;
	bFadeDelete_ = false;
	bAutoDelete_ = false;
	rateVolume_ = 100.0;
	rateVolumeFadePerSec_ = 0;

	bPause_ = false;

	division_ = nullptr;
	manager_ = nullptr;
}
SoundPlayer::~SoundPlayer() {
	Stop();
	ptr_release(pDirectSoundBuffer_);
}
void SoundPlayer::SetSoundDivision(SoundDivision* div) {
	division_ = div;
}
void SoundPlayer::SetSoundDivision(int index) {
	{
		Lock lock(lock_);
		DirectSoundManager* manager = DirectSoundManager::GetBase();
		SoundDivision* div = manager->GetSoundDivision(index);
		if (div) SetSoundDivision(div);
	}
}
bool SoundPlayer::Play() {
	return false;
}
bool SoundPlayer::Stop() {
	return false;
}
bool SoundPlayer::IsPlaying() {
	return false;
}
double SoundPlayer::GetVolumeRate() {
	double res = 0;
	{
		Lock lock(lock_);
		res = rateVolume_;
	}
	return res;
}
bool SoundPlayer::SetVolumeRate(double rateVolume) {
	{
		Lock lock(lock_);

		if (rateVolume < 0) rateVolume = 0.0;
		else if (rateVolume > 100) rateVolume = 100.0;
		rateVolume_ = rateVolume;

		if (pDirectSoundBuffer_) {
			double rateDiv = 100.0;
			if (division_)
				rateDiv = division_->GetVolumeRate();
			double rate = rateVolume_ / 100.0 * rateDiv / 100.0;

			//int volume = (int)((double)(DirectSoundManager::SD_VOLUME_MAX - DirectSoundManager::SD_VOLUME_MIN) * rate);
			//pDirectSoundBuffer_->SetVolume(DirectSoundManager::SD_VOLUME_MIN+volume);
			int volume = _GetVolumeAsDirectSoundDecibel(rate);
			pDirectSoundBuffer_->SetVolume(volume);
		}
	}

	return true;
}
bool SoundPlayer::SetPanRate(double ratePan) {
	{
		Lock lock(lock_);
		if (ratePan < -100) ratePan = -100.0;
		else if (ratePan > 100) ratePan = 100.0;

		if (pDirectSoundBuffer_) {
			double rateDiv = 100.0;
			if (division_)
				rateDiv = division_->GetVolumeRate();
			double rate = rateVolume_ / 100.0 * rateDiv / 100.0;

			LONG volume = (LONG)((DirectSoundManager::SD_VOLUME_MAX - DirectSoundManager::SD_VOLUME_MIN) * rate);
			//int volume = _GetValumeAsDirectSoundDecibel(rate);

			double span = (DSBPAN_RIGHT - DSBPAN_LEFT) / 2;
			span = volume / 2;
			double pan = span * ratePan / 100;
			HRESULT hr = pDirectSoundBuffer_->SetPan((LONG)pan);
		}
	}

	return true;
}
double SoundPlayer::GetFadeVolumeRate() {
	double res = 0;
	{
		Lock lock(lock_);
		res = rateVolumeFadePerSec_;
	}
	return res;
}
void SoundPlayer::SetFade(double rateVolumeFadePerSec) {
	{
		Lock lock(lock_);
		rateVolumeFadePerSec_ = rateVolumeFadePerSec;
	}
}
void SoundPlayer::SetFadeDelete(double rateVolumeFadePerSec) {
	{
		Lock lock(lock_);
		bFadeDelete_ = true;
		SetFade(rateVolumeFadePerSec);
	}
}
LONG SoundPlayer::_GetVolumeAsDirectSoundDecibel(float rate) {
	LONG result = 0;
	if (rate >= 1.0f) {
		result = DSBVOLUME_MAX;
	}
	else if (rate <= 0.0f) {
		result = DSBVOLUME_MIN;
	}
	else {
		// 10dBで音量2倍。
		// →(求めるdB)
		//	 = 10 * log2(音量)
		//	 = 10 * ( log10(音量) / log10(2) )
		//	 = 33.2... * log10(音量)
		result = (LONG)(33.2f * log10(rate) * 100);
	}
	return result;
}
DWORD SoundPlayer::GetCurrentPosition() {
	DWORD res = 0;
	if (pDirectSoundBuffer_) {
		HRESULT hr = pDirectSoundBuffer_->GetCurrentPosition(&res, nullptr);
		if (FAILED(hr)) res = 0;
	}
	return res;
}
void SoundPlayer::SetFrequency(DWORD freq) {
	if (manager_ == nullptr) return;
	if (pDirectSoundBuffer_) {
		if (freq > 0) {
			const DSCAPS* caps = manager_->GetDeviceCaps();
			DWORD rateMin = caps->dwMinSecondarySampleRate;
			DWORD rateMax = caps->dwMaxSecondarySampleRate;
			//DWORD rateMin = DSBFREQUENCY_MIN;
			//DWORD rateMax = DSBFREQUENCY_MAX;
			freq = std::clamp(freq, rateMin, rateMax);
		}
		HRESULT hr = pDirectSoundBuffer_->SetFrequency(freq);
		//std::wstring err = StringUtility::Format(L"%s: %s",
		//	DXGetErrorString(hr), DXGetErrorDescription(hr));
	}
}

//*******************************************************************
//SoundStreamingPlayer
//*******************************************************************
SoundStreamingPlayer::SoundStreamingPlayer() {
	pDirectSoundNotify_ = nullptr;
	ZeroMemory(hEvent_, sizeof(HANDLE) * 3);
	thread_ = new StreamingThread(this);

	bStreaming_ = true;
	bStreamOver_ = false;
	streamOverIndex_ = -1;

	ZeroMemory(lastStreamCopyPos_, sizeof(DWORD) * 2);
	ZeroMemory(bufferPositionAtCopy_, sizeof(DWORD) * 2);

	lastReadPointer_ = 0;
}
SoundStreamingPlayer::~SoundStreamingPlayer() {
	this->Stop();

	for (size_t iEvent = 0; iEvent < 3; ++iEvent)
		::CloseHandle(hEvent_[iEvent]);
	ptr_release(pDirectSoundNotify_);
	delete thread_;
}
void SoundStreamingPlayer::_CreateSoundEvent(WAVEFORMATEX& formatWave) {
	sizeCopy_ = formatWave.nAvgBytesPerSec;

	HRESULT hrNotify = pDirectSoundBuffer_->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&pDirectSoundNotify_);
	DSBPOSITIONNOTIFY pn[3];
	for (size_t iEvent = 0; iEvent < 3; ++iEvent) {
		hEvent_[iEvent] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		pn[iEvent].hEventNotify = hEvent_[iEvent];
	}

	pn[0].dwOffset = 0;
	pn[1].dwOffset = sizeCopy_;
	pn[2].dwOffset = DSBPN_OFFSETSTOP;
	pDirectSoundNotify_->SetNotificationPositions(3, pn);

	if (hrNotify == DSERR_BUFFERLOST) {
		this->Restore();
	}
}
void SoundStreamingPlayer::_CopyStream(int indexCopy) {
	{
		Lock lock(lock_);
		LPVOID pMem1, pMem2;
		DWORD dwSize1, dwSize2;

		pDirectSoundBuffer_->GetCurrentPosition(&bufferPositionAtCopy_[indexCopy], nullptr);

		HRESULT hr = pDirectSoundBuffer_->Lock(sizeCopy_ * indexCopy, sizeCopy_, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
		if (hr == DSERR_BUFFERLOST) {
			hr = pDirectSoundBuffer_->Restore();
			hr = pDirectSoundBuffer_->Lock(sizeCopy_ * indexCopy, sizeCopy_, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
		}

		if (FAILED(hr))
			Stop();
		else {
			if (bStreamOver_) {
				if (indexCopy == streamOverIndex_) {
					streamOverIndex_ = -1;
					Stop();
					goto lab_unlock_buf;
				}
				goto lab_set_mem_zero;
			}
			else if (!bStreaming_ || IsPlaying()) {
				if (dwSize1 > 0) {
					DWORD res = _CopyBuffer(pMem1, dwSize1);
					lastStreamCopyPos_[indexCopy] = res;
				}
				if (dwSize2 > 0) {
					_CopyBuffer(pMem2, dwSize2);
				}
				if (bStreamOver_)
					streamOverIndex_ = indexCopy;
			}
			else {
lab_set_mem_zero:
				memset(pMem1, 0, dwSize1);
				if (dwSize2 != 0)
					memset(pMem2, 0, dwSize2);
			}

lab_unlock_buf:
			pDirectSoundBuffer_->Unlock(pMem1, dwSize1, pMem2, dwSize2);
		}
	}
}
bool SoundStreamingPlayer::Play() {
	if (IsPlaying()) return true;

	{
		Lock lock(lock_);

		if (bFadeDelete_)
			SetVolumeRate(100);
		bFadeDelete_ = false;

		SetFade(0);

		bStreamOver_ = false;
		if (!bPause_ || !playStyle_.bResume_) {
			this->Seek(playStyle_.timeStart_);
			pDirectSoundBuffer_->SetCurrentPosition(0);
		}
		playStyle_.timeStart_ = 0;

		if (bStreaming_) {
			::ResetEvent(hEvent_[0]);
			::ResetEvent(hEvent_[1]);
			::ResetEvent(hEvent_[2]);

			thread_->Start();
			pDirectSoundBuffer_->Play(0, 0, DSBPLAY_LOOPING);//再生開始
		}
		else {
			DWORD dwFlags = 0;
			if (playStyle_.bLoop_)
				dwFlags = DSBPLAY_LOOPING;
			pDirectSoundBuffer_->Play(0, 0, dwFlags);
		}
		bPause_ = false;
	}
	return true;
}
bool SoundStreamingPlayer::Stop() {
	{
		Lock lock(lock_);

		if (IsPlaying())
			bPause_ = true;

		if (pDirectSoundBuffer_)
			pDirectSoundBuffer_->Stop();

		thread_->Stop();
	}
	return true;
}
void SoundStreamingPlayer::ResetStreamForSeek() {
	if (pDirectSoundBuffer_) {
		pDirectSoundBuffer_->SetCurrentPosition(0);
		_CopyStream(1);
		//DWORD posCopy = lastStreamCopyPos_;
		_CopyStream(0);
		//lastStreamCopyPos_ = posCopy;
	}
}
bool SoundStreamingPlayer::IsPlaying() {
	return thread_->GetStatus() == Thread::RUN;
}
DWORD SoundStreamingPlayer::GetCurrentPosition() {
	DWORD currentReader = 0;
	if (pDirectSoundBuffer_) {
		HRESULT hr = pDirectSoundBuffer_->GetCurrentPosition(&currentReader, nullptr);
		if (FAILED(hr)) currentReader = 0;
	}
	
	DWORD p0 = lastStreamCopyPos_[0];
	DWORD p1 = lastStreamCopyPos_[1];
	if (p0 < p1)
		return p0 + currentReader;
	else
		return p1 + currentReader - bufferPositionAtCopy_[0];
}

//StreamingThread
void SoundStreamingPlayer::StreamingThread::_Run() {
	SoundStreamingPlayer* player = _GetOuter();

	DWORD point = 0;
	player->pDirectSoundBuffer_->GetCurrentPosition(&point, 0);
	if (point == 0)
		player->_CopyStream(0);

	while (this->GetStatus() == RUN) {
		DWORD num = WaitForMultipleObjects(3, player->hEvent_, FALSE, INFINITE);
		if (num == WAIT_OBJECT_0)
			player->_CopyStream(1);
		else if (num == WAIT_OBJECT_0 + 1)
			player->_CopyStream(0);
		else if (num == WAIT_OBJECT_0 + 2)
			break;
	}
}
void SoundStreamingPlayer::StreamingThread::Notify(size_t index) {
	SoundStreamingPlayer* player = _GetOuter();
	SetEvent(player->hEvent_[index]);
}

//*******************************************************************
//SoundPlayerWave
//*******************************************************************
SoundPlayerWave::SoundPlayerWave() {

}
SoundPlayerWave::~SoundPlayerWave() {
}
bool SoundPlayerWave::_CreateBuffer(shared_ptr<SoundSourceData> source) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	soundSource_ = source;

	if (auto pSource = std::dynamic_pointer_cast<SoundSourceDataWave>(source)) {
		shared_ptr<FileReader> reader = pSource->reader_;

		try {
			QWORD waveSize = pSource->audioSizeTotal_;

			DSBUFFERDESC desc;
			ZeroMemory(&desc, sizeof(DSBUFFERDESC));
			desc.dwSize = sizeof(DSBUFFERDESC);
			desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY
				| DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER;
			desc.dwBufferBytes = waveSize;
			desc.lpwfxFormat = &pSource->formatWave_;
			HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc,
				(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
			if (FAILED(hrBuffer))
				throw gstd::wexception("IDirectSound8::CreateSoundBuffer failure");

			{
				LPVOID pMem1, pMem2;
				DWORD dwSize1, dwSize2;

				HRESULT hrLock = pDirectSoundBuffer_->Lock(0, waveSize, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
				if (hrLock == DSERR_BUFFERLOST) {
					hrLock = pDirectSoundBuffer_->Restore();
					hrLock = pDirectSoundBuffer_->Lock(0, waveSize, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
				}
				if (FAILED(hrLock))
					throw gstd::wexception("IDirectSoundBuffer8::Lock failure");

				reader->Seek(pSource->posWaveStart_);
				reader->Read(pMem1, dwSize1);
				if (dwSize2 != 0)
					reader->Read(pMem2, dwSize2);
				pDirectSoundBuffer_->Unlock(pMem1, dwSize1, pMem2, dwSize2);
			}
		}
		catch (bool) {
			return false;
		}
		catch (gstd::wexception& e) {
			throw e;
		}
	}
	else return false;

	return true;
}
bool SoundPlayerWave::Play() {
	{
		Lock lock(lock_);
		if (bFadeDelete_)
			SetVolumeRate(100);
		bFadeDelete_ = false;

		SetFade(0);

		DWORD dwFlags = 0;
		if (playStyle_.bLoop_)
			dwFlags = DSBPLAY_LOOPING;

		if (!bPause_ || !playStyle_.bResume_) {
			this->Seek(playStyle_.timeStart_);
		}
		playStyle_.timeStart_ = 0;

		HRESULT hr = pDirectSoundBuffer_->Play(0, 0, dwFlags);
		if (hr == DSERR_BUFFERLOST) {
			this->Restore();
		}

		bPause_ = false;
	}
	return true;
}
bool SoundPlayerWave::Stop() {
	{
		Lock lock(lock_);
		if (IsPlaying())
			bPause_ = true;

		if (pDirectSoundBuffer_)
			pDirectSoundBuffer_->Stop();
	}
	return true;
}
bool SoundPlayerWave::IsPlaying() {
	if (pDirectSoundBuffer_ == nullptr) return false;
	DWORD status = 0;
	pDirectSoundBuffer_->GetStatus(&status);
	bool res = (status & DSBSTATUS_PLAYING) > 0;
	return res;
}
bool SoundPlayerWave::Seek(double time) {
	if (soundSource_ == nullptr) return false;
	return Seek((DWORD)(time * soundSource_->formatWave_.nSamplesPerSec));
}
bool SoundPlayerWave::Seek(DWORD sample) {
	if (soundSource_ == nullptr) return false;
	{
		Lock lock(lock_);
		pDirectSoundBuffer_->SetCurrentPosition(sample * soundSource_->formatWave_.nBlockAlign);
	}
	return true;
}
//*******************************************************************
//SoundStreamingPlayerWave
//*******************************************************************
SoundStreamingPlayerWave::SoundStreamingPlayerWave() {
}
bool SoundStreamingPlayerWave::_CreateBuffer(shared_ptr<SoundSourceData> source) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	soundSource_ = source;

	if (auto pSource = std::dynamic_pointer_cast<SoundSourceDataWave>(source)) {
		shared_ptr<FileReader> reader = pSource->reader_;

		try {
			DSBUFFERDESC desc;
			ZeroMemory(&desc, sizeof(DSBUFFERDESC));
			desc.dwSize = sizeof(DSBUFFERDESC);
			desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY
				| DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
				| DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS;
			desc.dwBufferBytes = 2 * pSource->formatWave_.nAvgBytesPerSec;
			desc.lpwfxFormat = &pSource->formatWave_;
			HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc,
				(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
			if (FAILED(hrBuffer))
				throw gstd::wexception("IDirectSound8::CreateSoundBuffer failure");

			_CreateSoundEvent(pSource->formatWave_);
		}
		catch (bool) {
			return false;
		}
		catch (gstd::wexception& e) {
			throw e;
		}
	}
	else return false;

	return true;
}
DWORD SoundStreamingPlayerWave::_CopyBuffer(LPVOID pMem, DWORD dwSize) {
	SoundSourceDataWave* source = (SoundSourceDataWave*)soundSource_.get();
	auto& reader = source->reader_;

	double loopStart = playStyle_.timeLoopStart_;
	double loopEnd = playStyle_.timeLoopEnd_;

	DWORD posLoopStart = source->posWaveStart_ + loopStart * source->formatWave_.nAvgBytesPerSec;
	DWORD posLoopEnd = source->posWaveStart_ + loopEnd * source->formatWave_.nAvgBytesPerSec;
	DWORD blockSize = source->formatWave_.nBlockAlign;

	reader->Seek(lastReadPointer_);

	DWORD cPos = lastReadPointer_;
	DWORD resStreamPos = cPos;

	auto _ReadStreamWrapped = [&](DWORD readSize) {
		DWORD sizeNext = dwSize - readSize;

		reader->Read(pMem, readSize);

		if (playStyle_.bLoop_) {
			Seek(posLoopStart);
			reader->Read((char*)pMem + readSize, sizeNext);
		}
		else {
			memset((char*)pMem + readSize, 0, sizeNext);
			_SetStreamOver();
		}
	};

	if (cPos + dwSize > source->posWaveEnd_) {				//This read will contain the EOF
		DWORD sizeRemain = source->posWaveEnd_ - cPos;		//Size until the EOF

		_ReadStreamWrapped(sizeRemain);
	}
	else if (cPos + dwSize > posLoopEnd && loopEnd > 0) {	//This read will contain the looping point
		DWORD sizeRemain = posLoopEnd - cPos;				//Size until the loop

		_ReadStreamWrapped(sizeRemain);
	}
	else {
		reader->Read(pMem, dwSize);
	}

	lastReadPointer_ = reader->GetFilePointer();
	return resStreamPos;
}
bool SoundStreamingPlayerWave::Seek(double time) {
	if (soundSource_ == nullptr) return false;
	SoundSourceDataWave* source = (SoundSourceDataWave*)soundSource_.get();
	return Seek((DWORD)(time * source->formatWave_.nSamplesPerSec));
}
bool SoundStreamingPlayerWave::Seek(DWORD sample) {
	if (soundSource_ == nullptr) return false;
	SoundSourceDataWave* source = (SoundSourceDataWave*)soundSource_.get();
	{
		Lock lock(lock_);
		source->reader_->Seek(source->posWaveStart_ + sample * source->formatWave_.nBlockAlign);
		lastReadPointer_ = source->reader_->GetFilePointer();
	}
	return true;
}
//*******************************************************************
//SoundStreamingPlayerOgg
//*******************************************************************
SoundStreamingPlayerOgg::SoundStreamingPlayerOgg() {}
SoundStreamingPlayerOgg::~SoundStreamingPlayerOgg() {
	this->Stop();
	thread_->Join();
}
bool SoundStreamingPlayerOgg::_CreateBuffer(shared_ptr<SoundSourceData> source) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	soundSource_ = source;

	if (auto pSource = std::dynamic_pointer_cast<SoundSourceDataOgg>(source)) {
		shared_ptr<FileReader> reader = pSource->reader_;

		try {
			DWORD sizeBuffer = std::min(2 * pSource->formatWave_.nAvgBytesPerSec, (DWORD)pSource->audioSizeTotal_);

			DSBUFFERDESC desc;
			ZeroMemory(&desc, sizeof(DSBUFFERDESC));
			desc.dwSize = sizeof(DSBUFFERDESC);
			desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY
				| DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
				| DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS;
			desc.dwBufferBytes = sizeBuffer;
			desc.lpwfxFormat = &pSource->formatWave_;
			HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc,
				(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
			if (FAILED(hrBuffer))
				throw gstd::wexception("IDirectSound8::CreateSoundBuffer failure");

			_CreateSoundEvent(pSource->formatWave_);

			bStreaming_ = sizeBuffer != pSource->audioSizeTotal_;
			if (!bStreaming_) {
				sizeCopy_ = pSource->audioSizeTotal_;
				_CopyStream(0);
			}
			else {
				ov_pcm_seek(&pSource->fileOgg_, 0);
			}
		}
		catch (bool) {
			return false;
		}
		catch (gstd::wexception& e) {
			throw e;
		}
	}
	else return false;

	return true;
}
DWORD SoundStreamingPlayerOgg::_CopyBuffer(LPVOID pMem, DWORD dwSize) {
	SoundSourceDataOgg* source = (SoundSourceDataOgg*)soundSource_.get();
	auto& reader = source->reader_;
	OggVorbis_File* pFileOgg = &source->fileOgg_;

	DWORD samplePerSec = source->formatWave_.nSamplesPerSec;
	DWORD bytePerSample = source->formatWave_.nBlockAlign;
	DWORD bytePerSec = source->formatWave_.nAvgBytesPerSec;

	double loopStart = playStyle_.timeLoopStart_;
	double loopEnd = playStyle_.timeLoopEnd_;
	DWORD byteLoopStart = loopStart * bytePerSec;
	DWORD byteLoopEnd = loopEnd * bytePerSec;

	ov_pcm_seek(pFileOgg, lastReadPointer_);

	DWORD resStreamPos = lastReadPointer_ * bytePerSample;

	auto _DecodeOgg = [&](DWORD writeOff, DWORD writeTargetSize, DWORD* pWritten) -> bool {
		if (writeTargetSize == 0) return true;
		DWORD written = 0;
		while (written < writeTargetSize) {
			DWORD _remain = writeTargetSize - written;
			DWORD read = ov_read(pFileOgg, (char*)pMem + writeOff + written, _remain, 0, 2, 1, nullptr);
			if (read == 0) {
				*pWritten = written;
				return false;	//EOF
			}
			written += read;
		}
		*pWritten = written;
		return true;
	};

	DWORD pcmCurrent = ov_pcm_tell(pFileOgg);
	DWORD byteCurrent = pcmCurrent * bytePerSample;

	bool bReadLoopEnd = byteCurrent + dwSize > byteLoopEnd && byteLoopEnd > 0;
	bool bReadEof = byteCurrent + dwSize > source->audioSizeTotal_;
	if (bReadLoopEnd || bReadEof) {
		DWORD size1 = bReadLoopEnd ? (byteLoopEnd - byteCurrent) : dwSize;

		DWORD written = 0;
		bool bFileEnd = _DecodeOgg(0, size1, &written);

		DWORD remain = dwSize - written;
		if (playStyle_.bLoop_) {
			Seek(loopStart);
			bFileEnd = _DecodeOgg(written, remain, &written);
		}
		else {
			memset((char*)pMem + written, 0, remain);
			_SetStreamOver();
		}
	}
	else {
		DWORD written = 0;
		_DecodeOgg(0, dwSize, &written);
	}

	lastReadPointer_ = ov_pcm_tell(pFileOgg);
	return resStreamPos;
}
bool SoundStreamingPlayerOgg::Seek(double time) {
	if (soundSource_ == nullptr) return false;
	SoundSourceDataOgg* source = (SoundSourceDataOgg*)soundSource_.get();
	{
		Lock lock(lock_);
		ov_time_seek(&source->fileOgg_, time);
		lastReadPointer_ = time * source->formatWave_.nSamplesPerSec;
	}
	return true;
}
bool SoundStreamingPlayerOgg::Seek(DWORD sample) {
	if (soundSource_ == nullptr) return false;
	SoundSourceDataOgg* source = (SoundSourceDataOgg*)soundSource_.get();
	{
		Lock lock(lock_);
		ov_pcm_seek(&source->fileOgg_, sample);
		lastReadPointer_ = sample;
	}
	return true;
}

//*******************************************************************
//SoundStreamingPlayerMp3
//*******************************************************************
SoundStreamingPlayerMp3::SoundStreamingPlayerMp3() {
	timeCurrent_ = 0;
}
SoundStreamingPlayerMp3::~SoundStreamingPlayerMp3() {
}
bool SoundStreamingPlayerMp3::_CreateBuffer(shared_ptr<SoundSourceData> source) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	soundSource_ = source;

	if (auto pSource = std::dynamic_pointer_cast<SoundSourceDataMp3>(source)) {
		shared_ptr<FileReader> reader = pSource->reader_;

		try {
			DWORD sizeBuffer = std::min(2 * pSource->formatWave_.nAvgBytesPerSec, (DWORD)pSource->audioSizeTotal_);

			DSBUFFERDESC desc;
			ZeroMemory(&desc, sizeof(DSBUFFERDESC));
			desc.dwSize = sizeof(DSBUFFERDESC);
			desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY
				| DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
				| DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS;
			desc.dwBufferBytes = sizeBuffer;
			desc.lpwfxFormat = &pSource->formatWave_;
			HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc,
				(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
			if (FAILED(hrBuffer))
				throw gstd::wexception("IDirectSound8::CreateSoundBuffer failure");

			_CreateSoundEvent(pSource->formatWave_);

			bStreaming_ = sizeBuffer != pSource->audioSizeTotal_;
			if (!bStreaming_) {
				sizeCopy_ = pSource->audioSizeTotal_;
				_CopyStream(0);
			}

			reader->Seek(pSource->posMp3DataStart_);
		}
		catch (bool) {
			return false;
		}
		catch (gstd::wexception& e) {
			throw e;
		}
	}
	else return false;

	return true;
}
DWORD SoundStreamingPlayerMp3::_CopyBuffer(LPVOID pMem, DWORD dwSize) {
	SoundSourceDataMp3* source = (SoundSourceDataMp3*)soundSource_.get();
	auto& reader = source->reader_;
	HACMSTREAM hAcmStream = source->hAcmStream_;
	ACMSTREAMHEADER* pAcmHeader = &source->acmStreamHeader_;

	DWORD samplePerSec = source->formatWave_.nSamplesPerSec;
	DWORD bytePerSample = source->formatWave_.nBlockAlign;
	DWORD bytePerSec = source->formatWave_.nAvgBytesPerSec;

	double loopStart = playStyle_.timeLoopStart_;
	double loopEnd = playStyle_.timeLoopEnd_;
	DWORD byteLoopStart = loopStart * bytePerSec;
	DWORD byteLoopEnd = loopEnd * bytePerSec;

	reader->Seek(lastReadPointer_);

	DWORD resStreamPos = timeCurrent_ * bytePerSec;

	/*
	auto _WriteDecodedAcmStream = [&](DWORD writeOff, DWORD writeTargetSize, bool bDiscard, size_t* pWritten) -> bool {
		if (writeTargetSize == 0) return true;
		
		bool decodeRes = _DecodeAcmStream(bDiscard, writeTargetSize, pWritten);

		memcpy((char*)pMem + writeOff, bufDecode_.GetPointer(), writeTargetSize);

		return decodeRes;
	};

	uint64_t pcmCurrent = timeCurrent_ * samplePerSec;
	uint64_t byteCurrent = pcmCurrent * bytePerSample;

	bool bReadLoopEnd = byteCurrent + dwSize > byteLoopEnd && byteLoopEnd > 0;
	bool bReadEof = byteCurrent + dwSize > source->audioSizeTotal_;
	if (bReadLoopEnd || bReadEof) {
		size_t size1 = bReadLoopEnd ? (byteLoopEnd - byteCurrent) : dwSize;

		size_t written = 0;
		bool bFileEnd = _WriteDecodedAcmStream(0, size1, false, &written);

		size_t remain = dwSize - written;
		if (playStyle_.bLoop_) {
			Seek(loopStart);
			bFileEnd = _WriteDecodedAcmStream(written, remain, true, &written);
		}
		else {
			memset((char*)pMem + written, 0, remain);
			_SetStreamOver();
		}
	}
	else {
		size_t written = 0;
		_WriteDecodedAcmStream(0, dwSize, false, &written);
	}
	*/

	DWORD sizeWriteTotal = 0;
	while (sizeWriteTotal < dwSize) {
		DWORD cSize = dwSize - sizeWriteTotal;
		double cTime = (double)cSize / (double)bytePerSec;

		if (timeCurrent_ + cTime > loopEnd && loopEnd > 0) {
			//ループ終端
			double timeOver = timeCurrent_ + cTime - loopEnd;
			double cTime1 = cTime - timeOver;
			DWORD cSize1 = cTime1 * bytePerSec;

			bool bFileEnd = false;
			DWORD size1Write = 0;
			while (size1Write < cSize1) {
				DWORD tSize = cSize1 - size1Write;
				DWORD sw = _DecodeAcmStream((char*)pMem + sizeWriteTotal + size1Write, tSize);
				if (sw == 0) {
					bFileEnd = true;
					break;
				}
				size1Write += sw;
			}

			if (!bFileEnd) {
				sizeWriteTotal += size1Write;
				timeCurrent_ += (double)size1Write / (double)bytePerSec;
			}

			if (playStyle_.bLoop_) {
				Seek(loopStart);
			}
			else {
				_SetStreamOver();
				break;
			}
		}
		else {
			DWORD sizeWrite = _DecodeAcmStream((char*)pMem + sizeWriteTotal, cSize);
			sizeWriteTotal += sizeWrite;
			timeCurrent_ += (double)sizeWrite / (double)bytePerSec;

			if (sizeWrite == 0) {//ファイル終点
				if (playStyle_.bLoop_) {
					Seek(loopStart);
				}
				else {
					_SetStreamOver();
					break;
				}
			}
		}
	}

	lastReadPointer_ = reader->GetFilePointer();
	return resStreamPos;
}
DWORD SoundStreamingPlayerMp3::_DecodeAcmStream(char* pBuffer, DWORD size) {
	SoundSourceDataMp3* source = (SoundSourceDataMp3*)soundSource_.get();
	auto& reader = source->reader_;
	HACMSTREAM hAcmStream = source->hAcmStream_;
	ACMSTREAMHEADER* pAcmHeader = &source->acmStreamHeader_;

	/*
	size_t totalWritten = 0;
	size_t prevBufferSize = bufDecode_.GetSize();
	if (!bDiscard && prevBufferSize > targetSize) {
		size_t excessSize = prevBufferSize - targetSize;
		if (excessSize > 0) {
			//The buffer has excess, unused data from the previous decode operation
			totalWritten = excessSize;

			ByteBuffer tmpBuffer;
			tmpBuffer.SetSize(excessSize);
			memcpy(tmpBuffer.GetPointer(), bufDecode_.GetPointer() + targetSize, excessSize);

			//Move the excess data to the front of the new buffer
			memcpy(bufDecode_.GetPointer(), tmpBuffer.GetPointer(), excessSize);
		}
		memset(bufDecode_.GetPointer(), 0, targetSize - excessSize);
	}
	bufDecode_.SetSize(targetSize);

	bool res = false;
	{
		while (totalWritten < targetSize) {
			size_t read = reader->Read(pAcmHeader->pbSrc, pAcmHeader->cbSrcLength);
			if (read == 0)		//EOF from source
				goto lab_error_exit;

			size_t _remain = targetSize - totalWritten;

			MMRESULT mmRes = acmStreamConvert(hAcmStream, pAcmHeader,
				ACM_STREAMCONVERTF_BLOCKALIGN);
			if (mmRes != 0)		//acmStreamConvert had an error
				goto lab_error_exit;

			size_t decodedSize = pAcmHeader->cbDstLengthUsed;
			size_t copySize = std::min(_remain, decodedSize);

			if (totalWritten + decodedSize >= bufDecode_.GetSize())
				bufDecode_.SetSize(totalWritten + decodedSize);
			memcpy((char*)bufDecode_.GetPointer() + totalWritten, pAcmHeader->pbDst, copySize);

			totalWritten += decodedSize;
		}
		res = true;
	}

lab_error_exit:
	*pWritten = totalWritten;
	return res;
	*/

	DWORD sizeWrite = 0;
	DWORD bufSize = bufDecode_.GetSize();
	if (bufSize > 0) {
		//前回デコード分を書き込み
		DWORD copySize = std::min(size, bufSize);

		memcpy(pBuffer, bufDecode_.GetPointer(), copySize);
		sizeWrite += copySize;
		if (bufSize > copySize) {
			bufDecode_.SetSize(bufSize - copySize);
			return sizeWrite;
		}

		bufDecode_.SetSize(0);
		pBuffer += sizeWrite;
	}

	//デコード
	while (true) {
		DWORD readSize = reader->Read(pAcmHeader->pbSrc, pAcmHeader->cbSrcLength);
		if (readSize == 0) return 0;
		MMRESULT mmRes = acmStreamConvert(hAcmStream, pAcmHeader, ACM_STREAMCONVERTF_BLOCKALIGN);
		if (mmRes != 0) return 0;
		if (pAcmHeader->cbDstLengthUsed > 0) break;
	}

	DWORD sizeDecode = pAcmHeader->cbDstLengthUsed;
	DWORD copySize = std::min(size, sizeDecode);
	memcpy(pBuffer, pAcmHeader->pbDst, copySize);
	if (sizeDecode > copySize) {
		//今回余った分を、次回用にバッファリング
		DWORD newSize = sizeDecode - copySize;
		bufDecode_.SetSize(newSize);
		memcpy(bufDecode_.GetPointer(), pAcmHeader->pbDst + copySize, newSize);
	}
	sizeWrite += copySize;

	return sizeWrite;
}

bool SoundStreamingPlayerMp3::Seek(double time) {
	if (soundSource_ == nullptr) return false;
	SoundSourceDataMp3* source = (SoundSourceDataMp3*)soundSource_.get();
	return Seek((DWORD)(time * source->formatWave_.nSamplesPerSec));
}
bool SoundStreamingPlayerMp3::Seek(DWORD sample) {
	if (soundSource_ == nullptr) return false;
	SoundSourceDataMp3* source = (SoundSourceDataMp3*)soundSource_.get();
	{
		DWORD waveBlockSize = source->acmStreamHeader_.cbDstLength;
		DWORD mp3BlockSize = source->acmStreamHeader_.cbSrcLength;
		DWORD sampleAsBytes = sample * source->formatWave_.nBlockAlign;

		DWORD seekBlockIndex = sampleAsBytes / waveBlockSize;
		{
			Lock lock(lock_);

			DWORD posSeekMp3 = mp3BlockSize * seekBlockIndex;

			source->reader_->Seek(source->posMp3DataStart_ + posSeekMp3);
			lastReadPointer_ = source->reader_->GetFilePointer();

			bufDecode_.SetSize(0);
			timeCurrent_ = posSeekMp3 / (double)source->formatMp3_.wfx.nAvgBytesPerSec;
		}
	}
	return true;
}
