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

	listPlayerShotData_ = std::make_unique<StgShotDataList>();
	listEnemyShotData_ = std::make_unique<StgShotDataList>();

	rcDeleteClip_ = DxRect<LONG>(-64, -64, 64, 64);

	filterMin_ = D3DTEXF_LINEAR;
	filterMag_ = D3DTEXF_LINEAR;

	{
		RenderShaderLibrary* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();
		effectShot_ = shaderManager_->GetRender2DShader();
	}
	{
		size_t renderPriMax = stageController_->GetMainObjectManager()->GetRenderBucketCapacity();

		listRenderQueuePlayer_.resize(renderPriMax);
		listRenderQueueEnemy_.resize(renderPriMax);
		for (size_t i = 0; i < renderPriMax; ++i) {
			listRenderQueuePlayer_[i].listShot.resize(32);
			listRenderQueueEnemy_[i].listShot.resize(32);
		}
	}
	pLastTexture_ = nullptr;

	SetDeleteEventEnableByType(StgStageItemScript::EV_DELETE_SHOT_IMMEDIATE, true);
	SetDeleteEventEnableByType(StgStageItemScript::EV_DELETE_SHOT_FADE, true);
	SetDeleteEventEnableByType(StgStageItemScript::EV_DELETE_SHOT_TO_ITEM, true);
}
StgShotManager::~StgShotManager() {
	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj)
			obj->ClearShotObject();
	}
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

std::array<BlendMode, StgShotManager::BLEND_COUNT> StgShotManager::blendTypeRenderOrder = {
	MODE_BLEND_ADD_ARGB,
	MODE_BLEND_ADD_RGB,
	MODE_BLEND_SHADOW,
	MODE_BLEND_MULTIPLY,
	MODE_BLEND_SUBTRACT,
	MODE_BLEND_INV_DESTRGB,
	MODE_BLEND_ALPHA,
	MODE_BLEND_ALPHA_INV,
};
void StgShotManager::Render(int targetPriority) {
	if (targetPriority < 0 || targetPriority >= listRenderQueueEnemy_.size()) return;

	const RenderQueue& renderQueuePlayer = listRenderQueuePlayer_[targetPriority];
	const RenderQueue& renderQueueEnemy = listRenderQueueEnemy_[targetPriority];
	if (renderQueuePlayer.count == 0 && renderQueueEnemy.count == 0) return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	RenderShaderLibrary* shaderManager = ShaderManager::GetBase()->GetRenderLib();

	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetCullingMode(D3DCULL_NONE);
	graphics->SetLightingEnable(false);
	graphics->SetTextureFilter(filterMin_, filterMag_, D3DTEXF_NONE);

	DWORD bEnableFog = FALSE;
	device->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	auto& camera3D = graphics->GetCamera();
	auto& camera2D = graphics->GetCamera2D();

	D3DXMatrixMultiply(&matProj_, &camera2D->GetMatrix(), &graphics->GetViewPortMatrix());

	device->SetFVF(VERTEX_TLX::fvf);
	device->SetVertexDeclaration(shaderManager->GetVertexDeclarationTLX());
	pLastTexture_ = nullptr;

	if (D3DXHANDLE handle = effectShot_->GetParameterBySemantic(nullptr, "VIEWPROJECTION")) {
		effectShot_->SetMatrix(handle, &matProj_);
	}

	auto _RenderQueue = [&](const RenderQueue& renderQueue) {
		if (renderQueue.count == 0) return;

		for (size_t iBlend = 0; iBlend < blendTypeRenderOrder.size(); ++iBlend) {
			BlendMode blend = blendTypeRenderOrder[iBlend];

			graphics->SetBlendMode(blend);
			effectShot_->SetTechnique(blend == MODE_BLEND_ALPHA_INV ? "RenderInv" : "Render");

			for (size_t i = 0; i < renderQueue.count; ++i) {
				StgShotObject* pShot = renderQueue.listShot[i];
				pShot->Render(blend);
			}
		}
	};

	//Always renders enemy shots above player shots, completely obliterates TAΣ's wet dream.
	_RenderQueue(renderQueuePlayer);
	_RenderQueue(renderQueueEnemy);

	device->SetVertexShader(nullptr);
	device->SetPixelShader(nullptr);
	device->SetVertexDeclaration(nullptr);
	device->SetIndices(nullptr);

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void StgShotManager::LoadRenderQueue() {
	for (size_t i = 0; i < listRenderQueuePlayer_.size(); ++i) {
		listRenderQueuePlayer_[i].count = 0;
		listRenderQueueEnemy_[i].count = 0;
	}

	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted() || !obj->IsActive() || !obj->IsVisible()) continue;

		auto& [count, listShot] = (obj->GetOwnerType() == StgShotObject::OWNER_PLAYER ?
			listRenderQueuePlayer_ : listRenderQueueEnemy_)[obj->GetRenderPriorityI()];

		while (count >= listShot.size())
			listShot.resize(listShot.size() * 2);
		listShot[count++] = obj.get();
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

void StgShotManager::DeleteInCircle(int typeDelete, int typeTo, int typeOwner, int cx, int cy, optional<int> radius) {
	int r = radius ? *radius : 0;
	int rr = r * r;

	DxRect<int> rcBox(cx - r, cy - r, cx + r, cy + r);

	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		if (typeDelete == DEL_TYPE_SHOT && obj->IsSpellResist()) continue;

		int sx = obj->GetPositionX();
		int sy = obj->GetPositionY();

		bool bInRadius = rcBox.IsPointIntersected(sx, sy) && Math::HypotSq<int64_t>(cx - sx, cy - sy) <= rr;
		if (!radius.has_value() || bInRadius) {
			if (typeTo == TO_TYPE_IMMEDIATE)
				obj->DeleteImmediate();
			else if (typeTo == TO_TYPE_FADE)
				obj->SetFadeDelete();
			else if (typeTo == TO_TYPE_ITEM)
				obj->ConvertToItem();
		}
	}
}

std::vector<int> StgShotManager::GetShotIdInCircle(int typeOwner, int cx, int cy, optional<int> radius) {
	int r = radius ? *radius : 0;
	int rr = r * r;

	DxRect<int> rcBox(cx - r, cy - r, cx + r, cy + r);

	std::vector<int> res;
	for (ref_unsync_ptr<StgShotObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;

		int sx = obj->GetPositionX();
		int sy = obj->GetPositionY();

		bool bInRadius = rcBox.IsPointIntersected(sx, sy) && Math::HypotSq<int64_t>(cx - sx, cy - sy) <= rr;
		if (!radius.has_value() || bInRadius) {
			res.push_back(obj->GetObjectID());
		}
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

int StgShotManager::_TypeDeleteToEventType(TypeDelete type) {
	switch (type) {
	case TypeDelete::Immediate:
		return StgStageItemScript::EV_DELETE_SHOT_IMMEDIATE;
	case TypeDelete::Fade:
		return StgStageItemScript::EV_DELETE_SHOT_FADE;
	case TypeDelete::Item:
		return StgStageItemScript::EV_DELETE_SHOT_TO_ITEM;
	}
	return StgStageItemScript::EV_DELETE_SHOT_IMMEDIATE;
}
StgShotManager::TypeDelete StgShotManager::_EventTypeToTypeDelete(int type) {
	switch (type) {
	case StgStageItemScript::EV_DELETE_SHOT_IMMEDIATE:
		return TypeDelete::Immediate;
	case StgStageItemScript::EV_DELETE_SHOT_FADE:
		return TypeDelete::Fade;
	case StgStageItemScript::EV_DELETE_SHOT_TO_ITEM:
		return TypeDelete::Item;
	}
	return TypeDelete::Immediate;
}

void StgShotManager::SetDeleteEventEnableByType(int type, bool bEnable) {
	int bit = (int)_EventTypeToTypeDelete(type);
	listDeleteEventEnable_.set(bit, bEnable);
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
	defaultDelayData_ = -1;
	defaultDelayColor_ = 0xffffffff;	//Solid white
}
StgShotDataList::~StgShotDataList() {
}
void StgShotDataList::_LoadVertexBuffers(std::map<std::wstring, VBContainerList>::iterator placement, 
	shared_ptr<Texture> texture, std::vector<StgShotData*>& listAddData)
{
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	float texW = texture->GetWidth();
	float texH = texture->GetHeight();

	size_t countFrame = 0;
	for (StgShotData* iData : listAddData)
		countFrame += iData->GetFrameCount();

	size_t iBuffer = 0;
	while (countFrame > 0) {
		size_t thisCountFrame = std::min<size_t>(countFrame, StgShotVertexBufferContainer::MAX_DATA);

		placement->second.push_back(unique_ptr<StgShotVertexBufferContainer>(
			new StgShotVertexBufferContainer()));
		StgShotVertexBufferContainer* pVertexBufferContainer = placement->second.back().get();
		pVertexBufferContainer->SetTexture(texture);

		std::vector<VERTEX_TLX> bufferVertex(4 * thisCountFrame);
		size_t iVertex = 0;

		VERTEX_TLX verts[4];
		for (size_t iData = 0; iData < listAddData.size(); ++iData) {
			StgShotData* data = listAddData[iData];
			for (size_t iAnim = 0; iAnim < data->GetFrameCount(); ++iAnim) {
				StgShotDataFrame* pFrame = &data->listFrame_[iAnim];
				pFrame->listShotData_ = this;

				LONG* ptrSrc = reinterpret_cast<LONG*>(&pFrame->rcSrc_);
				float* ptrDst = reinterpret_cast<float*>(&pFrame->rcDst_);

				for (size_t iVert = 0; iVert < 4; ++iVert) {
					VERTEX_TLX* pv = &verts[iVert];

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

					StgShotObject::_SetVertexUV(pv,
						ptrSrc[(iVert & 1) << 1] / texW, ptrSrc[iVert | 1] / texH);
					StgShotObject::_SetVertexPosition(pv, ptrDst[(iVert & 1) << 1], ptrDst[iVert | 1], 0);
					StgShotObject::_SetVertexColorARGB(pv, 0xffffffff);
				}

				pFrame->pVertexBuffer_ = pVertexBufferContainer;
				pFrame->vertexOffset_ = iVertex;

				for (size_t j = 0; j < 4; ++j)
					bufferVertex[iVertex + j] = verts[j];
				iVertex += 4;
			}
		}

		HRESULT hr = pVertexBufferContainer->LoadData(bufferVertex, thisCountFrame);
		if (FAILED(hr)) {
			std::wstring err = StringUtility::Format(L"AddShotDataList::Failed to load shot data buffer: "
				"\t\r\n%s: %s",
				DXGetErrorString(hr), DXGetErrorDescription(hr));
			throw gstd::wexception(err);
		}

		++iBuffer;
		countFrame -= thisCountFrame;
	}
}
bool StgShotDataList::AddShotDataList(const std::wstring& path, bool bReload) {
	auto itrVB = mapVertexBuffer_.find(path);
	if (!bReload && itrVB != mapVertexBuffer_.end()) return true;

	std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);

	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open())
		throw gstd::wexception(L"AddShotDataList: " + ErrorUtility::GetFileNotFoundErrorMessage(pathReduce, true));

	std::string source = reader->ReadAllString();

	bool res = false;
	Scanner scanner(source);
	try {
		std::map<int, unique_ptr<StgShotData>> mapData;
		std::wstring pathImage;

		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::Type::TK_EOF)
				break;
			else if (tok.GetType() == Token::Type::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"ShotData") {
					_ScanShot(mapData, scanner);
				}
				else if (element == L"shot_image") {
					scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
					pathImage = scanner.Next().GetString();
				}
				else if (element == L"delay_id") {
					scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
					defaultDelayData_ = scanner.Next().GetInteger();
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
				
				if (scanner.HasNext())
					tok = scanner.Next();
			}
		}

		if (pathImage.size() == 0) throw gstd::wexception("Shot texture must be set.");
		std::wstring dir = PathProperty::GetFileDirectory(path);
		pathImage = StringUtility::Replace(pathImage, L"./", dir);
		pathImage = PathProperty::GetUnique(pathImage);

		shared_ptr<Texture> texture;
		{
			TextureManager* textureManager = TextureManager::GetBase();

			if ((texture = textureManager->GetTexture(pathImage)) == nullptr) {
				texture = std::make_shared<Texture>();
				if (!texture->CreateFromFile(pathImage, false, false))
					texture = nullptr;
			}
		}
		if (texture == nullptr) {
			throw gstd::wexception("Failed to load the specified shot texture.");
		}

		std::vector<StgShotData*> listAddData;

		size_t countFrame = 0;
		{
			size_t i = 0;
			for (auto itr = mapData.begin(); itr != mapData.end(); ++itr, ++i) {
				int id = itr->first;
				unique_ptr<StgShotData>& data = itr->second;
				if (data == nullptr) continue;

				for (auto& iFrame : data->listFrame_)
					iFrame.listShotData_ = this;
				countFrame += data->GetFrameCount();

				listAddData.push_back(data.get());
				if (listData_.size() <= id)
					listData_.resize(id + 1);
				listData_[id] = std::move(data);		//Moves unique_ptr object, do not use mapData after this point
			}
		}

		if (itrVB != mapVertexBuffer_.end()) {
			itrVB->second.clear();
			_LoadVertexBuffers(itrVB, texture, listAddData);
		}
		else {
			itrVB = mapVertexBuffer_.insert({ path, VBContainerList() }).first;
			_LoadVertexBuffers(itrVB, texture, listAddData);
		}

		Logger::WriteTop(StringUtility::Format(L"Loaded shot data: %s", pathReduce.c_str()));
		res = true;
	}
	catch (gstd::wexception& e) {
		std::wstring log = StringUtility::Format(L"Failed to load shot data: %s\r\n\t[Line=%d] (%s)",
			pathReduce.c_str(), scanner.GetCurrentLine(), e.what());
		Logger::WriteTop(log);
		res = false;
	}
	catch (...) {
		std::string log = StringUtility::Format("Failed to load shot data: %s\r\n\t[Line=%d] (Unknown error.)",
			pathReduce.c_str(), scanner.GetCurrentLine());
		Logger::WriteTop(log);
		res = false;
	}

	return res;
}
void StgShotDataList::_ScanShot(std::map<int, unique_ptr<StgShotData>>& mapData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::Type::TK_NEWLINE) tok = scanner.Next();
	scanner.CheckType(tok, Token::Type::TK_OPENC);

	struct Data {
		StgShotData* shotData;
		int id = -1;
	} data;
	data.shotData = new StgShotData(this);
	data.shotData->idDefaultDelay_ = defaultDelayData_;
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

		StgShotDataFrame dFrame;
		dFrame.rcSrc_ = rect;
		dFrame.rcDst_ = StgShotDataFrame::LoadDestRect(&rect);

		i->shotData->listFrame_ = { dFrame };
		i->shotData->totalFrame_ = 1;
	};
	auto funcSetDelayID = [](Data* i, Scanner& s) {
		s.CheckType(s.Next(), Token::Type::TK_EQUAL);
		i->shotData->idDefaultDelay_ = s.Next().GetInteger();
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
		i->shotData->listCol_.push_back(circle);
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
		i->shotData->listFrame_.clear();
		i->shotData->totalFrame_ = 0;
		_ScanAnimation(i->shotData, s);
	};

	//Do NOT use [&] lambdas
	static const std::unordered_map<std::wstring, std::function<void(Data*, Scanner&)>> mapFunc = {
		{ L"id", LAMBDA_SETV(id, GetInteger) },
		{ L"rect", funcSetRect },
		{ L"delay_id", funcSetDelayID },
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
		if (data.shotData->listCol_.size() == 0) {
			float r = 0;
			if (data.shotData->listFrame_.size() > 0) {
				DxRect<LONG>& rect = data.shotData->listFrame_[0].rcSrc_;
				r = std::min(abs(rect.GetWidth()), abs(rect.GetHeight())) / 3.0f - 3.0f;
			}
			data.shotData->listCol_.push_back(DxCircle(0, 0, r));
		}

		mapData[data.id] = unique_ptr<StgShotData>(data.shotData);
	}
}
void StgShotDataList::_ScanAnimation(StgShotData* shotData, Scanner& scanner) {
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
				DxRect<LONG> rect(StringUtility::ToInteger(list[1]), StringUtility::ToInteger(list[2]),
					StringUtility::ToInteger(list[3]), StringUtility::ToInteger(list[4]));

				StgShotDataFrame dFrame;
				dFrame.frame_ = frame;
				dFrame.rcSrc_ = rect;
				dFrame.rcDst_ = StgShotDataFrame::LoadDestRect(&rect);

				shotData->listFrame_.push_back(dFrame);
				shotData->totalFrame_ += frame;
			}
		}
	}
}

