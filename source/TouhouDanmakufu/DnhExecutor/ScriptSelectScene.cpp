#include "source/GcLib/pch.h"

#include "ScriptSelectScene.hpp"
#include "System.hpp"

//*******************************************************************
//ScriptSelectScene
//*******************************************************************
ScriptSelectScene::ScriptSelectScene() {
	std::wstring pathBack = EPathProperty::GetSystemImageDirectory() + L"System_ScriptSelect_Background.png";
	shared_ptr<Texture> textureBack(new Texture());
	textureBack->CreateFromFile(PathProperty::GetUnique(pathBack), false, true);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG screenWidth = graphics->GetScreenWidth();
	LONG screenHeight = graphics->GetScreenHeight();

	DxRect<int> srcBack(0, 0, 640, 480);
	DxRect<double> destBack(0, 0, screenWidth, screenHeight);

	spriteBack_ = std::make_shared<Sprite2D>();
	spriteBack_->SetTexture(textureBack);
	spriteBack_->SetVertex(srcBack, destBack);

	spriteImage_ = std::make_shared<Sprite2D>();

	pageMaxY_ = 11;
	objMenuText_.resize(COUNT_MENU_TEXT);
	bPageChangeX_ = true;

	frameSelect_ = 0;
}
ScriptSelectScene::~ScriptSelectScene() {
	ClearModel();
}
void ScriptSelectScene::Clear() {
	MenuTask::Clear();
	objMenuText_.clear();
	objMenuText_.resize(COUNT_MENU_TEXT);
}
void ScriptSelectScene::_ChangePage() {
	DxText dxText;
	dxText.SetFontBorderType(TextBorderType::Full);
	dxText.SetFontBorderWidth(2);
	dxText.SetFontSize(16);
	dxText.SetFontWeight(FW_BOLD);
    // dxText.SetFontItalic(FW_BOLD);
	dxText.SetSyntacticAnalysis(false);

	int top = (pageCurrent_ - 1) * (pageMaxY_ + 1);
	for (int iItem = 0; iItem <= pageMaxY_; iItem++) {
		int index = top + iItem;
		if (index < item_.size() && item_[index] != nullptr) {
			ScriptSelectSceneMenuItem* pItem = (ScriptSelectSceneMenuItem*)item_[index].get();
			if (pItem->GetType() == ScriptSelectSceneMenuItem::TYPE_DIR) {
				dxText.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
				dxText.SetFontColorBottom(D3DCOLOR_ARGB(255, 255, 64, 64));
				dxText.SetFontBorderColor(D3DCOLOR_ARGB(255, 128, 32, 32));
				
				std::wstring text = L"[DIR.]";
				text += PathProperty::GetDirectoryName(pItem->GetPath());
				dxText.SetText(text);
			}
			else {
				dxText.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
				dxText.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 255));
				dxText.SetFontBorderColor(D3DCOLOR_ARGB(255, 32, 32, 128));
				ref_count_ptr<ScriptInformation> info = pItem->GetScriptInformation();
				dxText.SetText(info->GetTitle());
			}

			objMenuText_[iItem] = dxText.CreateRenderObject();
		}
		else {
			objMenuText_[iItem] = nullptr;
		}
	}

	frameSelect_ = 0;
}
void ScriptSelectScene::Work() {
	{
		ref_count_ptr<ScriptSelectFileModel> model = ref_count_ptr<ScriptSelectFileModel>::Cast(model_);
		if (model != nullptr && model->GetStatus() == Thread::RUN) {
			if (model->GetWaitPath().size() > 0) return;
		}
	}
	if (!bActive_) return;

	int lastCursorY = cursorY_;

	MenuTask::Work();
	if (!_IsWaitedKeyFree()) return;

	EDirectInput* input = EDirectInput::GetInstance();
	if (input->GetVirtualKeyState(EDirectInput::KEY_OK) == KEY_PUSH) {
		ref_count_ptr<MenuItem> tItem = GetSelectedMenuItem();
		ref_count_ptr<ScriptSelectSceneMenuItem> item = ref_count_ptr<ScriptSelectSceneMenuItem>::Cast(tItem);
		if (item) {
			bool bDir = item->GetType() == ScriptSelectSceneMenuItem::TYPE_DIR;
			if (bDir) {
				//ディレクトリ
				ref_count_ptr<ScriptSelectModel> model = nullptr;
				int index = GetSelectedItemIndex();
				ScriptSelectSceneMenuItem* pItem = (ScriptSelectSceneMenuItem*)item_[index].get();
				const std::wstring& dir = pItem->GetPath();

				//ページリセット
				cursorX_ = 0;
				cursorY_ = 0;
				pageCurrent_ = 1;

				model = new ScriptSelectFileModel(TYPE_DIR, dir);
				SetModel(model);

				SystemController::GetInstance()->GetSystemInformation()->SetLastScriptSearchDirectory(dir);
				bWaitedKeyFree_ = false;
			}
			else {
				//スクリプト
				SetActive(false);

				int index = GetSelectedItemIndex();
				ScriptSelectSceneMenuItem* pItem = (ScriptSelectSceneMenuItem*)item_[index].get();
				ref_count_ptr<ScriptInformation> info = pItem->GetScriptInformation();

				const std::wstring& pathLastSelected = info->GetScriptPath();
				SystemController::GetInstance()->GetSystemInformation()->SetLastSelectedScriptPath(pathLastSelected);

				ETaskManager* taskManager = ETaskManager::GetInstance();

				shared_ptr<PlayTypeSelectScene> task(new PlayTypeSelectScene(info));
				taskManager->AddTask(task);
				taskManager->AddWorkFunction(TTaskFunction<PlayTypeSelectScene>::Create(task, 
					&PlayTypeSelectScene::Work), PlayTypeSelectScene::TASK_PRI_WORK);
				taskManager->AddRenderFunction(TTaskFunction<PlayTypeSelectScene>::Create(task, 
					&PlayTypeSelectScene::Render), PlayTypeSelectScene::TASK_PRI_RENDER);

				return;
			}
		}
	}
	else if (input->GetVirtualKeyState(EDirectInput::KEY_CANCEL) == KEY_PUSH) {
		bool bTitle = true;

		if (GetType() == TYPE_DIR) {
			ref_count_ptr<ScriptSelectFileModel> fileModel = ref_count_ptr<ScriptSelectFileModel>::Cast(model_);
			const std::wstring& dir = fileModel->GetDirectory();
			const std::wstring& root = EPathProperty::GetStgScriptRootDirectory();
			if (!File::IsEqualsPath(dir, root)) {
				//キャンセル
				bTitle = false;
				const std::wstring& dirOld = SystemController::GetInstance()->GetSystemInformation()->GetLastScriptSearchDirectory();
				std::wstring::size_type pos = dirOld.find_last_of(L"/", dirOld.size() - 2);
				std::wstring dir = dirOld.substr(0, pos) + L"/";
				ref_count_ptr<ScriptSelectFileModel> model = new ScriptSelectFileModel(TYPE_DIR, dir);
				model->SetWaitPath(dirOld);

				//SetWaitPathで選択中のパスに移動させるために
				//ページリセット
				cursorX_ = 0;
				cursorY_ = 0;
				pageCurrent_ = 1;
				SetModel(model);

				SystemController::GetInstance()->GetSystemInformation()->SetLastScriptSearchDirectory(dir);
				bWaitedKeyFree_ = false;
				return;
			}
		}

		if (bTitle) {
			SceneManager* sceneManager = SystemController::GetInstance()->GetSceneManager();
			sceneManager->TransTitleScene();
			return;
		}
	}

	if (lastCursorY != cursorY_) frameSelect_ = 0;
	else frameSelect_++;
}
void ScriptSelectScene::Render() {
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();
	graphics->SetRenderStateFor2D(MODE_BLEND_ALPHA);

	spriteBack_->Render();

	std::wstring strType;
	switch (GetType()) {
	case TYPE_SINGLE: strType = L"Single"; break;
	case TYPE_PLURAL: strType = L"Plural"; break;
	case TYPE_STAGE: strType = L"Stage"; break;
	case TYPE_PACKAGE: strType = L"Package"; break;
	case TYPE_DIR: strType = L"Directory"; break;
	case TYPE_ALL: strType = L"All"; break;
	}

	std::wstring strDescription = StringUtility::Format(
		L"Script select (%s, Page:%d/%d)", strType.c_str(), pageCurrent_, GetPageCount());

	DxText dxTextDescription;
	dxTextDescription.SetFontColorTop(D3DCOLOR_ARGB(255, 128, 128, 255));
	dxTextDescription.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 255));
	dxTextDescription.SetFontBorderType(TextBorderType::Full);
	dxTextDescription.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxTextDescription.SetFontBorderWidth(2);
	dxTextDescription.SetFontSize(20);
	dxTextDescription.SetFontWeight(FW_BOLD);
	dxTextDescription.SetText(strDescription);
	dxTextDescription.SetPosition(32, 8);
	//dxTextDescription.SetSyntacticAnalysis(false);
	dxTextDescription.Render();

	//ディレクトリ名
	if (GetType() == TYPE_DIR) {
		ref_count_ptr<ScriptSelectFileModel> fileModel = ref_count_ptr<ScriptSelectFileModel>::Cast(model_);
		std::wstring dir = fileModel->GetDirectory();
		std::wstring root = EPathProperty::GetStgScriptRootDirectory();

		dir = PathProperty::ReplaceYenToSlash(dir);
		root = PathProperty::ReplaceYenToSlash(root);
		std::wstring textDir = L"script/" + StringUtility::ReplaceAll(dir, root, L"");

		DxText dxTextDir;
		dxTextDir.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
		dxTextDir.SetFontColorBottom(D3DCOLOR_ARGB(255, 255, 128, 128));
		dxTextDir.SetFontBorderType(TextBorderType::None);
		dxTextDir.SetFontBorderWidth(0);
		dxTextDir.SetFontSize(14);
		dxTextDir.SetFontWeight(FW_BOLD);
		dxTextDir.SetText(strDescription);
		dxTextDir.SetPosition(40, 32);
		dxTextDir.SetText(textDir);
		dxTextDir.SetSyntacticAnalysis(false);
		dxTextDir.Render();
	}

	{
		//タイトル
		Lock lock(cs_);
		for (int iItem = 0; iItem <= pageMaxY_; iItem++) {
			shared_ptr<DxTextRenderObject> obj = objMenuText_[iItem];
			if (obj == nullptr) continue;
			int alphaText = bActive_ ? 255 : 128;
			obj->SetVertexColor(D3DCOLOR_ARGB(255, alphaText, alphaText, alphaText));
			obj->SetPosition(32, 48 + iItem * 17);
			obj->Render();

			if (iItem == cursorY_) {
				graphics->SetBlendMode(MODE_BLEND_ADD_RGB);
				int cycle = 60;
				int alpha = frameSelect_ % cycle;
				if (alpha < cycle / 2) alpha = 255 * (float)((float)(cycle / 2 - alpha) / (float)(cycle / 2));
				else alpha = 255 * (float)((float)(alpha - cycle / 2) / (float)(cycle / 2));
				obj->SetVertexColor(D3DCOLOR_ARGB(255, alpha, alpha, alpha));
				obj->Render();
				obj->SetVertexColor(D3DCOLOR_ARGB(255, 255, 255, 255));
				graphics->SetBlendMode(MODE_BLEND_ALPHA);
			}
		}

		ref_count_ptr<ScriptSelectSceneMenuItem> item =
			ref_count_ptr<ScriptSelectSceneMenuItem>::Cast(GetSelectedMenuItem());
		if (bActive_ && item != nullptr && item->GetType() != ScriptSelectSceneMenuItem::TYPE_DIR) {
			ref_count_ptr<ScriptInformation> info = item->GetScriptInformation();

			DxText dxTextInfo;
			dxTextInfo.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
			dxTextInfo.SetFontBorderType(TextBorderType::None);
			dxTextInfo.SetFontBorderWidth(0);
			dxTextInfo.SetFontSize(16);
			dxTextInfo.SetFontWeight(FW_BOLD);
			
			//dxTextInfo.SetSyntacticAnalysis(false);

			//イメージ
			shared_ptr<Texture> texture = spriteImage_->GetTexture();
			std::wstring pathImage1 = L"";
			if (texture) pathImage1 = texture->GetName();
			const std::wstring& pathImage2 = info->GetImagePath();
			if (pathImage1 != pathImage2) {
				texture = std::make_shared<Texture>();
				File file(pathImage2);
				if (file.IsExists()) {
					texture->CreateFromFileInLoadThread(pathImage2, false, true);
					spriteImage_->SetTexture(texture);
				}
				else
					spriteImage_->SetTexture(nullptr);

			}

			texture = spriteImage_->GetTexture();
			if (texture != nullptr && texture->IsLoad()) {
				DxRect<int> rcSrc(0, 0, texture->GetWidth(), texture->GetHeight());
				DxRect<double> rcDest(340, 250, 340 + 280, 250 + 210);
				spriteImage_->SetSourceRect(rcSrc);
				spriteImage_->SetDestinationRect(rcDest);
				spriteImage_->Render();
			}

			//スクリプトパス
			std::wstring path = info->GetScriptPath();
			std::wstring root = EPathProperty::GetStgScriptRootDirectory();
			root = PathProperty::ReplaceYenToSlash(root);
			path = PathProperty::ReplaceYenToSlash(path);
			path = StringUtility::ReplaceAll(path, root, L"");

			std::wstring archive = info->GetArchivePath();
			if (archive.size() > 0) {
				archive = PathProperty::ReplaceYenToSlash(archive);
				archive = StringUtility::ReplaceAll(archive, root, L"");
				path += StringUtility::Format(L" [%s]", archive.c_str());
			}

			dxTextInfo.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
			dxTextInfo.SetFontColorBottom(D3DCOLOR_ARGB(255, 255, 64, 64));
			dxTextInfo.SetText(path);
			dxTextInfo.SetPosition(16, 256);
			dxTextInfo.Render();

			//スクリプト種別
			int type = info->GetType();
			std::wstring strType = L"";
			switch (type) {
			case ScriptInformation::TYPE_SINGLE:strType = L"(Single)"; break;
			case ScriptInformation::TYPE_PLURAL:strType = L"(Plural)"; break;
			case ScriptInformation::TYPE_STAGE:strType = L"(Stage)"; break;
			case ScriptInformation::TYPE_PACKAGE:strType = L"(Package)"; break;
			}
			dxTextInfo.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
			dxTextInfo.SetFontColorBottom(D3DCOLOR_ARGB(255, 255, 64, 255));
			dxTextInfo.SetText(strType);
			dxTextInfo.SetPosition(32, 272);
			dxTextInfo.Render();

			//テキスト
			const std::wstring& text = info->GetText();
			dxTextInfo.SetFontColorTop(D3DCOLOR_ARGB(255, 160, 160, 160));
			dxTextInfo.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 255));
            dxTextInfo.SetFontBorderType(TextBorderType::Full);
		    dxTextInfo.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
			dxTextInfo.SetFontSize(16);
			dxTextInfo.SetLinePitch(1);
            dxTextInfo.SetFontBorderWidth(1);
			dxTextInfo.SetSidePitch(-1);
			dxTextInfo.SetText(text);
			dxTextInfo.SetPosition(24, 302);
			dxTextInfo.SetMaxWidth(texture == nullptr ? 600 : 320);
			dxTextInfo.Render();
		}
	}

	if (!model_->IsCreated()) {
		//ロード中
		std::wstring text = L"Now Loading...";
		DxText dxTextNowLoad;
		dxTextNowLoad.SetFontColorTop(D3DCOLOR_ARGB(255, 128, 128, 128));
		dxTextNowLoad.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 64));
		dxTextNowLoad.SetFontBorderType(TextBorderType::Full);
		dxTextNowLoad.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
		dxTextNowLoad.SetFontBorderWidth(2);
		dxTextNowLoad.SetFontSize(18);
		dxTextNowLoad.SetFontWeight(FW_BOLD);
		dxTextNowLoad.SetText(strDescription);
		dxTextNowLoad.SetPosition(24, 452);
		dxTextNowLoad.SetText(text);
		dxTextNowLoad.SetSyntacticAnalysis(false);
		dxTextNowLoad.Render();
	}
}

