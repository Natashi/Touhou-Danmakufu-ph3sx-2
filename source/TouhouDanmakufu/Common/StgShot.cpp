#include "source/GcLib/pch.h"

#include "StgShot.hpp"
#include "StgSystem.hpp"
#include "StgIntersection.hpp"
#include "StgItem.hpp"
#include "../../GcLib/directx/HLSL.hpp"

/**********************************************************
//StgShotManager
**********************************************************/
StgShotManager::StgShotManager(StgStageController* stageController) {
	stageController_ = stageController;

	listPlayerShotData_ = new StgShotDataList();
	listEnemyShotData_ = new StgShotDataList();

	RenderShaderManager* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();
	effectLayer_ = shaderManager_->GetRender2DShader();
	handleEffectWorld_ = effectLayer_->GetParameterBySemantic(nullptr, "WORLD");
}
StgShotManager::~StgShotManager() {
	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = (*itr);
		if (obj != nullptr) {
			obj->ClearShotObject();
		}
	}

	ptr_delete(listPlayerShotData_);
	ptr_delete(listEnemyShotData_);
}
void StgShotManager::Work() {
	for (auto itr = listObj_.begin(); itr != listObj_.end(); ) {
		shared_ptr<StgShotObject> obj = (*itr);
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
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetCullingMode(D3DCULL_NONE);
	graphics->SetLightingEnable(false);
	graphics->SetTextureFilter(DirectGraphics::MODE_TEXTURE_FILTER_LINEAR, 0);

	//graphics->SetTextureFilter(DirectGraphics::MODE_TEXTURE_FILTER_POINT);
	//			MODE_TEXTURE_FILTER_POINT,//補間なし
	//			MODE_TEXTURE_FILTER_LINEAR,//線形補間
	//フォグを解除する
	DWORD bEnableFog = FALSE;
	device->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();

	D3DXMATRIX& matCamera = camera2D->GetMatrix();

	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		shared_ptr<StgShotObject> obj = (*itr);
		if (obj->IsDeleted() || !obj->IsActive() || obj->GetRenderPriorityI() != targetPriority) continue;
		obj->RenderOnShotManager();
	}

	{
		D3DXMATRIX matProj = matCamera * graphics->GetViewPortMatrix();
		effectLayer_->SetMatrix(handleEffectWorld_, &matProj);
	}

	//描画
	size_t countBlendType = StgShotDataList::RENDER_TYPE_COUNT;
	int blendMode[] =
	{
		DirectGraphics::MODE_BLEND_ADD_ARGB,
		DirectGraphics::MODE_BLEND_ADD_RGB,
		DirectGraphics::MODE_BLEND_SHADOW,
		DirectGraphics::MODE_BLEND_MULTIPLY,
		DirectGraphics::MODE_BLEND_SUBTRACT,
		DirectGraphics::MODE_BLEND_INV_DESTRGB,
		DirectGraphics::MODE_BLEND_ALPHA,
		DirectGraphics::MODE_BLEND_ALPHA_INV,
	};

	RenderShaderManager* shaderManager = ShaderManager::GetBase()->GetRenderLib();
	VertexBufferManager* bufferManager = VertexBufferManager::GetBase();

	device->SetFVF(VERTEX_TLX::fvf);
	device->SetVertexDeclaration(shaderManager->GetVertexDeclarationTLX());

	device->SetStreamSource(0, bufferManager->GetGrowableVertexBuffer()->GetBuffer(), 0, sizeof(VERTEX_TLX));
	device->SetIndices(bufferManager->GetGrowableIndexBuffer()->GetBuffer());

	{
		UINT cPass = 1U;

		//Always renders enemy shots above player shots, completely obliterates TAΣ's wet dream.
		for (size_t iBlend = 0; iBlend < countBlendType; ++iBlend) {
			bool hasPolygon = false;
			std::vector<StgShotRenderer*>& listPlayer = listPlayerShotData_->GetRendererList(blendMode[iBlend] - 1);

			//In an attempt to minimize D3D calls.
			for (auto itr = listPlayer.begin(); itr != listPlayer.end() && !hasPolygon; ++itr)
				hasPolygon = (*itr)->countRenderIndex_ >= 3U;
			if (!hasPolygon) continue;

			graphics->SetBlendMode(blendMode[iBlend]);
			effectLayer_->SetTechnique(blendMode[iBlend] == DirectGraphics::MODE_BLEND_ALPHA_INV ? "RenderInv" : "Render");

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
			effectLayer_->SetTechnique(blendMode[iBlend] == DirectGraphics::MODE_BLEND_ALPHA_INV ? "RenderInv" : "Render");

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

void StgShotManager::RegistIntersectionTarget() {
	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		StgShotObject* obj = itr->get();
		if (!obj->IsDeleted() && obj->IsActive()) {
			obj->ClearIntersectedIdList();
			obj->RegistIntersectionTarget();
		}
	}
}
void StgShotManager::AddShot(shared_ptr<StgShotObject> obj) {
	obj->SetOwnObjectReference();
	listObj_.push_back(obj);
}

RECT StgShotManager::GetShotAutoDeleteClipRect() {
	ref_count_ptr<StgStageInformation> stageInfo = stageController_->GetStageInformation();
	RECT* rcStgFrame = stageInfo->GetStgFrameRect();
	RECT* rcClip = stageInfo->GetShotAutoDeleteClip();

	RECT res = { rcClip->left, rcClip->top,
		rcClip->right + (rcStgFrame->right - rcStgFrame->left),
		rcClip->bottom + (rcStgFrame->bottom - rcStgFrame->top) };
	return res;
}

void StgShotManager::DeleteInCircle(int typeDelete, int typeTo, int typeOwner, float cx, float cy, float radius) {
	float rd = radius * radius;

	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		StgShotObject* obj = itr->get();

		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		if (typeDelete == DEL_TYPE_SHOT && (obj->GetLife() == StgShotObject::LIFE_SPELL_REGIST)) continue;

		float sx = cx - obj->GetPositionX();
		float sy = cy - obj->GetPositionY();

		float tr = sx * sx + sy * sy;
		if (tr <= rd) {
			if (typeTo == TO_TYPE_IMMEDIATE) {
				obj->DeleteImmediate();
			}
			else if (typeTo == TO_TYPE_FADE) {
				obj->SetFadeDelete();
			}
			else if (typeTo == TO_TYPE_ITEM) {
				obj->ConvertToItem(false);
			}
		}
	}
}

std::vector<int> StgShotManager::GetShotIdInCircle(int typeOwner, float cx, float cy, float radius) {
	float rd = radius * radius;

	std::vector<int> res;
	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		StgShotObject* obj = itr->get();

		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;

		float sx = cx - obj->GetPositionX();
		float sy = cy - obj->GetPositionY();

		float tr = sx * sx + sy * sy;
		if (tr <= rd) res.push_back(obj->GetObjectID());
	}

	return res;
}
size_t StgShotManager::GetShotCount(int typeOwner) {
	size_t res = 0;

	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		StgShotObject* obj = itr->get();
		if (obj->IsDeleted()) continue;
		if ((typeOwner != StgShotObject::OWNER_NULL) && (obj->GetOwnerType() != typeOwner)) continue;
		++res;
	}

	return res;
}

void StgShotManager::GetValidRenderPriorityList(std::vector<PriListBool>& list) {
	auto objectManager = stageController_->GetMainObjectManager();
	list.resize(objectManager->GetRenderBucketCapacity());
	ZeroMemory(&list[0], objectManager->GetRenderBucketCapacity() * sizeof(PriListBool));

	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		StgShotObject* obj = itr->get();
		if (obj->IsDeleted()) continue;
		int pri = obj->GetRenderPriorityI();
		list[pri] = true;
	}
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

