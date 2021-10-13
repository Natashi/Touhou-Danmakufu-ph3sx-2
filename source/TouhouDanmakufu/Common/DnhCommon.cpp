#include "source/GcLib/pch.h"

#include "DnhCommon.hpp"
#include "DnhGcLibImpl.hpp"

#if defined(DNH_PROJ_EXECUTOR)
//*******************************************************************
//ScriptInformation
//*******************************************************************
const std::wstring ScriptInformation::DEFAULT = L"DEFAULT";

ref_count_ptr<ScriptInformation> ScriptInformation::CreateScriptInformation(const std::wstring& pathScript, bool bNeedHeader) {
	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(pathScript);
	if (reader == nullptr || !reader->Open()) {
		Logger::WriteTop(L"CreateScriptInformation: " + ErrorUtility::GetFileNotFoundErrorMessage(pathScript, true));
		return nullptr;
	}

	std::string source = reader->ReadAllString();

	ref_count_ptr<ScriptInformation> res = CreateScriptInformation(pathScript, L"", source, bNeedHeader);
	return res;
}

ref_count_ptr<ScriptInformation> ScriptInformation::CreateScriptInformation(const std::wstring& pathScript, 
	const std::wstring& pathArchive, const std::string& source, bool bNeedHeader) 
{
	ref_count_ptr<ScriptInformation> res = nullptr;

	Scanner scanner(source);
	Encoding::Type encoding = scanner.GetEncoding();
	try {
		struct Info {
			std::wstring idScript = L"";
			std::wstring title = L"";
			std::wstring text = L"";
			std::wstring pathImage = L"";
			std::wstring pathSystem = DEFAULT;
			std::wstring pathBackground = DEFAULT;
			std::wstring pathBGM = DEFAULT;
			std::vector<std::wstring> listPlayer;
			std::wstring replayName = L"";
		} info;

		bool bScript = false;
		int type = TYPE_SINGLE;
		if (!bNeedHeader) {
			type = TYPE_UNKNOWN;
			bScript = true;
		}

		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::Type::TK_EOF)
				break;
			else if (tok.GetType() == Token::Type::TK_SHARP) {
				tok = scanner.Next();
				std::wstring element = tok.GetElement();

				bool bValidDanmakufuHeader = false;
				if (element == L"TouhouDanmakufu" || element == L"東方弾幕風") {
					bValidDanmakufuHeader = true;
				}
				else {
					int start = tok.GetStartPointer();
					int end = tok.GetEndPointer();
					if (encoding == Encoding::UTF8 || encoding == Encoding::UTF8BOM) {
						static std::string multiThDmfANSI = StringUtility::ConvertWideToMulti(L"東方弾幕風", CP_ACP);
						static std::string multiThDmfUTF8 = StringUtility::ConvertWideToMulti(L"東方弾幕風", CP_UTF8);

						//Try ANSI, then UTF-8
						bValidDanmakufuHeader = scanner.CompareMemory(start, end, multiThDmfANSI.data())
							|| scanner.CompareMemory(start, end, multiThDmfUTF8.data());
					}
				}

				if (bValidDanmakufuHeader) {
					bScript = true;
					if (scanner.Next().GetType() != Token::Type::TK_OPENB) continue;
					tok = scanner.Next();
					std::wstring strType = tok.GetElement();
					if (scanner.Next().GetType() != Token::Type::TK_CLOSEB) throw gstd::wexception();

					static const std::unordered_map<std::wstring, int> mapTypes = {
						{ L"Single", TYPE_SINGLE },
						{ L"Plural", TYPE_PLURAL },
						{ L"Stage", TYPE_STAGE },
						{ L"Package", TYPE_PACKAGE },
						{ L"Player", TYPE_PLAYER },
					};
					auto itrFind = mapTypes.find(strType);
					if (itrFind != mapTypes.end()) type = itrFind->second;
					else if (!bNeedHeader) throw gstd::wexception();
				}
				else {
#define LAMBDA_GETSTR(m) [](Info* i, Scanner& sc) { i->m = _GetString(sc); }
					//Do NOT use [&] lambdas
					static const std::unordered_map<std::wstring, std::function<void(Info*, Scanner&)>> mapFunc = {
						{ L"ID", LAMBDA_GETSTR(idScript) },
						{ L"Title", LAMBDA_GETSTR(title) },
						{ L"Text", LAMBDA_GETSTR(text) },
						{ L"Image", LAMBDA_GETSTR(pathImage) },
						{ L"System", LAMBDA_GETSTR(pathSystem) },
						{ L"Background", LAMBDA_GETSTR(pathBackground) },
						{ L"BGM", LAMBDA_GETSTR(pathBGM) },
						{ L"Player", [](Info* i, Scanner& sc) { i->listPlayer = _GetStringList(sc); } },
						{ L"ReplayName", LAMBDA_GETSTR(replayName) },
					};
#undef LAMBDA_GETSTR
					auto itrFind = mapFunc.find(element);
					if (itrFind != mapFunc.end())
						itrFind->second(&info, scanner);
				}
			}
		}

		if (bScript && info.title.size() > 0) {
			if (info.idScript.size() == 0)
				info.idScript = PathProperty::GetFileNameWithoutExtension(pathScript);

			if (info.replayName.size() == 0)
				info.replayName = info.idScript.substr(0, std::min(8U, info.replayName.size()));

			if (info.pathImage.size() > 2) {
				std::wstring& pathImage = info.pathImage;
				if (pathImage[0] == L'.' &&
					(pathImage[1] == L'/' || pathImage[1] == L'\\')) {
					pathImage = pathImage.substr(2);
					pathImage = PathProperty::GetFileDirectory(pathScript) + pathImage;
				}
			}

			res = new ScriptInformation();
			res->SetScriptPath(pathScript);
			res->SetArchivePath(pathArchive);
			res->SetType(type);

			res->SetID(info.idScript);
			res->SetTitle(info.title);
			res->SetText(info.text);
			res->SetImagePath(info.pathImage);
			res->SetSystemPath(info.pathSystem);
			res->SetBackgroundPath(info.pathBackground);
			res->SetBgmPath(info.pathBGM);
			res->SetPlayerList(info.listPlayer);
			res->SetReplayName(info.replayName);
		}
	}
	catch (...) {
		res = nullptr;
	}

	return res;
}
bool ScriptInformation::IsExcludeExtention(const std::wstring& ext) {
	static std::set<std::wstring> setExt = {
		L".dat",

		L".ttf", L".otf",

		L".mid", L".wav", L".mp3", L".ogg",

		L".dxt",
		L".bmp", L".jpg", L".jpeg",
		L".tga", L".png", L".ppm",
		L".dib", L".hdr", L".pfm",

		L".mqo", L".elem",
	};
	return setExt.find(ext) != setExt.end();
}
std::wstring ScriptInformation::_GetString(Scanner& scanner) {
	std::wstring res = DEFAULT;
	scanner.CheckType(scanner.Next(), Token::Type::TK_OPENB);
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::Type::TK_STRING) {
		res = tok.GetString();
	}
	scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEB);
	return res;
}
std::vector<std::wstring> ScriptInformation::_GetStringList(Scanner& scanner) {
	std::vector<std::wstring> res;
	scanner.CheckType(scanner.Next(), Token::Type::TK_OPENB);
	while (true) {
		Token& tok = scanner.Next();
		Token::Type type = tok.GetType();
		if (type == Token::Type::TK_CLOSEB) break;
		else if (type == Token::Type::TK_STRING) {
			std::wstring wstr = tok.GetString();
			res.push_back(wstr);
		}
	}
	return res;
}
std::vector<ref_count_ptr<ScriptInformation>> ScriptInformation::CreatePlayerScriptInformationList() {
	std::vector<ref_count_ptr<ScriptInformation>> res;
	std::wstring dirInfo = PathProperty::GetFileDirectory(GetScriptPath());
	for (const std::wstring& pathPlayer : listPlayer_) {
		std::wstring path = EPathProperty::ExtendRelativeToFull(dirInfo, pathPlayer);

		shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr || !reader->Open()) {
			Logger::WriteTop(L"CreatePlayerScriptInformationList: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));
			continue;
		}

		std::string source = reader->ReadAllString();

		auto info = ScriptInformation::CreateScriptInformation(path, L"", source);
		if (info != nullptr && info->GetType() == ScriptInformation::TYPE_PLAYER) {
			res.push_back(info);
		}
	}
	return res;
}
std::vector<ref_count_ptr<ScriptInformation>> ScriptInformation::CreateScriptInformationList(const std::wstring& path, 
	bool bNeedHeader) 
{
	std::vector<ref_count_ptr<ScriptInformation>> res;
	File file(path);
	if (!file.Open()) return res;
	if (file.GetSize() < ArchiveFileHeader::MAGIC_LENGTH) return res;

	char header[ArchiveFileHeader::MAGIC_LENGTH];
	file.Read(&header, ArchiveFileHeader::MAGIC_LENGTH);
	{
		byte keyBase;
		byte keyStep;
		ArchiveEncryption::GetKeyHashHeader(ArchiveEncryption::ARCHIVE_ENCRYPTION_KEY, keyBase, keyStep);
		ArchiveEncryption::ShiftBlock((byte*)header, ArchiveFileHeader::MAGIC_LENGTH, keyBase, keyStep);
	}

	//Reading a .dat
	if (memcmp(header, ArchiveEncryption::HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH) == 0) {
		file.Close();

		ArchiveFile archive(path, 0);
		if (!archive.Open()) return res;

		std::multimap<std::wstring, shared_ptr<ArchiveFileEntry>>& mapEntry = archive.GetEntryMap();
		for (auto itr = mapEntry.begin(); itr != mapEntry.end(); itr++) {
			shared_ptr<ArchiveFileEntry> entry = itr->second;

			std::wstring ext = PathProperty::GetFileExtension(entry->name);
			if (ScriptInformation::IsExcludeExtention(ext)) continue;

			std::wstring tPath = PathProperty::GetFileDirectory(path) + entry->directory + entry->name;

			shared_ptr<ByteBuffer> buffer = ArchiveFile::CreateEntryBuffer(entry);
			std::string source = "";
			size_t size = buffer->GetSize();
			source.resize(size);
			buffer->Read(&source[0], size);

			auto info = CreateScriptInformation(tPath, path, source, bNeedHeader);
			if (info) res.push_back(info);
		}
	}
	else {
		std::wstring ext = PathProperty::GetFileExtension(path);
		if (ScriptInformation::IsExcludeExtention(ext)) return res;

		file.SetFilePointerBegin();
		std::string source = "";
		size_t size = file.GetSize();
		source.resize(size);
		file.Read(&source[0], size);

		auto info = CreateScriptInformation(path, L"", source, bNeedHeader);
		if (info) res.push_back(info);

		file.Close();
	}

	return res;
}
std::vector<ref_count_ptr<ScriptInformation>> ScriptInformation::FindPlayerScriptInformationList(const std::wstring& dir) {
	std::vector<ref_count_ptr<ScriptInformation>> res;
	WIN32_FIND_DATA data;
	HANDLE hFind;
	std::wstring findDir = dir + L"*.*";
	hFind = FindFirstFile(findDir.c_str(), &data);
	do {
		std::wstring name = data.cFileName;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(name != L".." && name != L".")) {
			//ディレクトリ
			std::wstring tDir = dir + name + L"/";

			std::vector<ref_count_ptr<ScriptInformation>> list = FindPlayerScriptInformationList(tDir);
			for (auto itr = list.begin(); itr != list.end(); itr++) {
				res.push_back(*itr);
			}
			continue;
		}
		if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0)
			continue;

		//ファイル
		std::wstring path = dir + name;

		//スクリプト解析
		std::vector<ref_count_ptr<ScriptInformation>> listInfo = CreateScriptInformationList(path, true);
		for (size_t iInfo = 0; iInfo < listInfo.size(); iInfo++) {
			ref_count_ptr<ScriptInformation> info = listInfo[iInfo];
			if (info != nullptr && info->GetType() == ScriptInformation::TYPE_PLAYER)
				res.push_back(info);
		}

	} while (FindNextFile(hFind, &data));
	FindClose(hFind);

	return res;
}
#endif