int ScriptSelectScene::GetType() {
	int res = TYPE_SINGLE;
	ref_count_ptr<ScriptSelectFileModel> fileModel = ref_count_ptr<ScriptSelectFileModel>::Cast(model_);
	if (fileModel) {
		res = fileModel->GetType();
	}
	return res;
}
void ScriptSelectScene::SetModel(ref_count_ptr<ScriptSelectModel> model) {
	ClearModel();

	if (model == nullptr) return;
	model->scene_ = this;
	model_ = model;

	ref_count_ptr<ScriptSelectFileModel> fileModel = ref_count_ptr<ScriptSelectFileModel>::Cast(model_);
	if (fileModel) {
		SystemController::GetInstance()->GetSystemInformation()->SetLastSelectScriptSceneType(
			fileModel->GetType());
	}

	model->CreateMenuItem();
}
void ScriptSelectScene::ClearModel() {
	Clear();
	if (model_) {
		ref_count_ptr<ScriptSelectFileModel> fileModel = ref_count_ptr<ScriptSelectFileModel>::Cast(model_);
		if (fileModel) fileModel->Stop();
		fileModel->Join();
	}

	model_ = nullptr;
}

void ScriptSelectScene::AddMenuItem(std::list<ref_count_ptr<ScriptSelectSceneMenuItem>>& listItem) {
	if (listItem.size() == 0) return;

	{
		Lock lock(cs_);

		for (auto itr = listItem.begin(); itr != listItem.end(); itr++) {
			MenuTask::AddMenuItem((*itr));
		}

		//現在選択中のアイテム
		ref_count_ptr<MenuItem> item = GetSelectedMenuItem();

		//ソート
		std::sort(item_.begin(), item_.end(), ScriptSelectScene::Sort());

		//現在選択中のアイテムに移動
		if (item) {
			bool bWait = false;
			std::wstring path = L"";
			ref_count_ptr<ScriptSelectFileModel> model = ref_count_ptr<ScriptSelectFileModel>::Cast(model_);
			if (model) {
				path = model->GetWaitPath();
				if (path.size() > 0)
					bWait = true;
			}

			if (path.size() == 0 && (pageCurrent_ > 1 || cursorY_ > 0)) {
				ScriptSelectSceneMenuItem* pItem = (ScriptSelectSceneMenuItem*)item.get();
				path = pItem->GetPath();
			}

			for (int iItem = 0; iItem < item_.size(); iItem++) {
				ScriptSelectSceneMenuItem* itrItem = (ScriptSelectSceneMenuItem*)item_[iItem].get();
				if (itrItem == nullptr) continue;

				bool bEqualsPath = File::IsEqualsPath(path, itrItem->GetPath());;
				if (!bEqualsPath) continue;

				pageCurrent_ = iItem / (pageMaxY_ + 1) + 1;
				cursorY_ = iItem % (pageMaxY_ + 1);

				if (bWait)
					model->SetWaitPath(L"");

				break;
			}
		}

		_ChangePage();
	}
}