/**********************************************************
//StgShotDataList
**********************************************************/
StgShotDataList::StgShotDataList() {
	listRenderer_.resize(RENDER_TYPE_COUNT);
	defaultDelayColor_ = 0xffffffff;	//Solid white
}
StgShotDataList::~StgShotDataList() {
	for (std::vector<StgShotRenderer*>& renderList : listRenderer_) {
		for (StgShotRenderer* renderer : renderList)
			ptr_delete(renderer);
		renderList.clear();
	}
	listRenderer_.clear();

	for (StgShotData* shotData : listData_)
		ptr_delete(shotData);
	listData_.clear();
}
bool StgShotDataList::AddShotDataList(std::wstring path, bool bReload) {
	if (!bReload && listReadPath_.find(path) != listReadPath_.end())return true;

	ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr) throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	if (!reader->Open())throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	std::string source = reader->ReadAllString();

	bool res = false;
	Scanner scanner(source);
	try {
		std::vector<StgShotData*> listData;
		std::wstring pathImage = L"";
		RECT rcDelay = { -1, -1, -1, -1 };
		RECT rcDelayDest = { -1, -1, -1, -1 };

		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::TK_EOF)//Eofの識別子が来たらファイルの調査終了
			{
				break;
			}
			else if (tok.GetType() == Token::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"ShotData") {
					_ScanShot(listData, scanner);
				}
				else if (element == L"shot_image") {
					scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
					pathImage = scanner.Next().GetString();
				}
				else if (element == L"delay_color") {
					std::vector<std::wstring> list = _GetArgumentList(scanner);
					defaultDelayColor_ = D3DCOLOR_ARGB(255,
						StringUtility::ToInteger(list[0]),
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]));
				}
				else if (element == L"delay_rect") {
					std::vector<std::wstring> list = _GetArgumentList(scanner);

					RECT rect;
					rect.left = StringUtility::ToInteger(list[0]);
					rect.top = StringUtility::ToInteger(list[1]);
					rect.right = StringUtility::ToInteger(list[2]);
					rect.bottom = StringUtility::ToInteger(list[3]);
					rcDelay = rect;

					int width = rect.right - rect.left;
					int height = rect.bottom - rect.top;
					RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
					if (width % 2 == 1) rcDest.right += 1;
					if (height % 2 == 1) rcDest.bottom += 1;
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

		shared_ptr<Texture> texture(new Texture());
		bool bTexture = texture->CreateFromFile(PathProperty::GetUnique(pathImage), false, false);
		if (!bTexture)throw gstd::wexception("The specified shot texture cannot be found.");

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
			if (data == nullptr)continue;

			data->indexTexture_ = textureIndex;
			{
				auto texture = listTexture_[data->indexTexture_];
				data->textureSize_.x = texture->GetWidth();
				data->textureSize_.y = texture->GetHeight();
			}

			if (data->rcDelay_.left < 0) {
				data->rcDelay_ = rcDelay;
				data->rcDstDelay_ = rcDelayDest;
			}
			listData_[iData] = data;
		}

		listReadPath_.insert(path);
		Logger::WriteTop(StringUtility::Format(L"Loaded shot data: %s", path.c_str()));
		res = true;
	}
	catch (gstd::wexception & e) {
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
	if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
	scanner.CheckType(tok, Token::TK_OPENC);

	StgShotData* data = new StgShotData(this);
	data->colorDelay_ = defaultDelayColor_;
	int id = -1;

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::TK_ID) {
			std::wstring element = tok.GetElement();

			if (element == L"id") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				id = scanner.Next().GetInteger();
			}
			else if (element == L"rect") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);

				StgShotData::AnimationData anime;

				RECT rect;
				rect.left = StringUtility::ToInteger(list[0]);
				rect.top = StringUtility::ToInteger(list[1]);
				rect.right = StringUtility::ToInteger(list[2]);
				rect.bottom = StringUtility::ToInteger(list[3]);
				anime.rcSrc_ = rect;

				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
				if (width % 2 == 1) rcDest.right += 1;
				if (height % 2 == 1) rcDest.bottom += 1;
				anime.rcDst_ = rcDest;

				data->listAnime_.resize(1);
				data->listAnime_[0] = anime;
				data->totalAnimeFrame_ = 1;
			}
			else if (element == L"delay_color") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				data->colorDelay_ = D3DCOLOR_ARGB(255,
					StringUtility::ToInteger(list[0]),
					StringUtility::ToInteger(list[1]),
					StringUtility::ToInteger(list[2]));
			}
			else if (element == L"delay_rect") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);

				RECT rect;
				rect.left = StringUtility::ToInteger(list[0]);
				rect.top = StringUtility::ToInteger(list[1]);
				rect.right = StringUtility::ToInteger(list[2]);
				rect.bottom = StringUtility::ToInteger(list[3]);
				data->rcDelay_ = rect;

				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
				if (width % 2 == 1) rcDest.right += 1;
				if (height % 2 == 1) rcDest.bottom += 1;
				data->rcDstDelay_ = rcDest;
			}
			else if (element == L"collision") {
				DxCircle circle;
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				if (list.size() == 1) {
					circle.SetR(StringUtility::ToInteger(list[0]));
				}
				else if (list.size() == 3) {
					circle.SetR(StringUtility::ToInteger(list[0]));
					circle.SetX(StringUtility::ToInteger(list[1]));
					circle.SetY(StringUtility::ToInteger(list[2]));
				}

				data->listCol_ = circle;
			}
			else if (element == L"render" || element == L"delay_render") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				std::wstring strRender = scanner.Next().GetElement();
				int typeRender = DirectGraphics::MODE_BLEND_ALPHA;

				if (strRender == L"ADD" || strRender == L"ADD_RGB")
					typeRender = DirectGraphics::MODE_BLEND_ADD_RGB;
				else if (strRender == L"ADD_ARGB")
					typeRender = DirectGraphics::MODE_BLEND_ADD_ARGB;
				else if (strRender == L"MULTIPLY")
					typeRender = DirectGraphics::MODE_BLEND_MULTIPLY;
				else if (strRender == L"SUBTRACT")
					typeRender = DirectGraphics::MODE_BLEND_SUBTRACT;
				else if (strRender == L"INV_DESTRGB")
					typeRender = DirectGraphics::MODE_BLEND_INV_DESTRGB;

				if (element.size() == 6 /*element == L"render"*/)data->typeRender_ = typeRender;
				else if (element.size() == 12 /*element == L"delay_render"*/)data->typeDelayRender_ = typeRender;
			}
			else if (element == L"alpha") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				data->alpha_ = scanner.Next().GetInteger();
			}
			else if (element == L"angular_velocity") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				tok = scanner.Next();
				if (tok.GetElement() == L"rand") {
					scanner.CheckType(scanner.Next(), Token::TK_OPENP);
					data->angularVelocityMin_ = Math::DegreeToRadian(scanner.Next().GetReal());
					scanner.CheckType(scanner.Next(), Token::TK_COMMA);
					data->angularVelocityMax_ = Math::DegreeToRadian(scanner.Next().GetReal());
					scanner.CheckType(scanner.Next(), Token::TK_CLOSEP);
				}
				else {
					data->angularVelocityMin_ = Math::DegreeToRadian(tok.GetReal());
					data->angularVelocityMax_ = data->angularVelocityMin_;
				}
			}
			else if (element == L"fixed_angle") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				tok = scanner.Next();
				data->bFixedAngle_ = tok.GetElement() == L"true";
			}
			else if (element == L"AnimationData") {
				_ScanAnimation(data, scanner);
			}
		}
	}

	if (id >= 0) {
		if (data->listCol_.GetR() <= 0) {
			int r = 0;
			if (data->listAnime_.size() > 0) {
				RECT& rect = data->listAnime_[0].rcSrc_;
				int rx = abs(rect.right - rect.left);
				int ry = abs(rect.bottom - rect.top);
				int r = std::min(rx, ry);
				r = r / 3 - 3;
			}
			DxCircle circle(0, 0, std::max(r, 2));
			data->listCol_ = circle;
		}
		if (listData.size() <= id)
			listData.resize(id + 1);

		listData[id] = data;
	}
}
void StgShotDataList::_ScanAnimation(StgShotData*& shotData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
	scanner.CheckType(tok, Token::TK_OPENC);

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::TK_ID) {
			std::wstring element = tok.GetElement();

			if (element == L"animation_data") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				if (list.size() == 5) {
					StgShotData::AnimationData anime;
					int frame = StringUtility::ToInteger(list[0]);
					RECT rcSrc = {
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]),
						StringUtility::ToInteger(list[3]),
						StringUtility::ToInteger(list[4]),
					};

					anime.frame_ = frame;
					anime.rcSrc_ = rcSrc;

					int width = rcSrc.right - rcSrc.left;
					int height = rcSrc.bottom - rcSrc.top;
					RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
					if (width % 2 == 1) rcDest.right += 1;
					if (height % 2 == 1) rcDest.bottom += 1;
					anime.rcDst_ = rcDest;

					shotData->listAnime_.push_back(anime);
					shotData->totalAnimeFrame_ += frame;
				}
			}
		}
	}
}
std::vector<std::wstring> StgShotDataList::_GetArgumentList(Scanner& scanner) {
	std::vector<std::wstring> res;
	scanner.CheckType(scanner.Next(), Token::TK_EQUAL);

	Token& tok = scanner.Next();

	if (tok.GetType() == Token::TK_OPENP) {
		while (true) {
			tok = scanner.Next();
			int type = tok.GetType();
			if (type == Token::TK_CLOSEP)break;
			else if (type != Token::TK_COMMA) {
				std::wstring str = tok.GetElement();
				res.push_back(str);
			}
		}
	}
	else {
		res.push_back(tok.GetElement());
	}
	return res;
}

//StgShotData
StgShotData::StgShotData(StgShotDataList* listShotData) {
	listShotData_ = listShotData;
	textureSize_ = D3DXVECTOR2(0, 0);
	typeRender_ = DirectGraphics::MODE_BLEND_ALPHA;
	typeDelayRender_ = DirectGraphics::MODE_BLEND_ADD_ARGB;
	colorDelay_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	SetRect(&rcDelay_, -1, -1, -1, -1);
	alpha_ = 255;
	totalAnimeFrame_ = 0;
	angularVelocityMin_ = 0;
	angularVelocityMax_ = 0;
	bFixedAngle_ = false;
}
StgShotData::~StgShotData() {}
StgShotRenderer* StgShotData::GetRenderer(int blendType) {
	if (blendType < DirectGraphics::MODE_BLEND_ALPHA || blendType > DirectGraphics::MODE_BLEND_ALPHA_INV)
		return listShotData_->GetRenderer(indexTexture_, 0);
	return listShotData_->GetRenderer(indexTexture_, blendType - 1);
}

