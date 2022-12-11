#include "source/GcLib/pch.h"

#include "StgItem.hpp"
#include "StgSystem.hpp"
#include "StgStageScript.hpp"
#include "StgPlayer.hpp"

//*******************************************************************
//StgItemManager
//*******************************************************************
StgItemManager::StgItemManager(StgStageController* stageController) {
	stageController_ = stageController;

	listItemData_ = std::make_unique<StgItemDataList>();

	const std::wstring& dir = EPathProperty::GetSystemImageDirectory();

	{
		listSpriteItem_.reset(new SpriteList2D());

		std::wstring pathItem = PathProperty::GetUnique(dir + L"System_Stg_Item.png");
		shared_ptr<Texture> textureItem(new Texture());
		textureItem->CreateFromFile(pathItem, false, false);
		listSpriteItem_->SetTexture(textureItem);
	}
	{
		listSpriteDigit_.reset(new SpriteList2D());

		std::wstring pathDigit = PathProperty::GetUnique(dir + L"System_Stg_Digit.png");
		shared_ptr<Texture> textureDigit(new Texture());
		textureDigit->CreateFromFile(pathDigit, false, false);
		listSpriteDigit_->SetTexture(textureDigit);
	}

	rcDeleteClip_ = DxRect<LONG>(-64, 0, 64, 64);

	filterMin_ = D3DTEXF_LINEAR;
	filterMag_ = D3DTEXF_LINEAR;

	bCancelToPlayer_ = false;
	bAllItemToPlayer_ = false;
	bDefaultBonusItemEnable_ = true;

	{
		RenderShaderLibrary* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();
		effectItem_ = shaderManager_->GetRender2DShader();
	}
	{
		size_t renderPriMax = stageController_->GetMainObjectManager()->GetRenderBucketCapacity();

		listRenderQueue_.resize(renderPriMax);
		for (size_t i = 0; i < renderPriMax; ++i) {
			listRenderQueue_[i].listItem.resize(32);
		}
	}
	pLastTexture_ = nullptr;
}
StgItemManager::~StgItemManager() {
}
void StgItemManager::Work() {
	ref_unsync_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
	if (objPlayer == nullptr) return;

	float px = objPlayer->GetX();
	float py = objPlayer->GetY();
	int pr = objPlayer->GetItemIntersectionRadius() * objPlayer->GetItemIntersectionRadius();
	int pAutoItemCollectY = objPlayer->GetAutoItemCollectY();

	for (auto itr = listObj_.begin(); itr != listObj_.end();) {
		ref_unsync_ptr<StgItemObject>& obj = *itr;

		if (obj->IsDeleted()) {
			//obj->Clear();
			itr = listObj_.erase(itr);
		}
		else {
			float ix = obj->GetPositionX();
			float iy = obj->GetPositionY();

			if (objPlayer->GetState() != StgPlayerObject::STATE_NORMAL) {
				if (obj->IsMoveToPlayer()) {
					obj->SetMoveToPlayer(false);
					obj->NotifyItemCancelEvent(StgItemObject::CANCEL_PLAYER_DOWN);
				}
			}
			else {
				float dx = px - ix;
				float dy = py - iy;
				int radius = dx * dx + dy * dy;

				int typeCollect = StgItemObject::COLLECT_PLAYER_SCOPE;
				uint64_t collectParam = 0;

				if (obj->bIntersectEnable_ && radius <= obj->itemIntersectRadius_) {
					obj->Intersect(nullptr, nullptr);
					goto lab_next_item;
				}

				int moveToPlayerFlags = obj->GetMoveToPlayerEnableFlags();

				if (bCancelToPlayer_ && obj->IsMoveToPlayer()) {
					obj->SetMoveToPlayer(false);
					obj->NotifyItemCancelEvent(StgItemObject::CANCEL_ALL);
				}
				else if (moveToPlayerFlags != 0 && !obj->IsMoveToPlayer()) {
					//Player item scope collection
					if ((radius <= pr) && (moveToPlayerFlags & StgItemObject::FLAG_MOVETOPL_PLAYER_SCOPE)) {
						typeCollect = StgItemObject::COLLECT_PLAYER_SCOPE;
						collectParam = (uint64_t)objPlayer->GetItemIntersectionRadius();
						goto lab_move_to_player;
					}

					//CollectAllItems collection
					if (bAllItemToPlayer_ && (moveToPlayerFlags & StgItemObject::FLAG_MOVETOPL_COLLECT_ALL)) {
						typeCollect = StgItemObject::COLLECT_ALL;
						goto lab_move_to_player;
					}

					//POC collection
					if ((pAutoItemCollectY >= 0) && (moveToPlayerFlags & StgItemObject::FLAG_MOVETOPL_POC_LINE)) {
						if (!obj->IsMoveToPlayer() && py <= pAutoItemCollectY) {
							typeCollect = StgItemObject::COLLECT_PLAYER_LINE;
							collectParam = (uint64_t)pAutoItemCollectY;
							goto lab_move_to_player;
						}
					}

					//CollectItemsInCircle collection
					if (moveToPlayerFlags & StgItemObject::FLAG_MOVETOPL_COLLECT_CIRCLE) {
						for (DxCircle& circle : listCircleToPlayer_) {
							float rr = circle.GetR() * circle.GetR();
							if (Math::HypotSq(ix - circle.GetX(), iy - circle.GetY()) <= rr) {
								typeCollect = StgItemObject::COLLECT_IN_CIRCLE;
								collectParam = (uint64_t)circle.GetR();
								goto lab_move_to_player;
							}
						}
					}

					goto lab_next_item;
lab_move_to_player:
					obj->SetMoveToPlayer(true);
					obj->NotifyItemCollectEvent(typeCollect, collectParam);
				}
			}

lab_next_item:
			++itr;
		}
	}

	listCircleToPlayer_.clear();

	bAllItemToPlayer_ = false;
	bCancelToPlayer_ = false;
}