//ScriptSelectSceneMenuItem
ScriptSelectSceneMenuItem::ScriptSelectSceneMenuItem(int type, const std::wstring& path, ref_count_ptr<ScriptInformation> info) {
	type_ = type;
	path_ = PathProperty::ReplaceYenToSlash(path);
	info_ = info;
}
ScriptSelectSceneMenuItem::~ScriptSelectSceneMenuItem() {}

//*******************************************************************
//ScriptSelectModel
//*******************************************************************
ScriptSelectModel::ScriptSelectModel() {
	bCreated_ = false;
}
ScriptSelectModel::~ScriptSelectModel() {}


//ScriptSelectFileModel
ScriptSelectFileModel::ScriptSelectFileModel(int type, const std::wstring& dir) {
	type_ = type;
	dir_ = dir;
}
ScriptSelectFileModel::~ScriptSelectFileModel() {

}
void ScriptSelectFileModel::_Run() {
	timeLastUpdate_ = ::timeGetTime() - 1000;
	_SearchScript(dir_);
	if (GetStatus() == RUN) {
		scene_->AddMenuItem(listItem_);
		listItem_.clear();
	}
	bCreated_ = true;
}
void ScriptSelectFileModel::_SearchScript(const std::wstring& dir) {
	WIN32_FIND_DATA data;
	HANDLE hFind;
	std::wstring findDir = dir + L"*.*";
	hFind = FindFirstFile(findDir.c_str(), &data);
	do {
		if (GetStatus() != RUN) return;

		int time = ::timeGetTime();
		if (abs(time - timeLastUpdate_) > 500) {
			//500ms毎に更新
			timeLastUpdate_ = time;
			scene_->AddMenuItem(listItem_);
			listItem_.clear();
		}

		std::wstring name = data.cFileName;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(name != L".." && name != L".")) {
			//ディレクトリ
			std::wstring tDir = dir + name;
			tDir += L"\\";

			if (type_ == TYPE_DIR) {
				//ディレクトリ
				ref_count_ptr<ScriptSelectSceneMenuItem> item = new ScriptSelectSceneMenuItem(
					ScriptSelectSceneMenuItem::TYPE_DIR, tDir, nullptr);
				listItem_.push_back(item);
			}
			else {
				_SearchScript(tDir);
			}
			continue;
		}
		if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0)
			continue;

		//ファイル
		std::wstring path = dir + name;

		_CreateMenuItem(path);

	} while (FindNextFile(hFind, &data));
	FindClose(hFind);
}
void ScriptSelectFileModel::_CreateMenuItem(const std::wstring& path) {
	std::vector<ref_count_ptr<ScriptInformation>> listInfo = ScriptInformation::CreateScriptInformationList(path, true);
	for (ref_count_ptr<ScriptInformation> info : listInfo) {
		if (!_IsValidScriptInformation(info)) continue;

		int typeItem = _ConvertTypeInfoToItem(info->GetType());
		listItem_.push_back(new ScriptSelectSceneMenuItem(typeItem, info->GetScriptPath(), info));
	}

}
bool ScriptSelectFileModel::_IsValidScriptInformation(ref_count_ptr<ScriptInformation> info) {
	int typeScript = info->GetType();
	bool bTarget = false;
	switch (type_) {
	case TYPE_SINGLE:
		bTarget = (typeScript == ScriptInformation::TYPE_SINGLE);
		break;
	case TYPE_PLURAL:
		bTarget = (typeScript == ScriptInformation::TYPE_PLURAL);
		break;
	case TYPE_STAGE:
		bTarget |= (typeScript == ScriptInformation::TYPE_STAGE);
		//bTarget |= (typeScript == ScriptInformation::TYPE_PACKAGE);
		break;
	case TYPE_PACKAGE:
		bTarget = (typeScript == ScriptInformation::TYPE_PACKAGE);
		break;
	case TYPE_DIR:
		bTarget = (typeScript != ScriptInformation::TYPE_PLAYER);
		break;
	case TYPE_ALL:
		bTarget = (typeScript != ScriptInformation::TYPE_PLAYER);
		break;
	}

	return bTarget;
}
int ScriptSelectFileModel::_ConvertTypeInfoToItem(int typeInfo) {
	int typeItem = ScriptSelectSceneMenuItem::TYPE_SINGLE;
	switch (typeInfo) {
	case ScriptInformation::TYPE_SINGLE:
		typeItem = ScriptSelectSceneMenuItem::TYPE_SINGLE;
		break;
	case ScriptInformation::TYPE_PLURAL:
		typeItem = ScriptSelectSceneMenuItem::TYPE_PLURAL;
		break;
	case ScriptInformation::TYPE_STAGE:
		typeItem = ScriptSelectSceneMenuItem::TYPE_STAGE;
		break;
	case ScriptInformation::TYPE_PACKAGE:
		typeItem = ScriptSelectSceneMenuItem::TYPE_PACKAGE;
		break;
	}
	return typeItem;
}
void ScriptSelectFileModel::CreateMenuItem() {
	bCreated_ = false;
	if (type_ == TYPE_DIR) {
		SystemController::GetInstance()->GetSystemInformation()->SetLastScriptSearchDirectory(dir_);
	}
	Start();
}