StgShotData::AnimationData* StgShotData::GetData(int frame) {
	if (totalAnimeFrame_ == 1)
		return &listAnime_[0];

	frame = frame % totalAnimeFrame_;
	int total = 0;

	std::vector<AnimationData>::iterator itr = listAnime_.begin();
	for (; itr != listAnime_.end(); ++itr) {
		//AnimationData* anime = itr;
		total += itr->frame_;
		if (total >= frame)
			return &(*itr);
	}
	return &listAnime_[0];
}

/**********************************************************
//StgShotRenderer
**********************************************************/
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
	IDirect3DTexture9* pTexture = texture_[0] ? texture_[0]->GetD3DTexture() : nullptr;
	device->SetTexture(0, pTexture);
	//device->SetTexture(1, pTexture);

	VertexBufferManager* bufferManager = VertexBufferManager::GetBase();

	GrowableVertexBuffer* vBuffer = bufferManager->GetGrowableVertexBuffer();
	GrowableIndexBuffer* iBuffer = bufferManager->GetGrowableIndexBuffer();
	IDirect3DVertexBuffer9* pVBuffer = vBuffer->GetBuffer();
	IDirect3DIndexBuffer9* pIBuffer = iBuffer->GetBuffer();
	if (countMaxVertex_ > vBuffer->GetSize()) {
		vBuffer->Expand(countMaxVertex_);
		pVBuffer = vBuffer->GetBuffer();
		device->SetStreamSource(0, pVBuffer, 0, sizeof(VERTEX_TLX));
	}
	if (countMaxIndex_ > iBuffer->GetSize()) {
		iBuffer->Expand(countMaxIndex_);
		pIBuffer = iBuffer->GetBuffer();
		device->SetIndices(pIBuffer);
	}

	countRenderVertex_ = std::min(countRenderVertex_, vBuffer->GetSize());
	countRenderIndex_ = std::min(countRenderIndex_, iBuffer->GetSize());
	{
		void* tmp;
		pVBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
		memcpy(tmp, vertex_.data(), countRenderVertex_ * sizeof(VERTEX_TLX));
		pVBuffer->Unlock();
	}
	{
		void* tmp;
		pIBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
		memcpy(tmp, vecIndex_.data(), countRenderIndex_ * sizeof(uint32_t));
		pIBuffer->Unlock();
	}

	device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, countRenderVertex_, 0, countRenderIndex_ / 3U);

	countRenderVertex_ = 0U;
	countRenderIndex_ = 0U;
}
/*
void StgShotRenderer::AddSquareVertex(VERTEX_TLX* listVertex) {
	TryExpandVertex(countRenderVertex_ + 6U);

	VERTEX_TLX arrangedVerts[] = {
		listVertex[0], listVertex[2], listVertex[1],
		listVertex[1], listVertex[2], listVertex[3]
	};
	memcpy((VERTEX_TLX*)vertex_.data() + countRenderVertex_, arrangedVerts, strideVertexStreamZero_ * 6U);

	countRenderVertex_ += 6U;
}
*/
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

/**********************************************************
//StgShotObject
**********************************************************/
StgShotObject::StgShotObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;

	frameWork_ = 0;
	posX_ = 0;
	posY_ = 0;
	idShotData_ = 0;
	typeBlend_ = DirectGraphics::MODE_BLEND_NONE;

	damage_ = 1;
	life_ = LIFE_SPELL_UNREGIST;
	bAutoDelete_ = true;
	bEraseShot_ = false;
	bSpellFactor_ = false;

	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	frameGrazeInvalid_ = 0;
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
	if (listReserveShot_) listReserveShot_->Clear(stageController_);
}
void StgShotObject::SetOwnObjectReference() {
	auto ptr = std::dynamic_pointer_cast<StgShotObject>(stageController_->GetMainRenderObject(idObject_));
	pOwnReference_ = ptr;
}
void StgShotObject::Work() {
}
void StgShotObject::_Move() {
	if (delay_.time == 0 || bEnableMotionDelay_)
		StgMoveObject::_Move();
	SetX(posX_);
	SetY(posY_);

	//弾画像置き換え処理
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

	if (typeOwner_ == StgShotObject::OWNER_PLAYER) {
		auto scriptManager = stageController_->GetScriptManager();
		shared_ptr<ManagedScript> scriptPlayer = scriptManager->GetPlayerScript();

		float posX = GetPositionX();
		float posY = GetPositionY();
		if (scriptManager != nullptr && scriptPlayer != nullptr) {
			float listPos[2] = { posX, posY };

			value listScriptValue[3];
			listScriptValue[0] = scriptPlayer->CreateRealValue(idObject_);
			listScriptValue[1] = scriptPlayer->CreateRealArrayValue(listPos, 2U);
			listScriptValue[2] = scriptPlayer->CreateRealValue(GetShotDataID());
			scriptPlayer->RequestEvent(StgStagePlayerScript::EV_DELETE_SHOT_PLAYER, listScriptValue, 3);
		}
	}

	objectManager->DeleteObject(this);
}
void StgShotObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;

	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();
	if (posX_ < rect.left || posX_ > rect.right || posY_ < rect.top || posY_ > rect.bottom) {
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

	if (frameAutoDelete_ <= 0) {
		_SendDeleteEvent(StgShotManager::BIT_EV_DELETE_IMMEDIATE);
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
		return;
	}
	frameAutoDelete_ = std::max(0, frameAutoDelete_ - 1);
}
void StgShotObject::_SendDeleteEvent(int bit) {
	if (typeOwner_ != OWNER_ENEMY) return;

	auto stageScriptManager = stageController_->GetScriptManager();
	shared_ptr<ManagedScript> scriptShot = stageScriptManager->GetShotScript();
	if (scriptShot == nullptr) return;

	StgShotManager* shotManager = stageController_->GetShotManager();
	bool bSendEnable = shotManager->IsDeleteEventEnable(bit);
	if (!bSendEnable) return;

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

	gstd::value listScriptValue[4];
	listScriptValue[0] = scriptShot->CreateRealValue(idObject_);
	listScriptValue[1] = scriptShot->CreateRealArrayValue(listPos, 2U);
	listScriptValue[2] = scriptShot->CreateBooleanValue(false);
	listScriptValue[3] = scriptShot->CreateRealValue(GetShotDataID());
	scriptShot->RequestEvent(typeEvent, listScriptValue, 4);
}
void StgShotObject::_AddReservedShotWork() {
	if (IsDeleted() || listReserveShot_ == nullptr) return;

	ref_count_ptr<ReserveShotList::ListElement>::unsync listData = listReserveShot_->GetNextFrameData();
	if (listData == nullptr)return;

	auto objectManager = stageController_->GetMainObjectManager();
	std::list<ReserveShotListData>* list = listData->GetDataList();
	std::list<ReserveShotListData>::iterator itr = list->begin();
	for (; itr != list->end(); itr++) {
		StgShotObject::ReserveShotListData& data = (*itr);
		int idShot = data.GetShotID();
		shared_ptr<StgShotObject> obj = std::dynamic_pointer_cast<StgShotObject>(objectManager->GetObject(idShot));
		if (obj == nullptr || obj->IsDeleted())continue;

		_AddReservedShot(obj, &data);
	}
}

void StgShotObject::_AddReservedShot(shared_ptr<StgShotObject> obj, StgShotObject::ReserveShotListData* data) {
	auto objectManager = stageController_->GetMainObjectManager();

	float ox = GetPositionX();
	float oy = GetPositionY();

	float dRadius = data->GetRadius();
	float sx = obj->GetPositionX();
	float sy = obj->GetPositionY();
	double angle = GetDirectionAngle() + data->GetAngle();

	float tx = ox + sx + dRadius * cos(angle);
	float ty = oy + sy + dRadius * sin(angle);
	obj->SetX(tx);
	obj->SetY(ty);

	StgShotManager* shotManager = stageController_->GetShotManager();
	if (shotManager->GetShotCountAll() < StgShotManager::SHOT_MAX) {
		shotManager->AddShot(obj);
		obj->Activate();
		objectManager->ActivateObject(obj->GetObjectID(), true);
	}
	else
		objectManager->DeleteObject(obj);
}

void StgShotObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {

}
StgShotData* StgShotObject::_GetShotData(int id) {
	StgShotData* res = nullptr;
	StgShotManager* shotManager = stageController_->GetShotManager();
	StgShotDataList* dataList = (typeOwner_ == OWNER_PLAYER) ?
		shotManager->GetPlayerShotDataList() : shotManager->GetEnemyShotDataList();

	if (dataList) res = dataList->GetData(id);
	return res;
}

