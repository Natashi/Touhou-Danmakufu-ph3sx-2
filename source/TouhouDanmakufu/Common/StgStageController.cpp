#include "source/GcLib/pch.h"

#include "StgStageController.hpp"
#include "StgSystem.hpp"

//*******************************************************************
//StgStageController
//*******************************************************************
StgStageController::StgStageController(StgSystemController* systemController) {
	systemController_ = systemController;
	infoSystem_ = systemController_->GetSystemInformation();
}
StgStageController::~StgStageController() {
	if (scriptManager_) {
		scriptManager_->OrphanAllScripts();
	}
}
void StgStageController::Initialize(ref_count_ptr<StgStageStartData> startData) {
	ELogger* logger = ELogger::GetInstance();
	auto infoLog = logger->GetInfoPanel();

	Math::InitializeFPU();

	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	DirectGraphics* graphics = DirectGraphics::GetBase();

	auto infoStage = startData->infoStage_;
	auto replayStageData = startData->replayStageData_;
	auto prevStageData = startData->prevStageInfo_;
	auto prevPlayerInfo = startData->prevPlayerInfo_;

	infoStage_ = infoStage;
	infoStage_->SetReplay(replayStageData != nullptr);

	{
		int replayState = infoStage_->IsReplay() ? 
			KeyReplayManager::STATE_REPLAY : 
			KeyReplayManager::STATE_RECORD;

		keyReplayManager_.reset(new KeyReplayManager(EDirectInput::GetInstance()));
		keyReplayManager_->SetManageState(replayState);

		keyReplayManager_->AddTarget(EDirectInput::KEY_LEFT);
		keyReplayManager_->AddTarget(EDirectInput::KEY_RIGHT);
		keyReplayManager_->AddTarget(EDirectInput::KEY_UP);
		keyReplayManager_->AddTarget(EDirectInput::KEY_DOWN);
		keyReplayManager_->AddTarget(EDirectInput::KEY_SHOT);
		keyReplayManager_->AddTarget(EDirectInput::KEY_BOMB);
		keyReplayManager_->AddTarget(EDirectInput::KEY_SLOWMOVE);
		keyReplayManager_->AddTarget(EDirectInput::KEY_USER1);
		keyReplayManager_->AddTarget(EDirectInput::KEY_USER2);
		keyReplayManager_->AddTarget(EDirectInput::KEY_OK);
		keyReplayManager_->AddTarget(EDirectInput::KEY_CANCEL);

		auto& listReplayTargetKey = infoSystem_->GetReplayTargetKeyList();
		for (auto& key : listReplayTargetKey) {
			keyReplayManager_->AddTarget(key);
		}

		if (replayStageData == nullptr)
			replayStageData.reset(new ReplayInformation::StageData());
		infoStage_->SetReplayData(replayStageData);
	}

	{
		auto slow = make_unique<PseudoSlowInformation>();
		infoSlow_ = slow.get();
		EFpsController::GetInstance()->AddFpsControlObject(MOVE(slow));
	}

	if (prevStageData) {
		infoStage_->SetScore(prevStageData->GetScore());
		infoStage_->SetGraze(prevStageData->GetGraze());
		infoStage_->SetPoint(prevStageData->GetPoint());
	}

	if (!infoStage_->IsReplay()) {
		uint32_t randSeed = infoStage_->GetRandProvider()->GetSeed();
		replayStageData->SetRandSeed(randSeed);
		
		if (infoLog)
			infoLog->SetInfo(11, "Rand seed", StringUtility::Format("%08x", randSeed));

		ref_count_ptr<ScriptInformation> infoParent = systemController_->GetSystemInformation()->GetMainScriptInformation();
		ref_count_ptr<ScriptInformation> infoMain = infoStage_->GetMainScriptInformation();
		const std::wstring& pathParentScript = infoParent->pathScript_;
		const std::wstring& pathMainScript = infoMain->pathScript_;
		std::wstring filenameMainScript = PathProperty::GetFileName(pathMainScript);
		std::wstring pathMainScriptRelative = PathProperty::GetRelativeDirectory(pathParentScript, pathMainScript);

		replayStageData->SetMainScriptID(infoMain->id_);
		replayStageData->SetMainScriptName(filenameMainScript);
		replayStageData->SetMainScriptRelativePath(pathMainScriptRelative);
		replayStageData->SetStartScore(infoStage_->GetScore());
		replayStageData->SetGraze(infoStage_->GetGraze());
		replayStageData->SetPoint(infoStage_->GetPoint());
	}
	else {	//Replay
		uint32_t randSeed = replayStageData->GetRandSeed();
		infoStage_->GetRandProvider()->Initialize(randSeed);

		if (infoLog)
			infoLog->SetInfo(11, "Rand seed", StringUtility::Format("%08x", randSeed));

		keyReplayManager_->ReadRecord(replayStageData->GetReplayKeyRecord());

		infoStage_->SetScore(replayStageData->GetStartScore());
		infoStage_->SetGraze(replayStageData->GetGraze());
		infoStage_->SetPoint(replayStageData->GetPoint());

		prevPlayerInfo.reset(new StgPlayerInformation());
		prevPlayerInfo->SetLife(replayStageData->GetPlayerLife());
		prevPlayerInfo->SetSpell(replayStageData->GetPlayerBombCount());
		prevPlayerInfo->SetPower(replayStageData->GetPlayerPower());
		prevPlayerInfo->SetRebirthFrame(replayStageData->GetPlayerRebirthFrame());
	}

	objectManagerMain_.reset(new StgStageScriptObjectManager(this));
	scriptManager_.reset(new StgStageScriptManager(this));

	enemyManager_.reset(new StgEnemyManager(this));
	shotManager_.reset(new StgShotManager(this));
	itemManager_.reset(new StgItemManager(this));
	intersectionManager_.reset(new StgIntersectionManager());
	pauseManager_.reset(new StgPauseScene(systemController_));

	intersectionManager_->SetVisualizerRenderPriority(infoStage->GetCameraFocusPermitPriority() - 1);

	// It's a package script, link its manager with the stage's
	if (auto packageController = systemController_->GetPackageController()) {
		shared_ptr<ScriptManager> packageScriptManager = std::dynamic_pointer_cast<ScriptManager>(
			packageController->GetScriptManager());
		shared_ptr<ScriptManager> stageScriptManager = std::dynamic_pointer_cast<ScriptManager>(
			scriptManager_);
		ScriptManager::AddRelativeScriptManagerMutual(packageScriptManager, stageScriptManager);
	}

	ref_count_ptr<ScriptInformation> infoMain = infoStage_->GetMainScriptInformation();
	std::wstring dirInfo = PathProperty::GetFileDirectory(infoMain->pathScript_);

	ELogger::WriteTop(StringUtility::Format(L"Main script: [%s]", 
		PathProperty::ReduceModuleDirectory(infoMain->pathScript_).c_str()));

	{
		std::wstring pathSystemScript = infoMain->pathSystem_;
		if (pathSystemScript == ScriptInformation::DEFAULT)
			pathSystemScript = EPathProperty::GetStgDefaultScriptDirectory() + L"Default_System.txt";
		if (pathSystemScript.size() > 0) {
			pathSystemScript = EPathProperty::ExtendRelativeToFull(dirInfo, pathSystemScript);
			ELogger::WriteTop(StringUtility::Format(L"System script: [%s]", 
				PathProperty::ReduceModuleDirectory(pathSystemScript).c_str()));

			auto script = scriptManager_->LoadScript(
				scriptManager_, pathSystemScript, StgStageScript::TYPE_SYSTEM);
			scriptManager_->StartScript(script);
		}
	}

	ref_unsync_ptr<StgPlayerObject> objPlayer = nullptr;
	ref_count_ptr<ScriptInformation> infoPlayer = infoStage_->GetPlayerScriptInformation();
	const std::wstring& pathPlayerScript = infoPlayer->pathScript_;

	if (pathPlayerScript.size() > 0) {
		ELogger::WriteTop(StringUtility::Format(L"Player script: [%s]", 
			PathProperty::ReduceModuleDirectory(pathPlayerScript).c_str()));
		int idPlayer = scriptManager_->GetObjectManager()->CreatePlayerObject();
		objPlayer = ref_unsync_ptr<StgPlayerObject>::Cast(GetMainRenderObject(idPlayer));

		if (systemController_->GetSystemInformation()->IsPackageMode())
			objPlayer->SetEnableStateEnd(false);

		auto script = scriptManager_->LoadScript(
			scriptManager_, pathPlayerScript, StgStageScript::TYPE_PLAYER);
		_SetupReplayTargetCommonDataArea(script);

		shared_ptr<StgStagePlayerScript> scriptPlayer =
			std::dynamic_pointer_cast<StgStagePlayerScript>(script);
		objPlayer->SetScript(scriptPlayer.get());

		scriptManager_->SetPlayerScript(script);
		scriptManager_->StartScript(script);

		if (prevPlayerInfo)
			objPlayer->SetPlayerInformation(prevPlayerInfo);
	}
	if (objPlayer)
		infoStage_->SetPlayerObjectInformation(objPlayer->GetPlayerInformation());

	if (infoMain->type_ == ScriptInformation::TYPE_SINGLE) {
		std::wstring pathMainScript = EPathProperty::GetSystemResourceDirectory() + L"script/System_SingleStage.txt";
		auto script = scriptManager_->LoadScript(
			scriptManager_, pathMainScript, StgStageScript::TYPE_STAGE);
		scriptManager_->StartScript(script);
	}
	else if (infoMain->type_ == ScriptInformation::TYPE_PLURAL) {
		std::wstring pathMainScript = EPathProperty::GetSystemResourceDirectory() + L"script/System_PluralStage.txt";
		auto script = scriptManager_->LoadScript(
			scriptManager_, pathMainScript, StgStageScript::TYPE_STAGE);
		scriptManager_->StartScript(script);
	}
	else {
		const std::wstring& pathMainScript = infoMain->pathScript_;
		if (pathMainScript.size() > 0) {
			auto script = scriptManager_->LoadScript(
				scriptManager_, pathMainScript, StgStageScript::TYPE_STAGE);
			_SetupReplayTargetCommonDataArea(script);
			scriptManager_->StartScript(script);
		}
	}

	{
		std::wstring pathBack = infoMain->pathBackground_;
		if (pathBack == ScriptInformation::DEFAULT)
			pathBack = L"";
		if (pathBack.size() > 0) {
			pathBack = EPathProperty::ExtendRelativeToFull(dirInfo, pathBack);
			ELogger::WriteTop(StringUtility::Format(L"Background script: [%s]", 
				PathProperty::ReduceModuleDirectory(pathBack).c_str()));
			auto script = scriptManager_->LoadScript(
				scriptManager_, pathBack, StgStageScript::TYPE_STAGE);
			scriptManager_->StartScript(script);
		}
	}

	if (!infoStage_->IsReplay()) {
		ref_unsync_ptr<StgPlayerObject> objPlayer = GetPlayerObject();
		if (objPlayer) {
			replayStageData->SetPlayerLife(objPlayer->GetLife());
			replayStageData->SetPlayerBombCount(objPlayer->GetSpell());
			replayStageData->SetPlayerPower(objPlayer->GetPower());
			replayStageData->SetPlayerRebirthFrame(objPlayer->GetRebirthFrame());
		}
		const std::wstring& pathPlayerScript = infoPlayer->pathScript_;
		std::wstring filenamePlayerScript = PathProperty::GetFileName(pathPlayerScript);
		replayStageData->SetPlayerScriptFileName(filenamePlayerScript);
		replayStageData->SetPlayerScriptID(infoPlayer->id_);
		replayStageData->SetPlayerScriptReplayName(infoPlayer->replayName_);
	}

	infoStage_->SetStageStartTime(SystemUtility::GetCpuTime2());
}
void StgStageController::CloseScene() {
	EFpsController::GetInstance()->RemoveFpsControlObject(infoSlow_);

	//リプレイ
	if (!infoStage_->IsReplay()) {
		RecordBuffer recKey;
		keyReplayManager_->WriteRecord(recKey);

		ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage_->GetReplayData();
		replayStageData->SetReplayKeyRecord(MOVE(recKey));

		//最終フレーム
		DWORD stageFrame = infoStage_->GetCurrentFrame();
		replayStageData->SetEndFrame(stageFrame);

		replayStageData->SetLastScore(infoStage_->GetScore());
	}
}
void StgStageController::_SetupReplayTargetCommonDataArea(shared_ptr<ManagedScript> pScript) {
	auto script = std::dynamic_pointer_cast<StgStageScript>(pScript);
	if (script == nullptr) return;

	const gstd::value& res = script->RequestEvent(StgStageScript::EV_REQUEST_REPLAY_TARGET_COMMON_AREA);
	if (!res.has_data()) return;
	type_data::type_kind kindRes = res.get_type()->get_kind();
	if (kindRes != type_data::type_kind::tk_array) return;

	ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage_->GetReplayData();
	std::set<std::string> listArea;
	
	for (size_t iArray = 0; iArray < res.length_as_array(); ++iArray) {
		const value& arrayValue = res[iArray];
		std::string area = StringUtility::ConvertWideToMulti(arrayValue.as_string());
		listArea.insert(area);
	}

	auto scriptCommonDataManager = systemController_->GetCommonDataManager();
	if (!infoStage_->IsReplay()) {
		for (auto& area : listArea) {
			auto areaInManager = scriptCommonDataManager->GetArea(area);
			replayStageData->SetCommonData(area, areaInManager);
		}
	}
	else {
		for (auto& area : listArea) {
			auto areaInReplay = replayStageData->GetCommonData(area);
			scriptCommonDataManager->SetArea(area, MOVE(areaInReplay));
		}
	}
}

