#include "source/GcLib/pch.h"

#include "StgShot.hpp"
#include "StgSystem.hpp"
#include "StgIntersection.hpp"
#include "StgItem.hpp"
#include "../../GcLib/directx/HLSL.hpp"

//****************************************************************************
//StgShotManager
//****************************************************************************
StgShotManager::StgShotManager(StgStageController* stageController) {
	stageController_ = stageController;

	listPlayerShotData_ = new StgShotDataList();
	listEnemyShotData_ = new StgShotDataList();

	rcDeleteClip_ = DxRect<LONG>(-64, -64, 64, 64);

	{
		RenderShaderLibrary* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();
		effectLayer_ = shaderManager_->GetRender2DShader();
		handleEffectWorld_ = effectLayer_->GetParameterBySemantic(nullptr, "WORLD");
	}
	{
		auto objectManager = stageController_->GetMainObjectManager();
		listRenderQueue_.resize(objectManager->GetRenderBucketCapacity());
		for (auto& iLayer : listRenderQueue_)
			iLayer.second.resize(32);
	}
}
StgShotManager::~StgShotManager() {
	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj)
			obj->ClearShotObject();
	}

	ptr_delete(listPlayerShotData_);
	ptr_delete(listEnemyShotData_);
}
void StgShotManager::Work() {
	for (auto itr = listObj_.begin(); itr != listObj_.end(); ) {
		ref_unsync_ptr<StgShotObject>& obj = *itr;
		if (obj->IsDeleted()) {
			obj->ClearShotObject();
			itr = listObj_.erase(itr);
		}
		else if (!obj->IsActive()) {
			itr = listObj_.erase(itr);
		}
		else ++itr;
	}
}
void StgShotManager::Render(int targetPriority) {
	if (targetPriority < 0 || targetPriority >= listRenderQueue_.size()) return;

	auto& renderQueueLayer = listRenderQueue_[targetPriority];
	if (renderQueueLayer.first == 0) return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetCullingMode(D3DCULL_NONE);
	graphics->SetLightingEnable(false);
	graphics->SetTextureFilter(D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);

	DWORD bEnableFog = FALSE;
	device->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();

	{
		D3DXMATRIX matProj;
		D3DXMatrixMultiply(&matProj, &camera2D->GetMatrix(), &graphics->GetViewPortMatrix());
		effectLayer_->SetMatrix(handleEffectWorld_, &matProj);
	}

	for (size_t i = 0; i < renderQueueLayer.first; ++i) {
		renderQueueLayer.second[i]->RenderOnShotManager();
	}

	size_t countBlendType = StgShotDataList::RENDER_TYPE_COUNT;
	BlendMode blendMode[] =
	{
		MODE_BLEND_ADD_ARGB,
		MODE_BLEND_ADD_RGB,
		MODE_BLEND_SHADOW,
		MODE_BLEND_MULTIPLY,
		MODE_BLEND_SUBTRACT,
		MODE_BLEND_INV_DESTRGB,
		MODE_BLEND_ALPHA,
		MODE_BLEND_ALPHA_INV,
	};

	RenderShaderLibrary* shaderManager = ShaderManager::GetBase()->GetRenderLib();
	VertexBufferManager* bufferManager = VertexBufferManager::GetBase();

	device->SetFVF(VERTEX_TLX::fvf);
	device->SetVertexDeclaration(shaderManager->GetVertexDeclarationTLX());

	device->SetStreamSource(0, bufferManager->GetGrowableVertexBuffer()->GetBuffer(), 0, sizeof(VERTEX_TLX));
	device->SetIndices(bufferManager->GetGrowableIndexBuffer()->GetBuffer());

	//Should I regroup these as texture->blend? I'll consider it later, I guess
	{
		UINT cPass = 1U;

		//Always renders enemy shots above player shots, completely obliterates TAΣ's wet dream.
		for (size_t iBlend = 0; iBlend < countBlendType; ++iBlend) {
			bool hasPolygon = false;
			std::vector<StgShotRenderer*>& listPlayer = listPlayerShotData_->GetRendererList(blendMode[iBlend] - 1);

			//In an attempt to minimize D3D calls in SetBlendMode.
			for (auto itr = listPlayer.begin(); itr != listPlayer.end() && !hasPolygon; ++itr)
				hasPolygon = (*itr)->countRenderIndex_ >= 3U;
			if (!hasPolygon) continue;

			graphics->SetBlendMode(blendMode[iBlend]);
			effectLayer_->SetTechnique(blendMode[iBlend] == MODE_BLEND_ALPHA_INV ? "RenderInv" : "Render");

			effectLayer_->Begin(&cPass, 0);
			if (cPass >= 1) {
				effectLayer_->BeginPass(0);
				for (auto itrRender = listPlayer.begin(); itrRender != listPlayer.end(); ++itrRender)
					(*itrRender)->Render(this);
				effectLayer_->EndPass();
			}
			effectLayer_->End();
		}
		for (size_t iBlend = 0; iBlend < countBlendType; ++iBlend) {
			bool hasPolygon = false;
			std::vector<StgShotRenderer*>& listEnemy = listEnemyShotData_->GetRendererList(blendMode[iBlend] - 1);

			for (auto itr = listEnemy.begin(); itr != listEnemy.end() && !hasPolygon; ++itr)
				hasPolygon = (*itr)->countRenderIndex_ >= 3U;
			if (!hasPolygon) continue;

			graphics->SetBlendMode(blendMode[iBlend]);
			effectLayer_->SetTechnique(blendMode[iBlend] == MODE_BLEND_ALPHA_INV ? "RenderInv" : "Render");

			effectLayer_->Begin(&cPass, 0);
			if (cPass >= 1) {
				effectLayer_->BeginPass(0);
				for (auto itrRender = listEnemy.begin(); itrRender != listEnemy.end(); ++itrRender)
					(*itrRender)->Render(this);
				effectLayer_->EndPass();
			}
			effectLayer_->End();
		}
	}

	device->SetVertexDeclaration(nullptr);
	device->SetIndices(nullptr);

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void StgShotManager::LoadRenderQueue() {
	for (auto& iLayer : listRenderQueue_)
		iLayer.first = 0;

	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted() || !obj->IsActive()) continue;
		auto& iQueue = listRenderQueue_[obj->GetRenderPriorityI()];
		while (iQueue.first >= iQueue.second.size())
			iQueue.second.resize(iQueue.second.size() * 2);
		iQueue.second[(iQueue.first)++] = obj.get();
	}
}

void StgShotManager::RegistIntersectionTarget() {
	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (!obj->IsDeleted() && obj->IsActive()) {
			obj->ClearIntersectedIdList();
			obj->RegistIntersectionTarget();
		}
	}
}
void StgShotManager::AddShot(ref_unsync_ptr<StgShotObject> obj) {
	obj->SetOwnObjectReference();
	listObj_.push_back(obj);
}

size_t StgShotManager::DeleteInCircle(int typeDelete, int typeTo, int typeOwner, int cx, int cy, int* radius) {
	int r = radius ? *radius : 0;
	int rr = r * r;

	int rect_x1 = cx - r;
	int rect_y1 = cy - r;
	int rect_x2 = cx + r;
	int rect_y2 = cy + r;

	size_t res = 0;

	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		if (typeDelete == DEL_TYPE_SHOT && obj->IsSpellResist()) continue;

		int sx = obj->GetPositionX();
		int sy = obj->GetPositionY();

		bool bInCircle = radius == nullptr;
		if (!bInCircle) {
			bool bPassAABB = (sx > rect_x1 && sy > rect_y1) && (sx < rect_x2 && sy < rect_y2);
			bInCircle = bPassAABB && Math::HypotSq<int64_t>(cx - sx, cy - sy) <= rr;
		}
		if (bInCircle) {
			if (typeTo == TO_TYPE_IMMEDIATE)
				obj->DeleteImmediate();
			else if (typeTo == TO_TYPE_FADE)
				obj->SetFadeDelete();
			else if (typeTo == TO_TYPE_ITEM)
				obj->ConvertToItem(false);
			++res;
		}
	}

	return res;
}
size_t StgShotManager::DeleteInRegularPolygon(int typeDelete, int typeTo, int typeOwner, int cx, int cy, int* radius, int edges, double angle) {
	int r = radius ? *radius : 0;

	int rect_x1 = cx - r;
	int rect_y1 = cy - r;
	int rect_x2 = cx + r;
	int rect_y2 = cy + r;

	size_t res = 0;

	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		if (typeDelete == DEL_TYPE_SHOT && obj->IsSpellResist()) continue;

		int sx = obj->GetPositionX();
		int sy = obj->GetPositionY();

		bool bInPolygon = radius == nullptr;
		if (!bInPolygon && ((sx > rect_x1 && sy > rect_y1) && (sx < rect_x2 && sy < rect_y2))) {
			float f = GM_PI / (float) edges;
			float cf = cosf(f);
			float dx = sx - cx;
			float dy = sy - cy;
			float dist = hypotf(dy, dx);
				
			bInPolygon = dist <= r;
			if (bInPolygon) {
				double r_apothem = r * cf;
				bInPolygon = dist <= r_apothem;
				if (!bInPolygon) {
					double ang = fmod(Math::NormalizeAngleRad(atan2(dy, dx)) - Math::DegreeToRadian(angle), 2 * f);
					bInPolygon = dist <= (r_apothem / cos(ang - f));
				}
			}
		}
		if (bInPolygon) {
			if (typeTo == TO_TYPE_IMMEDIATE)
				obj->DeleteImmediate();
			else if (typeTo == TO_TYPE_FADE)
				obj->SetFadeDelete();
			else if (typeTo == TO_TYPE_ITEM)
				obj->ConvertToItem(false);
			++res;
		}
	}

	return res;
}

std::vector<int> StgShotManager::GetShotIdInCircle(int typeOwner, int cx, int cy, int* radius) {
	int r = radius ? *radius : 0;
	int rr = r * r;

	int rect_x1 = cx - r;
	int rect_y1 = cy - r;
	int rect_x2 = cx + r;
	int rect_y2 = cy + r;

	std::vector<int> res;
	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;

		int sx = obj->GetPositionX();
		int sy = obj->GetPositionY();

		bool bInCircle = radius == nullptr;
		if (!bInCircle) {
			bool bPassAABB = (sx > rect_x1 && sy > rect_y1) && (sx < rect_x2&& sy < rect_y2);
			bInCircle = bPassAABB && Math::HypotSq<int64_t>(cx - sx, cy - sy) <= rr;
		}
		if (bInCircle)
			res.push_back(obj->GetObjectID());
	}

	return res;
}
std::vector<int> StgShotManager::GetShotIdInRegularPolygon(int typeOwner, int cx, int cy, int* radius, int edges, double angle) {
	int r = radius ? *radius : 0;
	int rr = r * r;

	int rect_x1 = cx - r;
	int rect_y1 = cy - r;
	int rect_x2 = cx + r;
	int rect_y2 = cy + r;

	std::vector<int> res;
	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;

		int sx = obj->GetPositionX();
		int sy = obj->GetPositionY();

		bool bInPolygon = radius == nullptr;
		if (!bInPolygon && ((sx > rect_x1 && sy > rect_y1) && (sx < rect_x2 && sy < rect_y2))) {
			float f = GM_PI / (float)edges;
			float cf = cosf(f);
			float dx = sx - cx;
			float dy = sy - cy;
			float dist = hypotf(dy, dx);

			bInPolygon = dist <= r;
			if (bInPolygon) {
				double r_apothem = r * cf;
				bInPolygon = dist <= r_apothem;
				if (!bInPolygon) {
					double ang = fmod(Math::NormalizeAngleRad(atan2(dy, dx)) - Math::DegreeToRadian(angle), 2 * f);
					bInPolygon = dist <= (r_apothem / cos(ang - f));
				}
			}
		}
		if (bInPolygon)
			res.push_back(obj->GetObjectID());
	}

	return res;
}
size_t StgShotManager::GetShotCount(int typeOwner) {
	size_t res = 0;

	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		++res;
	}

	return res;
}

void StgShotManager::SetDeleteEventEnableByType(int type, bool bEnable) {
	int bit = 0;
	switch (type) {
	case StgStageShotScript::EV_DELETE_SHOT_IMMEDIATE:
		bit = StgShotManager::BIT_EV_DELETE_IMMEDIATE;
		break;
	case StgStageShotScript::EV_DELETE_SHOT_TO_ITEM:
		bit = StgShotManager::BIT_EV_DELETE_TO_ITEM;
		break;
	case StgStageShotScript::EV_DELETE_SHOT_FADE:
		bit = StgShotManager::BIT_EV_DELETE_FADE;
		break;
	}

	if (bEnable) {
		listDeleteEventEnable_.set(bit);
	}
	else {
		listDeleteEventEnable_.reset(bit);
	}
}
bool StgShotManager::LoadPlayerShotData(const std::wstring& path, bool bReload) {
	return listPlayerShotData_->AddShotDataList(path, bReload);
}
bool StgShotManager::LoadEnemyShotData(const std::wstring& path, bool bReload) {
	return listEnemyShotData_->AddShotDataList(path, bReload);
}

