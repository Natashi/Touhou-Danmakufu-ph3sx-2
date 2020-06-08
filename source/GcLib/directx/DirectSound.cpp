#include "source/GcLib/pch.h"

#include "DirectSound.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//DirectSoundManager
**********************************************************/
DirectSoundManager* DirectSoundManager::thisBase_ = nullptr;

DirectSoundManager::DirectSoundManager() {
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
		delete itr->second;

	if (pDirectSoundBuffer_ != nullptr)pDirectSoundBuffer_->Release();
	if (pDirectSound_ != nullptr)pDirectSound_->Release();

	panelInfo_ = nullptr;
	thisBase_ = nullptr;
	Logger::WriteTop("DirectSound: Finalized.");
}
bool DirectSoundManager::Initialize(HWND hWnd) {
	if (thisBase_ != nullptr)return false;

	Logger::WriteTop("DirectSound: Initializing.");

	HRESULT hrSound = DirectSoundCreate8(nullptr, &pDirectSound_, nullptr);
	if (FAILED(hrSound)) {
		Logger::WriteTop("DirectSoundCreate8 failure.");
		return false;  // DirectSound8の作成に失敗
	}

	HRESULT hrCoop = pDirectSound_->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	if (FAILED(hrCoop)) {
		Logger::WriteTop("SetCooperativeLevel failure.");
		return false;
	}

	//PrimaryBuffer初期化
	DSBUFFERDESC desc;
	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
	desc.dwBufferBytes = 0;
	desc.lpwfxFormat = nullptr;
	HRESULT hrBuf = pDirectSound_->CreateSoundBuffer(&desc, (LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
	if (FAILED(hrBuf)) {
		Logger::WriteTop("CreateSoundBuffer failure.");
		return false;
	}

	WAVEFORMATEX pcmwf;
	ZeroMemory(&pcmwf, sizeof(PCMWAVEFORMAT));
	pcmwf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.nChannels = 2;
	pcmwf.nSamplesPerSec = 44100;
	pcmwf.nBlockAlign = 4;
	pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec*pcmwf.nBlockAlign;
	pcmwf.wBitsPerSample = 16;
	HRESULT hrFormat = pDirectSoundBuffer_->SetFormat(&pcmwf);
	if (FAILED(hrFormat)) {
		Logger::WriteTop("SetFormat failure.");
		return false;
	}

	//管理スレッド開始
	threadManage_ = new SoundManageThread(this);
	threadManage_->Start();

	Logger::WriteTop("DirectSound: Initialized.");

	thisBase_ = this;
	return true;
}
void DirectSoundManager::Clear() {
	try {
		Lock lock(lock_);
		std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >::iterator itrNameMap;
		for (itrNameMap = mapPlayer_.begin(); itrNameMap != mapPlayer_.end(); itrNameMap++) {
			std::list<gstd::ref_count_ptr<SoundPlayer> > &listPlayer = itrNameMap->second;
			std::list<gstd::ref_count_ptr<SoundPlayer> >::iterator itrPlayer;
			for (itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
				SoundPlayer* player = (*itrPlayer).GetPointer();
				if (player == nullptr)continue;
				player->Stop();
			}
		}
		mapPlayer_.clear();
	}
	catch (...) {}
}
gstd::ref_count_ptr<SoundPlayer> DirectSoundManager::GetPlayer(std::wstring path, bool bCreateAlways) {
	gstd::ref_count_ptr<SoundPlayer> res;
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
gstd::ref_count_ptr<SoundPlayer> DirectSoundManager::_GetPlayer(std::wstring path) {
	if (mapPlayer_.find(path) == mapPlayer_.end())return nullptr;

	gstd::ref_count_ptr<SoundPlayer> res = nullptr;

	std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >::iterator itrNameMap;
	for (itrNameMap = mapPlayer_.begin(); itrNameMap != mapPlayer_.end(); itrNameMap++) {
		std::list<gstd::ref_count_ptr<SoundPlayer> > &listPlayer = itrNameMap->second;
		std::list<gstd::ref_count_ptr<SoundPlayer> >::iterator itrPlayer;
		for (itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
			SoundPlayer* player = (*itrPlayer).GetPointer();
			if (player == nullptr)continue;
			if (player->bDelete_)continue;
			if (player->GetPath() != path)continue;
			res = *itrPlayer;
			break;
		}
	}
	return res;
}
gstd::ref_count_ptr<SoundPlayer> DirectSoundManager::_CreatePlayer(std::wstring path) {
	gstd::ref_count_ptr<SoundPlayer> res;
	FileManager* fileManager = FileManager::GetBase();
	try {
		path = PathProperty::GetUnique(path);
		ref_count_ptr<FileReader> reader = fileManager->GetFileReader(path);
		if (reader == nullptr)throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path).c_str());
		if (!reader->Open())throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path).c_str());

		//フォーマット確認
		size_t sizeFile = reader->GetFileSize();
		FileFormat format = SD_UNKNOWN;
		if (sizeFile <= 64)throw gstd::wexception();

		ByteBuffer header;
		header.SetSize(64);
		reader->Read(header.GetPointer(), header.GetSize());

		if (!memcmp(header.GetPointer(), "RIFF", 4)) {//WAVE
			format = SD_WAVE;
			WAVEFORMATEX pcmwf;
			ZeroMemory(&pcmwf, sizeof(PCMWAVEFORMAT));
			memcpy(&pcmwf, header.GetPointer(20), sizeof(WAVEFORMATEX));
			if (pcmwf.wFormatTag == 85)format = SD_AWAVE;//waveヘッダmp3
			else format = SD_WAVE;
		}
		else if (!memcmp(header.GetPointer(), "OggS", 4)) {//OggVorbis
			format = SD_OGG;
		}
		else if (!memcmp(header.GetPointer(), "MThd", 4)) {//midi
			format = SD_MIDI;
		}
		else {//多分mp3
			format = SD_MP3;
		}

		//プレイヤ作成
		if (format == SD_WAVE) {//WAVE
			if (sizeFile < 1024 * 1024) {
				//メモリ保持再生
				res = new SoundPlayerWave();
			}
			else {
				//ストリーミング
				res = new SoundStreamingPlayerWave();
			}
		}
		else if (format == SD_OGG) {//OggVorbis
			res = new SoundStreamingPlayerOgg();
		}
		else if (format == SD_MIDI) {//midi
		}
		else if (format == SD_MP3 || format == SD_AWAVE) {//mp3
			res = new SoundStreamingPlayerMp3();
		}

		bool bSuccess = true;
		bSuccess &= res != nullptr;

		if (res != nullptr) {
			//ファイルを読み込みサウンドバッファを作成
			bSuccess &= res->_CreateBuffer(reader);
			res->Seek(0.0);
		}

		if (bSuccess) {
			res->manager_ = this;
			res->path_ = path;
			mapPlayer_[path].push_back(res);
			std::wstring str = StringUtility::Format(L"DirectSound: Audio loaded [%s]", path.c_str());
			Logger::WriteTop(str);
		}
		else {
			std::wstring str = StringUtility::Format(L"DirectSound: Audio load failed [%s]", path.c_str());
			Logger::WriteTop(str);
		}
	}
	catch (gstd::wexception &e) {
		std::wstring str = StringUtility::Format(L"DirectSound：Audio load failed [%s]\n\t%s", path.c_str(), e.what());
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
gstd::ref_count_ptr<SoundInfo> DirectSoundManager::GetSoundInfo(std::wstring path) {
	std::wstring name = PathProperty::GetFileName(path);
	auto itrInfo = mapInfo_.find(name);
	if (itrInfo == mapInfo_.end()) return nullptr;
	return itrInfo->second;
}
bool DirectSoundManager::AddSoundInfoFromFile(std::wstring path) {
	bool res = false;
	FileManager* fileManager = FileManager::GetBase();
	ref_count_ptr<FileReader> reader = fileManager->GetFileReader(path);
	if (reader == nullptr || !reader->Open()) {
		Logger::WriteTop(ErrorUtility::GetFileNotFoundErrorMessage(path));
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
			if (tok.GetType() == Token::TK_EOF)//Eofの識別子が来たらファイルの調査終了
			{
				break;
			}
			else if (tok.GetType() == Token::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"SoundInfo") {
					tok = scanner.Next();
					if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
					scanner.CheckType(tok, Token::TK_OPENC);

					std::wstring name;
					ref_count_ptr<SoundInfo> info = new SoundInfo();
					while (scanner.HasNext()) {
						tok = scanner.Next();
						if (tok.GetType() == Token::TK_CLOSEC)break;
						if (tok.GetType() != Token::TK_ID)continue;

						element = tok.GetElement();
						if (element == L"name") {
							scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
							tok = scanner.Next();
							info->name_ = tok.GetString();
						}
						if (element == L"title") {
							scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
							tok = scanner.Next();
							info->title_ = tok.GetString();
						}
						if (element == L"loop_start_time") {
							scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
							tok = scanner.Next();
							info->timeLoopStart_ = tok.GetReal();
						}
						if (element == L"loop_end_time") {
							scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
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
std::vector<gstd::ref_count_ptr<SoundInfo> > DirectSoundManager::GetSoundInfoList() {
	std::vector<gstd::ref_count_ptr<SoundInfo> > res;
	std::map<std::wstring, ref_count_ptr<SoundInfo> >::iterator itrNameMap;
	for (itrNameMap = mapInfo_.begin(); itrNameMap != mapInfo_.end(); itrNameMap++) {
		gstd::ref_count_ptr<SoundInfo> info = itrNameMap->second;
		res.push_back(info);
	}
	return res;
}
void DirectSoundManager::SetFadeDeleteAll() {
	try {
		Lock lock(lock_);
		std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >::iterator itrNameMap;
		for (itrNameMap = mapPlayer_.begin(); itrNameMap != mapPlayer_.end(); itrNameMap++) {
			std::list<gstd::ref_count_ptr<SoundPlayer> > &listPlayer = itrNameMap->second;
			std::list<gstd::ref_count_ptr<SoundPlayer> >::iterator itrPlayer;
			for (itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
				SoundPlayer* player = (*itrPlayer).GetPointer();
				if (player == nullptr)continue;
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

		if (manager->panelInfo_ != nullptr &&
			this->GetStatus() == RUN) {
			//クリティカルセクションの中で更新すると
			//メインスレッドとロックする可能性があります
			manager->panelInfo_->Update(manager);
		}
		timePrevious_ = timeCurrent_;
		::Sleep(100);
	}
}
void DirectSoundManager::SoundManageThread::_Arrange() {
	DirectSoundManager* manager = _GetOuter();
	std::set<std::wstring> setRemoveKey;
	std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >& mapPlayer = manager->mapPlayer_;
	std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >::iterator itrNameMap;
	for (itrNameMap = mapPlayer.begin(); itrNameMap != mapPlayer.end(); itrNameMap++) {
		std::list<gstd::ref_count_ptr<SoundPlayer> > &listPlayer = itrNameMap->second;
		std::list<gstd::ref_count_ptr<SoundPlayer> >::iterator itrPlayer;
		for (itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); ) {
			SoundPlayer* player = (*itrPlayer).GetPointer();
			bool bDelete = false;
			if (player != nullptr) {
				bDelete |= player->bDelete_;
				bDelete |= player->bAutoDelete_ && !player->IsPlaying();
			}

			if (bDelete) {
				Logger::WriteTop(StringUtility::Format(L"DirectSound: Released data [%s]", player->GetPath().c_str()));
				player->Stop();
				itrPlayer = listPlayer.erase(itrPlayer);
			}
			else
				itrPlayer++;
		}

		if (listPlayer.size() == 0)
			setRemoveKey.insert(itrNameMap->first);
	}

	std::set<std::wstring>::iterator itrRemove;
	for (itrRemove = setRemoveKey.begin(); itrRemove != setRemoveKey.end(); itrRemove++) {
		std::wstring key = *itrRemove;
		mapPlayer.erase(key);
	}
}
void DirectSoundManager::SoundManageThread::_Fade() {
	DirectSoundManager* manager = _GetOuter();
	int timeGap = timeCurrent_ - timePrevious_;
	std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >& mapPlayer = manager->mapPlayer_;

	std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >::iterator itrNameMap;
	for (itrNameMap = mapPlayer.begin(); itrNameMap != mapPlayer.end(); itrNameMap++) {
		std::list<gstd::ref_count_ptr<SoundPlayer> > &listPlayer = itrNameMap->second;
		std::list<gstd::ref_count_ptr<SoundPlayer> >::iterator itrPlayer;
		for (itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
			SoundPlayer* player = (*itrPlayer).GetPointer();
			if (player == nullptr)continue;
			double rateFade = player->GetFadeVolumeRate();
			if (rateFade == 0)continue;

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
	if (!IsWindowVisible())return;
	int time = timeGetTime();
	if (abs(time - timeLastUpdate_) < timeUpdateInterval_)return;
	timeLastUpdate_ = time;

	//クリティカルセクション内で、ウィンドウにメッセージを送ると
	//ロックする可能性があるので、表示する情報のみをコピーする
	std::list<Info> listInfo;
	{
		Lock lock(soundManager->GetLock());
		std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >& mapPlayer = soundManager->mapPlayer_;
		std::map<std::wstring, std::list<gstd::ref_count_ptr<SoundPlayer> > >::iterator itrNameMap;
		for (itrNameMap = mapPlayer.begin(); itrNameMap != mapPlayer.end(); itrNameMap++) {
			std::wstring path = itrNameMap->first;
			std::list<gstd::ref_count_ptr<SoundPlayer> > &listPlayer = itrNameMap->second;
			std::list<gstd::ref_count_ptr<SoundPlayer> >::iterator itrPlayer;
			for (itrPlayer = listPlayer.begin(); itrPlayer != listPlayer.end(); itrPlayer++) {
				SoundPlayer* player = (*itrPlayer).GetPointer();
				if (player == nullptr)continue;

				int address = (int)player;
				Info info;
				info.address = address;
				info.path = path;
				info.countRef = (*itrPlayer).GetReferenceCount();
				listInfo.push_back(info);
			}
		}
	}

	std::set<std::wstring> setKey;
	std::list<Info>::iterator itrInfo;
	for (itrInfo = listInfo.begin(); itrInfo != listInfo.end(); itrInfo++) {
		Info& info = *itrInfo;
		int address = info.address;
		std::wstring path = info.path;
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

	flgUpdateStreamOffset_ = true;
	lastStreamCopyPos_ = 0U;
	audioSizeTotal_ = 0U;

	division_ = nullptr;
	manager_ = nullptr;
}
SoundPlayer::~SoundPlayer() {
	Stop();
	if (pDirectSoundBuffer_ != nullptr)pDirectSoundBuffer_->Release();
}
void SoundPlayer::_SetSoundInfo() {
	gstd::ref_count_ptr<SoundInfo> info = manager_->GetSoundInfo(path_);
	if (info == nullptr)return;
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
		if (div != nullptr)SetSoundDivision(div);
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
		if (rateVolume < 0)rateVolume = 0.0;
		if (rateVolume > 100)rateVolume = 100.0;
		rateVolume_ = rateVolume;

		double rateDiv = 100.0;
		if (division_ != nullptr)
			rateDiv = division_->GetVolumeRate();
		double rate = rateVolume_ / 100.0 * rateDiv / 100.0;

		//int volume = (int)((double)(DirectSoundManager::SD_VOLUME_MAX - DirectSoundManager::SD_VOLUME_MIN) * rate);
		//pDirectSoundBuffer_->SetVolume(DirectSoundManager::SD_VOLUME_MIN+volume);
		int volume = _GetValumeAsDirectSoundDecibel(rate);
		pDirectSoundBuffer_->SetVolume(volume);
	}

	return true;
}
bool SoundPlayer::SetPanRate(double ratePan) {
	{
		Lock lock(lock_);
		if (ratePan < -100)ratePan = -100.0;
		if (ratePan > 100)ratePan = 100.0;

		double rateDiv = 100.0;
		if (division_ != nullptr)
			rateDiv = division_->GetVolumeRate();
		double rate = rateVolume_ / 100.0 * rateDiv / 100.0;
		int volume = (int)((double)(DirectSoundManager::SD_VOLUME_MAX - DirectSoundManager::SD_VOLUME_MIN) * rate);
		//int volume = _GetValumeAsDirectSoundDecibel(rate);

		double span = (DSBPAN_RIGHT - DSBPAN_LEFT) / 2;
		span = volume / 2;
		double pan = span * ratePan / 100;
		HRESULT hr = pDirectSoundBuffer_->SetPan((int)pan);
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
int SoundPlayer::_GetValumeAsDirectSoundDecibel(float rate) {
	int result = 0;
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
		result = (int)(33.2f * log10(rate) * 100);
	}
	return result;
}
DWORD SoundPlayer::GetCurrentPosition() {
	/*
	DWORD res = 0;
	if (pDirectSoundBuffer_ != nullptr) {
		HRESULT hr = pDirectSoundBuffer_->GetCurrentPosition(&res, nullptr);
		if (FAILED(hr)) res = 0;
	}
	*/
	return (lastStreamCopyPos_);
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
}
SoundStreamingPlayer::~SoundStreamingPlayer() {
	this->Stop();
	for (size_t iEvent = 0; iEvent < 3; iEvent++) {
		::CloseHandle(hEvent_[iEvent]);
	}
	if (pDirectSoundNotify_ != nullptr)pDirectSoundNotify_->Release();
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
				lastStreamCopyPos_ = res;
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

		flgUpdateStreamOffset_ = true;
	}
}
bool SoundStreamingPlayer::Play(PlayStyle& style) {
	if (IsPlaying())return true;

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
		if (!(bPause_ && style.IsRestart())) {
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

		if (pDirectSoundBuffer_ != nullptr)
			pDirectSoundBuffer_->Stop();

		thread_->Stop();
	}
	return true;
}
void SoundStreamingPlayer::ResetStreamForSeek() {
	pDirectSoundBuffer_->SetCurrentPosition(0);
	_CopyStream(1);
	size_t posCopy = lastStreamCopyPos_;
	_CopyStream(0);
	lastStreamCopyPos_ = posCopy;
}
bool SoundStreamingPlayer::IsPlaying() {
	return thread_->GetStatus() == Thread::RUN;
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
bool SoundPlayerWave::_CreateBuffer(gstd::ref_count_ptr<gstd::FileReader> reader) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	reader_ = reader;
	reader_->SetFilePointerBegin();

	try {
		//wave解析
		char chunk[5] = "";
		int sizeRiff = 0;
		int sizeFormat = 0;
		DWORD sizeWave = 0;

		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "RIFF", 4) != 0)throw(false);
		reader_->Read(&sizeRiff, sizeof(long));
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "WAVE", 4) != 0)throw(false);
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "fmt ", 4) != 0)throw(false);
		reader_->Read(&sizeFormat, sizeof(long));
		sizeFormat = sizeof(WAVEFORMATEX) - sizeof(WORD);
		reader_->Read(&formatWave_, sizeFormat);
		while (true) {
			char ex = 0;
			if (reader_->Read(&ex, sizeof(char)) == 0)throw(false);
			for (int iChar = 0; iChar < 3; iChar++)
				chunk[iChar] = chunk[iChar + 1];
			chunk[3] = ex;

			//if(reader_->Read(&chunk, 4)==0)throw(false);
			if (memcmp(chunk, "data", 4) == 0)break;
		}
		reader_->Read(&sizeWave, sizeof(int));

		audioSizeTotal_ = sizeWave;

		int posWaveStart = reader_->GetFilePointer();
		int posWaveEnd = posWaveStart + sizeWave;

		//Bufferの作製
		DSBUFFERDESC desc;
		ZeroMemory(&desc, sizeof(DSBUFFERDESC));
		desc.dwSize = sizeof(DSBUFFERDESC);
		desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER;
		desc.dwBufferBytes = sizeWave;
		desc.lpwfxFormat = &formatWave_;
		HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, (LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
		if (FAILED(hrBuffer))throw(false);

		//Bufferへ書き込む
		LPVOID pMem1, pMem2;
		DWORD dwSize1, dwSize2;
		HRESULT hrLock = pDirectSoundBuffer_->Lock(0, sizeWave, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
		if (hrLock == DSERR_BUFFERLOST) {
			hrLock = pDirectSoundBuffer_->Restore();
			hrLock = pDirectSoundBuffer_->Lock(0, sizeWave, &pMem1, &dwSize1, &pMem2, &dwSize2, 0);
		}
		if (FAILED(hrLock))throw(false);

		reader_->Seek(posWaveStart);
		reader_->Read(pMem1, dwSize1);
		if (dwSize2 != 0)
			reader_->Read(pMem2, dwSize2);
		hrLock = pDirectSoundBuffer_->Unlock(pMem1, dwSize1, pMem2, dwSize2);
	}
	catch (...) {
		return false;
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

		if (!(bPause_ && style.IsRestart())) {
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

		if (pDirectSoundBuffer_ != nullptr)
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
bool SoundStreamingPlayerWave::_CreateBuffer(gstd::ref_count_ptr<gstd::FileReader> reader) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	reader_ = reader;
	reader_->SetFilePointerBegin();

	try {
		//wave解析
		char chunk[5] = "";
		int sizeRiff = 0;
		int sizeFormat = 0;
		int sizeWave = 0;

		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "RIFF", 4) != 0)throw(false);
		reader_->Read(&sizeRiff, sizeof(long));
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "WAVE", 4) != 0)throw(false);
		reader_->Read(&chunk, 4);
		if (memcmp(chunk, "fmt ", 4) != 0)throw(false);
		reader_->Read(&sizeFormat, sizeof(long));
		sizeFormat = sizeof(WAVEFORMATEX) - sizeof(WORD);
		reader_->Read(&formatWave_, sizeFormat);
		while (true) {
			char ex = 0;
			if (reader_->Read(&ex, sizeof(char)) == 0)throw(false);
			for (int iChar = 0; iChar < 3; iChar++)
				chunk[iChar] = chunk[iChar + 1];
			chunk[3] = ex;

			//if(reader_->Read(&chunk, 4)==0)throw(false);
			if (memcmp(chunk, "data", 4) == 0)break;
		}
		reader_->Read(&sizeWave, sizeof(int));

		audioSizeTotal_ = sizeWave;

		posWaveStart_ = reader_->GetFilePointer();
		posWaveEnd_ = posWaveStart_ + sizeWave;
		reader_->Seek(posWaveStart_);
	}
	catch (...) {
		return false;
	}

	//Bufferの作製
	DSBUFFERDESC desc;
	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_LOCSOFTWARE;
	desc.dwBufferBytes = 2 * formatWave_.nAvgBytesPerSec;
	desc.lpwfxFormat = &formatWave_;
	HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, (LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
	if (FAILED(hrBuffer))return false;

	//イベント作成
	_CreateSoundEvent(formatWave_);

	return true;
}
size_t SoundStreamingPlayerWave::_CopyBuffer(LPVOID pMem, DWORD dwSize) {
	int cSize = dwSize;
	int posLoopStart = posWaveStart_ + timeLoopStart_ * formatWave_.nAvgBytesPerSec;
	int posLoopEnd = posWaveStart_ + timeLoopEnd_ * formatWave_.nAvgBytesPerSec;
	int blockSize = formatWave_.nBlockAlign;

	int cPos = reader_->GetFilePointer();
	size_t resStreamPos = cPos;

	if (cPos + cSize > posWaveEnd_) {//ファイル終点
		int size1 = cSize + cPos - posWaveEnd_;
		int size2 = (dwSize - cSize) / blockSize * blockSize;
		size1 = dwSize - size2;

		reader_->Read(pMem, size2);
		Seek(timeLoopStart_);
		if (bLoop_)
			reader_->Read((char*)pMem + size2, size1);
		else
			_RequestStop();
	}
	else if (cPos + cSize > posLoopEnd && timeLoopEnd_ > 0) {//ループの終端
		int sizeOver = cPos + cSize - posLoopEnd; //ループを超えたデータのサイズ
		int cSize1 = (cSize - sizeOver) / blockSize * blockSize;
		int cSize2 = sizeOver / blockSize * blockSize;

		reader_->Read(pMem, cSize1);
		Seek(timeLoopStart_);
		if (bLoop_)
			reader_->Read((char*)pMem + cSize1, cSize2);
		else
			_RequestStop();
	}
	else reader_->Read(pMem, cSize);

	return resStreamPos;
}
bool SoundStreamingPlayerWave::Seek(double time) {
	{
		Lock lock(lock_);
		int blockSize = formatWave_.nBlockAlign;
		int pos = time * formatWave_.nAvgBytesPerSec;
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
bool SoundStreamingPlayerOgg::_CreateBuffer(gstd::ref_count_ptr<gstd::FileReader> reader) {
	FileManager* fileManager = FileManager::GetBase();
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();

	reader_ = reader;
	reader_->SetFilePointerBegin();

	//oggVorbis使用準備
	oggCallBacks_.read_func = SoundStreamingPlayerOgg::_ReadOgg;
	oggCallBacks_.seek_func = SoundStreamingPlayerOgg::_SeekOgg;
	oggCallBacks_.close_func = SoundStreamingPlayerOgg::_CloseOgg;
	oggCallBacks_.tell_func = SoundStreamingPlayerOgg::_TellOgg;
	ov_open_callbacks((void*)this, &fileOgg_, nullptr, 0, oggCallBacks_);
	vorbis_info* vi = ov_info(&fileOgg_, -1);
	if (vi == nullptr) {
		ov_clear(&fileOgg_);
		return false;
	}

	//WAVEHEADERの大きさを取得
	//ただしsizeof演算子だと正確な大きさが測れません。
	WAVEFILEHEADER wfh;
	LONG sizeWaveHeader = sizeof(wfh.cRIFF) + sizeof(wfh.iSizeRIFF) + sizeof(wfh.cType) +
		sizeof(wfh.cFmt) + sizeof(wfh.iSizeFmt) + sizeof(wfh.WaveFmt) +
		sizeof(wfh.cData) + sizeof(wfh.iSizeData);
	LONG sizeData = (LONG)ceil((double)vi->channels * (double)vi->rate * ov_time_total(&fileOgg_, -1) * 2.0);

	audioSizeTotal_ = ov_pcm_total(&fileOgg_, -1);

	//WAVEHEADERの中身を定義
	memcpy(wfh.cRIFF, "RIFF", 4);
	wfh.iSizeRIFF = sizeWaveHeader + sizeData - 8;
	memcpy(wfh.cType, "WAVE", 4);
	memcpy(wfh.cFmt, "fmt ", 4);
	wfh.iSizeFmt = sizeof(WAVEFORMATEX);
	wfh.WaveFmt.cbSize = sizeof(WAVEFORMATEX);
	wfh.WaveFmt.wFormatTag = WAVE_FORMAT_PCM;
	wfh.WaveFmt.nChannels = vi->channels;
	wfh.WaveFmt.nSamplesPerSec = vi->rate;
	wfh.WaveFmt.nAvgBytesPerSec = vi->rate * vi->channels * 2;
	wfh.WaveFmt.nBlockAlign = vi->channels * 2;
	wfh.WaveFmt.wBitsPerSample = 2 * 8;
	memcpy(wfh.cData, "data", 4);
	wfh.iSizeData = sizeData;

	this->formatWave_ = wfh.WaveFmt;

	//Bufferの作製
	DWORD sizeBuffer = std::min(2 * formatWave_.nAvgBytesPerSec, (DWORD)sizeData);

	DSBUFFERDESC desc;
	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_LOCSOFTWARE;
	desc.dwBufferBytes = sizeBuffer;
	desc.lpwfxFormat = &formatWave_;
	HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, (LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
	if (FAILED(hrBuffer))return false;

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
	int blockSize = formatWave_.nBlockAlign;

	//lastStreamCopyPos_ = (DWORD)(ov_time_tell(&fileOgg_) * formatWave_.nAvgBytesPerSec);
	size_t resStreamPos = ov_pcm_tell(&fileOgg_) * (size_t)formatWave_.nBlockAlign;

	int sizeWriteTotal = 0;
	while (sizeWriteTotal < dwSize) {
		double timeCurrent = ov_time_tell(&fileOgg_);
		int cSize = dwSize - sizeWriteTotal;
		double cTime = (double)cSize / (double)formatWave_.nAvgBytesPerSec;

		if (timeCurrent + cTime > timeLoopEnd_ && timeLoopEnd_ > 0) {
			//ループ終端
			double timeOver = timeCurrent + cTime - timeLoopEnd_;
			double cTime1 = cTime - timeOver;
			int cSize1 = cTime1 * formatWave_.nAvgBytesPerSec;
			cSize1 = cSize1 / blockSize * blockSize;

			bool bFileEnd = false;
			int size1Write = 0;
			while (size1Write < cSize1) {
				int tSize = cSize1 - size1Write;
				int sw = ov_read(&fileOgg_, (char*)pMem + sizeWriteTotal + size1Write, tSize, 0, 2, 1, nullptr);
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
			int sizeWrite = ov_read(&fileOgg_, (char*)pMem + sizeWriteTotal, cSize, 0, 2, 1, nullptr);
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
		int current = player->reader_->GetFilePointer() + low;
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
	if (hAcmStream_ != nullptr) {
		acmStreamUnprepareHeader(hAcmStream_, &acmStreamHeader_, 0);
		acmStreamClose(hAcmStream_, 0);

		delete[] acmStreamHeader_.pbSrc;
		delete[] acmStreamHeader_.pbDst;
	}
}

bool SoundStreamingPlayerMp3::_CreateBuffer(gstd::ref_count_ptr<gstd::FileReader> reader) {
	reader_ = reader;
	reader_->SetFilePointerBegin();

	size_t fileSize = reader->GetFileSize();
	if (fileSize < 64)return false;

	int offsetDataStart = 0;
	int dataSize = 0;
	char headerFile[10];
	reader->Read(headerFile, sizeof(headerFile));
	if (memcmp(headerFile, "ID3", 3) == 0) {
		// タグサイズを取得
		DWORD tagSize =
			((headerFile[6] << 21) |
			(headerFile[7] << 14) |
				(headerFile[8] << 7) |
				(headerFile[9])) + 10;

		// データの位置、サイズを計算
		offsetDataStart = tagSize;
		dataSize = fileSize - offsetDataStart;
	}
	else {
		offsetDataStart = 0;

		// 末尾のタグに移動
		char tag[3];
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
	unsigned char headerData[4];
	reader->Read(headerData, sizeof(headerData));

	if (!(headerData[0] == 0xFF && (headerData[1] & 0xF0) == 0xF0))
		return false;

	unsigned char version = (headerData[1] >> 3) & 0x03;
	unsigned char layer = (headerData[1] >> 1) & 0x03;

	// ビットレート
	const short bitRateTable[][16] = {
		// MPEG1, Layer1
		{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,-1},
		// MPEG1, Layer2
		{0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,-1},
		// MPEG1, Layer3
		{0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,-1},
		// MPEG2/2.5, Layer1,2
		{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,-1},
		// MPEG2/2.5, Layer3
		{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,-1}
	};

	int indexBitrate = 0;
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
	int indexSampleRate = 0;
	switch (version) {
	case 0: indexSampleRate = 2; break;
	case 2: indexSampleRate = 1; break;
	case 3: indexSampleRate = 0; break;
	}
	int sampleRate = sampleRateTable[indexSampleRate][(headerData[2] >> 2) & 0x03];

	unsigned char padding = headerData[2] >> 1 & 0x01;
	unsigned char channel = headerData[3] >> 6;

	// サイズ取得
	int mp3BlockSize = ((144 * bitRate * 1000) / sampleRate) + padding;

	formatMp3_.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
	formatMp3_.wfx.nChannels = channel == 3 ? 1 : 2;
	formatMp3_.wfx.nSamplesPerSec = sampleRate;
	formatMp3_.wfx.nAvgBytesPerSec = (bitRate * 1000) / 8;
	formatMp3_.wfx.nBlockAlign = 1;
	formatMp3_.wfx.wBitsPerSample = 0;
	formatMp3_.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;

	formatMp3_.wID = MPEGLAYER3_ID_MPEG;
	formatMp3_.fdwFlags = padding ? MPEGLAYER3_FLAG_PADDING_ON : MPEGLAYER3_FLAG_PADDING_OFF;
	formatMp3_.nBlockSize = mp3BlockSize;
	formatMp3_.nFramesPerBlock = 1;
	formatMp3_.nCodecDelay = 1393;

	formatWave_.wFormatTag = WAVE_FORMAT_PCM;
	MMRESULT mmResSuggest = acmFormatSuggest(
		nullptr, &formatMp3_.wfx, &formatWave_, sizeof(WAVEFORMATEX),
		ACM_FORMATSUGGESTF_WFORMATTAG
	);
	if (mmResSuggest != 0)
		return false;

	MMRESULT mmResStreamOpen = acmStreamOpen(
		&hAcmStream_, nullptr, &formatMp3_.wfx, &formatWave_, nullptr, 0, 0, 0);
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
	desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_LOCSOFTWARE;
	desc.dwBufferBytes = sizeBuffer;
	desc.lpwfxFormat = &formatWave_;
	HRESULT hrBuffer = soundManager->GetDirectSound()->CreateSoundBuffer(&desc, (LPDIRECTSOUNDBUFFER*)&pDirectSoundBuffer_, nullptr);
	if (FAILED(hrBuffer))return false;

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
	int blockSize = formatWave_.nBlockAlign;

	//lastStreamCopyPos_ = (DWORD)(timeCurrent_ * formatWave_.nAvgBytesPerSec);
	size_t resStreamPos = timeCurrent_ * formatWave_.nAvgBytesPerSec;

	int sizeWriteTotal = 0;
	while (sizeWriteTotal < dwSize) {
		int cSize = dwSize - sizeWriteTotal;
		double cTime = (double)cSize / (double)formatWave_.nAvgBytesPerSec;

		if (timeCurrent_ + cTime > timeLoopEnd_ && timeLoopEnd_ > 0) {
			//ループ終端
			double timeOver = timeCurrent_ + cTime - timeLoopEnd_;
			double cTime1 = cTime - timeOver;
			int cSize1 = cTime1 * formatWave_.nAvgBytesPerSec;
			cSize1 = cSize1 / blockSize * blockSize;

			bool bFileEnd = false;
			int size1Write = 0;
			while (size1Write < cSize1) {
				int tSize = cSize1 - size1Write;
				int sw = _ReadAcmStream((char*)pMem + sizeWriteTotal + size1Write, tSize);
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
			int sizeWrite = _ReadAcmStream((char*)pMem + sizeWriteTotal, cSize);
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
int SoundStreamingPlayerMp3::_ReadAcmStream(char* pBuffer, int size) {
	int sizeWrite = 0;
	if (bufDecode_ != nullptr) {
		//前回デコード分を書き込み
		int bufSize = bufDecode_->GetSize();
		int copySize = std::min(size, bufSize);

		memcpy(pBuffer, bufDecode_->GetPointer(), copySize);
		sizeWrite += copySize;
		if (bufSize > copySize) {
			int newSize = bufSize - copySize;
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
		int readSize = reader_->Read(acmStreamHeader_.pbSrc, acmStreamHeader_.cbSrcLength);
		if (readSize == 0)return 0;
		MMRESULT mmRes = acmStreamConvert(hAcmStream_, &acmStreamHeader_, ACM_STREAMCONVERTF_BLOCKALIGN);
		if (mmRes != 0)return 0;
		if (acmStreamHeader_.cbDstLengthUsed > 0)break;
	}

	int sizeDecode = acmStreamHeader_.cbDstLengthUsed;
	int copySize = std::min(size, sizeDecode);
	memcpy(pBuffer, acmStreamHeader_.pbDst, copySize);
	if (sizeDecode > copySize) {
		//今回余った分を、次回用にバッファリング
		int newSize = sizeDecode - copySize;
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
