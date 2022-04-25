#include "source/GcLib/pch.h"

#include "StgSystem.hpp"

#include "StgPackageController.hpp"
#include "StgStageScript.hpp"
#include "StgPlayer.hpp"

#include "../DnhExecutor/GcLibImpl.hpp"

//****************************************************************************
//StgSystemController
//****************************************************************************
StgSystemController* StgSystemController::base_ = nullptr;
StgSystemController::StgSystemController() {
	stageController_ = nullptr;
	packageController_ = nullptr;
	bPrevWindowFocused_ = true;
}
StgSystemController::~StgSystemController() {
	_ResetSystem();
}
void StgSystemController::_ResetSystem() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->ResetCamera();
	graphics->ResetDeviceState();
	graphics->ResetDisplaySettings();

	ScriptClientBase::randCalls_ = 0;
	ScriptClientBase::prandCalls_ = 0;
	if (scriptEngineCache_)
		scriptEngineCache_->Clear();

	if (DxScriptResourceCache* dxRsrcCache = DxScriptResourceCache::GetBase())
		dxRsrcCache->ClearResource();

	DirectSoundManager* soundManager = DirectSoundManager::GetBase();
	soundManager->Clear();

	EFpsController* fpsController = EFpsController::GetInstance();
	fpsController->SetFastModeKey(DIK_LCONTROL);

	EDirectInput* input = EDirectInput::GetInstance();
	input->ResetVirtualKeyMap();

	EFileManager* fileManager = EFileManager::GetInstance();
	fileManager->ClearArchiveFileCache();
}
void StgSystemController::Initialize(ref_count_ptr<StgSystemInformation> infoSystem) {
	base_ = this;

	infoSystem_ = infoSystem;

	scriptEngineCache_.reset(new ScriptEngineCache());
	commonDataManager_.reset(new ScriptCommonDataManager());
	infoControlScript_ = new StgControlScriptInformation();
}
void StgSystemController::Start(ref_count_ptr<ScriptInformation> infoPlayer, ref_count_ptr<ReplayInformation> infoReplay) {
	_ResetSystem();

	ref_count_ptr<ScriptInformation> infoMain = infoSystem_->GetMainScriptInformation();

	EFileManager* fileManager = EFileManager::GetInstance();
	const std::wstring& archiveMain = infoMain->pathArchive_;
	if (archiveMain.size() > 0) {
		fileManager->AddArchiveFile(archiveMain, 0);
	}

	if (infoPlayer) {
		const std::wstring& archivePlayer = infoPlayer->pathArchive_;
		if (archivePlayer.size() > 0) {
			fileManager->AddArchiveFile(archivePlayer, 0);
		}
	}

	if (infoSystem_->IsPackageMode()) {
		infoSystem_->SetScene(StgSystemInformation::SCENE_PACKAGE_CONTROL);
		packageController_ = std::make_shared<StgPackageController>(this);
		packageController_->Initialize();
	}
	else {
		ref_count_ptr<ReplayInformation::StageData> replayStageData = nullptr;
		if (infoReplay)
			replayStageData = infoReplay->GetStageData(0);
		ref_count_ptr<StgStageInformation> infoStage = new StgStageInformation();
		infoStage->SetMainScriptInformation(infoMain);
		infoStage->SetPlayerScriptInformation(infoPlayer);
		StartStgScene(infoStage, replayStageData);
	}
}
void StgSystemController::Work() {
	try {
		_ControlScene();

		ELogger* logger = ELogger::GetInstance();
		logger->UpdateCommonDataInfoPanel();
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());
		infoSystem_->SetError(e.what());
	}

	EDirectInput* input = EDirectInput::GetInstance();
	if (infoSystem_->IsRetry()) {
		infoSystem_->SetError(L"Retry");

		DirectSoundManager* soundManager = DirectSoundManager::GetBase();
		soundManager->Clear();

		if (infoSystem_->IsPackageMode()) {
			ref_count_ptr<StgStageStartData> oldStageStartData;
			ref_count_ptr<StgPackageInformation> infoPackage = packageController_->GetPackageInformation();
			std::vector<ref_count_ptr<StgStageStartData>> listStageData = infoPackage->GetStageDataList();
			if (listStageData.size() > 0) {
				oldStageStartData = *listStageData.begin();
			}
			else {
				oldStageStartData = infoPackage->GetNextStageData();
			}
			ref_count_ptr<StgStageInformation> oldStageInformation = oldStageStartData->GetStageInformation();

			ref_count_ptr<StgStageStartData> newStageStartData = new StgStageStartData();
			ref_count_ptr<StgStageInformation> newStageInformaiton = new StgStageInformation();
			newStageInformaiton->SetMainScriptInformation(oldStageInformation->GetMainScriptInformation());
			newStageInformaiton->SetPlayerScriptInformation(oldStageInformation->GetPlayerScriptInformation());
			newStageInformaiton->SetStageIndex(oldStageInformation->GetStageIndex());
			newStageStartData->SetStageInformation(newStageInformaiton);
			infoPackage->SetNextStageData(newStageStartData);
			infoSystem_->ResetRetry();

			StartStgScene(newStageStartData);
		}
		else {
			DoRetry();
		}
		return;
	}

	if (infoSystem_->IsError() || infoSystem_->IsStgEnd()) {
		bool bRetry = false;
		if (infoSystem_->IsError()) {
			std::wstring error = infoSystem_->GetErrorMessage();
			if (error.size() > 0) {
				ErrorDialog::ShowErrorDialog(error);
			}
			else {
				bRetry = true;
			}
		}

		if (!bRetry) {
			DirectSoundManager* soundManager = DirectSoundManager::GetBase();
			soundManager->Clear();
		}

		ELogger* logger = ELogger::GetInstance();
		logger->UpdateCommonDataInfoPanel();

		DoEnd();
		return;
	}
}
void StgSystemController::Render() {
	if (infoSystem_->IsError()) return;

	try {
		int scene = infoSystem_->GetScene();
		switch (scene) {
		case StgSystemInformation::SCENE_STG:
		case StgSystemInformation::SCENE_PACKAGE_CONTROL:
		{
			RenderScriptObject();
			break;
		}
		case StgSystemInformation::SCENE_END:
		{
			if (endScene_)
				endScene_->Render();
			break;
		}
		case StgSystemInformation::SCENE_REPLAY_SAVE:
		{
			if (replaySaveScene_)
				replaySaveScene_->Render();
			break;
		}
		}
	}
	catch (gstd::wexception& e) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
		camera2D->SetEnable(false);
		camera2D->Reset();
		Logger::WriteTop(e.what());
		infoSystem_->SetError(e.what());
	}
}
void StgSystemController::RenderScriptObject() {
	int scene = infoSystem_->GetScene();
	bool bPackageMode = infoSystem_->IsPackageMode();
	bool bPause = false;
	if (scene == StgSystemInformation::SCENE_STG) {
		ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
		bPause = infoStage->IsPause();
	}

	if (bPause && !bPackageMode) {
		stageController_->GetPauseManager()->Render();
	}
	else {
		bool bReplay = false;
		size_t countRender = 0;
		if (scene == StgSystemInformation::SCENE_STG && stageController_ != nullptr) {
			StgStageScriptObjectManager* objectManagerStage = stageController_->GetMainObjectManager();
			countRender = std::max(objectManagerStage->GetRenderBucketCapacity() - 1, countRender);

			ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
			bReplay = infoStage->IsReplay();
		}

		if (infoSystem_->IsPackageMode()) {
			DxScriptObjectManager* objectManagerPackage = packageController_->GetMainObjectManager();
			countRender = std::max(objectManagerPackage->GetRenderBucketCapacity() - 1, countRender);
		}

		int invalidPriMin = infoSystem_->GetInvalidRenderPriorityMin();
		int invalidPriMax = infoSystem_->GetInvalidRenderPriorityMax();
		if (invalidPriMin < 0 && invalidPriMax < 0) {
			RenderScriptObject(0, countRender);
		}
		else {
			RenderScriptObject(0, invalidPriMin - 1);
			RenderScriptObject(invalidPriMax + 1, countRender);
		}


		if (bReplay) {
/*
			ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
			ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage->GetReplayData();
			DirectGraphics* graphics = DirectGraphics::GetBase();
			graphics->SetBlendMode(DirectGraphics::MODE_BLEND_ALPHA);
			graphics->SetZBufferEnable(false);
			graphics->SetZWriteEnable(false);
			graphics->SetFogEnable(false);

			RECT rest = infoStage->GetStgFrameRect();
			int frame = infoStage->GetCurrentFrame();
			int fps = replayStageData->GetFramePerSecond(frame);
			std::string strFps = StringUtility::Format("%02d", fps);
			DxText dxText;
			dxText.SetFontColorTop(D3DCOLOR_ARGB(255,128,128,255));
			dxText.SetFontColorBottom(D3DCOLOR_ARGB(255,64,64,255));
			dxText.SetFontBorderType(directx::DxFont::BORDER_FULL);
			dxText.SetFontBorderColor(D3DCOLOR_ARGB(255,255,255,255));
			dxText.SetFontBorderWidth(1);
			dxText.SetFontSize(12);
			dxText.SetFontBold(true);
			dxText.SetText(strFps);
			dxText.SetPosition(rest.right - 18, rest.bottom - 14);
			dxText.Render();
*/
		}
	}

}
void StgSystemController::RenderScriptObject(int priMin, int priMax) {
	StgStageScriptObjectManager* objManagerStage = nullptr;
	DxScriptObjectManager* objManagerPackage = nullptr;

	std::vector<DxScriptObjectManager::RenderList>* pRenderListStage = nullptr;
	std::vector<DxScriptObjectManager::RenderList>* pRenderListPackage = nullptr;

	int scene = infoSystem_->GetScene();
	bool bPause = false;
	if (scene == StgSystemInformation::SCENE_STG) {
		ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
		bPause = infoStage->IsPause();
	}

	bool bValidStage = (scene == StgSystemInformation::SCENE_STG || !infoSystem_->IsPackageMode()) &&
		stageController_ != nullptr && !bPause;
	if (bValidStage) {
		objManagerStage = stageController_->GetMainObjectManager();
		objManagerStage->PrepareRenderObject();
		pRenderListStage = objManagerStage->GetRenderObjectListPointer();
	}
	if (infoSystem_->IsPackageMode()) {
		objManagerPackage = packageController_->GetMainObjectManager();
		objManagerPackage->PrepareRenderObject();
		pRenderListPackage = objManagerPackage->GetRenderObjectListPointer();
	}

	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
	double focusRatioX = camera2D->GetRatioX();
	double focusRatioY = camera2D->GetRatioY();
	double focusAngleZ = camera2D->GetAngleZ();
	D3DXVECTOR2 orgFocusPos = camera2D->GetFocusPosition();
	D3DXVECTOR2 focusPos = orgFocusPos;

	ref_count_ptr<StgStageInformation> stageInfo = nullptr;
	if (bValidStage) {
		stageInfo = stageController_->GetStageInformation();
		DxRect<LONG>* rcStgFrame = stageInfo->GetStgFrameRect();

		ref_count_ptr<D3DXVECTOR2> pos = new D3DXVECTOR2;
		pos->x = (rcStgFrame->right - rcStgFrame->left) / 2.0f;
		pos->y = (rcStgFrame->bottom - rcStgFrame->top) / 2.0f;
		camera2D->SetResetFocus(pos);

		orgFocusPos = camera2D->GetFocusPosition();
		focusPos = orgFocusPos;
	}
	else {
		stageInfo = new StgStageInformation();

		DxRect<LONG> rect;
		rect.right = graphics->GetScreenWidth();
		rect.bottom = graphics->GetScreenHeight();

		stageInfo->SetStgFrameRect(rect);
		if (scene != StgSystemInformation::SCENE_STG) {
			orgFocusPos = camera2D->GetFocusPosition();
			focusPos = orgFocusPos;
		}
	}

	DxRect<LONG>* rcStgFrame = stageInfo->GetStgFrameRect();

	LONG stgWidth = rcStgFrame->right - rcStgFrame->left;
	LONG stgHeight = rcStgFrame->bottom - rcStgFrame->top;
	POINT stgCenter = { rcStgFrame->left + stgWidth / 2, rcStgFrame->top + stgHeight / 2 };

	int priMinStgFrame = stageInfo->GetStgFrameMinPriority();
	int priMaxStgFrame = stageInfo->GetStgFrameMaxPriority();
	int priShot = stageInfo->GetShotObjectPriority();
	int priItem = stageInfo->GetItemObjectPriority();
	int priCamera = stageInfo->GetCameraFocusPermitPriority();
	int invalidPriMin = infoSystem_->GetInvalidRenderPriorityMin();
	int invalidPriMax = infoSystem_->GetInvalidRenderPriorityMax();

	focusPos.x -= stgWidth / 2;
	focusPos.y -= stgHeight / 2;

	{
		DxScriptObjectManager::FogData* pFog = DxScriptObjectManager::GetFogData();
		graphics->SetVertexFog(pFog->enable, pFog->color, pFog->start, pFog->end);
	}

	{
		camera2D->SetEnable(false);
		camera2D->Reset();

		camera2D->SetRatioX(focusRatioX);
		camera2D->SetRatioY(focusRatioY);
		camera2D->SetAngleZ(focusAngleZ);
		camera2D->SetClip(*rcStgFrame);
		camera2D->SetFocus(stgCenter.x + focusPos.x, stgCenter.y + focusPos.y);
		camera2D->UpdateMatrix();

		camera3D->SetPerspectiveWidth(graphics->GetScreenWidth());
		camera3D->SetPerspectiveHeight(graphics->GetScreenHeight());
		camera3D->SetProjectionMatrix();
		camera3D->UpdateDeviceViewProjectionMatrix();

		graphics->ResetViewPort();

		camera3D->PushMatrixState();
	}

	bool bClearZBufferFor2DCoordinate = false;
	bool bRunMinStgFrame = false;
	bool bRunMaxStgFrame = false;

	if (bValidStage) {
		stageController_->GetItemManager()->LoadRenderQueue();
		stageController_->GetShotManager()->LoadRenderQueue();
	}

	for (int iPri = priMin; iPri <= priMax; iPri++) {
		if (!bRunMinStgFrame && iPri >= priMinStgFrame) {
			if (bValidStage && iPri < invalidPriMin)
				graphics->ClearRenderTarget(rcStgFrame);

			double clipNear = camera3D->GetNearClip();
			double clipFar = camera3D->GetFarClip();

			camera2D->SetEnable(true);

			camera3D->SetPerspectiveWidth(stgWidth);
			camera3D->SetPerspectiveHeight(stgHeight);
			camera3D->SetProjectionMatrix();
			camera3D->UpdateDeviceViewProjectionMatrix();
			graphics->SetViewPort(rcStgFrame->left, rcStgFrame->top, stgWidth, stgHeight);
			//graphics->SetViewPortMatrix(rcStgFrame->left, rcStgFrame->top, stgWidth, stgHeight);

			bRunMinStgFrame = true;
			bClearZBufferFor2DCoordinate = false;
		}

		if (objManagerStage != nullptr && !bPause) {
			ID3DXEffect* effect = nullptr;
			UINT cPass = 1;
			if (shared_ptr<Shader> shader = objManagerStage->GetShader(iPri)) {
				effect = shader->GetEffect();

				if (shader->LoadTechnique())
					shader->LoadParameter();
				effect->Begin(&cPass, 0);
			}

			DxScriptObjectManager::RenderList& renderList = pRenderListStage->at(iPri);

			for (UINT iPass = 0; iPass < cPass; ++iPass) {
				if (effect) effect->BeginPass(iPass);

				if (bValidStage) {
					stageController_->GetItemManager()->Render(iPri);
					stageController_->GetShotManager()->Render(iPri);
				}
				if (pRenderListStage != nullptr && iPri < pRenderListStage->size()) {
					for (auto itr = renderList.begin(); itr != renderList.end(); ++itr) {
						if (DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(itr->get())) {
							if (!bClearZBufferFor2DCoordinate)
								bClearZBufferFor2DCoordinate = CheckMeshAndClearZBuffer(obj);
							obj->Render();
						}
					}
					//renderList.clear();
				}

				if (effect) effect->EndPass();
			}
			renderList.Clear();

			if (effect) effect->End();

			//Intersection visualizer
			{
				StgIntersectionManager* itscMgr = stageController_->GetIntersectionManager();
				if (iPri == itscMgr->GetVisualizerRenderPriority() && graphics->IsMainRenderLoop()) {
					itscMgr->RenderVisualizer();
				}
			}
		}

		if (objManagerPackage) {
			ID3DXEffect* effect = nullptr;
			UINT cPass = 1;
			if (shared_ptr<Shader> shader = objManagerPackage->GetShader(iPri)) {
				effect = shader->GetEffect();

				if (shader->LoadTechnique())
					shader->LoadParameter();
				effect->Begin(&cPass, 0);
			}

			DxScriptObjectManager::RenderList& renderList = pRenderListPackage->at(iPri);

			for (UINT iPass = 0; iPass < cPass; ++iPass) {
				if (effect) effect->BeginPass(iPass);

				if (pRenderListPackage != nullptr && iPri < pRenderListPackage->size()) {
					for (auto itr = renderList.begin(); itr != renderList.end(); ++itr) {
						if (DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(itr->get())) {
							if (!bClearZBufferFor2DCoordinate)
								bClearZBufferFor2DCoordinate = CheckMeshAndClearZBuffer(obj);
							obj->Render();
						}
					}
					//renderList.clear();
				}

				if (effect) effect->EndPass();
			}
			renderList.Clear();

			if (effect) effect->End();
		}

		if (iPri == priCamera) {
			camera2D->SetFocus(stgCenter.x, stgCenter.y);
			camera2D->SetRatio(1);
			camera2D->SetAngleZ(0);
			//camera2D->SetEnable(false);
			camera2D->UpdateMatrix();
		}
		if (!bRunMaxStgFrame && iPri >= priMaxStgFrame) {
			camera2D->SetEnable(false);
			camera2D->Reset();

			camera3D->PopMatrixState();
			graphics->ResetViewPort();

			bRunMaxStgFrame = true;
			bClearZBufferFor2DCoordinate = false;
		}
	}
	camera2D->SetFocus(orgFocusPos);
	camera2D->SetRatioX(focusRatioX);
	camera2D->SetRatioY(focusRatioY);
	camera2D->SetAngleZ(focusAngleZ);

	camera3D->PopMatrixState();		//Just in case

	//--------------------------------

	if (objManagerStage)
		objManagerStage->ClearRenderObject();
	if (objManagerPackage)
		objManagerPackage->ClearRenderObject();
}
bool StgSystemController::CheckMeshAndClearZBuffer(DxScriptRenderObject* obj) {
	if (obj == nullptr) return false;
	if (DxScriptMeshObject* objMesh = dynamic_cast<DxScriptMeshObject*>(obj)) {
		if (DxMesh* mesh = objMesh->GetMesh().get()) {
			if (mesh->IsCoordinate2D()) {
				DirectGraphics::GetBase()->GetDevice()->Clear(0, nullptr,
					D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0, 0);
				return true;
			}
		}
	}
	return false;
}