void StgShotObject::AddShot(int frame, int idShot, float radius, double angle) {
	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->ActivateObject(idShot, false);

	if (listReserveShot_ == nullptr)
		listReserveShot_ = new ReserveShotList();
	listReserveShot_->AddData(frame, idShot, radius, angle);
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
			timerTransform_ += std::max(transform.param_s[0], 0);
			break;
		case StgPatternShotTransform::TRANSFORM_ADD_SPEED_ANGLE:
		{
			{
				shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, transform.param_d[0]));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 
					Math::DegreeToRadian(transform.param_d[1])));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX,
					GetSpeed() + transform.param_d[0] * transform.param_s[0]));
				AddPattern(transform.param_s[1], pattern, true);
			}
			{
				shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, 0));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(transform.param_s[1] + transform.param_s[0], pattern, true);
			}	
			break;
		}
		case StgPatternShotTransform::TRANSFORM_ANGULAR_MOVE:
		{
			StgNormalShotObject* shot = (StgNormalShotObject*)this;
			if (shot) shot->angularVelocity_ = Math::DegreeToRadian(transform.param_d[1]);

			{
				shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 
					Math::DegreeToRadian(transform.param_d[0])));
				AddPattern(0, pattern, true);
			}
			{
				shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(transform.param_s[0], pattern, true);
			}

			break;
		}
		case StgPatternShotTransform::TRANSFORM_N_DECEL_CHANGE:
		{
			int timer = transform.param_s[0];
			int countRep = transform.param_s[1];

			timerTransform_ += timer * countRep;

			for (int framePattern = 0; countRep > 0; --countRep, framePattern += timer) {
				float nowSpeed = GetSpeed();

				{
					shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPEED, nowSpeed));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, -nowSpeed / timer));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, 0));
					AddPattern(framePattern, pattern, true);
				}

				{
					shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPEED, transform.param_d[0]));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, 0));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, 0));

					double angleArgument = Math::NormalizeAngleRad(Math::DegreeToRadian(transform.param_d[1]));
					switch (transform.param_s[2]) {
					case 0:
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ANGLE, angleArgument));
						break;
					case 1:
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::ADD_ANGLE, angleArgument));
						break;
					case 2:
					{
						shared_ptr<RandProvider> rand = stageController_->GetStageInformation()->GetRandProvider();
						pattern->SetRelativeObjectID(stageController_->GetPlayerObject());
						pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ANGLE, 
							rand->GetReal(-angleArgument, angleArgument)));
						break;
					}
					case 3:
					{
						pattern->SetRelativeObjectID(stageController_->GetPlayerObject());
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
			idShotData_ = transform.param_s[0];
			break;
		}
		case StgPatternShotTransform::TRANSFORM_BLEND_CHANGE:
		{
			typeBlend_ = transform.param_s[0];
			break;
		}
		case StgPatternShotTransform::TRANSFORM_TO_SPEED_ANGLE:
		{
			float nowSpeed = GetSpeed();
			float targetSpeed = transform.param_d[0];
			double nowAngle = GetDirectionAngle();
			double targetAngle = transform.param_d[1];

			if (targetAngle == StgMovePattern::TOPLAYER_CHANGE) {
				StgPlayerObject* objPlayer = stageController_->GetPlayerObjectPtr();
				if (objPlayer)
					targetAngle = atan2(objPlayer->GetY() - GetPositionY(), objPlayer->GetX() - GetPositionX());
			}
			else if (targetAngle == StgMovePattern::NO_CHANGE) {
				targetAngle = nowAngle;
			}
			else
				targetAngle = Math::DegreeToRadian(targetAngle);

			{
				shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				if (targetSpeed != StgMovePattern::NO_CHANGE) {
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL,
						(targetSpeed - nowSpeed) / transform.param_s[0]));
					pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, targetSpeed));
				}
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 
					Math::AngleDifferenceRad(nowAngle, targetAngle) / transform.param_s[0]));
				AddPattern(0, pattern, true);
			}
			{
				shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, 0));
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, 0));
				AddPattern(transform.param_s[0], pattern, true);
			}

			break;
		}
		case StgPatternShotTransform::TRANSFORM_ADDPATTERNA1:
		{
			float speed = transform.param_d[0];
			double angle = transform.param_d[1];

			shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
			pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ZERO, 0));
			if (speed != StgMovePattern::NO_CHANGE)
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPEED, speed));
			if (angle != StgMovePattern::NO_CHANGE)
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ANGLE, Math::DegreeToRadian(angle)));

			AddPattern(transform.param_s[0], pattern, true);
			break;
		}
		case StgPatternShotTransform::TRANSFORM_ADDPATTERNA2:
		{
			float accel = transform.param_d[0];
			double agvel = transform.param_d[1];
			float maxsp = transform.param_d[2];

			shared_ptr<StgMovePattern_Angle> pattern(new StgMovePattern_Angle(this));
			pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ZERO, 0));
			if (accel != StgMovePattern::NO_CHANGE)
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_ACCEL, accel));
			if (agvel != StgMovePattern::NO_CHANGE)
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_AGVEL, Math::DegreeToRadian(agvel)));
			if (maxsp != StgMovePattern::NO_CHANGE)
				pattern->AddCommand(std::make_pair(StgMovePattern_Angle::SET_SPMAX, maxsp));
			pattern->SetShotDataID(transform.param_s[1]);
			if (transform.param_s[2] != DxScript::ID_INVALID)
				pattern->SetRelativeObjectID(transform.param_s[2]);

			AddPattern(transform.param_s[0], pattern, true);
			break;
		}
		default:
			break;
		}

		listTransformationShotAct_.pop_front();
	}
}

//StgShotObject::ReserveShotList
ref_count_ptr<StgShotObject::ReserveShotList::ListElement>::unsync StgShotObject::ReserveShotList::GetNextFrameData() {
	ref_count_ptr<ListElement>::unsync res = nullptr;

	auto itr = mapData_.find(frame_);
	if (itr != mapData_.end()) {
		res = itr->second;
		mapData_.erase(itr);
	}

	++frame_;
	return res;
}
void StgShotObject::ReserveShotList::AddData(int frame, int idShot, float radius, double angle) {
	ref_count_ptr<ListElement>::unsync list;

	auto itr = mapData_.find(frame);
	if (itr == mapData_.end()) {
		list = new ListElement();
		mapData_[frame] = list;
	}
	else {
		list = itr->second;
	}

	ReserveShotListData data;
	data.idShot_ = idShot;
	data.radius_ = radius;
	data.angle_ = angle;
	list->Add(data);
}
void StgShotObject::ReserveShotList::Clear(StgStageController* stageController) {
	auto objectManager = stageController->GetMainObjectManager();
	if (objectManager == nullptr) return;

	auto itrMap = mapData_.begin();
	for (; itrMap != mapData_.end(); ++itrMap) {
		ref_count_ptr<ListElement>::unsync listElement = itrMap->second;
		std::list<ReserveShotListData>* list = listElement->GetDataList();
		std::list<ReserveShotListData>::iterator itr = list->begin();
		for (; itr != list->end(); ++itr) {
			StgShotObject::ReserveShotListData& data = (*itr);
			int idShot = data.GetShotID();
			shared_ptr<StgShotObject> objShot = std::dynamic_pointer_cast<StgShotObject>(objectManager->GetObject(idShot));
			if (objShot) objShot->ClearShotObject();
			objectManager->DeleteObject(objShot);
		}
	}
}

float StgShotObject::DelayParameter::_CalculateValue(D3DXVECTOR3* param, lerp_func func) {
	switch (type) {
	case DELAY_LERP:
		return func(param->x, param->y, time / param->z);
	case DELAY_DEFAULT:
	default:
		return std::min(param->x + time / param->z, param->y);
	}
}

/**********************************************************
//StgNormalShotObject
**********************************************************/
StgNormalShotObject::StgNormalShotObject(StgStageController* stageController) : StgShotObject(stageController) {
	typeObject_ = TypeObject::OBJ_SHOT;
	angularVelocity_ = 0;

	move_ = D3DXVECTOR2(1, 0);
	lastAngle_ = 0;

	pShotIntersectionTarget_ = std::make_shared<StgIntersectionTarget_Circle>();
}
StgNormalShotObject::~StgNormalShotObject() {

}
void StgNormalShotObject::Work() {
	if (!bEnableMovement_) return;

	_ProcessTransformAct();
	_Move();

	if (delay_.time > 0) --(delay_.time);
	else _AddReservedShotWork();

	++frameWork_;

	if (frameFadeDelete_ >= 0) --frameFadeDelete_;
	
	{
		angle_.z += angularVelocity_;

		double angleZ = angle_.z;
		if (StgShotData* shotData = _GetShotData()) {
			if (!shotData->IsFixedAngle()) angleZ += GetDirectionAngle() + Math::DegreeToRadian(90);
		}

		if (angleZ != lastAngle_) {
			move_ = D3DXVECTOR2(cosf(angleZ), sinf(angleZ));
			lastAngle_ = angleZ;
		}
	}

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
}