std::array<BlendMode, StgItemManager::BLEND_COUNT> StgItemManager::blendTypeRenderOrder = {
	MODE_BLEND_ADD_ARGB,
	MODE_BLEND_ADD_RGB,
	MODE_BLEND_SHADOW,
	MODE_BLEND_MULTIPLY,
	MODE_BLEND_SUBTRACT,
	MODE_BLEND_INV_DESTRGB,
	MODE_BLEND_ALPHA,
	MODE_BLEND_ALPHA_INV,
};
void StgItemManager::Render(int targetPriority) {
	if (targetPriority < 0 || targetPriority >= listRenderQueue_.size()) return;

	const RenderQueue& renderQueue = listRenderQueue_[targetPriority];
	if (renderQueue.count == 0) return;

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

	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();

	D3DXMatrixMultiply(&matProj_, &camera2D->GetMatrix(), &graphics->GetViewPortMatrix());

	//Render default items and score texts
	{
		for (size_t i = 0; i < renderQueue.count; ++i) {
			StgItemObject* pItem = renderQueue.listItem[i];
			pItem->RenderOnItemManager();
		}

		graphics->SetBlendMode(MODE_BLEND_ADD_ARGB);
		listSpriteDigit_->Render();
		listSpriteDigit_->ClearVertexCount();

		graphics->SetBlendMode(MODE_BLEND_ALPHA);
		listSpriteItem_->Render();
		listSpriteItem_->ClearVertexCount();
	}

	device->SetFVF(VERTEX_TLX::fvf);
	device->SetVertexDeclaration(shaderManager->GetVertexDeclarationTLX());
	pLastTexture_ = nullptr;

	if (D3DXHANDLE handle = effectItem_->GetParameterBySemantic(nullptr, "VIEWPROJECTION")) {
		effectItem_->SetMatrix(handle, &matProj_);
	}

	for (size_t iBlend = 0; iBlend < blendTypeRenderOrder.size(); ++iBlend) {
		BlendMode blend = blendTypeRenderOrder[iBlend];

		graphics->SetBlendMode(blend);
		effectItem_->SetTechnique(blend == MODE_BLEND_ALPHA_INV ? "RenderInv" : "Render");

		for (size_t i = 0; i < renderQueue.count; ++i) {
			StgItemObject* pItem = renderQueue.listItem[i];
			pItem->Render(blend);	//Render custom items
		}
	}

	device->SetVertexShader(nullptr);
	device->SetPixelShader(nullptr);
	device->SetVertexDeclaration(nullptr);
	device->SetIndices(nullptr);

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void StgItemManager::LoadRenderQueue() {
	for (size_t i = 0; i < listRenderQueue_.size(); ++i) {
		listRenderQueue_[i].count = 0;
	}

	for (ref_unsync_ptr<StgItemObject>& obj : listObj_) {
		if (obj->IsDeleted() || !obj->IsActive() || !obj->IsVisible()) continue;

		auto& [count, listItem] = listRenderQueue_[obj->GetRenderPriorityI()];

		while (count >= listItem.size())
			listItem.resize(listItem.size() * 2);
		listItem[count++] = obj.get();
	}
}

bool StgItemManager::LoadItemData(const std::wstring& path, bool bReload) {
	return listItemData_->AddItemDataList(path, bReload);
}
ref_unsync_ptr<StgItemObject> StgItemManager::CreateItem(int type) {
	ref_unsync_ptr<StgItemObject> res;
	switch (type) {
	case StgItemObject::ITEM_1UP:
	case StgItemObject::ITEM_1UP_S:
		res = new StgItemObject_1UP(stageController_);
		break;
	case StgItemObject::ITEM_SPELL:
	case StgItemObject::ITEM_SPELL_S:
		res = new StgItemObject_Bomb(stageController_);
		break;
	case StgItemObject::ITEM_POWER:
	case StgItemObject::ITEM_POWER_S:
		res = new StgItemObject_Power(stageController_);
		break;
	case StgItemObject::ITEM_POINT:
	case StgItemObject::ITEM_POINT_S:
		res = new StgItemObject_Point(stageController_);
		break;
	case StgItemObject::ITEM_USER:
		res = new StgItemObject_User(stageController_);
		break;
	}
	res->SetItemType(type);

	return res;
}
void StgItemManager::CollectItemsAll() {
	bAllItemToPlayer_ = true;
}
void StgItemManager::CollectItemsInCircle(const DxCircle& circle) {
	listCircleToPlayer_.push_back(circle);
}
void StgItemManager::CancelCollectItems() {
	bCancelToPlayer_ = true;
}

std::vector<int> StgItemManager::GetItemIdInCircle(int cx, int cy, int radius, int* itemType) {
	int rr = radius * radius;

	std::vector<int> res;
	for (ref_unsync_ptr<StgItemObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if (itemType != nullptr && (*itemType != obj->GetItemType())) continue;

		if (Math::HypotSq<int>(cx - obj->GetPositionX(), cy - obj->GetPositionY()) <= rr)
			res.push_back(obj->GetObjectID());
	}

	return res;
}

//*******************************************************************
//StgItemDataList
//*******************************************************************
StgItemDataList::StgItemDataList() {
}
StgItemDataList::~StgItemDataList() {
}
void StgItemDataList::_LoadVertexBuffers(std::map<std::wstring, VBContainerList>::iterator placement,
	shared_ptr<Texture> texture, std::vector<StgItemData*>& listAddData)
{
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	float texW = texture->GetWidth();
	float texH = texture->GetHeight();

	size_t countFrame = 0;
	for (StgItemData* iData : listAddData)
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
			StgItemData* data = listAddData[iData];
			for (size_t iAnim = 0; iAnim < data->GetFrameCount(); ++iAnim) {
				StgItemDataFrame* pFrame = &data->listFrame_[iAnim];
				pFrame->listItemData_ = this;

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
			std::wstring err = StringUtility::Format(L"AddItemDataList::Failed to load shot data buffer: "
				"\t\r\n%s: %s",
				DXGetErrorString(hr), DXGetErrorDescription(hr));
			throw gstd::wexception(err);
		}

		++iBuffer;
		countFrame -= thisCountFrame;
	}
}
bool StgItemDataList::AddItemDataList(const std::wstring& path, bool bReload) {
	auto itrVB = mapVertexBuffer_.find(path);
	if (!bReload && itrVB != mapVertexBuffer_.end()) return true;

	std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);

	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open()) 
		throw gstd::wexception(L"AddItemDataList: " + ErrorUtility::GetFileNotFoundErrorMessage(pathReduce, true));

	std::string source = reader->ReadAllString();

	bool res = false;
	Scanner scanner(source);
	try {
		std::map<int, unique_ptr<StgItemData>> mapData;
		std::wstring pathImage = L"";

		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::Type::TK_EOF)
				break;
			else if (tok.GetType() == Token::Type::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"ItemData") {
					_ScanItem(mapData, scanner);
				}
				else if (element == L"item_image") {
					scanner.CheckType(scanner.Next(), Token::Type::TK_EQUAL);
					pathImage = scanner.Next().GetString();
				}

				if (scanner.HasNext())
					tok = scanner.Next();
			}
		}

		if (pathImage.size() == 0) throw gstd::wexception("Item texture must be set.");
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

		std::vector<StgItemData*> listAddData;

		size_t countFrame = 0;
		{
			size_t i = 0;
			for (auto itr = mapData.begin(); itr != mapData.end(); ++itr, ++i) {
				int id = itr->first;
				unique_ptr<StgItemData>& data = itr->second;
				if (data == nullptr) continue;

				for (auto& iFrame : data->listFrame_)
					iFrame.listItemData_ = this;
				countFrame += data->GetFrameCount();

				listAddData.push_back(data.get());
				if (listData_.size() <= id)
					listData_.resize(id + 1);
				listData_[id] = std::move(data);		//Moves unique_ptr object, do not use mapData after this point

				if (unique_ptr<StgItemData>& dataOut = listData_[id]->dataOut_) {
					//Item data has an out frame
					
					dataOut->listFrame_[0].listItemData_ = this;
					++countFrame;

					listAddData.push_back(dataOut.get());
				}
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

		Logger::WriteTop(StringUtility::Format(L"Loaded item data: %s", pathReduce.c_str()));
		res = true;
	}
	catch (gstd::wexception& e) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: %s\r\n\t[Line=%d] (%s)",
			pathReduce.c_str(), scanner.GetCurrentLine(), e.what());
		Logger::WriteTop(log);
		res = false;
	}
	catch (...) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: %s\r\n\t[Line=%d] (Unknown error.)",
			pathReduce.c_str(), scanner.GetCurrentLine());
		Logger::WriteTop(log);
		res = false;
	}

	return res;
}
void StgItemDataList::_ScanItem(std::map<int, unique_ptr<StgItemData>>& mapData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::Type::TK_NEWLINE) tok = scanner.Next();
	scanner.CheckType(tok, Token::Type::TK_OPENC);

	StgItemDataList* ptrThis = this;
	struct Data {
		StgItemData* itemData;
		int id = -1;
		int typeItem = -1;
	} data;
	data.itemData = new StgItemData(this);

	//--------------------------------------------------------------