//*******************************************************************
//ErrorDialog
//*******************************************************************
HWND ErrorDialog::hWndParentStatic_ = nullptr;
ErrorDialog::ErrorDialog(HWND hParent) {
	hParent_ = hParent;
}
LRESULT ErrorDialog::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
	{
		_FinishMessageLoop();
		break;
	}
	case WM_CLOSE:
		DestroyWindow(hWnd_);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			DestroyWindow(hWnd_);
		}
		break;
	case WM_COMMAND:
	{
		int param = wParam & 0xffff;
		if (param == button_.GetWindowId()) {
			DestroyWindow(hWnd_);
		}

		break;
	}
	case WM_SIZE:
	{
		RECT rect;
		::GetClientRect(hWnd_, &rect);
		int wx = rect.left;
		int wy = rect.top;
		int wWidth = rect.right - rect.left;
		int wHeight = rect.bottom - rect.top;

		RECT rcButton = button_.GetClientRect();
		int widthButton = rcButton.right - rcButton.left;
		int heightButton = rcButton.bottom - rcButton.top;
		button_.SetBounds(wWidth / 2 - widthButton / 2, wHeight - heightButton - 8, widthButton, heightButton);

		edit_.SetBounds(wx + 8, wy + 8, wWidth - 16, wHeight - heightButton - 24);

		break;
	}

	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}