//****************************************************************************
//StgShotDataList
//****************************************************************************
StgShotDataList::StgShotDataList() {
	listRenderer_.resize(RENDER_TYPE_COUNT);
	defaultDelayColor_ = 0xffffffff;	//Solid white
}
StgShotDataList::~StgShotDataList() {
	for (std::vector<StgShotRenderer*>& renderList : listRenderer_) {
		for (StgShotRenderer*& renderer : renderList)
			ptr_delete(renderer);
		renderList.clear();
	}
	listRenderer_.clear();

	for (StgShotData*& shotData : listData_)
		ptr_delete(shotData);
	listData_.clear();
}
bool StgShotDataList::AddShotDataList(const std::wstring& path, bool bReload) {
	if (!bReload && listReadPath_.find(path) != listReadPath_.end()) return true;

	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open())
		throw gstd::wexception(L"AddShotDataList: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));
	std::string source = reader->ReadAllString();

	bool res = false;
	Scanner scanner(source);
	try {
		std::vector<StgShotData*> listData;
		std::wstring pathImage = L"";

		DxRect<int> rcDelay(-1, -1, -1, -1);
		DxRect<int> rcDelayDest(-1, -1, -1, -1);

		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::Type::TK_EOF)
				break;
			else if (tok.GetType() == Token::Type::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"ShotData") {
					_ScanShot(listData, scanner);
				}
				else if (element == L"shot_image") {
					scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
					pathImage = scanner.Next().GetString();
				}
				else if (element == L"delay_color") {
					std::vector<std::wstring> list = scanner.GetArgumentList();

					if (list.size() < 3)
						throw wexception("Invalid argument list size (expected 3)");

					defaultDelayColor_ = D3DCOLOR_ARGB(255,
						StringUtility::ToInteger(list[0]),
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]));
				}
				else if (element == L"delay_rect") {
					std::vector<std::wstring> list = scanner.GetArgumentList();

					if (list.size() < 4)
						throw wexception("Invalid argument list size (expected 4)");

					DxRect<int> rect(
						StringUtility::ToInteger(list[0]),
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]),
						StringUtility::ToInteger(list[3]));
					rcDelay = rect;

					LONG width = rect.right - rect.left;
					LONG height = rect.bottom - rect.top;
					DxRect<int> rcDest(-width / 2, -height / 2, width / 2, height / 2);
					if (width % 2 == 1) rcDest.right++;
					if (height % 2 == 1) rcDest.bottom++;
					rcDelayDest = rcDest;
				}
				if (scanner.HasNext())
					tok = scanner.Next();
			}
		}

		//テクスチャ読み込み
		if (pathImage.size() == 0) throw gstd::wexception("Shot texture must be set.");
		std::wstring dir = PathProperty::GetFileDirectory(path);
		pathImage = StringUtility::Replace(pathImage, L"./", dir);
		pathImage = PathProperty::GetUnique(pathImage);

		shared_ptr<Texture> texture(new Texture());
		bool bTexture = texture->CreateFromFile(pathImage, false, false);
		if (!bTexture) throw gstd::wexception("The specified shot texture cannot be found.");

		int textureIndex = -1;
		for (int iTexture = 0; iTexture < listTexture_.size(); iTexture++) {
			shared_ptr<Texture> tSearch = listTexture_[iTexture];
			if (tSearch->GetName() == texture->GetName()) {
				textureIndex = iTexture;
				break;
			}
		}
		if (textureIndex < 0) {
			textureIndex = listTexture_.size();
			listTexture_.push_back(texture);
			for (size_t iRender = 0; iRender < listRenderer_.size(); ++iRender) {
				StgShotRenderer* render = new StgShotRenderer();
				render->SetTexture(texture);
				listRenderer_[iRender].push_back(render);
			}
		}

		if (listData_.size() < listData.size())
			listData_.resize(listData.size());
		for (size_t iData = 0; iData < listData.size(); iData++) {
			StgShotData* data = listData[iData];
			if (data == nullptr) continue;

			data->indexTexture_ = textureIndex;
			{
				shared_ptr<Texture>& texture = listTexture_[data->indexTexture_];
				data->textureSize_.x = texture->GetWidth();
				data->textureSize_.y = texture->GetHeight();
			}

			if (data->delay_.rcSrc_.left < 0) {
				data->delay_.rcSrc_ = rcDelay;
				data->delay_.rcDst_ = rcDelayDest;
			}
			listData_[iData] = data;
		}

		listReadPath_.insert(path);
		Logger::WriteTop(StringUtility::Format(L"Loaded shot data: %s", path.c_str()));
		res = true;
	}
	catch (gstd::wexception& e) {
		std::wstring log = StringUtility::Format(L"Failed to load shot data: [Line=%d] (%s)", scanner.GetCurrentLine(), e.what());
		Logger::WriteTop(log);
		res = false;
	}
	catch (...) {
		std::string log = StringUtility::Format("Failed to load shot data: [Line=%d] (Unknown error.)", scanner.GetCurrentLine());
		Logger::WriteTop(log);
		res = false;
	}

	return res;
}
void StgShotDataList::_ScanShot(std::vector<StgShotData*>& listData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::Type::TK_NEWLINE) tok = scanner.Next();
	scanner.CheckType(tok, Token::Type::TK_OPENC);

	struct Data {
		StgShotData* shotData;
		int id = -1;
	} data;
	data.shotData = new StgShotData(this);
	data.shotData->colorDelay_ = defaultDelayColor_;

	//--------------------------------------------------------------

#define LAMBDA_SETV(m, f) [](Data* i, Scanner& s) { \
		s.CheckType(s.Next(), Token::Type::TK_EQUAL); \
		i->m = s.Next().f(); \
	}
	auto funcSetRect = [](Data* i, Scanner& s) {
		std::vector<std::wstring> list = s.GetArgumentList();

		if (list.size() < 4)
			throw wexception("Invalid argument list size (expected 4)");

		DxRect<LONG> rect(StringUtility::ToInteger(list[0]), StringUtility::ToInteger(list[1]), 
			StringUtility::ToInteger(list[2]), StringUtility::ToInteger(list[3]));

		StgShotData::AnimationData anime;
		anime.rcSrc_ = rect;
		anime.rcDst_ = StgShotData::AnimationData::SetDestRect(&rect);

		i->shotData->listAnime_.resize(1);
		i->shotData->listAnime_[0] = anime;
		i->shotData->totalAnimeFrame_ = 1;
	};
	auto funcSetDelayRect = [](Data* i, Scanner& s) {
		std::vector<std::wstring> list = s.GetArgumentList();

		if (list.size() < 4)
			throw wexception("Invalid argument list size (expected 4)");

		DxRect<LONG> rect(StringUtility::ToInteger(list[0]), StringUtility::ToInteger(list[1]),
			StringUtility::ToInteger(list[2]), StringUtility::ToInteger(list[3]));

		i->shotData->delay_.rcSrc_ = rect;
		i->shotData->delay_.rcDst_ = StgShotData::AnimationData::SetDestRect(&rect);
	};
	auto funcSetDelayColor = [](Data* i, Scanner& s) {
		std::vector<std::wstring> list = s.GetArgumentList();

		if (list.size() < 3)
			throw wexception("Invalid argument list size (expected 3)");

		i->shotData->colorDelay_ = D3DCOLOR_ARGB(255,
			StringUtility::ToInteger(list[0]),
			StringUtility::ToInteger(list[1]),
			StringUtility::ToInteger(list[2]));
	};

	static const std::unordered_map<std::wstring, BlendMode> mapBlendType = {
		{ L"ADD", MODE_BLEND_ADD_RGB },
		{ L"ADD_RGB", MODE_BLEND_ADD_RGB },
		{ L"ADD_ARGB", MODE_BLEND_ADD_ARGB },
		{ L"MULTIPLY", MODE_BLEND_MULTIPLY },
		{ L"SUBTRACT", MODE_BLEND_SUBTRACT },
		{ L"SHADOW", MODE_BLEND_SHADOW },
		{ L"ALPHA_INV", MODE_BLEND_ALPHA_INV },
	};
#define LAMBDA_SETBLEND(m) [](Data* i, Scanner& s) { \
		s.CheckType(s.Next(), Token::Type::TK_EQUAL); \
		BlendMode typeRender = MODE_BLEND_ALPHA; \
		auto itr = mapBlendType.find(s.Next().GetElement()); \
		if (itr != mapBlendType.end()) \
			typeRender = itr->second; \
		i->shotData->m = typeRender; \
	}
	auto funcSetCollision = [](Data* i, Scanner& s) {
		DxCircle circle;
		std::vector<std::wstring> list = s.GetArgumentList();
		if (list.size() == 1) {
			circle.SetR(StringUtility::ToDouble(list[0]));
		}
		else if (list.size() == 3) {
			circle.SetR(StringUtility::ToDouble(list[0]));
			circle.SetX(StringUtility::ToDouble(list[1]));
			circle.SetY(StringUtility::ToDouble(list[2]));
		}
		i->shotData->listCol_ = circle;
	};
	auto funcSetAngularVel = [](Data* i, Scanner& s) {
		s.CheckType(s.Next(), Token::Type::TK_EQUAL);
		Token& tok = s.Next();
		if (tok.GetElement() == L"rand") {
			std::vector<std::wstring> list = s.GetArgumentList(false);
			if (list.size() == 2) {
				i->shotData->angularVelocityMin_ = Math::DegreeToRadian(StringUtility::ToDouble(list[0]));
				i->shotData->angularVelocityMax_ = Math::DegreeToRadian(StringUtility::ToDouble(list[1]));
			}
		}
		else {
			i->shotData->angularVelocityMin_ = Math::DegreeToRadian(tok.GetReal());
			i->shotData->angularVelocityMax_ = i->shotData->angularVelocityMin_;
		}
	};
	auto funcLoadAnimation = [](Data* i, Scanner& s) {
		i->shotData->listAnime_.clear();
		i->shotData->totalAnimeFrame_ = 0;
		_ScanAnimation(i->shotData, s);
	};

	//Do NOT use [&] lambdas
	static const std::unordered_map<std::wstring, std::function<void(Data*, Scanner&)>> mapFunc = {
		{ L"id", LAMBDA_SETV(id, GetInteger) },
		{ L"rect", funcSetRect },
		{ L"delay_rect", funcSetDelayRect },
		{ L"delay_color", funcSetDelayColor },
		{ L"collision", funcSetCollision },
		{ L"render", LAMBDA_SETBLEND(typeRender_) },
		{ L"delay_render", LAMBDA_SETBLEND(typeDelayRender_) },
		{ L"alpha", LAMBDA_SETV(shotData->alpha_, GetInteger) },
		{ L"angular_velocity", funcSetAngularVel },
		{ L"fixed_angle", LAMBDA_SETV(shotData->bFixedAngle_, GetBoolean) },
		{ L"AnimationData", funcLoadAnimation },
	};

#undef LAMBDA_SETV
#undef LAMBDA_CREATELIST
#undef LAMBDA_SETBLEND

	//--------------------------------------------------------------

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::Type::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::Type::TK_ID) {
			std::wstring element = tok.GetElement();

			auto itrFind = mapFunc.find(element);
			if (itrFind != mapFunc.end())
				itrFind->second(&data, scanner);
		}
	}

	if (data.id >= 0) {
		if (data.shotData->listCol_.GetR() <= 0) {
			float r = 0;
			if (data.shotData->listAnime_.size() > 0) {
				DxRect<LONG>& rect = data.shotData->listAnime_[0].rcSrc_;
				int rx = abs(rect.right - rect.left);
				int ry = abs(rect.bottom - rect.top);
				r = std::min(rx, ry) / 3.0f - 3.0f;
			}
			DxCircle circle(0, 0, std::max(r, 2.0f));
			data.shotData->listCol_ = circle;
		}
		if (listData.size() <= data.id)
			listData.resize(data.id + 1);

		listData[data.id] = data.shotData;
	}
}
void StgShotDataList::_ScanAnimation(StgShotData*& shotData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::Type::TK_NEWLINE) tok = scanner.Next();
	scanner.CheckType(tok, Token::Type::TK_OPENC);

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::Type::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::Type::TK_ID) {
			std::wstring element = tok.GetElement();

			if (element == L"animation_data") {
				std::vector<std::wstring> list = scanner.GetArgumentList();

				if (list.size() < 5)
					throw wexception("Invalid argument list size (expected 5)");

				int frame = StringUtility::ToInteger(list[0]);
				DxRect<LONG> rcSrc(StringUtility::ToInteger(list[1]), StringUtility::ToInteger(list[2]),
					StringUtility::ToInteger(list[3]), StringUtility::ToInteger(list[4]));

				StgShotData::AnimationData anime;
				anime.frame_ = frame;
				anime.rcSrc_ = rcSrc;
				anime.rcDst_ = StgShotData::AnimationData::SetDestRect(&rcSrc);

				shotData->listAnime_.push_back(anime);
				shotData->totalAnimeFrame_ += frame;
			}
		}
	}
}

//*******************************************************************
//StgShotData
//*******************************************************************
StgShotData::StgShotData(StgShotDataList* listShotData) {
	listShotData_ = listShotData;

	indexTexture_ = -1;
	textureSize_ = D3DXVECTOR2(0, 0);

	typeRender_ = MODE_BLEND_ALPHA;
	typeDelayRender_ = MODE_BLEND_ADD_ARGB;

	alpha_ = 255;

	delay_.rcSrc_ = DxRect<LONG>(-1, -1, -1, -1);
	colorDelay_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	
	totalAnimeFrame_ = 0;

	angularVelocityMin_ = 0;
	angularVelocityMax_ = 0;
	bFixedAngle_ = false;
}
StgShotData::~StgShotData() {
}
StgShotRenderer* StgShotData::GetRenderer(BlendMode blendType) {
	if (blendType < MODE_BLEND_ALPHA || blendType > MODE_BLEND_ALPHA_INV)
		return listShotData_->GetRenderer(indexTexture_, 0);
	return listShotData_->GetRenderer(indexTexture_, blendType - 1);
}

StgShotData::AnimationData* StgShotData::GetData(size_t frame) {
	if (totalAnimeFrame_ <= 1U)
		return &listAnime_[0];

	frame = frame % totalAnimeFrame_;
	size_t total = 0;

	for (auto itr = listAnime_.begin(); itr != listAnime_.end(); ++itr) {
		total += itr->frame_;
		if (total >= frame)
			return &(*itr);
	}
	return &listAnime_[0];
}
DxRect<float> StgShotData::AnimationData::SetDestRect(DxRect<LONG>* src) {
	LONG rw = src->GetWidth();
	LONG rh = src->GetHeight();
	float width = rw / 2.0f;
	float height = rh / 2.0f;
	return DxRect<float>(-width + 0.5f, -height + 0.5f, width + 0.5f, height + 0.5f);
}

