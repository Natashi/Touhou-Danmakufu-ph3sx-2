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

			res->pathScript_ = pathScript;
			res->pathArchive_ = pathArchive;
			res->type_ = type;

			res->id_ = info.idScript;
			res->title_ = info.title;
			res->text_ = info.text;
			res->pathImage_ = info.pathImage;
			res->pathSystem_ = info.pathSystem;
			res->pathBackground_ = info.pathBackground;
			res->listPlayer_ = info.listPlayer;
			res->replayName_ = info.replayName;
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
	std::wstring dirInfo = PathProperty::GetFileDirectory(pathScript_);
	for (const std::wstring& pathPlayer : listPlayer_) {
		std::wstring path = EPathProperty::ExtendRelativeToFull(dirInfo, pathPlayer);

		shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr || !reader->Open()) {
			Logger::WriteTop(L"CreatePlayerScriptInformationList: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));
			continue;
		}

		std::string source = reader->ReadAllString();

		auto info = ScriptInformation::CreateScriptInformation(path, L"", source);
		if (info != nullptr && info->type_ == ScriptInformation::TYPE_PLAYER) {
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

	//Found a .dat, open it to read script files within
	if (memcmp(header, ArchiveEncryption::HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH) == 0) {
		file.Close();

		ArchiveFile archive(path, 0);
		if (!archive.Open()) return res;

		auto& mapEntry = archive.GetEntryMap();
		for (auto itr = mapEntry.begin(); itr != mapEntry.end(); itr++) {
			ArchiveFileEntry* entry = &itr->second;

			std::wstring ext = PathProperty::GetFileExtension(entry->path);
			if (ScriptInformation::IsExcludeExtention(ext))
				continue;

			std::wstring tPath = PathProperty::GetModuleDirectory() + entry->fullPath;

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

	if (stdfs::exists(dir) && stdfs::is_directory(dir)) {
		for (auto& itr : stdfs::directory_iterator(dir)) {
			if (itr.is_directory()) {
				std::wstring tDir = PathProperty::ReplaceYenToSlash(itr.path());
				tDir = PathProperty::AppendSlash(tDir);

				std::vector<ref_count_ptr<ScriptInformation>> list = FindPlayerScriptInformationList(tDir);
				for (auto itr = list.begin(); itr != list.end(); itr++) {
					res.push_back(*itr);
				}
			}
			else {
				std::wstring tPath = PathProperty::ReplaceYenToSlash(itr.path());

				std::vector<ref_count_ptr<ScriptInformation>> listInfo = CreateScriptInformationList(tPath, true);
				for (size_t iInfo = 0; iInfo < listInfo.size(); iInfo++) {
					ref_count_ptr<ScriptInformation> info = listInfo[iInfo];
					if (info != nullptr && info->type_ == ScriptInformation::TYPE_PLAYER)
						res.push_back(info);
				}
			}
		}
	}

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