#define LAMBDA_SETI(m) [](Data* i, Scanner& s) { \
						s.CheckType(s.Next(), Token::Type::TK_EQUAL); \
						i->m = s.Next().GetInteger(); \
					}
	auto funcSetRect = [](Data* i, Scanner& s) {
		std::vector<std::wstring> list = s.GetArgumentList();

		if (list.size() < 4)
			throw wexception("Invalid argument list size (expected 4)");

		DxRect<LONG> rect(StringUtility::ToInteger(list[0]), StringUtility::ToInteger(list[1]),
			StringUtility::ToInteger(list[2]), StringUtility::ToInteger(list[3]));

		StgItemDataFrame dFrame;
		dFrame.rcSrc_ = rect;
		dFrame.rcDst_ = StgItemDataFrame::LoadDestRect(&rect);

		i->itemData->listFrame_ = { dFrame };
		i->itemData->totalFrame_ = 1;
	};
	auto funcSetOut = [&ptrThis](Data* i, Scanner& s) {
		std::vector<std::wstring> list = s.GetArgumentList();

		if (list.size() < 4)
			throw wexception("Invalid argument list size (expected 4)");

		DxRect<LONG> rect(StringUtility::ToInteger(list[0]), StringUtility::ToInteger(list[1]),
			StringUtility::ToInteger(list[2]), StringUtility::ToInteger(list[3]));

		StgItemDataFrame dFrame;
		dFrame.rcSrc_ = rect;
		dFrame.rcDst_ = StgItemDataFrame::LoadDestRect(&rect);

		StgItemData* out = new StgItemData(ptrThis);
		out->listFrame_ = { dFrame };
		out->totalFrame_ = 1;
		i->itemData->dataOut_.reset(out);
	};
	auto funcSetBlendType = [](Data* i, Scanner& s) {
		s.CheckType(s.Next(), Token::Type::TK_EQUAL);

		static const std::unordered_map<std::wstring, BlendMode> mapBlendType = {
			{ L"ADD", MODE_BLEND_ADD_RGB },
			{ L"ADD_RGB", MODE_BLEND_ADD_RGB },
			{ L"ADD_ARGB", MODE_BLEND_ADD_ARGB },
			{ L"MULTIPLY", MODE_BLEND_MULTIPLY },
			{ L"SUBTRACT", MODE_BLEND_SUBTRACT },
			{ L"SHADOW", MODE_BLEND_SHADOW },
			{ L"ALPHA_INV", MODE_BLEND_ALPHA_INV },
		};

		BlendMode typeRender = MODE_BLEND_ALPHA;
		auto itr = mapBlendType.find(s.Next().GetElement());
		if (itr != mapBlendType.end())
			typeRender = itr->second;

		i->itemData->typeRender_ = typeRender;
	};
	auto funcLoadAnimation = [](Data* i, Scanner& s) {
		i->itemData->listFrame_.clear();
		i->itemData->totalFrame_ = 0;
		_ScanAnimation(i->itemData, s);
	};

	//Do NOT use [&] lambdas
	static const std::unordered_map<std::wstring, std::function<void(Data*, Scanner&)>> mapFunc = {
		{ L"id", LAMBDA_SETI(id) },
		{ L"type", LAMBDA_SETI(typeItem) },
		{ L"alpha", LAMBDA_SETI(itemData->alpha_) },
		{ L"rect", funcSetRect },
		{ L"out", funcSetOut },
		{ L"render", funcSetBlendType },
		{ L"AnimationData", funcLoadAnimation },
	};