void StgNormalShotObject::_AddIntersectionRelativeTarget() {
	if (IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0) return;
	ClearIntersected();

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	std::vector<StgIntersectionTarget::ptr> listTarget = GetIntersectionTargetList();
	for (auto iTarget : listTarget) {
		intersectionManager->AddTarget(iTarget);
	}

	//RegistIntersectionRelativeTarget(intersectionManager);
}
std::vector<StgIntersectionTarget::ptr> StgNormalShotObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;

	if (IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0) return res;
	if (bUserIntersectionMode_ || !bIntersectionEnable_) return res;//ユーザ定義あたり判定モード

	if (pOwnReference_.expired()) return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return res;

	DxCircle circle = *shotData->GetIntersectionCircleList();
	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	if (StgIntersectionTarget_Circle* target = 
		dynamic_cast<StgIntersectionTarget_Circle*>(pShotIntersectionTarget_.get())) 
	{
		if (circle.GetX() != 0 || circle.GetY() != 0) {
			float px = circle.GetX() * move_.x + circle.GetY() * move_.y;
			float py = circle.GetX() * move_.y - circle.GetY() * move_.x;
			circle.SetX(px + posX_);
			circle.SetY(py + posY_);
		}
		else {
			circle.SetX(posX_);
			circle.SetY(posY_);
		}
		circle.SetR(circle.GetR() * ((hitboxScale_.x + hitboxScale_.y) / 2.0f));

		target->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(pOwnReference_);
		target->SetCircle(circle);

		res.push_back(pShotIntersectionTarget_);
	}

	return res;
}

void StgNormalShotObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	StgShotData* delayData = delay_.id >= 0 ? _GetShotData(delay_.id) : shotData;
	if (shotData == nullptr || delayData == nullptr) return;

	StgShotRenderer* renderer = nullptr;

	int shotBlendType = DirectGraphics::MODE_BLEND_ALPHA;
	if (delay_.time > 0) {
		int objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = delayData->GetRenderer(shotData->GetDelayRenderType());
		}
		else {
			renderer = delayData->GetRenderer(objDelayBlendType);
		}
	}
	else {
		int objBlendType = GetBlendType();
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
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

	RECT* rcSrc;
	RECT* rcDest;
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
		else{
			rcSrc = delayData->GetDelayRect();
			rcDest = delayData->GetDelayDest();
		}

		color = shotData->GetDelayColor();
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
	LONG* ptrDst = reinterpret_cast<LONG*>(rcDest);
	for (size_t iVert = 0U; iVert < 4U; ++iVert) {
		VERTEX_TLX vt;

		_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1] / textureSize->x, ptrSrc[iVert | 0b1] / textureSize->y);
		_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
		_SetVertexColorARGB(vt, color);

		float px = vt.position.x * scaleX;
		float py = vt.position.y * scaleY;
		vt.position.x = (px * move_.x - py * move_.y) + sposx;
		vt.position.y = (px * move_.y + py * move_.x) + sposy;
		vt.position.z = position_.z;

		//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
		verts[iVert] = vt;
	}

	renderer->AddSquareVertex(verts);
}

void StgNormalShotObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	shared_ptr<StgIntersectionObject> ptrObj = otherTarget->GetObject().lock();

	float damage = 0;
	int otherType = otherTarget->GetTargetType();
	switch (otherType) {
	case StgIntersectionTarget::TYPE_PLAYER:
	{
		//自機
		frameGrazeInvalid_ = INT_MAX;
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SHOT:
	{
		if (ptrObj) {
			StgShotObject* shot = (StgShotObject*)ptrObj.get();
			if (shot) {
				bool bEraseShot = shot->IsEraseShot();
				if (bEraseShot && life_ != LIFE_SPELL_REGIST)
					ConvertToItem(false);
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SPELL:
	{
		//自機スペル
		if (ptrObj) {
			StgPlayerSpellObject* spell = (StgPlayerSpellObject*)ptrObj.get();
			if (spell) {
				bool bEraseShot = spell->IsEraseShot();
				if (bEraseShot && life_ != LIFE_SPELL_REGIST)
					ConvertToItem(false);
			}
		}
		break;
	}
	case StgIntersectionTarget::TYPE_ENEMY:
	case StgIntersectionTarget::TYPE_ENEMY_SHOT:
	{
		damage = 1;
		break;
	}
	}

	if (life_ != LIFE_SPELL_REGIST)
		life_ = std::max(life_ - damage, 0.0);
}
void StgNormalShotObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();
	shared_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	float posX = GetPositionX();
	float posY = GetPositionY();
	if (scriptItem) {
		float listPos[2] = { posX, posY };

		gstd::value listScriptValue[4];
		listScriptValue[0] = scriptItem->CreateRealValue(idObject_);
		listScriptValue[1] = scriptItem->CreateRealArrayValue(listPos, 2U);
		listScriptValue[2] = scriptItem->CreateBooleanValue(flgPlayerCollision);
		listScriptValue[3] = scriptItem->CreateRealValue(GetShotDataID());
		scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
	}

	if (itemManager->IsDefaultBonusItemEnable() && !flgPlayerCollision) {
		if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
			shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
			auto objectManager = stageController_->GetMainObjectManager();
			int id = objectManager->AddObject(obj);
			if (id != DxScript::ID_INVALID) {
				//弾の座標にアイテムを作成する
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

	//角速度更新
	StgShotData* shotData = _GetShotData();
	if (shotData != nullptr && oldData != shotData) {
		if (angularVelocity_ != 0) {
			angularVelocity_ = 0;
			angle_.z = 0;
		}

		double avMin = shotData->GetAngularVelocityMin();
		double avMax = shotData->GetAngularVelocityMax();
		if (avMin != 0 || avMax != 0) {
			ref_count_ptr<StgStageInformation> stageInfo = stageController_->GetStageInformation();
			shared_ptr<RandProvider> rand = stageInfo->GetRandProvider();
			angularVelocity_ = rand->GetReal(avMin, avMax);
		}
	}
}

/**********************************************************
//StgLaserObject(レーザー基本部)
**********************************************************/
StgLaserObject::StgLaserObject(StgStageController* stageController) : StgShotObject(stageController) {
	life_ = LIFE_SPELL_REGIST;
	length_ = 0;
	widthRender_ = 0;
	widthIntersection_ = 0;
	invalidLengthStart_ = 0.1f;
	invalidLengthEnd_ = 0.1f;
	frameGrazeInvalidStart_ = 20;
	itemDistance_ = 24;

	delay_ = DelayParameter(0.5, 3.5, 15.0f);

	move_ = D3DXVECTOR2(1, 0);
	lastAngle_ = 0;
}
void StgLaserObject::_AddIntersectionRelativeTarget() {
	if (delay_.time > 0 || frameFadeDelete_ >= 0) return;
	ClearIntersected();

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	std::vector<StgIntersectionTarget::ptr> listTarget = GetIntersectionTargetList();
	for (auto iTarget : listTarget)
		intersectionManager->AddTarget(iTarget);
}
void StgLaserObject::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	shared_ptr<StgIntersectionObject> ptrObj = otherTarget->GetObject().lock();

	float damage = 0;
	int otherType = otherTarget->GetTargetType();
	switch (otherType) {
	case StgIntersectionTarget::TYPE_PLAYER:
	{
		//自機
		if (frameGrazeInvalid_ <= 0) {
			frameGrazeInvalid_ = frameGrazeInvalidStart_ > 0 ? frameGrazeInvalidStart_ : INT_MAX;
		}
		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SHOT:
	{
		//自機弾弾
		if (ptrObj) {
			StgShotObject* shot = (StgShotObject*)ptrObj.get();
			if (shot) {
				bool bEraseShot = shot->IsEraseShot();
				if (bEraseShot && life_ != LIFE_SPELL_REGIST) {
					damage = shot->GetDamage();
					ConvertToItem(false);
				}
			}
		}

		break;
	}
	case StgIntersectionTarget::TYPE_PLAYER_SPELL:
	{
		//自機スペル
		StgPlayerSpellObject* spell = (StgPlayerSpellObject*)ptrObj.get();
		if (spell) {
			bool bEraseShot = spell->IsEraseShot();
			if (bEraseShot && life_ != LIFE_SPELL_REGIST) {
				damage = spell->GetDamage();
				ConvertToItem(false);
			}
		}
		break;
	}
	}
	if (life_ != LIFE_SPELL_REGIST)
		life_ = std::max(life_ - damage, 0.0);
}


/**********************************************************
//StgLooseLaserObject(射出型レーザー)
**********************************************************/
StgLooseLaserObject::StgLooseLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::OBJ_LOOSE_LASER;

	pShotIntersectionTarget_ = std::make_shared<StgIntersectionTarget_Line>();
}
void StgLooseLaserObject::Work() {
	//1フレーム目は移動しない
	if (frameWork_ == 0) {
		posXE_ = posX_;
		posYE_ = posY_;
	}

	if (!bEnableMovement_) return;

	_ProcessTransformAct();
	_Move();

	if (delay_.time > 0) --(delay_.time);
	else _AddReservedShotWork();

	++frameWork_;

	if (frameFadeDelete_ >= 0) --frameFadeDelete_;

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
	//	_AddIntersectionRelativeTarget();
	--frameGrazeInvalid_;
}
void StgLooseLaserObject::_Move() {
	if (delay_.time == 0)
		StgMoveObject::_Move();
	DxScriptRenderObject::SetX(posX_);
	DxScriptRenderObject::SetY(posY_);

	if (delay_.time <= 0) {
		float dx = posXE_ - posX_;
		float dy = posYE_ - posY_;

		if ((dx * dx + dy * dy) > (length_ * length_)) {
			float speed = GetSpeed();
			posXE_ += speed * move_.x;
			posYE_ += speed * move_.y;
		}
	}
	if (lastAngle_ != GetDirectionAngle()) {
		lastAngle_ = GetDirectionAngle();
		move_ = D3DXVECTOR2(cosf(lastAngle_), sinf(lastAngle_));
	}
}
void StgLooseLaserObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();
	if ((posX_ < rect.left && posXE_ < rect.left) || (posX_ > rect.right&& posXE_ > rect.right) ||
		(posY_ < rect.top && posYE_ < rect.top) || (posY_ > rect.bottom&& posYE_ > rect.bottom)) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}

std::vector<StgIntersectionTarget::ptr> StgLooseLaserObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;

	if (IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0) return res;
	if (bUserIntersectionMode_ || !bIntersectionEnable_) return res;//ユーザ定義あたり判定モード

	if (pOwnReference_.expired()) return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return res;

	float dx = posXE_ - posX_;
	float dy = posYE_ - posY_;

	int posXS = posX_ + dx * invalidLengthStart_;
	int posYS = posY_ + dy * invalidLengthStart_;
	int posXE = posXE_ - dx * invalidLengthEnd_;
	int posYE = posYE_ - dy * invalidLengthEnd_;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	DxWidthLine line(posXS, posYS, posXE, posYE, widthIntersection_ * hitboxScale_.x);

	if (StgIntersectionTarget_Line* target =
		dynamic_cast<StgIntersectionTarget_Line*>(pShotIntersectionTarget_.get()))
	{
		target->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(pOwnReference_);
		target->SetLine(line);

		res.push_back(pShotIntersectionTarget_);
	}
	return res;
}

void StgLooseLaserObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	StgShotData* delayData = delay_.id >= 0 ? _GetShotData(delay_.id) : shotData;
	if (shotData == nullptr || delayData == nullptr) return;

	StgShotRenderer* renderer = nullptr;
	if (delay_.time > 0) {
		//遅延時間
		int objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = delayData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
		}
		else {
			renderer = delayData->GetRenderer(objDelayBlendType);
		}
	}
	else {
		int objBlendType = GetBlendType();
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
		}
		else {
			renderer = shotData->GetRenderer(objBlendType);
		}
	}

	if (renderer == nullptr) return;

	D3DXVECTOR2* textureSize = &shotData->GetTextureSize();

	float scaleX = 1.0f;
	float scaleY = 1.0f;
	D3DXVECTOR2 renderF = D3DXVECTOR2(1, 0);

	FLOAT sposx = position_.x;
	FLOAT sposy = position_.y;

	RECT* rcSrc;
	RECT rcDest;
	D3DCOLOR color;

	if (delay_.time > 0) {
		float expa = delay_.GetScale();
		scaleX = expa;
		scaleY = expa;

		renderF = move_;

		if (delay_.id >= 0) {
			textureSize = &delayData->GetTextureSize();

			StgShotData::AnimationData* anime = delayData->GetData(frameWork_);
			rcSrc = anime->GetSource();
			rcDest = *anime->GetDest();
		}
		else {
			rcSrc = delayData->GetDelayRect();
			rcDest = *delayData->GetDelayDest();
		}

		color = shotData->GetDelayColor();
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * delay_.GetAlpha());
			color = (color & 0x00ffffff) | (alpha << 24);
		}

		if (bRoundingPosition_) {
			sposx = roundf(sposx);
			sposy = roundf(sposy);
		}
	}
	else {
		scaleX = scale_.x;
		scaleY = scale_.y;

		float dx = posXE_ - posX_;
		float dy = posYE_ - posY_;
		float radius = sqrtf(dx * dx + dy * dy);

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
		SetRect(&rcDest, -widthRender_ / 2, 0, widthRender_ / 2, radius);
	}

	VERTEX_TLX verts[4];
	LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
	LONG* ptrDst = reinterpret_cast<LONG*>(&rcDest);
	for (size_t iVert = 0U; iVert < 4U; iVert++) {
		VERTEX_TLX vt;

		_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1] / textureSize->x, ptrSrc[iVert | 0b1] / textureSize->y);
		_SetVertexPosition(vt, ptrDst[iVert | 0b1], ptrDst[(iVert & 0b1) << 1]);
		_SetVertexColorARGB(vt, color);

		float px = vt.position.x * scaleX;
		float py = vt.position.y * scaleY;
		vt.position.x = (px * renderF.x - py * renderF.y) + sposx;
		vt.position.y = (px * renderF.y + py * renderF.x) + sposy;
		vt.position.z = position_.z;

		//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
		verts[iVert] = vt;
	}

	renderer->AddSquareVertex(verts);
}
void StgLooseLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();
	shared_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	int ex = GetPositionX();
	int ey = GetPositionY();

	float dx = posXE_ - posX_;
	float dy = posYE_ - posY_;
	float length = sqrtf(dx * dx + dy * dy);

	float listPos[2];
	gstd::value listScriptValue[4];

	for (float itemPos = 0; itemPos < length; itemPos += itemDistance_) {
		float posX = ex - itemPos * move_.x;
		float posY = ey - itemPos * move_.y;

		if (scriptItem) {
			listPos[0] = posX;
			listPos[1] = posY;

			listScriptValue[0] = scriptItem->CreateRealValue(idObject_);
			listScriptValue[1] = scriptItem->CreateRealArrayValue(listPos, 2U);
			listScriptValue[2] = scriptItem->CreateBooleanValue(flgPlayerCollision);
			listScriptValue[3] = scriptItem->CreateRealValue(GetShotDataID());
			scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
		}

		if (itemManager->IsDefaultBonusItemEnable() && delay_.time == 0 && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
				int id = stageController_->GetMainObjectManager()->AddObject(obj);
				if (id != DxScript::ID_INVALID) {
					//弾の座標にアイテムを作成する
					itemManager->AddItem(obj);
					obj->SetPositionX(posX);
					obj->SetPositionY(posY);
				}
			}
		}
	}
}