void StgSystemController::_ControlScene() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	if (config->bEnableUnfocusedProcessing_) {
		bool bNowWindowFocused = EApplication::GetInstance()->IsWindowFocused();

		if (bPrevWindowFocused_ != bNowWindowFocused) {
			ScriptManager* scriptManager = nullptr;
			if (packageController_)
				scriptManager = packageController_->GetScriptManager();
			else scriptManager = stageController_->GetScriptManager();

			if (!bNowWindowFocused) {
				//Focus lost
				scriptManager->RequestEventAll(StgStageScript::EV_APP_LOSE_FOCUS);
			}
			else {
				//Focus restored
				scriptManager->RequestEventAll(StgStageScript::EV_APP_RESTORE_FOCUS);
			}
		}

		bPrevWindowFocused_ = bNowWindowFocused;
	}

	if (infoSystem_->IsPackageMode()) {
		packageController_->Work();

		ref_count_ptr<StgPackageInformation> infoPackage = packageController_->GetPackageInformation();
		if (infoPackage->IsEnd()) {
			infoSystem_->SetStgEnd();
		}
	}

	int scene = infoSystem_->GetScene();
	switch (scene) {
	case StgSystemInformation::SCENE_STG:
	{
		ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
		if (!infoStage->IsEnd())
			stageController_->Work();

		if (infoStage->IsEnd()) {
			stageController_->CloseScene();
			if (infoSystem_->IsPackageMode()) {
				stageController_->RenderToTransitionTexture();
				if (infoStage->GetResult() == StgStageInformation::RESULT_UNKNOWN) {
					int sceneResult = StgStageInformation::RESULT_CLEARED;
					ref_unsync_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
					if (objPlayer) {
						int statePlayer = objPlayer->GetState();
						if (statePlayer == StgPlayerObject::STATE_END)
							sceneResult = StgStageInformation::RESULT_PLAYER_DOWN;
					}
					infoStage->SetResult(sceneResult);
				}
				infoSystem_->SetScene(StgSystemInformation::SCENE_PACKAGE_CONTROL);

				ref_count_ptr<StgPackageInformation> infoPackage = packageController_->GetPackageInformation();
				infoPackage->FinishCurrentStage();

				if (StgStageScriptManager* scriptManager = stageController_->GetScriptManager()) {
					scriptManager->CloseScriptOnType(StgStageScript::TYPE_STAGE);
					scriptManager->CloseScriptOnType(StgStageScript::TYPE_ITEM);
					scriptManager->CloseScriptOnType(StgStageScript::TYPE_SHOT);
					scriptManager->CloseScriptOnType(StgStageScript::TYPE_PLAYER);
					scriptManager->CloseScriptOnType(StgStageScript::TYPE_SYSTEM);
					scriptManager->OrphanAllScripts();
				}
			}
			else
				TransStgEndScene();
		}
		break;
	}
	case StgSystemInformation::SCENE_END:
		endScene_->Work();
		break;
	case StgSystemInformation::SCENE_REPLAY_SAVE:
		replaySaveScene_->Work();
		break;
	}

	if (infoSystem_->IsPackageMode()) {
		//シーン変化時には即座にパッケージ管理機能を実行する
		//パッケージスクリプト内で起動するシーン遷移の描画などが追いつかなくなるため
		if (scene != infoSystem_->GetScene()) {
			packageController_->Work();
		}
	}

	ELogger* logger = ELogger::GetInstance();
	if (logger->IsWindowVisible()) {
		size_t taskCount = 0;
		size_t objectCount = 0;
		if (packageController_) {
			StgControlScriptManager* scriptManager = packageController_->GetScriptManager();
			if (scriptManager)
				taskCount = scriptManager->GetAllScriptThreadCount();

			DxScriptObjectManager* objectManager = packageController_->GetMainObjectManager();
			if (objectManager)
				objectCount += objectManager->GetAliveObjectCount();
		}
		if (stageController_) {
			ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
			if (!infoStage->IsEnd()) {
				StgStageScriptManager* scriptManager = stageController_->GetScriptManager();
				if (scriptManager)
					taskCount = scriptManager->GetAllScriptThreadCount();

				StgStageScriptObjectManager* objectManager = stageController_->GetMainObjectManager();
				if (objectManager)
					objectCount += objectManager->GetAliveObjectCount();
			}
		}
		logger->SetInfo(4, L"Task count", StringUtility::Format(L"%u", taskCount));
		logger->SetInfo(5, L"Object count", StringUtility::Format(L"%u", objectCount));
	}
}

