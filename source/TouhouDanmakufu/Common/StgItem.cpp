#include "source/GcLib/pch.h"

#include "StgItem.hpp"
#include "StgSystem.hpp"
#include "StgStageScript.hpp"
#include "StgPlayer.hpp"

//*******************************************************************
//StgItemManager
//*******************************************************************
StgItemManager::StgItemManager(StgStageController* stageController) {
	auto textureManager = TextureManager::GetBase();
	
	stageController_ = stageController;

	listItemData_ = make_unique<StgItemDataList>();

	const std::wstring& dir = EPathProperty::GetSystemImageDirectory();

	{
		listSpriteItem_.reset(new SpriteList2D());

		std::wstring pathItem = PathProperty::GetUnique(dir + L"System_Stg_Item.png");
		auto textureItem = textureManager->CreateFromFile(pathItem);

		listSpriteItem_->SetTexture(textureItem);
	}
	{
		listSpriteDigit_.reset(new SpriteList2D());

		std::wstring pathDigit = PathProperty::GetUnique(dir + L"System_Stg_Digit.png");
		auto textureDigit = textureManager->CreateFromFile(pathDigit);
		
		listSpriteDigit_->SetTexture(textureDigit);
	}

	rcDeleteClip_ = DxRect<LONG>(-64, 0, 64, 64);

	filterMin_ = D3DTEXF_LINEAR;
	filterMag_ = D3DTEXF_LINEAR;

	bCancelToPlayer_ = false;
	bAllItemToPlayer_ = false;
	
	useDefaultBonusItem = true;
	defaultItemSpeedMultiplier = 1;

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

void StgItemManager::Work() {
	auto objPlayer = stageController_->GetPlayerObject();
	if (objPlayer == nullptr) return;

	float px = objPlayer->GetX();
	float py = objPlayer->GetY();
	int playerRadiusSq = objPlayer->GetItemIntersectionRadius() * objPlayer->GetItemIntersectionRadius();
	int playerPOC = objPlayer->GetAutoItemCollectY();

	for (auto itr = listObj_.begin(); itr != listObj_.end();) {
		auto& obj = *itr;

		if (obj->IsDeleted()) {
			//obj->Clear();
			itr = listObj_.erase(itr);
		}
		else {
			if (objPlayer->GetState() != StgPlayerObject::STATE_NORMAL) {
				// If player is dead, cancel existing move-to-player movement
				if (obj->isMovingToPlayer) {
					obj->isMovingToPlayer = false;
					obj->NotifyItemCancelEvent(StgItemObject::CancelType::PlayerDead);
				}
			}
			else {
				float dx = px - obj->position[0];
				float dy = py - obj->position[1];
				int radius = dx * dx + dy * dy;

				if (obj->isIntersectEnable && radius <= obj->itemIntersectRadius) {
					obj->Intersect(nullptr, nullptr);
				}
				else {
					int flags = obj->moveToPlayerFlags;

					if (bCancelToPlayer_ && obj->isMovingToPlayer) {
						// If receiving cancel event, set value to false and emit event

						obj->isMovingToPlayer = false;
						obj->NotifyItemCancelEvent(StgItemObject::CancelType::CancelAll);
					}
					else if (flags != 0 && !obj->isMovingToPlayer) {
						struct Param {
							StgItemObject::CollectType typeCollect;
							uint64_t eventParam;
						};
						optional<Param> moveToPlayer;

						if ((flags & StgItemObject::MoveToPlayerFlag_PlayerScope) && radius <= playerRadiusSq) {
							// Player item scope collection
							
							moveToPlayer = {
								StgItemObject::CollectType::PlayerScope,
								(uint64_t)objPlayer->GetItemIntersectionRadius(),
							};
						}
						else if ((flags & StgItemObject::MoveToPlayerFlag_CollectAllItems) && bAllItemToPlayer_) {
							// CollectAllItems collection
							
							moveToPlayer = {
								StgItemObject::CollectType::CollectAll,
								0,
							};
						}
						else if ((flags & StgItemObject::MoveToPlayerFlag_PlayerPoc) && py <= playerPOC) {
							// POC collection
							
							moveToPlayer = {
								StgItemObject::CollectType::PlayerPoc,
								(uint64_t)playerPOC,
							};
						}
						else if (flags & StgItemObject::MoveToPlayerFlag_Circle) {
							// CollectItemsInCircle collection
							
							for (DxCircle& circle : listCircleToPlayer_) {
								float rr = circle.GetR() * circle.GetR();

								double distSq = Math::HypotSq(
									obj->position[0] - circle.GetX(), 
									obj->position[1] - circle.GetY());

								if (distSq <= rr) {
									moveToPlayer = {
										StgItemObject::CollectType::InCircle,
										(uint64_t)circle.GetR(),
									};
								}
							}
						}

						if (moveToPlayer) {
							obj->isMovingToPlayer = true;
							obj->NotifyItemCollectEvent(
								moveToPlayer->typeCollect,
								moveToPlayer->eventParam);
						}
					}
				}
			}

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

	const auto& [count, renderItems] = listRenderQueue_[targetPriority];
	if (count == 0)
		return;

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

	//Render default items and score texts
	{
		for (size_t i = 0; i < count; ++i) {
			auto pItem = renderItems[i];
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

	for (auto blend : blendTypeRenderOrder) {
		graphics->SetBlendMode(blend);
		effectItem_->SetTechnique(blend == MODE_BLEND_ALPHA_INV ? "RenderInv" : "Render");

		for (size_t i = 0; i < count; ++i) {
			StgItemObject* pItem = renderItems[i];
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

	for (auto& obj : listObj_) {
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
ref_unsync_ptr<StgItemObject> StgItemManager::CreateItem(ItemType type) {
	ref_unsync_ptr<StgItemObject> res;
	switch (type) {
		case ItemType::OneUp:
		case ItemType::OneUpSmall:
			res.reset(new StgItemObject_1UP(stageController_));
			break;
		case ItemType::Spell:
		case ItemType::SpellSmall:
			res.reset(new StgItemObject_Bomb(stageController_));
			break;
		case ItemType::Power:
		case ItemType::PowerSmall:
			res.reset(new StgItemObject_Power(stageController_));
			break;
		case ItemType::Point:
		case ItemType::PointSmall:
			res.reset(new StgItemObject_Point(stageController_));
			break;
		default:
			res.reset(new StgItemObject_User(stageController_));
			break;
	}
	res->itemType = type;

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

std::vector<int> StgItemManager::GetItemIdInCircle(int cx, int cy,
	optional<int> radius, optional<ItemType> itemType)
{
	int r = radius.has_value() ? *radius : 0;
	int rr = r * r;

	std::vector<int> res;
	for (ref_unsync_ptr<StgItemObject>& obj : listObj_) {
		if (obj->IsDeleted()) continue;
		if (itemType.has_value() && (*itemType != obj->itemType)) continue;

		bool inRadius = Math::HypotSq<int>(cx - obj->position[0], cy - obj->position[1]) <= rr;
		if (!radius.has_value() || inRadius)
			res.push_back(obj->GetObjectID());
	}

	return res;
}

//*******************************************************************
//StgItemDataList
//*******************************************************************
void StgItemDataList::_LoadVertexBuffers(const std::wstring& name,
	shared_ptr<Texture> texture, const std::vector<StgItemData*>& listAddData)
{
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	float texW = texture->GetWidth();
	float texH = texture->GetHeight();

	size_t countFrame = 0;
	for (StgItemData* iData : listAddData)
		countFrame += iData->GetFrameCount();

	auto& containerList = mapVertexBuffer_[name];

	size_t iBuffer = 0;
	while (countFrame > 0) {
		{
			size_t nameId = std::hash<std::wstring>{}(name);
			std::string vbName = STR_FMT("item_vb_%x_%d", nameId, iBuffer);

			containerList.emplace_back(new StgShotVertexBufferContainer(vbName));
		}

		auto pVertexBufferContainer = containerList.back().get();
		pVertexBufferContainer->SetTexture(texture);

		size_t thisCountFrame = std::min<size_t>(countFrame, StgShotVertexBufferContainer::MAX_DATA);

		std::vector<VERTEX_TLX> bufferVertex(4 * thisCountFrame);
		size_t iVertex = 0;

		VERTEX_TLX verts[4];
		for (auto& data : listAddData) {
			for (auto& frame : data->listFrame_) {
				frame.listItemData_ = this;

				LONG* ptrSrc = reinterpret_cast<LONG*>(&frame.rcSrc_);
				float* ptrDst = reinterpret_cast<float*>(&frame.rcDst_);

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

				frame.pVertexBuffer_ = pVertexBufferContainer;
				frame.vertexOffset_ = iVertex;

				for (size_t j = 0; j < 4; ++j)
					bufferVertex[iVertex + j] = verts[j];
				iVertex += 4;
			}
		}

		HRESULT hr = pVertexBufferContainer->LoadData(bufferVertex, thisCountFrame);
		if (FAILED(hr)) {
			std::wstring err = StringUtility::Format(
				L"AddItemDataList::Failed to load item data buffer:\n\t%s: %s",
				DXGetErrorString(hr), DXGetErrorDescription(hr));
			Logger::WriteError(err);
			throw gstd::wexception(err);
		}

		++iBuffer;
		countFrame -= thisCountFrame;
	}
}
bool StgItemDataList::AddItemDataList(const std::wstring& path, bool bReload) {
	auto textureManager = TextureManager::GetBase();
	
	auto find = mapVertexBuffer_.find(path);
	if (find != mapVertexBuffer_.end()) {
		if (!bReload) {
			return true;
		}
		else {
			find->second.clear();
		}
	}

	std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);

	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open()) 
		throw gstd::wexception(L"AddItemDataList: " + ErrorUtility::GetFileNotFoundErrorMessage(pathReduce, true));

	std::string source = reader->ReadToString();

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

		auto texture = textureManager->CreateFromFile(pathImage);
		if (texture == nullptr) {
			throw gstd::wexception("Failed to load the specified shot texture.");
		}

		std::vector<StgItemData*> listAddData;

		size_t countFrame = 0;
		{
			size_t i = 0;
			for (auto& [id, data] : mapData) {
				if (data == nullptr) continue;

				for (auto& iFrame : data->listFrame_)
					iFrame.listItemData_ = this;
				countFrame += data->GetFrameCount();

				listAddData.push_back(data.get());
				if (listData_.size() <= id)
					listData_.resize(id + 1);
				listData_[id] = std::move(data);		//Moves unique_ptr object, do not use mapData after this point

				if (auto& dataOut = listData_[id]->dataOut_) {
					//Item data has an out frame
					
					dataOut->listFrame_[0].listItemData_ = this;
					++countFrame;

					listAddData.push_back(dataOut.get());
				}

				++i;
			}
		}

		_LoadVertexBuffers(path, texture, listAddData);

		Logger::WriteTop(StringUtility::Format(L"Loaded item data: %s", pathReduce.c_str()));
		res = true;
	}
	catch (gstd::wexception& e) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: %s\r\n\t[Line=%d] (%s)",
			pathReduce.c_str(), scanner.GetCurrentLine(), e.what());
		Logger::WriteError(log);
		res = false;
	}
	catch (...) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: %s\r\n\t[Line=%d] (Unknown error.)",
			pathReduce.c_str(), scanner.GetCurrentLine());
		Logger::WriteError(log);
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
		//int typeItem = -1;
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
		//{ L"type", LAMBDA_SETI(typeItem) },
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
		// TODO: WTF did this do??
		/*if (data.typeItem < 0)
			data.typeItem = data.id;
		data.itemData->typeItem_ = data.typeItem;*/

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
StgItemDataFrame::StgItemDataFrame() :
	listItemData_(nullptr),
	pVertexBuffer_(nullptr),
	vertexOffset_(0),
	frame_(0) {}

DxRect<float> StgItemDataFrame::LoadDestRect(DxRect<LONG>* src) {
	float width = src->GetWidth() / 2.0f;
	float height = src->GetHeight() / 2.0f;
	return DxRect<float>(-width, -height, width, height);
}

//*******************************************************************
//StgItemData
//*******************************************************************
StgItemData::StgItemData(StgItemDataList* listItemData) :
	listItemData_(listItemData),
	//typeItem_(-1),
	typeRender_(MODE_BLEND_ALPHA),
	alpha_(255),
	totalFrame_(0) {}

StgItemDataFrame* StgItemData::GetFrame(size_t frame) {
	if (totalFrame_ <= 1U)
		return &listFrame_[0];

	frame = frame % totalFrame_;
	size_t total = 0;

	for (auto& iFrame : listFrame_) {
		total += iFrame.frame_;
		if (total >= frame)
			return &iFrame;
	}
	return &listFrame_[0];
}

//*******************************************************************
//StgItemObject
//*******************************************************************
StgItemObject::StgItemObject(StgStageController* stageController) :
	StgMoveObject(stageController),
	frameWork_(0), itemType(ItemType::User),
	score(0), useDefaultScoreText(true),
	isMovingToPlayer(false), moveToPlayerFlags(MoveToPlayerFlag_All),
	canAutoDelete(true),
	isIntersectEnable(true), itemIntersectRadius(16 * 16),
	isDefaultCollectionMove(true),
	isRoundingPosition(false)
{
	typeObject_ = TypeObject::Item;

	pattern_.reset(new StgMovePattern_Item(this));
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);

	int priItemI = stageController_->GetStageInformation()->GetItemObjectPriority();
	SetRenderPriorityI(priItemI);
}

void StgItemObject::Clone(DxScriptObjectBase* _src) {
	DxScriptShaderObject::Clone(_src);

	//auto src = (StgItemObject*)_src;

	throw new wexception("Object cannot be cloned: ObjItem (non-user-defined)");
}

void StgItemObject::Work() {
	if (enableMovement) {
		// If the item is supposed to be moving towards the player but there's no attached item move pattern,
		//    initialize a new one

		bool noMovePattern = GetPattern() == nullptr || GetPattern()->GetType() != MovePatternType::Item;

		if (noMovePattern && isDefaultCollectionMove && isMovingToPlayer) {
			auto pattern = new StgMovePattern_Item(this);
			pattern->itemMoveType = StgMovePattern_Item::ItemMoveType::ToPlayer;

			pattern_.reset(pattern);
		}
	}

	StgMoveObject::_Move();
	SetX(position[0]);
	SetY(position[1]);

	_DeleteInAutoClip();

	++frameWork_;
}
void StgItemObject::RenderOnItemManager() {
	StgItemManager* itemManager = stageController_->GetItemManager();
	SpriteList2D* renderer = itemType == ItemType::ScoreText
		? itemManager->GetDigitRenderer()
		: itemManager->GetItemRenderer();

	auto spos = position;
	if (isRoundingPosition) {
		spos = {
			round(spos[0]),
			round(spos[1]),
		};
	}

	if (itemType != ItemType::ScoreText) {
		float scale;
		switch (itemType) {
			case ItemType::OneUp:
			case ItemType::Spell:
			case ItemType::Power:
			case ItemType::Point:
				scale = 1.0f;
				break;
			case ItemType::OneUpSmall:
			case ItemType::SpellSmall:
			case ItemType::PowerSmall:
			case ItemType::PointSmall:
			case ItemType::Bonus:
				scale = 0.75f;
				break;
			default:
				scale = 0.75f;
		}

		DxRect<int> rcSrc;
		switch (itemType) {
			case ItemType::OneUp:
			case ItemType::OneUpSmall:
				rcSrc.Set(1, 1, 16, 16);
				break;
			case ItemType::Spell:
			case ItemType::SpellSmall:
				rcSrc.Set(20, 1, 35, 16);
				break;
			case ItemType::Power:
			case ItemType::PowerSmall:
				rcSrc.Set(40, 1, 55, 16);
				break;
			case ItemType::Point:
			case ItemType::PointSmall:
				rcSrc.Set(1, 20, 16, 35);
				break;
			default:
				rcSrc.Set(20, 20, 35, 35);
		}
		
		D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255);
		if (spos[1] <= 0) {
			D3DCOLOR colorOver;
			switch (itemType) {
				case ItemType::OneUp:
				case ItemType::OneUpSmall:
					colorOver = D3DCOLOR_ARGB(255, 236, 0, 236);
					break;
				case ItemType::Spell:
				case ItemType::SpellSmall:
					colorOver = D3DCOLOR_ARGB(255, 0, 160, 0);
					break;
				case ItemType::Power:
				case ItemType::PowerSmall:
					colorOver = D3DCOLOR_ARGB(255, 209, 0, 0);
					break;
				case ItemType::Point:
				case ItemType::PointSmall:
					colorOver = D3DCOLOR_ARGB(255, 0, 0, 160);
					break;
				default:
					colorOver = D3DCOLOR_ARGB(255, 255, 255, 255);
			}
			
			if (color != colorOver) {
				rcSrc.Set(113, 1, 126, 10);
				spos[1] = 6;
			}
			color = colorOver;
		}

		renderer->SetColor(color);
		renderer->SetPosition(spos[0], spos[1], 0);
		renderer->SetScaleXYZ(scale, scale, scale);
		renderer->SetSourceRect(rcSrc);
		renderer->SetDestinationCenter();
		renderer->AddVertex();
	}
	else {
		renderer->SetScaleXYZ(1.0, 1.0, 1.0);
		renderer->SetColor(color_);
		renderer->SetPosition(0, 0, 0);

		auto score2 = score;

		std::vector<int> listNum;
		while (true) {
			listNum.push_back(score2 % 10);
			
			score2 /= 10;
			if (score2 == 0)
				break;
		}
		
		for (int iNum = listNum.size() - 1; iNum >= 0; iNum--) {
			constexpr int fontSize = 14;
			
			DxRect<double> rcSrc(listNum[iNum] * 36, 0, 
				(listNum[iNum] + 1) * 36 - 1, 31);
			DxRect<double> rcDest(
				spos[0] + (listNum.size() - 1 - iNum) * fontSize / 2,
				spos[1],
				spos[0] + (listNum.size() - iNum) * fontSize / 2,
				spos[1] + fontSize);

			renderer->SetSourceRect(rcSrc);
			renderer->SetDestinationRect(rcDest);
			renderer->AddVertex();
		}
	}
}
void StgItemObject::_DeleteInAutoClip() {
	if (!canAutoDelete)
		return;
	
	DxRect<LONG>* const rcStgFrame = stageController_->GetStageInformation()->GetStgFrameRect();
	DxRect<LONG>* const rcClipBase = stageController_->GetEnemyManager()->GetEnemyDeleteClip();
	DxRect<LONG> rcDeleteClip(rcClipBase->left, LONG_MIN,
		rcStgFrame->GetWidth() + rcClipBase->right,
		rcStgFrame->GetHeight() + rcClipBase->bottom);

	if (!rcDeleteClip.IsPointIntersected(position[0], position[1])) {
		stageController_->GetMainObjectManager()->DeleteObject(this);
	}
}
void StgItemObject::_CreateScoreItem() {
	auto objectManager = stageController_->GetMainObjectManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
		ref_unsync_ptr<StgItemObject_ScoreText> obj(new StgItemObject_ScoreText(stageController_));

		obj->SetX(position[0]);
		obj->SetY(position[1]);
		obj->score = score;

		objectManager->AddObject(obj);
		itemManager->AddItem(obj);
	}
}
void StgItemObject::_NotifyEventToPlayerScript(gstd::value* listValue, size_t count) {
	auto player = stageController_->GetPlayerObject();
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
void StgItemObject::SetToPosition(const Math::DVec2& pos) {
	if (auto pattern = dcast(StgMovePattern_Item*, pattern_.get()))
		pattern->SetToPosition(pos);
}

StgMovePattern_Item::ItemMoveType StgItemObject::GetMoveType() {
	if (auto pattern = dcast(StgMovePattern_Item*, pattern_.get()))
		return pattern->itemMoveType;
	return StgMovePattern_Item::ItemMoveType::None;
}
void StgItemObject::SetMoveType(StgMovePattern_Item::ItemMoveType type) {
	if (auto pattern = dcast(StgMovePattern_Item*, pattern_.get()))
		pattern->itemMoveType = type;
}

void StgItemObject::NotifyItemCollectEvent(CollectType type, uint64_t eventParam) {
	auto stageScriptManager = stageController_->GetScriptManager();
	LOCK_WEAK(itemScript, stageScriptManager->GetItemScript()) {
		gstd::value eventArg[4];
		eventArg[0] = DxScript::CreateIntValue(idObject_);
		eventArg[1] = DxScript::CreateIntValue((int)itemType);
		eventArg[2] = DxScript::CreateIntValue((int)type);
		eventArg[3] = DxScript::CreateFloatValue(eventParam);

		itemScript->RequestEvent(StgStageItemScript::EV_COLLECT_ITEM, eventArg, 4U);
	}
}
void StgItemObject::NotifyItemCancelEvent(CancelType type) {
	auto stageScriptManager = stageController_->GetScriptManager();
	LOCK_WEAK(itemScript, stageScriptManager->GetItemScript()) {
		gstd::value eventArg[4];
		eventArg[0] = DxScript::CreateIntValue(idObject_);
		eventArg[1] = DxScript::CreateIntValue((int)itemType);
		eventArg[2] = DxScript::CreateIntValue((int)type);

		itemScript->RequestEvent(StgStageItemScript::EV_CANCEL_ITEM, eventArg, 3U);
	}
}

//StgItemObject_1UP
StgItemObject_1UP::StgItemObject_1UP(StgStageController* stageController) : StgItemObject(stageController) {
	itemType = ItemType::OneUp;
	SetMoveType(StgMovePattern_Item::ItemMoveType::ToPosition);
}
void StgItemObject_1UP::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	gstd::value listValue[2] = { 
		DxScript::CreateIntValue((int)itemType), 
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bomb
StgItemObject_Bomb::StgItemObject_Bomb(StgStageController* stageController) : StgItemObject(stageController) {
	itemType = ItemType::Spell;
	SetMoveType(StgMovePattern_Item::ItemMoveType::ToPosition);
}
void StgItemObject_Bomb::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	gstd::value listValue[2] = {
		DxScript::CreateIntValue((int)itemType),
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Power
StgItemObject_Power::StgItemObject_Power(StgStageController* stageController) : StgItemObject(stageController) {
	itemType = ItemType::Power;
	SetMoveType(StgMovePattern_Item::ItemMoveType::ToPosition);

	score = 10;
}
void StgItemObject_Power::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	if (useDefaultScoreText)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score);

	gstd::value listValue[2] = {
		DxScript::CreateIntValue((int)itemType),
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Point
StgItemObject_Point::StgItemObject_Point(StgStageController* stageController) : StgItemObject(stageController) {
	itemType = ItemType::Point;
	SetMoveType(StgMovePattern_Item::ItemMoveType::ToPosition);
}
void StgItemObject_Point::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	if (useDefaultScoreText)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score);

	gstd::value listValue[2] = {
		DxScript::CreateIntValue((int)itemType),
		DxScript::CreateIntValue(idObject_)
	};
	_NotifyEventToPlayerScript(listValue, 2);
	_NotifyEventToItemScript(listValue, 2);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bonus
StgItemObject_Bonus::StgItemObject_Bonus(StgStageController* stageController) : StgItemObject(stageController) {
	itemType = ItemType::Bonus;
	SetMoveType(StgMovePattern_Item::ItemMoveType::ToPlayer);

	int graze = stageController->GetStageInformation()->GetGraze();
	score = graze / 40.0 * 10 + 300;
}
void StgItemObject_Bonus::Work() {
	StgItemObject::Work();

	ref_unsync_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
	if (objPlayer != nullptr && objPlayer->GetState() != StgPlayerObject::STATE_NORMAL) {
		_CreateScoreItem();
		stageController_->GetStageInformation()->AddScore(score);

		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgItemObject_Bonus::Intersect(StgIntersectionTarget* ownTarget, StgIntersectionTarget* otherTarget) {
	_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_ScoreText
StgItemObject_ScoreText::StgItemObject_ScoreText(StgStageController* stageController) : StgItemObject(stageController) {
	itemType = ItemType::ScoreText;

	SetMoveType(StgMovePattern_Item::ItemMoveType::ScoreText);

	// Disable the score text from being autocollected, since it's technically an item object
	moveToPlayerFlags = MoveToPlayerFlag_None;

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
	itemType = ItemType::User;
	SetMoveType(StgMovePattern_Item::ItemMoveType::Down);

	idImage_ = -1;
	useDefaultScoreText = true;
}

void StgItemObject_User::Clone(DxScriptObjectBase* _src) {
	DxScriptShaderObject::Clone(_src);

	auto src = (StgItemObject_User*)_src;
	StgMoveObject::Copy((StgMoveObject*)src);
	StgIntersectionObject::Copy((StgIntersectionObject*)src);

	itemType = src->itemType;
	frameWork_ = src->frameWork_;
	score = src->score;

	isMovingToPlayer = src->isMovingToPlayer;
	moveToPlayerFlags = src->moveToPlayerFlags;

	useDefaultScoreText = src->useDefaultScoreText;
	canAutoDelete = src->canAutoDelete;
	isIntersectEnable = src->isIntersectEnable;
	itemIntersectRadius = src->itemIntersectRadius;
	isDefaultCollectionMove = src->isDefaultCollectionMove;
	isRoundingPosition = src->isRoundingPosition;

	idImage_ = src->idImage_;
	renderTarget_ = src->renderTarget_;
}

void StgItemObject_User::SetImageID(int id) {
	idImage_ = id;

	// TODO: WTF did this do??
	/*if (auto data = _GetItemData()) {
		itemType = data->GetItemType();
	}*/
}
StgItemData* StgItemObject_User::_GetItemData() {
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (auto dataList = itemManager->GetItemDataList()) {
		return dataList->GetData(idImage_);
	}
	return nullptr;
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

		if (isRoundingPosition) {
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
	if (useDefaultScoreText) 
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score);

	gstd::value listValue[2] = {
		DxScript::CreateIntValue((int)itemType),
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
StgMovePattern_Item::StgMovePattern_Item(StgMoveObject* target) :
	StgMovePattern(target),
	itemMoveType(ItemMoveType::None),
	frame_(0),
	speed_(0),
	angDirection_(Math::DegreeToRadian(270)),
	posTo_({}) {}

void StgMovePattern_Item::Move() {
	StgItemObject* itemObject = (StgItemObject*)target_;
	StgStageController* stageController = itemObject->GetStageController();

	double speedMultiplier = stageController->GetItemManager()->defaultItemSpeedMultiplier;
	
	double px = target_->position[0];
	double py = target_->position[1];

	if (itemMoveType == ItemMoveType::ToPlayer || (itemObject->isDefaultCollectionMove && itemObject->isMovingToPlayer)) {
		speed_ = (frame_ == 0) 
			? 6 
			: speed_ + 0.075;
		
		if (auto objPlayer = stageController->GetPlayerObject()) {
			double angle = atan2(objPlayer->GetY() - py, objPlayer->GetX() - px);

			angDirection_ = angle;
			c_ = cos(angDirection_);
			s_ = sin(angDirection_);
		}
	}
	else if (itemMoveType == ItemMoveType::ToPosition) {
		double dx = posTo_[0] - px;
		double dy = posTo_[1] - py;
		speed_ = hypot(dx, dy) / 16.0;

		double angle = atan2(dy, dx);
		angDirection_ = angle;

		if (frame_ == 0) {
			c_ = cos(angDirection_);
			s_ = sin(angDirection_);
		}
		else if (frame_ == 60) {
			// Then transition to float down
			itemMoveType = ItemMoveType::Down;

			speed_ = 0;

			angDirection_ = Math::DegreeToRadian(90);
			c_ = 0;
			s_ = 1;
		}
	}
	else if (itemMoveType == ItemMoveType::Down) {
		speed_ = std::min(speed_ + 3 / 60.0, 2.5);

		angDirection_ = Math::DegreeToRadian(90);
		c_ = 0;
		s_ = 1;
	}
	else if (itemMoveType == ItemMoveType::ScoreText) {
		speed_ = 1;

		angDirection_ = Math::DegreeToRadian(270);
		c_ = 0;
		s_ = -1;
	}

	if (itemMoveType != ItemMoveType::None) {
		target_->position = {
			px + speed_ * speedMultiplier * c_,
			py + speed_ * speedMultiplier * s_,
		};
	}

	++frame_;
}