/**********************************************************
//StgStraightLaserObject(設置型レーザー)
**********************************************************/
StgStraightLaserObject::StgStraightLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::OBJ_STRAIGHT_LASER;

	angLaser_ = 0;
	frameFadeDelete_ = -1;

	bUseSouce_ = true;
	bUseEnd_ = false;
	idImageEnd_ = -1;

	scaleX_ = 0.05f;
	bLaserExpand_ = true;

	move_ = D3DXVECTOR2(1, 0);

	pShotIntersectionTarget_ = std::make_shared<StgIntersectionTarget_Line>();
}
void StgStraightLaserObject::Work() {
	if (!bEnableMovement_) return;

	_ProcessTransformAct();
	_Move();

	if (delay_.time > 0) --(delay_.time);
	else {
		_AddReservedShotWork();
		if (bLaserExpand_)
			scaleX_ = std::min(1.0f, scaleX_ + 0.1f);
	}

	++frameWork_;

	if (frameFadeDelete_ >= 0) --frameFadeDelete_;

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
	--frameGrazeInvalid_;

	if (lastAngle_ != angLaser_) {
		lastAngle_ = angLaser_;
		move_ = D3DXVECTOR2(cosf(angLaser_), sinf(angLaser_));
	}
}

void StgStraightLaserObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();

	int posXE = posX_ + (int)(length_ * move_.x);
	int posYE = posY_ + (int)(length_ * move_.y);

	if ((posX_ < rect.left && posXE < rect.left) || (posX_ > rect.right&& posXE > rect.right) ||
		(posY_ < rect.top && posYE < rect.top) || (posY_ > rect.bottom&& posYE > rect.bottom)) {
		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgStraightLaserObject::_DeleteInAutoDeleteFrame() {
	if (IsDeleted() || delay_.time > 0) return;

	if (frameAutoDelete_ <= 0)
		SetFadeDelete();
	else --frameAutoDelete_;
}
std::vector<StgIntersectionTarget::ptr> StgStraightLaserObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;

	if (IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0) return res;
	if (bUserIntersectionMode_ || !bIntersectionEnable_) return res;//ユーザ定義あたり判定モード

	if (pOwnReference_.expired()) return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return res;
	if (scaleX_ < 1.0 && typeOwner_ != OWNER_PLAYER) return res;

	float _posXE = posX_ + (length_ * move_.x) * hitboxScale_.y;
	float _posYE = posY_ + (length_ * move_.y) * hitboxScale_.y;
	float posXS = posX_ + (_posXE - posX_) * invalidLengthStart_;
	float posYS = posY_ + (_posYE - posY_) * invalidLengthStart_;
	float posXE = _posXE + (posX_ - _posXE) * invalidLengthEnd_;
	float posYE = _posYE + (posY_ - _posYE) * invalidLengthEnd_;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();
	DxWidthLine line(posXS, posYS, posXE, posYE, widthIntersection_ * hitboxScale_.x);

	if (StgIntersectionTarget_Line* target =
		dynamic_cast<StgIntersectionTarget_Line*>(pShotIntersectionTarget_.get())) 
	{
		target->SetTargetType(typeOwner_ == OWNER_PLAYER ?
			StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
		target->SetObject(pOwnReference_);
		target->SetLine(line);

		res.push_back(pShotIntersectionTarget_);
	}
	return res;
}
void StgStraightLaserObject::_AddReservedShot(shared_ptr<StgShotObject> obj, StgShotObject::ReserveShotListData* data) {
	auto objectManager = stageController_->GetMainObjectManager();

	double ownAngle = GetDirectionAngle();
	float ox = GetPositionX();
	float oy = GetPositionY();

	float dRadius = data->GetRadius();
	float sx = obj->GetPositionX();
	float sy = obj->GetPositionY();
	double objAngle = obj->GetDirectionAngle();
	double angle = angLaser_ + data->GetAngle();

	float tx = ox + sx + dRadius * cosf(angle);
	float ty = oy + sy + dRadius * sinf(angle);
	obj->SetPositionX(tx);
	obj->SetPositionY(ty);
	obj->SetDirectionAngle(angle + obj->GetDirectionAngle());

	StgShotManager* shotManager = stageController_->GetShotManager();
	shotManager->AddShot(obj);
	obj->Activate();
	objectManager->ActivateObject(obj->GetObjectID(), true);
}
void StgStraightLaserObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return;

	D3DXVECTOR2* textureSize = &shotData->GetTextureSize();

	FLOAT sposx = position_.x;
	FLOAT sposy = position_.y;

	RECT* rcSrc;
	D3DCOLOR color;

	int objBlendType = GetBlendType();
	int shotBlendType = objBlendType;
	{
		StgShotRenderer* renderer = nullptr;
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
			shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
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
			D3DXVECTOR4 rcDest(-_rWidth, length_, _rWidth, 0);

			VERTEX_TLX verts[4];
			LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
			FLOAT* ptrDst = reinterpret_cast<FLOAT*>(&rcDest);
			for (size_t iVert = 0U; iVert < 4U; ++iVert) {
				VERTEX_TLX vt;

				_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1] / textureSize->x, ptrSrc[iVert | 0b1] / textureSize->y);
				_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
				_SetVertexColorARGB(vt, color);

				float px = vt.position.x * scale_.x;
				float py = vt.position.y * scale_.y;

				vt.position.x = (px * move_.y + py * move_.x) + sposx;
				vt.position.y = (-px * move_.x + py * move_.y) + sposy;
				vt.position.z = position_.z;

				//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
				verts[iVert] = vt;
			}

			renderer->AddSquareVertex(verts);
		}
	}

	{
		int objSourceBlendType = GetSourceBlendType();

		if ((bUseSouce_ || bUseEnd_) && (frameFadeDelete_ < 0)) {	//Delay cloud(s)
			color = shotData->GetDelayColor();

			int sourceWidth = widthRender_ * 2 / 3;
			D3DXVECTOR4 rcDest(-sourceWidth, -sourceWidth, sourceWidth, sourceWidth);

			auto _AddDelay = [&](StgShotData* delayShotData, RECT* delayRect, D3DXVECTOR2& delayPos) {
				StgShotRenderer* renderer = nullptr;

				if (objSourceBlendType == DirectGraphics::MODE_BLEND_NONE)
					renderer = delayShotData->GetRenderer(shotBlendType);
				else
					renderer = delayShotData->GetRenderer(objSourceBlendType);
				if (renderer == nullptr) return;

				VERTEX_TLX verts[4];
				LONG* ptrSrc = reinterpret_cast<LONG*>(delayRect);
				FLOAT* ptrDst = reinterpret_cast<FLOAT*>(&rcDest);
				for (size_t iVert = 0U; iVert < 4U; ++iVert) {
					VERTEX_TLX vt;

					_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1] / delayShotData->GetTextureSize().x, 
						ptrSrc[iVert | 0b1] / delayShotData->GetTextureSize().y);
					_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
					_SetVertexColorARGB(vt, color);

					float px = vt.position.x;
					float py = vt.position.y;
					vt.position.x = (px * move_.y + py * move_.x) + delayPos.x;
					vt.position.y = (-px * move_.x + py * move_.y) + delayPos.y;
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
				RECT* delayRect = nullptr;

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

				_AddDelay(delayData, delayRect, delayPos);
			}
			if (bUseEnd_) {
				D3DXVECTOR2 delayPos = D3DXVECTOR2(sposx + length_ * cosf(angLaser_), sposy + length_ * sinf(angLaser_));
				if (bRoundingPosition_) {
					delayPos.x = roundf(delayPos.x);
					delayPos.y = roundf(delayPos.y);
				}

				StgShotData* delayData = nullptr;
				RECT* delayRect = nullptr;

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

				_AddDelay(delayData, delayRect, delayPos);
			}
		}
	}
}
void StgStraightLaserObject::_ConvertToItemAndSendEvent(bool flgPlayerCollision) {
	StgItemManager* itemManager = stageController_->GetItemManager();
	auto stageScriptManager = stageController_->GetScriptManager();
	shared_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	float ex = posX_;
	float ey = posY_;

	float listPos[2];
	gstd::value listScriptValue[4];

	for (float itemPos = 0; itemPos < (float)length_; itemPos += itemDistance_) {
		float posX = ex + itemPos * move_.x;
		float posY = ey + itemPos * move_.y;

		if (scriptItem) {
			listPos[0] = posX;
			listPos[1] = posY;

			listScriptValue[0] = scriptItem->CreateRealValue(idObject_);
			listScriptValue[1] = scriptItem->CreateRealArrayValue(listPos, 2U);
			listScriptValue[2] = scriptItem->CreateBooleanValue(flgPlayerCollision);
			listScriptValue[3] = scriptItem->CreateRealValue(GetShotDataID());
			scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
		}

		if (itemManager->IsDefaultBonusItemEnable() && delay_.time == 0 && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
				int id = stageController_->GetMainObjectManager()->AddObject(obj);
				if (id != DxScript::ID_INVALID) {
					//弾の座標にアイテムを作成する
					itemManager->AddItem(obj);
					obj->SetPositionX(posX);
					obj->SetPositionY(posY);
				}
			}
		}
	}
}