void StgSystemController::StartStgScene(ref_count_ptr<StgStageInformation> infoStage, ref_count_ptr<ReplayInformation::StageData> replayStageData) {
	ref_count_ptr<StgStageStartData> startData = new StgStageStartData();
	startData->SetStageInformation(infoStage);
	startData->SetStageReplayData(replayStageData);
	StartStgScene(startData);
}
void StgSystemController::StartStgScene(ref_count_ptr<StgStageStartData> startData) {
	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	infoSystem_->SetScene(StgSystemInformation::SCENE_STG);

	stageController_ = std::make_shared<StgStageController>(this);

	stageController_->Initialize(startData);
}
void StgSystemController::TransStgEndScene() {
	bool bReplay = false;
	if (stageController_) {
		ref_count_ptr<StgStageInformation> infoStage = stageController_->GetStageInformation();
		bReplay = infoStage->IsReplay();
	}

	if (!bReplay) {
		ref_count_ptr<ReplayInformation> infoReplay = CreateReplayInformation();
		infoSystem_->SetActiveReplayInformation(infoReplay);
		endScene_ = std::make_shared<StgEndScene>(this);
		endScene_->Start();
		infoSystem_->SetScene(StgSystemInformation::SCENE_END);
	}
	else {
		infoSystem_->SetStgEnd();
	}
}