bool ErrorDialog::ShowModal(std::wstring msg) {
	HINSTANCE hInst = ::GetModuleHandle(NULL);
	std::wstring wName = L"ErrorWindow";

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
	wcex.hInstance = hInst;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = wName.c_str();
	wcex.hIconSm = NULL;
	RegisterClassEx(&wcex);

	hWnd_ = ::CreateWindow(wcex.lpszClassName,
		wName.c_str(),
		WS_OVERLAPPEDWINDOW,
		0, 0, 480, 320, hParent_, (HMENU)NULL, hInst, NULL);
	::ShowWindow(hWnd_, SW_HIDE);
	this->Attach(hWnd_);


	gstd::WEditBox::Style styleEdit;
	styleEdit.SetStyle(WS_CHILD | WS_VISIBLE |
		ES_MULTILINE | ES_READONLY | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
		WS_HSCROLL | WS_VSCROLL);
	styleEdit.SetStyleEx(WS_EX_CLIENTEDGE);
	edit_.Create(hWnd_, styleEdit);
	edit_.SetText(msg);

	button_.Create(hWnd_);
	button_.SetText(L"OK");
	button_.SetBounds(0, 0, 88, 20);

	MoveWindowCenter();
	SetWindowVisible(true);
	_RunMessageLoop();
	return true;
}

