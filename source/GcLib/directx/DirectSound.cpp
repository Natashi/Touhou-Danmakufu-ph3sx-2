#include "source/GcLib/pch.h"

#include "DirectSound.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//DirectSoundManager
**********************************************************/
DirectSoundManager* DirectSoundManager::thisBase_ = nullptr;

DirectSoundManager::DirectSoundManager() {
	ZeroMemory(&dxSoundCaps_, sizeof(DSCAPS));
	pDirectSound_ = nullptr;
	pDirectSoundBuffer_ = nullptr;
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

	ptr_release(pDirectSoundBuffer_);
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
		WrapDX(pDirectSound_->CreateSoundBuffer(&desc, (LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr),
			L"CreateSoundBuffer(primary)");

		WAVEFORMATEX pcmwf;
		ZeroMemory(&pcmwf, sizeof(WAVEFORMATEX));
		pcmwf.wFormatTag = WAVE_FORMAT_PCM;
		pcmwf.nChannels = 2;
		pcmwf.nSamplesPerSec = 44100;
		pcmwf.nBlockAlign = 4;
		pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
		pcmwf.wBitsPerSample = 16;
		WrapDX(pDirectSoundBuffer_->SetFormat(&pcmwf), L"SetFormat");
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

		for (auto itrNameMap = mapPlayer_.begin(); itrNameMap != mapPlayer_.end(); itrNameMap++) {
			std::list<shared_ptr<SoundPlayer>>& listPlayer = itrNameMap->second;
			
			for (auto itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
				SoundPlayer* player = itrPlayer->get();
				if (player == nullptr) continue;
				player->Stop();
			}
		}

		mapPlayer_.clear();
	}
	catch (...) {}
}
shared_ptr<SoundPlayer> DirectSoundManager::GetPlayer(const std::wstring& path, bool bCreateAlways) {
	shared_ptr<SoundPlayer> res;
	try {
		Lock lock(lock_);
		if (bCreateAlways) {
			res = _CreatePlayer(path);
		}
		else {
			res = _GetPlayer(path);
			if (res == nullptr) {
				res = _CreatePlayer(path);
			}
		}
	}
	catch (...) {}

	return res;
}
shared_ptr<SoundPlayer> DirectSoundManager::GetStreamingPlayer(const std::wstring& path) {
	shared_ptr<SoundPlayer> res;
	try {
		Lock lock(lock_);

		res = _GetPlayer(path);
		if (res == nullptr) {
			res = _CreatePlayer(path);
		}
		else {
			//If multiple streaming players are reading from the same FileReader, things will break.
			//Only allow maximum ref_count of 2 (in DirectSoundManager and the parent sound object)
			SoundStreamingPlayer* playerStreaming = dynamic_cast<SoundStreamingPlayer*>(res.get());
			if (playerStreaming != nullptr && res.use_count() > 2)
				res = _CreatePlayer(path);
		}
	}
	catch (...) {}

	return res;
}
shared_ptr<SoundPlayer> DirectSoundManager::_GetPlayer(const std::wstring& path) {
	if (mapPlayer_.find(path) == mapPlayer_.end()) return nullptr;

	size_t pathHash = std::hash<std::wstring>{}(path);

	shared_ptr<SoundPlayer> res = nullptr;

	for (auto itrNameMap = mapPlayer_.begin(); itrNameMap != mapPlayer_.end(); itrNameMap++) {
		std::list<shared_ptr<SoundPlayer>>& listPlayer = itrNameMap->second;
		
		for (auto itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
			SoundPlayer* player = itrPlayer->get();
			if (player == nullptr) continue;
			if (player->bDelete_) continue;
			if (player->GetPathHash() != pathHash) continue;
			res = *itrPlayer;
			break;
		}
	}
	return res;
}
shared_ptr<SoundPlayer> DirectSoundManager::_CreatePlayer(std::wstring path) {
	FileManager* fileManager = FileManager::GetBase();

	shared_ptr<SoundPlayer> res;

	try {
		path = PathProperty::GetUnique(path);
		shared_ptr<FileReader> reader = fileManager->GetFileReader(path);
		if (reader == nullptr || !reader->Open())
			throw gstd::wexception(L"CreateSoundPlayer: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));

		size_t sizeFile = reader->GetFileSize();
		if (sizeFile <= 64) throw gstd::wexception();

		ByteBuffer header;
		header.SetSize(0x100);
		reader->Read(header.GetPointer(), header.GetSize());

		SoundFileFormat format = SoundFileFormat::Unknown;
		if (!memcmp(header.GetPointer(), "RIFF", 4)) {		//WAVE
			format = SoundFileFormat::Wave;

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
		}
		else if (!memcmp(header.GetPointer(), "OggS", 4)) {	//Ogg Vorbis
			format = SoundFileFormat::Ogg;
		}
		else if (!memcmp(header.GetPointer(), "MThd", 4)) {	//midi
			format = SoundFileFormat::Midi;
		}
		else {		//Death sentence
			format = SoundFileFormat::Mp3;
		}

		//Create the sound player object
		switch (format) {
		case SoundFileFormat::Wave:
			if (sizeFile < 1024 * 1024) {
				//The file is small enough (<1MB), just load the entire thing into memory
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
		case SoundFileFormat::Midi:
			//Commit seppuku
			//Commit sudoku
			//Commit suzaku
			//Commit shiragiku
			//Commit UaUP
			break;
		case SoundFileFormat::Mp3:
		case SoundFileFormat::AWave:
			res = std::shared_ptr<SoundStreamingPlayerMp3>(new SoundStreamingPlayerMp3(), SoundPlayer::PtrDelete);
			break;
		}

		bool bSuccess = res != nullptr;
		if (res) {
			//Prepare the file for reading and create a DirectSound buffer
			bSuccess &= res->_CreateBuffer(reader);
			res->Seek(0.0);
		}

		if (bSuccess) {
			res->manager_ = this;
			res->format_ = format;
			res->path_ = path;
			res->pathHash_ = std::hash<std::wstring>{}(path);
			mapPlayer_[path].push_back(res);
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
shared_ptr<SoundInfo> DirectSoundManager::GetSoundInfo(const std::wstring& path) {
	std::wstring name = PathProperty::GetFileName(path);
	auto itrInfo = mapInfo_.find(name);
	if (itrInfo == mapInfo_.end()) return nullptr;
	return itrInfo->second;
}
bool DirectSoundManager::AddSoundInfoFromFile(const std::wstring& path) {
	bool res = false;
	FileManager* fileManager = FileManager::GetBase();
	shared_ptr<FileReader> reader = fileManager->GetFileReader(path);
	if (reader == nullptr || !reader->Open()) {
		Logger::WriteTop(L"AddSoundInfoFromFile: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));
		return false;
	}

	size_t sizeFile = reader->GetFileSize();
	std::vector<char> text;
	text.resize(sizeFile + 1);
	reader->Read(&text[0], sizeFile);
	text[sizeFile] = '\0';

	Scanner scanner(text);
	try {
		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::Type::TK_EOF)
			{
				break;
			}
			else if (tok.GetType() == Token::Type::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"SoundInfo") {
					tok = scanner.Next();
					if (tok.GetType() == Token::Type::TK_NEWLINE) tok = scanner.Next();
					scanner.CheckType(tok, Token::Type::TK_OPENC);

					std::wstring name;
					shared_ptr<SoundInfo> info(new SoundInfo());

					while (scanner.HasNext()) {
						tok = scanner.Next();
						if (tok.GetType() == Token::Type::TK_CLOSEC) break;
						if (tok.GetType() != Token::Type::TK_ID) continue;

						element = tok.GetElement();
						if (element == L"name") {
							scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
							tok = scanner.Next();
							info->name_ = tok.GetString();
						}
						if (element == L"title") {
							scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
							tok = scanner.Next();
							info->title_ = tok.GetString();
						}
						if (element == L"loop_start_time") {
							scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
							tok = scanner.Next();
							info->timeLoopStart_ = tok.GetReal();
						}
						if (element == L"loop_end_time") {
							scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
							tok = scanner.Next();
							info->timeLoopEnd_ = tok.GetReal();
						}
					}

					if (name.size() > 0) {
						mapInfo_[name] = info;
						std::wstring log = StringUtility::Format(L"Audio data: path[%s] title[%s] start[%.3f] end[%.3f]",
							name.c_str(), info->GetTitle().c_str(), info->GetLoopStartTime(), info->GetLoopEndTime());
						Logger::WriteTop(log);
					}

				}
			}
		}

		res = true;
	}
	catch (gstd::wexception e) {
		int line = scanner.GetCurrentLine();
		std::wstring log = StringUtility::Format(L"Data parsing error: path[%s] line[%d] msg[%s]",
			path.c_str(), line, e.what());
		Logger::WriteTop(log);
	}

	return res;
}
std::vector<shared_ptr<SoundInfo>> DirectSoundManager::GetSoundInfoList() {
	std::vector<shared_ptr<SoundInfo>> res;
	for (auto itrNameMap = mapInfo_.begin(); itrNameMap != mapInfo_.end(); itrNameMap++) {
		shared_ptr<SoundInfo> info = itrNameMap->second;
		res.push_back(info);
	}
	return res;
}
void DirectSoundManager::SetFadeDeleteAll() {
	try {
		Lock lock(lock_);

		for (auto itrNameMap = mapPlayer_.begin(); itrNameMap != mapPlayer_.end(); itrNameMap++) {
			std::list<shared_ptr<SoundPlayer>>& listPlayer = itrNameMap->second;

			for (auto itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
				SoundPlayer* player = itrPlayer->get();
				if (player == nullptr) continue;
				player->SetFadeDelete(SoundPlayer::FADE_DEFAULT);
			}
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

	auto& mapPlayer = manager->mapPlayer_;
	for (auto itrNameMap = mapPlayer.begin(); itrNameMap != mapPlayer.end();) {
		std::list<shared_ptr<SoundPlayer>>& listPlayer = itrNameMap->second;

		for (auto itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); ) {
			SoundPlayer* player = itrPlayer->get();
			if (player == nullptr) {
				itrPlayer = listPlayer.erase(itrPlayer);
				continue;
			}

			bool bDelete = player->bDelete_ || (player->bAutoDelete_ && !player->IsPlaying());

			if (bDelete) {
				Logger::WriteTop(StringUtility::Format(L"DirectSound: Released data [%s]", player->GetPath().c_str()));
				player->Stop();
				itrPlayer = listPlayer.erase(itrPlayer);
			}
			else ++itrPlayer;
		}

		if (listPlayer.size() == 0) itrNameMap = mapPlayer.erase(itrNameMap);
		else ++itrNameMap;
	}
}
void DirectSoundManager::SoundManageThread::_Fade() {
	DirectSoundManager* manager = _GetOuter();
	int timeGap = timeCurrent_ - timePrevious_;

	std::map<std::wstring, std::list<shared_ptr<SoundPlayer>>>& mapPlayer = manager->mapPlayer_;
	
	for (auto itrNameMap = mapPlayer.begin(); itrNameMap != mapPlayer.end(); itrNameMap++) {
		std::list<shared_ptr<SoundPlayer>>& listPlayer = itrNameMap->second;
		
		for (auto itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
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
}


/**********************************************************
//SoundInfoPanel
**********************************************************/
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
	int time = timeGetTime();
	if (abs(time - timeLastUpdate_) < timeUpdateInterval_) return;
	timeLastUpdate_ = time;

	//クリティカルセクション内で、ウィンドウにメッセージを送ると
	//ロックする可能性があるので、表示する情報のみをコピーする
	std::list<Info> listInfo;
	{
		Lock lock(soundManager->GetLock());

		auto& mapPlayer = soundManager->mapPlayer_;

		for (auto itrNameMap = mapPlayer.begin(); itrNameMap != mapPlayer.end(); itrNameMap++) {
			const std::wstring& path = itrNameMap->first;
			std::list<shared_ptr<SoundPlayer>>& listPlayer = itrNameMap->second;
			
			for (auto itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
				SoundPlayer* player = itrPlayer->get();
				if (player == nullptr) continue;

				int address = (int)player;
				Info info;
				info.address = address;
				info.path = path;
				info.countRef = itrPlayer->use_count();
				listInfo.push_back(info);
			}
		}
	}

	std::set<std::wstring> setKey;
	for (auto itrInfo = listInfo.begin(); itrInfo != listInfo.end(); itrInfo++) {
		Info& info = *itrInfo;
		int address = info.address;
		const std::wstring& path = info.path;
		int countRef = info.countRef;
		std::wstring key = StringUtility::Format(L"%08x", address);
		int index = wndListView_.GetIndexInColumn(key, ROW_ADDRESS);
		if (index == -1) {
			index = wndListView_.GetRowCount();
			wndListView_.SetText(index, ROW_ADDRESS, key);
		}

		wndListView_.SetText(index, ROW_FILENAME, PathProperty::GetFileName(path));
		wndListView_.SetText(index, ROW_FULLPATH, path);
		wndListView_.SetText(index, ROW_COUNT_REFFRENCE, StringUtility::Format(L"%d", countRef));

		setKey.insert(key);
	}

	for (int iRow = 0; iRow < wndListView_.GetRowCount();) {
		std::wstring key = wndListView_.GetText(iRow, ROW_ADDRESS);
		if (setKey.find(key) != setKey.end())iRow++;
		else wndListView_.DeleteRow(iRow);
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
/**********************************************************
//SoundDivision
**********************************************************/
SoundDivision::SoundDivision() {
	rateVolume_ = 100;
}
SoundDivision::~SoundDivision() {

}

/**********************************************************
//SoundPlayer
**********************************************************/
SoundPlayer::SoundPlayer() {
	pDirectSoundBuffer_ = nullptr;
	reader_ = nullptr;

	pathHash_ = 0;

	ZeroMemory(&formatWave_, sizeof(WAVEFORMATEX));
	bLoop_ = false;
	timeLoopStart_ = 0;
	timeLoopEnd_ = 0;

	bDelete_ = false;
	bFadeDelete_ = false;
	bAutoDelete_ = false;
	rateVolume_ = 100.0;
	rateVolumeFadePerSec_ = 0;
	bPause_ = false;

	audioSizeTotal_ = 0U;

	division_ = nullptr;
	manager_ = nullptr;
}
SoundPlayer::~SoundPlayer() {
	Stop();
	ptr_release(pDirectSoundBuffer_);
}
void SoundPlayer::_SetSoundInfo() {
	shared_ptr<SoundInfo> info = manager_->GetSoundInfo(path_);
	if (info == nullptr) return;
	timeLoopStart_ = info->GetLoopStartTime();
	timeLoopEnd_ = info->GetLoopEndTime();
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
	PlayStyle style;
	return Play(style);
}
bool SoundPlayer::Play(PlayStyle& style) {
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

		double rateDiv = 100.0;
		if (division_)
			rateDiv = division_->GetVolumeRate();
		double rate = rateVolume_ / 100.0 * rateDiv / 100.0;

		//int volume = (int)((double)(DirectSoundManager::SD_VOLUME_MAX - DirectSoundManager::SD_VOLUME_MIN) * rate);
		//pDirectSoundBuffer_->SetVolume(DirectSoundManager::SD_VOLUME_MIN+volume);
		int volume = _GetVolumeAsDirectSoundDecibel(rate);
		pDirectSoundBuffer_->SetVolume(volume);
	}

	return true;
}
bool SoundPlayer::SetPanRate(double ratePan) {
	{
		Lock lock(lock_);
		if (ratePan < -100) ratePan = -100.0;
		else if (ratePan > 100) ratePan = 100.0;

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

//PlayStyle
SoundPlayer::PlayStyle::PlayStyle() {
	bLoop_ = false;
	timeLoopStart_ = 0;
	timeLoopEnd_ = 0;
	timeStart_ = 0;
	bRestart_ = false;
}
SoundPlayer::PlayStyle::~PlayStyle() {

}

/**********************************************************
//SoundStreamingPlayer
**********************************************************/
SoundStreamingPlayer::SoundStreamingPlayer() {
	pDirectSoundNotify_ = nullptr;
	ZeroMemory(hEvent_, sizeof(HANDLE) * 3);
	thread_ = new StreamingThread(this);

	bStreaming_ = true;
	bRequestStop_ = false;

	ZeroMemory(lastStreamCopyPos_, sizeof(size_t) * 2);
	ZeroMemory(bufferPositionAtCopy_, sizeof(DWORD) * 2);
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
			_RequestStop();

		if (bRequestStop_) {
			Stop();
		}

		if (!bStreaming_ || (IsPlaying() && !bRequestStop_)) {
			if (dwSize1 > 0) {	//バッファ前半
				size_t res = _CopyBuffer(pMem1, dwSize1);
				lastStreamCopyPos_[indexCopy] = res;
			}
			if (dwSize2 > 0) {	//バッファ後半
				_CopyBuffer(pMem2, dwSize2);
			}
		}
		else {
			//演奏中でなければ無音を書き込む
			memset(pMem1, 0, dwSize1);
			if (dwSize2 != 0)
				memcpy(pMem2, 0, dwSize2);
		}
		pDirectSoundBuffer_->Unlock(pMem1, dwSize1, pMem2, dwSize2);
	}
}
bool SoundStreamingPlayer::Play(PlayStyle& style) {
	if (IsPlaying()) return true;

	{
		Lock lock(lock_);

		if (bFadeDelete_)
			SetVolumeRate(100);
		bFadeDelete_ = false;

		SetFade(0);

		bLoop_ = style.IsLoopEnable();
		timeLoopStart_ = style.GetLoopStartTime();
		timeLoopEnd_ = style.GetLoopEndTime();
		//		if(timeLoopEnd_ == 0)timeLoopEnd_ = (double)sizeWave_ / (double)formatWave_.nAvgBytesPerSec;
		//		fadeValuePerMillSec_ = 0;

		_SetSoundInfo();

		bRequestStop_ = false;
		if (!bPause_ || !style.IsRestart()) {
			this->Seek(style.GetStartTime());
			pDirectSoundBuffer_->SetCurrentPosition(0);
		}
		style.SetStartTime(0);

		if (bStreaming_) {
			thread_->Start();
			pDirectSoundBuffer_->Play(0, 0, DSBPLAY_LOOPING);//再生開始
		}
		else {
			DWORD dwFlags = 0;
			if (style.IsLoopEnable())
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
	pDirectSoundBuffer_->SetCurrentPosition(0);
	_CopyStream(1);
	//size_t posCopy = lastStreamCopyPos_;
	_CopyStream(0);
	//lastStreamCopyPos_ = posCopy;
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
	
	size_t p0 = lastStreamCopyPos_[0];
	size_t p1 = lastStreamCopyPos_[1];
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

/**********************************************************
//SoundPlayerWave
**********************************************************/
SoundPlayerWave::SoundPlayerWave() {

}
SoundPlayerWave::~SoundPlayerWave() {}
bool SoundPlayerWave::_CreateBuffer(shared_ptr<gstd::FileReader> reader) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	reader_ = reader;
	reader_->SetFilePointerBegin();

	try {
		byte chunk[4];
		uint32_t sizeChunk = 0;
		uint32_t sizeRiff = 0;

		//First, check if we're actually reading a .wav
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "RIFF", 4) != 0) throw false;
		reader_->Read(&sizeRiff, sizeof(uint32_t));
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "WAVE", 4) != 0) throw false;

		bool bReadValidFmtChunk = false;
		uint32_t fmtChunkOffset = 0;
		bool bFoundValidDataChunk = false;
		uint32_t dataChunkOffset = 0;

		//Scan chunks
		while (reader_->Read(&chunk, 4)) {
			reader_->Read(&sizeChunk, sizeof(uint32_t));

			if (!bReadValidFmtChunk && memcmp(chunk, "fmt ", 4) == 0 && sizeChunk >= 0x10) {
				bReadValidFmtChunk = true;
				fmtChunkOffset = reader_->GetFilePointer() - sizeof(uint32_t);
			}
			else if (!bFoundValidDataChunk && memcmp(chunk, "data", 4) == 0) {
				bFoundValidDataChunk = true;
				dataChunkOffset = reader_->GetFilePointer() - sizeof(uint32_t);
			}
			
			reader_->Seek(reader_->GetFilePointer() + sizeChunk);
			if (bReadValidFmtChunk && bFoundValidDataChunk) break;
		}

		if (!bReadValidFmtChunk) throw gstd::wexception("wave format not found");
		if (!bFoundValidDataChunk) throw gstd::wexception("wave data not found");

		reader_->Seek(fmtChunkOffset);
		reader_->Read(&sizeChunk, sizeof(uint32_t));
		reader_->Read(&formatWave_, sizeChunk);

		switch (formatWave_.wFormatTag) {
		case WAVE_FORMAT_UNKNOWN:
		case WAVE_FORMAT_EXTENSIBLE:
		case WAVE_FORMAT_DEVELOPMENT:
			//Unsupported format type
			throw gstd::wexception("unsupported wave format");
		}

		reader_->Seek(dataChunkOffset);
		reader_->Read(&sizeChunk, sizeof(uint32_t));

		//sizeChunk is now the size of the wave data
		audioSizeTotal_ = sizeChunk;

		uint32_t posWaveStart = dataChunkOffset + sizeof(uint32_t);
		uint32_t posWaveEnd = posWaveStart + sizeChunk;

		//Bufferの作製
		DSBUFFERDESC desc;
		ZeroMemory(&desc, sizeof(DSBUFFERDESC));
		desc.dwSize = sizeof(DSBUFFERDESC);
		desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY 
			| DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER;
		desc.dwBufferBytes = sizeChunk;
		desc.lpwfxFormat = &formatWave_;
		HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, 
			(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
		if (FAILED(hrBuffer)) throw gstd::wexception("IDirectSound8::CreateSoundBuffer failure");

		//Bufferへ書き込む
		LPVOID pMem1, pMem2;
		DWORD dwSize1, dwSize2;
		HRESULT hrLock = pDirectSoundBuffer_->Lock(0, sizeChunk, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
		if (hrLock == DSERR_BUFFERLOST) {
			hrLock = pDirectSoundBuffer_->Restore();
			hrLock = pDirectSoundBuffer_->Lock(0, sizeChunk, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
		}
		if (FAILED(hrLock)) throw gstd::wexception("IDirectSoundBuffer8::Lock failure");

		reader_->Seek(posWaveStart);
		reader_->Read(pMem1, dwSize1);
		if (dwSize2 != 0)
			reader_->Read(pMem2, dwSize2);
		hrLock = pDirectSoundBuffer_->Unlock(pMem1, dwSize1, pMem2, dwSize2);
	}
	catch (bool) {
		return false;
	}
	catch (gstd::wexception& e) {
		throw e;
	}

	return true;
}
bool SoundPlayerWave::Play(PlayStyle& style) {
	{
		Lock lock(lock_);
		if (bFadeDelete_)
			SetVolumeRate(100);
		bFadeDelete_ = false;

		SetFade(0);

		DWORD dwFlags = 0;
		if (style.IsLoopEnable())
			dwFlags = DSBPLAY_LOOPING;

		if (!bPause_ || !style.IsRestart()) {
			this->Seek(style.GetStartTime());
		}
		style.SetStartTime(0);

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
	DWORD status = 0;
	pDirectSoundBuffer_->GetStatus(&status);
	bool res = (status & DSBSTATUS_PLAYING) > 0;
	return res;
}
bool SoundPlayerWave::Seek(double time) {
	{
		Lock lock(lock_);
		int pos = time * formatWave_.nAvgBytesPerSec;
		pDirectSoundBuffer_->SetCurrentPosition(pos);
	}
	return true;
}
bool SoundPlayerWave::Seek(int64_t sample) {
	{
		Lock lock(lock_);
		pDirectSoundBuffer_->SetCurrentPosition(sample * formatWave_.nBlockAlign);
	}
	return true;
}
/**********************************************************
//SoundStreamingPlayerWave
**********************************************************/
SoundStreamingPlayerWave::SoundStreamingPlayerWave() {
	posWaveStart_ = 0;
	posWaveEnd_ = 0;
}
bool SoundStreamingPlayerWave::_CreateBuffer(shared_ptr<gstd::FileReader> reader) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	reader_ = reader;
	reader_->SetFilePointerBegin();

	try {
		char chunk[4];
		uint32_t sizeChunk = 0;
		uint32_t sizeRiff = 0;

		//First, check if we're actually reading a .wav
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "RIFF", 4) != 0) throw false;
		reader_->Read(&sizeRiff, sizeof(uint32_t));
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "WAVE", 4) != 0) throw false;

		bool bReadValidFmtChunk = false;
		uint32_t fmtChunkOffset = 0;
		bool bFoundValidDataChunk = false;
		uint32_t dataChunkOffset = 0;

		//Scan chunks
		while (reader_->Read(&chunk, 4)) {
			reader_->Read(&sizeChunk, sizeof(uint32_t));

			if (!bReadValidFmtChunk && memcmp(chunk, "fmt ", 4) == 0 && sizeChunk >= 0x10) {
				bReadValidFmtChunk = true;
				fmtChunkOffset = reader_->GetFilePointer() - sizeof(uint32_t);
			}
			else if (!bFoundValidDataChunk && memcmp(chunk, "data", 4) == 0) {
				bFoundValidDataChunk = true;
				dataChunkOffset = reader_->GetFilePointer() - sizeof(uint32_t);
			}

			reader_->Seek(reader_->GetFilePointer() + sizeChunk);
			if (bReadValidFmtChunk && bFoundValidDataChunk) break;
		}

		if (!bReadValidFmtChunk) throw gstd::wexception("wave format not found");
		if (!bFoundValidDataChunk) throw gstd::wexception("wave data not found");

		reader_->Seek(fmtChunkOffset);
		reader_->Read(&sizeChunk, sizeof(uint32_t));
		reader_->Read(&formatWave_, sizeChunk);

		switch (formatWave_.wFormatTag) {
		case WAVE_FORMAT_UNKNOWN:
		case WAVE_FORMAT_EXTENSIBLE:
		case WAVE_FORMAT_DEVELOPMENT:
			//Unsupported format type
			throw gstd::wexception("unsupported wave format");
		}

		reader_->Seek(dataChunkOffset);
		reader_->Read(&sizeChunk, sizeof(uint32_t));

		//sizeChunk is now the size of the wave data
		audioSizeTotal_ = sizeChunk;

		posWaveStart_ = dataChunkOffset + sizeof(uint32_t);
		posWaveEnd_ = posWaveStart_ + sizeChunk;

		reader_->Seek(posWaveStart_);
	}
	catch (bool) {
		return false;
	}
	catch (gstd::wexception& e) {
		throw e;
	}

	//Bufferの作製
	DSBUFFERDESC desc;
	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY
		| DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
		| DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS;
	desc.dwBufferBytes = 2 * formatWave_.nAvgBytesPerSec;
	desc.lpwfxFormat = &formatWave_;
	HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, 
		(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
	if (FAILED(hrBuffer)) return false;

	//イベント作成
	_CreateSoundEvent(formatWave_);

	return true;
}
size_t SoundStreamingPlayerWave::_CopyBuffer(LPVOID pMem, DWORD dwSize) {
	size_t cSize = dwSize;
	size_t posLoopStart = posWaveStart_ + timeLoopStart_ * formatWave_.nAvgBytesPerSec;
	size_t posLoopEnd = posWaveStart_ + timeLoopEnd_ * formatWave_.nAvgBytesPerSec;
	size_t blockSize = formatWave_.nBlockAlign;

	size_t cPos = reader_->GetFilePointer();
	size_t resStreamPos = cPos;

	if (cPos + cSize > posWaveEnd_) {//ファイル終点
		size_t size1 = cSize + cPos - posWaveEnd_;
		size_t size2 = (dwSize - cSize) / blockSize * blockSize;
		size1 = dwSize - size2;

		reader_->Read(pMem, size2);
		Seek(timeLoopStart_);
		if (bLoop_)
			reader_->Read((char*)pMem + size2, size1);
		else
			_RequestStop();
	}
	else if (cPos + cSize > posLoopEnd && timeLoopEnd_ > 0) {//ループの終端
		size_t sizeOver = cPos + cSize - posLoopEnd; //ループを超えたデータのサイズ
		size_t cSize1 = (cSize - sizeOver) / blockSize * blockSize;
		size_t cSize2 = sizeOver / blockSize * blockSize;

		reader_->Read(pMem, cSize1);
		Seek(timeLoopStart_);
		if (bLoop_)
			reader_->Read((char*)pMem + cSize1, cSize2);
		else
			_RequestStop();
	}
	else {
		reader_->Read(pMem, cSize);
	}

	return resStreamPos;
}
bool SoundStreamingPlayerWave::Seek(double time) {
	{
		Lock lock(lock_);
		size_t blockSize = formatWave_.nBlockAlign;
		size_t pos = time * formatWave_.nAvgBytesPerSec;
		pos = pos / blockSize * blockSize;
		reader_->Seek(posWaveStart_ + pos);
	}
	return true;
}
bool SoundStreamingPlayerWave::Seek(int64_t sample) {
	{
		Lock lock(lock_);
		reader_->Seek(sample * formatWave_.nBlockAlign);
	}
	return true;
}
/**********************************************************
//SoundStreamingPlayerOgg
**********************************************************/
SoundStreamingPlayerOgg::SoundStreamingPlayerOgg() {}
SoundStreamingPlayerOgg::~SoundStreamingPlayerOgg() {
	this->Stop();
	thread_->Join();
	ov_clear(&fileOgg_);
}
bool SoundStreamingPlayerOgg::_CreateBuffer(shared_ptr<gstd::FileReader> reader) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	reader_ = reader;
	reader_->SetFilePointerBegin();

	oggCallBacks_.read_func = SoundStreamingPlayerOgg::_ReadOgg;
	oggCallBacks_.seek_func = SoundStreamingPlayerOgg::_SeekOgg;
	oggCallBacks_.close_func = SoundStreamingPlayerOgg::_CloseOgg;
	oggCallBacks_.tell_func = SoundStreamingPlayerOgg::_TellOgg;
	if (ov_open_callbacks((void*)this, &fileOgg_, nullptr, 0, oggCallBacks_) < 0)
		return false;

	vorbis_info* vi = ov_info(&fileOgg_, -1);
	if (vi == nullptr) {
		ov_clear(&fileOgg_);
		return false;
	}

	formatWave_.cbSize = sizeof(WAVEFORMATEX);
	formatWave_.wFormatTag = WAVE_FORMAT_PCM;
	formatWave_.nChannels = vi->channels;
	formatWave_.nSamplesPerSec = vi->rate;
	formatWave_.nAvgBytesPerSec = vi->rate * vi->channels * 2;
	formatWave_.nBlockAlign = vi->channels * 2;
	formatWave_.wBitsPerSample = 2 * 8;
	formatWave_.cbSize = 0;

	LONG sizeData = (LONG)ceil((double)vi->channels * (double)vi->rate * ov_time_total(&fileOgg_, -1) * 2.0);
	audioSizeTotal_ = sizeData;

	//Bufferの作製
	DWORD sizeBuffer = std::min(2 * formatWave_.nAvgBytesPerSec, (DWORD)sizeData);

	DSBUFFERDESC desc;
	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY 
		| DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
		| DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS;
	desc.dwBufferBytes = sizeBuffer;
	desc.lpwfxFormat = &formatWave_;
	HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, 
		(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
	if (FAILED(hrBuffer)) return false;

	//イベント作成
	_CreateSoundEvent(formatWave_);

	bStreaming_ = sizeBuffer != sizeData;
	if (!bStreaming_) {
		sizeCopy_ = sizeData;
		_CopyStream(0);
	}

	return true;
}
size_t SoundStreamingPlayerOgg::_CopyBuffer(LPVOID pMem, DWORD dwSize) {
	size_t blockSize = formatWave_.nBlockAlign;

	//lastStreamCopyPos_ = (DWORD)(ov_time_tell(&fileOgg_) * formatWave_.nAvgBytesPerSec);
	size_t resStreamPos = ov_pcm_tell(&fileOgg_) * (size_t)formatWave_.nBlockAlign;

	size_t sizeWriteTotal = 0;
	while (sizeWriteTotal < dwSize) {
		double timeCurrent = ov_time_tell(&fileOgg_);
		size_t cSize = dwSize - sizeWriteTotal;
		double cTime = (double)cSize / (double)formatWave_.nAvgBytesPerSec;

		if (timeCurrent + cTime > timeLoopEnd_ && timeLoopEnd_ > 0) {
			//ループ終端
			double timeOver = timeCurrent + cTime - timeLoopEnd_;
			double cTime1 = cTime - timeOver;

			size_t cSize1 = cTime1 * formatWave_.nAvgBytesPerSec;
			cSize1 = cSize1 / blockSize * blockSize;

			bool bFileEnd = false;
			size_t size1Write = 0;
			while (size1Write < cSize1) {
				size_t tSize = cSize1 - size1Write;
				size_t sw = ov_read(&fileOgg_, (char*)pMem + sizeWriteTotal + size1Write, tSize, 0, 2, 1, nullptr);
				if (sw == 0) {
					bFileEnd = true;
					break;
				}
				size1Write += sw;
			}

			if (!bFileEnd) {
				sizeWriteTotal += size1Write;
				timeCurrent += (double)size1Write / (double)formatWave_.nAvgBytesPerSec;
			}

			if (bLoop_) {
				Seek(timeLoopStart_);
			}
			else {
				_RequestStop();
				break;
			}
		}
		else {
			size_t sizeWrite = ov_read(&fileOgg_, (char*)pMem + sizeWriteTotal, cSize, 0, 2, 1, nullptr);
			sizeWriteTotal += sizeWrite;
			timeCurrent += (double)sizeWrite / (double)formatWave_.nAvgBytesPerSec;

			if (sizeWrite == 0) {//ファイル終点
				if (bLoop_) {
					Seek(timeLoopStart_);
				}
				else {
					_RequestStop();
					break;
				}
			}
		}
	}

	return resStreamPos;
}
bool SoundStreamingPlayerOgg::Seek(double time) {
	{
		Lock lock(lock_);
		ov_time_seek(&fileOgg_, time);
	}
	return true;
}
bool SoundStreamingPlayerOgg::Seek(int64_t sample) {
	{
		Lock lock(lock_);
		ov_pcm_seek(&fileOgg_, sample);
	}
	return true;
}

size_t SoundStreamingPlayerOgg::_ReadOgg(void* ptr, size_t size, size_t nmemb, void* source) {
	SoundStreamingPlayerOgg* player = (SoundStreamingPlayerOgg*)source;

	size_t sizeCopy = size * nmemb;
	sizeCopy = player->reader_->Read(ptr, sizeCopy);

	return sizeCopy / size;
}
int SoundStreamingPlayerOgg::_SeekOgg(void* source, ogg_int64_t offset, int whence) {
	//現在の位置情報などをセットするコールバック関数です。
	SoundStreamingPlayerOgg* player = (SoundStreamingPlayerOgg*)source;
	LONG high = (LONG)((offset & 0xFFFFFFFF00000000) >> 32);
	LONG low = (LONG)((offset & 0x00000000FFFFFFFF) >> 0);

	switch (whence) {
	case SEEK_CUR:
	{
		size_t current = player->reader_->GetFilePointer() + low;
		player->reader_->Seek(current);
		break;
	}
	case SEEK_END:
	{
		player->reader_->SetFilePointerEnd();
		break;
	}
	case SEEK_SET:
	{
		player->reader_->Seek(low);
		break;
	}
	}
	return 0;
}
int SoundStreamingPlayerOgg::_CloseOgg(void* source) {
	//ファイルを閉じる処理をする関数です。
	//ここでは何もしていません。
	return 0;
}
long SoundStreamingPlayerOgg::_TellOgg(void* source) {
	//ファイルの現在の位置を返す関数です。
	SoundStreamingPlayerOgg* player = (SoundStreamingPlayerOgg*)source;
	return player->reader_->GetFilePointer();
}

/**********************************************************
//SoundStreamingPlayerMp3
**********************************************************/
SoundStreamingPlayerMp3::SoundStreamingPlayerMp3() {
	hAcmStream_ = nullptr;
	posMp3DataStart_ = 0;
	posMp3DataEnd_ = 0;
}
SoundStreamingPlayerMp3::~SoundStreamingPlayerMp3() {
	if (hAcmStream_) {
		acmStreamUnprepareHeader(hAcmStream_, &acmStreamHeader_, 0);
		acmStreamClose(hAcmStream_, 0);

		delete[] acmStreamHeader_.pbSrc;
		delete[] acmStreamHeader_.pbDst;
	}
}

bool SoundStreamingPlayerMp3::_CreateBuffer(shared_ptr<gstd::FileReader> reader) {
	reader_ = reader;
	reader_->SetFilePointerBegin();

	size_t fileSize = reader->GetFileSize();
	if (fileSize < 64) return false;

	size_t offsetDataStart = 0;
	size_t dataSize = 0;
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

		// 末尾のタグに移動
		byte tag[3];
		reader->Seek(fileSize - 128);
		reader->Read(tag, sizeof(tag));

		// データの位置、サイズを計算
		if (memcmp(tag, "TAG", 3) == 0)
			dataSize = fileSize - 128; // 末尾のタグを省く
		else
			dataSize = fileSize; // ファイル全体がMP3データ
	}

	dataSize -= 4; //mp3ヘッダ
	posMp3DataStart_ = offsetDataStart + 4;
	posMp3DataEnd_ = posMp3DataStart_ + dataSize;

	reader->Seek(offsetDataStart);

	byte headerData[4];
	reader->Read(headerData, sizeof(headerData));

	if (!(headerData[0] == 0xFF && (headerData[1] & 0xF0) == 0xF0))
		return false;

	byte version = (headerData[1] >> 3) & 0x03;
	byte layer = (headerData[1] >> 1) & 0x03;

	// ビットレート
	const short bitRateTable[][16] = {
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

	size_t indexBitrate = 0;
	if (version == 3) {
		indexBitrate = 3 - layer;
	}
	else {
		if (layer == 3) indexBitrate = 3;
		else indexBitrate = 4;
	}
	short bitRate = bitRateTable[indexBitrate][headerData[2] >> 4];

	//サンプリングレート
	const int sampleRateTable[][4] = {
		{ 44100, 48000, 32000, -1 }, // MPEG1
		{ 22050, 24000, 16000, -1 }, // MPEG2
		{ 11025, 12000, 8000, -1} // MPEG2.5
	};
	size_t indexSampleRate = 0;
	switch (version) {
	case 0: indexSampleRate = 2; break;
	case 2: indexSampleRate = 1; break;
	case 3: indexSampleRate = 0; break;
	}
	size_t sampleRate = sampleRateTable[indexSampleRate][(headerData[2] >> 2) & 0x03];

	byte padding = headerData[2] >> 1 & 0x01;
	byte channel = headerData[3] >> 6;

	// サイズ取得
	size_t mp3BlockSize = ((144 * bitRate * 1000) / sampleRate) + padding;

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
	formatMp3_.nCodecDelay = 1393;

	formatWave_.wFormatTag = WAVE_FORMAT_PCM;
	MMRESULT mmResSuggest = acmFormatSuggest(nullptr, &formatMp3_.wfx, &formatWave_, 
		sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG
	);
	if (mmResSuggest != 0)
		return false;

	MMRESULT mmResStreamOpen = acmStreamOpen(&hAcmStream_, nullptr, &formatMp3_.wfx, 
		&formatWave_, nullptr, 0, 0, 0);
	if (mmResStreamOpen != 0)
		return false;

	DWORD waveBlockSize = 0;
	acmStreamSize(hAcmStream_, mp3BlockSize, &waveBlockSize, ACM_STREAMSIZEF_SOURCE);

	acmStreamSize(hAcmStream_, dataSize, &waveDataSize_, ACM_STREAMSIZEF_SOURCE);

	ZeroMemory(&acmStreamHeader_, sizeof(ACMSTREAMHEADER));
	acmStreamHeader_.cbStruct = sizeof(ACMSTREAMHEADER);
	acmStreamHeader_.pbSrc = new BYTE[mp3BlockSize];
	acmStreamHeader_.cbSrcLength = mp3BlockSize;
	acmStreamHeader_.pbDst = new BYTE[waveBlockSize];
	acmStreamHeader_.cbDstLength = waveBlockSize;

	MMRESULT mmResPrepareHeader = acmStreamPrepareHeader(hAcmStream_, &acmStreamHeader_, 0);
	if (mmResPrepareHeader != 0)
		return false;

	//Bufferの作製
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();
	DWORD sizeBuffer = std::min(2 * formatWave_.nAvgBytesPerSec, waveDataSize_);

	audioSizeTotal_ = waveDataSize_;

	DSBUFFERDESC desc;
	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY
		| DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
		| DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS;
	desc.dwBufferBytes = sizeBuffer;
	desc.lpwfxFormat = &formatWave_;
	HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, 
		(LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
	if (FAILED(hrBuffer)) return false;

	//イベント作成
	_CreateSoundEvent(formatWave_);

	bStreaming_ = sizeBuffer != waveDataSize_;
	if (!bStreaming_) {
		sizeCopy_ = waveDataSize_;
		_CopyStream(0);
	}

	reader->Seek(posMp3DataStart_);
	return true;
}
size_t SoundStreamingPlayerMp3::_CopyBuffer(LPVOID pMem, DWORD dwSize) {
	size_t blockSize = formatWave_.nBlockAlign;

	//lastStreamCopyPos_ = (DWORD)(timeCurrent_ * formatWave_.nAvgBytesPerSec);
	size_t resStreamPos = timeCurrent_ * formatWave_.nAvgBytesPerSec;

	size_t sizeWriteTotal = 0;
	while (sizeWriteTotal < dwSize) {
		size_t cSize = dwSize - sizeWriteTotal;
		double cTime = (double)cSize / (double)formatWave_.nAvgBytesPerSec;

		if (timeCurrent_ + cTime > timeLoopEnd_ && timeLoopEnd_ > 0) {
			//ループ終端
			double timeOver = timeCurrent_ + cTime - timeLoopEnd_;
			double cTime1 = cTime - timeOver;
			size_t cSize1 = cTime1 * formatWave_.nAvgBytesPerSec;
			cSize1 = cSize1 / blockSize * blockSize;

			bool bFileEnd = false;
			size_t size1Write = 0;
			while (size1Write < cSize1) {
				size_t tSize = cSize1 - size1Write;
				size_t sw = _ReadAcmStream((char*)pMem + sizeWriteTotal + size1Write, tSize);
				if (sw == 0) {
					bFileEnd = true;
					break;
				}
				size1Write += sw;
			}

			if (!bFileEnd) {
				sizeWriteTotal += size1Write;
				timeCurrent_ += (double)size1Write / (double)formatWave_.nAvgBytesPerSec;
			}

			if (bLoop_) {
				Seek(timeLoopStart_);
			}
			else {
				_RequestStop();
				break;
			}
		}
		else {
			size_t sizeWrite = _ReadAcmStream((char*)pMem + sizeWriteTotal, cSize);
			sizeWriteTotal += sizeWrite;
			timeCurrent_ += (double)sizeWrite / (double)formatWave_.nAvgBytesPerSec;

			if (sizeWrite == 0) {//ファイル終点
				if (bLoop_) {
					Seek(timeLoopStart_);
				}
				else {
					_RequestStop();
					break;
				}
			}
		}
	}

	return resStreamPos;
}
size_t SoundStreamingPlayerMp3::_ReadAcmStream(char* pBuffer, size_t size) {
	size_t sizeWrite = 0;
	if (bufDecode_) {
		//前回デコード分を書き込み
		size_t bufSize = bufDecode_->GetSize();
		size_t copySize = std::min(size, bufSize);

		memcpy(pBuffer, bufDecode_->GetPointer(), copySize);
		sizeWrite += copySize;
		if (bufSize > copySize) {
			size_t newSize = bufSize - copySize;
			gstd::ref_count_ptr<gstd::ByteBuffer> buffer = new gstd::ByteBuffer();
			buffer->SetSize(newSize);
			memcpy(buffer->GetPointer(), bufDecode_->GetPointer() + copySize, newSize);

			bufDecode_ = buffer;
			return sizeWrite;
		}

		bufDecode_ = nullptr;
		pBuffer += sizeWrite;
	}

	//デコード
	while (true) {
		size_t readSize = reader_->Read(acmStreamHeader_.pbSrc, acmStreamHeader_.cbSrcLength);
		if (readSize == 0) return 0;
		MMRESULT mmRes = acmStreamConvert(hAcmStream_, &acmStreamHeader_, ACM_STREAMCONVERTF_BLOCKALIGN);
		if (mmRes != 0) return 0;
		if (acmStreamHeader_.cbDstLengthUsed > 0) break;
	}

	size_t sizeDecode = acmStreamHeader_.cbDstLengthUsed;
	size_t copySize = std::min(size, sizeDecode);
	memcpy(pBuffer, acmStreamHeader_.pbDst, copySize);
	if (sizeDecode > copySize) {
		//今回余った分を、次回用にバッファリング
		size_t newSize = sizeDecode - copySize;
		bufDecode_ = new gstd::ByteBuffer();
		bufDecode_->SetSize(newSize);
		memcpy(bufDecode_->GetPointer(), acmStreamHeader_.pbDst + copySize, newSize);
	}
	sizeWrite += copySize;

	return sizeWrite;
}

bool SoundStreamingPlayerMp3::Seek(double time) {
	double waveBlockSize = acmStreamHeader_.cbDstLength;
	double mp3BlockSize = acmStreamHeader_.cbSrcLength;
	double posSeekWave = time * formatWave_.nAvgBytesPerSec;

	double waveBlockIndex = posSeekWave / waveBlockSize;
	int mp3BlockIndex = (int)floor(waveBlockIndex + 0.5);//(waveBlockIndex * mp3BlockSize / waveBlockSize);
	{
		Lock lock(lock_);
		double posSeekMp3 = mp3BlockSize * mp3BlockIndex;
		reader_->Seek(posMp3DataStart_ + posSeekMp3);

		bufDecode_ = nullptr;
		timeCurrent_ = mp3BlockIndex * mp3BlockSize / formatMp3_.wfx.nAvgBytesPerSec;
	}

	return true;
}
bool SoundStreamingPlayerMp3::Seek(int64_t sample) {
	double waveBlockSize = acmStreamHeader_.cbDstLength;
	double mp3BlockSize = acmStreamHeader_.cbSrcLength;
	double posSeekWave = sample * formatWave_.nBlockAlign;

	double waveBlockIndex = posSeekWave / waveBlockSize;
	int mp3BlockIndex = (int)floor(waveBlockIndex + 0.5);//(waveBlockIndex * mp3BlockSize / waveBlockSize);
	{
		Lock lock(lock_);
		double posSeekMp3 = mp3BlockSize * mp3BlockIndex;
		reader_->Seek(posMp3DataStart_ + posSeekMp3);

		bufDecode_ = nullptr;
		timeCurrent_ = mp3BlockIndex * mp3BlockSize / formatMp3_.wfx.nAvgBytesPerSec;
	}

	return true;
}