void StgStageController::Work() {
	EDirectInput* input = EDirectInput::GetInstance();
	auto infoSystem = systemController_->GetSystemInformation();

	bool bPackageMode = infoSystem->IsPackageMode();

	bool bPermitRetryKey = !input->IsTargetKeyCode(DIK_BACK);
	if (!bPackageMode && bPermitRetryKey && input->GetKeyState(DIK_BACK) == KEY_PUSH) {
		//リトライ
		if (!infoStage_->IsReplay()) {
			infoSystem->SetRetry();
			return;
		}
	}

	bool bCurrentPause = infoStage_->IsPause();
	if (bPackageMode && bCurrentPause) {
		//パッケージモードで停止中の場合は、パッケージスクリプトで処理する
		return;
	}

	bool bPauseKey = (input->GetVirtualKeyState(EDirectInput::KEY_PAUSE) == KEY_PUSH);
	if (bPauseKey && !bPackageMode) {
		//停止キー押下
		if (!bCurrentPause)
			pauseManager_->Start();
		else
			pauseManager_->Finish();
	}
	else {
		if (!bCurrentPause) {
			//Update replay keys
			keyReplayManager_->Update();

			//Clean up objects
			objectManagerMain_->CleanupObject();

			//Process all non-player scripts
			scriptManager_->Work(StgStageScript::TYPE_SYSTEM);
			scriptManager_->Work(StgStageScript::TYPE_STAGE);
			scriptManager_->Work(StgStageScript::TYPE_SHOT);
			scriptManager_->Work(StgStageScript::TYPE_ITEM);

			ref_unsync_ptr<StgPlayerObject> objPlayer = GetPlayerObject();

			//Move the player
			if (objPlayer)
				objPlayer->Move();
			//Process the player script
			scriptManager_->Work(StgStageScript::TYPE_PLAYER);

			//Skip all this if the stage has already ended
			if (infoStage_->IsEnd()) return;
			objectManagerMain_->WorkObject();

			enemyManager_->Work();
			shotManager_->Work();
			itemManager_->Work();

			//Process intersections
			enemyManager_->RegistIntersectionTarget();
			shotManager_->RegistIntersectionTarget();
			intersectionManager_->Work();

			//Process graze events
			if (objPlayer)
				objPlayer->SendGrazeEvent();

			if (!infoStage_->IsReplay()) {
				//Add FPS entry to the replay data
				DWORD stageFrame = infoStage_->GetCurrentFrame();
				if (stageFrame % 60 == 0) {
					ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage_->GetReplayData();
					float framePerSecond = EFpsController::GetInstance()->GetCurrentFps();
					replayStageData->AddFramePerSecond(framePerSecond);
				}
			}

			infoStage_->AdvanceFrame();
		}
		else {
			pauseManager_->Work();
		}
	}

	ELogger* logger = ELogger::GetInstance();
	auto infoLog = logger->GetInfoPanel();

	infoLog->SetInfo(6, "Shot count", std::to_string(shotManager_->GetShotCountAll()));
	infoLog->SetInfo(7, "Enemy count", std::to_string(enemyManager_->GetEnemyCount()));
	infoLog->SetInfo(8, "Item count", std::to_string(itemManager_->GetItemCount()));
}
void StgStageController::Render() {
	bool bPause = infoStage_->IsPause();
	if (!bPause) {
		objectManagerMain_->RenderObject();

		if (infoStage_->IsReplay()) {
			//リプレイ中
		}
	}
	else {
		//停止
		pauseManager_->Render();
	}
}
void StgStageController::RenderToTransitionTexture() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	TextureManager* textureManager = ETextureManager::GetInstance();
	shared_ptr<Texture> texture = textureManager->GetTexture(TextureManager::TARGET_TRANSITION);

	graphics->SetAllowRenderTargetChange(false);
	graphics->SetRenderTarget(texture);
	graphics->ResetDeviceState();

	graphics->BeginScene(false, true);
	//objectManager->RenderObject();
	systemController_->RenderScriptObject();
	graphics->EndScene(false);

	graphics->SetRenderTarget(nullptr);
	graphics->SetAllowRenderTargetChange(true);

	/*
	if (false) {
		static int count = 0;
		std::wstring path = PathProperty::GetModuleDirectory() + StringUtility::FormatToWide("tempRT_transition\\temp_%04d.png", count);
		RECT rect = { 0, 0, 640, 480 };
		IDirect3DSurface9* pBackSurface = texture->GetD3DSurface();
		D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_PNG, pBackSurface, nullptr, &rect);
		count++;
	}
	*/
}
/*
shared_ptr<DxScriptObjectBase> StgStageController::GetMainRenderObject(int idObject) {
	return objectManagerMain_->GetObject(idObject);
}
shared_ptr<StgPlayerObject> StgStageController::GetPlayerObject() {
	int idPlayer = objectManagerMain_->GetPlayerObjectID();
	if (idPlayer == DxScript::ID_INVALID) return nullptr;
	return std::dynamic_pointer_cast<StgPlayerObject>(GetMainRenderObject(idPlayer));
}
*/