//****************************************************************************
//StgShotRenderer
//****************************************************************************
StgShotRenderer::StgShotRenderer() {
	countRenderVertex_ = 0U;
	countMaxVertex_ = 8192U;
	countRenderIndex_ = 0U;
	countMaxIndex_ = 16384U;

	SetVertexCount(countMaxVertex_);
	vecIndex_.resize(countMaxIndex_);
}
StgShotRenderer::~StgShotRenderer() {

}
void StgShotRenderer::Render(StgShotManager* manager) {
	if (countRenderIndex_ < 3) return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	IDirect3DTexture9* pTexture = texture_ ? texture_->GetD3DTexture() : nullptr;
	device->SetTexture(0, pTexture);
	//device->SetTexture(1, pTexture);

	VertexBufferManager* bufferManager = VertexBufferManager::GetBase();

	GrowableVertexBuffer* vertexBuffer = bufferManager->GetGrowableVertexBuffer();
	GrowableIndexBuffer* indexBuffer = bufferManager->GetGrowableIndexBuffer();
	vertexBuffer->Expand(countMaxVertex_);
	indexBuffer->Expand(countMaxIndex_);

	{
		BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

		lockParam.SetSource(vertex_, countRenderVertex_, sizeof(VERTEX_TLX));
		vertexBuffer->UpdateBuffer(&lockParam);

		lockParam.SetSource(vecIndex_, countRenderIndex_, sizeof(uint32_t));
		indexBuffer->UpdateBuffer(&lockParam);
	}

	device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_TLX));
	device->SetIndices(indexBuffer->GetBuffer());

	device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, countRenderVertex_, 0, countRenderIndex_ / 3U);

	countRenderVertex_ = 0U;
	countRenderIndex_ = 0U;
}
void StgShotRenderer::AddSquareVertex(VERTEX_TLX* listVertex) {
	TryExpandVertex(countRenderVertex_ + 4U);
	memcpy((VERTEX_TLX*)vertex_.data() + countRenderVertex_, listVertex, strideVertexStreamZero_ * 4U);

	TryExpandIndex(countRenderIndex_ + 6U);
	vecIndex_[countRenderIndex_ + 0] = countRenderVertex_ + 0;
	vecIndex_[countRenderIndex_ + 1] = countRenderVertex_ + 2;
	vecIndex_[countRenderIndex_ + 2] = countRenderVertex_ + 1;
	vecIndex_[countRenderIndex_ + 3] = countRenderVertex_ + 1;
	vecIndex_[countRenderIndex_ + 4] = countRenderVertex_ + 2;
	vecIndex_[countRenderIndex_ + 5] = countRenderVertex_ + 3;

	countRenderVertex_ += 4U;
	countRenderIndex_ += 6U;
}
void StgShotRenderer::AddSquareVertex_CurveLaser(VERTEX_TLX* listVertex, bool bAddIndex) {
	TryExpandVertex(countRenderVertex_ + 2U);
	memcpy((VERTEX_TLX*)vertex_.data() + countRenderVertex_, listVertex, strideVertexStreamZero_ * 2U);

	if (bAddIndex) {
		TryExpandIndex(countRenderIndex_ + 6U);
		vecIndex_[countRenderIndex_ + 0] = countRenderVertex_ + 0;
		vecIndex_[countRenderIndex_ + 1] = countRenderVertex_ + 2;
		vecIndex_[countRenderIndex_ + 2] = countRenderVertex_ + 1;
		vecIndex_[countRenderIndex_ + 3] = countRenderVertex_ + 1;
		vecIndex_[countRenderIndex_ + 4] = countRenderVertex_ + 2;
		vecIndex_[countRenderIndex_ + 5] = countRenderVertex_ + 3;
		countRenderIndex_ += 6U;
	}

	countRenderVertex_ += 2U;
}

//****************************************************************************
//StgShotObject
//****************************************************************************
StgShotObject::StgShotObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;

	frameWork_ = 0;
	posX_ = 0;
	posY_ = 0;
	idShotData_ = 0;
	SetBlendType(MODE_BLEND_NONE);

	damage_ = 1;
	life_ = 1;
	bAutoDelete_ = true;
	bEraseShot_ = false;
	bSpellFactor_ = false;
	bSpellResist_ = false;

	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	frameGrazeInvalid_ = 0;
	frameGrazeInvalidStart_ = -1;

	frameFadeDelete_ = -1;
	frameAutoDelete_ = INT_MAX;

	typeOwner_ = OWNER_ENEMY;

	pShotIntersectionTarget_ = nullptr;
	bUserIntersectionMode_ = false;
	bIntersectionEnable_ = true;
	bChangeItemEnable_ = true;

	bEnableMotionDelay_ = false;
	bRoundingPosition_ = false;

	hitboxScale_ = D3DXVECTOR2(1.0f, 1.0f);

	timerTransform_ = 0;
	timerTransformNext_ = 0;

	int priShotI = stageController_->GetStageInformation()->GetShotObjectPriority();
	SetRenderPriorityI(priShotI);
}
StgShotObject::~StgShotObject() {
}
void StgShotObject::SetOwnObjectReference() {
	auto ptr = ref_unsync_ptr<StgShotObject>::Cast(stageController_->GetMainRenderObject(idObject_));
	pOwnReference_ = ptr;
}
void StgShotObject::Work() {
}
void StgShotObject::_Move() {
	if (delay_.time == 0 || bEnableMotionDelay_)
		StgMoveObject::_Move();
	SetX(posX_);
	SetY(posY_);

	if (pattern_) {
		int idShot = pattern_->GetShotDataID();
		if (idShot != StgMovePattern::NO_CHANGE) {
			SetShotDataID(idShot);
		}
	}
}
void StgShotObject::_DeleteInLife() {
	if (IsDeleted() || life_ > 0) return;

	_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_IMMEDIATE);

	auto objectManager = stageController_->GetMainObjectManager();
	auto scriptManager = stageController_->GetScriptManager();

	if (scriptManager != nullptr && typeOwner_ == StgShotObject::OWNER_PLAYER) {
		float posX = GetPositionX();
		float posY = GetPositionY();
		LOCK_WEAK (scriptPlayer, scriptManager->GetPlayerScript()) {
			float listPos[2] = { posX, posY };

			value listScriptValue[3];
			listScriptValue[0] = scriptPlayer->CreateIntValue(idObject_);
			listScriptValue[1] = scriptPlayer->CreateRealArrayValue(listPos, 2U);
			listScriptValue[2] = scriptPlayer->CreateIntValue(GetShotDataID());
			scriptPlayer->RequestEvent(StgStagePlayerScript::EV_DELETE_SHOT_PLAYER, listScriptValue, 3);
		}
	}

	objectManager->DeleteObject(this);
}
void StgShotObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;

	StgShotManager* shotManager = stageController_->GetShotManager();

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetShotManager()->GetShotDeleteClip();

	if ((LONG)posX_ < rcClipBase->left || (LONG)posX_ > rcStgFrame->GetWidth() + rcClipBase->right
		|| (LONG)posY_ < rcClipBase->top || (LONG)posY_ > rcStgFrame->GetHeight() + rcClipBase->bottom)
	{
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgShotObject::_DeleteInFadeDelete() {
	if (IsDeleted()) return;
	if (frameFadeDelete_ == 0) {
		_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_FADE);
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgShotObject::_DeleteInAutoDeleteFrame() {
	if (IsDeleted() || delay_.time > 0) return;

	if (frameAutoDelete_ <= 0)
		SetFadeDelete();
	else --frameAutoDelete_;
}
void StgShotObject::_CommonWorkTask() {
	if (bEnableMovement_) {
		++frameWork_;
		if (frameFadeDelete_ >= 0) --frameFadeDelete_;
		_DeleteInLife();
		_DeleteInAutoClip();
		_DeleteInAutoDeleteFrame();
		_DeleteInFadeDelete();
	}
	--frameGrazeInvalid_;
}
void StgShotObject::_SendDeleteEvent(int bit) {
	if (typeOwner_ != OWNER_ENEMY) return;

	StgShotManager* shotManager = stageController_->GetShotManager();
	if (!shotManager->IsDeleteEventEnable(bit)) return;

	LOCK_WEAK(scriptShot, stageController_->GetScriptManager()->GetShotScript()) {
		double listPos[2] = { GetPositionX(), GetPositionY() };

		int typeEvent = 0;
		switch (bit) {
		case StgShotManager::BIT_EV_DELETE_IMMEDIATE:
			typeEvent = StgStageShotScript::EV_DELETE_SHOT_IMMEDIATE;
			break;
		case StgShotManager::BIT_EV_DELETE_TO_ITEM:
			typeEvent = StgStageShotScript::EV_DELETE_SHOT_TO_ITEM;
			break;
		case StgShotManager::BIT_EV_DELETE_FADE:
			typeEvent = StgStageShotScript::EV_DELETE_SHOT_FADE;
			break;
		}

		value listScriptValue[4];
		listScriptValue[0] = ManagedScript::CreateIntValue(idObject_);
		listScriptValue[1] = ManagedScript::CreateRealArrayValue(listPos, 2U);
		listScriptValue[2] = ManagedScript::CreateBooleanValue(false);
		listScriptValue[3] = ManagedScript::CreateIntValue(GetShotDataID());
		scriptShot->RequestEvent(typeEvent, listScriptValue, 4);
	}
}

void StgShotObject::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	ref_unsync_weak_ptr<StgIntersectionObject> obj = otherTarget->GetObject();

	StgIntersectionTarget::Type otherType = otherTarget->GetTargetType();
	switch (otherType) {
	case StgIntersectionTarget::TYPE_PLAYER:
	{
		if (frameGrazeInvalid_ <= 0)
			frameGrazeInvalid_ = frameGrazeInvalidStart_ > 0 ? 
			frameGrazeInvalidStart_ : INT_MAX;
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SHOT:
	{
		if (obj) {
			if (StgShotObject* shot = dynamic_cast<StgShotObject*>(obj.get())) {
				bool bEraseShot = shot->IsEraseShot();
				if (bEraseShot && !bSpellResist_)
					ConvertToItem(false);
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SPELL:
	{
		if (obj) {
			if (StgPlayerSpellObject* spell = dynamic_cast<StgPlayerSpellObject*>(obj.get())) {
				bool bEraseShot = spell->IsEraseShot();
				if (bEraseShot && !bSpellResist_)
					ConvertToItem(false);
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_ENEMY:
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		//Don't reduce penetration with lasers
		if (!bSpellResist_ && dynamic_cast<StgLaserObject*>(this) == nullptr) {
			--life_;
		}
		break;
	}
	}
}
StgShotData* StgShotObject::_GetShotData(int id) {
	StgShotData* res = nullptr;
	StgShotManager* shotManager = stageController_->GetShotManager();
	StgShotDataList* dataList = (typeOwner_ == OWNER_PLAYER) ?
		shotManager->GetPlayerShotDataList() : shotManager->GetEnemyShotDataList();

	if (dataList) res = dataList->GetData(id);
	return res;
}

void StgShotObject::_SetVertexPosition(VERTEX_TLX& vertex, float x, float y, float z, float w) {
	constexpr float bias = -0.5f;
	vertex.position.x = x + bias;
	vertex.position.y = y + bias;
	vertex.position.z = z;
	vertex.position.w = w;
}
void StgShotObject::_SetVertexUV(VERTEX_TLX& vertex, float u, float v) {
	vertex.texcoord.x = u;
	vertex.texcoord.y = v;
}
void StgShotObject::_SetVertexColorARGB(VERTEX_TLX& vertex, D3DCOLOR color) {
	vertex.diffuse_color = color;
}
void StgShotObject::SetAlpha(int alpha) {
	ColorAccess::ClampColor(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
void StgShotObject::SetColor(int r, int g, int b) {
	__m128i c = Vectorize::Set(color_ >> 24, r, g, b);
	color_ = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));
}

void StgShotObject::ConvertToItem(bool flgPlayerCollision) {
	if (IsDeleted()) return;

	if (bChangeItemEnable_) {
		_ConvertToItemAndSendEvent(flgPlayerCollision);
		_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_TO_ITEM);
	}

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}
void StgShotObject::DeleteImmediate() {
	if (IsDeleted()) return;

	_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_IMMEDIATE);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

void StgShotObject::_ProcessTransformAct() {
	if (listTransformationShotAct_.size() == 0) return;

	if (timerTransform_ == 0) timerTransform_ = delay_.time;
	while (timerTransform_ == frameWork_ && listTransformationShotAct_.size() > 0) {
		StgPatternShotTransform& transform = listTransformationShotAct_.front();

		switch (transform.act) {
		case StgPatternShotTransform::TRANSFORM_WAIT:
			timerTransform_ += std::max((int)transform.param[0], 0);
			break;
		case StgPatternShotTransform::TRANSFORM_ADD_SPEED_ANGLE:
		{
			int duration = transform.param[0];
			int delay = transform.param[1];
			double accel = transform.param[2];
			double agvel = transform.param[3];

			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, accel));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL,
					Math::DegreeToRadian(agvel)));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX2, accel * duration));
				AddPattern(delay, pattern, true);
			}
			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, 0));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(delay + duration, pattern, true);
			}
			break;
		}
		case StgPatternShotTransform::TRANSFORM_ANGULAR_MOVE:
		{
			int duration = transform.param[0];
			double agvel = transform.param[1];
			double spin = transform.param[2];

			StgNormalShotObject* shot = (StgNormalShotObject*)this;
			if (shot)
				shot->angularVelocity_ = Math::DegreeToRadian(spin);

			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL,
					Math::DegreeToRadian(agvel)));
				AddPattern(0, pattern, true);
			}
			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(duration, pattern, true);
			}

			break;
		}
		case StgPatternShotTransform::TRANSFORM_N_DECEL_CHANGE:
		{
			int timer = transform.param[0];
			int countRep = transform.param[1];
			int typeChange = transform.param[2];
			double changeSpeed = transform.param[3];
			double changeAngle = transform.param[4];

			timerTransform_ += timer * countRep;

			for (int framePattern = 0; countRep > 0; --countRep, framePattern += timer) {
				double nowSpeed = GetSpeed();

				{
					ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPEED, nowSpeed));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, -nowSpeed / timer));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, 0));
					AddPattern(framePattern, pattern, true);
				}

				{
					ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPEED, changeSpeed));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, 0));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, 0));

					double angleArgument = Math::NormalizeAngleRad(Math::DegreeToRadian(changeAngle));
					switch (typeChange) {
					case 0:
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ANGLE, angleArgument));
						break;
					case 1:
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::ADD_ANGLE, angleArgument));
						break;
					case 2:
					{
						auto objPlayer = stageController_->GetPlayerObject();
						if (objPlayer)
							pattern->SetRelativeObject(objPlayer);
						shared_ptr<RandProvider> rand = stageController_->GetStageInformation()->GetRandProvider();
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ANGLE,
							rand->GetReal(-angleArgument, angleArgument)));
						break;
					}
					case 3:
					{
						auto objPlayer = stageController_->GetPlayerObject();
						if (objPlayer)
							pattern->SetRelativeObject(objPlayer);
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ANGLE, angleArgument));
						break;
					}
					case 4:
					{
						shared_ptr<RandProvider> rand = stageController_->GetStageInformation()->GetRandProvider();
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ANGLE,
							rand->GetReal(0, GM_PI_X2)));
						break;
					}
					case 5:
					default:
						//pattern->SetDirectionAngle(StgMovePattern::NO_CHANGE);
						break;
					}

					AddPattern(framePattern + timer, pattern, true);
				}
			}

			break;
		}
		case StgPatternShotTransform::TRANSFORM_GRAPHIC_CHANGE:
		{
			idShotData_ = transform.param[0];
			break;
		}
		case StgPatternShotTransform::TRANSFORM_BLEND_CHANGE:
		{
			SetBlendType((BlendMode)transform.param[0]);
			break;
		}
		case StgPatternShotTransform::TRANSFORM_TO_SPEED_ANGLE:
		{
			int duration = transform.param[0];
			double targetSpeed = transform.param[1];
			double targetAngle = transform.param[2];

			double nowSpeed = GetSpeed();
			double nowAngle = GetDirectionAngle();

			if (targetAngle == StgMovePattern::TOPLAYER_CHANGE) {
				ref_unsync_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
				if (objPlayer)
					targetAngle = atan2(objPlayer->GetY() - GetPositionY(), objPlayer->GetX() - GetPositionX());
			}
			else if (targetAngle == StgMovePattern::NO_CHANGE) {
				targetAngle = nowAngle;
			}
			else
				targetAngle = Math::DegreeToRadian(targetAngle);

			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
				if (targetSpeed != StgMovePattern::NO_CHANGE) {
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL,
						(targetSpeed - nowSpeed) / duration));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, targetSpeed));
				}
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL,
					Math::AngleDifferenceRad(nowAngle, targetAngle) / duration));
				AddPattern(0, pattern, true);
			}
			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, 0));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(duration, pattern, true);
			}

			break;
		}