//*******************************************************************
//StgShotDataFrame
//*******************************************************************
StgShotDataFrame::StgShotDataFrame() {
	listShotData_ = nullptr;
	pVertexBuffer_ = nullptr;
	vertexOffset_ = 0;
	frame_ = 0;
}
DxRect<float> StgShotDataFrame::LoadDestRect(DxRect<LONG>* src) {
	float width = src->GetWidth() / 2.0f;
	float height = src->GetHeight() / 2.0f;
	return DxRect<float>(-width, -height, width, height);
}

//*******************************************************************
//StgShotData
//*******************************************************************
StgShotData::StgShotData(StgShotDataList* listShotData) {
	listShotData_ = listShotData;

	typeRender_ = MODE_BLEND_ALPHA;
	typeDelayRender_ = MODE_BLEND_ADD_ARGB;

	alpha_ = 255;

	idDefaultDelay_ = -1;
	colorDelay_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	totalFrame_ = 0;

	angularVelocityMin_ = 0;
	angularVelocityMax_ = 0;
	bFixedAngle_ = false;
}
StgShotData::~StgShotData() {
}

StgShotDataFrame* StgShotData::GetFrame(size_t frame) {
	if (totalFrame_ <= 1U)
		return &listFrame_[0];

	frame = frame % totalFrame_;
	size_t total = 0;

	for (auto itr = listFrame_.begin(); itr != listFrame_.end(); ++itr) {
		total += itr->frame_;
		if (total >= frame)
			return &(*itr);
	}
	return &listFrame_[0];
}

//****************************************************************************
//StgShotVertexBufferContainer
//****************************************************************************
StgShotVertexBufferContainer::StgShotVertexBufferContainer() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	VertexBufferManager* vbManager = VertexBufferManager::GetBase();

	pVertexBuffer_ = (FixedVertexBuffer*)vbManager->CreateExtraVertexBuffer();

	countData_ = 0;
}
StgShotVertexBufferContainer::~StgShotVertexBufferContainer() {
	VertexBufferManager* vbManager = VertexBufferManager::GetBase();
	vbManager->ReleaseExtraVertexBuffer((size_t)pVertexBuffer_);
}

HRESULT StgShotVertexBufferContainer::LoadData(const std::vector<VERTEX_TLX>& data, size_t countFrame) {
	pVertexBuffer_->Setup(data.size(), StgShotVertexBufferContainer::STRIDE, VERTEX_TLX::fvf);

	HRESULT hr = pVertexBuffer_->Create(0, D3DPOOL_MANAGED);
	if (FAILED(hr)) {
		return hr;
	}

	BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);
	lockParam.SetSource(const_cast<std::vector<VERTEX_TLX>&>(data), pVertexBuffer_->GetSize(), sizeof(VERTEX_TLX));

	hr = pVertexBuffer_->UpdateBuffer(&lockParam);
	countData_ = countFrame;

	return hr;
}

//****************************************************************************
//StgShotObject
//****************************************************************************
StgShotObject::StgShotObject(StgStageController* stageController) : StgMoveObject(stageController) {
	frameWork_ = 0;
	posX_ = 0;
	posY_ = 0;
	idShotData_ = 0;
	SetBlendType(MODE_BLEND_NONE);

	bRequestedPlayerDeleteEvent_ = false;
	damage_ = 1;
	life_ = 1;

	bAutoDelete_ = true;
	bEraseShot_ = false;
	bSpellFactor_ = false;
	bSpellResist_ = false;

	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	frameGrazeInvalid_ = 0;
	frameGrazeInvalidStart_ = -1;

	bPenetrateShot_ = true;
	frameEnemyHitInvalid_ = 0;

	frameFadeDelete_ = -1;
	frameAutoDelete_ = INT_MAX;
	typeAutoDelete_ = StgShotManager::TO_TYPE_FADE;

	typeOwner_ = OWNER_ENEMY;

	bUserIntersectionMode_ = false;
	bIntersectionEnable_ = true;
	bChangeItemEnable_ = true;

	bEnableMotionDelay_ = false;
	bRoundingPosition_ = false;
	roundingAngle_ = 0;

	hitboxScale_ = D3DXVECTOR2(1.0f, 1.0f);

	timerTransform_ = 0;
	timerTransformNext_ = 0;

	int priShotI = stageController_->GetStageInformation()->GetShotObjectPriority();
	SetRenderPriorityI(priShotI);
}
StgShotObject::~StgShotObject() {
}

void StgShotObject::Clone(DxScriptObjectBase* _src) {
	DxScriptShaderObject::Clone(_src);

	auto src = (StgShotObject*)_src;
	StgMoveObject::Copy((StgMoveObject*)src);
	StgIntersectionObject::Copy((StgIntersectionObject*)src);

	frameWork_ = src->frameWork_;
	idShotData_ = src->idShotData_;
	typeOwner_ = src->typeOwner_;

	move_ = src->move_;
	lastAngle_ = src->lastAngle_;

	hitboxScale_ = src->hitboxScale_;
	delay_ = src->delay_;

	frameGrazeInvalid_ = src->frameGrazeInvalid_;
	frameGrazeInvalidStart_ = src->frameGrazeInvalidStart_;
	frameFadeDelete_ = src->frameFadeDelete_;
	bPenetrateShot_ = src->bPenetrateShot_;

	renderTarget_ = src->renderTarget_;

	frameEnemyHitInvalid_ = src->frameEnemyHitInvalid_;
	mapEnemyHitCooldown_ = src->mapEnemyHitCooldown_;

	bRequestedPlayerDeleteEvent_ = src->bRequestedPlayerDeleteEvent_;
	damage_ = src->damage_;
	life_ = src->life_;

	bAutoDelete_ = src->bAutoDelete_;
	bEraseShot_ = src->bEraseShot_;
	bSpellFactor_ = src->bSpellFactor_;
	bSpellResist_ = src->bSpellResist_;
	frameAutoDelete_ = src->frameAutoDelete_;
	typeAutoDelete_ = src->typeAutoDelete_;

	listIntersectionTarget_ = src->listIntersectionTarget_;
	bUserIntersectionMode_ = src->bUserIntersectionMode_;
	bIntersectionEnable_ = src->bIntersectionEnable_;
	bChangeItemEnable_ = src->bChangeItemEnable_;

	bEnableMotionDelay_ = src->bEnableMotionDelay_;
	bRoundingPosition_ = src->bRoundingPosition_;
	roundingAngle_ = src->roundingAngle_;

	listTransformationShotAct_ = src->listTransformationShotAct_;
	timerTransform_ = src->timerTransform_;
	timerTransformNext_ = src->timerTransformNext_;
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
}
void StgShotObject::_DeleteInLife() {
	if (IsDeleted() || life_ > 0) return;

	_SendDeleteEvent(TypeDelete::Immediate);
	_RequestPlayerDeleteEvent(DxScript::ID_INVALID);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}
void StgShotObject::_RequestPlayerDeleteEvent(int hitObjectID) {	//A super ugly hack, but it'll do for now
	if (bRequestedPlayerDeleteEvent_) return;
	bRequestedPlayerDeleteEvent_ = true;

	auto objectManager = stageController_->GetMainObjectManager();
	auto scriptManager = stageController_->GetScriptManager();

	if (scriptManager != nullptr && typeOwner_ == StgShotObject::OWNER_PLAYER) {
		float posX = GetPositionX();
		float posY = GetPositionY();
		LOCK_WEAK(scriptPlayer, scriptManager->GetPlayerScript()) {
			float listPos[2] = { posX, posY };

			value listScriptValue[4];
			listScriptValue[0] = scriptPlayer->CreateIntValue(idObject_);
			listScriptValue[1] = scriptPlayer->CreateFloatArrayValue(listPos, 2U);
			listScriptValue[2] = scriptPlayer->CreateIntValue(GetShotDataID());
			listScriptValue[3] = scriptPlayer->CreateIntValue(hitObjectID);
			scriptPlayer->RequestEvent(StgStagePlayerScript::EV_DELETE_SHOT_PLAYER, listScriptValue, 4);
		}
	}
}

void StgShotObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetShotManager()->GetShotDeleteClip();
	DxRect<LONG> rcDeleteClip(rcClipBase->left, rcClipBase->top,
		rcStgFrame->GetWidth() + rcClipBase->right,
		rcStgFrame->GetHeight() + rcClipBase->bottom);

	if (!rcDeleteClip.IsPointIntersected(posX_, posY_)) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgShotObject::_DeleteInFadeDelete() {
	if (IsDeleted()) return;
	if (frameFadeDelete_ == 0) {
		_SendDeleteEvent(TypeDelete::Fade);

		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgShotObject::_DeleteInAutoDeleteFrame() {
	if (IsDeleted() || delay_.time > 0) return;

	if (frameAutoDelete_ <= 0) {
		switch (typeAutoDelete_) {
		case StgShotManager::TO_TYPE_IMMEDIATE:
			DeleteImmediate();
			break;
		case StgShotManager::TO_TYPE_FADE:
			SetFadeDelete();
			break;
		case StgShotManager::TO_TYPE_ITEM:
			ConvertToItem();
			break;
		}
	}
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

	//----------------------------------------------------------

	for (auto itr = mapEnemyHitCooldown_.begin(); itr != mapEnemyHitCooldown_.end();) {
		if (itr->first.expired() || itr->first->IsDeleted() || (--(itr->second) == 0))
			itr = mapEnemyHitCooldown_.erase(itr);
		else ++itr;
	}
}

bool StgShotObject::CheckEnemyHitCooldownExists(ref_unsync_weak_ptr<StgEnemyObject> obj) {
	if (mapEnemyHitCooldown_.empty()) return false;
	return mapEnemyHitCooldown_.find(obj) != mapEnemyHitCooldown_.end();
}
void StgShotObject::AddEnemyHitCooldown(ref_unsync_weak_ptr<StgEnemyObject> obj, uint32_t time) {
	if (obj) {
		mapEnemyHitCooldown_[obj] = time;
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
					ConvertToItem();
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
					ConvertToItem();
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_ENEMY:
	{
		if (!bSpellResist_) {
			//Register intersection only if the enemy is off hit cooldown
			if (!CheckEnemyHitCooldownExists(ref_unsync_weak_ptr<StgEnemyObject>::Cast(obj)))
				--life_;
		}
		break;
	}
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		if (!bSpellResist_ && bPenetrateShot_)
			--life_;
		break;
	}
	}

	if (life_ == 0)
		_RequestPlayerDeleteEvent(obj.IsExists() ? obj->GetDxScriptObjectID() : DxScript::ID_INVALID);
}
StgShotData* StgShotObject::_GetShotData(int id) {
	StgShotManager* shotManager = stageController_->GetShotManager();
	StgShotDataList* dataList = (typeOwner_ == OWNER_PLAYER) ?
		shotManager->GetPlayerShotDataList() : shotManager->GetEnemyShotDataList();
	return dataList ? dataList->GetData(id) : nullptr;
}

void StgShotObject::_SetVertexPosition(VERTEX_TLX* vertex, float x, float y, float z, float w) {
	constexpr float bias = 0.0f;

	x *= DirectGraphics::g_dxCoordsMul_;
	y *= DirectGraphics::g_dxCoordsMul_;

	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
	vertex->position.w = w;
}
void StgShotObject::_SetVertexUV(VERTEX_TLX* vertex, float u, float v) {
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void StgShotObject::_SetVertexColorARGB(VERTEX_TLX* vertex, D3DCOLOR color) {
	vertex->diffuse_color = color;
}
void StgShotObject::SetAlpha(int alpha) {
	ColorAccess::ClampColor(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
void StgShotObject::SetColor(int r, int g, int b) {
	__m128i c = Vectorize::Set(color_ >> 24, r, g, b);
	color_ = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));
}

void StgShotObject::ConvertToItem() {
	if (IsDeleted()) return;

	_SendDeleteEvent(bChangeItemEnable_ ? TypeDelete::Item : TypeDelete::Immediate);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}
void StgShotObject::DeleteImmediate() {
	if (IsDeleted()) return;

	_SendDeleteEvent(TypeDelete::Immediate);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

void StgShotObject::_ProcessTransformAct() {
	if (listTransformationShotAct_.size() == 0) return;

	if (timerTransform_ == 0) timerTransform_ = delay_.time;
	while (timerTransform_ == frameWork_ && listTransformationShotAct_.size() > 0) {
		StgShotPatternTransform& transform = listTransformationShotAct_.front();

		switch (transform.act) {
		case StgShotPatternTransform::TRANSFORM_WAIT:
			timerTransform_ += std::max((int)transform.param[0], 0);
			break;
		case StgShotPatternTransform::TRANSFORM_ADD_SPEED_ANGLE:
		{
			int duration = transform.param[0];
			int delay = transform.param[1];
			double accel = transform.param[2];
			double agvel = transform.param[3];

			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, accel));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL,
					Math::DegreeToRadian(agvel)));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX2, accel * duration));
				AddPattern(delay, pattern, true);
			}
			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, 0));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(delay + duration, pattern, true);
			}
			break;
		}
		case StgShotPatternTransform::TRANSFORM_ANGULAR_MOVE:
		{
			int duration = transform.param[0];
			double agvel = transform.param[1];
			double spin = transform.param[2];

			if (StgNormalShotObject* shot = dynamic_cast<StgNormalShotObject*>(this))
				shot->angularVelocity_ = Math::DegreeToRadian(spin);

			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL,
					Math::DegreeToRadian(agvel)));
				AddPattern(0, pattern, true);
			}
			{
				ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(duration, pattern, true);
			}

			break;
		}
		case StgShotPatternTransform::TRANSFORM_N_DECEL_CHANGE:
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
					ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPEED, nowSpeed));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, -nowSpeed / timer));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, 0));
					AddPattern(framePattern, pattern, true);
				}

				{
					ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
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
		case StgShotPatternTransform::TRANSFORM_GRAPHIC_CHANGE:
		{
			idShotData_ = transform.param[0];
			break;
		}
		case StgShotPatternTransform::TRANSFORM_BLEND_CHANGE:
		{
			SetBlendType((BlendMode)transform.param[0]);
			break;
		}
		case StgShotPatternTransform::TRANSFORM_TO_SPEED_ANGLE:
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
				ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
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
				ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
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
		case StgShotPatternTransform::TRANSFORM_ADDPATTERN_A1:
		{
			int time = transform.param[0];

			double speed = transform.param[1];
			double angle = transform.param[2];

			ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
			pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ZERO, 0));

			ADD_CMD(StgMovePattern_Angle::SET_SPEED, speed);
			ADD_CMD2(StgMovePattern_Angle::SET_ANGLE, angle, Math::DegreeToRadian(angle));

			AddPattern(time, pattern, true);
			break;
		}
		case StgShotPatternTransform::TRANSFORM_ADDPATTERN_A2:
		{
			int time = transform.param[0];

			double speed = transform.param[1];
			double angle = transform.param[2];

			double accel = transform.param[3];
			double maxsp = transform.param[4];
			double agvel = transform.param[5];

			int shotID = transform.param[6];
			int relativeObj = transform.param[7];

			ref_unsync_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));

			ADD_CMD(StgMovePattern_Angle::SET_SPEED, speed);
			ADD_CMD2(StgMovePattern_Angle::SET_ANGLE, angle, Math::DegreeToRadian(angle));
			ADD_CMD(StgMovePattern_Angle::SET_ACCEL, accel);
			ADD_CMD(StgMovePattern_Angle::SET_SPMAX, maxsp);
			ADD_CMD2(StgMovePattern_Angle::SET_AGVEL, agvel, Math::DegreeToRadian(agvel));

			pattern->SetShotDataID(shotID);
			pattern->SetRelativeObject(relativeObj);

			AddPattern(time, pattern, true);
			break;
		}
		case StgShotPatternTransform::TRANSFORM_ADDPATTERN_B1:
		{
			int time = transform.param[0];

			double speedX = transform.param[1];
			double speedY = transform.param[2];

			ref_unsync_ptr<StgMovePattern_XY> pattern(new StgMovePattern_XY(this));
			pattern->AddCommand(std::make_pair(StgMovePattern_XY::SET_ZERO, 0));

			ADD_CMD(StgMovePattern_XY::SET_S_X, speedX);
			ADD_CMD(StgMovePattern_XY::SET_S_Y, speedY);

			AddPattern(time, pattern, true);
			break;
		}
		case StgShotPatternTransform::TRANSFORM_ADDPATTERN_B2:
		{
			int time = transform.param[0];

			double speedX = transform.param[1];
			double speedY = transform.param[2];
			double accelX = transform.param[3];
			double accelY = transform.param[4];
			double maxspX = transform.param[5];
			double maxspY = transform.param[6];

			int shotID = transform.param[7];

			ref_unsync_ptr<StgMovePattern_XY> pattern(new StgMovePattern_XY(this));

			ADD_CMD(StgMovePattern_XY::SET_S_X, speedX);
			ADD_CMD(StgMovePattern_XY::SET_S_Y, speedY);
			ADD_CMD(StgMovePattern_XY::SET_A_X, accelX);
			ADD_CMD(StgMovePattern_XY::SET_A_Y, accelY);
			ADD_CMD(StgMovePattern_XY::SET_M_X, maxspX);
			ADD_CMD(StgMovePattern_XY::SET_M_Y, maxspY);

			pattern->SetShotDataID(shotID);

			AddPattern(time, pattern, true);
			break;
		}
		case StgShotPatternTransform::TRANSFORM_ADDPATTERN_C1:
		{
			int time = transform.param[0];

			double speedX = transform.param[1];
			double speedY = transform.param[2];
			double angOff = transform.param[3];

			ref_unsync_ptr<StgMovePattern_XY_Angle> pattern(new StgMovePattern_XY_Angle(this));
			pattern->AddCommand(std::make_pair(StgMovePattern_XY_Angle::SET_ZERO, 0));

			ADD_CMD(StgMovePattern_XY_Angle::SET_S_X, speedX);
			ADD_CMD(StgMovePattern_XY_Angle::SET_S_Y, speedY);
			ADD_CMD2(StgMovePattern_XY_Angle::SET_ANGLE, angOff, Math::DegreeToRadian(angOff));

			AddPattern(time, pattern, true);
			break;
		}
		case StgShotPatternTransform::TRANSFORM_ADDPATTERN_C2:
		{
			int time = transform.param[0];

			double speedX = transform.param[1];
			double speedY = transform.param[2];
			double accelX = transform.param[3];
			double accelY = transform.param[4];
			double maxspX = transform.param[5];
			double maxspY = transform.param[6];
			double angOff = transform.param[7];
			double angVel = transform.param[8];

			int shotID = transform.param[9];

			ref_unsync_ptr<StgMovePattern_XY_Angle> pattern(new StgMovePattern_XY_Angle(this));

			ADD_CMD(StgMovePattern_XY_Angle::SET_S_X, speedX);
			ADD_CMD(StgMovePattern_XY_Angle::SET_S_Y, speedY);
			ADD_CMD(StgMovePattern_XY_Angle::SET_A_X, accelX);
			ADD_CMD(StgMovePattern_XY_Angle::SET_A_Y, accelY);
			ADD_CMD(StgMovePattern_XY_Angle::SET_M_X, maxspX);
			ADD_CMD(StgMovePattern_XY_Angle::SET_M_Y, maxspY);
			ADD_CMD2(StgMovePattern_XY_Angle::SET_ANGLE, angOff, Math::DegreeToRadian(angOff));
			ADD_CMD2(StgMovePattern_XY_Angle::SET_AGVEL, angVel, Math::DegreeToRadian(angVel));

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
}
StgNormalShotObject::~StgNormalShotObject() {
}