//*******************************************************************
//StgStageInformation
//*******************************************************************
StgStageInformation::StgStageInformation() {
	bEndStg_ = false;
	bPause_ = false;
	bReplay_ = false;
	frame_ = 0;
	stageIndex_ = 0;

	rcStgFrame_.Set(32, 16, 32 + 384, 16 + 448);
	SetStgFrameRect(rcStgFrame_);

	priMinStgFrame_ = 20;
	priMaxStgFrame_ = 80;
	priShotObject_ = 50;
	priItemObject_ = 40;
	priCameraFocusPermit_ = 69;

	rand_ = make_shared<RandProvider>();
	rand_->Initialize((uint32_t)SystemUtility::GetCpuTime2() ^ 0xf5682aebui32);
	score_ = 0;
	graze_ = 0;
	point_ = 0;
	result_ = RESULT_UNKNOWN;

	timeStart_ = 0;
}
StgStageInformation::~StgStageInformation() {}
void StgStageInformation::SetStgFrameRect(const DxRect<LONG>& rect, bool bUpdateFocusResetValue) {
	rcStgFrame_ = rect;
	rcStgFrame_.left *= DirectGraphics::g_dxCoordsMul_;
	rcStgFrame_.right *= DirectGraphics::g_dxCoordsMul_;
	rcStgFrame_.top *= DirectGraphics::g_dxCoordsMul_;
	rcStgFrame_.bottom *= DirectGraphics::g_dxCoordsMul_;

	if (bUpdateFocusResetValue) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		auto& camera2D = graphics->GetCamera2D();

		D3DXVECTOR2 pos = {
			(rcStgFrame_.right - rcStgFrame_.left) / 2.0f,
			(rcStgFrame_.bottom - rcStgFrame_.top) / 2.0f
		};
		camera2D->SetResetFocus(pos);
		camera2D->Reset();
	}
}