#define ADD_CMD(__cmd, __arg) if (__arg != StgMovePattern::NO_CHANGE) \
								pattern->AddCommand(std::make_pair(__cmd, __arg));
#define ADD_CMD2(__cmd, __target, __arg) if (__target != StgMovePattern::NO_CHANGE) \
								pattern->AddCommand(std::make_pair(__cmd, __arg));
		case StgPatternShotTransform::TRANSFORM_ADDPATTERN_A1:
		{
			int time = transform.param[0];

			double speed = transform.param[1];
			double angle = transform.param[2];

			ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);
			pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ZERO, 0));

			ADD_CMD(StgMovePattern_Angle::SET_SPEED, speed);
			ADD_CMD2(StgMovePattern_Angle::SET_ANGLE, angle, Math::DegreeToRadian(angle));

			AddPattern(time, pattern, true);
			break;
		}
		case StgPatternShotTransform::TRANSFORM_ADDPATTERN_A2:
		{
			int time = transform.param[0];

			double speed = transform.param[1];
			double angle = transform.param[2];

			double accel = transform.param[3];
			double agvel = transform.param[4];
			double maxsp = transform.param[5];

			int shotID = transform.param[6];
			int relativeObj = transform.param[7];

			ref_unsync_ptr<StgMovePattern_Angle> pattern = new StgMovePattern_Angle(this);

			ADD_CMD(StgMovePattern_Angle::SET_SPEED, speed);
			ADD_CMD2(StgMovePattern_Angle::SET_ANGLE, angle, Math::DegreeToRadian(angle));
			ADD_CMD(StgMovePattern_Angle::SET_ACCEL, accel);
			ADD_CMD2(StgMovePattern_Angle::SET_AGVEL, agvel, Math::DegreeToRadian(agvel));
			ADD_CMD(StgMovePattern_Angle::SET_SPMAX, maxsp);

			if (shotID != StgMovePattern::NO_CHANGE)
				pattern->SetShotDataID(shotID);
			if (relativeObj != DxScript::ID_INVALID)
				pattern->SetRelativeObject(relativeObj);

			AddPattern(time, pattern, true);
			break;
		}
		case StgPatternShotTransform::TRANSFORM_ADDPATTERN_B1:
		{
			int time = transform.param[0];

			double speedX = transform.param[1];
			double speedY = transform.param[2];

			ref_unsync_ptr<StgMovePattern_XY> pattern = new StgMovePattern_XY(this);
			pattern->AddCommand(std::make_pair(StgMovePattern_XY::SET_ZERO, 0));

			ADD_CMD(StgMovePattern_XY::SET_S_X, speedX);
			ADD_CMD(StgMovePattern_XY::SET_S_Y, speedY);

			AddPattern(time, pattern, true);
			break;
		}
		case StgPatternShotTransform::TRANSFORM_ADDPATTERN_B2:
		{
			int time = transform.param[0];

			double speedX = transform.param[1];
			double speedY = transform.param[2];
			double accelX = transform.param[3];
			double accelY = transform.param[4];
			double maxspX = transform.param[5];
			double maxspY = transform.param[6];

			int shotID = transform.param[7];

			ref_unsync_ptr<StgMovePattern_XY> pattern = new StgMovePattern_XY(this);

			ADD_CMD(StgMovePattern_XY::SET_S_X, speedX);
			ADD_CMD(StgMovePattern_XY::SET_S_Y, speedY);
			ADD_CMD(StgMovePattern_XY::SET_A_X, accelX);
			ADD_CMD(StgMovePattern_XY::SET_A_Y, accelY);
			ADD_CMD(StgMovePattern_XY::SET_M_X, maxspX);
			ADD_CMD(StgMovePattern_XY::SET_M_Y, maxspY);

			if (shotID != StgMovePattern::NO_CHANGE)
				pattern->SetShotDataID(shotID);

			AddPattern(time, pattern, true);
			break;
		}
#undef ADD_CMD
#undef ADD_CMD2
		default:
			break;
		}

		listTransformationShotAct_.pop_front();
	}
}

//StgShotObject::DelayParameter
float StgShotObject::DelayParameter::_CalculateValue(D3DXVECTOR3* param, lerp_func func) {
	switch (type) {
	case DELAY_LERP:
		return func(param->x, param->y, time / param->z);
	case DELAY_DEFAULT:
	default:
		return std::min(param->x + time / param->z, param->y);
	}
}

//****************************************************************************
//StgNormalShotObject
//****************************************************************************
StgNormalShotObject::StgNormalShotObject(StgStageController* stageController) : StgShotObject(stageController) {
	typeObject_ = TypeObject::Shot;
	angularVelocity_ = 0;
	bFixedAngle_ = false;

	move_ = D3DXVECTOR2(1, 0);
	lastAngle_ = 0;

	pShotIntersectionTarget_ = new StgIntersectionTarget_Circle();
	listIntersectionTarget_.push_back(pShotIntersectionTarget_);
}
StgNormalShotObject::~StgNormalShotObject() {

}
void StgNormalShotObject::Work() {
	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();

		if (delay_.time > 0) {
			--(delay_.time);
			delay_.angle += delay_.spin;
		}

		{
			angle_.z += angularVelocity_;

			bool bDelay = delay_.time > 0 && delay_.spin != 0;

			double angleZ = bDelay ? delay_.angle : angle_.z;
			if (StgShotData* shotData = _GetShotData()) {
				if (!bFixedAngle_ && !bDelay) angleZ += GetDirectionAngle() + Math::DegreeToRadian(90);
			}
			if (angleZ != lastAngle_) {
				move_ = D3DXVECTOR2(cosf(angleZ), sinf(angleZ));
				lastAngle_ = angleZ;
			}
		}
	}

	_CommonWorkTask();
}

void StgNormalShotObject::_AddIntersectionRelativeTarget() {
	if (IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0) return;
	ClearIntersected();

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	std::vector<ref_unsync_ptr<StgIntersectionTarget>> listTarget = GetIntersectionTargetList();
	for (auto& iTarget : listTarget) {
		intersectionManager->AddTarget(iTarget);
	}

	//RegistIntersectionRelativeTarget(intersectionManager);
}
std::vector<ref_unsync_ptr<StgIntersectionTarget>> StgNormalShotObject::GetIntersectionTargetList() {
	if ((IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_) || pOwnReference_.expired())
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	DxCircle* orgCircle = shotData->GetIntersectionCircleList();
	StgIntersectionTarget_Circle* target = (StgIntersectionTarget_Circle*)pShotIntersectionTarget_.get();
	{
		DxCircle& circle = target->GetCircle();

		float intersectMeanScale = (hitboxScale_.x + hitboxScale_.y) / 2.0f;

		if (orgCircle->GetX() != 0 || orgCircle->GetY() != 0) {
			__m128 v1 = Vectorize::Mul(
				Vectorize::SetF(orgCircle->GetX(), orgCircle->GetY(), orgCircle->GetX(), orgCircle->GetY()),
				Vectorize::Set(move_.x, move_.y, move_.y, move_.x));
			float px = (v1.m128_f32[0] + v1.m128_f32[1]) * intersectMeanScale;
			float py = (v1.m128_f32[2] - v1.m128_f32[3]) * intersectMeanScale;
			circle.SetX(px + posX_);
			circle.SetY(py + posY_);
		}
		else {
			circle.SetX(posX_);
			circle.SetY(posY_);
		}
		circle.SetR(orgCircle->GetR() * intersectMeanScale);

		target->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(pOwnReference_);
		target->SetIntersectionSpace();

		listIntersectionTarget_[0] = pShotIntersectionTarget_;
	}
	return listIntersectionTarget_;
}

void StgNormalShotObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	StgShotData* delayData = delay_.id >= 0 ? _GetShotData(delay_.id) : shotData;
	if (shotData == nullptr || delayData == nullptr) return;

	StgShotRenderer* renderer = nullptr;

	BlendMode shotBlendType = MODE_BLEND_ALPHA;
	if (delay_.time > 0) {
		BlendMode objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == MODE_BLEND_NONE) {
			renderer = delayData->GetRenderer(shotData->GetDelayRenderType());
		}
		else {
			renderer = delayData->GetRenderer(objDelayBlendType);
		}
	}
	else {
		BlendMode objBlendType = GetBlendType();
		if (objBlendType == MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer();
			shotBlendType = shotData->GetRenderType();
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
	}

	if (renderer == nullptr) return;

	D3DXVECTOR2* textureSize = &shotData->GetTextureSize();

	float scaleX = 1.0f;
	float scaleY = 1.0f;

	DxRect<LONG>* rcSrc = nullptr;
	DxRect<float>* rcDest = nullptr;
	D3DCOLOR color;

	if (delay_.time > 0) {
		float expa = delay_.GetScale();
		scaleX = expa;
		scaleY = expa;

		if (delay_.id >= 0) {
			textureSize = &delayData->GetTextureSize();

			StgShotData::AnimationData* anime = delayData->GetData(frameWork_);
			rcSrc = anime->GetSource();
			rcDest = anime->GetDest();
		}
		else {
			rcSrc = delayData->GetDelayRect();
			rcDest = delayData->GetDelayDest();
		}

		color = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
		if (delay_.colorMix) ColorAccess::MultiplyColor(color, color_);
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * delay_.GetAlpha());
			color = (color & 0x00ffffff) | (alpha << 24);
		}
	}
	else {
		scaleX = scale_.x;
		scaleY = scale_.y;

		StgShotData::AnimationData* anime = shotData->GetData(frameWork_);
		rcSrc = anime->GetSource();
		rcDest = anime->GetDest();

		color = color_;

		float alphaRate = shotData->GetAlpha() / 255.0f;
		if (frameFadeDelete_ >= 0) alphaRate *= (float)frameFadeDelete_ / FRAME_FADEDELETE;
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * alphaRate);
			color = (color & 0x00ffffff) | (alpha << 24);
		}
	}

	//if (bIntersected_) color = D3DCOLOR_ARGB(255, 255, 0, 0);

	FLOAT sposx = position_.x;
	FLOAT sposy = position_.y;
	if (bRoundingPosition_) {
		sposx = roundf(sposx);
		sposy = roundf(sposy);
	}

	VERTEX_TLX verts[4];
	LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
	float* ptrDst = reinterpret_cast<float*>(rcDest);
	for (size_t iVert = 0U; iVert < 4U; ++iVert) {
		//((iVert & 1) << 1)
		//   0 -> 0
		//   1 -> 2
		//   2 -> 0
		//   3 -> 2
		//(iVert | 1)
		//   0 -> 1
		//   1 -> 1
		//   2 -> 3
		//   3 -> 3

		VERTEX_TLX vt;
		_SetVertexUV(vt, ptrSrc[(iVert & 1) << 1], ptrSrc[iVert | 1]);
		_SetVertexPosition(vt, ptrDst[(iVert & 1) << 1], ptrDst[iVert | 1], position_.z);
		_SetVertexColorARGB(vt, color);
		verts[iVert] = vt;
	}
	D3DXVECTOR2 texSizeInv = D3DXVECTOR2(1.0f / textureSize->x, 1.0f / textureSize->y);
	DxMath::TransformVertex2D(verts, &D3DXVECTOR2(scaleX, scaleY), &move_, &D3DXVECTOR2(sposx, sposy), &texSizeInv);

	renderer->AddSquareVertex(verts);
}

void StgNormalShotObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();

	float posX = GetPositionX();
	float posY = GetPositionY();
	LOCK_WEAK(itemScript, stageScriptManager->GetItemScript()) {
		float listPos[2] = { posX, posY };

		gstd::value listScriptValue[4];
		listScriptValue[0] = itemScript->CreateIntValue(idObject_);
		listScriptValue[1] = itemScript->CreateRealArrayValue(listPos, 2U);
		listScriptValue[2] = itemScript->CreateBooleanValue(flgPlayerCollision);
		listScriptValue[3] = itemScript->CreateIntValue(GetShotDataID());
		itemScript->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
	}

	if (itemManager->IsDefaultBonusItemEnable() && !flgPlayerCollision) {
		if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
			ref_unsync_ptr<StgItemObject> obj = new StgItemObject_Bonus(stageController_);
			auto objectManager = stageController_->GetMainObjectManager();
			int id = objectManager->AddObject(obj);
			if (id != DxScript::ID_INVALID) {
				itemManager->AddItem(obj);
				obj->SetPositionX(posX);
				obj->SetPositionY(posY);
			}
		}
	}
}
void StgNormalShotObject::SetShotDataID(int id) {
	StgShotData* oldData = _GetShotData();
	StgShotObject::SetShotDataID(id);

	StgShotData* shotData = _GetShotData();
	if (shotData != nullptr && oldData != shotData) {
		if (angularVelocity_ != 0) {
			angularVelocity_ = 0;
			angle_.z = 0;
		}

		double avMin = shotData->GetAngularVelocityMin();
		double avMax = shotData->GetAngularVelocityMax();
		bFixedAngle_ = shotData->IsFixedAngle();
		if (avMin != 0 || avMax != 0) {
			ref_count_ptr<StgStageInformation> stageInfo = stageController_->GetStageInformation();
			shared_ptr<RandProvider> rand = stageInfo->GetRandProvider();
			angularVelocity_ = rand->GetReal(avMin, avMax);
		}
	}
}

//****************************************************************************
//StgLaserObject(レーザー基本部)
//****************************************************************************
StgLaserObject::StgLaserObject(StgStageController* stageController) : StgShotObject(stageController) {
	life_ = 9999999;
	bSpellResist_ = true;

	length_ = 0;
	lengthF_ = 0;
	widthRender_ = 0;
	widthIntersection_ = -1;
	extendRate_ = 0;
	maxLength_ = 0;
	invalidLengthStart_ = 0.1f;
	invalidLengthEnd_ = 0.1f;
	frameGrazeInvalidStart_ = 20;
	itemDistance_ = 24;

	delay_ = DelayParameter(0.5, 3.5, 15.0f);

	move_ = D3DXVECTOR2(1, 0);
	lastAngle_ = 0;
}
void StgLaserObject::_AddIntersectionRelativeTarget() {
	if ((delay_.time > 0 && !bEnableMotionDelay_) || frameFadeDelete_ >= 0) return;
	ClearIntersected();

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	std::vector<ref_unsync_ptr<StgIntersectionTarget>> listTarget = GetIntersectionTargetList();
	for (auto& iTarget : listTarget)
		intersectionManager->AddTarget(iTarget);
}
void StgLaserObject::_ExtendLength() {
	if (extendRate_ != 0) {
		lengthF_ += extendRate_;

		if (extendRate_ > 0)
			lengthF_ = std::min(lengthF_, (float)maxLength_);
		if (extendRate_ < 0)
			lengthF_ = std::max(lengthF_, (float)maxLength_);

		length_ = (int)lengthF_;
	}
}

//****************************************************************************
//StgLooseLaserObject(射出型レーザー)
//****************************************************************************
StgLooseLaserObject::StgLooseLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::LooseLaser;

	pShotIntersectionTarget_ = new StgIntersectionTarget_Line();
	listIntersectionTarget_.push_back(pShotIntersectionTarget_);
}
void StgLooseLaserObject::Work() {
	if (frameWork_ == 0) {
		posXE_ = posXO_ = posX_;
		posYE_ = posYO_ = posY_;
	}

	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();
		

		if (delay_.time > 0) {
			--(delay_.time);
			delay_.angle += delay_.spin;
		}
		else _ExtendLength();
	}

	_CommonWorkTask();
	//	_AddIntersectionRelativeTarget();
}
void StgLooseLaserObject::_Move() {
	if (delay_.time == 0 || bEnableMotionDelay_)
		StgMoveObject::_Move();
	DxScriptRenderObject::SetX(posX_);
	DxScriptRenderObject::SetY(posY_);

	double angleZ = GetDirectionAngle();

	if (delay_.time <= 0 || bEnableMotionDelay_) {
		__m128 v1 = Vectorize::Sub(
			Vectorize::SetF(posXE_, posYE_, length_, 0),
			Vectorize::SetF(posX_, posY_, 0, 0));
		v1 = Vectorize::Mul(v1, v1);
		if ((v1.m128_f32[0] + v1.m128_f32[1]) > v1.m128_f32[2]) {
			float speed = GetSpeed();
			posXE_ += speed * move_.x;
			posYE_ += speed * move_.y;
		}

		
	}
	if (lastAngle_ != angleZ) {
		lastAngle_ = angleZ;
		move_ = D3DXVECTOR2(cosf(lastAngle_), sinf(lastAngle_));
	}
}
void StgLooseLaserObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetShotManager()->GetShotDeleteClip();

	LONG rcLeft = rcClipBase->left;
	LONG rcTop = rcClipBase->top;
	LONG rcRight = rcStgFrame->GetWidth() + rcClipBase->right;
	LONG rcBottom = rcStgFrame->GetHeight() + rcClipBase->bottom;

	bool bDelete = false;

#ifdef __L_MATH_VECTORIZE
	__m128i rc_pos = Vectorize::SetI(posX_, posXE_, posY_, posYE_);
	//SSE2
	__m128i res = _mm_cmplt_epi32(rc_pos, 
		Vectorize::SetI(rcLeft, rcLeft, rcTop, rcTop));
	bDelete = (res.m128i_i32[0] && res.m128i_i32[1]) || (res.m128i_i32[2] && res.m128i_i32[3]);
	if (!bDelete) {
		res = _mm_cmpgt_epi32(rc_pos, 
			Vectorize::SetI(rcRight, rcRight, rcBottom, rcBottom));
		bDelete = (res.m128i_i32[0] && res.m128i_i32[1]) || (res.m128i_i32[2] && res.m128i_i32[3]);
	}
#else
	bDelete = (posX_ < rcLeft && posXE_ < rcLeft) || (posX_ > rcRight && posXE_ > rcRight)
		|| (posY_ < rcTop && posYE_ < rcTop) || (posY_ > rcBottom && posYE_ > rcBottom);
#endif

	if (bDelete) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}

std::vector<ref_unsync_ptr<StgIntersectionTarget>> StgLooseLaserObject::GetIntersectionTargetList() {
	if ((IsDeleted() || (delay_.time > 0 && !bEnableMotionDelay_) || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_)
		|| (pOwnReference_.expired() || widthIntersection_ == 0))
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	float invLengthS = (1.0f - (1.0f - invalidLengthStart_) * hitboxScale_.y) * 0.5f;
	float invLengthE = (1.0f - (1.0f - invalidLengthEnd_) * hitboxScale_.y) * 0.5f;

	float lineXS = Math::Lerp::Linear((float)posX_, posXE_, invLengthS);
	float lineYS = Math::Lerp::Linear((float)posY_, posYE_, invLengthS);
	float lineXE = Math::Lerp::Linear(posXE_, (float)posX_, invLengthE);
	float lineYE = Math::Lerp::Linear(posYE_, (float)posY_, invLengthE);

	StgIntersectionTarget_Line* target = (StgIntersectionTarget_Line*)pShotIntersectionTarget_.get();
	{
		DxWidthLine& line = target->GetLine();
		line = DxWidthLine(lineXS, lineYS, lineXE, lineYE, widthIntersection_ * hitboxScale_.x);

		target->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(pOwnReference_);
		target->SetIntersectionSpace();

		listIntersectionTarget_[0] = pShotIntersectionTarget_;
	}
	return listIntersectionTarget_;
}

void StgLooseLaserObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	StgShotData* delayData = delay_.id >= 0 ? _GetShotData(delay_.id) : shotData;
	if (shotData == nullptr || delayData == nullptr) return;

	D3DXVECTOR2* textureSize = nullptr;

	float scaleX = 1.0f;
	float scaleY = 1.0f;
	D3DXVECTOR2 renderF = D3DXVECTOR2(1, 0);

	FLOAT sposx = position_.x;
	FLOAT sposy = position_.y;

	DxRect<LONG>* rcSrc = nullptr;
	DxRect<float> rcDest;
	D3DCOLOR color;

#define RENDER_VERTEX \
	VERTEX_TLX verts[4]; \
	LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);\
	float* ptrDst = reinterpret_cast<float*>(&rcDest);\
	for (size_t iVert = 0U; iVert < 4U; ++iVert) {\
		VERTEX_TLX vt; \
		_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1], ptrSrc[iVert | 0b1]); \
		_SetVertexPosition(vt, ptrDst[iVert | 0b1], ptrDst[(iVert & 0b1) << 1], position_.z); \
		_SetVertexColorARGB(vt, color); \
		verts[iVert] = vt; \
	} \
	D3DXVECTOR2 texSizeInv = D3DXVECTOR2(1.0f / textureSize->x, 1.0f / textureSize->y); \
	DxMath::TransformVertex2D(verts, &D3DXVECTOR2(scaleX, scaleY), &renderF, &D3DXVECTOR2(sposx, sposy), &texSizeInv); \
	renderer->AddSquareVertex(verts);


	if (delay_.time > 0) {
		textureSize = &delayData->GetTextureSize();

		StgShotRenderer* renderer = nullptr;

		BlendMode objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == MODE_BLEND_NONE) {
			renderer = delayData->GetRenderer(MODE_BLEND_ADD_ARGB);
		}
		else {
			renderer = delayData->GetRenderer(objDelayBlendType);
		}

		if (renderer == nullptr) return;

		float expa = delay_.GetScale();
		scaleX = expa;
		scaleY = expa;

		renderF = (delay_.spin != 0) ? D3DXVECTOR2(cosf(delay_.angle), sinf(delay_.angle)) : move_;

		if (bEnableMotionDelay_) {
			sposx = posXO_;
			sposy = posYO_;
		}

		if (delay_.id >= 0) {
			StgShotData::AnimationData* anime = delayData->GetData(frameWork_);
			rcSrc = anime->GetSource();
			rcDest = *anime->GetDest();
		}
		else {
			rcSrc = delayData->GetDelayRect();
			rcDest = *delayData->GetDelayDest();
		}

		color = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
		if (delay_.colorMix) ColorAccess::MultiplyColor(color, color_);
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * delay_.GetAlpha());
			color = (color & 0x00ffffff) | (alpha << 24);
		}

		if (bRoundingPosition_) {
			sposx = roundf(sposx);
			sposy = roundf(sposy);
		}

		RENDER_VERTEX;
	}



	if (delay_.time == 0 || bEnableMotionDelay_) {
		textureSize = &shotData->GetTextureSize();

		StgShotRenderer* renderer = nullptr;

		BlendMode objBlendType = GetBlendType();
		if (objBlendType == MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(MODE_BLEND_ADD_ARGB);
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}

		if (renderer == nullptr) return;

		sposx = position_.x;
		sposy = position_.y;
		scaleX = scale_.x;
		scaleY = scale_.y;

		float dx = posXE_ - posX_;
		float dy = posYE_ - posY_;
		float radius = hypotf(dx, dy);

		renderF = D3DXVECTOR2(dx, dy) / radius;

		
		StgShotData::AnimationData* anime = shotData->GetData(frameWork_);
		rcSrc = anime->GetSource();

		color = color_;

		float alphaRate = shotData->GetAlpha() / 255.0f;
		if (frameFadeDelete_ >= 0) alphaRate *= (float)frameFadeDelete_ / FRAME_FADEDELETE;
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * alphaRate);
			color = (color & 0x00ffffff) | (alpha << 24);
		}

		//color = ColorAccess::ApplyAlpha(color, alpha);
		rcDest.Set(widthRender_ / 2, 0, -widthRender_ / 2, radius);

		RENDER_VERTEX;
	}
#undef RENDER_VERTEX
	
}
void StgLooseLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();

	shared_ptr<ManagedScript> itemScript = stageScriptManager->GetItemScript().lock();

	int ex = GetPositionX();
	int ey = GetPositionY();

	float dx = posXE_ - posX_;
	float dy = posYE_ - posY_;
	float length = hypotf(dx, dy);

	float listPos[2];
	gstd::value listScriptValue[4];

	for (float itemPos = 0; itemPos < length; itemPos += itemDistance_) {
		float posX = ex - itemPos * move_.x;
		float posY = ey - itemPos * move_.y;

		if (itemScript) {
			listPos[0] = posX;
			listPos[1] = posY;

			listScriptValue[0] = itemScript->CreateIntValue(idObject_);
			listScriptValue[1] = itemScript->CreateRealArrayValue(listPos, 2U);
			listScriptValue[2] = itemScript->CreateBooleanValue(flgPlayerCollision);
			listScriptValue[3] = itemScript->CreateIntValue(GetShotDataID());
			itemScript->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
		}

		if (itemManager->IsDefaultBonusItemEnable() && (delay_.time == 0 || bEnableMotionDelay_) && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				ref_unsync_ptr<StgItemObject> obj = new StgItemObject_Bonus(stageController_);
				int id = stageController_->GetMainObjectManager()->AddObject(obj);
				if (id != DxScript::ID_INVALID) {
					itemManager->AddItem(obj);
					obj->SetPositionX(posX);
					obj->SetPositionY(posY);
				}
			}
		}
	}
}

