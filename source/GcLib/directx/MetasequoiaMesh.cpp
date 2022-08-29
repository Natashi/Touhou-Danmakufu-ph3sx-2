#include "source/GcLib/pch.h"

#include "MetasequoiaMesh.hpp"
#include "DxUtility.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//MetasequoiaMesh
//*******************************************************************

//MetasequoiaMeshData
MetasequoiaMeshData::MetasequoiaMeshData() {}
MetasequoiaMeshData::~MetasequoiaMeshData() {
	for (auto& obj : renderList_) ptr_delete(obj);
	for (auto& obj : materialList_) ptr_delete(obj);
}
bool MetasequoiaMeshData::CreateFromFileReader(shared_ptr<gstd::FileReader> reader) {
	bool res = false;
	path_ = reader->GetOriginalPath();
	std::string text;
	size_t size = reader->GetFileSize();
	text.resize(size);
	reader->Read(&text[0], size);

	gstd::Scanner scanner(text);
	try {
		while (scanner.HasNext()) {
			gstd::Token& tok = scanner.Next();
			if (tok.GetElement() == L"Material") {
				_ReadMaterial(scanner);
			}
			else if (tok.GetElement() == L"Object") {
				_ReadObject(scanner);
			}
		}

		res = true;
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(StringUtility::Format(L"MetasequoiaMeshData parsing error. [line %d-> %s]", 
			scanner.GetCurrentLine(), e.what()));
		res = false;
	}
	return res;
}
void MetasequoiaMeshData::_ReadMaterial(gstd::Scanner& scanner) {
	size_t countMaterial = scanner.Next().GetInteger();
	materialList_.resize(countMaterial);
	for (size_t iMat = 0; iMat < countMaterial; iMat++) {
		materialList_[iMat] = new Material();
	}
	scanner.CheckType(scanner.Next(), Token::Type::TK_OPENC);
	scanner.CheckType(scanner.Next(), Token::Type::TK_NEWLINE);

	size_t posMat = 0;
	Material* mat = materialList_[posMat];
	D3DCOLORVALUE color;
	ZeroMemory(&color, sizeof(D3DCOLORVALUE));
	while (true) {
		gstd::Token& tok = scanner.Next();
		if (tok.GetType() == Token::Type::TK_CLOSEC) break;

		if (tok.GetType() == Token::Type::TK_NEWLINE) {
			posMat++;
			if (materialList_.size() <= posMat)
				break;
			mat = materialList_[posMat];
			ZeroMemory(&color, sizeof(D3DCOLORVALUE));
		}
		else if (tok.GetType() == Token::Type::TK_STRING) {
			mat->name_ = tok.GetString();
		}
		else if (tok.GetElement() == L"col") {
			scanner.CheckType(scanner.Next(), Token::Type::TK_OPENP);
			color.r = scanner.Next().GetReal();
			color.g = scanner.Next().GetReal();
			color.b = scanner.Next().GetReal();
			color.a = scanner.Next().GetReal();
			scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEP);
		}
		else if (tok.GetElement() == L"dif" ||
			tok.GetElement() == L"amb" ||
			tok.GetElement() == L"emi" ||
			tok.GetElement() == L"spc") 
		{
			D3DCOLORVALUE* value = nullptr;
			if (tok.GetElement() == L"dif") value = &mat->mat_.Diffuse;
			else if (tok.GetElement() == L"amb") value = &mat->mat_.Ambient;
			else if (tok.GetElement() == L"emi") value = &mat->mat_.Emissive;
			else if (tok.GetElement() == L"spc") value = &mat->mat_.Specular;

			scanner.CheckType(scanner.Next(), Token::Type::TK_OPENP);
			float num = scanner.Next().GetReal();
			if (value) {
				value->a = color.a;
				value->r = color.r * num;
				value->g = color.g * num;
				value->b = color.b * num;
			}
			scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEP);
		}
		else if (tok.GetElement() == L"power") {
			scanner.CheckType(scanner.Next(), Token::Type::TK_OPENP);
			mat->mat_.Power = scanner.Next().GetReal();
			scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEP);
		}
		else if (tok.GetElement() == L"tex") {
			scanner.CheckType(scanner.Next(), Token::Type::TK_OPENP);
			tok = scanner.Next();

			std::wstring wPathTexture = tok.GetString();
			std::wstring path = PathProperty::GetFileDirectory(path_) + wPathTexture;
			mat->texture_ = std::make_shared<Texture>();
			mat->texture_->CreateFromFile(PathProperty::GetUnique(path), false, false);
			scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEP);
		}
	}
}
void MetasequoiaMeshData::_ReadObject(gstd::Scanner& scanner) {
	Object obj;
	obj.name_ = scanner.Next().GetString();
	scanner.CheckType(scanner.Next(), Token::Type::TK_OPENC);
	scanner.CheckType(scanner.Next(), Token::Type::TK_NEWLINE);

	//<material index, face list>
	std::map<int, std::list<MetasequoiaMeshData::Object::Face*>> mapFace;

	{
		int type_color = 0;

		while (true) {
			gstd::Token& tok = scanner.Next();
			if (tok.GetType() == Token::Type::TK_CLOSEC) break;

			if (tok.GetElement() == L"visible") {
				obj.bVisible_ = scanner.Next().GetInteger() == 15;
			}
			else if (tok.GetElement() == L"color") {
				obj.color_.x = scanner.Next().GetReal();
				obj.color_.y = scanner.Next().GetReal();
				obj.color_.z = scanner.Next().GetReal();
			}
			else if (tok.GetElement() == L"color_type") {
				type_color = scanner.Next().GetReal();
			}
			else if (tok.GetElement() == L"vertex") {
				size_t count = scanner.Next().GetInteger();
				obj.vertices_.resize(count);
				scanner.CheckType(scanner.Next(), Token::Type::TK_OPENC);
				scanner.CheckType(scanner.Next(), Token::Type::TK_NEWLINE);
				for (size_t iVert = 0; iVert < count; ++iVert) {
					obj.vertices_[iVert].x = scanner.Next().GetReal();
					obj.vertices_[iVert].y = scanner.Next().GetReal();
					obj.vertices_[iVert].z = -scanner.Next().GetReal();
					scanner.CheckType(scanner.Next(), Token::Type::TK_NEWLINE);
				}
				scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEC);
				scanner.CheckType(scanner.Next(), Token::Type::TK_NEWLINE);
			}
			else if (tok.GetElement() == L"face") {
				size_t countFace = scanner.Next().GetInteger();
				obj.faces_.resize(countFace);
				scanner.CheckType(scanner.Next(), Token::Type::TK_OPENC);
				scanner.CheckType(scanner.Next(), Token::Type::TK_NEWLINE);

				for (size_t iFace = 0; iFace < countFace; ++iFace) {
					size_t countVert = scanner.Next().GetInteger();
					obj.faces_[iFace].vertices_.resize(countVert);
					MetasequoiaMeshData::Object::Face* face = &obj.faces_[iFace];

					while (true) {
						gstd::Token& tok = scanner.Next();
						if (tok.GetType() == Token::Type::TK_NEWLINE) break;
						if (tok.GetElement() == L"V") {
							scanner.CheckType(scanner.Next(), Token::Type::TK_OPENP);
							for (size_t iVert = 0; iVert < countVert; ++iVert)
								face->vertices_[iVert].indexVertex_ = scanner.Next().GetInteger();
							scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEP);
						}
						else if (tok.GetElement() == L"M") {
							scanner.CheckType(scanner.Next(), Token::Type::TK_OPENP);
							face->indexMaterial_ = scanner.Next().GetInteger();
							scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEP);
						}
						else if (tok.GetElement() == L"UV") {
							scanner.CheckType(scanner.Next(), Token::Type::TK_OPENP);
							for (size_t iVert = 0; iVert < countVert; ++iVert) {
								face->vertices_[iVert].tcoord_.x = scanner.Next().GetReal();
								face->vertices_[iVert].tcoord_.y = scanner.Next().GetReal();
							}
							scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEP);
						}
					}

					mapFace[face->indexMaterial_].push_back(face);
				}

				scanner.CheckType(scanner.Next(), Token::Type::TK_CLOSEC);
				scanner.CheckType(scanner.Next(), Token::Type::TK_NEWLINE);
			}
			else if (tok.GetType() == Token::Type::TK_ID) {
				while (scanner.GetToken().GetType() != Token::Type::TK_OPENC &&
					scanner.GetToken().GetType() != Token::Type::TK_NEWLINE) {
					scanner.Next();
				}
				if (scanner.GetToken().GetType() == Token::Type::TK_OPENC) {
					while (scanner.GetToken().GetType() != Token::Type::TK_CLOSEC)
						scanner.Next();
				}
			}
		}

		//Post-processing
		{
			if (type_color != 1) obj.color_ = D3DXVECTOR3(1, 1, 1);
		}
	}

	//It's not visible, what the fuck do I have to do more?
	if (!obj.bVisible_) return;

	//Faces are sorted per-material
	size_t iMapFace = 0U;
	for (auto itrMap = mapFace.begin(); itrMap != mapFace.end(); itrMap++, iMapFace++) {
		int indexMaterial = itrMap->first;
		//if (indexMaterial < 0) continue;

		std::list<MetasequoiaMeshData::Object::Face*>& listFace = itrMap->second;

		MetasequoiaMeshData::RenderObject* render = new MetasequoiaMeshData::RenderObject();
		renderList_.push_back(render);
		if (indexMaterial >= 0 && indexMaterial < materialList_.size())
			render->material_ = materialList_[indexMaterial];

		render->objectColor_ = obj.color_;

		size_t countVert = 0;
		for (auto itrFace = listFace.begin(); itrFace != listFace.end(); ++itrFace) {
			MetasequoiaMeshData::Object::Face* face = *itrFace;
			size_t vc = face->vertices_.size();
			switch (vc) {
			case 3:		//Triangle
				countVert += 3;
				break;
			case 4:		//Quad
				countVert += 6;
				break;
			default:	//Polygon (Kill me)
				if (vc > 4)
					countVert += 3 * (vc - 2);
				break;
			}
		}
		render->SetVertexCount(countVert);

		size_t posVert = 0;
		for (auto itrFace = listFace.begin(); itrFace != listFace.end(); itrFace++) {
			MetasequoiaMeshData::Object::Face* face = *itrFace;
			if (face->vertices_.size() == 3) {			//Triangle
				size_t indexVert[3] = {
					face->vertices_[0].indexVertex_,
					face->vertices_[1].indexVertex_,
					face->vertices_[2].indexVertex_,
				};
				D3DXVECTOR3 normal = DxMath::Normalize(
					DxMath::CrossProduct(
						obj.vertices_[indexVert[1]] - obj.vertices_[indexVert[0]],
						obj.vertices_[indexVert[2]] - obj.vertices_[indexVert[0]]
					)
				);

				for (size_t iVert = 0; iVert < 3; iVert++) {
					size_t mqoVertexIndex = indexVert[iVert];
					size_t nxVertexIndex = posVert + iVert;
					VERTEX_NX* vert = render->GetVertex(nxVertexIndex);
					vert->position = obj.vertices_[mqoVertexIndex];
					vert->texcoord = face->vertices_[iVert].tcoord_;
					vert->normal = normal;
				}
				posVert += 3;
			}
			else if (face->vertices_.size() == 4) {		//Quad
				size_t indexVert[4] = {
					face->vertices_[0].indexVertex_,
					face->vertices_[1].indexVertex_,
					face->vertices_[2].indexVertex_,
					face->vertices_[3].indexVertex_,
				};
				D3DXVECTOR3 normals[2] = {
					DxMath::Normalize(
						DxMath::CrossProduct(
							obj.vertices_[indexVert[1]] - obj.vertices_[indexVert[0]],
							obj.vertices_[indexVert[2]] - obj.vertices_[indexVert[0]]
						)
					),
					DxMath::Normalize(
						DxMath::CrossProduct(
							obj.vertices_[indexVert[3]] - obj.vertices_[indexVert[2]],
							obj.vertices_[indexVert[0]] - obj.vertices_[indexVert[2]]
						)
					)
				};

				//MQO quad vertex arrangement:
				//    0----1
				//         |
				//    3----2
				size_t vIndexFaces[6] = { 0, 1, 3, 1, 2, 3 };
				for (size_t iVert = 0; iVert < 6; iVert++) {
					size_t nxVertexIndex = posVert + iVert;
					VERTEX_NX* vert = render->GetVertex(nxVertexIndex);

					size_t indexFace = vIndexFaces[iVert];

					vert->position = obj.vertices_[indexVert[indexFace]];
					vert->texcoord = face->vertices_[indexFace].tcoord_;
					vert->normal = normals[iVert / 3U];
				}

				posVert += 6;
			}
			else if (face->vertices_.size() > 4) {		//Polygon -> Triangle fans
				class FaceTriangulator {
				public:
					struct Node {
						D3DXVECTOR3 pos;
						D3DXVECTOR2 texCoord;
					};

					static float TriangleArea(D3DXVECTOR3* a, D3DXVECTOR3* b, D3DXVECTOR3* c) {
						D3DXVECTOR3 v_ab = (*a) - (*b);
						D3DXVECTOR3 v_ac = (*a) - (*c);
						D3DXVECTOR3 vCross;
						D3DXVec3Cross(&vCross, &v_ab, &v_ac);
						return 0.5f * D3DXVec3Length(&vCross);
					}
					static void FilterDuplicate(std::vector<Node>& listPoints) {
						//Try to eliminate duplicated nodes
						while (listPoints.size() >= 2) {
							bool bLoop = false;
							for (auto iNode = listPoints.begin(); iNode != std::prev(listPoints.end());) {
								if (iNode->pos == std::next(iNode)->pos) {
									iNode = listPoints.erase(iNode);
									bLoop = true;
								}
								else ++iNode;
							}
							if (!bLoop) break;
						}
					}
					static void FilterColinear(std::vector<Node>& listPoints) {
						//Try to eliminate colinear nodes
						while (listPoints.size() >= 3) {
							bool bLoop = false;
							for (auto iNode = std::next(listPoints.begin()); iNode != std::prev(listPoints.end());) {
								if (TriangleArea(&std::prev(iNode)->pos, &iNode->pos, &std::next(iNode)->pos) == 0) {
									iNode = listPoints.erase(iNode);
									bLoop = true;
								}
								else ++iNode;
							}
							if (!bLoop) break;
						}
						/*
						while (listPoints.size() > 3) {
							auto iNodeBeg = listPoints.begin();
							auto iNodeEnd = listPoints.rbegin();
							bool bLoop = false;
							if (TriangleArea(&iNodeBeg->pos, &std::next(iNodeBeg)->pos, &iNodeEnd->pos) == 0
								|| TriangleArea(&iNodeBeg->pos, &std::next(iNodeEnd)->pos, &iNodeEnd->pos) == 0) 
							{
								listPoints.erase(iNodeBeg);
								bLoop = true;
							}
							if (!bLoop) break;
						}
						*/
					}
				};

				std::vector<FaceTriangulator::Node> polygonNodes;
				polygonNodes.resize(face->vertices_.size());
				for (size_t i = 0; i < face->vertices_.size(); ++i) {
					size_t index = face->vertices_[i].indexVertex_;
					polygonNodes[i].pos = obj.vertices_[index];
					polygonNodes[i].texCoord = face->vertices_[i].tcoord_;
				}

				//Far from perfect, but this is as much I could do by myself ;w;
				FaceTriangulator::FilterDuplicate(polygonNodes);
				FaceTriangulator::FilterColinear(polygonNodes);

				size_t countTriangle = polygonNodes.size() - 2;

				//Build triangles from the nodes
				std::vector<FaceTriangulator::Node>::iterator itrNodeBase = polygonNodes.begin();
				for (size_t iTri = 0; iTri < countTriangle; ++iTri) {
					DxTriangle3D tri(itrNodeBase->pos, std::next(itrNodeBase, iTri + 1)->pos,
						std::next(itrNodeBase, iTri + 2)->pos);

					if (tri.GetArea() <= 0.04f) continue;

					D3DXVECTOR2 listUV[3] = {
						itrNodeBase->texCoord,
						std::next(itrNodeBase, iTri + 1)->texCoord,
						std::next(itrNodeBase, iTri + 2)->texCoord,
					};

					for (size_t iVert = 0; iVert < 3; ++iVert) {
						VERTEX_NX* vert = render->GetVertex(posVert + iVert);
						vert->position = tri.GetPosition(iVert);
						vert->texcoord = listUV[iVert];
						vert->normal = tri.GetNormal();
					}
					posVert += 3;
				}
			}
		}

		if (countVert > 0) {
			IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
			IDirect3DVertexBuffer9*& vertexBuf = render->pVertexBuffer_;

			size_t vertexBufSize = std::min(countVert, 65536U) * sizeof(VERTEX_NX);
			render->vertexBufferSize_ = vertexBufSize;

			void* pVoid;
			VERTEX_NX* pVertData = render->GetVertex(0);

			device->CreateVertexBuffer(vertexBufSize, 0, VERTEX_NX::fvf, D3DPOOL_MANAGED, &vertexBuf, nullptr);

			vertexBuf->Lock(0, vertexBufSize, &pVoid, D3DLOCK_DISCARD);
			memcpy(pVoid, pVertData, vertexBufSize);
			vertexBuf->Unlock();
		}
	}
}