void StgSystemController::TransReplaySaveScene() {
	replaySaveScene_ = std::make_shared<StgReplaySaveScene>(this);
	replaySaveScene_->Start();
	infoSystem_->SetScene(StgSystemInformation::SCENE_REPLAY_SAVE);
}

ref_count_ptr<ReplayInformation> StgSystemController::CreateReplayInformation() {
	ref_count_ptr<ReplayInformation> res = new ReplayInformation();

	//メインスクリプト関連
	ref_count_ptr<StgStageInformation> infoLastStage = stageController_->GetStageInformation();
	ref_count_ptr<ScriptInformation> infoMain = infoSystem_->GetMainScriptInformation();
	const std::wstring& pathMainScript = infoMain->pathScript_;
	std::wstring nameMainScript = PathProperty::GetFileName(pathMainScript);

	//自機関連
	ref_count_ptr<ScriptInformation> infoPlayer = infoLastStage->GetPlayerScriptInformation();
	const std::wstring& pathPlayerScript = infoPlayer->pathScript_;
	std::wstring filenamePlayerScript = PathProperty::GetFileName(pathPlayerScript);
	res->SetPlayerScriptFileName(filenamePlayerScript);
	res->SetPlayerScriptID(infoPlayer->id_);
	res->SetPlayerScriptReplayName(infoPlayer->replayName_);

	//システム関連
	int64_t totalScore = infoLastStage->GetScore();
	double fpsAverage = 0;

	//ステージ
	if (infoSystem_->IsPackageMode()) {
		ref_count_ptr<StgPackageInformation> infoPackage = packageController_->GetPackageInformation();
		std::vector<ref_count_ptr<StgStageStartData>>& listStageData = infoPackage->GetStageDataList();
		for (size_t iStage = 0; iStage < listStageData.size(); iStage++) {
			ref_count_ptr<StgStageStartData> stageData = listStageData[iStage];
			ref_count_ptr<StgStageInformation> infoStage = stageData->GetStageInformation();
			ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage->GetReplayData();
			res->SetStageData(infoStage->GetStageIndex(), replayStageData);

			fpsAverage += replayStageData->GetFramePerSecondAverage();
		}
		if (listStageData.size() > 0)
			fpsAverage = fpsAverage / listStageData.size();
	}
	else {
		ref_count_ptr<ReplayInformation::StageData> replayStageData = infoLastStage->GetReplayData();
		res->SetStageData(0, replayStageData);
		fpsAverage = replayStageData->GetFramePerSecondAverage();
	}

	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	res->SetDate(sysTime);
	res->SetTotalScore(totalScore);
	res->SetAverageFps(fpsAverage);

	return res;
}