//****************************************************************************
//StgStraightLaserObject(設置型レーザー)
//****************************************************************************
StgStraightLaserObject::StgStraightLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::StraightLaser;

	angLaser_ = 0;
	angVelLaser_ = 0;
	frameFadeDelete_ = -1;

	bUseSouce_ = true;
	bUseEnd_ = false;
	idImageEnd_ = -1;

	delaySize_ = D3DXVECTOR2(1, 1);

	scaleX_ = 0.05f;

	bLaserExpand_ = true;

	move_ = D3DXVECTOR2(1, 0);

	pShotIntersectionTarget_ = new StgIntersectionTarget_Line();
	listIntersectionTarget_.push_back(pShotIntersectionTarget_);
}
void StgStraightLaserObject::Work() {
	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();
		_ExtendLength();

		if (delay_.spin != 0) delay_.angle += delay_.spin;
		
		if (angVelLaser_ != 0) angLaser_ += angVelLaser_;

		if (delay_.time > 0) --(delay_.time);
		else {
			if (bLaserExpand_)
				scaleX_ = std::min(1.0f, scaleX_ + 0.1f);
		}

		if (lastAngle_ != angLaser_) {
			lastAngle_ = angLaser_;
			move_ = D3DXVECTOR2(cosf(angLaser_), sinf(angLaser_));
		}
	}

	_CommonWorkTask();
}

void StgStraightLaserObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetShotManager()->GetShotDeleteClip();

	LONG rcLeft = rcClipBase->left;
	LONG rcTop = rcClipBase->top;
	LONG rcRight = rcStgFrame->GetWidth() + rcClipBase->right;
	LONG rcBottom = rcStgFrame->GetHeight() + rcClipBase->bottom;

	bool bDelete = false;

#ifdef __L_MATH_VECTORIZE
	__m128 v_pos = Vectorize::Set(posX_, posY_, 0.0f, 0.0f);
	v_pos = Vectorize::MulAdd(Vectorize::SetF(length_, length_, 0.0f, 0.0f),
		Vectorize::Set(move_.x, move_.y, 0.0f, 0.0f), v_pos);
	__m128i rc_pos = Vectorize::SetI(posX_, v_pos.m128_f32[0], posY_, v_pos.m128_f32[1]);
	//SSE2
	__m128i res = _mm_cmplt_epi32(rc_pos, 
		Vectorize::SetI(rcLeft, rcLeft, rcTop, rcTop));
	bDelete = (res.m128i_i32[0] && res.m128i_i32[1]) || (res.m128i_i32[2] && res.m128i_i32[3]);
	if (!bDelete) {
		res = _mm_cmpgt_epi32(rc_pos,
			Vectorize::SetI(rcRight, rcRight, rcBottom, rcBottom));
		bDelete = (res.m128i_i32[0] && res.m128i_i32[1]) || (res.m128i_i32[2] && res.m128i_i32[3]);
	}
#else
	int posXE = posX_ + (int)(length_ * move_.x);
	int posYE = posY_ + (int)(length_ * move_.y);
	bDelete = (posX_ < rcLeft && posXE < rcLeft) || (posX_ > rcRight && posXE > rcRight)
		|| (posY_ < rcTop && posYE < rcTop) || (posY_ > rcBottom && posYE > rcBottom);
#endif

	if (bDelete) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
std::vector<ref_unsync_ptr<StgIntersectionTarget>> StgStraightLaserObject::GetIntersectionTargetList() {
	std::vector<ref_unsync_ptr<StgIntersectionTarget>> res;

	if ((IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_)
		|| (pOwnReference_.expired() || widthIntersection_ == 0)
		|| (scaleX_ < 1.0 && typeOwner_ != OWNER_PLAYER))
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	float length = length_ * hitboxScale_.y;
	__m128 v1 = Vectorize::Mul(
		Vectorize::SetF(length, length, invalidLengthStart_, invalidLengthEnd_),
		Vectorize::Set(move_.x, move_.y, 0.5f, 0.5f));
	float _posXE = posX_ + v1.m128_f32[0];
	float _posYE = posY_ + v1.m128_f32[1];
	float lineXS = Math::Lerp::Linear((float)posX_, _posXE, v1.m128_f32[2]);
	float lineYS = Math::Lerp::Linear((float)posY_, _posYE, v1.m128_f32[2]);
	float lineXE = Math::Lerp::Linear(_posXE, (float)posX_, v1.m128_f32[3]);
	float lineYE = Math::Lerp::Linear(_posYE, (float)posY_, v1.m128_f32[3]);

	StgIntersectionTarget_Line* target = (StgIntersectionTarget_Line*)pShotIntersectionTarget_.get();
	{
		DxWidthLine& line = target->GetLine();
		line = DxWidthLine(lineXS, lineYS, lineXE, lineYE, widthIntersection_ * hitboxScale_.x);

		target->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(pOwnReference_);
		target->SetIntersectionSpace();

		listIntersectionTarget_[0] = pShotIntersectionTarget_;
	}
	return listIntersectionTarget_;
}
void StgStraightLaserObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return;

	D3DXVECTOR2* textureSize = &shotData->GetTextureSize();

	FLOAT sposx = position_.x;
	FLOAT sposy = position_.y;

	DxRect<LONG>* rcSrc = nullptr;
	D3DCOLOR color;

	BlendMode objBlendType = GetBlendType();
	BlendMode shotBlendType = objBlendType;
	{
		StgShotRenderer* renderer = nullptr;
		if (objBlendType == MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(MODE_BLEND_ADD_ARGB);
			shotBlendType = MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
		if (renderer == nullptr) return;

		StgShotData::AnimationData* anime = shotData->GetData(frameWork_);
		rcSrc = anime->GetSource();
		//rcDest = anime->rcDst_;
		color = color_;

		float alphaRate = shotData->GetAlpha() / 255.0f;
		if (frameFadeDelete_ >= 0) alphaRate *= (float)frameFadeDelete_ / FRAME_FADEDELETE;
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * alphaRate);
			color = (color & 0x00ffffff) | (alpha << 24);
		}

		if (widthRender_ > 0) {
			float _rWidth = fabs(widthRender_ / 2.0f) * scaleX_;
			_rWidth = std::max(_rWidth, 0.5f);
			D3DXVECTOR4 rcDest(_rWidth, length_, -_rWidth, 0);

			VERTEX_TLX verts[4];
			LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
			FLOAT* ptrDst = reinterpret_cast<FLOAT*>(&rcDest);
			for (size_t iVert = 0U; iVert < 4U; ++iVert) {
				VERTEX_TLX vt;

				_SetVertexUV(vt, ptrSrc[(iVert & 1) << 1] / textureSize->x, ptrSrc[iVert | 1] / textureSize->y);
				_SetVertexPosition(vt, ptrDst[(iVert & 1) << 1], ptrDst[iVert | 1]);
				_SetVertexColorARGB(vt, color);

				float px = vt.position.x * scale_.x;
				float py = vt.position.y * scale_.y;

				vt.position.x = (px * move_.y + py * move_.x) + sposx;
				vt.position.y = (-px * move_.x + py * move_.y) + sposy;
				vt.position.z = position_.z;

				verts[iVert] = vt;
			}

			renderer->AddSquareVertex(verts);
		}
	}

	{
		BlendMode objSourceBlendType = GetSourceBlendType();

		if ((bUseSouce_ || bUseEnd_) && (frameFadeDelete_ < 0)) {	//Delay cloud(s)
			color = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
			if (delay_.colorMix) ColorAccess::MultiplyColor(color, color_);

			int sourceWidth = widthRender_ * 2 / 3;
			DxRect<float> rcDest(-sourceWidth + 0.5f, -sourceWidth + 0.5f, sourceWidth + 0.5f, sourceWidth + 0.5f);

			auto _AddDelay = [&](StgShotData* delayShotData, DxRect<LONG>* delayRect, D3DXVECTOR2& delayPos, float delaySize) {
				StgShotRenderer* renderer = nullptr;

				if (objSourceBlendType == MODE_BLEND_NONE)
					renderer = delayShotData->GetRenderer(shotBlendType);
				else
					renderer = delayShotData->GetRenderer(objSourceBlendType);
				if (renderer == nullptr) return;

				VERTEX_TLX verts[4];
				LONG* ptrSrc = reinterpret_cast<LONG*>(delayRect);
				float* ptrDst = reinterpret_cast<float*>(&rcDest);

				D3DXVECTOR2 move = (delay_.spin != 0) ? D3DXVECTOR2(cosf(delay_.angle), sinf(delay_.angle)) : move_;
				

				for (size_t iVert = 0U; iVert < 4U; ++iVert) {
					VERTEX_TLX vt;

					_SetVertexUV(vt, ptrSrc[(iVert & 1) << 1] / delayShotData->GetTextureSize().x,
						ptrSrc[iVert | 1] / delayShotData->GetTextureSize().y);
					_SetVertexPosition(vt, ptrDst[(iVert & 1) << 1], ptrDst[iVert | 1]);
					_SetVertexColorARGB(vt, color);

					float px = vt.position.x * delaySize;
					float py = vt.position.y * delaySize;
					vt.position.x = (py * move.x + px * move.y) + delayPos.x;
					vt.position.y = (py * move.y - px * move.x) + delayPos.y;
					vt.position.z = position_.z;

					//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
					verts[iVert] = vt;
				}

				renderer->AddSquareVertex(verts);
			};

			if (bUseSouce_) {
				D3DXVECTOR2 delayPos = D3DXVECTOR2(sposx, sposy);
				if (bRoundingPosition_) {
					delayPos.x = roundf(delayPos.x);
					delayPos.y = roundf(delayPos.y);
				}

				StgShotData* delayData = nullptr;
				DxRect<LONG>* delayRect = nullptr;

				if (delay_.id >= 0) {
					delayData = _GetShotData(delay_.id);
					if (delayData == nullptr) return;
					StgShotData::AnimationData* anime = delayData->GetData(frameWork_);
					delayRect = anime->GetSource();
				}
				else {
					delayData = shotData;
					delayRect = shotData->GetDelayRect();
				}

				_AddDelay(delayData, delayRect, delayPos, delaySize_.x);
			}
			if (bUseEnd_) {
				D3DXVECTOR2 delayPos = D3DXVECTOR2(sposx + length_ * cosf(angLaser_), sposy + length_ * sinf(angLaser_));
				if (bRoundingPosition_) {
					delayPos.x = roundf(delayPos.x);
					delayPos.y = roundf(delayPos.y);
				}

				StgShotData* delayData = nullptr;
				DxRect<LONG>* delayRect = nullptr;

				if (idImageEnd_ >= 0) {
					delayData = _GetShotData(idImageEnd_);
					if (delayData == nullptr) return;
					StgShotData::AnimationData* anime = delayData->GetData(frameWork_);
					delayRect = anime->GetSource();
				}
				else {
					delayData = shotData;
					delayRect = shotData->GetDelayRect();
				}

				_AddDelay(delayData, delayRect, delayPos, delaySize_.y);
			}
		}
	}
}
void StgStraightLaserObject::_AddIntersectionRelativeTarget() {
	if (delay_.time > 0 || frameFadeDelete_ >= 0) return;
	ClearIntersected();

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	std::vector<ref_unsync_ptr<StgIntersectionTarget>> listTarget = GetIntersectionTargetList();
	for (auto& iTarget : listTarget)
		intersectionManager->AddTarget(iTarget);
}
void StgStraightLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();

	shared_ptr<ManagedScript> itemScript = stageScriptManager->GetItemScript().lock();

	float listPos[2];
	gstd::value listScriptValue[4];

	for (float itemPos = 0; itemPos < (float)length_; itemPos += itemDistance_) {
		float itemX = posX_ + itemPos * move_.x;
		float itemY = posY_ + itemPos * move_.y;

		if (itemScript) {
			listPos[0] = itemX;
			listPos[1] = itemY;

			listScriptValue[0] = itemScript->CreateIntValue(idObject_);
			listScriptValue[1] = itemScript->CreateRealArrayValue(listPos, 2U);
			listScriptValue[2] = itemScript->CreateBooleanValue(flgPlayerCollision);
			listScriptValue[3] = itemScript->CreateIntValue(GetShotDataID());
			itemScript->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
		}

		if (itemManager->IsDefaultBonusItemEnable() && delay_.time == 0 && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				ref_unsync_ptr<StgItemObject> obj = new StgItemObject_Bonus(stageController_);
				int id = stageController_->GetMainObjectManager()->AddObject(obj);
				if (id != DxScript::ID_INVALID) {
					itemManager->AddItem(obj);
					obj->SetPositionX(itemX);
					obj->SetPositionY(itemY);
				}
			}
		}
	}
}

//****************************************************************************
//StgCurveLaserObject(曲がる型レーザー)
//****************************************************************************
StgCurveLaserObject::StgCurveLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::CurveLaser;
	tipDecrement_ = 0.0f;

	invalidLengthStart_ = 0.02f;
	invalidLengthEnd_ = 0.02f;

	itemDistance_ = 6.0f;

	pShotIntersectionTarget_ = nullptr;
}
void StgCurveLaserObject::Work() {
	if (frameWork_ == 0) {
		posXO_ = posX_;
		posYO_ = posY_;
	}

	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();

		if (delay_.time > 0) {
			--(delay_.time);
			delay_.angle += delay_.spin;
		}
		// else _ExtendLength();
	}

	_CommonWorkTask();
	//	_AddIntersectionRelativeTarget();
}
void StgCurveLaserObject::_Move() {
	if (delay_.time == 0 || bEnableMotionDelay_)
		StgMoveObject::_Move();
	DxScriptRenderObject::SetX(posX_);
	DxScriptRenderObject::SetY(posY_);

	double angleZ = GetDirectionAngle();

	{
		if (lastAngle_ != angleZ) {
			lastAngle_ = angleZ;
			move_ = D3DXVECTOR2(cosf(lastAngle_), sinf(lastAngle_));
		}
		D3DXVECTOR2 newNodePos(posX_, posY_);
		D3DXVECTOR2 newNodeVertF(-move_.y, move_.x);	//90 degrees rotation
		PushNode(CreateNode(newNodePos, newNodeVertF));
	}
}