//*******************************************************************
//PlayTypeSelectScene
//*******************************************************************
PlayTypeSelectScene::PlayTypeSelectScene(ref_count_ptr<ScriptInformation> info) {
	pageMaxY_ = 10;
	bPageChangeX_ = true;

	info_ = info;
	int mx = 24;
	int my = 256;
	AddMenuItem(new PlayTypeSelectMenuItem(L"Play", mx, my));

	//リプレイ
	if (info->GetType() != ScriptInformation::TYPE_PACKAGE) {
		int itemCount = 1;
		const std::wstring& pathScript = info->GetScriptPath();
		replayInfoManager_ = new ReplayInformationManager();
		replayInfoManager_->UpdateInformationList(pathScript);
		std::vector<int> listReplayIndex = replayInfoManager_->GetIndexList();
		for (int iList = 0; iList < listReplayIndex.size(); iList++) {
			int index = listReplayIndex[iList];
			ref_count_ptr<ReplayInformation> replay = replayInfoManager_->GetInformation(index);
			int itemY = 256 + (itemCount % pageMaxY_) * 20;

			std::wstring text = StringUtility::Format(L"No.%02d %-8s %012I64d %-8s (%2.2ffps) <%s>",
				index,
				replay->GetUserName().c_str(),
				replay->GetTotalScore(),
				replay->GetPlayerScriptReplayName().c_str(),
				replay->GetAverageFps(),
				replay->GetDateAsString().c_str()
			);
			AddMenuItem(new PlayTypeSelectMenuItem(text, mx, itemY));
			itemCount++;
		}
	}

}
void PlayTypeSelectScene::Work() {
	MenuTask::Work();
	if (!_IsWaitedKeyFree()) return;

	EDirectInput* input = EDirectInput::GetInstance();
	if (input->GetVirtualKeyState(EDirectInput::KEY_OK) == KEY_PUSH) {
		if (info_->GetType() == ScriptInformation::TYPE_PACKAGE) {
			//パッケージモード
			SceneManager* sceneManager = SystemController::GetInstance()->GetSceneManager();
			sceneManager->TransPackageScene(info_);
		}
		else {
			//パッケージ以外
			int indexSelect = GetSelectedItemIndex();
			if (indexSelect == 0) {
				//自機選択
				TransitionManager* transitionManager = SystemController::GetInstance()->GetTransitionManager();
				transitionManager->DoFadeOut();

				ETaskManager* taskManager = ETaskManager::GetInstance();
				taskManager->SetRenderFunctionEnable(false, typeid(ScriptSelectScene));
				taskManager->SetWorkFunctionEnable(false, typeid(PlayTypeSelectScene));
				taskManager->SetRenderFunctionEnable(false, typeid(PlayTypeSelectScene));

				shared_ptr<PlayerSelectScene> task(new PlayerSelectScene(info_));
				taskManager->AddTask(task);
				taskManager->AddWorkFunction(TTaskFunction<PlayerSelectScene>::Create(task, 
					&PlayerSelectScene::Work), PlayerSelectScene::TASK_PRI_WORK);
				taskManager->AddRenderFunction(TTaskFunction<PlayerSelectScene>::Create(task, 
					&PlayerSelectScene::Render), PlayerSelectScene::TASK_PRI_RENDER);
			}
			else {
				//リプレイ
				std::vector<int> listReplayIndex = replayInfoManager_->GetIndexList();
				int replayIndex = listReplayIndex[indexSelect - 1];
				ref_count_ptr<ReplayInformation> replay = replayInfoManager_->GetInformation(replayIndex);

				SceneManager* sceneManager = SystemController::GetInstance()->GetSceneManager();
				sceneManager->TransStgScene(info_, replay);
			}
		}

		return;
	}
	else if (input->GetVirtualKeyState(EDirectInput::KEY_CANCEL) == KEY_PUSH) {
		ETaskManager* taskManager = ETaskManager::GetInstance();

		auto scriptSelectScene = std::dynamic_pointer_cast<ScriptSelectScene>(taskManager->GetTask(typeid(ScriptSelectScene)));
		scriptSelectScene->SetActive(true);

		taskManager->RemoveTask(typeid(PlayTypeSelectScene));
		return;
	}

}
void PlayTypeSelectScene::Render() {
	MenuTask::Render();
}