//This causes a memory leak but who cares, it's only once and will get deleted once the game closes anyway
MetasequoiaMeshData::Material* MetasequoiaMeshData::RenderObject::nullMaterial_ = new Material();
void MetasequoiaMeshData::RenderObject::Render(D3DXMATRIX* matTransform) {
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();

	SetTexture(material_ ? material_->texture_ : nullptr);

	D3DXVECTOR4 tColor = D3DXVECTOR4(1, objectColor_.x, objectColor_.y, objectColor_.z) * 255.0f;
	ColorAccess::MultiplyColor(tColor, color_);
	D3DCOLOR dColor = ColorAccess::ToD3DCOLOR(tColor);

	D3DMATERIAL9 tMaterial = ColorAccess::MultiplyColor(
		material_ ? material_->mat_ : nullMaterial_->mat_, dColor);
	device->SetMaterial(&tMaterial);

	RenderObjectNX::Render(matTransform);
}

//MetasequoiaMesh
bool MetasequoiaMesh::CreateFromFileReader(shared_ptr<gstd::FileReader> reader) {
	bool res = false;
	{
		Lock lock(DxMeshManager::GetBase()->GetLock());
		if (data_) Release();

		const std::wstring& name = reader->GetOriginalPath();

		data_ = _GetFromManager(name);
		if (data_ == nullptr) {
			if (!reader->Open()) throw gstd::wexception("MetasequoiaMesh: Cannot open file for reading.");
			data_ = std::make_shared<MetasequoiaMeshData>();
			data_->SetName(name);
			MetasequoiaMeshData* data = (MetasequoiaMeshData*)data_.get();
			res = data->CreateFromFileReader(reader);
			if (res) {
				Logger::WriteTop(StringUtility::Format(L"MetasequoiaMesh: Mesh loaded. [%s]", name.c_str()));
				_AddManager(name, data_);
			}
			else {
				data_ = nullptr;
			}
		}
		else
			res = true;
	}
	return res;
}
bool MetasequoiaMesh::CreateFromFileInLoadThread(const std::wstring& path) {
	return DxMesh::CreateFromFileInLoadThread(path, MESH_METASEQUOIA);
}
std::wstring MetasequoiaMesh::GetPath() {
	if (data_ == nullptr) return L"";
	return ((MetasequoiaMeshData*)data_.get())->path_;
}