void StgSystemController::TerminateScriptAll() {
	std::wstring error = L"Forced termination.";
	if (packageController_) {
		ScriptManager* scriptManager = packageController_->GetScriptManager();
		if (scriptManager)
			scriptManager->TerminateScriptAll(error);
	}

	if (stageController_) {
		auto scriptManager = stageController_->GetScriptManager();
		if (scriptManager)
			scriptManager->TerminateScriptAll(error);

		ref_count_ptr<StgPauseScene> pauseScene = stageController_->GetPauseManager();
		if (pauseScene) {
			ScriptManager* pauseScriptManager = pauseScene->GetScriptManager();
			if (pauseScriptManager)
				pauseScriptManager->TerminateScriptAll(error);
		}
	}

	if (endScene_) {
		ScriptManager* scriptManager = endScene_->GetScriptManager();
		if (scriptManager)
			scriptManager->TerminateScriptAll(error);
	}

	if (replaySaveScene_) {
		ScriptManager* scriptManager = replaySaveScene_->GetScriptManager();
		if (scriptManager)
			scriptManager->TerminateScriptAll(error);
	}
}
void StgSystemController::GetAllScriptList(std::list<weak_ptr<ScriptManager>>& listRes) {
	listRes.clear();

	if (packageController_)
		listRes.push_back(packageController_->GetScriptManagerRef());
	if (stageController_) {
		listRes.push_back(stageController_->GetScriptManagerRef());

		ref_count_ptr<StgPauseScene> pauseScene = stageController_->GetPauseManager();
		if (pauseScene)
			listRes.push_back(pauseScene->GetScriptManagerRef());
	}

	if (endScene_)
		listRes.push_back(endScene_->GetScriptManagerRef());
	if (replaySaveScene_)
		listRes.push_back(replaySaveScene_->GetScriptManagerRef());
}