//PlayTypeSelectMenuItem
PlayTypeSelectMenuItem::PlayTypeSelectMenuItem(const std::wstring& text, int x, int y) {
    posRoot_.x = x;
    posRoot_.y = y;
	pos_.x = x;
	pos_.y = y;

	DxText dxText;
	dxText.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxText.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 64));
	dxText.SetFontBorderType(TextBorderType::Full);
	dxText.SetFontBorderColor(D3DCOLOR_ARGB(255, 32, 32, 128));
	dxText.SetFontBorderWidth(1);
	dxText.SetFontSize(14);
	dxText.SetFontWeight(FW_BOLD);
	dxText.SetText(text);
	dxText.SetSyntacticAnalysis(false);
	objText_ = dxText.CreateRenderObject();
}
PlayTypeSelectMenuItem::~PlayTypeSelectMenuItem() {}
void PlayTypeSelectMenuItem::Work() {
	_WorkSelectedItem();
}
void PlayTypeSelectMenuItem::Render() {
	objText_->SetPosition(pos_);
	objText_->Render();
    
    bool bSelected = menu_->GetSelectedMenuItem() == this;

    pos_.x = Math::Lerp::Linear(pos_.x, posRoot_.x + (bSelected ? 16 : 0), 0.25);

	if (bSelected) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		graphics->SetBlendMode(MODE_BLEND_ADD_RGB);

		int alpha = _GetSelectedItemAlpha();
		objText_->SetVertexColor(D3DCOLOR_ARGB(255, alpha, alpha, alpha));
		objText_->Render();
		objText_->SetVertexColor(D3DCOLOR_ARGB(255, 255, 255, 255));
		graphics->SetBlendMode(MODE_BLEND_ALPHA);
	}
}