void MetasequoiaMesh::Render() {
	MetasequoiaMesh::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void MetasequoiaMesh::Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	if (data_ == nullptr) return;

	MetasequoiaMeshData* data = (MetasequoiaMeshData*)data_.get();

	while (!data->bLoad_) {
		::Sleep(10);
	}

	{
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();
		ref_count_ptr<DxCamera> camera = graphics->GetCamera();

		DWORD bFogEnable = FALSE;
		if (bCoordinate2D_) {
			device->SetTransform(D3DTS_VIEW, &camera->GetIdentity());
			device->GetRenderState(D3DRS_FOGENABLE, &bFogEnable);
			device->SetRenderState(D3DRS_FOGENABLE, FALSE);
			RenderObject::SetCoordinate2dDeviceMatrix();
		}

		D3DXMATRIX mat = RenderObject::CreateWorldMatrix(position_, scale_,
			angX, angY, angZ, &camera->GetIdentity(), bCoordinate2D_);
		device->SetTransform(D3DTS_WORLD, &mat);

		{
			size_t i = 0;
			for (auto render : data->renderList_) {
				render->SetVertexShaderRendering(bVertexShaderMode_);

				render->SetShader(shader_);
				render->SetColor(color_);
				render->Render(&mat);
			}
		}

		if (bCoordinate2D_) {
			device->SetTransform(D3DTS_VIEW, &camera->GetViewProjectionMatrix());
			device->SetRenderState(D3DRS_FOGENABLE, bFogEnable);
		}
	}
}