//****************************************************************************
//StgSystemInformation
//****************************************************************************
StgSystemInformation::StgSystemInformation() {
	scene_ = SCENE_NULL;
	bEndStg_ = false;
	bRetry_ = false;

	invalidPriMin_ = -1;
	invalidPriMax_ = -1;

	pathPauseScript_ = EPathProperty::GetStgDefaultScriptDirectory() + L"Default_Pause.txt";
	pathEndSceneScript_ = EPathProperty::GetStgDefaultScriptDirectory() + L"Default_EndScene.txt";
	pathReplaySaveSceneScript_ = EPathProperty::GetStgDefaultScriptDirectory() + L"Default_ReplaySaveScene.txt";
}
StgSystemInformation::~StgSystemInformation() {}
std::wstring StgSystemInformation::GetErrorMessage() {
	std::wstring res = L"";
	for (auto itr = listError_.begin(); itr != listError_.end(); itr++) {
		const std::wstring& str = *itr;
		if (str == L"Retry") continue;
		res += str + L"\r\n" + L"\r\n";
	}
	return res;
}
bool StgSystemInformation::IsPackageMode() {
	return infoMain_->type_ == ScriptInformation::TYPE_PACKAGE;
}
void StgSystemInformation::ResetRetry() {
	bEndStg_ = false;
	bRetry_ = false;
	listError_.clear();
}
void StgSystemInformation::SetInvalidRenderPriority(int priMin, int priMax) {
	invalidPriMin_ = priMin;
	invalidPriMax_ = priMax;
}