//*******************************************************************
//DnhConfiguration
//*******************************************************************
DnhConfiguration::DnhConfiguration() {
	modeScreen_ = ScreenMode::SCREENMODE_WINDOW;
	modeColor_ = ColorMode::COLOR_MODE_32BIT;
	fpsType_ = FPS_NORMAL;
	fastModeSpeed_ = 20;

	windowSizeList_ = { { 640, 480 }, { 800, 600 }, { 960, 720 }, { 1280, 960 } };
	sizeWindow_ = 0;

	bVSync_ = true;
	referenceRasterizer_ = false;
	bPseudoFullscreen_ = true;
	multiSamples_ = D3DMULTISAMPLE_NONE;

	pathExeLaunch_ = DNH_EXE_NAME;

	padIndex_ = 0;
	mapKey_[EDirectInput::KEY_LEFT] = new VirtualKey(DIK_LEFT, 0, 0);
	mapKey_[EDirectInput::KEY_RIGHT] = new VirtualKey(DIK_RIGHT, 0, 1);
	mapKey_[EDirectInput::KEY_UP] = new VirtualKey(DIK_UP, 0, 2);
	mapKey_[EDirectInput::KEY_DOWN] = new VirtualKey(DIK_DOWN, 0, 3);

	mapKey_[EDirectInput::KEY_OK] = new VirtualKey(DIK_Z, 0, 5);
	mapKey_[EDirectInput::KEY_CANCEL] = new VirtualKey(DIK_X, 0, 6);

	mapKey_[EDirectInput::KEY_SHOT] = new VirtualKey(DIK_Z, 0, 5);
	mapKey_[EDirectInput::KEY_BOMB] = new VirtualKey(DIK_X, 0, 6);
	mapKey_[EDirectInput::KEY_SLOWMOVE] = new VirtualKey(DIK_LSHIFT, 0, 7);
	mapKey_[EDirectInput::KEY_USER1] = new VirtualKey(DIK_C, 0, 8);
	mapKey_[EDirectInput::KEY_USER2] = new VirtualKey(DIK_V, 0, 9);

	mapKey_[EDirectInput::KEY_PAUSE] = new VirtualKey(DIK_ESCAPE, 0, 10);

	bLogWindow_ = false;
	bLogFile_ = false;
	bMouseVisible_ = true;

	screenWidth_ = 640;
	screenHeight_ = 480;

	LoadConfigFile();
	_LoadDefinitionFile();
}
DnhConfiguration::~DnhConfiguration() {}