#undef LAMBDA_SETI
#undef LAMBDA_CREATELIST

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
		if (data.typeItem < 0)
			data.typeItem = data.id;
		data.itemData->typeItem_ = data.typeItem;

		mapData[data.id] = unique_ptr<StgItemData>(data.itemData);
	}
}
void StgItemDataList::_ScanAnimation(StgItemData* itemData, Scanner& scanner) {
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

				StgItemDataFrame dFrame;
				dFrame.frame_ = frame;
				dFrame.rcSrc_ = rect;
				dFrame.rcDst_ = StgItemDataFrame::LoadDestRect(&rect);

				itemData->listFrame_.push_back(dFrame);
				itemData->totalFrame_ += frame;
			}
		}
	}
}

//*******************************************************************
//StgItemDataFrame
//*******************************************************************
StgItemDataFrame::StgItemDataFrame() {
	listItemData_ = nullptr;
	pVertexBuffer_ = nullptr;
	vertexOffset_ = 0;
	frame_ = 0;
}
DxRect<float> StgItemDataFrame::LoadDestRect(DxRect<LONG>* src) {
	float width = src->GetWidth() / 2.0f;
	float height = src->GetHeight() / 2.0f;
	return DxRect<float>(-width, -height, width, height);
}

//*******************************************************************
//StgItemData
//*******************************************************************
StgItemData::StgItemData(StgItemDataList* listItemData) {
	listItemData_ = listItemData;

	typeItem_ = -1;
	typeRender_ = MODE_BLEND_ALPHA;

	alpha_ = 255;

	totalFrame_ = 0;
}
StgItemData::~StgItemData() {
}

