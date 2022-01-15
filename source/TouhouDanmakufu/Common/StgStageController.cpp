#include "source/GcLib/pch.h"

#include "StgStageController.hpp"
#include "StgSystem.hpp"

//*******************************************************************
//StgStageController
//*******************************************************************
StgStageController::StgStageController(StgSystemController* systemController) {
	systemController_ = systemController;
	infoSystem_ = systemController_->GetSystemInformation();

	scriptManager_ = nullptr;
	enemyManager_ = nullptr;
	shotManager_ = nullptr;
	itemManager_ = nullptr;
	intersectionManager_ = nullptr;
}
StgStageController::~StgStageController() {
	objectManagerMain_ = nullptr;
	if (scriptManager_) {
		scriptManager_->OrphanAllScripts();
		scriptManager_ = nullptr;
	}
	ptr_delete(enemyManager_);
	ptr_delete(shotManager_);
	ptr_delete(itemManager_);
	ptr_delete(intersectionManager_);
}
void StgStageController::Initialize(ref_count_ptr<StgStageStartData> startData) {
	Math::InitializeFPU();

	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	DirectGraphics* graphics = DirectGraphics::GetBase();
	/*
	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	camera3D->Reset();
	camera3D->SetPerspectiveWidth(384);
	camera3D->SetPerspectiveHeight(448);
	camera3D->SetPerspectiveClip(10, 2000);
	camera3D->thisProjectionChanged_ = true;

	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
	camera2D->Reset();
	*/

	ref_count_ptr<StgStageInformation> infoStage = startData->GetStageInformation();
	ref_count_ptr<ReplayInformation::StageData> replayStageData = startData->GetStageReplayData();
	ref_count_ptr<StgStageInformation> prevStageData = startData->GetPrevStageInformation();
	ref_count_ptr<StgPlayerInformation> prevPlayerInfo = startData->GetPrevPlayerInformation();

	infoStage_ = infoStage;
	infoStage_->SetReplay(replayStageData != nullptr);

	int replayState = infoStage_->IsReplay() ? KeyReplayManager::STATE_REPLAY : KeyReplayManager::STATE_RECORD;
	keyReplayManager_ = new KeyReplayManager(EDirectInput::GetInstance());
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
	std::set<int16_t> listReplayTargetKey = infoSystem_->GetReplayTargetKeyList();
	for (auto itrKey = listReplayTargetKey.begin(); itrKey != listReplayTargetKey.end(); itrKey++) {
		int16_t id = *itrKey;
		keyReplayManager_->AddTarget(id);
	}

	if (replayStageData == nullptr)
		replayStageData = new ReplayInformation::StageData();
	infoStage_->SetReplayData(replayStageData);

	infoSlow_ = new PseudoSlowInformation();
	ref_count_weak_ptr<PseudoSlowInformation> wPtr = infoSlow_;
	EFpsController::GetInstance()->AddFpsControlObject(wPtr);

	if (prevStageData) {
		infoStage_->SetScore(prevStageData->GetScore());
		infoStage_->SetGraze(prevStageData->GetGraze());
		infoStage_->SetPoint(prevStageData->GetPoint());
	}

	if (!infoStage_->IsReplay()) {
		uint32_t randSeed = infoStage_->GetRandProvider()->GetSeed();
		replayStageData->SetRandSeed(randSeed);

		ELogger* logger = ELogger::GetInstance();
		if (logger->IsWindowVisible())
			logger->SetInfo(11, L"Rand seed", StringUtility::Format(L"%08x", randSeed));

		ref_count_ptr<ScriptInformation> infoParent = systemController_->GetSystemInformation()->GetMainScriptInformation();
		ref_count_ptr<ScriptInformation> infoMain = infoStage_->GetMainScriptInformation();
		const std::wstring& pathParentScript = infoParent->GetScriptPath();
		const std::wstring& pathMainScript = infoMain->GetScriptPath();
		std::wstring filenameMainScript = PathProperty::GetFileName(pathMainScript);
		std::wstring pathMainScriptRelative = PathProperty::GetRelativeDirectory(pathParentScript, pathMainScript);

		replayStageData->SetMainScriptID(infoMain->GetID());
		replayStageData->SetMainScriptName(filenameMainScript);
		replayStageData->SetMainScriptRelativePath(pathMainScriptRelative);
		replayStageData->SetStartScore(infoStage_->GetScore());
		replayStageData->SetGraze(infoStage_->GetGraze());
		replayStageData->SetPoint(infoStage_->GetPoint());
	}
	else {	//Replay
		uint32_t randSeed = replayStageData->GetRandSeed();
		infoStage_->GetRandProvider()->Initialize(randSeed);

		ELogger* logger = ELogger::GetInstance();
		if (logger->IsWindowVisible())
			logger->SetInfo(11, L"Rand seed", StringUtility::Format(L"%08x", randSeed));

		keyReplayManager_->ReadRecord(*replayStageData->GetReplayKeyRecord());

		infoStage_->SetScore(replayStageData->GetStartScore());
		infoStage_->SetGraze(replayStageData->GetGraze());
		infoStage_->SetPoint(replayStageData->GetPoint());

		prevPlayerInfo = new StgPlayerInformation();
		prevPlayerInfo->SetLife(replayStageData->GetPlayerLife());
		prevPlayerInfo->SetSpell(replayStageData->GetPlayerBombCount());
		prevPlayerInfo->SetPower(replayStageData->GetPlayerPower());
		prevPlayerInfo->SetRebirthFrame(replayStageData->GetPlayerRebirthFrame());
	}

	objectManagerMain_ = std::make_shared<StgStageScriptObjectManager>(this);
	//_objectManagerMain_ = objectManagerMain_;
	scriptManager_ = std::make_shared<StgStageScriptManager>(this);
	enemyManager_ = new StgEnemyManager(this);
	shotManager_ = new StgShotManager(this);
	itemManager_ = new StgItemManager(this);
	intersectionManager_ = new StgIntersectionManager();
	pauseManager_ = new StgPauseScene(systemController_);

	//It's a package script, link its manager with the stage's
	if (StgPackageController* packageController = systemController_->GetPackageController()) {
		shared_ptr<ScriptManager> packageScriptManager = std::dynamic_pointer_cast<ScriptManager>(packageController->GetScriptManagerRef());
		shared_ptr<ScriptManager> stageScriptManager = std::dynamic_pointer_cast<ScriptManager>(scriptManager_);
		ScriptManager::AddRelativeScriptManagerMutual(packageScriptManager, stageScriptManager);
	}

	ref_count_ptr<ScriptInformation> infoMain = infoStage_->GetMainScriptInformation();
	std::wstring dirInfo = PathProperty::GetFileDirectory(infoMain->GetScriptPath());

	ELogger::WriteTop(StringUtility::Format(L"Main script: [%s]", 
		PathProperty::ReduceModuleDirectory(infoMain->GetScriptPath()).c_str()));

	{
		std::wstring pathSystemScript = infoMain->GetSystemPath();
		if (pathSystemScript == ScriptInformation::DEFAULT)
			pathSystemScript = EPathProperty::GetStgDefaultScriptDirectory() + L"Default_System.txt";
		if (pathSystemScript.size() > 0) {
			pathSystemScript = EPathProperty::ExtendRelativeToFull(dirInfo, pathSystemScript);
			ELogger::WriteTop(StringUtility::Format(L"System script: [%s]", 
				PathProperty::ReduceModuleDirectory(pathSystemScript).c_str()));

			auto script = scriptManager_->LoadScript(pathSystemScript, StgStageScript::TYPE_SYSTEM);
			scriptManager_->StartScript(script);
		}
	}

	ref_unsync_ptr<StgPlayerObject> objPlayer = nullptr;
	ref_count_ptr<ScriptInformation> infoPlayer = infoStage_->GetPlayerScriptInformation();
	const std::wstring& pathPlayerScript = infoPlayer->GetScriptPath();

	if (pathPlayerScript.size() > 0) {
		ELogger::WriteTop(StringUtility::Format(L"Player script: [%s]", 
			PathProperty::ReduceModuleDirectory(pathPlayerScript).c_str()));
		int idPlayer = scriptManager_->GetObjectManager()->CreatePlayerObject();
		objPlayer = ref_unsync_ptr<StgPlayerObject>::Cast(GetMainRenderObject(idPlayer));

		if (systemController_->GetSystemInformation()->IsPackageMode())
			objPlayer->SetEnableStateEnd(false);

		auto script = scriptManager_->LoadScript(pathPlayerScript, StgStageScript::TYPE_PLAYER);
		_SetupReplayTargetCommonDataArea(script);

		shared_ptr<StgStagePlayerScript> scriptPlayer =
			std::dynamic_pointer_cast<StgStagePlayerScript>(script);
		objPlayer->SetScript(scriptPlayer.get());

		scriptManager_->SetPlayerScript(script);
		scriptManager_->StartScript(script);

		if (prevPlayerInfo)
			objPlayer->SetPlayerInforamtion(prevPlayerInfo);
	}
	if (objPlayer)
		infoStage_->SetPlayerObjectInformation(objPlayer->GetPlayerInformation());

	if (infoMain->GetType() == ScriptInformation::TYPE_SINGLE) {
		std::wstring pathMainScript = EPathProperty::GetSystemResourceDirectory() + L"script/System_SingleStage.txt";
		auto script = scriptManager_->LoadScript(pathMainScript, StgStageScript::TYPE_STAGE);
		scriptManager_->StartScript(script);
	}
	else if (infoMain->GetType() == ScriptInformation::TYPE_PLURAL) {
		std::wstring pathMainScript = EPathProperty::GetSystemResourceDirectory() + L"script/System_PluralStage.txt";
		auto script = scriptManager_->LoadScript(pathMainScript, StgStageScript::TYPE_STAGE);
		scriptManager_->StartScript(script);
	}
	else {
		const std::wstring& pathMainScript = infoMain->GetScriptPath();
		if (pathMainScript.size() > 0) {
			auto script = scriptManager_->LoadScript(pathMainScript, StgStageScript::TYPE_STAGE);
			_SetupReplayTargetCommonDataArea(script);
			scriptManager_->StartScript(script);
		}
	}

	{
		std::wstring pathBack = infoMain->GetBackgroundPath();
		if (pathBack == ScriptInformation::DEFAULT)
			pathBack = L"";
		if (pathBack.size() > 0) {
			pathBack = EPathProperty::ExtendRelativeToFull(dirInfo, pathBack);
			ELogger::WriteTop(StringUtility::Format(L"Background script: [%s]", 
				PathProperty::ReduceModuleDirectory(pathBack).c_str()));
			auto script = scriptManager_->LoadScript(pathBack, StgStageScript::TYPE_STAGE);
			scriptManager_->StartScript(script);
		}
	}

	{
		std::wstring pathBGM = infoMain->GetBgmPath();
		if (pathBGM == ScriptInformation::DEFAULT)
			pathBGM = L"";
		if (pathBGM.size() > 0) {
			pathBGM = EPathProperty::ExtendRelativeToFull(dirInfo, pathBGM);
			ELogger::WriteTop(StringUtility::Format(L"BGM: [%s]", 
				PathProperty::ReduceModuleDirectory(pathBGM).c_str()));
			shared_ptr<SoundPlayer> player = DirectSoundManager::GetBase()->GetPlayer(pathBGM);
			if (player) {
				player->SetAutoDelete(true);
				player->SetSoundDivision(SoundDivision::DIVISION_BGM);

				player->GetPlayStyle()->bLoop_ = true;
				player->Play();
			}
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
		const std::wstring& pathPlayerScript = infoPlayer->GetScriptPath();
		std::wstring filenamePlayerScript = PathProperty::GetFileName(pathPlayerScript);
		replayStageData->SetPlayerScriptFileName(filenamePlayerScript);
		replayStageData->SetPlayerScriptID(infoPlayer->GetID());
		replayStageData->SetPlayerScriptReplayName(infoPlayer->GetReplayName());
	}

	infoStage_->SetStageStartTime(timeGetTime());
}
void StgStageController::CloseScene() {
	ref_count_weak_ptr<PseudoSlowInformation> wPtr = infoSlow_;
	EFpsController::GetInstance()->RemoveFpsControlObject(wPtr);

	//リプレイ
	if (!infoStage_->IsReplay()) {
		//キー
		ref_count_ptr<RecordBuffer> recKey = new RecordBuffer();
		keyReplayManager_->WriteRecord(*recKey);

		ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage_->GetReplayData();
		replayStageData->SetReplayKeyRecord(recKey);

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

	shared_ptr<ScriptCommonDataManager> scriptCommonDataManager = systemController_->GetCommonDataManager();
	if (!infoStage_->IsReplay()) {
		for (auto itrArea = listArea.begin(); itrArea != listArea.end(); ++itrArea) {
			const std::string& area = (*itrArea);
			shared_ptr<ScriptCommonData> commonData = scriptCommonDataManager->GetData(area);
			replayStageData->SetCommonData(area, commonData);
		}
	}
	else {
		for (auto itrArea = listArea.begin(); itrArea != listArea.end(); ++itrArea) {
			const std::string& area = (*itrArea);
			shared_ptr<ScriptCommonData> commonData = replayStageData->GetCommonData(area);
			scriptCommonDataManager->SetData(area, commonData);
		}
	}
}

void StgStageController::Work() {
	EDirectInput* input = EDirectInput::GetInstance();
	ref_count_ptr<StgSystemInformation>& infoSystem = systemController_->GetSystemInformation();
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

	if (!Application::GetBase()->IsFocused() && !infoStage_->IsReplay()) {
		if (!bCurrentPause)
			input->GetVirtualKey(EDirectInput::KEY_PAUSE)->SetKeyState(KEY_PUSH);
		else
			input->ClearKeyState();
	}

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
	if (logger->IsWindowVisible()) {
		logger->SetInfo(6, L"Shot count", StringUtility::Format(L"%d", shotManager_->GetShotCountAll()));
		logger->SetInfo(7, L"Enemy count", StringUtility::Format(L"%d", enemyManager_->GetEnemyCount()));
		logger->SetInfo(8, L"Item count", StringUtility::Format(L"%d", itemManager_->GetItemCount()));
	}
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

	graphics->SetRenderTarget(texture, false);
	graphics->BeginScene(false, true);

	//objectManager->RenderObject();
	systemController_->RenderScriptObject();

	graphics->EndScene(false);
	graphics->SetRenderTarget(nullptr, false);

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

	rand_ = std::make_shared<RandProvider>();
	rand_->Initialize(timeGetTime() ^ 0xf5682aeb);
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

	ref_count_ptr<D3DXVECTOR2> pos = new D3DXVECTOR2;
	pos->x = (rcStgFrame_.right - rcStgFrame_.left) / 2.0f;
	pos->y = (rcStgFrame_.bottom - rcStgFrame_.top) / 2.0f;

	if (bUpdateFocusResetValue) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
		camera2D->SetResetFocus(pos);
		camera2D->Reset();
	}
}

//*******************************************************************
//PseudoSlowInformation
//*******************************************************************
DWORD PseudoSlowInformation::GetFps() {
	DWORD fps = STANDARD_FPS;
	int target = TARGET_ALL;

	auto itrPlayer = mapDataPlayer_.find(target);
	if (itrPlayer != mapDataPlayer_.end())
		fps = std::min(fps, itrPlayer->second->GetFps());

	auto itrEnemy = mapDataEnemy_.find(target);
	if (itrEnemy != mapDataEnemy_.end())
		fps = std::min(fps, itrEnemy->second->GetFps());

	return fps;
}
bool PseudoSlowInformation::IsValidFrame(int target) {
	auto itr = mapValid_.find(target);
	bool res = itr == mapValid_.end() || itr->second;
	return res;
}
void PseudoSlowInformation::Next() {
	DWORD fps = STANDARD_FPS;
	int target = TARGET_ALL;

	auto itrPlayer = mapDataPlayer_.find(target);
	if (itrPlayer != mapDataPlayer_.end())
		fps = std::min(fps, itrPlayer->second->GetFps());

	auto itrEnemy = mapDataEnemy_.find(target);
	if (itrEnemy != mapDataEnemy_.end())
		fps = std::min(fps, itrEnemy->second->GetFps());

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
	fps = std::clamp(fps, (DWORD)1, (DWORD)STANDARD_FPS);
	ref_count_ptr<SlowData> data = new SlowData();
	data->SetFps(fps);
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