void StgNormalShotObject::Clone(DxScriptObjectBase* _src) {
	StgShotObject::Clone(_src);

	auto src = (StgNormalShotObject*)_src;

	angularVelocity_ = src->angularVelocity_;
	bFixedAngle_ = src->bFixedAngle_;
}

void StgNormalShotObject::Work() {
	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();

		if (delay_.time > 0) {
			--(delay_.time);
			delay_.angle.x += delay_.angle.y;
		}

		{
			angle_.z += angularVelocity_;

			bool bDelay = delay_.time > 0 && delay_.angle.y != 0;

			double angleZ = bDelay ? delay_.angle.x : angle_.z;
			if (StgShotData* shotData = _GetShotData()) {
				if (!bFixedAngle_ && !bDelay) angleZ += GetDirectionAngle() + Math::DegreeToRadian(90);
			}

			if (angleZ != lastAngle_) {
				double ang = (roundingAngle_ > 0) ? round(angleZ / roundingAngle_) * roundingAngle_ : angleZ;
				move_ = D3DXVECTOR2(cosf(ang), sinf(ang));
				lastAngle_ = angleZ;
			}
		}
	}

	_CommonWorkTask();
}

void StgNormalShotObject::_AddIntersectionRelativeTarget() {
	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	if ((IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_)
		|| pOwnReference_.expired())
		return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return;

	ClearIntersected();
	bool res = GetIntersectionTargetList_NoVector(shotData);
	if (res) {
		for (auto& iTarget : listIntersectionTarget_) {
			if (iTarget.first && iTarget.second != nullptr)
				intersectionManager->AddTarget(iTarget.second);
		}
	}
}
StgIntersectionObject::IntersectionListType StgNormalShotObject::GetIntersectionTargetList() {
	if ((IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_)
		|| pOwnReference_.expired())
		return IntersectionListType();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return IntersectionListType();

	bool res = GetIntersectionTargetList_NoVector(shotData);
	if (res) return listIntersectionTarget_;

	return IntersectionListType();
}
bool StgNormalShotObject::GetIntersectionTargetList_NoVector(StgShotData* shotData) {
	float intersectionScale = (hitboxScale_.x + hitboxScale_.y) / 2.0f;
	if (abs(intersectionScale) < 0.01f)
		return false;

	auto& listCircle = shotData->GetIntersectionCircleList();
	if (listIntersectionTarget_.size() < listCircle.size())
		listIntersectionTarget_.resize(listCircle.size(), CreateEmptyIntersection());
	for (auto& i : listIntersectionTarget_) i.first = false;

	for (size_t i = 0; i < listCircle.size(); ++i) {
		IntersectionPairType* pPair = &listIntersectionTarget_[i];

		StgIntersectionTarget_Circle* pTarget = (StgIntersectionTarget_Circle*)(pPair->second.get());
		if (pTarget == nullptr) {
			pTarget = new StgIntersectionTarget_Circle();
			pPair->second.reset(pTarget);
		}

		const DxCircle* pSrcCircle = &listCircle[i];
		DxCircle* pDstCircle = &pTarget->GetCircle();
		if (pSrcCircle->GetR() <= 0)
			continue;
		pPair->first = true;

		if (pSrcCircle->GetX() != 0 || pSrcCircle->GetY() != 0) {
			float px = pSrcCircle->GetX() * move_.x + pSrcCircle->GetY() * move_.y;
			float py = pSrcCircle->GetX() * move_.y - pSrcCircle->GetY() * move_.x;
			pDstCircle->SetX(posX_ + px * intersectionScale);
			pDstCircle->SetY(posY_ + py * intersectionScale);
		}
		else {
			pDstCircle->SetX(posX_);
			pDstCircle->SetY(posY_);
		}
		pDstCircle->SetR(pSrcCircle->GetR() * intersectionScale);

		pTarget->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		pTarget->SetObject(pOwnReference_);
		pTarget->SetIntersectionSpace();
	}

	return true;
}

void StgShotObject::_DefaultShotRender(StgShotData* shotData, StgShotDataFrame* shotFrame, const D3DXMATRIX& matWorld, D3DCOLOR color) {
	if (shotFrame == nullptr) return;
	StgShotManager* shotManager = stageController_->GetShotManager();

	StgShotVertexBufferContainer* pVB = shotFrame->GetVertexBufferContainer();
	DWORD vertexOffset = shotFrame->vertexOffset_;

	if (pVB) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		if (graphics->IsAllowRenderTargetChange()) {
			if (auto pRT = renderTarget_.lock())
				graphics->SetRenderTarget(pRT);
			else graphics->SetRenderTarget(nullptr);
		}

		IDirect3DTexture9* pTexture = pVB->GetD3DTexture();
		if (pTexture != shotManager->pLastTexture_) {
			device->SetTexture(0, pTexture);
			shotManager->pLastTexture_ = pTexture;
		}
		device->SetStreamSource(0, pVB->GetD3DBuffer(), vertexOffset * sizeof(VERTEX_TLX), sizeof(VERTEX_TLX));

		{
			ID3DXEffect* effect = shotManager->GetEffect();
			if (shader_) {
				effect = shader_->GetEffect();
				if (shader_->LoadTechnique()) {
					shader_->LoadParameter();
				}
			}

			if (effect) {
				D3DXHANDLE handle = nullptr;
				if (handle = effect->GetParameterBySemantic(nullptr, "WORLD")) {
					effect->SetMatrix(handle, &matWorld);
				}
				if (shader_) {
					if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION")) {
						effect->SetMatrix(handle, shotManager->GetProjectionMatrix());
					}
				}
				if (handle = effect->GetParameterBySemantic(nullptr, "ICOLOR")) {
					//To normalized RGBA vector
					D3DXVECTOR4 vColor = ColorAccess::ToVec4Normalized(color, ColorAccess::PERMUTE_RGBA);
					effect->SetVector(handle, &vColor);
				}

				UINT countPass = 1;
				effect->Begin(&countPass, D3DXFX_DONOTSAVESHADERSTATE);
				for (UINT iPass = 0; iPass < countPass; ++iPass) {
					effect->BeginPass(iPass);
					device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
					effect->EndPass();
				}
				effect->End();
			}
		}
	}
}

void StgNormalShotObject::Render(BlendMode targetBlend) {
	//if (!IsVisible()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return;

	FLOAT sposx = position_.x;
	FLOAT sposy = position_.y;
	if (bRoundingPosition_) {
		sposx = roundf(sposx);
		sposy = roundf(sposy);
	}

	float scaleX = 1.0f;
	float scaleY = 1.0f;
	D3DCOLOR color;

	auto _Render = [&](StgShotData* pData, StgShotDataFrame* pFrame) {
		if (pData == nullptr || pFrame == nullptr) return;

		D3DXMATRIX matTransform(
			scaleX * move_.x, scaleX * move_.y, 0, 0,
			scaleY * -move_.y, scaleY * move_.x, 0, 0,
			0, 0, 1, 0,
			sposx, sposy, 0, 1
		);
		_DefaultShotRender(pData, pFrame, matTransform, color);
	};

	if (delay_.time > 0) {
		BlendMode objBlendType = GetDelayBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? shotData->GetDelayRenderType() : objBlendType;
		if (objBlendType != targetBlend) return;

		StgShotData* delayData = _GetShotData(delay_.id >= 0 ? delay_.id : shotData->GetDefaultDelayID());
		if (delayData) {
			StgShotDataFrame* delayFrame = delayData ? delayData->GetFrame(frameWork_) : nullptr;

			scaleX = scaleY = delay_.GetScale();
			if (delay_.scaleMix) {
				scaleX *= scale_.x;
				scaleY *= scale_.y;
			}

			color = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
			if (delay_.colorMix) ColorAccess::MultiplyColor(color, color_);
			{
				byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * delay_.GetAlpha());
				color = (color & 0x00ffffff) | (alpha << 24);
			}

			_Render(delayData, delayFrame);
		}
	}
	else {
		BlendMode objBlendType = GetBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? shotData->GetRenderType() : objBlendType;
		if (objBlendType != targetBlend) return;

		scaleX = scale_.x;
		scaleY = scale_.y;
		color = color_;

		{
			float alphaRate = shotData->GetAlpha() / 255.0f;
			if (frameFadeDelete_ >= 0)
				alphaRate *= std::clamp<float>((float)frameFadeDelete_ / FRAME_FADEDELETE, 0, 1);
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * alphaRate);
			color = (color & 0x00ffffff) | (alpha << 24);
		}

		StgShotDataFrame* shotFrame = shotData->GetFrame(frameWork_);
		_Render(shotData, shotFrame);
	}

	//if (bIntersected_) color = D3DCOLOR_ARGB(255, 255, 0, 0);
}

void StgNormalShotObject::_SendDeleteEvent(TypeDelete type) {
	if (typeOwner_ != OWNER_ENEMY) return;

	auto stageScriptManager = stageController_->GetScriptManager();
	auto objectManager = stageController_->GetMainObjectManager();

	StgShotManager* shotManager = stageController_->GetShotManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (!shotManager->IsDeleteEventEnable(type)) return;

	{
		int typeEvent = StgShotManager::_TypeDeleteToEventType(type);

		{
			Math::DVec2 pos{ GetPositionX(), GetPositionY() };

			LOCK_WEAK(itemScript, stageScriptManager->GetItemScript()) {
				gstd::value listScriptValue[3];
				listScriptValue[0] = DxScript::CreateIntValue(idObject_);
				listScriptValue[1] = DxScript::CreateFloatArrayValue(pos);
				listScriptValue[2] = DxScript::CreateIntValue(GetShotDataID());
				itemScript->RequestEvent(typeEvent, listScriptValue, 3);
			}

			//Create default delete item
			if (type == TypeDelete::Item && itemManager->IsDefaultBonusItemEnable()) {
				if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
					ref_unsync_ptr<StgItemObject> obj(new StgItemObject_Bonus(stageController_));

					int id = objectManager->AddObject(obj);
					if (id != DxScript::ID_INVALID) {
						itemManager->AddItem(obj);
						obj->SetPositionX(pos[0]);
						obj->SetPositionY(pos[1]);
					}
				}
			}
		}
	}
}

void StgNormalShotObject::SetShotDataID(int id) {
	bool bPreserveSpin = id < 0;	//If id is negative, preserve angle and spin

	StgShotData* oldData = _GetShotData();
	StgShotObject::SetShotDataID(abs(id));

	StgShotData* shotData = _GetShotData();
	if (shotData != nullptr && oldData != shotData) {
		double newSpin = 0;

		double avMin = shotData->GetAngularVelocityMin();
		double avMax = shotData->GetAngularVelocityMax();
		if (avMin != avMax) {
			ref_count_ptr<StgStageInformation> stageInfo = stageController_->GetStageInformation();
			shared_ptr<RandProvider> rand = stageInfo->GetRandProvider();
			newSpin = rand->GetReal(avMin, avMax);
		}
		else newSpin = avMin;

		if (!bPreserveSpin) {
			angularVelocity_ = newSpin;
			angle_.z = 0;
		}
		bFixedAngle_ = shotData->IsFixedAngle();
	}
}

//****************************************************************************
//StgLaserObject(レーザー基本部)
//****************************************************************************
StgLaserObject::StgLaserObject(StgStageController* stageController) : StgShotObject(stageController) {
	life_ = 9999999;
	bSpellResist_ = true;

	length_ = 0;
	widthRender_ = 0;
	widthIntersection_ = -1;
	invalidLengthStart_ = 0.1f;
	invalidLengthEnd_ = 0.1f;
	frameGrazeInvalidStart_ = 20;
	itemDistance_ = 24;

	delay_ = DelayParameter(0.5, 3.5, 15.0f);

	move_ = D3DXVECTOR2(1, 0);
	lastAngle_ = 0;
}

void StgLaserObject::Clone(DxScriptObjectBase* _src) {
	StgShotObject::Clone(_src);

	auto src = (StgLaserObject*)_src;

	length_ = src->length_;
	widthRender_ = src->widthRender_;
	widthIntersection_ = src->widthIntersection_;

	invalidLengthStart_ = src->invalidLengthStart_;
	invalidLengthEnd_ = src->invalidLengthEnd_;
	itemDistance_ = src->itemDistance_;
}