StgItemDataFrame* StgItemData::GetFrame(size_t frame) {
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

//*******************************************************************
//StgItemObject
//*******************************************************************
StgItemObject::StgItemObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::Item;

	pattern_ = new StgMovePattern_Item(this);
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	typeItem_ = INT_MIN;

	frameWork_ = 0;

	score_ = 0;

	bMoveToPlayer_ = false;
	moveToPlayerFlags_ = FLAG_MOVETOPL_ALL;

	bDefaultScoreText_ = true;

	bAutoDelete_ = true;
	bIntersectEnable_ = true;
	itemIntersectRadius_ = 16 * 16;

	bDefaultCollectionMove_ = true;
	bRoundingPosition_ = false;

	int priItemI = stageController_->GetStageInformation()->GetItemObjectPriority();
	SetRenderPriorityI(priItemI);
}

void StgItemObject::Clone(DxScriptObjectBase* _src) {
	DxScriptShaderObject::Clone(_src);

	auto src = (StgItemObject*)_src;

	throw new wexception("Object cannot be cloned: ObjItem (non-user-defined)");
}

void StgItemObject::Work() {
	if (bEnableMovement_) {
		bool bNullMovePattern = dynamic_cast<StgMovePattern_Item*>(GetPattern().get()) == nullptr;
		if (bNullMovePattern && bDefaultCollectionMove_ && IsMoveToPlayer()) {
			float speed = 8;
			ref_unsync_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
			if (objPlayer) {
				float angle = atan2f(objPlayer->GetY() - GetPositionY(), objPlayer->GetX() - GetPositionX());
				float angDirection = angle;
				SetSpeed(speed);
				SetDirectionAngle(angDirection);
			}
		}
	}
	StgMoveObject::_Move();
	SetX(posX_);
	SetY(posY_);

	_DeleteInAutoClip();

	++frameWork_;
}
void StgItemObject::RenderOnItemManager() {
	StgItemManager* itemManager = stageController_->GetItemManager();
	SpriteList2D* renderer = typeItem_ == ITEM_SCORE_TEXT ?
		itemManager->GetDigitRenderer() : itemManager->GetItemRenderer();

	FLOAT sposx = posX_;
	FLOAT sposy = posY_;
	if (bRoundingPosition_) {
		sposx = roundf(sposx);
		sposy = roundf(sposy);
	}

	if (typeItem_ != ITEM_SCORE_TEXT) {
		float scale = 1.0f;
		switch (typeItem_) {
		case ITEM_1UP:
		case ITEM_SPELL:
		case ITEM_POWER:
		case ITEM_POINT:
			scale = 1.0f;
			break;
		case ITEM_1UP_S:
		case ITEM_SPELL_S:
		case ITEM_POWER_S:
		case ITEM_POINT_S:
		case ITEM_BONUS:
			scale = 0.75f;
			break;
		}

		DxRect<int> rcSrc;
		switch (typeItem_) {
		case ITEM_1UP:
		case ITEM_1UP_S:
			rcSrc.Set(1, 1, 16, 16);
			break;
		case ITEM_SPELL:
		case ITEM_SPELL_S:
			rcSrc.Set(20, 1, 35, 16);
			break;
		case ITEM_POWER:
		case ITEM_POWER_S:
			rcSrc.Set(40, 1, 55, 16);
			break;
		case ITEM_POINT:
		case ITEM_POINT_S:
			rcSrc.Set(1, 20, 16, 35);
			break;
		case ITEM_BONUS:
			rcSrc.Set(20, 20, 35, 35);
			break;
		}

		//上にはみ出している
		D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255);
		if (sposy <= 0) {
			D3DCOLOR colorOver = D3DCOLOR_ARGB(255, 255, 255, 255);
			switch (typeItem_) {
			case ITEM_1UP:
			case ITEM_1UP_S:
				colorOver = D3DCOLOR_ARGB(255, 236, 0, 236);
				break;
			case ITEM_SPELL:
			case ITEM_SPELL_S:
				colorOver = D3DCOLOR_ARGB(255, 0, 160, 0);
				break;
			case ITEM_POWER:
			case ITEM_POWER_S:
				colorOver = D3DCOLOR_ARGB(255, 209, 0, 0);
				break;
			case ITEM_POINT:
			case ITEM_POINT_S:
				colorOver = D3DCOLOR_ARGB(255, 0, 0, 160);
				break;
			}
			if (color != colorOver) {
				rcSrc.Set(113, 1, 126, 10);
				sposy = 6;
			}
			color = colorOver;
		}

		renderer->SetColor(color);
		renderer->SetPosition(sposx, sposy, 0);
		renderer->SetScaleXYZ(scale, scale, scale);
		renderer->SetSourceRect(rcSrc);
		renderer->SetDestinationCenter();
		renderer->AddVertex();
	}
	else {
		renderer->SetScaleXYZ(1.0, 1.0, 1.0);
		renderer->SetColor(color_);
		renderer->SetPosition(0, 0, 0);

		int fontSize = 14;
		int64_t score = score_;
		std::vector<int> listNum;
		while (true) {
			int tnum = score % 10;
			score /= 10;
			listNum.push_back(tnum);
			if (score == 0) break;
		}
		for (int iNum = listNum.size() - 1; iNum >= 0; iNum--) {
			DxRect<double> rcSrc(listNum[iNum] * 36, 0, 
				(listNum[iNum] + 1) * 36 - 1, 31);
			DxRect<double> rcDest(sposx + (listNum.size() - 1 - iNum) * fontSize / 2, sposy,
				sposx + (listNum.size() - iNum)*fontSize / 2, sposy + fontSize);
			renderer->SetSourceRect(rcSrc);
			renderer->SetDestinationRect(rcDest);
			renderer->AddVertex();
		}
	}
}
void StgItemObject::_DeleteInAutoClip() {
	if (!bAutoDelete_) return;
	DirectGraphics* graphics = DirectGraphics::GetBase();

	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetItemManager()->GetItemDeleteClip();

	bool bDelete = ((LONG)posX_ < rcClipBase->left) || ((LONG)posX_ > rcStgFrame->GetWidth() + rcClipBase->right)
		|| ((LONG)posY_ > rcStgFrame->GetHeight() + rcClipBase->bottom);
	if (!bDelete) return;

	stageController_->GetMainObjectManager()->DeleteObject(this);
}
void StgItemObject::_CreateScoreItem() {
	auto objectManager = stageController_->GetMainObjectManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
		ref_unsync_ptr<StgItemObject_ScoreText> obj = new StgItemObject_ScoreText(stageController_);

		obj->SetX(posX_);
		obj->SetY(posY_);
		obj->SetScore(score_);

		objectManager->AddObject(obj);
		itemManager->AddItem(obj);
	}
}
void StgItemObject::_NotifyEventToPlayerScript(gstd::value* listValue, size_t count) {
	ref_unsync_ptr<StgPlayerObject> player = stageController_->GetPlayerObject();
	if (player == nullptr) return;

	if (StgStagePlayerScript* scriptPlayer = player->GetPlayerScript()) {
		scriptPlayer->RequestEvent(StgStageItemScript::EV_GET_ITEM, listValue, count);
	}
}
void StgItemObject::_NotifyEventToItemScript(gstd::value* listValue, size_t count) {
	auto stageScriptManager = stageController_->GetScriptManager();

	LOCK_WEAK(itemScript, stageScriptManager->GetItemScript()) {
		itemScript->RequestEvent(StgStageItemScript::EV_GET_ITEM, listValue, count);
	}
}
void StgItemObject::SetAlpha(int alpha) {
	ColorAccess::ClampColor(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
void StgItemObject::SetColor(int r, int g, int b) {
	__m128i c = Vectorize::Set(color_ >> 24, r, g, b);
	color_ = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));
}
void StgItemObject::SetToPosition(D3DXVECTOR2& pos) {
	if (auto move = ref_unsync_ptr<StgMovePattern_Item>::Cast(pattern_))
		move->SetToPosition(pos);
}
int StgItemObject::GetMoveType() {
	int res = StgMovePattern_Item::MOVE_NONE;
	if (auto move = ref_unsync_ptr<StgMovePattern_Item>::Cast(pattern_))
		res = move->GetItemMoveType();
	return res;
}
void StgItemObject::SetMoveType(int type) {
	if (auto move = ref_unsync_ptr<StgMovePattern_Item>::Cast(pattern_))
		move->SetItemMoveType(type);
}