bool DnhConfiguration::_LoadDefinitionFile() {
	PropertyFile prop;
	if (!prop.Load(PathProperty::GetModuleDirectory() + L"th_dnh.def")) return false;

	pathPackageScript_ = prop.GetString(L"package.script.main", L"");
	if (pathPackageScript_.size() > 0) {
		pathPackageScript_ = PathProperty::GetModuleDirectory() + pathPackageScript_;
		pathPackageScript_ = PathProperty::ReplaceYenToSlash(pathPackageScript_);
	}

	windowTitle_ = prop.GetString(L"window.title", L"");

	screenWidth_ = prop.GetInteger(L"screen.width", 640);
	// screenWidth_ = std::clamp(screenWidth_, 320L, 1920L);

	screenHeight_ = prop.GetInteger(L"screen.height", 480);
	// screenHeight_ = std::clamp(screenHeight_, 240L, 1200L);

	fastModeSpeed_ = prop.GetInteger(L"skip.rate", 20);
	// fastModeSpeed_ = std::clamp(fastModeSpeed_, 1, 50);

	{
		std::wstring str = prop.GetString(L"dynamic.scaling", L"false");
		bDynamicScaling_ = str == L"true" ? true : StringUtility::ToInteger(str);
	}

	{
		if (prop.HasProperty(L"window.size.list")) {
			std::wstring strList = prop.GetString(L"window.size.list", L"");
			if (strList.size() >= 3) {	//Minimum format: "0x0"
				std::wregex reg(L"([0-9]+)x([0-9]+)");
				auto itrBegin = std::wsregex_iterator(strList.begin(), strList.end(), reg);
				auto itrEnd = std::wsregex_iterator();

				if (itrBegin != itrEnd) {
					windowSizeList_.clear();

					for (auto itr = itrBegin; itr != itrEnd; ++itr) {
						const std::wsmatch& match = *itr;

						POINT size;
						size.x = wcstol(match[1].str().c_str(), nullptr, 10);
						size.y = wcstol(match[2].str().c_str(), nullptr, 10);
						// size.x = std::clamp(size.x, 320L, 1920L);
						// size.y = std::clamp(size.y, 240L, 1200L);

						windowSizeList_.push_back(size);
					}
				}
			}
		}
		if (windowSizeList_.size() == 0)
			windowSizeList_ = { { 640, 480 }, { 800, 600 }, { 960, 720 }, { 1280, 960 } };
	}

	return true;
}