void StgLaserObject::_AddIntersectionRelativeTarget() {
	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	if ((IsDeleted() || (delay_.time > 0 && !bEnableMotionDelay_) || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_)
		|| pOwnReference_.expired() || widthIntersection_ <= 0)
		return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return;

	ClearIntersected();

	bool res = GetIntersectionTargetList_NoVector(shotData);
	if (res) {
		for (auto& iTarget : listIntersectionTarget_) {
			if (iTarget.first && iTarget.second != nullptr)
				intersectionManager->AddTarget(iTarget.second);
		}
	}
}
StgIntersectionObject::IntersectionListType StgLaserObject::GetIntersectionTargetList() {
	if ((IsDeleted() || (delay_.time > 0 && !bEnableMotionDelay_) || frameFadeDelete_ >= 0)
		|| (bUserIntersectionMode_ || !bIntersectionEnable_)
		|| pOwnReference_.expired() || widthIntersection_ <= 0)
		return IntersectionListType();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr)
		return IntersectionListType();

	bool res = GetIntersectionTargetList_NoVector(shotData);
	if (res) return listIntersectionTarget_;

	return IntersectionListType();
}

//****************************************************************************
//StgLooseLaserObject(射出型レーザー)
//****************************************************************************
StgLooseLaserObject::StgLooseLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::LooseLaser;

	posTail_ = { 0, 0 };
	posOrigin_ = D3DXVECTOR2(0, 0);

	currentLength_ = 0;

	listIntersectionTarget_.push_back(CreateEmptyIntersection());
}

void StgLooseLaserObject::Clone(DxScriptObjectBase* _src) {
	StgLaserObject::Clone(_src);

	auto src = (StgLooseLaserObject*)_src;

	posTail_ = src->posTail_;
	posOrigin_ = src->posOrigin_;

	currentLength_ = src->currentLength_;
}

