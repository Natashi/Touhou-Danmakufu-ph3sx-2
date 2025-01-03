#include "source/GcLib/pch.h"

#include "TitleScene.hpp"
#include "System.hpp"

//*******************************************************************
//TitleScene
//*******************************************************************
TitleScene::TitleScene() {
	pageMaxY_ = ITEM_COUNT - 1;

	std::wstring pathBack = EPathProperty::GetSystemImageDirectory() + L"System_Title_Background.png";
	shared_ptr<Texture> textureBack(new Texture());
	textureBack->CreateFromFile(PathProperty::GetUnique(pathBack), false, true);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	int screenWidth = graphics->GetScreenWidth();
	int screenHeight = graphics->GetScreenHeight();

	DxRect<int> srcBack(0, 0, 640, 480);
	DxRect<double> destBack(0, 0, screenWidth, screenHeight);

	spriteBack_ = make_shared<Sprite2D>();
	spriteBack_->SetTexture(textureBack);
	spriteBack_->SetVertex(srcBack, destBack);

	std::wstring strText[] = { L"All", L"Single", L"Plural", L"Stage", L"Package", L"Directory", L"Quit" };
	std::wstring strDescription[] = {
		L"",
		L"",
		L"",
		L"",
		L"",
		L"",
		L"",
		L""
	};
	for (int iItem = 0; iItem < ITEM_COUNT; iItem++) {
		int x = 48 + iItem * 6 + 12 * pow((double)-1, (int)(iItem - 1));
		int y = 154 + iItem * 30;
		AddMenuItem(make_unique<TitleSceneMenuItem>(strText[iItem], strDescription[iItem], x, y));
	}

	cursorY_ = SystemController::GetInstance()->GetSystemInformation()->GetLastTitleSelectedIndex();
}
void TitleScene::Work() {
	MenuTask::Work();
	if (!_IsWaitedKeyFree()) return;

	EDirectInput* input = EDirectInput::GetInstance();
	if (input->GetVirtualKeyState(EDirectInput::KEY_OK) == KEY_PUSH) {
		SceneManager* sceneManager = SystemController::GetInstance()->GetSceneManager();

		//選択インデックス保存
		SystemController::GetInstance()->GetSystemInformation()->SetLastTitleSelectedIndex(cursorY_);

		switch (cursorY_) {
		case ITEM_ALL:
			sceneManager->TransScriptSelectScene_All();
			break;
		case ITEM_SINGLE:
			sceneManager->TransScriptSelectScene_Single();
			break;
		case ITEM_PLURAL:
			sceneManager->TransScriptSelectScene_Plural();
			break;
		case ITEM_STAGE:
			sceneManager->TransScriptSelectScene_Stage();
			break;
		case ITEM_PACKAGE:
			sceneManager->TransScriptSelectScene_Package();
			break;
		case ITEM_DIRECTORY:
			sceneManager->TransScriptSelectScene_Directory();
			break;
		case ITEM_QUIT:
			EApplication::GetInstance()->End();
			break;
		}

		return;
	}
	else if (input->GetVirtualKeyState(EDirectInput::KEY_CANCEL) == KEY_PUSH) {
		cursorY_ = ITEM_QUIT;
	}
}
void TitleScene::Render() {
	EDirectGraphics* graphics = EDirectGraphics::GetInstance();
	graphics->SetRenderStateFor2D(MODE_BLEND_ALPHA);

	spriteBack_->Render();

	MenuTask::Render();
}

//TitleSceneMenuItem
TitleSceneMenuItem::TitleSceneMenuItem(std::wstring text, std::wstring description, int x, int y) {
	pos_.x = x;
	pos_.y = y;

	DxText dxText;
	dxText.SetFontColorTop(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxText.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 64));
	dxText.SetFontBorderType(TextBorderType::Full);
	dxText.SetFontBorderColor(D3DCOLOR_ARGB(255, 32, 32, 128));
	dxText.SetFontBorderWidth(2);
	dxText.SetFontSize(24);
	dxText.SetFontWeight(FW_BOLD);
	dxText.SetText(text);
	objText_ = dxText.CreateRenderObject();
}
TitleSceneMenuItem::~TitleSceneMenuItem() {}
void TitleSceneMenuItem::Work() {
	_WorkSelectedItem();
}
void TitleSceneMenuItem::Render() {
	objText_->SetPosition(pos_);
	objText_->Render();

	if (menu_->GetSelectedMenuItem() == this) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		graphics->SetBlendMode(MODE_BLEND_ADD_RGB);

		int alpha = _GetSelectedItemAlpha();
		objText_->SetVertexColor(D3DCOLOR_ARGB(255, alpha, alpha, alpha));
		objText_->Render();
		objText_->SetVertexColor(D3DCOLOR_ARGB(255, 255, 255, 255));
		graphics->SetBlendMode(MODE_BLEND_ALPHA);
	}
}