bool DnhConfiguration::LoadConfigFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"config.dat";

	RecordBuffer record;
	bool res = record.ReadFromFile(path, GAME_VERSION_NUM, "DNHCNFG\0", 8U);
	if (!res) return false;

	record.GetRecord<ScreenMode>("modeScreen", modeScreen_);
	record.GetRecord<ColorMode>("modeColor", modeColor_);
	
	record.GetRecord<int>("fpsType", fpsType_);
	
	record.GetRecord<size_t>("sizeWindow", sizeWindow_);

	record.GetRecord<bool>("bVSync", bVSync_);
	record.GetRecord<bool>("bDeviceREF", referenceRasterizer_);
	record.GetRecord<bool>("bPseudoFullscreen", bPseudoFullscreen_);

	record.GetRecord<D3DMULTISAMPLE_TYPE>("typeMultiSamples", multiSamples_);

	pathExeLaunch_ = record.GetRecordAsStringW("pathLaunch");
	if (pathExeLaunch_.size() == 0) pathExeLaunch_ = DNH_EXE_NAME;

	if (record.IsExists("padIndex"))
		padIndex_ = record.GetRecordAsInteger("padIndex");

	{
		ByteBuffer bufKey;
		int bufKeySize = record.GetRecordAsInteger("mapKey_size");
		bufKey.SetSize(bufKeySize);
		record.GetRecord("mapKey", bufKey.GetPointer(), bufKey.GetSize());

		size_t mapKeyCount = bufKey.ReadValue<size_t>();
		if (mapKeyCount == mapKey_.size()) {
			for (size_t iKey = 0; iKey < mapKeyCount; iKey++) {
				int16_t id = bufKey.ReadShort();
				int16_t keyCode = bufKey.ReadShort();
				int16_t padIndex = bufKey.ReadShort();
				int16_t padButton = bufKey.ReadShort();

				mapKey_[id] = new VirtualKey(keyCode, padIndex, padButton);
			}
		}
	}

	bLogWindow_ = record.GetRecordAsBoolean("bLogWindow");
	bLogFile_ = record.GetRecordAsBoolean("bLogFile");
	if (record.IsExists("bMouseVisible"))
		bMouseVisible_ = record.GetRecordAsBoolean("bMouseVisible");

	return res;
}
bool DnhConfiguration::SaveConfigFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"config.dat";

	RecordBuffer record;

	record.SetRecord<ScreenMode>("modeScreen", modeScreen_);
	record.SetRecord<ColorMode>("modeColor", modeColor_);

	record.SetRecordAsInteger("fpsType", fpsType_);

	record.SetRecord<size_t>("sizeWindow", sizeWindow_);

	record.SetRecordAsBoolean("bVSync", bVSync_);
	record.SetRecordAsBoolean("bDeviceREF", referenceRasterizer_);
	record.SetRecordAsBoolean("bPseudoFullscreen", bPseudoFullscreen_);

	record.SetRecord<D3DMULTISAMPLE_TYPE>("typeMultiSamples", multiSamples_);

	record.SetRecordAsStringW("pathLaunch", pathExeLaunch_);

	record.SetRecordAsInteger("padIndex", padIndex_);

	{
		ByteBuffer bufKey;
		bufKey.WriteValue(mapKey_.size());
		for (auto itrKey = mapKey_.begin(); itrKey != mapKey_.end(); itrKey++) {
			int16_t id = itrKey->first;
			ref_count_ptr<VirtualKey> vk = itrKey->second;

			bufKey.WriteShort(id);
			bufKey.WriteShort(vk->GetKeyCode());
			bufKey.WriteShort(padIndex_);
			bufKey.WriteShort(vk->GetPadButton());
		}
		record.SetRecordAsInteger("mapKey_size", bufKey.GetSize());
		record.SetRecord("mapKey", bufKey.GetPointer(), bufKey.GetSize());
	}

	record.SetRecordAsBoolean("bLogWindow", bLogWindow_);
	record.SetRecordAsBoolean("bLogFile", bLogFile_);
	record.SetRecordAsBoolean("bMouseVisible", bMouseVisible_);

	record.WriteToFile(path, GAME_VERSION_NUM, "DNHCNFG\0", 8U);
	return true;
}
ref_count_ptr<VirtualKey> DnhConfiguration::GetVirtualKey(int16_t id) {
	auto itr = mapKey_.find(id);
	if (itr == mapKey_.end()) return nullptr;
	return itr->second;
}