void StgLooseLaserObject::Work() {
	if (frameWork_ == 0) {
		posTail_[0] = posOrigin_.x = posX_;
		posTail_[1] = posOrigin_.y = posY_;
	}

	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();


		if (delay_.time > 0) {
			--(delay_.time);
			delay_.angle.x += delay_.angle.y;
		}
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
		currentLength_ = hypot(posTail_[0] - posX_, posTail_[1] - posY_);
		if (currentLength_ >= length_) {
			//float speed = GetSpeed();
			posTail_[0] = posX_ - length_ * move_.x;
			posTail_[1] = posY_ - length_ * move_.y;
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
	DxRect<LONG> rcDeleteClip(rcClipBase->left, rcClipBase->top,
		rcStgFrame->GetWidth() + rcClipBase->right,
		rcStgFrame->GetHeight() + rcClipBase->bottom);

	if (!rcDeleteClip.IsPointIntersected(posX_, posY_) && !rcDeleteClip.IsPointIntersected(posTail_)) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}

bool StgLooseLaserObject::GetIntersectionTargetList_NoVector(StgShotData* shotData) {
	if (abs(hitboxScale_.x) < 0.01f)
		return false;

	float invLengthS = (1.0f - (1.0f - invalidLengthStart_) * hitboxScale_.y) * 0.5f;
	float invLengthE = (1.0f - (1.0f - invalidLengthEnd_) * hitboxScale_.y) * 0.5f;

	float lineXS = Math::Lerp::Linear(posX_, posTail_[0], invLengthS);
	float lineYS = Math::Lerp::Linear(posY_, posTail_[1], invLengthS);
	float lineXE = Math::Lerp::Linear(posTail_[0], posX_, invLengthE);
	float lineYE = Math::Lerp::Linear(posTail_[1], posY_, invLengthE);

	{
		IntersectionPairType* pPair = &listIntersectionTarget_[0];

		StgIntersectionTarget_Line* pTarget = (StgIntersectionTarget_Line*)(pPair->second.get());
		if (pTarget == nullptr) {
			pTarget = new StgIntersectionTarget_Line();
			pPair->second.reset(pTarget);
		}
		pPair->first = true;

		DxWidthLine* pDstLine = &pTarget->GetLine();
		*pDstLine = DxWidthLine(lineXS, lineYS, lineXE, lineYE, widthIntersection_ * hitboxScale_.x);

		pTarget->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		pTarget->SetObject(pOwnReference_);
		pTarget->SetIntersectionSpace();
	}

	return true;
}

void StgLooseLaserObject::Render(BlendMode targetBlend) {
	//if (!IsVisible()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return;

	D3DXVECTOR2 rPos;
	D3DXVECTOR2 rScale;
	D3DXVECTOR2 rAngle;		//[cos, sin]
	D3DCOLOR rColor;

	//Render delay
	if (delay_.time > 0) {
		BlendMode objBlendType = GetDelayBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? MODE_BLEND_ADD_ARGB : objBlendType;

		if (objBlendType == targetBlend) {
			StgShotData* delayData = _GetShotData(delay_.id >= 0 ? delay_.id : shotData->GetDefaultDelayID());
			if (delayData) {
				StgShotDataFrame* delayFrame = delayData ? delayData->GetFrame(frameWork_) : nullptr;

				rPos = bEnableMotionDelay_ ? posOrigin_ : D3DXVECTOR2(position_);
				if (bRoundingPosition_) {
					rPos.x = roundf(rPos.x);
					rPos.y = roundf(rPos.y);
				}
				rScale.x = rScale.y = delay_.GetScale();
				rAngle = (delay_.angle.y != 0) ? D3DXVECTOR2(cosf(delay_.angle.x), sinf(delay_.angle.x)) : move_;

				rColor = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
				if (delay_.colorMix) ColorAccess::MultiplyColor(rColor, color_);
				{
					byte alpha = ColorAccess::ClampColorRet(((rColor >> 24) & 0xff) * delay_.GetAlpha());
					rColor = (rColor & 0x00ffffff) | (alpha << 24);
				}

				D3DXMATRIX matTransform(
					rScale.x * rAngle.x, rScale.x * rAngle.y, 0, 0,
					rScale.y * -rAngle.y, rScale.y * rAngle.x, 0, 0,
					0, 0, 1, 0,
					rPos.x, rPos.y, 0, 1
				);
				_DefaultShotRender(delayData, delayFrame, matTransform, rColor);
			}
		}
	}

	//Render laser
	if (delay_.time == 0 || bEnableMotionDelay_) {
		BlendMode objBlendType = GetBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? MODE_BLEND_ADD_ARGB : objBlendType;

		if (objBlendType == targetBlend) {
			StgShotDataFrame* shotFrame = shotData->GetFrame(frameWork_);

			float dx = posTail_[0] - posX_;
			float dy = posTail_[1] - posY_;

			if (currentLength_ > 0 && widthRender_ != 0) {
				rPos = D3DXVECTOR2(posX_ + posTail_[0], posY_ + posTail_[1]) / 2;	//Render from the laser center
				if (bRoundingPosition_) {
					rPos.x = roundf(rPos.x);
					rPos.y = roundf(rPos.y);
				}

				DxRect<float>* rcDst = shotFrame->GetDestRect();
				rScale.x = widthRender_ / rcDst->GetWidth() * scale_.x;
				rScale.y = currentLength_ / rcDst->GetHeight() * scale_.y;

				rAngle = D3DXVECTOR2(dy, -dx) / currentLength_;

				rColor = color_;
				{
					float alphaRate = shotData->GetAlpha() / 255.0f;
					if (frameFadeDelete_ >= 0)
						alphaRate *= std::clamp<float>((float)frameFadeDelete_ / FRAME_FADEDELETE, 0, 1);
					byte alpha = ColorAccess::ClampColorRet(((rColor >> 24) & 0xff) * alphaRate);
					rColor = (rColor & 0x00ffffff) | (alpha << 24);
				}

				D3DXMATRIX matTransform(
					rScale.x * rAngle.x, rScale.x * rAngle.y, 0, 0,
					rScale.y * -rAngle.y, rScale.y * rAngle.x, 0, 0,
					0, 0, 1, 0,
					rPos.x, rPos.y, 0, 1
				);
				_DefaultShotRender(shotData, shotFrame, matTransform, rColor);
			}

		}
	}
}

void StgLooseLaserObject::_SendDeleteEvent(TypeDelete type) {
	if (typeOwner_ != OWNER_ENEMY) return;

	auto stageScriptManager = stageController_->GetScriptManager();
	auto objectManager = stageController_->GetMainObjectManager();

	StgShotManager* shotManager = stageController_->GetShotManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (!shotManager->IsDeleteEventEnable(type)) return;

	auto itemScript = stageScriptManager->GetItemScript().lock();

	{
		int typeEvent = StgShotManager::_TypeDeleteToEventType(type);

		double ex = GetPositionX();
		double ey = GetPositionY();

		Math::DVec2 pos;
		gstd::value listScriptValue[3];

		for (double itemPos = 0; itemPos < currentLength_; itemPos += itemDistance_) {
			pos = { ex - itemPos * move_.x, ey - itemPos * move_.y };

			if (itemScript) {
				gstd::value listScriptValue[3];
				listScriptValue[0] = DxScript::CreateIntValue(idObject_);
				listScriptValue[1] = DxScript::CreateFloatArrayValue(pos);
				listScriptValue[2] = DxScript::CreateIntValue(GetShotDataID());
				itemScript->RequestEvent(typeEvent, listScriptValue, 3);
			}

			//Create default delete item
			if (type == TypeDelete::Item && itemManager->IsDefaultBonusItemEnable()) {
				if (delay_.time == 0 || bEnableMotionDelay_) {
					if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
						ref_unsync_ptr<StgItemObject> obj(new StgItemObject_Bonus(stageController_));

						int id = objectManager->AddObject(obj);
						if (id != DxScript::ID_INVALID) {
							itemManager->AddItem(obj);
							obj->SetPositionX(pos[0]);
							obj->SetPositionY(pos[1]);
						}
					}
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
	frameFadeDelete_ = -1;

	bUseSouce_ = true;
	bUseEnd_ = false;
	idImageEnd_ = -1;

	delaySize_ = D3DXVECTOR2(1, 1);

	scaleX_ = 0.05f;

	bLaserExpand_ = true;

	move_ = D3DXVECTOR2(1, 0);

	listIntersectionTarget_.push_back(CreateEmptyIntersection());
}

void StgStraightLaserObject::Clone(DxScriptObjectBase* _src) {
	StgLaserObject::Clone(_src);

	auto src = (StgStraightLaserObject*)_src;

	angLaser_ = src->angLaser_;

	bUseSouce_ = src->bUseSouce_;
	bUseEnd_ = src->bUseEnd_;
	idImageEnd_ = src->idImageEnd_;

	delaySize_ = src->delaySize_;
	scaleX_ = src->scaleX_;
	bLaserExpand_ = src->bLaserExpand_;
}

void StgStraightLaserObject::Work() {
	if (frameWork_ == 0 && delay_.time == 0) 
		scaleX_ = 1.0f;

	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();

		if (!bLaserExpand_ || delay_.time > 0) {
			if (delay_.time > 0) --(delay_.time);
			scaleX_ = std::max(0.05f, scaleX_ - 0.1f);
		}
		else if (bLaserExpand_)
			scaleX_ = std::min(1.0f, scaleX_ + 0.1f);

		delay_.angle.x += delay_.angle.y;

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
	DxRect<LONG> rcDeleteClip(rcClipBase->left, rcClipBase->top,
		rcStgFrame->GetWidth() + rcClipBase->right,
		rcStgFrame->GetHeight() + rcClipBase->bottom);

	double posXE = posX_ + length_ * move_.x;
	double posYE = posY_ + length_ * move_.y;

	if (!rcDeleteClip.IsPointIntersected(posX_, posY_) && !rcDeleteClip.IsPointIntersected(posXE, posYE)) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
bool StgStraightLaserObject::GetIntersectionTargetList_NoVector(StgShotData* shotData) {
	if (scaleX_ < 1 && typeOwner_ != OWNER_PLAYER)
		return false;

	float length = length_ * hitboxScale_.y;
	if (abs(hitboxScale_.x) < 0.01f || abs(length) < 0.01f)
		return false;

	double posXE = posX_ + length * move_.x;
	double posYE = posY_ + length * move_.y;
	float invLenHalfS = invalidLengthStart_ * 0.5f;
	float invLenHalfE = invalidLengthEnd_ * 0.5f;

	float lineXS = Math::Lerp::Linear(posX_, posXE, invLenHalfS);
	float lineYS = Math::Lerp::Linear(posY_, posYE, invLenHalfS);
	float lineXE = Math::Lerp::Linear(posXE, posX_, invLenHalfE);
	float lineYE = Math::Lerp::Linear(posYE, posY_, invLenHalfE);

	{
		IntersectionPairType* pPair = &listIntersectionTarget_[0];

		StgIntersectionTarget_Line* pTarget = (StgIntersectionTarget_Line*)(pPair->second.get());
		if (pTarget == nullptr) {
			pTarget = new StgIntersectionTarget_Line();
			pPair->second.reset(pTarget);
		}
		pPair->first = true;

		DxWidthLine* pDstLine = &pTarget->GetLine();
		*pDstLine = DxWidthLine(lineXS, lineYS, lineXE, lineYE, widthIntersection_ * hitboxScale_.x);

		pTarget->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		pTarget->SetObject(pOwnReference_);
		pTarget->SetIntersectionSpace();
	}

	return true;
}

void StgStraightLaserObject::Render(BlendMode targetBlend) {
	//if (!IsVisible()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return;

	D3DCOLOR rColor;

	//Render laser
	{
		BlendMode objBlendType = GetBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? MODE_BLEND_ADD_ARGB : objBlendType;

		if (objBlendType == targetBlend) {
			StgShotDataFrame* shotFrame = shotData->GetFrame(frameWork_);

			D3DXVECTOR2 rAngle(move_.y, -move_.x);

			float _renderWd = std::max<float>(abs(widthRender_) * scaleX_, 2.0f) * scale_.x;
			float _renderLn = length_ * scale_.y;

			//Render from the laser center
			D3DXVECTOR2 rPos = D3DXVECTOR2(posX_ * 2 + move_.x * _renderLn, posY_ * 2 + move_.y * _renderLn) / 2;
			if (bRoundingPosition_) {
				rPos.x = roundf(rPos.x);
				rPos.y = roundf(rPos.y);
			}

			DxRect<float>* rcDst = shotFrame->GetDestRect();
			D3DXVECTOR2 rScale(_renderWd / rcDst->GetWidth(), _renderLn / rcDst->GetHeight());

			rColor = color_;
			{
				float alphaRate = shotData->GetAlpha() / 255.0f;
				if (frameFadeDelete_ >= 0)
					alphaRate *= std::clamp<float>((float)frameFadeDelete_ / FRAME_FADEDELETE_LASER, 0, 1);
				byte alpha = ColorAccess::ClampColorRet(((rColor >> 24) & 0xff) * alphaRate);
				rColor = (rColor & 0x00ffffff) | (alpha << 24);
			}

			D3DXMATRIX matTransform(
				rScale.x * -rAngle.x, rScale.x * -rAngle.y, 0, 0,
				rScale.y * rAngle.y, rScale.y * -rAngle.x, 0, 0,
				0, 0, 1, 0,
				rPos.x, rPos.y, 0, 1
			);
			_DefaultShotRender(shotData, shotFrame, matTransform, rColor);
		}
	}

	//Render delay(s)
	if ((bUseSouce_ || bUseEnd_) && (frameFadeDelete_ < 0)) {
		BlendMode objBlendType = GetDelayBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? MODE_BLEND_ADD_ARGB : objBlendType;

		if (objBlendType == targetBlend) {
			rColor = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
			if (delay_.colorMix) ColorAccess::MultiplyColor(rColor, color_);

			const float delaySizeBase = widthRender_ * 4 / 3.0f;

			auto _AddDelay = [&](D3DXVECTOR2 delayPos, int delayID, float delaySize) {
				if (bRoundingPosition_) {
					delayPos.x = roundf(delayPos.x);
					delayPos.y = roundf(delayPos.y);
				}
				delaySize *= delaySizeBase;

				StgShotData* delayData = _GetShotData(delay_.id >= 0 ? delay_.id : shotData->GetDefaultDelayID());
				if (delayData) {
					StgShotDataFrame* delayFrame = delayData->GetFrame(frameWork_);

					D3DXVECTOR2 rAngle = (delay_.angle.y != 0) ? D3DXVECTOR2(cosf(delay_.angle.x), sinf(delay_.angle.x)) : move_;
					float rScaleX = delaySize / delayFrame->GetDestRect()->GetWidth();
					float rScaleY = delaySize / delayFrame->GetDestRect()->GetHeight();

					D3DXMATRIX matTransform(
						rScaleX * rAngle.x, rScaleX * rAngle.y, 0, 0,
						rScaleY * -rAngle.y, rScaleY * rAngle.x, 0, 0,
						0, 0, 1, 0,
						delayPos.x, delayPos.y, 0, 1
					);
					_DefaultShotRender(delayData, delayFrame, matTransform, rColor);
				}
			};

			if (bUseSouce_) {
				D3DXVECTOR2 delayPos(position_);
				_AddDelay(delayPos, delay_.id, delaySize_.x);
			}
			if (bUseEnd_) {
				D3DXVECTOR2 delayPos(position_.x + length_ * cosf(angLaser_),
					position_.y + length_ * sinf(angLaser_));
				_AddDelay(delayPos, idImageEnd_, delaySize_.y);
			}
		}
	}
}

void StgStraightLaserObject::_SendDeleteEvent(TypeDelete type) {
	if (typeOwner_ != OWNER_ENEMY) return;

	auto stageScriptManager = stageController_->GetScriptManager();
	auto objectManager = stageController_->GetMainObjectManager();

	StgShotManager* shotManager = stageController_->GetShotManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (!shotManager->IsDeleteEventEnable(type)) return;

	auto itemScript = stageScriptManager->GetItemScript().lock();

	{
		int typeEvent = StgShotManager::_TypeDeleteToEventType(type);

		Math::DVec2 pos;
		gstd::value listScriptValue[3];

		for (double itemPos = 0; itemPos < length_; itemPos += itemDistance_) {
			pos = { posX_ + itemPos * move_.x, posY_ + itemPos * move_.y };

			if (itemScript) {
				gstd::value listScriptValue[3];
				listScriptValue[0] = DxScript::CreateIntValue(idObject_);
				listScriptValue[1] = DxScript::CreateFloatArrayValue(pos);
				listScriptValue[2] = DxScript::CreateIntValue(GetShotDataID());
				itemScript->RequestEvent(typeEvent, listScriptValue, 3);
			}

			//Create default delete item
			if (type == TypeDelete::Item && itemManager->IsDefaultBonusItemEnable()) {
				if (delay_.time == 0) {
					if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
						ref_unsync_ptr<StgItemObject> obj(new StgItemObject_Bonus(stageController_));

						int id = objectManager->AddObject(obj);
						if (id != DxScript::ID_INVALID) {
							itemManager->AddItem(obj);
							obj->SetPositionX(pos[0]);
							obj->SetPositionY(pos[1]);
						}
					}
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

	bCap_ = false;
	posOrigin_ = D3DXVECTOR2(0, 0);
}

void StgCurveLaserObject::Clone(DxScriptObjectBase* _src) {
	StgLaserObject::Clone(_src);

	auto src = (StgCurveLaserObject*)_src;

	listPosition_ = src->listPosition_;
	vertexData_ = src->vertexData_;
	listRectIncrement_ = src->listRectIncrement_;

	posXO_ = src->posXO_;
	posYO_ = src->posYO_;
	tipDecrement_ = src->tipDecrement_;
	bCap_ = src->bCap_;
	posOrigin_ = src->posOrigin_;
}

void StgCurveLaserObject::Work() {
	if (frameWork_ == 0) {
		posOrigin_.x = posX_;
		posOrigin_.y = posY_;
	}

	if (bEnableMovement_) {
		_ProcessTransformAct();
		_Move();

		if (delay_.time > 0) {
			--(delay_.time);
			delay_.angle.x += delay_.angle.y;
		}
	}

	_CommonWorkTask();
	//	_AddIntersectionRelativeTarget();
}
void StgCurveLaserObject::_Move() {
	if (delay_.time == 0 || bEnableMotionDelay_)
		StgMoveObject::_Move();
	DxScriptRenderObject::SetX(posX_);
	DxScriptRenderObject::SetY(posY_);

	{
		double angleZ = GetDirectionAngle();
		if (lastAngle_ != angleZ) {
			lastAngle_ = angleZ;
			move_ = D3DXVECTOR2(cosf(lastAngle_), sinf(lastAngle_));
		}

		D3DXVECTOR2 newNodePos(posX_, posY_);
		D3DXVECTOR2 newNodeVertF(-move_.y, move_.x);	//90 degrees rotation
		PushNode(CreateNode(newNodePos, newNodeVertF, 1.0f));
	}
}

StgCurveLaserObject::LaserNode StgCurveLaserObject::CreateNode(const D3DXVECTOR2& pos, const D3DXVECTOR2& rFac, float widthMul, D3DCOLOR col) {
	LaserNode node;
	node.parent = this;
	node.pos = pos;
	{
		float nx = rFac.x;
		float ny = rFac.y;
		node.vertOff[0] = { nx, ny };
		node.vertOff[1] = { -nx, -ny };
	}
	node.widthMul = widthMul;
	node.color = col;
	return node;
}
bool StgCurveLaserObject::GetNode(size_t indexNode, std::list<LaserNode>::iterator& res) {
	//Search from whichever end is closer to the target node index
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
	while (listPosition_.size() > length_)
		listPosition_.pop_back();
	return listPosition_.begin();
}

void StgCurveLaserObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetShotManager()->GetShotDeleteClip();
	DxRect<LONG> rcDeleteClip(rcClipBase->left, rcClipBase->top,
		rcStgFrame->GetWidth() + rcClipBase->right,
		rcStgFrame->GetHeight() + rcClipBase->bottom);

	//Checks if the node is within the bounding rect
	auto PredicateNodeInRect = [&](LaserNode& node) {
		return rcDeleteClip.IsPointIntersected((float*)&node.pos);
	};
	std::list<LaserNode>::iterator itrFind = std::find_if(listPosition_.begin(), listPosition_.end(),
		PredicateNodeInRect);

	//Can't find any node within the bounding rect
	if (itrFind == listPosition_.end()) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
bool StgCurveLaserObject::GetIntersectionTargetList_NoVector(StgShotData* shotData) {
	if (abs(hitboxScale_.x) < 0.01f || abs(hitboxScale_.y) < 0.01f)
		return false;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	size_t countPos = listPosition_.size();
	size_t countIntersection = countPos > 0U ? countPos - 1U : 0U;

	if (countIntersection == 0)
		return false;

	if (listIntersectionTarget_.size() < countIntersection)
		listIntersectionTarget_.resize(countIntersection, CreateEmptyIntersection());
	for (auto& i : listIntersectionTarget_) i.first = false;

	float iLengthS = invalidLengthStart_ * 0.5f;
	float iLengthE = 0.5f + (1.0f - invalidLengthEnd_) * 0.5f;
	int posInvalidS = (int)(countPos * iLengthS);
	int posInvalidE = (int)(countPos * iLengthE);
	float iWidth = widthIntersection_ * hitboxScale_.x;

	std::list<LaserNode>::iterator itr = listPosition_.begin();
	for (size_t iPos = 0; iPos < countIntersection; ++iPos, ++itr) {
		IntersectionPairType* pPair = &listIntersectionTarget_[iPos];

		if ((int)iPos < posInvalidS || (int)iPos > posInvalidE) {
			pPair->first = false;
			continue;
		}

		StgIntersectionTarget_Line* pTarget = (StgIntersectionTarget_Line*)(pPair->second.get());
		if (pTarget == nullptr) {
			pTarget = new StgIntersectionTarget_Line();
			pPair->second.reset(pTarget);
		}
		pPair->first = true;

		std::list<LaserNode>::iterator itrNext = std::next(itr);
		D3DXVECTOR2* nodeS = &itr->pos;
		D3DXVECTOR2* nodeE = &itrNext->pos;

		DxWidthLine* pDstLine = &pTarget->GetLine();
		*pDstLine = DxWidthLine(nodeS->x, nodeS->y, nodeE->x, nodeE->y, iWidth);

		pTarget->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		pTarget->SetObject(pOwnReference_);
		pTarget->SetIntersectionSpace();
	}

	return true;
}

void StgCurveLaserObject::Render(BlendMode targetBlend) {
	//if (!IsVisible()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return;

	//Render delay
	if (delay_.time > 0) {
		BlendMode objBlendType = GetDelayBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? MODE_BLEND_ADD_ARGB : objBlendType;

		if (objBlendType == targetBlend) {
			StgShotData* delayData = _GetShotData(delay_.id >= 0 ? delay_.id : shotData->GetDefaultDelayID());
			if (delayData) {
				StgShotDataFrame* shotFrame = delayData->GetFrame(frameWork_);

				D3DXVECTOR2 rScale;
				D3DXVECTOR2 rAngle;		//[cos, sin]
				D3DCOLOR rColor;

				D3DXVECTOR2 rPos = bEnableMotionDelay_ ? posOrigin_ : D3DXVECTOR2(position_);
				if (bRoundingPosition_) {
					rPos.x = roundf(rPos.x);
					rPos.y = roundf(rPos.y);
				}
				rScale.x = rScale.y = delay_.GetScale();
				rAngle = (delay_.angle.y != 0) ? D3DXVECTOR2(cosf(delay_.angle.x), sinf(delay_.angle.x)) : move_;

				rColor = (delay_.colorRep != 0) ? delay_.colorRep : shotData->GetDelayColor();
				if (delay_.colorMix) ColorAccess::MultiplyColor(rColor, color_);
				{
					byte alpha = ColorAccess::ClampColorRet(((rColor >> 24) & 0xff) * delay_.GetAlpha());
					rColor = (rColor & 0x00ffffff) | (alpha << 24);
				}

				D3DXMATRIX matTransform(
					rScale.x * rAngle.x, rScale.x * rAngle.y, 0, 0,
					rScale.y * -rAngle.y, rScale.y * rAngle.x, 0, 0,
					0, 0, 1, 0,
					rPos.x, rPos.y, 0, 1
				);
				_DefaultShotRender(delayData, shotFrame, matTransform, rColor);
			}
		}
	}

	//Render laser
	if (listPosition_.size() > 1U) {
		BlendMode objBlendType = GetBlendType();
		objBlendType = objBlendType == MODE_BLEND_NONE ? MODE_BLEND_ADD_ARGB : objBlendType;

		if (objBlendType == targetBlend) {
			StgShotDataFrame* shotFrame = shotData->GetFrame(frameWork_);

			size_t countPos = listPosition_.size();
			size_t countRect = countPos - 1U;
			size_t halfPos = countRect / 2U;

			shared_ptr<Texture> texture = shotFrame->GetVertexBufferContainer()->GetTexture();
			D3DXVECTOR2 texSizeInv = D3DXVECTOR2(1.0f / texture->GetWidth(), 1.0f / texture->GetHeight());

			const DxRect<LONG>* rcSrcOrg = shotFrame->GetSourceRect();
			const LONG* ptrSrc = reinterpret_cast<const LONG*>(rcSrcOrg);

			float alphaRateShot = shotData->GetAlpha() / 255.0f;
			if (frameFadeDelete_ >= 0)
				alphaRateShot *= std::clamp<float>((float)frameFadeDelete_ / FRAME_FADEDELETE, 0, 1);

			float baseAlpha = (color_ >> 24) & 0xff;
			float tipAlpha = baseAlpha * (1.0f - tipDecrement_);

			float rcLen = rcSrcOrg->bottom - rcSrcOrg->top;
			float rcLenH = rcLen * 0.5f;

			float rcInc = (rcLen / (float)countRect) * texSizeInv.y;
			float rectV = rcSrcOrg->top * texSizeInv.y;

			float incDistFactor = rcLen * texSizeInv.y / widthRender_;
			float rcMidPt = rcLenH * texSizeInv.y;

			listRectIncrement_.resize(countPos);
			std::fill(listRectIncrement_.begin(), listRectIncrement_.end(), 0);
			{
				bool bCappable = false;
				if (bCap_) {
					// :WHAT:

					size_t i = 0;
					size_t iPos = 0;
					float remLen = rcMidPt;

					auto tryCap = [&](auto itr) -> bool {
						if (i > halfPos) // Auto-fails if cap crosses the half-way point
							return false;

						auto itrNext = std::next(itr);
						D3DXVECTOR2* pos = &itr->pos;
						D3DXVECTOR2* posNext = &itrNext->pos;
						// D3DXVECTOR2* off = &itr->vertOff[0];
						// float wid = std::max(hypotf(off->x, off->y) * 2, 1.0f);
						float incDist = hypotf(posNext->x - pos->x, posNext->y - pos->y) * incDistFactor;

						if (listRectIncrement_[iPos] == 0) // Fails if element was already written to
							listRectIncrement_[iPos] = std::min(incDist, remLen);
						else
							return false;

						remLen -= incDist;
						return true;
					};

					auto itrHead = listPosition_.begin();
					auto itrTail = listPosition_.rbegin();
					auto itrHeadEnd = listPosition_.rend();
					auto itrTailEnd = listPosition_.end();

					bCappable = true;
					for (auto itr = itrHead; bCappable && remLen > 0 && itr != itrTailEnd; ++itr, ++i, ++iPos)
						bCappable = tryCap(itr);

					i = 0;
					iPos = countPos - 2; // Ends straight up do not work otherwise?
					remLen = rcMidPt;
					for (auto itr = itrTail; bCappable && remLen > 0 && itr != itrHeadEnd; ++itr, ++i, --iPos)
						bCappable = tryCap(itr);
				}
				if (!bCappable) // If capping fails (or is disabled), just use the regular increment
					std::fill(listRectIncrement_.begin(), listRectIncrement_.end(), rcInc);
			}

			vertexData_.resize(countPos * 2U);

			float inv_halfPos = 1.0f / halfPos, inv_halfPosDec = 1.0f / (halfPos - 1);
			float halfWidthRender = widthRender_ / 2.0f;

			size_t iPos = 0U;
			for (auto itr = listPosition_.begin(); itr != listPosition_.end(); ++itr, ++iPos) {
				float nodeAlpha = baseAlpha;
				if (iPos > halfPos)
					nodeAlpha = Math::Lerp::Linear(baseAlpha, tipAlpha, (iPos - halfPos + 1) * inv_halfPos);
				else if (iPos < halfPos)
					nodeAlpha = Math::Lerp::Linear(tipAlpha, baseAlpha, iPos * inv_halfPosDec);
				nodeAlpha = std::max(0.0f, nodeAlpha);

				float renderWd = std::max(halfWidthRender * itr->widthMul, 1.0f) * scale_.x;

				D3DCOLOR thisColor = 0xffffffff;
				{
					byte alpha = ColorAccess::ClampColorRet(nodeAlpha * alphaRateShot);
					thisColor = (thisColor & 0x00ffffff) | (alpha << 24);
				}
				if (itr->color != 0xffffffff) ColorAccess::MultiplyColor(thisColor, itr->color);

				for (size_t iVert = 0U; iVert < 2U; ++iVert) {
					VERTEX_TLX* pv = &vertexData_[iPos * 2 + iVert];

					_SetVertexUV(pv, ptrSrc[(iVert & 1) << 1] * texSizeInv.x, rectV);
					_SetVertexPosition(pv, itr->pos.x + itr->vertOff[iVert].x * renderWd,
						itr->pos.y + itr->vertOff[iVert].y * renderWd, position_.z);
					_SetVertexColorARGB(pv, thisColor);
				}

				rectV += listRectIncrement_[iPos];
			}

			{
				DirectGraphics* graphics = DirectGraphics::GetBase();
				IDirect3DDevice9* device = graphics->GetDevice();

				VertexBufferManager* vbManager = VertexBufferManager::GetBase();
				FixedVertexBuffer* vertexBuffer = vbManager->GetVertexBufferTLX();

				if (graphics->IsAllowRenderTargetChange()) {
					if (auto pRT = renderTarget_.lock())
						graphics->SetRenderTarget(pRT);
					else graphics->SetRenderTarget(nullptr);
				}

				IDirect3DTexture9* pTexture = texture->GetD3DTexture();
				if (pTexture != shotManager->pLastTexture_) {
					device->SetTexture(0, pTexture);
					shotManager->pLastTexture_ = pTexture;
				}

				size_t countVert = vertexData_.size();
				size_t countPrim = RenderObjectPrimitive::GetPrimitiveCount(D3DPT_TRIANGLESTRIP, countVert);

				{
					BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

					lockParam.SetSource(vertexData_, countVert, sizeof(VERTEX_TLX));
					vertexBuffer->UpdateBuffer(&lockParam);
				}

				device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_TLX));

				{
					ID3DXEffect* effect = shotManager->GetEffect();
					if (shader_) {
						effect = shader_->GetEffect();
						if (shader_->LoadTechnique()) {
							shader_->LoadParameter();
						}
					}

					if (effect) {
						D3DXHANDLE handle = nullptr;
						if (handle = effect->GetParameterBySemantic(nullptr, "WORLD")) {
							effect->SetMatrix(handle, &graphics->GetCamera()->GetIdentity());
						}
						if (shader_) {
							if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION")) {
								effect->SetMatrix(handle, shotManager->GetProjectionMatrix());
							}
						}
						if (handle = effect->GetParameterBySemantic(nullptr, "ICOLOR")) {
							//To normalized RGBA vector
							D3DXVECTOR4 vColor = ColorAccess::ToVec4Normalized(color_, ColorAccess::PERMUTE_RGBA);
							effect->SetVector(handle, &vColor);
						}

						UINT countPass = 1;
						effect->Begin(&countPass, D3DXFX_DONOTSAVESHADERSTATE);
						for (UINT iPass = 0; iPass < countPass; ++iPass) {
							effect->BeginPass(iPass);
							device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, countPrim);
							effect->EndPass();
						}
						effect->End();
					}
				}
			}
		}
	}
}

void StgCurveLaserObject::_SendDeleteEvent(TypeDelete type) {
	if (typeOwner_ != OWNER_ENEMY) return;

	auto stageScriptManager = stageController_->GetScriptManager();
	auto objectManager = stageController_->GetMainObjectManager();

	StgShotManager* shotManager = stageController_->GetShotManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (!shotManager->IsDeleteEventEnable(type)) return;

	auto itemScript = stageScriptManager->GetItemScript().lock();

	{
		int typeEvent = StgShotManager::_TypeDeleteToEventType(type);

		gstd::value listScriptValue[3];
		listScriptValue[0] = DxScript::CreateIntValue(idObject_);
		listScriptValue[2] = DxScript::CreateIntValue(GetShotDataID());

		size_t countToItem = 0U;
		auto _RequestItem = [&](double ix, double iy) {
			if (itemScript) {
				listScriptValue[1] = itemScript->CreateFloatArrayValue(Math::DVec2{ ix, iy });
				itemScript->RequestEvent(typeEvent, listScriptValue, 3);
			}

			//Create default delete item
			if (type == TypeDelete::Item && itemManager->IsDefaultBonusItemEnable()) {
				if (delay_.time == 0 || bEnableMotionDelay_) {
					if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
						ref_unsync_ptr<StgItemObject> obj(new StgItemObject_Bonus(stageController_));

						int id = objectManager->AddObject(obj);
						if (id != DxScript::ID_INVALID) {
							itemManager->AddItem(obj);
							obj->SetPositionX(ix);
							obj->SetPositionY(iy);
						}
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
				_RequestItem(Math::Lerp::Linear(pos->x, posNext->x, lerpMul),
					Math::Lerp::Linear(pos->y, posNext->y, lerpMul));
				createDist += itemDistance_;
				lengthAcc -= itemDistance_;
			}
		}
	}
}

//****************************************************************************
//StgShotPatternGeneratorObject (ECL-style bullets firing)
//****************************************************************************
StgShotPatternGeneratorObject::StgShotPatternGeneratorObject(StgStageController* stageController) : StgObjectBase(stageController) {
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

	delay_ = 0;
	//delayMove_ = false;

	laserWidth_ = 16;
	laserLength_ = 64;
}

void StgShotPatternGeneratorObject::Clone(DxScriptObjectBase* _src) {
	DxScriptObjectBase::Clone(_src);

	auto src = (StgShotPatternGeneratorObject*)_src;

	parent_ = src->parent_;

	idShotData_ = src->idShotData_;
	typeOwner_ = src->typeOwner_;
	typeShot_ = src->typeShot_;
	typePattern_ = src->typePattern_;
	iniBlendType_ = src->iniBlendType_;

	shotWay_ = src->shotWay_;
	shotStack_ = src->shotStack_;

	basePointX_ = src->basePointX_;
	basePointY_ = src->basePointY_;
	basePointOffsetX_ = src->basePointOffsetX_;
	basePointOffsetY_ = src->basePointOffsetY_;
	fireRadiusOffset_ = src->fireRadiusOffset_;

	speedBase_ = src->speedBase_;
	speedArgument_ = src->speedArgument_;
	angleBase_ = src->angleBase_;
	angleArgument_ = src->angleArgument_;

	delay_ = src->delay_;
	//delayMove_ = src->delayMove_;

	laserWidth_ = src->laserWidth_;
	laserLength_ = src->laserLength_;

	listTransformation_ = src->listTransformation_;
}

void StgShotPatternGeneratorObject::SetTransformation(size_t off, StgShotPatternTransform& entry) {
	if (off >= listTransformation_.size()) listTransformation_.resize(off + 1);
	listTransformation_[off] = entry;
}

void StgShotPatternGeneratorObject::FireSet(void* scriptData, StgStageController* controller, std::vector<int>* idVector) {
	if (idVector) idVector->clear();

	StgStageScript* script = (StgStageScript*)scriptData;

	ref_unsync_ptr<StgPlayerObject> objPlayer = controller->GetPlayerObject();
	auto objManager = controller->GetMainObjectManager();
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

	std::list<StgShotPatternTransform> transformAsList;
	for (StgShotPatternTransform& iTransform : listTransformation_)
		transformAsList.push_back(iTransform);

	auto __CreateShot = [&](float _x, float _y, double _ss, double _sa) -> bool {
		if (shotManager->GetShotCountAll() >= StgShotManager::SHOT_MAX) return false;

		ref_unsync_ptr<StgShotObject> objShot;
		switch (typeShot_) {
		case TypeObject::Shot:
		{
			ref_unsync_ptr<StgNormalShotObject> ptrShot(new StgNormalShotObject(controller));
			objShot = ptrShot;
			break;
		}
		case TypeObject::LooseLaser:
		{
			ref_unsync_ptr<StgLooseLaserObject> ptrShot(new StgLooseLaserObject(controller));
			ptrShot->SetLength(laserLength_);
			ptrShot->SetRenderWidth(laserWidth_);
			objShot = ptrShot;
			break;
		}
		case TypeObject::StraightLaser:
		{
			ref_unsync_ptr<StgStraightLaserObject> ptrShot(new StgStraightLaserObject(controller));
			ptrShot->SetLength(laserLength_);
			ptrShot->SetRenderWidth(laserWidth_);
			objShot = ptrShot;
			break;
		}
		case TypeObject::CurveLaser:
		{
			ref_unsync_ptr<StgCurveLaserObject> ptrShot(new StgCurveLaserObject(controller));
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
			double ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_FAN_AIMED)
				ini_angle += atan2(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);
			double ang_off_way = (double)(shotWay_ / 2) - (shotWay_ % 2 == 0 ? 0.5 : 0.0);

			for (size_t iWay = 0; iWay < shotWay_; ++iWay) {
				double sa = ini_angle + (iWay - ang_off_way) * angleArgument_;
				double r_fac[2] = { cos(sa), sin(sa) };

				for (size_t iStack = 0; iStack < shotStack_; ++iStack) {
					double ss = speedBase_;
					if (shotStack_ > 1) ss += (speedArgument_ - speedBase_) * (iStack / ((double)shotStack_ - 1));

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
			double ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_RING_AIMED)
				ini_angle += atan2(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			for (size_t iWay = 0; iWay < shotWay_; ++iWay) {
				double sa_b = ini_angle + (GM_PI_X2 / (double)shotWay_) * iWay;

				for (size_t iStack = 0; iStack < shotStack_; ++iStack) {
					double ss = speedBase_;
					if (shotStack_ > 1) ss += (speedArgument_ - speedBase_) * (iStack / ((double)shotStack_ - 1));

					double sa = sa_b + iStack * angleArgument_;
					float sx = basePosX + fireRadiusOffset_ * cos(sa);
					float sy = basePosY + fireRadiusOffset_ * sin(sa);

					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_ARROW:
		case PATTERN_TYPE_ARROW_AIMED:
		{
			double ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_ARROW_AIMED)
				ini_angle += atan2(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);
			size_t stk_cen = shotStack_ / 2;

			for (size_t iStack = 0U; iStack < shotStack_; ++iStack) {
				double ss = speedBase_;
				if (shotStack_ > 1) {
					if (shotStack_ % 2 == 0) {
						if (shotStack_ > 2) {
							double tmp = (iStack < stk_cen) ? (stk_cen - iStack - 1) : (iStack - stk_cen);
							ss = speedBase_ + (speedArgument_ - speedBase_) * (tmp / (stk_cen - 1));
						}
					}
					else {
						double tmp = abs((double)iStack - stk_cen);
						ss = speedBase_ + (speedArgument_ - speedBase_) * (tmp / std::max(1U, stk_cen - 1));
					}
				}

				for (size_t iWay = 0; iWay < shotWay_; ++iWay) {
					double sa = ini_angle + (GM_PI_X2 / (double)shotWay_) * iWay;
					if (shotStack_ > 1) {
						sa += (double)((shotStack_ % 2 == 0) ?
							((double)iStack - (stk_cen - 0.5)) : ((double)iStack - stk_cen)) * angleArgument_;
					}

					float sx = basePosX + fireRadiusOffset_ * cos(sa);
					float sy = basePosY + fireRadiusOffset_ * sin(sa);

					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_POLYGON:
		case PATTERN_TYPE_POLYGON_AIMED:
		{
			double ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_POLYGON_AIMED)
				ini_angle += atan2(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			size_t numEdges = shotWay_;
			size_t numShotPerEdge = shotStack_;
			int edgeSkip = std::round(Math::RadianToDegree(angleArgument_));

			double r_fac[2] = { cos(ini_angle), sin(ini_angle) };

			for (size_t iEdge = 0; iEdge < numEdges; ++iEdge) {
				double from_ang = (GM_PI_X2 / numEdges) * (double)iEdge;
				double to_ang = (GM_PI_X2 / numEdges) * (double)((int)iEdge + edgeSkip);
				double from_pos[2] = { cos(from_ang), sin(from_ang) };
				double to_pos[2] = { cos(to_ang), sin(to_ang) };

				for (size_t iShot = 0; iShot < numShotPerEdge; ++iShot) {
					//Will always be just a little short of a full 1, intentional.
					double rate = iShot / (double)numShotPerEdge;

					double _sx_b = Math::Lerp::Linear(from_pos[0], to_pos[0], rate);
					double _sy_b = Math::Lerp::Linear(from_pos[1], to_pos[1], rate);
					double _sx = _sx_b * r_fac[0] + _sy_b * r_fac[1];
					double _sy = _sx_b * r_fac[1] - _sy_b * r_fac[0];
					float sx = basePosX + fireRadiusOffset_ * _sx;
					float sy = basePosY + fireRadiusOffset_ * _sy;

					double sa = atan2(_sy, _sx);
					double ss = hypot(_sx, _sy) * speedBase_;

					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_ELLIPSE:
		case PATTERN_TYPE_ELLIPSE_AIMED:
		{
			double el_pointing_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_ELLIPSE_AIMED)
				el_pointing_angle += atan2(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			double r_eccentricity = speedArgument_ / speedBase_;

			for (size_t iWay = 0; iWay < shotWay_; ++iWay) {
				double angle_cur = GM_PI_X2 / (double)shotWay_ * iWay + angleArgument_;

				double rpos[2] = { 1 * cos(angle_cur), r_eccentricity * sin(angle_cur) };
				Math::Rotate2D(rpos, el_pointing_angle, 0, 0);

				double sa = atan2(rpos[1], rpos[0]);
				double ss = hypot(rpos[0], rpos[1]) * speedBase_;
				float sx = basePosX + fireRadiusOffset_ * rpos[0];
				float sy = basePosY + fireRadiusOffset_ * rpos[1];

				__CreateShot(sx, sy, ss, sa);
			}
			break;
		}
		case PATTERN_TYPE_SCATTER_ANGLE:
		case PATTERN_TYPE_SCATTER_SPEED:
		case PATTERN_TYPE_SCATTER:
		{
			double ini_angle = angleBase_;

			for (size_t iWay = 0; iWay < shotWay_; ++iWay) {
				for (size_t iStack = 0; iStack < shotStack_; ++iStack) {
					double ss = speedBase_ + (speedArgument_ - speedBase_) *
						((shotStack_ > 1 && typePattern_ == PATTERN_TYPE_SCATTER_ANGLE) ?
							(iStack / ((double)shotStack_ - 1)) : randGenerator->GetReal());

					double sa = ini_angle + ((typePattern_ == PATTERN_TYPE_SCATTER_SPEED) ?
						(GM_PI_X2 / (double)shotWay_ * iWay) + angleArgument_ * iStack :
						randGenerator->GetReal(-angleArgument_, angleArgument_));

					float sx = basePosX + fireRadiusOffset_ * cos(sa);
					float sy = basePosY + fireRadiusOffset_ * sin(sa);

					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_LINE:
		case PATTERN_TYPE_LINE_AIMED:
		{
			double ini_angle = angleBase_;
			double angle_off = angleArgument_ / 2;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_LINE_AIMED)
				ini_angle += atan2(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			double from_ang = ini_angle + angle_off;
			double to_ang = ini_angle - angle_off;

			double from_pos[2] = { cos(from_ang), sin(from_ang) };
			double to_pos[2] = { cos(to_ang), sin(to_ang) };

			for (size_t iWay = 0U; iWay < shotWay_; ++iWay) {
				//Will always be just a little short of a full 1, intentional.
				double rate = shotWay_ > 1 ? (iWay / ((double)shotWay_ - 1)) : 0.5;

				double _sx = Math::Lerp::Linear(from_pos[0], to_pos[0], rate);
				double _sy = Math::Lerp::Linear(from_pos[1], to_pos[1], rate);
				float sx = basePosX + fireRadiusOffset_ * _sx;
				float sy = basePosY + fireRadiusOffset_ * _sy;

				double sa = atan2(_sy, _sx);
				double _ss = hypot(_sx, _sy);

				for (size_t iStack = 0; iStack < shotStack_; ++iStack) {
					double ss = speedBase_;
					if (shotStack_ > 1) ss += (speedArgument_ - speedBase_) * (iStack / ((double)shotStack_ - 1));

					__CreateShot(sx, sy, ss * _ss, sa);
				}
			}
			break;
		}
		case PATTERN_TYPE_ROSE:
		case PATTERN_TYPE_ROSE_AIMED:
		{
			double ini_angle = angleBase_;
			if (objPlayer != nullptr && typePattern_ == PATTERN_TYPE_ROSE_AIMED)
				ini_angle += atan2(objPlayer->GetY() - basePosY, objPlayer->GetX() - basePosX);

			size_t numPetal = shotWay_;
			size_t numShotPerPetal = shotStack_;
			int petalSkip = std::round(Math::RadianToDegree(angleArgument_));

			double petalGap = GM_PI_X2 / numPetal;
			double angGap = (GM_PI_X2 / (numPetal * numShotPerPetal) * petalSkip);

			for (size_t iStack = 0; iStack < numShotPerPetal; ++iStack) {
				double ss = speedBase_ + (speedArgument_ - speedBase_) * sin(GM_PI / numShotPerPetal * iStack);

				for (size_t iShot = 0; iShot < numPetal; ++iShot) {
					double sa = ini_angle + iShot * petalGap + iStack * angGap;
					float sx = basePosX + fireRadiusOffset_ * cos(sa);
					float sy = basePosY + fireRadiusOffset_ * sin(sa);

					__CreateShot(sx, sy, ss, sa);
				}
			}
			break;
		}
		}
	}
}