StgCurveLaserObject::LaserNode StgCurveLaserObject::CreateNode(const D3DXVECTOR2& pos, const D3DXVECTOR2& rFac, D3DCOLOR col) {
	LaserNode node;
	node.pos = pos;
	{
		float wRender = widthRender_ / 2.0f;

		float nx = wRender * rFac.x;
		float ny = wRender * rFac.y;
		node.vertOff[0] = { nx, ny };
		node.vertOff[1] = { -nx, -ny };
	}
	node.color = col;
	return node;
}
bool StgCurveLaserObject::GetNode(size_t indexNode, std::list<LaserNode>::iterator& res) {
	//I wish there was a better way to do this.
	size_t listSizeMax = listPosition_.size();
	if (indexNode >= listSizeMax) res = listPosition_.end();
	if (indexNode < listSizeMax / 2U)
		res = std::next(listPosition_.begin(), indexNode);
	else
		res = std::next(listPosition_.rbegin(), listSizeMax - indexNode).base();
	return res != listPosition_.end();
}
void StgCurveLaserObject::GetNodePointerList(std::vector<LaserNode*>* listRes) {
	listRes->resize(listPosition_.size(), nullptr);
	size_t i = 0;
	for (LaserNode& iNode : listPosition_) {
		(*listRes)[i++] = &iNode;
	}
}
std::list<StgCurveLaserObject::LaserNode>::iterator StgCurveLaserObject::PushNode(const LaserNode& node) {
	listPosition_.push_front(node);
	if (listPosition_.size() > length_)
		listPosition_.pop_back();
	return listPosition_.begin();
}

void StgCurveLaserObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetShotManager()->GetShotDeleteClip();
	LONG rcRight = rcStgFrame->GetWidth() + rcClipBase->right;
	LONG rcBottom = rcStgFrame->GetHeight() + rcClipBase->bottom;

	//Checks if the node is within the bounding rect
	auto PredicateNodeInRect = [&](LaserNode& node) {
		D3DXVECTOR2* pos = &node.pos;
		bool bInX = pos->x >= rcClipBase->left && pos->x <= rcRight;
		bool bInY = pos->y >= rcClipBase->top && pos->y <= rcBottom;
		return bInX && bInY;
	};

	std::list<LaserNode>::iterator itrFind = std::find_if(listPosition_.begin(), listPosition_.end(),
		PredicateNodeInRect);

	//Can't find any node within the bounding rect
	if (itrFind == listPosition_.end()) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
std::vector<ref_unsync_ptr<StgIntersectionTarget>> StgCurveLaserObject::GetIntersectionTargetList() {
	if ((IsDeleted() || (delay_.time > 0 && !bEnableMotionDelay_) || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_)
		|| (pOwnReference_.expired() || widthIntersection_ == 0))
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return std::vector<ref_unsync_ptr<StgIntersectionTarget>>();

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	size_t countPos = listPosition_.size();
	size_t countIntersection = countPos > 0U ? countPos - 1U : 0U;

	if (countIntersection > 0U) {
		float iLengthS = invalidLengthStart_ * 0.5f;
		float iLengthE = 0.5f + (1.0f - invalidLengthEnd_) * 0.5f;
		int posInvalidS = (int)(countPos * iLengthS);
		int posInvalidE = (int)(countPos * iLengthE);
		float iWidth = widthIntersection_ * hitboxScale_.x;

		listIntersectionTarget_.resize(countIntersection, nullptr);
		//std::fill(listIntersectionTarget_.begin(), listIntersectionTarget_.end(), nullptr);

		std::list<LaserNode>::iterator itr = listPosition_.begin();
		for (size_t iPos = 0; iPos < countIntersection; ++iPos, ++itr) {
			ref_unsync_ptr<StgIntersectionTarget>& target = listIntersectionTarget_[iPos];
			if ((int)iPos < posInvalidS || (int)iPos > posInvalidE) {
				if (target)
					target->SetIntersectionSpace(DxRect<LONG>());
				continue;
			}

			std::list<LaserNode>::iterator itrNext = std::next(itr);
			D3DXVECTOR2* nodeS = &itr->pos;
			D3DXVECTOR2* nodeE = &itrNext->pos;

			if (target == nullptr) {
				target = new StgIntersectionTarget_Line();
			}
			{
				StgIntersectionTarget_Line* pTarget = (StgIntersectionTarget_Line*)target.get();
				DxWidthLine& line = pTarget->GetLine();
				line = DxWidthLine(nodeS->x, nodeS->y, nodeE->x, nodeE->y, iWidth);

				pTarget->SetTargetType(typeOwner_ == OWNER_PLAYER ?
					StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
				target->SetObject(pOwnReference_);
				pTarget->SetIntersectionSpace();
			}
		}
	}

	return listIntersectionTarget_;
}

void StgCurveLaserObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	StgShotData* delayData = delay_.id >= 0 ? _GetShotData(delay_.id) : shotData;
	if (shotData == nullptr) return;

	BlendMode shotBlendType = MODE_BLEND_ADD_ARGB;
	StgShotRenderer* renderer = nullptr;

	if (delayData != nullptr && delay_.time > 0) {
		BlendMode objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == MODE_BLEND_NONE) {
			renderer = delayData->GetRenderer(MODE_BLEND_ADD_ARGB);
			shotBlendType = MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = delayData->GetRenderer(objDelayBlendType);
		}
		if (renderer == nullptr) return;

		DxRect<LONG>* rcSrc = nullptr;
		DxRect<float>* rcDest = nullptr;
		D3DXVECTOR2* delaySize = &delayData->GetTextureSize();

		if (delay_.id >= 0) {
			StgShotData::AnimationData* anime = delayData->GetData(frameWork_);
			rcSrc = anime->GetSource();
			rcDest = anime->GetDest();
		}
		else {
			rcSrc = shotData->GetDelayRect();
			rcDest = shotData->GetDelayDest();
		}

		float expa = delay_.GetScale();

		FLOAT sX = posXO_;
		FLOAT sY = posYO_;
		if (bRoundingPosition_) {
			sX = roundf(sX);
			sY = roundf(sY);
		}

		D3DCOLOR color = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
		if (delay_.colorMix) ColorAccess::MultiplyColor(color, color_);
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * delay_.GetAlpha());
			color = (color & 0x00ffffff) | (alpha << 24);
		}

		VERTEX_TLX verts[4];
		LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
		float* ptrDst = reinterpret_cast<float*>(rcDest);

		D3DXVECTOR2 move = (delay_.spin != 0) ? D3DXVECTOR2(cosf(delay_.angle), sinf(delay_.angle)) : move_;

		for (size_t iVert = 0U; iVert < 4U; ++iVert) {
			VERTEX_TLX vt;
			_SetVertexUV(vt, ptrSrc[(iVert & 1) << 1], ptrSrc[iVert | 1]);
			_SetVertexPosition(vt, ptrDst[(iVert & 1) << 1], ptrDst[iVert | 1], position_.z);
			_SetVertexColorARGB(vt, color);
			verts[iVert] = vt;
		}
		D3DXVECTOR2 delaySizeInv = D3DXVECTOR2(1.0f / delaySize->x, 1.0f / delaySize->y);
		DxMath::TransformVertex2D(verts, &D3DXVECTOR2(expa, expa), &move, &D3DXVECTOR2(sX, sY), &delaySizeInv);

		renderer->AddSquareVertex(verts);
	}
	if (listPosition_.size() > 1U) {
		BlendMode objBlendType = GetBlendType();
		BlendMode shotBlendType = objBlendType;
		if (objBlendType == MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(MODE_BLEND_ADD_ARGB);
			shotBlendType = MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
		if (renderer == nullptr) return;

		D3DXVECTOR2* textureSize = &shotData->GetTextureSize();
		StgShotData::AnimationData* anime = shotData->GetData(frameWork_);

		//---------------------------------------------------

		size_t countPos = listPosition_.size();
		size_t countRect = countPos - 1U;
		size_t halfPos = countRect / 2U;

		float alphaRateShot = shotData->GetAlpha() / 255.0f;
		if (frameFadeDelete_ >= 0) alphaRateShot *= (float)frameFadeDelete_ / FRAME_FADEDELETE;

		float baseAlpha = (color_ >> 24) & 0xff;
		float tipAlpha = baseAlpha * (1.0f - tipDecrement_);

		D3DXVECTOR2 texSizeInv = D3DXVECTOR2(1.0f / textureSize->x, 1.0f / textureSize->y);

		DxRect<LONG>* rcSrcOrg = anime->GetSource();
		float rcInc = ((rcSrcOrg->bottom - rcSrcOrg->top) / (float)countRect) * texSizeInv.y;
		float rectV = rcSrcOrg->top * texSizeInv.y;

		LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrcOrg);

		size_t iPos = 0U;
		for (auto itr = listPosition_.begin(); itr != listPosition_.end(); ++itr, ++iPos) {
			float nodeAlpha = baseAlpha;
			if (iPos > halfPos)
				nodeAlpha = Math::Lerp::Linear(baseAlpha, tipAlpha, (iPos - halfPos + 1) / (float)halfPos);
			else if (iPos < halfPos)
				nodeAlpha = Math::Lerp::Linear(tipAlpha, baseAlpha, iPos / (halfPos - 1.0f));
			nodeAlpha = std::max(0.0f, nodeAlpha);

			D3DCOLOR thisColor = color_;
			{
				byte alpha = ColorAccess::ClampColorRet(nodeAlpha * alphaRateShot);
				thisColor = (thisColor & 0x00ffffff) | (alpha << 24);
			}
			if (itr->color != 0xffffffff) ColorAccess::MultiplyColor(thisColor, itr->color);

			VERTEX_TLX verts[2];
			for (size_t iVert = 0U; iVert < 2U; ++iVert) {
				VERTEX_TLX vt;

				_SetVertexUV(vt, ptrSrc[(iVert & 1) << 1] * texSizeInv.x, rectV);
				_SetVertexPosition(vt, itr->pos.x + itr->vertOff[iVert].x,
					itr->pos.y + itr->vertOff[iVert].y, position_.z);
				_SetVertexColorARGB(vt, thisColor);

				verts[iVert] = vt;
			}
			renderer->AddSquareVertex_CurveLaser(verts, std::next(itr) != listPosition_.end());

			rectV += rcInc;
		}
	}
}
void StgCurveLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();

	shared_ptr<ManagedScript> itemScript = stageScriptManager->GetItemScript().lock();

	gstd::value listScriptValue[4];
	if (itemScript) {
		listScriptValue[0] = itemScript->CreateIntValue(idObject_);
		listScriptValue[2] = itemScript->CreateBooleanValue(flgPlayerCollision);
		listScriptValue[3] = itemScript->CreateIntValue(GetShotDataID());
	}

	size_t countToItem = 0U;

	auto RequestItem = [&](float ix, float iy) {
		if (itemScript) {
			float listPos[2] = { ix, iy };
			listScriptValue[1] = itemScript->CreateRealArrayValue(listPos, 2U);
			itemScript->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
		}
		if (itemManager->IsDefaultBonusItemEnable() && (delay_.time == 0 || bEnableMotionDelay_) && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				ref_unsync_ptr<StgItemObject> obj = new StgItemObject_Bonus(stageController_);
				if (stageController_->GetMainObjectManager()->AddObject(obj) != DxScript::ID_INVALID) {
					itemManager->AddItem(obj);
					obj->SetPositionX(ix);
					obj->SetPositionY(iy);
				}
			}
		}
		++countToItem;
	};

	float lengthAcc = 0.0;
	for (std::list<LaserNode>::iterator itr = listPosition_.begin(); itr != listPosition_.end(); itr++) {
		std::list<LaserNode>::iterator itrNext = std::next(itr);
		if (itrNext == listPosition_.end()) break;

		D3DXVECTOR2* pos = &itr->pos;
		D3DXVECTOR2* posNext = &itrNext->pos;
		float nodeDist = hypotf(posNext->x - pos->x, posNext->y - pos->y);
		lengthAcc += nodeDist;

		float createDist = 0U;
		while (lengthAcc >= itemDistance_) {
			float lerpMul = (itemDistance_ >= nodeDist) ? 0.0 : (createDist / nodeDist);
			RequestItem(Math::Lerp::Linear(pos->x, posNext->x, lerpMul),
				Math::Lerp::Linear(pos->y, posNext->y, lerpMul));
			createDist += itemDistance_;
			lengthAcc -= itemDistance_;
		}
	}
}


//****************************************************************************
//StgPatternShotObjectGenerator (ECL-style bullets firing)
//****************************************************************************
StgPatternShotObjectGenerator::StgPatternShotObjectGenerator() {
	typeObject_ = TypeObject::ShotPattern;

	idShotData_ = -1;
	typeOwner_ = StgShotObject::OWNER_ENEMY;
	typePattern_ = PATTERN_TYPE_FAN;
	typeShot_ = TypeObject::Shot;
	iniBlendType_ = MODE_BLEND_NONE;

	shotWay_ = 1U;
	shotStack_ = 1U;

	basePointX_ = BASEPOINT_RESET;
	basePointY_ = BASEPOINT_RESET;
	basePointOffsetX_ = 0;
	basePointOffsetY_ = 0;
	fireRadiusOffset_ = 0;

	speedBase_ = 1;
	speedArgument_ = 1;
	angleBase_ = 0;
	angleArgument_ = 0;

    extra_ = 0;

	delay_ = 0;
	//delayMove_ = false;

	laserWidth_ = 16;
	laserLength_ = 64;
}
StgPatternShotObjectGenerator::~StgPatternShotObjectGenerator() {
}