void StgItemObject::NotifyItemCollectEvent(int type, uint64_t eventParam) {
	auto stageScriptManager = stageController_->GetScriptManager();
	LOCK_WEAK(itemScript, stageScriptManager->GetItemScript()) {
		gstd::value eventArg[4];
		eventArg[0] = DxScript::CreateIntValue(idObject_);
		eventArg[1] = DxScript::CreateIntValue(typeItem_);
		eventArg[2] = DxScript::CreateIntValue(type);
		eventArg[3] = DxScript::CreateFloatValue(eventParam);

		itemScript->RequestEvent(StgStageItemScript::EV_COLLECT_ITEM, eventArg, 4U);
	}
}
void StgItemObject::NotifyItemCancelEvent(int type) {
	auto stageScriptManager = stageController_->GetScriptManager();
	LOCK_WEAK(itemScript, stageScriptManager->GetItemScript()) {
		gstd::value eventArg[4];
		eventArg[0] = DxScript::CreateIntValue(idObject_);
		eventArg[1] = DxScript::CreateIntValue(typeItem_);
		eventArg[2] = DxScript::CreateIntValue(type);

		itemScript->RequestEvent(StgStageItemScript::EV_CANCEL_ITEM, eventArg, 3U);
	}
}

//StgItemObject_1UP
StgItemObject_1UP::StgItemObject_1UP(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_1UP;
	SetMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_1UP::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	gstd::value listValue[2] = { 
		DxScript::CreateIntValue(typeItem_), 
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bomb
StgItemObject_Bomb::StgItemObject_Bomb(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_SPELL;
	SetMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_Bomb::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	gstd::value listValue[2] = {
		DxScript::CreateIntValue(typeItem_),
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Power
StgItemObject_Power::StgItemObject_Power(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_POWER;
	SetMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);

	score_ = 10;
}
void StgItemObject_Power::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	if (bDefaultScoreText_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	gstd::value listValue[2] = {
		DxScript::CreateIntValue(typeItem_),
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Point
StgItemObject_Point::StgItemObject_Point(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_POINT;
	SetMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_Point::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	if (bDefaultScoreText_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	gstd::value listValue[2] = {
		DxScript::CreateIntValue(typeItem_),
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bonus
StgItemObject_Bonus::StgItemObject_Bonus(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_BONUS;
	SetMoveType(StgMovePattern_Item::MOVE_TOPLAYER);

	int graze = stageController->GetStageInformation()->GetGraze();
	score_ = (int)(graze / 40) * 10 + 300;
}
void StgItemObject_Bonus::Work() {
	StgItemObject::Work();

	ref_unsync_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
	if (objPlayer != nullptr && objPlayer->GetState() != StgPlayerObject::STATE_NORMAL) {
		_CreateScoreItem();
		stageController_->GetStageInformation()->AddScore(score_);

		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgItemObject_Bonus::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_ScoreText
StgItemObject_ScoreText::StgItemObject_ScoreText(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_SCORE_TEXT;

	SetMoveType(StgMovePattern_Item::MOVE_SCORE);

	//Disable the text obj from being autocollected by any means
	moveToPlayerFlags_ = FLAG_MOVETOPL_NONE;

	frameDelete_ = 0;
}
void StgItemObject_ScoreText::Work() {
	StgItemObject::Work();
	int alpha = 255 - frameDelete_ * 8;
	color_ = D3DCOLOR_ARGB(alpha, alpha, alpha, alpha);

	if (frameDelete_ > 30) {
		stageController_->GetMainObjectManager()->DeleteObject(this);
		return;
	}
	++frameDelete_;
}
void StgItemObject_ScoreText::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) { }

//StgItemObject_User
StgItemObject_User::StgItemObject_User(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_USER;
	SetMoveType(StgMovePattern_Item::MOVE_DOWN);

	idImage_ = -1;
	bDefaultScoreText_ = true;
}

void StgItemObject_User::Clone(DxScriptObjectBase* _src) {
	DxScriptShaderObject::Clone(_src);

	auto src = (StgItemObject_User*)_src;
	StgMoveObject::Copy((StgMoveObject*)src);
	StgIntersectionObject::Copy((StgIntersectionObject*)src);

	typeItem_ = src->typeItem_;
	frameWork_ = src->frameWork_;
	score_ = src->score_;

	bMoveToPlayer_ = src->bMoveToPlayer_;
	moveToPlayerFlags_ = src->moveToPlayerFlags_;

	bDefaultScoreText_ = src->bDefaultScoreText_;
	bAutoDelete_ = src->bAutoDelete_;
	bIntersectEnable_ = src->bIntersectEnable_;
	itemIntersectRadius_ = src->itemIntersectRadius_;
	bDefaultCollectionMove_ = src->bDefaultCollectionMove_;
	bRoundingPosition_ = src->bRoundingPosition_;

	idImage_ = src->idImage_;
	renderTarget_ = src->renderTarget_;
}

void StgItemObject_User::SetImageID(int id) {
	idImage_ = id;
	StgItemData* data = _GetItemData();
	if (data) {
		typeItem_ = data->GetItemType();
	}
}
StgItemData* StgItemObject_User::_GetItemData() {
	StgItemManager* itemManager = stageController_->GetItemManager();
	StgItemDataList* dataList = itemManager->GetItemDataList();
	return dataList ? dataList->GetData(idImage_) : nullptr;
}

void StgItemObject_User::Work() {
	StgItemObject::Work();
	++frameWork_;
}
void StgItemObject_User::Render(BlendMode targetBlend) {
	//if (!IsVisible()) return;
	StgItemManager* itemManager = stageController_->GetItemManager();

	StgItemData* itemData = _GetItemData();
	if (itemData == nullptr) return;

	BlendMode objBlendType = GetBlendType();
	objBlendType = objBlendType == MODE_BLEND_NONE ? itemData->GetRenderType() : objBlendType;
	if (objBlendType != targetBlend) return;

	D3DXVECTOR2 rPos(position_), rScale, rAngle;
	D3DCOLOR rColor;

	{
		StgItemDataFrame* itemFrame = itemData->GetFrame(frameWork_);
		StgItemData* outData = itemData->GetOutData();

		DxRect<LONG>* rcSrc = itemFrame->GetSourceRect();

		bool bOutY = false;
		if (position_.y + rcSrc->GetHeight() / 2 <= 0)
			bOutY = true;

		if (!bOutY) {
			rScale = D3DXVECTOR2(scale_);
			rAngle = D3DXVECTOR2(cosf(angle_.z), sinf(angle_.z));
		}
		else {
			rPos.y = (rcSrc->bottom - rcSrc->top) / 2;
			rScale = D3DXVECTOR2(1, 1);
			rAngle = D3DXVECTOR2(1, 0);
		}

		if (bRoundingPosition_) {
			rPos.x = roundf(rPos.x);
			rPos.y = roundf(rPos.y);
		}

		rColor = color_;
		{
			float alphaRate = itemData->GetAlpha() / 255.0f;
			byte alpha = ColorAccess::ClampColorRet(((rColor >> 24) & 0xff) * alphaRate);
			rColor = (rColor & 0x00ffffff) | (alpha << 24);
		}

		StgItemDataFrame* targetFrame = bOutY ? (outData ? outData->GetFrame(0) : nullptr) : itemFrame;
		if (targetFrame) {
			StgShotVertexBufferContainer* pVB = targetFrame->GetVertexBufferContainer();
			DWORD vertexOffset = targetFrame->vertexOffset_;

			if (pVB) {
				DirectGraphics* graphics = DirectGraphics::GetBase();
				IDirect3DDevice9* device = graphics->GetDevice();

				if (graphics->IsAllowRenderTargetChange()) {
					if (auto pRT = renderTarget_.lock())
						graphics->SetRenderTarget(pRT);
					else graphics->SetRenderTarget(nullptr);
				}

				IDirect3DTexture9* pTexture = pVB->GetD3DTexture();
				if (pTexture != itemManager->pLastTexture_) {
					device->SetTexture(0, pTexture);
					itemManager->pLastTexture_ = pTexture;
				}
				device->SetStreamSource(0, pVB->GetD3DBuffer(), vertexOffset * sizeof(VERTEX_TLX), sizeof(VERTEX_TLX));

				{
					ID3DXEffect* effect = itemManager->GetEffect();
					if (shader_) {
						effect = shader_->GetEffect();
						if (shader_->LoadTechnique()) {
							shader_->LoadParameter();
						}
					}

					if (effect) {
						D3DXHANDLE handle = nullptr;
						if (handle = effect->GetParameterBySemantic(nullptr, "WORLD")) {
							D3DXMATRIX matTransform(
								rScale.x * rAngle.x, rScale.x * rAngle.y, 0, 0,
								rScale.y * -rAngle.y, rScale.y * rAngle.x, 0, 0,
								0, 0, 1, 0,
								rPos.x, rPos.y, 0, 1
							);
							effect->SetMatrix(handle, &matTransform);
						}
						if (shader_) {
							if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION")) {
								effect->SetMatrix(handle, itemManager->GetProjectionMatrix());
							}
						}
						if (handle = effect->GetParameterBySemantic(nullptr, "ICOLOR")) {
							//To normalized RGBA vector
							D3DXVECTOR4 vColor = ColorAccess::ToVec4Normalized(rColor, ColorAccess::PERMUTE_RGBA);
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
	}
}
void StgItemObject_User::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	if (bDefaultScoreText_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	gstd::value listValue[2] = {
		DxScript::CreateIntValue(typeItem_),
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//*******************************************************************
//StgMovePattern_Item
//*******************************************************************
StgMovePattern_Item::StgMovePattern_Item(StgMoveObject* target) : StgMovePattern(target) {
	frame_ = 0;
	typeMove_ = MOVE_DOWN;
	speed_ = 0;
	angDirection_ = Math::DegreeToRadian(270);
	posTo_ = D3DXVECTOR2(0, 0);
}

void StgMovePattern_Item::CopyFrom(StgMovePattern* _src) {
	StgMovePattern::CopyFrom(_src);
	auto src = (StgMovePattern_Item*)_src;

	frame_ = src->frame_;
	typeMove_ = src->typeMove_;
	speed_ = src->speed_;
	angDirection_ = src->angDirection_;
	posTo_ = src->posTo_;
}

void StgMovePattern_Item::Move() {
	StgItemObject* itemObject = (StgItemObject*)target_;
	StgStageController* stageController = itemObject->GetStageController();

	double px = target_->GetPositionX();
	double py = target_->GetPositionY();
	if (typeMove_ == MOVE_TOPLAYER || (itemObject->IsDefaultCollectionMovement() && itemObject->IsMoveToPlayer())) {
		if (frame_ == 0) speed_ = 6;
		speed_ += 0.075;
		ref_unsync_ptr<StgPlayerObject> objPlayer = stageController->GetPlayerObject();
		if (objPlayer) {
			double angle = atan2(objPlayer->GetY() - py, objPlayer->GetX() - px);
			angDirection_ = angle;
			c_ = cos(angDirection_);
			s_ = sin(angDirection_);
		}
	}
	else if (typeMove_ == MOVE_TOPOSITION_A) {
		double dx = posTo_.x - px;
		double dy = posTo_.y - py;
		speed_ = hypot(dx, dy) / 16.0;

		double angle = atan2(dy, dx);
		angDirection_ = angle;
		if (frame_ == 0) {
			c_ = cos(angDirection_);
			s_ = sin(angDirection_);
		}
		else if (frame_ == 60) {
			speed_ = 0;
			angDirection_ = Math::DegreeToRadian(90);
			typeMove_ = MOVE_DOWN;
			c_ = 0;
			s_ = 1;
		}
	}
	else if (typeMove_ == MOVE_DOWN) {
		speed_ += 3.0 / 60.0;
		if (speed_ > 2.5) speed_ = 2.5;
		angDirection_ = Math::DegreeToRadian(90);
		c_ = 0;
		s_ = 1;
	}
	else if (typeMove_ == MOVE_SCORE) {
		speed_ = 1;
		angDirection_ = Math::DegreeToRadian(270);
		c_ = 0;
		s_ = -1;
	}

	if (typeMove_ != MOVE_NONE) {
		target_->SetPositionX(px + speed_ * c_);
		target_->SetPositionY(py + speed_ * s_);
	}

	++frame_;
}