/**********************************************************
//StgCurveLaserObject(曲がる型レーザー)
**********************************************************/
StgCurveLaserObject::StgCurveLaserObject(StgStageController* stageController) : StgLaserObject(stageController) {
	typeObject_ = TypeObject::OBJ_CURVE_LASER;
	tipDecrement_ = 0.0;

	invalidLengthStart_ = 0.0f;
	invalidLengthEnd_ = 0.0f;

	itemDistance_ = 6.0;

	pShotIntersectionTarget_ = nullptr;
}
void StgCurveLaserObject::Work() {
	if (!bEnableMovement_) return;

	_ProcessTransformAct();
	_Move();

	if (delay_.time > 0) --(delay_.time);
	else _AddReservedShotWork();

	++frameWork_;

	if (frameFadeDelete_ >= 0) --frameFadeDelete_;

	_DeleteInAutoClip();
	_DeleteInLife();
	_DeleteInFadeDelete();
	_DeleteInAutoDeleteFrame();
	//	_AddIntersectionRelativeTarget();
	--frameGrazeInvalid_;
}
void StgCurveLaserObject::_Move() {
	StgMoveObject::_Move();
	DxScriptRenderObject::SetX(posX_);
	DxScriptRenderObject::SetY(posY_);

	{
		if (lastAngle_ != GetDirectionAngle()) {
			lastAngle_ = GetDirectionAngle();
			move_ = D3DXVECTOR2(cosf(lastAngle_), sinf(lastAngle_));
		}

		LaserNode pos;
		pos.pos.SetX(posX_);
		pos.pos.SetY(posY_);
		{
			float wRender = widthRender_ / 2.0f;

			float nx = -wRender * move_.y;
			float ny = wRender * move_.x;

			pos.vertOff[0] = { nx, ny };
			pos.vertOff[1] = { -nx, -ny };
		}

		listPosition_.push_front(pos);
		if (listPosition_.size() > length_)
			listPosition_.pop_back();
	}
}
void StgCurveLaserObject::_DeleteInAutoClip() {
	if (IsDeleted() || !IsAutoDelete()) return;
	StgShotManager* shotManager = stageController_->GetShotManager();
	RECT rect = shotManager->GetShotAutoDeleteClipRect();

	//Checks if the node is within the bounding rect
	auto PredicateNodeInRect = [&](LaserNode& node) {
		DxPoint* pos = &node.pos;
		bool bInX = pos->GetX() >= rect.left && pos->GetX() <= rect.right;
		bool bInY = pos->GetY() >= rect.top && pos->GetY() <= rect.bottom;
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
std::vector<StgIntersectionTarget::ptr> StgCurveLaserObject::GetIntersectionTargetList() {
	std::vector<StgIntersectionTarget::ptr> res;

	if (IsDeleted() || delay_.time > 0 || frameFadeDelete_ >= 0) return res;
	if (bUserIntersectionMode_ || !bIntersectionEnable_) return res;//ユーザ定義あたり判定モード

	if (pOwnReference_.expired()) return res;

	StgShotData* shotData = _GetShotData();
	if (shotData == nullptr) return res;

	StgIntersectionManager* intersectionManager = stageController_->GetIntersectionManager();

	size_t countPos = listPosition_.size();
	std::list<LaserNode>::iterator itr = listPosition_.begin();

	float iLengthS = invalidLengthStart_ * 0.5f;
	float iLengthE = 0.5f + (1.0f - invalidLengthStart_) * 0.5f;
	int posInvalidS = (int)(countPos * iLengthS);
	int posInvalidE = (int)(countPos * iLengthE);
	double iWidth = widthIntersection_ * hitboxScale_.x;

	for (size_t iPos = 0; iPos < countPos - 1; ++iPos) {
		if ((int)iPos < posInvalidS || (int)iPos > posInvalidE) {
			++itr;
			continue;
		}

		std::list<LaserNode>::iterator itrNext = std::next(itr);

		double posXS = (*itr).pos.GetX();
		double posYS = (*itr).pos.GetY();
		double posXE = (*itrNext).pos.GetX();
		double posYE = (*itrNext).pos.GetY();
		++itr;

		DxWidthLine line(posXS, posYS, posXE, posYE, iWidth);
		shared_ptr<StgIntersectionTarget_Line> target(new StgIntersectionTarget_Line);
		if (target) {
			target->SetTargetType(typeOwner_ == OWNER_PLAYER ? 
				StgIntersectionTarget::TYPE_PLAYER_SHOT : StgIntersectionTarget::TYPE_ENEMY_SHOT);
			target->SetObject(pOwnReference_);
			target->SetLine(line);

			res.push_back(target);
		}
	}
	return res;
}

void StgCurveLaserObject::RenderOnShotManager() {
	if (!IsVisible()) return;

	StgShotData* shotData = _GetShotData();
	StgShotData* delayData = delay_.id >= 0 ? _GetShotData(delay_.id) : shotData;
	if (shotData == nullptr) return;

	int shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
	StgShotRenderer* renderer = nullptr;

	if (delayData != nullptr && delay_.time > 0) {
		int objDelayBlendType = GetSourceBlendType();
		if (objDelayBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
			shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
		}
		else {
			renderer = shotData->GetRenderer(objDelayBlendType);
		}
		if (renderer == nullptr) return;

		RECT* rcSrc;
		RECT* rcDest;
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

		FLOAT sX = listPosition_.back().pos.GetX();
		FLOAT sY = listPosition_.back().pos.GetY();
		if (bRoundingPosition_) {
			sX = roundf(sX);
			sY = roundf(sY);
		}

		D3DCOLOR color = shotData->GetDelayColor();
		{
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * delay_.GetAlpha());
			color = (color & 0x00ffffff) | (alpha << 24);
		}

		VERTEX_TLX verts[4];
		LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
		LONG* ptrDst = reinterpret_cast<LONG*>(rcDest);
		for (size_t iVert = 0U; iVert < 4U; iVert++) {
			VERTEX_TLX vt;

			_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1] / delaySize->x, ptrSrc[iVert | 0b1] / delaySize->y);
			_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
			_SetVertexColorARGB(vt, color);

			float px = vt.position.x * expa;
			float py = vt.position.y * expa;
			vt.position.x = (px * move_.x - py * move_.y) + sX;
			vt.position.y = (px * move_.y + py * move_.x) + sY;
			vt.position.z = position_.z;

			//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
			verts[iVert] = vt;
		}

		renderer->AddSquareVertex(verts);
	}
	if (listPosition_.size() > 1U) {
		int objBlendType = GetBlendType();
		int shotBlendType = objBlendType;
		if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
			renderer = shotData->GetRenderer(DirectGraphics::MODE_BLEND_ADD_ARGB);
			shotBlendType = DirectGraphics::MODE_BLEND_ADD_ARGB;
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

		RECT* rcSrcOrg = anime->GetSource();
		float rcInc = ((rcSrcOrg->bottom - rcSrcOrg->top) / (float)countRect) / textureSize->y;
		float rectV = (rcSrcOrg->top) / textureSize->y;

		LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrcOrg);

		std::list<LaserNode>::iterator itr = listPosition_.begin();
		size_t iPos = 0U;
		for (std::list<LaserNode>::iterator itr = listPosition_.begin(); itr != listPosition_.end(); ++itr, ++iPos) {
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

			VERTEX_TLX verts[2];
			for (size_t iVert = 0U; iVert < 2U; ++iVert) {
				VERTEX_TLX vt;

				_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1] / textureSize->x, rectV);
				_SetVertexPosition(vt, itr->pos.GetX() + itr->vertOff[iVert].GetX(),
					itr->pos.GetY() + itr->vertOff[iVert].GetY(), position_.z);
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
	shared_ptr<ManagedScript> scriptItem = stageScriptManager->GetItemScript();

	//assert(scriptItem != nullptr);

	gstd::value listScriptValue[4];
	if (scriptItem) {
		listScriptValue[0] = scriptItem->CreateRealValue(idObject_);
		listScriptValue[2] = scriptItem->CreateBooleanValue(flgPlayerCollision);
		listScriptValue[3] = scriptItem->CreateRealValue(GetShotDataID());
	}

	size_t countToItem = 0U;

	auto RequestItem = [&](float ix, float iy) {
		if (scriptItem) {
			float listPos[2] = { ix, iy };
			listScriptValue[1] = scriptItem->CreateRealArrayValue(listPos, 2U);
			scriptItem->RequestEvent(StgStageScript::EV_DELETE_SHOT_TO_ITEM, listScriptValue, 4);
		}
		if (itemManager->IsDefaultBonusItemEnable() && delay_.time == 0 && !flgPlayerCollision) {
			if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
				shared_ptr<StgItemObject> obj = shared_ptr<StgItemObject>(new StgItemObject_Bonus(stageController_));
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

		DxPoint& pos = (*itr).pos;
		DxPoint& posNext = (*itrNext).pos;
		float nodeDist = hypotf(posNext.GetX() - pos.GetX(), posNext.GetY() - pos.GetY());
		lengthAcc += nodeDist;

		float createDist = 0U;
		while (lengthAcc >= itemDistance_) {
			float lerpMul = (itemDistance_ >= nodeDist) ? 0.0 : (createDist / nodeDist);
			RequestItem(Math::Lerp::Linear(pos.GetX(), posNext.GetX(), lerpMul), 
				Math::Lerp::Linear(pos.GetY(), posNext.GetY(), lerpMul));
			createDist += itemDistance_;
			lengthAcc -= itemDistance_;
		}
	}
}