void StgPatternShotObjectGenerator::CopyFrom(StgPatternShotObjectGenerator* other) {
	parent_ = other->parent_;
	listTransformation_ = other->listTransformation_;

	idShotData_ = other->idShotData_;
	//typeOwner_ = other->typeOwner_;
	typeShot_ = other->typeShot_;
	typePattern_ = other->typePattern_;
	iniBlendType_ = other->iniBlendType_;

	shotWay_ = other->shotWay_;
	shotStack_ = other->shotStack_;

	basePointX_ = other->basePointX_;
	basePointY_ = other->basePointY_;
	basePointOffsetX_ = other->basePointOffsetX_;
	basePointOffsetY_ = other->basePointOffsetY_;
	fireRadiusOffset_ = other->fireRadiusOffset_;

	speedBase_ = other->speedBase_;
	speedArgument_ = other->speedArgument_;
	angleBase_ = other->angleBase_;
	angleArgument_ = other->angleArgument_;

	delay_ = other->delay_;
	//delayMove_ = other->delayMove_;

	laserWidth_ = other->laserWidth_;
	laserLength_ = other->laserLength_;
}
void StgPatternShotObjectGenerator::SetTransformation(size_t off, StgPatternShotTransform& entry) {
	if (off >= listTransformation_.size()) listTransformation_.resize(off + 1);
	listTransformation_[off] = entry;
}

void StgPatternShotObjectGenerator::FireSet(void* scriptData, StgStageController* controller, std::vector<int>* idVector) {
	if (idVector) idVector->clear();

	StgStageScript* script = (StgStageScript*)scriptData;
	ref_unsync_ptr<StgPlayerObject> objPlayer = controller->GetPlayerObject();
	StgStageScriptObjectManager* objManager = controller->GetMainObjectManager();
	StgShotManager* shotManager = controller->GetShotManager();
	shared_ptr<RandProvider> randGenerator = controller->GetStageInformation()->GetRandProvider();

	if (idShotData_ < 0) return;
	if (shotWay_ == 0U || shotStack_ == 0U) return;

	float basePosX = basePointX_;
	float basePosY = basePointY_;
	if (!parent_.expired()) {
		if (basePointX_ == BASEPOINT_RESET)
			basePosX = parent_->GetPositionX();
		if (basePointY_ == BASEPOINT_RESET)
			basePosY = parent_->GetPositionY();
	}
	basePosX += basePointOffsetX_;
	basePosY += basePointOffsetY_;

	std::list<StgPatternShotTransform> transformAsList;
	for (StgPatternShotTransform& iTransform : listTransformation_)
		transformAsList.push_back(iTransform);

	auto __CreateShot = [&](float _x, float _y, float _ss, float _sa) -> bool {
		if (shotManager->GetShotCountAll() >= StgShotManager::SHOT_MAX) return false;

		ref_unsync_ptr<StgShotObject> objShot;
		switch (typeShot_) {
		case TypeObject::Shot:
		{
			ref_unsync_ptr<StgNormalShotObject> ptrShot = new StgNormalShotObject(controller);
			objShot = ptrShot;
			break;
		}
		case TypeObject::LooseLaser:
		{
			ref_unsync_ptr<StgLooseLaserObject> ptrShot = new StgLooseLaserObject(controller);
			ptrShot->SetLength(laserLength_);
			ptrShot->SetRenderWidth(laserWidth_);
			objShot = ptrShot;
			break;
		}
		case TypeObject::CurveLaser:
		{
			ref_unsync_ptr<StgCurveLaserObject> ptrShot = new StgCurveLaserObject(controller);
			ptrShot->SetLength(laserLength_);
			ptrShot->SetRenderWidth(laserWidth_);
			objShot = ptrShot;
			break;
		}
		}

		if (objShot == nullptr) return false;

		objShot->SetX(_x);
		objShot->SetY(_y);
		objShot->SetSpeed(_ss);
		objShot->SetDirectionAngle(_sa);
		objShot->SetShotDataID(idShotData_);
		objShot->SetDelay(delay_);
		objShot->SetOwnerType(typeOwner_);

		objShot->SetTransformList(transformAsList);

		objShot->SetBlendType(iniBlendType_);
		//objShot->SetEnableDelayMotion(delayMove_);

		int idRes = script->AddObject(objShot);
		if (idRes == DxScript::ID_INVALID) return false;

		shotManager->AddShot(objShot);

		if (idVector) idVector->push_back(idRes);
		return true;
	};

	{
		switch (typePattern_) {
		case PATTERN_TYPE_FAN:
		case PATTERN_TYPE_FAN_AIMED:
		{
			float ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_FAN_AIMED)
				ini_angle += atan2f(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);
			float ang_off_way = (float)(shotWay_ / 2U) - (shotWay_ % 2U == 0U ? 0.5 : 0.0);

			for (size_t iWay = 0U; iWay < shotWay_; ++iWay) {
				float sa = ini_angle + (iWay - ang_off_way) * angleArgument_;
				float r_fac[2] = { cosf(sa), sinf(sa) };
				for (size_t iStack = 0U; iStack < shotStack_; ++iStack) {
					float ss = speedBase_;
					if (shotStack_ > 1U) ss += (speedArgument_ - speedBase_) * (iStack / (float)(shotStack_ - 1U));

					float sx = basePosX + fireRadiusOffset_ * r_fac[0];
					float sy = basePosY + fireRadiusOffset_ * r_fac[1];
					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_RING:
		case PATTERN_TYPE_RING_AIMED:
		{
			float ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_RING_AIMED)
				ini_angle += atan2f(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			for (size_t iWay = 0U; iWay < shotWay_; ++iWay) {
				float sa_b = ini_angle + (GM_PI_X2 / (float)shotWay_) * iWay;
				for (size_t iStack = 0U; iStack < shotStack_; ++iStack) {
					float ss = speedBase_;
					if (shotStack_ > 1U) ss += (speedArgument_ - speedBase_) * (iStack / (float)(shotStack_ - 1U));

					float sa = sa_b + iStack * angleArgument_;
					float r_fac[2] = { cosf(sa), sinf(sa) };

					float sx = basePosX + fireRadiusOffset_ * r_fac[0];
					float sy = basePosY + fireRadiusOffset_ * r_fac[1];
					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_ARROW:
		case PATTERN_TYPE_ARROW_AIMED:
		{
			float ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_ARROW_AIMED)
				ini_angle += atan2f(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);
			size_t stk_cen = shotStack_ / 2U;

			for (size_t iStack = 0U; iStack < shotStack_; ++iStack) {
				float ss = speedBase_;
				if (shotStack_ > 1) {
					if (shotStack_ % 2U == 0U) {
						if (shotStack_ > 2U) {
							float tmp = (iStack < stk_cen) ? (stk_cen - iStack - 1U) : (iStack - stk_cen);
							ss = speedBase_ + (speedArgument_ - speedBase_) * (tmp / (stk_cen - 1));
						}
					}
					else {
						float tmp = fabs((float)iStack - stk_cen);
						ss = speedBase_ + (speedArgument_ - speedBase_) * (tmp / std::max(1U, stk_cen - 1U));
					}
				}

				for (size_t iWay = 0U; iWay < shotWay_; ++iWay) {
					float sa = ini_angle + (GM_PI_X2 / (float)shotWay_) * iWay;
					if (shotStack_ > 1U) {
						sa += (float)((shotStack_ % 2U == 0) ?
							((float)iStack - (stk_cen - 0.5)) : ((float)iStack - stk_cen)) * angleArgument_;
					}

					float r_fac[2] = { cosf(sa), sinf(sa) };

					float sx = basePosX + fireRadiusOffset_ * r_fac[0];
					float sy = basePosY + fireRadiusOffset_ * r_fac[1];
					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_POLYGON:
		case PATTERN_TYPE_POLYGON_AIMED:
		{
			float ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_POLYGON_AIMED)
				ini_angle += atan2f(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			size_t numEdges = shotWay_;
			size_t numShotPerEdge = shotStack_;
			int edgeSkip = std::round(Math::RadianToDegree(angleArgument_));

			float r_fac[2] = { cosf(ini_angle), sinf(ini_angle) };

			for (size_t iEdge = 0U; iEdge < numEdges; ++iEdge) {
				float from_ang = (GM_PI_X2 / numEdges) * (float)iEdge;
				float to_ang = (GM_PI_X2 / numEdges) * (float)((int)iEdge + edgeSkip);
				float from_pos[2] = { cosf(from_ang), sinf(from_ang) };
				float to_pos[2] = { cosf(to_ang), sinf(to_ang) };

				for (size_t iShot = 0U; iShot < numShotPerEdge; ++iShot) {
					//Will always be just a little short of a full 1, intentional.
					float rate = iShot / (float)numShotPerEdge;

					float _sx_b = Math::Lerp::Linear(from_pos[0], to_pos[0], rate);
					float _sy_b = Math::Lerp::Linear(from_pos[1], to_pos[1], rate);
					float _sx = _sx_b * r_fac[0] + _sy_b * r_fac[1];
					float _sy = _sx_b * r_fac[1] - _sy_b * r_fac[0];
					float sx = basePosX + fireRadiusOffset_ * _sx;
					float sy = basePosY + fireRadiusOffset_ * _sy;
					float sa = atan2f(_sy, _sx);
					float ss = hypotf(_sx, _sy) * speedBase_;
					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_ELLIPSE:
		case PATTERN_TYPE_ELLIPSE_AIMED:
		{
			float el_pointing_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_ELLIPSE_AIMED)
				el_pointing_angle += atan2f(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			float r_eccentricity = speedArgument_ / speedBase_;

			for (size_t iWay = 0U; iWay < shotWay_; ++iWay) {
				float angle_cur = GM_PI_X2 / (float)shotWay_ * iWay + angleArgument_;
				float _rx = 1 * cosf(angle_cur);
				float _ry = r_eccentricity * sinf(angle_cur);

				float r_fac[2] = { cosf(el_pointing_angle), sinf(el_pointing_angle) };
				float rx = _rx * r_fac[0] + _ry * r_fac[1];
				float ry = _rx * r_fac[1] - _ry * r_fac[0];

				float sa = atan2f(ry, rx);
				float ss = hypotf(rx, ry) * speedBase_;
				float sx = basePosX + fireRadiusOffset_ * rx;
				float sy = basePosY + fireRadiusOffset_ * ry;
				__CreateShot(sx, sy, ss, sa);
			}
			break;
		}
		case PATTERN_TYPE_SCATTER_ANGLE:
		case PATTERN_TYPE_SCATTER_SPEED:
		case PATTERN_TYPE_SCATTER:
		{
			float ini_angle = angleBase_;

			for (size_t iWay = 0U; iWay < shotWay_; ++iWay) {
				for (size_t iStack = 0U; iStack < shotStack_; ++iStack) {
					float ss = speedBase_ + (speedArgument_ - speedBase_) *
						((shotStack_ > 1U && typePattern_ == PATTERN_TYPE_SCATTER_ANGLE) ?
							(iStack / (float)(shotStack_ - 1U)) : randGenerator->GetReal());

					float sa = ini_angle + ((typePattern_ == PATTERN_TYPE_SCATTER_SPEED) ?
						(GM_PI_X2 / (float)shotWay_ * iWay) + angleArgument_ * iStack :
						randGenerator->GetReal(-angleArgument_, angleArgument_));
					float r_fac[2] = { cosf(sa), sinf(sa) };

					float sx = basePosX + fireRadiusOffset_ * r_fac[0];
					float sy = basePosY + fireRadiusOffset_ * r_fac[1];
					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
        case PATTERN_TYPE_LINE:
        case PATTERN_TYPE_LINE_AIMED:
		{
			float ini_angle = angleBase_;
            float angle_off = (float)(angleArgument_ / 2U);
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_LINE_AIMED)
				ini_angle += atan2f(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			float from_ang = ini_angle + angle_off;
			float to_ang = ini_angle - angle_off;

            float from_pos[2] = { cosf(from_ang), sinf(from_ang) };
            float to_pos[2] = { cosf(to_ang), sinf(to_ang) };

            for (size_t iWay = 0U; iWay < shotWay_; ++iWay) {
                //Will always be just a little short of a full 1, intentional.
                float rate = shotWay_ > 1U ? iWay / ((float)shotWay_ - 1) : 0.5f;

                float _sx = Math::Lerp::Linear(from_pos[0], to_pos[0], rate);
                float _sy = Math::Lerp::Linear(from_pos[1], to_pos[1], rate);
                float sx = basePosX + fireRadiusOffset_ * _sx;
                float sy = basePosY + fireRadiusOffset_ * _sy;
                float sa = atan2f(_sy, _sx);
                float _ss = hypotf(_sx, _sy);
                for (size_t iStack = 0U; iStack < shotStack_; ++iStack) {
                    float ss = speedBase_;
                    if (shotStack_ > 1U) ss += (speedArgument_ - speedBase_) * (iStack / (float)(shotStack_ - 1U));
                    __CreateShot(sx, sy, ss * _ss, sa);
                }
                
            }
			break;
		}
		case PATTERN_TYPE_ROSE:
		case PATTERN_TYPE_ROSE_AIMED:
		{
			float ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_ROSE_AIMED)
				ini_angle += atan2f(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			size_t numPetal = shotWay_;
			size_t numShotPerPetal = shotStack_;
			int petalSkip = std::round(Math::RadianToDegree(angleArgument_));

			float petalGap = GM_PI_X2 / numPetal;
			float angGap = (GM_PI_X2 / (numPetal * numShotPerPetal) * petalSkip);

			for (size_t iStack = 0U; iStack < numShotPerPetal; ++iStack) {
				float ss = speedBase_ + (speedArgument_ - speedBase_) * sinf(GM_PI / numShotPerPetal * iStack);

				for (size_t iShot = 0U; iShot < numPetal; ++iShot) {
					float sa = ini_angle + iShot * petalGap + iStack * angGap;
					float r_fac[2] = { cosf(sa), sinf(sa) };

					float sx = basePosX + fireRadiusOffset_ * r_fac[0];
					float sy = basePosY + fireRadiusOffset_ * r_fac[1];
					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		}
	}
}