//*******************************************************************
//PseudoSlowInformation
//*******************************************************************
DWORD PseudoSlowInformation::GetFps() {
	const uint32_t STANDARD_FPS = DnhConfiguration::GetInstance()->fpsStandard_;

	DWORD fps = STANDARD_FPS;
	int target = TARGET_ALL;

	auto itrPlayer = mapDataPlayer_.find(target);
	if (itrPlayer != mapDataPlayer_.end())
		fps = std::min(fps, itrPlayer->second.GetFps());

	auto itrEnemy = mapDataEnemy_.find(target);
	if (itrEnemy != mapDataEnemy_.end())
		fps = std::min(fps, itrEnemy->second.GetFps());

	return fps;
}
bool PseudoSlowInformation::IsValidFrame(int target) {
	auto itr = mapValid_.find(target);
	bool res = itr == mapValid_.end() || itr->second;
	return res;
}
void PseudoSlowInformation::Next() {
	const uint32_t STANDARD_FPS = DnhConfiguration::GetInstance()->fpsStandard_;

	DWORD fps = STANDARD_FPS;
	int target = TARGET_ALL;

	auto itrPlayer = mapDataPlayer_.find(target);
	if (itrPlayer != mapDataPlayer_.end())
		fps = std::min(fps, itrPlayer->second.GetFps());

	auto itrEnemy = mapDataEnemy_.find(target);
	if (itrEnemy != mapDataEnemy_.end())
		fps = std::min(fps, itrEnemy->second.GetFps());

	bool bValid = false;
	if (fps == STANDARD_FPS) {
		bValid = true;
	}
	else {
		current_ += fps;
		if (current_ >= STANDARD_FPS) {
			current_ %= STANDARD_FPS;
			bValid = true;
		}
	}

	mapValid_[target] = bValid;
}
void PseudoSlowInformation::AddSlow(DWORD fps, int owner, int target) {
	const uint32_t STANDARD_FPS = DnhConfiguration::GetInstance()->fpsStandard_;

	fps = std::clamp<DWORD>(fps, 1, STANDARD_FPS);

	SlowData data;
	data.SetFps(fps);

	switch (owner) {
	case OWNER_PLAYER:
		mapDataPlayer_[target] = data;
		break;
	case OWNER_ENEMY:
		mapDataEnemy_[target] = data;
		break;
	}
}
void PseudoSlowInformation::RemoveSlow(int owner, int target) {
	switch (owner) {
	case OWNER_PLAYER:
		mapDataPlayer_.erase(target);
		break;
	case OWNER_ENEMY:
		mapDataEnemy_.erase(target);
		break;
	}
}