/**********************************************************
//StgPatternShotObjectGenerator (ECL-style bullets firing) [Under construction]
**********************************************************/
StgPatternShotObjectGenerator::StgPatternShotObjectGenerator() {
	parent_ = nullptr;
	idShotData_ = -1;
	typeOwner_ = StgShotObject::OWNER_ENEMY;
	typePattern_ = PATTERN_TYPE_FAN;
	typeShot_ = TypeObject::OBJ_SHOT;
	iniBlendType_ = DirectGraphics::MODE_BLEND_NONE;

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
StgPatternShotObjectGenerator::~StgPatternShotObjectGenerator() {
	parent_ = nullptr;
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
	StgPlayerObject* objPlayer = controller->GetPlayerObjectPtr();
	StgStageScriptObjectManager* objManager = controller->GetMainObjectManager();
	StgShotManager* shotManager = controller->GetShotManager();
	shared_ptr<RandProvider> randGenerator = controller->GetStageInformation()->GetRandProvider();

	if (idShotData_ < 0) return;
	if (shotWay_ == 0U || shotStack_ == 0U) return;

	float basePosX = (parent_ != nullptr && basePointX_ == BASEPOINT_RESET) ? parent_->GetPositionX() : basePointX_;
	float basePosY = (parent_ != nullptr && basePointY_ == BASEPOINT_RESET) ? parent_->GetPositionY() : basePointY_;
	basePosX += basePointOffsetX_;
	basePosY += basePointOffsetY_;

	std::list<StgPatternShotTransform> transformAsList;
	for (StgPatternShotTransform& iTransform : listTransformation_)
		transformAsList.push_back(iTransform);

	auto __CreateShot = [&](float _x, float _y, float _ss, float _sa) -> bool {
		if (shotManager->GetShotCountAll() >= StgShotManager::SHOT_MAX) return false;

		shared_ptr<StgShotObject> objShot = nullptr;
		switch (typeShot_) {
		case TypeObject::OBJ_SHOT:
		{
			shared_ptr<StgNormalShotObject> ptrShot = std::make_shared<StgNormalShotObject>(controller);
			objShot = ptrShot;
			break;
		}
		case TypeObject::OBJ_LOOSE_LASER:
		{
			shared_ptr<StgLooseLaserObject> ptrShot = std::make_shared<StgLooseLaserObject>(controller);
			ptrShot->SetLength(laserLength_);
			ptrShot->SetRenderWidth(laserWidth_);
			objShot = ptrShot;
			break;
		}
		case TypeObject::OBJ_CURVE_LASER:
		{
			shared_ptr<StgCurveLaserObject> ptrShot = std::make_shared<StgCurveLaserObject>(controller);
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
					float ss = speedBase_  + (speedArgument_ - speedBase_) *
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
		}
	}
}