//*******************************************************************
//PlayerSelectScene
//*******************************************************************
PlayerSelectScene::PlayerSelectScene(ref_count_ptr<ScriptInformation> info) {
	pageMaxY_ = 11;
	bPageChangeX_ = true;
	frameSelect_ = 0;

	info_ = info;

	std::wstring pathBack = EPathProperty::GetSystemImageDirectory() + L"System_ScriptSelect_Background.png";
	shared_ptr<Texture> textureBack(new Texture());
	textureBack->CreateFromFile(PathProperty::GetUnique(pathBack), false, true);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	int screenWidth = graphics->GetScreenWidth();
	int screenHeight = graphics->GetScreenHeight();

	DxRect<int> srcBack(0, 0, 640, 480);
	DxRect<double> destBack(0, 0, screenWidth, screenHeight);

	spriteBack_ = std::make_shared<Sprite2D>();
	spriteBack_->SetTexture(textureBack);
	spriteBack_->SetVertex(srcBack, destBack);

	spriteImage_ = std::make_shared<Sprite2D>();

	SystemInformation* systemInfo = SystemController::GetInstance()->GetSystemInformation();

	//自機一覧を作成
	std::vector<std::wstring>& listPlayerPath = info_->GetPlayerList();
	if (listPlayerPath.size() == 0) {
		listPlayer_ = systemInfo->GetFreePlayerScriptInformationList();
	}
	else {
		listPlayer_ = info_->CreatePlayerScriptInformationList();
	}

	//メニュー作成
	for (int iMenu = 0; iMenu < listPlayer_.size(); iMenu++) {
		AddMenuItem(new PlayerSelectMenuItem(listPlayer_[iMenu]));
	}

	std::vector<ref_count_ptr<ScriptInformation>> listLastPlayerSelect = systemInfo->GetLastPlayerSelectedList();
	bool bSameList = false;
	if (listPlayer_.size() == listLastPlayerSelect.size()) {
		bSameList = true;
		for (int iList = 0; iList < listPlayer_.size(); iList++) {
			bSameList &= listPlayer_[iList] == listLastPlayerSelect[iList];
		}
	}
	if (bSameList) {
		int lastIndex = systemInfo->GetLastSelectedPlayerIndex();
		cursorY_ = lastIndex % (pageMaxY_ + 1);
		pageCurrent_ = lastIndex / (pageMaxY_ + 1) + 1;
	}

}
void PlayerSelectScene::Work() {
	int lastCursorY = cursorY_;

	MenuTask::Work();
	if (!_IsWaitedKeyFree()) return;

	EDirectInput* input = EDirectInput::GetInstance();
	if (input->GetVirtualKeyState(EDirectInput::KEY_OK) == KEY_PUSH && listPlayer_.size() > 0) {
		int index = GetSelectedItemIndex();
		ref_count_ptr<ScriptInformation> infoPlayer = listPlayer_[index];
		SystemController::GetInstance()->GetSystemInformation()->SetLastSelectedPlayerIndex(index, listPlayer_);

		SceneManager* sceneManager = SystemController::GetInstance()->GetSceneManager();
		sceneManager->TransStgScene(info_, infoPlayer, nullptr);

		return;
	}
	else if (input->GetVirtualKeyState(EDirectInput::KEY_CANCEL) == KEY_PUSH) {
		ETaskManager* taskManager = ETaskManager::GetInstance();
		taskManager->SetRenderFunctionEnable(true, typeid(ScriptSelectScene));
		taskManager->SetWorkFunctionEnable(true, typeid(PlayTypeSelectScene));
		taskManager->SetRenderFunctionEnable(true, typeid(PlayTypeSelectScene));

		taskManager->RemoveTask(typeid(PlayerSelectScene));

		return;
	}

	if (lastCursorY != cursorY_) frameSelect_ = 0;
	else frameSelect_++;
}
void PlayerSelectScene::Render() {
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();
	graphics->SetRenderStateFor2D(MODE_BLEND_ALPHA);

	spriteBack_->Render();

	int top = (pageCurrent_ - 1) * (pageMaxY_ + 1);
	ref_count_ptr<ScriptInformation> infoSelected = nullptr;
	{
		Lock lock(cs_);
		for (int iItem = 0; iItem <= pageMaxY_; iItem++) {
			int index = top + iItem;
			if (index < item_.size() && item_[index] != nullptr) {
				PlayerSelectMenuItem* pItem = (PlayerSelectMenuItem*)item_[index].get();
				ref_count_ptr<ScriptInformation> info = pItem->GetScriptInformation();
				if (GetSelectedItemIndex() == index)
					infoSelected = info;
			}
		}
	}

	if (infoSelected) {
		DxText dxTextInfo;
		dxTextInfo.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
		dxTextInfo.SetFontBorderType(TextBorderType::None);
		dxTextInfo.SetFontWeight(FW_BOLD);
		//dxTextInfo.SetSyntacticAnalysis(false);

		//イメージ
		shared_ptr<Texture> texture = spriteImage_->GetTexture();
		std::wstring pathImage1 = L"";
		if (texture) pathImage1 = texture->GetName();
		const std::wstring& pathImage2 = infoSelected->GetImagePath();
		if (pathImage1 != pathImage2) {
			texture = std::make_shared<Texture>();
			File file(pathImage2);
			if (file.IsExists()) {
				texture->CreateFromFileInLoadThread(pathImage2, false, true);
				spriteImage_->SetTexture(texture);
			}
			else
				spriteImage_->SetTexture(nullptr);
		}

		texture = spriteImage_->GetTexture();
		if (texture != nullptr && texture->IsLoad()) {
			DxRect<int> rcSrc(0, 0, texture->GetWidth(), texture->GetHeight());
			DxRect<double> rcDest(0, 0, texture->GetWidth(), texture->GetHeight());
			spriteImage_->SetSourceRect(rcSrc);
			spriteImage_->SetDestinationRect(rcDest);
			spriteImage_->Render();
		}

		//スクリプトパス
		std::wstring path = infoSelected->GetScriptPath();
		std::wstring root = EPathProperty::GetStgScriptRootDirectory();
		root = PathProperty::ReplaceYenToSlash(root);
		path = PathProperty::ReplaceYenToSlash(path);
		path = StringUtility::ReplaceAll(path, root, L"");

		std::wstring archive = infoSelected->GetArchivePath();
		if (archive.size() > 0) {
			archive = PathProperty::ReplaceYenToSlash(archive);
			archive = StringUtility::ReplaceAll(archive, root, L"");
			path += StringUtility::Format(L" [%s]", archive.c_str());
		}
		dxTextInfo.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
		dxTextInfo.SetFontColorBottom(D3DCOLOR_ARGB(255, 255, 128, 128));
		dxTextInfo.SetFontSize(14);
		dxTextInfo.SetText(path);
		dxTextInfo.SetPosition(40, 32);
        
		dxTextInfo.Render();

		//テキスト
		dxTextInfo.SetFontBorderType(TextBorderType::Shadow);
		dxTextInfo.SetFontWeight(FW_BOLD);
		dxTextInfo.SetFontBorderWidth(1);
		dxTextInfo.SetFontSize(16);
		dxTextInfo.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
		dxTextInfo.SetFontColorTop(D3DCOLOR_ARGB(255, 160, 160, 160));
		dxTextInfo.SetFontColorBottom(D3DCOLOR_ARGB(255, 255, 64, 64));
		dxTextInfo.SetText(infoSelected->GetText());
        dxTextInfo.SetSidePitch(-1);
        dxTextInfo.SetLinePitch(1);
		dxTextInfo.SetPosition(320, 272);
        dxTextInfo.SetMaxWidth(320);
		dxTextInfo.Render();
	}

	std::wstring strDescription = StringUtility::Format(
		L"Player select (Page:%d/%d)", pageCurrent_, GetPageCount());

	DxText dxTextDescription;
	dxTextDescription.SetFontColorTop(D3DCOLOR_ARGB(255, 128, 128, 255));
	dxTextDescription.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 255));
	dxTextDescription.SetFontBorderType(TextBorderType::Full);
	dxTextDescription.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxTextDescription.SetFontBorderWidth(2);
	dxTextDescription.SetFontSize(20);
	dxTextDescription.SetFontWeight(FW_BOLD);
	dxTextDescription.SetText(strDescription);
	dxTextDescription.SetPosition(32, 8);
	dxTextDescription.SetSyntacticAnalysis(false);
	dxTextDescription.Render();

	{
		Lock lock(cs_);
		for (int iItem = 0; iItem <= pageMaxY_; iItem++) {
			int index = top + iItem;
			if (index < item_.size() && item_[index] != nullptr) {
				int mx = 320;
				int my = 48 + (iItem % (pageMaxY_ + 1)) * 17;

				PlayerSelectMenuItem* pItem = (PlayerSelectMenuItem*)item_[index].get();
				ref_count_ptr<ScriptInformation> info = pItem->GetScriptInformation();

				DxText dxText;
				dxText.SetPosition(mx, my);
				dxText.SetFontBorderType(TextBorderType::Full);
				dxText.SetFontBorderWidth(2);
				dxText.SetFontSize(16);
				dxText.SetFontWeight(FW_BOLD);
				dxText.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
				dxText.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 255));
				dxText.SetFontBorderColor(D3DCOLOR_ARGB(255, 32, 32, 128));
				dxText.SetText(info->GetTitle());
				//dxText.SetSyntacticAnalysis(false);
				dxText.Render();

				if (GetSelectedItemIndex() == index) {
					graphics->SetBlendMode(MODE_BLEND_ADD_RGB);
					int cycle = 60;
					int alpha = frameSelect_ % cycle;
					if (alpha < cycle / 2) alpha = 255 * (float)((float)(cycle / 2 - alpha) / (float)(cycle / 2));
					else alpha = 255 * (float)((float)(alpha - cycle / 2) / (float)(cycle / 2));
					dxText.SetVertexColor(D3DCOLOR_ARGB(255, alpha, alpha, alpha));
					dxText.Render();
					graphics->SetBlendMode(MODE_BLEND_ALPHA);
				}
			}
		}
	}
}

//PlayerSelectMenuItem
PlayerSelectMenuItem::PlayerSelectMenuItem(ref_count_ptr<ScriptInformation> info) {
	info_ = info;
}
PlayerSelectMenuItem::~PlayerSelectMenuItem() {}
void PlayerSelectMenuItem::Work() {
	_WorkSelectedItem();
}
void PlayerSelectMenuItem::Render() {

}
