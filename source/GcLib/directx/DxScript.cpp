#include "source/GcLib/pch.h"

#include "DxScript.hpp"
#include "DxUtility.hpp"
#include "DirectGraphics.hpp"
#include "DirectInput.hpp"
#include "MetasequoiaMesh.hpp"
#include "ElfreinaMesh.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//DxScriptObjectBase
**********************************************************/
DxScriptObjectBase::DxScriptObjectBase() {
	bVisible_ = true;
	priRender_ = 50;
	bDeleted_ = false;
	bActive_ = false;
	manager_ = nullptr;
	idObject_ = DxScript::ID_INVALID;
	idScript_ = ScriptClientBase::ID_SCRIPT_FREE;
	typeObject_ = TypeObject::OBJ_INVALID;
}
DxScriptObjectBase::~DxScriptObjectBase() {
	//if (manager_ != nullptr && idObject_ != DxScript::ID_INVALID)
	//	manager_->listUnusedIndex_.push_back(idObject_);
}
void DxScriptObjectBase::SetRenderPriority(double pri) {
	priRender_ = pri * (manager_->GetRenderBucketCapacity() - 1U);
}
double DxScriptObjectBase::GetRenderPriority() {
	return (double)priRender_ / (manager_->GetRenderBucketCapacity() - 1U);
}

bool DxScriptObjectBase::IsObjectValueExists(std::wstring key) { 
	return IsObjectValueExists(GetKeyHash(key));
}
gstd::value DxScriptObjectBase::GetObjectValue(std::wstring key) { 
	return GetObjectValue(GetKeyHash(key));
}
void DxScriptObjectBase::SetObjectValue(std::wstring key, gstd::value val) { 
	SetObjectValue(GetKeyHash(key), val);
}
void DxScriptObjectBase::DeleteObjectValue(std::wstring key) { 
	DeleteObjectValue(GetKeyHash(key));
}

/**********************************************************
//DxScriptRenderObject
**********************************************************/
DxScriptRenderObject::DxScriptRenderObject() {
	bZWrite_ = false;
	bZTest_ = false;
	bFogEnable_ = false;
	typeBlend_ = DirectGraphics::MODE_BLEND_ALPHA;
	modeCulling_ = D3DCULL_NONE;
	position_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	angle_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	
	color_ = 0xffffffff;

	filterMin_ = D3DTEXF_LINEAR;
	filterMag_ = D3DTEXF_LINEAR;
	filterMip_ = D3DTEXF_NONE;
	bVertexShaderMode_ = false;
}

/**********************************************************
//DxScriptShaderObject
**********************************************************/
DxScriptShaderObject::DxScriptShaderObject() {
	typeObject_ = TypeObject::OBJ_SHADER;
}

/**********************************************************
//DxScriptPrimitiveObject
**********************************************************/
DxScriptPrimitiveObject::DxScriptPrimitiveObject() {
	idRelative_ = -1;

	angX_ = D3DXVECTOR2(1, 0);
	angY_ = D3DXVECTOR2(1, 0);
	angZ_ = D3DXVECTOR2(1, 0);
}
void DxScriptPrimitiveObject::SetAngleX(float x) {
	x = gstd::Math::DegreeToRadian(x);
	if (angle_.x != x) {
		angle_.x = x;
		angX_ = D3DXVECTOR2(cosf(-x), sinf(-x));
	}
}
void DxScriptPrimitiveObject::SetAngleY(float y) {
	y = gstd::Math::DegreeToRadian(y);
	if (angle_.y != y) {
		angle_.y = y;
		angY_ = D3DXVECTOR2(cosf(-y), sinf(-y));
	}
}
void DxScriptPrimitiveObject::SetAngleZ(float z) {
	z = gstd::Math::DegreeToRadian(z);
	if (angle_.z != z) {
		angle_.z = z;
		angZ_ = D3DXVECTOR2(cosf(-z), sinf(-z));
	}
}

/**********************************************************
//DxScriptPrimitiveObject2D
**********************************************************/
DxScriptPrimitiveObject2D::DxScriptPrimitiveObject2D() {
	typeObject_ = TypeObject::OBJ_PRIMITIVE_2D;

	objRender_ = std::make_shared<RenderObjectTLX>();
	bZWrite_ = false;
	bZTest_ = false;
}
void DxScriptPrimitiveObject2D::Render() {
	RenderObjectTLX* obj = GetObjectPointer();

	//フォグを解除する
	DirectGraphics* graphics = DirectGraphics::GetBase();
	DWORD bEnableFog = FALSE;
	graphics->GetDevice()->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	SetRenderState();
	obj->Render(angX_, angY_, angZ_);

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void DxScriptPrimitiveObject2D::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	RenderObjectTLX* obj = GetObjectPointer();
	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetZBufferEnable(false);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	obj->SetFilteringMin(filterMin_);
	obj->SetFilteringMag(filterMag_);
	obj->SetFilteringMip(filterMip_);
	obj->SetVertexShaderRendering(bVertexShaderMode_);
}
bool DxScriptPrimitiveObject2D::IsValidVertexIndex(size_t index) {
	return index >= 0 && index < GetObjectPointer()->GetVertexCount();
}
void DxScriptPrimitiveObject2D::SetColor(int r, int g, int b) {
	RenderObjectTLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	obj->SetColorRGB(D3DCOLOR_XRGB(r, g, b));
	color_ = D3DCOLOR_ARGB(color_ >> 24, r, g, b);
}
void DxScriptPrimitiveObject2D::SetAlpha(int alpha) {
	RenderObjectTLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(alpha);
	obj->SetAlpha(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
void DxScriptPrimitiveObject2D::SetVertexPosition(size_t index, float x, float y, float z) {
	RenderObjectTLX* obj = GetObjectPointer();
	obj->SetVertexPosition(index, x, y, z);
}
void DxScriptPrimitiveObject2D::SetVertexUV(size_t index, float u, float v) {
	RenderObjectTLX* obj = GetObjectPointer();
	obj->SetVertexUV(index, u, v);
}
void DxScriptPrimitiveObject2D::SetVertexAlpha(size_t index, int alpha) {
	RenderObjectTLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(alpha);
	obj->SetVertexAlpha(index, alpha);
}
void DxScriptPrimitiveObject2D::SetVertexColor(size_t index, int r, int g, int b) {
	RenderObjectTLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	obj->SetVertexColorRGB(index, r, g, b);
}
void DxScriptPrimitiveObject2D::SetPermitCamera(bool bPermit) {
	RenderObjectTLX* obj = GetObjectPointer();
	obj->SetPermitCamera(bPermit);
}
D3DXVECTOR3 DxScriptPrimitiveObject2D::GetVertexPosition(size_t index) {
	D3DXVECTOR3 res(0, 0, 0);
	if (!IsValidVertexIndex(index))return res;
	RenderObjectTLX* obj = GetObjectPointer();
	VERTEX_TLX* vert = obj->GetVertex(index);

	constexpr float bias = 0.5f;
	res.x = vert->position.x + bias;
	res.y = vert->position.y + bias;
	res.z = 0;

	return res;
}

/**********************************************************
//DxScriptSpriteObject2D
**********************************************************/
DxScriptSpriteObject2D::DxScriptSpriteObject2D() {
	typeObject_ = TypeObject::OBJ_SPRITE_2D;
	objRender_ = std::make_shared<Sprite2D>();
}
void DxScriptSpriteObject2D::Copy(DxScriptSpriteObject2D* src) {
	priRender_ = src->priRender_;
	bZWrite_ = src->bZWrite_;
	bZTest_ = src->bZTest_;
	modeCulling_ = src->modeCulling_;
	bVisible_ = src->bVisible_;
	manager_ = src->manager_;
	position_ = src->position_;
	angle_ = src->angle_;
	scale_ = src->scale_;
	typeBlend_ = src->typeBlend_;

	Sprite2D* destSprite2D = (Sprite2D*)objRender_.get();
	Sprite2D* srcSprite2D = (Sprite2D*)src->objRender_.get();
	destSprite2D->Copy(srcSprite2D);
}

/**********************************************************
//DxScriptSpriteListObject2D
**********************************************************/
DxScriptSpriteListObject2D::DxScriptSpriteListObject2D() {
	typeObject_ = TypeObject::OBJ_SPRITE_LIST_2D;
	objRender_ = std::make_shared<SpriteList2D>();
}
void DxScriptSpriteListObject2D::SetColor(int r, int g, int b) {
	D3DCOLOR color = GetSpritePointer()->GetColor();

	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	color_ = D3DCOLOR_ARGB(color >> 24, r, g, b);

	GetSpritePointer()->SetColor(color_);
}
void DxScriptSpriteListObject2D::SetAlpha(int alpha) {
	D3DCOLOR color = GetSpritePointer()->GetColor();
	ColorAccess::ClampColor(alpha);
	color_ = (color & 0x00ffffff) | ((byte)alpha << 24);

	GetSpritePointer()->SetColor(color_);
}
void DxScriptSpriteListObject2D::AddVertex() {
	SpriteList2D* obj = GetSpritePointer();
	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	obj->AddVertex(angX_, angY_, angZ_);
}
void DxScriptSpriteListObject2D::CloseVertex() {
	SpriteList2D* obj = GetSpritePointer();
	obj->CloseVertex();

	position_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	angle_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);

	angX_ = D3DXVECTOR2(1, 0);
	angY_ = D3DXVECTOR2(1, 0);
	angZ_ = D3DXVECTOR2(1, 0);
}

/**********************************************************
//DxScriptPrimitiveObject3D
**********************************************************/
DxScriptPrimitiveObject3D::DxScriptPrimitiveObject3D() {
	typeObject_ = TypeObject::OBJ_PRIMITIVE_3D;
	objRender_ = std::make_shared<RenderObjectLX>();
	bZWrite_ = false;
	bZTest_ = true;
	bFogEnable_ = true;
}
void DxScriptPrimitiveObject3D::Render() {
	RenderObjectLX* obj = GetObjectPointer();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool bEnvFogEnable = graphics->IsFogEnable();
	SetRenderState();
	//obj->Render();
	obj->Render(angX_, angY_, angZ_);

	//フォグの状態をリセット
	if (bEnvFogEnable)
		graphics->SetFogEnable(true);
}
void DxScriptPrimitiveObject3D::SetRenderState() {
	if (idRelative_ >= 0) {
		shared_ptr<DxScriptObjectBase> objRelative = manager_->GetObject(idRelative_);
		if (objRelative != nullptr) {
			objRelative->SetRenderState();
			DxScriptMeshObject* objMesh = dynamic_cast<DxScriptMeshObject*>(objRelative.get());
			if (objMesh != nullptr) {
				int frameAnime = objMesh->GetAnimeFrame();
				std::wstring nameAnime = objMesh->GetAnimeName();
				shared_ptr<DxMesh> mesh = objMesh->GetMesh();
				D3DXMATRIX mat = mesh->GetAnimationMatrix(nameAnime, frameAnime, nameRelativeBone_);
				objRender_->SetRalativeMatrix(mat);
			}
		}
	}

	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool bEnvFogEnable = graphics->IsFogEnable();

	RenderObjectLX* obj = GetObjectPointer();
	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	if (bEnvFogEnable)
		graphics->SetFogEnable(bFogEnable_);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	obj->SetFilteringMin(filterMin_);
	obj->SetFilteringMag(filterMag_);
	obj->SetFilteringMip(filterMip_);
	obj->SetVertexShaderRendering(bVertexShaderMode_);
}
bool DxScriptPrimitiveObject3D::IsValidVertexIndex(size_t index) {
	return index >= 0 && index < GetObjectPointer()->GetVertexCount();
}
void DxScriptPrimitiveObject3D::SetColor(int r, int g, int b) {
	RenderObjectLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	obj->SetColorRGB(D3DCOLOR_XRGB(r, g, b));
	color_ = D3DCOLOR_ARGB(color_ >> 24, r, g, b);
}
void DxScriptPrimitiveObject3D::SetAlpha(int alpha) {
	RenderObjectLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(alpha);
	obj->SetAlpha(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
void DxScriptPrimitiveObject3D::SetVertexPosition(size_t index, float x, float y, float z) {
	RenderObjectLX* obj = GetObjectPointer();
	obj->SetVertexPosition(index, x, y, z);
}
void DxScriptPrimitiveObject3D::SetVertexUV(size_t index, float u, float v) {
	RenderObjectLX* obj = GetObjectPointer();
	obj->SetVertexUV(index, u, v);
}
void DxScriptPrimitiveObject3D::SetVertexAlpha(size_t index, int alpha) {
	RenderObjectLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(alpha);
	obj->SetVertexAlpha(index, alpha);
}
void DxScriptPrimitiveObject3D::SetVertexColor(size_t index, int r, int g, int b) {
	if (!IsValidVertexIndex(index))return;
	RenderObjectLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	obj->SetVertexColorRGB(index, r, g, b);
}
D3DXVECTOR3 DxScriptPrimitiveObject3D::GetVertexPosition(size_t index) {
	D3DXVECTOR3 res(0, 0, 0);
	if (!IsValidVertexIndex(index))return res;
	RenderObjectLX* obj = GetObjectPointer();
	VERTEX_LX* vert = obj->GetVertex(index);

	res.x = vert->position.x;
	res.y = vert->position.y;
	res.z = vert->position.z;

	return res;
}
/**********************************************************
//DxScriptSpriteObject3D
**********************************************************/
DxScriptSpriteObject3D::DxScriptSpriteObject3D() {
	typeObject_ = TypeObject::OBJ_SPRITE_3D;
	objRender_ = std::make_shared<Sprite3D>();
}
/**********************************************************
//DxScriptTrajectoryObject3D
**********************************************************/
DxScriptTrajectoryObject3D::DxScriptTrajectoryObject3D() {
	typeObject_ = TypeObject::OBJ_TRAJECTORY_3D;
	objRender_ = std::make_shared<TrajectoryObject3D>();
	color_ = 0xffffffff;
}
void DxScriptTrajectoryObject3D::Work() {
	if (idRelative_ >= 0) {
		shared_ptr<DxScriptObjectBase> objRelative = manager_->GetObject(idRelative_);
		if (objRelative != nullptr) {
			objRelative->SetRenderState();
			DxScriptMeshObject* objMesh = dynamic_cast<DxScriptMeshObject*>(objRelative.get());
			if (objMesh != nullptr) {
				int frameAnime = objMesh->GetAnimeFrame();
				std::wstring nameAnime = objMesh->GetAnimeName();
				shared_ptr<DxMesh> mesh = objMesh->GetMesh();
				D3DXMATRIX matAnime = mesh->GetAnimationMatrix(nameAnime, frameAnime, nameRelativeBone_);

				TrajectoryObject3D* objRender = GetObjectPointer();
				objRender->AddPoint(matAnime);
			}
		}
	}

	TrajectoryObject3D* obj = GetObjectPointer();
	obj->Work();
}
void DxScriptTrajectoryObject3D::Render() {
	TrajectoryObject3D* obj = GetObjectPointer();
	SetRenderState();
	//obj->Render();
	obj->Render(angX_, angY_, angZ_);
}
void DxScriptTrajectoryObject3D::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	TrajectoryObject3D* obj = GetObjectPointer();
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	obj->SetFilteringMin(filterMin_);
	obj->SetFilteringMag(filterMag_);
	obj->SetFilteringMip(filterMip_);
	obj->SetVertexShaderRendering(bVertexShaderMode_);
}
void DxScriptTrajectoryObject3D::SetColor(int r, int g, int b) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	TrajectoryObject3D* obj = GetObjectPointer();

	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	color_ = D3DCOLOR_XRGB(r, g, b);

	obj->SetColor(color_);
}

/**********************************************************
//DxScriptParticleListObject2D
**********************************************************/
DxScriptParticleListObject2D::DxScriptParticleListObject2D() {
	typeObject_ = TypeObject::OBJ_PARTICLE_LIST_2D;
	objRender_ = std::make_shared<ParticleRenderer2D>();
}
void DxScriptParticleListObject2D::Render() {
	ParticleRenderer2D* obj = GetParticlePointer();

	//フォグを解除する
	DirectGraphics* graphics = DirectGraphics::GetBase();
	DWORD bEnableFog = FALSE;
	graphics->GetDevice()->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	SetRenderState();
	obj->Render();

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void DxScriptParticleListObject2D::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ParticleRenderer2D* obj = GetParticlePointer();
	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetZBufferEnable(false);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	obj->SetFilteringMin(filterMin_);
	obj->SetFilteringMag(filterMag_);
	obj->SetFilteringMip(filterMip_);
}
/**********************************************************
//DxScriptParticleListObject3D
**********************************************************/
DxScriptParticleListObject3D::DxScriptParticleListObject3D() {
	typeObject_ = TypeObject::OBJ_PARTICLE_LIST_3D;
	objRender_ = std::make_shared<ParticleRenderer3D>();
}
void DxScriptParticleListObject3D::Render() {
	ParticleRenderer3D* obj = GetParticlePointer();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool bEnvFogEnable = graphics->IsFogEnable();

	SetRenderState();
	obj->Render();

	if (bEnvFogEnable)
		graphics->SetFogEnable(true);
}
void DxScriptParticleListObject3D::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ParticleRenderer3D* obj = GetParticlePointer();
	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	if (graphics->IsFogEnable()) graphics->SetFogEnable(bFogEnable_);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	obj->SetFilteringMin(filterMin_);
	obj->SetFilteringMag(filterMag_);
	obj->SetFilteringMip(filterMip_);
}

/**********************************************************
//DxScriptMeshObject
**********************************************************/
DxScriptMeshObject::DxScriptMeshObject() {
	typeObject_ = TypeObject::OBJ_MESH;
	bZWrite_ = true;
	bZTest_ = true;
	bFogEnable_ = true;
	time_ = 0;
	anime_ = L"";
	color_ = 0xffffffff;
	bCoordinate2D_ = false;

	angX_ = D3DXVECTOR2(1, 0);
	angY_ = D3DXVECTOR2(1, 0);
	angZ_ = D3DXVECTOR2(1, 0);

	filterMin_ = D3DTEXF_LINEAR;
	filterMag_ = D3DTEXF_LINEAR;
	filterMip_ = D3DTEXF_NONE;
	bVertexShaderMode_ = false;
}

void DxScriptMeshObject::Render() {
	if (mesh_ == nullptr)return;
	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool bEnvFogEnable = graphics->IsFogEnable();
	SetRenderState();
	mesh_->Render(anime_, time_, angX_, angY_, angZ_);

	//フォグの状態をリセット
	if (bEnvFogEnable)
		graphics->SetFogEnable(true);
}
void DxScriptMeshObject::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool bEnvFogEnable = graphics->IsFogEnable();

	mesh_->SetPosition(position_);
	mesh_->SetAngle(angle_);
	mesh_->SetScale(scale_);
	mesh_->SetColor(color_);
	mesh_->SetCoordinate2D(bCoordinate2D_);

	graphics->SetLightingEnable(true);
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	if (bEnvFogEnable)
		graphics->SetFogEnable(bFogEnable_);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);

	mesh_->SetFilteringMin(filterMin_);
	mesh_->SetFilteringMag(filterMag_);
	mesh_->SetFilteringMip(filterMip_);
	mesh_->SetVertexShaderRendering(bVertexShaderMode_);
}
void DxScriptMeshObject::SetColor(int r, int g, int b) {
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	color_ = D3DCOLOR_ARGB(color_ >> 24, r, g, b);
}
void DxScriptMeshObject::SetAlpha(int alpha) {
	ColorAccess::ClampColor(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
void DxScriptMeshObject::_UpdateMeshState() {
	if (mesh_ == nullptr)return;
	mesh_->SetPosition(position_);
	mesh_->SetAngle(angle_);
	mesh_->SetScale(scale_);
	mesh_->SetColor(color_);
}
void DxScriptMeshObject::SetShader(shared_ptr<Shader> shader) {
	if (mesh_ == nullptr)return;
	mesh_->SetShader(shader);
}
void DxScriptMeshObject::SetAngleX(float x) {
	x = gstd::Math::DegreeToRadian(x);
	if (angle_.x != x) {
		angle_.x = x;
		angX_ = D3DXVECTOR2(cosf(-x), sinf(-x));
		_UpdateMeshState();
	}
}
void DxScriptMeshObject::SetAngleY(float y) {
	y = gstd::Math::DegreeToRadian(y);
	if (angle_.y != y) {
		angle_.y = y;
		angY_ = D3DXVECTOR2(cosf(-y), sinf(-y));
		_UpdateMeshState();
	}
}
void DxScriptMeshObject::SetAngleZ(float z) {
	z = gstd::Math::DegreeToRadian(z);
	if (angle_.z != z) {
		angle_.z = z;
		angZ_ = D3DXVECTOR2(cosf(-z), sinf(-z));
		_UpdateMeshState();
	}
}

/**********************************************************
//DxScriptTextObject
**********************************************************/
DxScriptTextObject::DxScriptTextObject() {
	typeObject_ = TypeObject::OBJ_TEXT;
	bChange_ = true;
	bAutoCenter_ = true;
	center_ = D3DXVECTOR2(0, 0);

	angX_ = D3DXVECTOR2(1, 0);
	angY_ = D3DXVECTOR2(1, 0);
	angZ_ = D3DXVECTOR2(1, 0);
}
void DxScriptTextObject::Render() {
	//フォグを解除する
	DirectGraphics* graphics = DirectGraphics::GetBase();
	DWORD bEnableFog = FALSE;
	graphics->GetDevice()->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	SetRenderState();
	text_.SetPosition(position_.x, position_.y);
	_UpdateRenderer();
	objRender_->SetPosition(position_.x, position_.y);
	objRender_->SetAngle(angle_);
	objRender_->SetScale(scale_);
	objRender_->SetVertexColor(text_.GetVertexColor());
	objRender_->SetTransCenter(center_);
	objRender_->SetAutoCenter(bAutoCenter_);
	objRender_->Render(angX_, angY_, angZ_);
	bChange_ = false;

	if (bEnableFog)
		graphics->SetFogEnable(true);
}
void DxScriptTextObject::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetZBufferEnable(false);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(D3DCULL_NONE);
	//obj->SetFilteringMin(filterMin_);
	//obj->SetFilteringMag(filterMag_);
}
void DxScriptTextObject::_UpdateRenderer() {
	if (bChange_) {
		textInfo_ = text_.GetTextInfo();
		objRender_ = text_.CreateRenderObject(textInfo_);
	}
	bChange_ = false;
}
void DxScriptTextObject::SetCharset(BYTE set) {
	switch (set) {
	case ANSI_CHARSET:
	case HANGUL_CHARSET:
	case ARABIC_CHARSET:
	case HEBREW_CHARSET:
	case THAI_CHARSET:
	case SHIFTJIS_CHARSET:
		break;
	default:
		set = ANSI_CHARSET;
		break;
	}
	text_.SetFontCharset(set); 
	bChange_ = true;
}
void DxScriptTextObject::SetText(std::wstring text) {
	size_t newHash = std::hash<std::wstring>{}(text);
	if (newHash == text_.GetTextHash()) return;

	text_.SetTextHash(newHash);
	text_.SetText(text); 

	bChange_ = true;
}
std::vector<int> DxScriptTextObject::GetTextCountCU() {
	_UpdateRenderer();
	int lineCount = textInfo_->GetLineCount();
	std::vector<int> listCount;
	for (int iLine = 0; iLine < lineCount; ++iLine) {
		shared_ptr<DxTextLine> textLine = textInfo_->GetTextLine(iLine);
		int count = textLine->GetTextCodeCount();
		listCount.push_back(count);
	}
	return listCount;
}
void DxScriptTextObject::SetAlpha(int alpha) {
	D3DCOLOR color = text_.GetVertexColor();
	ColorAccess::ClampColor(alpha);
	color_ = (color & 0x00ffffff) | ((byte)alpha << 24);
	SetVertexColor(color_);
}
void DxScriptTextObject::SetColor(int r, int g, int b) {
	D3DCOLOR color = text_.GetVertexColor();
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	color_ = D3DCOLOR_ARGB(color >> 24, r, g, b);
	SetVertexColor(color_);
}
int DxScriptTextObject::GetTotalWidth() {
	_UpdateRenderer();
	int res = textInfo_->GetTotalWidth();
	return res;
}
int DxScriptTextObject::GetTotalHeight() {
	_UpdateRenderer();
	int res = textInfo_->GetTotalHeight();
	return res;
}
void DxScriptTextObject::SetShader(shared_ptr<Shader> shader) {
	text_.SetShader(shader);
	bChange_ = true;
}
void DxScriptTextObject::SetAngleX(float x) {
	x = gstd::Math::DegreeToRadian(x);
	if (angle_.x != x) {
		angle_.x = x;
		angX_ = D3DXVECTOR2(cosf(-x), sinf(-x));
	}
}
void DxScriptTextObject::SetAngleY(float y) {
	y = gstd::Math::DegreeToRadian(y);
	if (angle_.y != y) {
		angle_.y = y;
		angY_ = D3DXVECTOR2(cosf(-y), sinf(-y));
	}
}
void DxScriptTextObject::SetAngleZ(float z) {
	z = gstd::Math::DegreeToRadian(z);
	if (angle_.z != z) {
		angle_.z = z;
		angZ_ = D3DXVECTOR2(cosf(-z), sinf(-z));
	}
}

/**********************************************************
//DxSoundObject
**********************************************************/
DxSoundObject::DxSoundObject() {
	typeObject_ = TypeObject::OBJ_SOUND;
}
DxSoundObject::~DxSoundObject() {
	if (player_ == nullptr)return;
	player_->Delete();
}
bool DxSoundObject::Load(std::wstring path) {
	DirectSoundManager* manager = DirectSoundManager::GetBase();
	player_ = manager->GetPlayer(path);
	if (player_ == nullptr)return false;

	return true;
}
void DxSoundObject::Play() {
	if (player_ != nullptr)
		player_->Play(style_);
}

/**********************************************************
//DxFileObject
**********************************************************/
DxFileObject::DxFileObject() {
	isArchived_ = false;
}
DxFileObject::~DxFileObject() {}
bool DxFileObject::OpenR(std::wstring path) {
	file_ = new File(path);
	bool res = file_->Open();
	if (!res) file_ = nullptr;
	return res;
}
bool DxFileObject::OpenR(gstd::ref_count_ptr<gstd::FileReader> reader) {
	file_ = nullptr;
	reader_ = reader;
	return true;
}
bool DxFileObject::OpenRW(std::wstring path) {
	path = PathProperty::Canonicalize(path);
	path = PathProperty::ReplaceYenToSlash(path);

	std::wstring dir = PathProperty::GetFileDirectory(path);
	bool bDir = File::CreateDirectory(dir);
	if (!bDir) return false;

	std::wstring dirModule = PathProperty::GetModuleDirectory();
	if (dir.find(dirModule) == std::wstring::npos)return false;

	file_ = new File(path);
	bool res = file_->Open(File::WRITE);
	if (!res) file_ = nullptr;
	return res;
}
void DxFileObject::Close() {
	if (file_ == nullptr)return;
	file_->Close();
}

/**********************************************************
//DxTextFileObject
**********************************************************/
DxTextFileObject::DxTextFileObject() {
	typeObject_ = TypeObject::OBJ_FILE_TEXT;
	encoding_ = Encoding::UTF16LE;
	bomSize_ = 2U;
	memcpy(bomHead_, Encoding::BOM_UTF16LE, 2);
	bytePerChar_ = 2U;
}
DxTextFileObject::~DxTextFileObject() {}
bool DxTextFileObject::OpenR(std::wstring path) {
	listLine_.clear();
	bool res = DxFileObject::OpenR(path);
	if (!res) return false;

	size_t size = file_->GetSize();
	if (size == 0) return true;

	std::vector<char> text;
	text.resize(size);
	file_->Read(&text[0], size);

	return _ParseLines(text);
}
bool DxTextFileObject::OpenR(gstd::ref_count_ptr<gstd::FileReader> reader) {
	listLine_.clear();
	reader_ = reader;

	size_t size = reader->GetFileSize();
	if (size == 0)return true;

	std::vector<char> text;
	text.resize(size);
	reader->Read(&text[0], size);

	return _ParseLines(text);
}
bool DxTextFileObject::OpenRW(std::wstring path) {
	listLine_.clear();
	bool res = DxFileObject::OpenRW(path);
	if (!res) return false;

	size_t size = file_->GetSize();
	if (size == 0) return true;

	std::vector<char> text;
	text.resize(size);
	file_->Read(&text[0], size);

	return _ParseLines(text);
}
bool DxTextFileObject::_ParseLines(std::vector<char>& src) {
	std::vector<char>::iterator lineBegin;

	if (memcmp(&src[0], Encoding::BOM_UTF16LE, 2) == 0) {
		encoding_ = Encoding::UTF16LE;
		bomSize_ = 2U;
		bytePerChar_ = 2U;
		memcpy(bomHead_, Encoding::BOM_UTF16LE, 2);
	}
	else if (memcmp(&src[0], Encoding::BOM_UTF16BE, 2) == 0) {
		encoding_ = Encoding::UTF16BE;
		bomSize_ = 2U;
		bytePerChar_ = 2U;
		memcpy(bomHead_, Encoding::BOM_UTF16BE, 2);
	}
	else {
		encoding_ = Encoding::UTF8;
		bomSize_ = 0U;
		bytePerChar_ = 1U;
		ZeroMemory(bomHead_, 2);
	}

	try {
		if (bytePerChar_ == 1U) {
			std::vector<char> tmp;

			auto itr = src.begin();
			for (; itr != src.end();) {
				char* ch = &*itr;
				if (*ch == '\r' || *ch == '\n') {
					++itr;
					++ch;
					if (*ch == '\r' || *ch == '\n') ++itr;

					listLine_.push_back(tmp);
					tmp.clear();
				}
				else {
					tmp.push_back(*ch);
					++itr;
				}
			}

			if (tmp.size() > 0) listLine_.push_back(tmp);
		}
		else if (bytePerChar_ == 2U) {
			bool bLittleEndian = encoding_ == Encoding::UTF16LE;
			lineBegin = src.begin() + 2;	//sizeof(BOM)

			wchar_t CH_CR = L'\r';
			wchar_t CH_LF = L'\n';
			if (!bLittleEndian) {
				CH_CR = (CH_CR >> 8) | (CH_CR << 8);
				CH_LF = (CH_LF >> 8) | (CH_LF << 8);
			}

			auto push_line = [&](std::vector<char>& vecLine) {
				if (!bLittleEndian && vecLine.size() > 0) {
					if (vecLine.size() % 2 == 1) throw;
					size_t iChar = 0;
					for (wchar_t* pChar = (wchar_t*)&vecLine[0]; iChar < vecLine.size(); iChar += 2, ++pChar) {
						wchar_t ch = *pChar;
						*pChar = (ch >> 8) | (ch << 8);
					}
				}
				listLine_.push_back(vecLine);
			};

			std::vector<char> tmp;

			auto itr = src.begin() + 2;		//sizeof(BOM)
			for (; itr != src.end();) {
				wchar_t* wch = (wchar_t*)&*itr;
				if (*wch == CH_CR || *wch == CH_LF) {
					itr += 2;
					++wch;
					if (*wch == CH_CR || *wch == CH_LF) itr += 2;
					
					push_line(tmp);
					tmp.clear();
				}
				else {
					tmp.push_back(((char*)wch)[0]);
					tmp.push_back(((char*)wch)[1]);
					itr += 2;
				}
			}

			if (tmp.size() > 0) push_line(tmp);
		}
	}
	catch (...) {
		return false;
	}
	return true;
}
bool DxTextFileObject::Store() {
	if (isArchived_) return false;
	if (file_ == nullptr)return false;

	file_->SeekWrite(0, std::ios::beg);

	if (bomSize_ > 0) file_->Write(bomHead_, bomSize_);

	for (size_t iLine = 0; iLine < listLine_.size(); ++iLine) {
		std::vector<char>& str = listLine_[iLine];

		if (encoding_ == Encoding::UTF16LE || bytePerChar_ == 1) {
			if (str.size() > 0) file_->Write(&str[0], str.size());

			if (iLine < listLine_.size() - 1) {
				if (bytePerChar_ == 1) file_->Write("\r\n", 2);
				else if (bytePerChar_ == 2) file_->Write(L"\r\n", 4);
			}
		}
		else if (encoding_ == Encoding::UTF16BE) {
			if (str.size() > 0) {
				std::vector<char> strSwap = str;
				auto itrVec = strSwap.begin();
				for (wchar_t* pChar = (wchar_t*)(&*itrVec); itrVec != strSwap.end(); itrVec += 2, ++pChar) {
					wchar_t ch = *pChar;
					*pChar = (ch >> 8) | (ch << 8);
				}
				file_->Write(&strSwap[0], strSwap.size());
			}

			if (iLine < listLine_.size() - 1)
				file_->Write("\0\r\0\n", 4);
		}
	}
	return true;
}
std::string DxTextFileObject::GetLineAsString(size_t line) {
	line--; //Line number begins at 1
	if (line >= listLine_.size()) return "";

	std::string res;
	std::vector<char>& src = listLine_[line];
	if (src.size() > 0) {
		if (bytePerChar_ == 1) {
			res.resize(src.size());
			memcpy(&res[0], &src[0], src.size());
		}
		else if (bytePerChar_ == 2) {
			std::wstring _res;
			_res.resize(src.size());
			memcpy(&_res[0], &src[0], src.size());
			res = StringUtility::ConvertWideToMulti(_res);
		}
	}
	return res;
}
std::wstring DxTextFileObject::GetLineAsWString(size_t line) {
	line--; //Line number begins at 1
	if (line >= listLine_.size()) return L"";

	std::wstring res = L"";
	std::vector<char>& src = listLine_[line];
	if (src.size() > 0) {
		if (bytePerChar_ == 2) {
			res.resize(src.size() / 2U);
			memcpy(&res[0], &src[0], src.size());
		}
		else if (bytePerChar_ == 1) {
			std::string _res;
			_res.resize(src.size());
			memcpy(&_res[0], &src[0], src.size());
			res = StringUtility::ConvertMultiToWide(_res);
		}
	}
	return res;
}
void DxTextFileObject::SetLineAsString(std::string& text, size_t line) {
	line--; //Line number begins at 1
	if (line >= listLine_.size()) return;

	std::vector<char> newLine;
	if (text.size() > 0) {
		if (bytePerChar_ == 1) {
			newLine.resize(text.size());
			memcpy(&newLine[0], &text[0], newLine.size());
		}
		else if (bytePerChar_ == 2) {
			std::wstring _res = StringUtility::ConvertMultiToWide(text);
			newLine.resize(_res.size() * 2U);
			memcpy(&newLine[0], &_res[0], newLine.size());
		}
	}
	listLine_[line] = newLine;
}
void DxTextFileObject::SetLineAsWString(std::wstring& text, size_t line) {
	line--; //Line number begins at 1
	if (line >= listLine_.size()) return;

	std::vector<char> newLine;
	if (text.size() > 0) {
		if (bytePerChar_ == 2) {
			newLine.resize(text.size() * 2U);
			memcpy(&newLine[0], &text[0], newLine.size());
		}
		else if (bytePerChar_ == 1) {
			std::string _res = StringUtility::ConvertWideToMulti(text);
			newLine.resize(_res.size());
			memcpy(&newLine[0], &_res[0], newLine.size());
		}
	}
	listLine_[line] = newLine;
}
void DxTextFileObject::_AddLine(char* pChar, size_t count) {
	if (isArchived_) return;
	std::vector<char> newLine;
	newLine.resize(count);
	memcpy(&newLine[0], pChar, count);
	listLine_.push_back(newLine);
}
void DxTextFileObject::AddLine(std::string line) {
	if (bytePerChar_ == 1) _AddLine(&line[0], line.size() * sizeof(char));
	else if (bytePerChar_ == 2) {
		std::wstring str = StringUtility::ConvertMultiToWide(line);
		_AddLine((char*)&str[0], str.size() * sizeof(wchar_t));
	}
}
void DxTextFileObject::AddLine(std::wstring line) {
	if (bytePerChar_ == 2) _AddLine((char*)&line[0], line.size() * sizeof(wchar_t));
	else if (bytePerChar_ == 1) {
		std::string str = StringUtility::ConvertWideToMulti(line);
		_AddLine(&str[0], str.size() * sizeof(char));
	}
}

/**********************************************************
//DxBinaryFileObject
**********************************************************/
DxBinaryFileObject::DxBinaryFileObject() {
	typeObject_ = TypeObject::OBJ_FILE_BINARY;
	byteOrder_ = ByteOrder::ENDIAN_LITTLE;
	codePage_ = CP_ACP;
}
DxBinaryFileObject::~DxBinaryFileObject() {}
bool DxBinaryFileObject::OpenR(std::wstring path) {
	bool res = DxFileObject::OpenR(path);
	if (!res)return false;

	size_t size = file_->GetSize();
	buffer_ = new gstd::ByteBuffer();
	buffer_->SetSize(size);

	file_->Read(buffer_->GetPointer(), size);

	return true;
}
bool DxBinaryFileObject::OpenR(gstd::ref_count_ptr<gstd::FileReader> reader) {
	reader_ = reader;

	size_t size = reader->GetFileSize();
	buffer_ = dynamic_cast<ManagedFileReader*>(reader.GetPointer())->GetBuffer();

	return true;
}
bool DxBinaryFileObject::OpenRW(std::wstring path) {
	bool res = DxFileObject::OpenRW(path);
	if (!res) return false;

	size_t size = file_->GetSize();
	buffer_ = new gstd::ByteBuffer();
	buffer_->SetSize(size);

	file_->Read(buffer_->GetPointer(), size);

	return true;
}
bool DxBinaryFileObject::Store() {
	if (isArchived_) return false;
	if (file_ == nullptr) return false;

	file_->SetFilePointerBegin();
	file_->Write(buffer_->GetPointer(), buffer_->GetSize());

	return true;
}
bool DxBinaryFileObject::IsReadableSize(size_t size) {
	size_t pos = buffer_->GetOffset();
	bool res = pos + size <= buffer_->GetSize();
	return res;
}

/**********************************************************
//DxScriptObjectManager
**********************************************************/
DxScriptObjectManager::DxScriptObjectManager() {
	SetMaxObject(DEFAULT_CONTAINER_CAPACITY);
	SetRenderBucketCapacity(101);
	totalObjectCreateCount_ = 0;

	bFogEnable_ = false;
	fogColor_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	fogStart_ = 0;
	fogEnd_ = 0;
}
DxScriptObjectManager::~DxScriptObjectManager() {

}
bool DxScriptObjectManager::SetMaxObject(size_t size) {
	/*
	listUnusedIndex_.clear();
	for (size_t iObj = 0U; iObj < obj_.size(); ++iObj) {
		shared_ptr<DxScriptObjectBase> obj = obj_[iObj];
		if (obj == nullptr) listUnusedIndex_.push_back(iObj);
	}
	*/
	if (obj_.size() == 131072U) return false;
	size = std::min(size, 131072U);

	for (size_t iObj = obj_.size(); iObj < size; ++iObj)
		listUnusedIndex_.push_back(iObj);
	obj_.resize(size, nullptr);
	return true;
}
void DxScriptObjectManager::SetRenderBucketCapacity(size_t capacity) {
	objRender_.resize(capacity);
	listShader_.resize(capacity);
}
void DxScriptObjectManager::_ArrangeActiveObjectList() {
	std::list<shared_ptr<DxScriptObjectBase>>::iterator itr;
	for (itr = listActiveObject_.begin(); itr != listActiveObject_.end();) {
		shared_ptr<DxScriptObjectBase> obj = (*itr);
		if (obj == nullptr || obj->IsDeleted() || !obj->IsActive())
			itr = listActiveObject_.erase(itr);
		else ++itr;
	}
}
int DxScriptObjectManager::AddObject(shared_ptr<DxScriptObjectBase> obj, bool bActivate) {
	int res = DxScript::ID_INVALID;

	auto ExpandContainerCapacity = [&]() -> bool {
		size_t oldSize = obj_.size();
		bool res = SetMaxObject(oldSize * 2U);
		Logger::WriteTop(StringUtility::Format("DxScriptObjectManager: Object pool expansion. [%d->%d]",
			oldSize, obj_.size()));
		return res;
	};

	{
		do {
			if (listUnusedIndex_.size() == 0U) {
				if (!ExpandContainerCapacity()) break;
			}
			res = listUnusedIndex_.front();
			listUnusedIndex_.pop_front();
		} while (obj_[res] != nullptr);

		if (res != DxScript::ID_INVALID) {
			obj_[res] = obj;
			if (bActivate) {
				obj->bActive_ = true;
				listActiveObject_.push_back(obj);
			}
			obj->idObject_ = res;
			obj->manager_ = this;

			++totalObjectCreateCount_;
		}
	}

	return res;
}
void DxScriptObjectManager::ActivateObject(int id, bool bActivate) {
	DxScriptObjectManager::ActivateObject(GetObject(id), bActivate);
}
void DxScriptObjectManager::ActivateObject(shared_ptr<DxScriptObjectBase> obj, bool bActivate) {
	if (obj == nullptr || obj->IsDeleted()) return;

	if (bActivate && !obj->IsActive()) {
		obj->bActive_ = true;
		listActiveObject_.push_back(obj);
	}
	else if (!bActivate) {
		obj->bActive_ = false;
	}
}
std::vector<int> DxScriptObjectManager::GetValidObjectIdentifier() {
	std::vector<int> res;
	for (size_t iObj = 0; iObj < obj_.size(); ++iObj) {
		if (obj_[iObj] == nullptr) continue;
		res.push_back(obj_[iObj]->idObject_);
	}
	return res;
}
DxScriptObjectBase* DxScriptObjectManager::GetObjectPointer(int id) {
	if (id < 0 || id >= obj_.size())return nullptr;
	return obj_[id].get();
}
void DxScriptObjectManager::DeleteObject(int id) {
	if (id < 0 || id >= obj_.size()) return;

	auto ptr = obj_[id];
	if (ptr == nullptr) return;

	ptr->bDeleted_ = true;
	obj_[id] = nullptr;

	listUnusedIndex_.push_back(id);
}
void DxScriptObjectManager::DeleteObject(shared_ptr<DxScriptObjectBase> obj) {
	DxScriptObjectManager::DeleteObject(obj.get());
}
void DxScriptObjectManager::DeleteObject(DxScriptObjectBase* obj) {
	if (obj == nullptr) return;

	obj->bDeleted_ = true;
	obj_[obj->GetObjectID()] = nullptr;

	listUnusedIndex_.push_back(obj->GetObjectID());
}
void DxScriptObjectManager::ClearObject() {
	size_t size = obj_.size();
	obj_.clear();
	obj_.resize(size, nullptr);
	listActiveObject_.clear();

	listUnusedIndex_.clear();
	for (size_t iObj = 0; iObj < size; ++iObj) {
		listUnusedIndex_.push_back(iObj);
	}
}
shared_ptr<Shader> DxScriptObjectManager::GetShader(int index) {
	if (index < 0 || index >= listShader_.size())return nullptr;
	shared_ptr<Shader> shader = listShader_[index];
	return shader;
}
void DxScriptObjectManager::DeleteObjectByScriptID(int64_t idScript) {
	if (idScript == ScriptClientBase::ID_SCRIPT_FREE) return;

	for (size_t iObj = 0; iObj < obj_.size(); ++iObj) {
		if (obj_[iObj] == nullptr) continue;
		if (obj_[iObj]->GetScriptID() != idScript) continue;
		DeleteObject(obj_[iObj]);
	}
}
void DxScriptObjectManager::AddRenderObject(shared_ptr<DxScriptObjectBase> obj) {
	if (!obj->IsVisible())return;

	size_t renderSize = objRender_.size();

	int tPri = obj->priRender_;
	if (tPri < 0) tPri = 0;
	else if (tPri > renderSize - 1) tPri = renderSize - 1;
	objRender_[tPri].push_back(obj);
}
void DxScriptObjectManager::WorkObject() {
	//_ArrangeActiveObjectList();
	std::list<shared_ptr<DxScriptObjectBase>>::iterator itr;

	for (itr = listActiveObject_.begin(); itr != listActiveObject_.end();) {
		shared_ptr<DxScriptObjectBase> obj = *itr;
		if (obj == nullptr || obj->IsDeleted()) {
			itr = listActiveObject_.erase(itr);
			continue;
		}
		obj->Work();
		++itr;
	}

//音声再生
	DirectSoundManager* soundManager = DirectSoundManager::GetBase();
	auto itrSound = mapReservedSound_.begin();
	for (; itrSound != mapReservedSound_.end(); ++itrSound) {
		gstd::ref_count_ptr<SoundInfo> info = itrSound->second;
		gstd::ref_count_ptr<SoundPlayer> player = info->player_;
		SoundPlayer::PlayStyle style = info->style_;
		player->Play(style);
	}
	mapReservedSound_.clear();
}
void DxScriptObjectManager::RenderObject() {
	PrepareRenderObject();

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetVertexFog(bFogEnable_, fogColor_, fogStart_, fogEnd_);

	for (size_t iPri = 0; iPri < objRender_.size(); ++iPri) {
		shared_ptr<Shader> shader = listShader_[iPri];
		ID3DXEffect* effect = nullptr;
		UINT cPass = 1;
		if (shader != nullptr) {
			effect = shader->GetEffect();
			shader->LoadParameter();
			effect->Begin(&cPass, 0);
		}

		std::list<shared_ptr<DxScriptObjectBase>>::iterator itr;

		for (UINT iPass = 0; iPass < cPass; ++iPass) {
			if (effect != nullptr) effect->BeginPass(iPass);

			for (itr = objRender_[iPri].begin(); itr != objRender_[iPri].end(); ++itr) {
				(*itr)->Render();
			}

			if (effect != nullptr) effect->EndPass();
		}

		objRender_[iPri].clear();

		if (effect != nullptr) effect->End();
	}

}
void DxScriptObjectManager::PrepareRenderObject() {
	std::list<shared_ptr<DxScriptObjectBase>>::iterator itr;
	for (itr = listActiveObject_.begin(); itr != listActiveObject_.end(); ++itr) {
		shared_ptr<DxScriptObjectBase> obj = (*itr);
		if (obj == nullptr || obj->IsDeleted())continue;
		if (!obj->IsVisible())continue;
		AddRenderObject(obj);
	}
}
void DxScriptObjectManager::ClearRenderObject() {
//#pragma omp parallel for
	for (size_t iPri = 0; iPri < objRender_.size(); ++iPri) {
		objRender_[iPri].clear();
	}
}
void DxScriptObjectManager::SetShader(shared_ptr<Shader> shader, int min, int max) {
	for (int iPri = min; iPri <= max; ++iPri) {
		if (iPri < 0 || iPri >= listShader_.size()) continue;
		listShader_[iPri] = shader;
	}
}
void DxScriptObjectManager::ResetShader() {
	ResetShader(0, listShader_.size());
}
void DxScriptObjectManager::ResetShader(int min, int max) {
	SetShader(nullptr, min, max);
}

void DxScriptObjectManager::ReserveSound(gstd::ref_count_ptr<SoundPlayer> player, SoundPlayer::PlayStyle& style) {
	ref_count_ptr<SoundInfo> info = new SoundInfo();
	info->player_ = player;
	info->style_ = style;

	std::wstring path = player->GetPath();
	mapReservedSound_[path] = info;
}
void DxScriptObjectManager::DeleteReservedSound(gstd::ref_count_ptr<SoundPlayer> player) {
	std::wstring path = player->GetPath();
	mapReservedSound_.erase(path);
}
void DxScriptObjectManager::SetFogParam(bool bEnable, D3DCOLOR fogColor, float start, float end) {
	bFogEnable_ = bEnable;
	fogColor_ = fogColor;
	fogStart_ = start;
	fogEnd_ = end;
}

/**********************************************************
//DxScript
**********************************************************/
function const dxFunction[] =
{
	//Dx関数：システム系系
	{ "InstallFont", DxScript::Func_InstallFont, 1 },

	{ "MatrixIdentity", DxScript::Func_MatrixIdentity, 0 },
	{ "MatrixInverse", DxScript::Func_MatrixInverse, 1 },
	{ "MatrixAdd", DxScript::Func_MatrixAdd, 2 },
	{ "MatrixSubtract", DxScript::Func_MatrixSubtract, 2 },
	{ "MatrixMultiply", DxScript::Func_MatrixMultiply, 2 },
	{ "MatrixDivide", DxScript::Func_MatrixDivide, 2 },
	{ "MatrixTranspose", DxScript::Func_MatrixTranspose, 1 },
	{ "MatrixDeterminant", DxScript::Func_MatrixDeterminant, 1 },
	{ "MatrixLookAtLH", DxScript::Func_MatrixLookatLH, 3 },
	{ "MatrixLookAtRH", DxScript::Func_MatrixLookatRH, 3 },

	//Dx関数：音声系
	{ "LoadSound", DxScript::Func_LoadSound, 1 },
	{ "RemoveSound", DxScript::Func_RemoveSound, 1 },
	{ "PlayBGM", DxScript::Func_PlayBGM, 3 },
	{ "PlaySE", DxScript::Func_PlaySE, 1 },
	{ "StopSound", DxScript::Func_StopSound, 1 },
	{ "SetSoundDivisionVolumeRate", DxScript::Func_SetSoundDivisionVolumeRate, 2 },
	{ "GetSoundDivisionVolumeRate", DxScript::Func_GetSoundDivisionVolumeRate, 1 },

	//Dx関数：キー系
	{ "GetKeyState", DxScript::Func_GetKeyState, 1 },
	{ "GetMouseX", DxScript::Func_GetMouseX, 0 },
	{ "GetMouseY", DxScript::Func_GetMouseY, 0 },
	{ "GetMouseMoveZ", DxScript::Func_GetMouseMoveZ, 0 },
	{ "GetMouseState", DxScript::Func_GetMouseState, 1 },
	{ "GetVirtualKeyState", DxScript::Func_GetVirtualKeyState, 1 },
	{ "SetVirtualKeyState", DxScript::Func_SetVirtualKeyState, 2 },

	//Dx関数：描画系
	{ "GetScreenWidth", DxScript::Func_GetScreenWidth, 0 },
	{ "GetScreenHeight", DxScript::Func_GetScreenHeight, 0 },
	{ "LoadTexture", DxScript::Func_LoadTexture, 1 },
	{ "LoadTextureEx", DxScript::Func_LoadTextureEx, 3 },
	{ "LoadTextureInLoadThread", DxScript::Func_LoadTextureInLoadThread, 1 },
	{ "LoadTextureInLoadThreadEx", DxScript::Func_LoadTextureInLoadThreadEx, 3 },
	{ "RemoveTexture", DxScript::Func_RemoveTexture, 1 },
	{ "GetTextureWidth", DxScript::Func_GetTextureWidth, 1 },
	{ "GetTextureHeight", DxScript::Func_GetTextureHeight, 1 },
	{ "SetFogEnable", DxScript::Func_SetFogEnable, 1 },
	{ "SetFogParam", DxScript::Func_SetFogParam, 5 },
	{ "CreateRenderTarget", DxScript::Func_CreateRenderTarget, 1 },
	{ "CreateRenderTargetEx", DxScript::Func_CreateRenderTargetEx, 3 },
	//{ "SetRenderTarget", DxScript::Func_SetRenderTarget, 2 },
	//{ "ResetRenderTarget", DxScript::Func_ResetRenderTarget, 0 },
	{ "ClearRenderTargetA1", DxScript::Func_ClearRenderTargetA1, 1 },
	{ "ClearRenderTargetA2", DxScript::Func_ClearRenderTargetA2, 5 },
	{ "ClearRenderTargetA3", DxScript::Func_ClearRenderTargetA3, 9 },
	{ "GetTransitionRenderTargetName", DxScript::Func_GetTransitionRenderTargetName, 0 },
	{ "SaveRenderedTextureA1", DxScript::Func_SaveRenderedTextureA1, 2 },
	{ "SaveRenderedTextureA2", DxScript::Func_SaveRenderedTextureA2, 6 },
	{ "SaveRenderedTextureA3", DxScript::Func_SaveRenderedTextureA3, 7 },
	{ "IsPixelShaderSupported", DxScript::Func_IsPixelShaderSupported, 2 },
	//{ "SetAntiAliasing", DxScript::Func_SetEnableAntiAliasing, 1 },
	{ "SetShader", DxScript::Func_SetShader, 3 },
	{ "SetShaderI", DxScript::Func_SetShaderI, 3 },
	{ "ResetShader", DxScript::Func_ResetShader, 2 },
	{ "ResetShaderI", DxScript::Func_ResetShaderI, 2 },

	//Dx関数：カメラ3D
	{ "SetCameraFocusX", DxScript::Func_SetCameraFocusX, 1 },
	{ "SetCameraFocusY", DxScript::Func_SetCameraFocusY, 1 },
	{ "SetCameraFocusZ", DxScript::Func_SetCameraFocusZ, 1 },
	{ "SetCameraFocusXYZ", DxScript::Func_SetCameraFocusXYZ, 3 },
	{ "SetCameraRadius", DxScript::Func_SetCameraRadius, 1 },
	{ "SetCameraAzimuthAngle", DxScript::Func_SetCameraAzimuthAngle, 1 },
	{ "SetCameraElevationAngle", DxScript::Func_SetCameraElevationAngle, 1 },
	{ "SetCameraYaw", DxScript::Func_SetCameraYaw, 1 },
	{ "SetCameraPitch", DxScript::Func_SetCameraPitch, 1 },
	{ "SetCameraRoll", DxScript::Func_SetCameraRoll, 1 },
	{ "SetCameraMode", DxScript::Func_SetCameraMode, 1 },
	//{ "SetCameraPosEye", DxScript::Func_SetCameraPosEye, 3 },
	{ "SetCameraPosEye", DxScript::Func_SetCameraFocusXYZ, 3 },
	{ "SetCameraPosLookAt", DxScript::Func_SetCameraPosLookAt, 3 },
	{ "GetCameraX", DxScript::Func_GetCameraX, 0 },
	{ "GetCameraY", DxScript::Func_GetCameraY, 0 },
	{ "GetCameraZ", DxScript::Func_GetCameraZ, 0 },
	{ "GetCameraFocusX", DxScript::Func_GetCameraFocusX, 0 },
	{ "GetCameraFocusY", DxScript::Func_GetCameraFocusY, 0 },
	{ "GetCameraFocusZ", DxScript::Func_GetCameraFocusZ, 0 },
	{ "GetCameraRadius", DxScript::Func_GetCameraRadius, 0 },
	{ "GetCameraAzimuthAngle", DxScript::Func_GetCameraAzimuthAngle, 0 },
	{ "GetCameraElevationAngle", DxScript::Func_GetCameraElevationAngle, 0 },
	{ "GetCameraYaw", DxScript::Func_GetCameraYaw, 0 },
	{ "GetCameraPitch", DxScript::Func_GetCameraPitch, 0 },
	{ "GetCameraRoll", DxScript::Func_GetCameraRoll, 0 },
	{ "GetCameraViewProjectionMatrix", DxScript::Func_GetCameraViewProjectionMatrix, 0 },
	{ "SetCameraPerspectiveClip", DxScript::Func_SetCameraPerspectiveClip, 2 },

	//Dx関数：カメラ2D
	{ "Set2DCameraFocusX", DxScript::Func_Set2DCameraFocusX, 1 },
	{ "Set2DCameraFocusY", DxScript::Func_Set2DCameraFocusY, 1 },
	{ "Set2DCameraAngleZ", DxScript::Func_Set2DCameraAngleZ, 1 },
	{ "Set2DCameraRatio", DxScript::Func_Set2DCameraRatio, 1 },
	{ "Set2DCameraRatioX", DxScript::Func_Set2DCameraRatioX, 1 },
	{ "Set2DCameraRatioY", DxScript::Func_Set2DCameraRatioY, 1 },
	{ "Reset2DCamera", DxScript::Func_Reset2DCamera, 0 },
	{ "Get2DCameraX", DxScript::Func_Get2DCameraX, 0 },
	{ "Get2DCameraY", DxScript::Func_Get2DCameraY, 0 },
	{ "Get2DCameraAngleZ", DxScript::Func_Get2DCameraAngleZ, 0 },
	{ "Get2DCameraRatio", DxScript::Func_Get2DCameraRatio, 0 },
	{ "Get2DCameraRatioX", DxScript::Func_Get2DCameraRatioX, 0 },
	{ "Get2DCameraRatioY", DxScript::Func_Get2DCameraRatioY, 0 },

	//Dx関数：その他
	{ "GetObjectDistance", DxScript::Func_GetObjectDistance, 2 },
	{ "GetObject2dPosition", DxScript::Func_GetObject2dPosition, 1 },
	{ "Get2dPosition", DxScript::Func_Get2dPosition, 3 },

	{ "IsIntersected_Circle_Circle", DxScript::Func_IsIntersected_Circle_Circle, 6 },
	{ "IsIntersected_Line_Circle", DxScript::Func_IsIntersected_Line_Circle, 8 },
	{ "IsIntersected_Line_Line", DxScript::Func_IsIntersected_Line_Line, 10 },

	//Dx関数：オブジェクト操作(共通)
	{ "Obj_Delete", DxScript::Func_Obj_Delete, 1 },
	{ "Obj_IsDeleted", DxScript::Func_Obj_IsDeleted, 1 },
	{ "Obj_SetVisible", DxScript::Func_Obj_SetVisible, 2 },
	{ "Obj_IsVisible", DxScript::Func_Obj_IsVisible, 1 },
	{ "Obj_SetRenderPriority", DxScript::Func_Obj_SetRenderPriority, 2 },
	{ "Obj_SetRenderPriorityI", DxScript::Func_Obj_SetRenderPriorityI, 2 },
	{ "Obj_GetRenderPriority", DxScript::Func_Obj_GetRenderPriority, 1 },
	{ "Obj_GetRenderPriorityI", DxScript::Func_Obj_GetRenderPriorityI, 1 },
	{ "Obj_GetValue", DxScript::Func_Obj_GetValue, 2 },
	{ "Obj_GetValueD", DxScript::Func_Obj_GetValueD, 3 },
	{ "Obj_SetValue", DxScript::Func_Obj_SetValue, 3 },
	{ "Obj_DeleteValue", DxScript::Func_Obj_DeleteValue, 2 },
	{ "Obj_IsValueExists", DxScript::Func_Obj_IsValueExists, 2 },
	{ "Obj_GetValueR", DxScript::Func_Obj_GetValueR, 2 },
	{ "Obj_GetValueDR", DxScript::Func_Obj_GetValueDR, 3 },
	{ "Obj_SetValueR", DxScript::Func_Obj_SetValueR, 3 },
	{ "Obj_DeleteValueR", DxScript::Func_Obj_DeleteValueR, 2 },
	{ "Obj_IsValueExistsR", DxScript::Func_Obj_IsValueExistsR, 2 },
	{ "Obj_GetType", DxScript::Func_Obj_GetType, 1 },

	//Dx関数：オブジェクト操作(RenderObject)
	{ "ObjRender_SetX", DxScript::Func_ObjRender_SetX, 2 },
	{ "ObjRender_SetY", DxScript::Func_ObjRender_SetY, 2 },
	{ "ObjRender_SetZ", DxScript::Func_ObjRender_SetZ, 2 },
	{ "ObjRender_SetPosition", DxScript::Func_ObjRender_SetPosition, 4 },
	{ "ObjRender_SetAngleX", DxScript::Func_ObjRender_SetAngleX, 2 },
	{ "ObjRender_SetAngleY", DxScript::Func_ObjRender_SetAngleY, 2 },
	{ "ObjRender_SetAngleZ", DxScript::Func_ObjRender_SetAngleZ, 2 },
	{ "ObjRender_SetAngleXYZ", DxScript::Func_ObjRender_SetAngleXYZ, 4 },
	{ "ObjRender_SetScaleX", DxScript::Func_ObjRender_SetScaleX, 2 },
	{ "ObjRender_SetScaleY", DxScript::Func_ObjRender_SetScaleY, 2 },
	{ "ObjRender_SetScaleZ", DxScript::Func_ObjRender_SetScaleZ, 2 },
	{ "ObjRender_SetScaleXYZ", DxScript::Func_ObjRender_SetScaleXYZ, 4 },
	{ "ObjRender_SetColor", DxScript::Func_ObjRender_SetColor, 4 },
	{ "ObjRender_SetColorHSV", DxScript::Func_ObjRender_SetColorHSV, 4 },
	{ "ObjRender_GetColor", DxScript::Func_ObjRender_GetColor, 1 },
	{ "ObjRender_SetAlpha", DxScript::Func_ObjRender_SetAlpha, 2 },
	{ "ObjRender_GetAlpha", DxScript::Func_ObjRender_GetAlpha, 1 },
	{ "ObjRender_SetBlendType", DxScript::Func_ObjRender_SetBlendType, 2 },
	{ "ObjRender_GetX", DxScript::Func_ObjRender_GetX, 1 },
	{ "ObjRender_GetY", DxScript::Func_ObjRender_GetY, 1 },
	{ "ObjRender_GetZ", DxScript::Func_ObjRender_GetZ, 1 },
	{ "ObjRender_GetAngleX", DxScript::Func_ObjRender_GetAngleX, 1 },
	{ "ObjRender_GetAngleY", DxScript::Func_ObjRender_GetAngleY, 1 },
	{ "ObjRender_GetAngleZ", DxScript::Func_ObjRender_GetAngleZ, 1 },
	{ "ObjRender_GetScaleX", DxScript::Func_ObjRender_GetScaleX, 1 },
	{ "ObjRender_GetScaleY", DxScript::Func_ObjRender_GetScaleY, 1 },
	{ "ObjRender_GetScaleZ", DxScript::Func_ObjRender_GetScaleZ, 1 },
	{ "ObjRender_SetZWrite", DxScript::Func_ObjRender_SetZWrite, 2 },
	{ "ObjRender_SetZTest", DxScript::Func_ObjRender_SetZTest, 2 },
	{ "ObjRender_SetFogEnable", DxScript::Func_ObjRender_SetFogEnable, 2 },
	{ "ObjRender_SetCullingMode", DxScript::Func_ObjRender_SetCullingMode, 2 },
	{ "ObjRender_SetRelativeObject", DxScript::Func_ObjRender_SetRalativeObject, 3 },
	{ "ObjRender_SetPermitCamera", DxScript::Func_ObjRender_SetPermitCamera, 2 },
	{ "ObjRender_GetBlendType", DxScript::Func_ObjRender_GetBlendType, 1 },
	{ "ObjRender_SetTextureFilterMin", DxScript::Func_ObjRender_SetTextureFilterMin, 2 },
	{ "ObjRender_SetTextureFilterMag", DxScript::Func_ObjRender_SetTextureFilterMag, 2 },
	{ "ObjRender_SetTextureFilterMip", DxScript::Func_ObjRender_SetTextureFilterMip, 2 },
	{ "ObjRender_SetVertexShaderRenderingMode", DxScript::Func_ObjRender_SetVertexShaderRenderingMode, 2 },

	//Dx関数：オブジェクト操作(ShaderObject)
	{ "ObjShader_Create", DxScript::Func_ObjShader_Create, 0 },
	{ "ObjShader_SetShaderF", DxScript::Func_ObjShader_SetShaderF, 2 },
	{ "ObjShader_SetShaderO", DxScript::Func_ObjShader_SetShaderO, 2 },
	{ "ObjShader_ResetShader", DxScript::Func_ObjShader_ResetShader, 1 },
	{ "ObjShader_SetTechnique", DxScript::Func_ObjShader_SetTechnique, 2 },
	{ "ObjShader_SetMatrix", DxScript::Func_ObjShader_SetMatrix, 3 },
	{ "ObjShader_SetMatrixArray", DxScript::Func_ObjShader_SetMatrixArray, 3 },
	{ "ObjShader_SetVector", DxScript::Func_ObjShader_SetVector, 6 },
	{ "ObjShader_SetFloat", DxScript::Func_ObjShader_SetFloat, 3 },
	{ "ObjShader_SetFloatArray", DxScript::Func_ObjShader_SetFloatArray, 3 },
	{ "ObjShader_SetTexture", DxScript::Func_ObjShader_SetTexture, 3 },

	//Dx関数：オブジェクト操作(PrimitiveObject)
	{ "ObjPrim_Create", DxScript::Func_ObjPrimitive_Create, 1 },
	{ "ObjPrim_SetPrimitiveType", DxScript::Func_ObjPrimitive_SetPrimitiveType, 2 },
	{ "ObjPrim_GetPrimitiveType", DxScript::Func_ObjPrimitive_GetPrimitiveType, 1 },
	{ "ObjPrim_SetVertexCount", DxScript::Func_ObjPrimitive_SetVertexCount, 2 },
	{ "ObjPrim_SetTexture", DxScript::Func_ObjPrimitive_SetTexture, 2 },
	{ "ObjPrim_GetVertexCount", DxScript::Func_ObjPrimitive_GetVertexCount, 1 },
	{ "ObjPrim_SetVertexPosition", DxScript::Func_ObjPrimitive_SetVertexPosition, 5 },
	{ "ObjPrim_SetVertexUV", DxScript::Func_ObjPrimitive_SetVertexUV, 4 },
	{ "ObjPrim_SetVertexUVT", DxScript::Func_ObjPrimitive_SetVertexUVT, 4 },
	{ "ObjPrim_SetVertexColor", DxScript::Func_ObjPrimitive_SetVertexColor, 5 },
	{ "ObjPrim_SetVertexAlpha", DxScript::Func_ObjPrimitive_SetVertexAlpha, 3 },
	{ "ObjPrim_GetVertexPosition", DxScript::Func_ObjPrimitive_GetVertexPosition, 2 },
	{ "ObjPrim_SetVertexIndex", DxScript::Func_ObjPrimitive_SetVertexIndex, 2 },

	//Dx関数：オブジェクト操作(Sprite2D)
	{ "ObjSprite2D_SetSourceRect", DxScript::Func_ObjSprite2D_SetSourceRect, 5 },
	{ "ObjSprite2D_SetDestRect", DxScript::Func_ObjSprite2D_SetDestRect, 5 },
	{ "ObjSprite2D_SetDestCenter", DxScript::Func_ObjSprite2D_SetDestCenter, 1 },

	//Dx関数：オブジェクト操作(SpriteList2D)
	{ "ObjSpriteList2D_SetSourceRect", DxScript::Func_ObjSpriteList2D_SetSourceRect, 5 },
	{ "ObjSpriteList2D_SetDestRect", DxScript::Func_ObjSpriteList2D_SetDestRect, 5 },
	{ "ObjSpriteList2D_SetDestCenter", DxScript::Func_ObjSpriteList2D_SetDestCenter, 1 },
	{ "ObjSpriteList2D_AddVertex", DxScript::Func_ObjSpriteList2D_AddVertex, 1 },
	{ "ObjSpriteList2D_CloseVertex", DxScript::Func_ObjSpriteList2D_CloseVertex, 1 },
	{ "ObjSpriteList2D_ClearVertexCount", DxScript::Func_ObjSpriteList2D_ClearVertexCount, 1 },
	{ "ObjSpriteList2D_SetAutoClearVertexCount", DxScript::Func_ObjSpriteList2D_SetAutoClearVertexCount, 2 },

	//Dx関数：オブジェクト操作(Sprite3D)
	{ "ObjSprite3D_SetSourceRect", DxScript::Func_ObjSprite3D_SetSourceRect, 5 },
	{ "ObjSprite3D_SetDestRect", DxScript::Func_ObjSprite3D_SetDestRect, 5 },
	{ "ObjSprite3D_SetSourceDestRect", DxScript::Func_ObjSprite3D_SetSourceDestRect, 5 },
	{ "ObjSprite3D_SetBillboard", DxScript::Func_ObjSprite3D_SetBillboard, 2 },

	//Dx関数：オブジェクト操作(TrajectoryObject3D)
	{ "ObjTrajectory3D_SetInitialPoint", DxScript::Func_ObjTrajectory3D_SetInitialPoint, 7 },
	{ "ObjTrajectory3D_SetAlphaVariation", DxScript::Func_ObjTrajectory3D_SetAlphaVariation, 2 },
	{ "ObjTrajectory3D_SetComplementCount", DxScript::Func_ObjTrajectory3D_SetComplementCount, 2 },

	//DxScriptParticleListObject
	{ "ObjParticleList_Create", DxScript::Func_ObjParticleList_Create, 1 },
	{ "ObjParticleList_SetPosition", DxScript::Func_ObjParticleList_SetPosition, 4 },
	{ "ObjParticleList_SetScale", DxScript::Func_ObjParticleList_SetScale, 4 },
	{ "ObjParticleList_SetAngleX", DxScript::Func_ObjParticleList_SetAngleSingle<0>, 2 },
	{ "ObjParticleList_SetAngleY", DxScript::Func_ObjParticleList_SetAngleSingle<1>, 2 },
	{ "ObjParticleList_SetAngleZ", DxScript::Func_ObjParticleList_SetAngleSingle<2>, 2 },
	{ "ObjParticleList_SetAngle", DxScript::Func_ObjParticleList_SetAngle, 4 },
	{ "ObjParticleList_SetColor", DxScript::Func_ObjParticleList_SetColor, 4 },
	{ "ObjParticleList_SetAlpha", DxScript::Func_ObjParticleList_SetAlpha, 2 },
	{ "ObjParticleList_SetExtraData", DxScript::Func_ObjParticleList_SetExtraData, 4 },
	{ "ObjParticleList_AddInstance", DxScript::Func_ObjParticleList_AddInstance, 1 },
	{ "ObjParticleList_ClearInstance", DxScript::Func_ObjParticleList_ClearInstance, 1 },

	//Dx関数：オブジェクト操作(DxMesh)
	{ "ObjMesh_Create", DxScript::Func_ObjMesh_Create, 0 },
	{ "ObjMesh_Load", DxScript::Func_ObjMesh_Load, 2 },
	{ "ObjMesh_SetColor", DxScript::Func_ObjMesh_SetColor, 4 },
	{ "ObjMesh_SetAlpha", DxScript::Func_ObjMesh_SetAlpha, 2 },
	{ "ObjMesh_SetAnimation", DxScript::Func_ObjMesh_SetAnimation, 3 },
	{ "ObjMesh_SetCoordinate2D", DxScript::Func_ObjMesh_SetCoordinate2D, 2 },
	{ "ObjMesh_GetPath", DxScript::Func_ObjMesh_GetPath, 1 },

	//Dx関数：テキスト操作(DxText)
	{ "ObjText_Create", DxScript::Func_ObjText_Create, 0 },
	{ "ObjText_SetText", DxScript::Func_ObjText_SetText, 2 },
	{ "ObjText_SetFontType", DxScript::Func_ObjText_SetFontType, 2 },
	{ "ObjText_SetFontSize", DxScript::Func_ObjText_SetFontSize, 2 },
	{ "ObjText_SetFontBold", DxScript::Func_ObjText_SetFontBold, 2 },
	{ "ObjText_SetFontWeight", DxScript::Func_ObjText_SetFontWeight, 2 },
	{ "ObjText_SetFontColorTop", DxScript::Func_ObjText_SetFontColorTop, 4 },
	{ "ObjText_SetFontColorBottom", DxScript::Func_ObjText_SetFontColorBottom, 4 },
	{ "ObjText_SetFontBorderWidth", DxScript::Func_ObjText_SetFontBorderWidth, 2 },
	{ "ObjText_SetFontBorderType", DxScript::Func_ObjText_SetFontBorderType, 2 },
	{ "ObjText_SetFontBorderColor", DxScript::Func_ObjText_SetFontBorderColor, 4 },
	{ "ObjText_SetFontCharacterSet", DxScript::Func_ObjText_SetFontCharacterSet, 2 },
	{ "ObjText_SetMaxWidth", DxScript::Func_ObjText_SetMaxWidth, 2 },
	{ "ObjText_SetMaxHeight", DxScript::Func_ObjText_SetMaxHeight, 2 },
	{ "ObjText_SetLinePitch", DxScript::Func_ObjText_SetLinePitch, 2 },
	{ "ObjText_SetSidePitch", DxScript::Func_ObjText_SetSidePitch, 2 },
	{ "ObjText_SetVertexColor", DxScript::Func_ObjText_SetVertexColor, 5 },
	{ "ObjText_SetTransCenter", DxScript::Func_ObjText_SetTransCenter, 3 },
	{ "ObjText_SetAutoTransCenter", DxScript::Func_ObjText_SetAutoTransCenter, 2 },
	{ "ObjText_SetHorizontalAlignment", DxScript::Func_ObjText_SetHorizontalAlignment, 2 },
	{ "ObjText_SetSyntacticAnalysis", DxScript::Func_ObjText_SetSyntacticAnalysis, 2 },
	{ "ObjText_GetTextLength", DxScript::Func_ObjText_GetTextLength, 1 },
	{ "ObjText_GetTextLengthCU", DxScript::Func_ObjText_GetTextLengthCU, 1 },
	{ "ObjText_GetTextLengthCUL", DxScript::Func_ObjText_GetTextLengthCUL, 1 },
	{ "ObjText_GetTotalWidth", DxScript::Func_ObjText_GetTotalWidth, 1 },
	{ "ObjText_GetTotalHeight", DxScript::Func_ObjText_GetTotalHeight, 1 },

	//Dx関数：音声操作(DxSoundObject)
	{ "ObjSound_Create", DxScript::Func_ObjSound_Create, 0 },
	{ "ObjSound_Load", DxScript::Func_ObjSound_Load, 2 },
	{ "ObjSound_Play", DxScript::Func_ObjSound_Play, 1 },
	{ "ObjSound_Stop", DxScript::Func_ObjSound_Stop, 1 },
	{ "ObjSound_SetVolumeRate", DxScript::Func_ObjSound_SetVolumeRate, 2 },
	{ "ObjSound_SetPanRate", DxScript::Func_ObjSound_SetPanRate, 2 },
	{ "ObjSound_SetFade", DxScript::Func_ObjSound_SetFade, 2 },
	{ "ObjSound_SetLoopEnable", DxScript::Func_ObjSound_SetLoopEnable, 2 },
	{ "ObjSound_SetLoopTime", DxScript::Func_ObjSound_SetLoopTime, 3 },
	{ "ObjSound_SetLoopSampleCount", DxScript::Func_ObjSound_SetLoopSampleCount, 3 },
	{ "ObjSound_Seek", DxScript::Func_ObjSound_Seek, 2 },
	{ "ObjSound_SeekSampleCount", DxScript::Func_ObjSound_SeekSampleCount, 2 },
	{ "ObjSound_SetRestartEnable", DxScript::Func_ObjSound_SetRestartEnable, 2 },
	{ "ObjSound_SetSoundDivision", DxScript::Func_ObjSound_SetSoundDivision, 2 },
	{ "ObjSound_IsPlaying", DxScript::Func_ObjSound_IsPlaying, 1 },
	{ "ObjSound_GetVolumeRate", DxScript::Func_ObjSound_GetVolumeRate, 1 },
	{ "ObjSound_GetWavePosition", DxScript::Func_ObjSound_GetWavePosition, 1 },
	{ "ObjSound_GetWavePositionSampleCount", DxScript::Func_ObjSound_GetWavePositionSampleCount, 1 },
	{ "ObjSound_GetTotalLength", DxScript::Func_ObjSound_GetTotalLength, 1 },
	{ "ObjSound_GetTotalLengthSampleCount", DxScript::Func_ObjSound_GetTotalLengthSampleCount, 1 },

	//Dx関数：ファイル操作(DxFileObject)
	{ "ObjFile_Create", DxScript::Func_ObjFile_Create, 1 },
	{ "ObjFile_Open", DxScript::Func_ObjFile_Open, 2 },
	{ "ObjFile_OpenNW", DxScript::Func_ObjFile_OpenNW, 2 },
	{ "ObjFile_Store", DxScript::Func_ObjFile_Store, 1 },
	{ "ObjFile_GetSize", DxScript::Func_ObjFile_GetSize, 1 },

	//Dx関数：ファイル操作(DxTextFileObject)
	{ "ObjFileT_GetLineCount", DxScript::Func_ObjFileT_GetLineCount, 1 },
	{ "ObjFileT_GetLineText", DxScript::Func_ObjFileT_GetLineText, 2 },
	{ "ObjFileT_SetLineText", DxScript::Func_ObjFileT_SetLineText, 3 },
	{ "ObjFileT_SplitLineText", DxScript::Func_ObjFileT_SplitLineText, 3 },
	{ "ObjFileT_AddLine", DxScript::Func_ObjFileT_AddLine, 2 },
	{ "ObjFileT_ClearLine", DxScript::Func_ObjFileT_ClearLine, 1 },

	////Dx関数：ファイル操作(DxBinalyFileObject)
	{ "ObjFileB_SetByteOrder", DxScript::Func_ObjFileB_SetByteOrder, 2 },
	{ "ObjFileB_SetCharacterCode", DxScript::Func_ObjFileB_SetCharacterCode, 2 },
	{ "ObjFileB_GetPointer", DxScript::Func_ObjFileB_GetPointer, 1 },
	{ "ObjFileB_Seek", DxScript::Func_ObjFileB_Seek, 2 },
	{ "ObjFileB_ReadBoolean", DxScript::Func_ObjFileB_ReadBoolean, 1 },
	{ "ObjFileB_ReadByte", DxScript::Func_ObjFileB_ReadByte, 1 },
	{ "ObjFileB_ReadShort", DxScript::Func_ObjFileB_ReadShort, 1 },
	{ "ObjFileB_ReadInteger", DxScript::Func_ObjFileB_ReadInteger, 1 },
	{ "ObjFileB_ReadLong", DxScript::Func_ObjFileB_ReadLong, 1 },
	{ "ObjFileB_ReadFloat", DxScript::Func_ObjFileB_ReadFloat, 1 },
	{ "ObjFileB_ReadDouble", DxScript::Func_ObjFileB_ReadDouble, 1 },
	{ "ObjFileB_ReadString", DxScript::Func_ObjFileB_ReadString, 2 },
	{ "ObjFileB_WriteBoolean", DxScript::Func_ObjFileB_WriteBoolean, 2 },
	{ "ObjFileB_WriteByte", DxScript::Func_ObjFileB_WriteByte, 2 },
	{ "ObjFileB_WriteShort", DxScript::Func_ObjFileB_WriteShort, 2 },
	{ "ObjFileB_WriteInteger", DxScript::Func_ObjFileB_WriteInteger, 2 },
	{ "ObjFileB_WriteLong", DxScript::Func_ObjFileB_WriteLong, 2 },
	{ "ObjFileB_WriteFloat", DxScript::Func_ObjFileB_WriteFloat, 2 },
	{ "ObjFileB_WriteDouble", DxScript::Func_ObjFileB_WriteDouble, 2 },

	//定数
	{ "ID_INVALID", constant<DxScript::ID_INVALID>::func, 0 },
	{ "OBJ_PRIMITIVE_2D", constant<(int)TypeObject::OBJ_PRIMITIVE_2D>::func, 0 },
	{ "OBJ_SPRITE_2D", constant<(int)TypeObject::OBJ_SPRITE_2D>::func, 0 },
	{ "OBJ_SPRITE_LIST_2D", constant<(int)TypeObject::OBJ_SPRITE_LIST_2D>::func, 0 },
	{ "OBJ_PRIMITIVE_3D", constant<(int)TypeObject::OBJ_PRIMITIVE_3D>::func, 0 },
	{ "OBJ_SPRITE_3D", constant<(int)TypeObject::OBJ_SPRITE_3D>::func, 0 },
	{ "OBJ_TRAJECTORY_3D", constant<(int)TypeObject::OBJ_TRAJECTORY_3D>::func, 0 },
	{ "OBJ_PARTICLE_LIST_2D", constant<(int)TypeObject::OBJ_PARTICLE_LIST_2D>::func, 0 },
	{ "OBJ_PARTICLE_LIST_3D", constant<(int)TypeObject::OBJ_PARTICLE_LIST_3D>::func, 0 },
	{ "OBJ_SHADER", constant<(int)TypeObject::OBJ_SHADER>::func, 0 },
	{ "OBJ_MESH", constant<(int)TypeObject::OBJ_MESH>::func, 0 },
	{ "OBJ_TEXT", constant<(int)TypeObject::OBJ_TEXT>::func, 0 },
	{ "OBJ_SOUND", constant<(int)TypeObject::OBJ_SOUND>::func, 0 },
	{ "OBJ_FILE_TEXT", constant<(int)TypeObject::OBJ_FILE_TEXT>::func, 0 },
	{ "OBJ_FILE_BINARY", constant<(int)TypeObject::OBJ_FILE_BINARY>::func, 0 },

	{ "BLEND_NONE", constant<DirectGraphics::MODE_BLEND_NONE>::func, 0 },
	{ "BLEND_ALPHA", constant<DirectGraphics::MODE_BLEND_ALPHA>::func, 0 },
	{ "BLEND_ADD_RGB", constant<DirectGraphics::MODE_BLEND_ADD_RGB>::func, 0 },
	{ "BLEND_ADD_ARGB", constant<DirectGraphics::MODE_BLEND_ADD_ARGB>::func, 0 },
	{ "BLEND_MULTIPLY", constant<DirectGraphics::MODE_BLEND_MULTIPLY>::func, 0 },
	{ "BLEND_SUBTRACT", constant<DirectGraphics::MODE_BLEND_SUBTRACT>::func, 0 },
	{ "BLEND_SHADOW", constant<DirectGraphics::MODE_BLEND_SHADOW>::func, 0 },
	{ "BLEND_INV_DESTRGB", constant<DirectGraphics::MODE_BLEND_INV_DESTRGB>::func, 0 },
	{ "BLEND_ALPHA_INV", constant<DirectGraphics::MODE_BLEND_ALPHA_INV>::func, 0 },

	{ "CULL_NONE", constant<D3DCULL_NONE>::func, 0 },
	{ "CULL_CW", constant<D3DCULL_CW>::func, 0 },
	{ "CULL_CCW", constant<D3DCULL_CCW>::func, 0 },

	{ "IFF_BMP", constant<D3DXIFF_BMP>::func, 0 },
	{ "IFF_JPG", constant<D3DXIFF_JPG>::func, 0 },
	{ "IFF_TGA", constant<D3DXIFF_TGA>::func, 0 },
	{ "IFF_PNG", constant<D3DXIFF_PNG>::func, 0 },
	{ "IFF_DDS", constant<D3DXIFF_DDS>::func, 0 },
	{ "IFF_PPM", constant<D3DXIFF_PPM>::func, 0 },
	//	{"IFF_DIB",constant<D3DXIFF_DIB>::func,0},
	//	{"IFF_HDR",constant<D3DXIFF_HDR>::func,0},
	//	{"IFF_PFM",constant<D3DXIFF_PFM>::func,0},

	{ "FILTER_NONE", constant<D3DTEXF_NONE>::func, 0 },
	{ "FILTER_POINT", constant<D3DTEXF_POINT>::func, 0 },
	{ "FILTER_LINEAR", constant<D3DTEXF_LINEAR>::func, 0 },
	{ "FILTER_ANISOTROPIC", constant<D3DTEXF_ANISOTROPIC>::func, 0 },

	{ "CAMERA_NORMAL", constant<DxCamera::MODE_NORMAL>::func, 0 },
	{ "CAMERA_LOOKAT", constant<DxCamera::MODE_LOOKAT>::func, 0 },

	{ "PRIMITIVE_POINT_LIST", constant<D3DPT_POINTLIST>::func, 0 },
	{ "PRIMITIVE_LINELIST", constant<D3DPT_LINELIST>::func, 0 },
	{ "PRIMITIVE_LINESTRIP", constant<D3DPT_LINESTRIP>::func, 0 },
	{ "PRIMITIVE_TRIANGLELIST", constant<D3DPT_TRIANGLELIST>::func, 0 },
	{ "PRIMITIVE_TRIANGLESTRIP", constant<D3DPT_TRIANGLESTRIP>::func, 0 },
	{ "PRIMITIVE_TRIANGLEFAN", constant<D3DPT_TRIANGLEFAN>::func, 0 },

	{ "BORDER_NONE", constant<DxFont::BORDER_NONE>::func, 0 },
	{ "BORDER_FULL", constant<DxFont::BORDER_FULL>::func, 0 },
	{ "BORDER_SHADOW", constant<DxFont::BORDER_SHADOW>::func, 0 },

	{ "CHARSET_ANSI", constant<ANSI_CHARSET>::func, 0 },
	{ "CHARSET_SHIFTJIS", constant<SHIFTJIS_CHARSET>::func, 0 },
	{ "CHARSET_HANGUL", constant<HANGUL_CHARSET>::func, 0 },
	{ "CHARSET_ARABIC", constant<ARABIC_CHARSET>::func, 0 },
	{ "CHARSET_HEBREW", constant<HEBREW_CHARSET>::func, 0 },
	{ "CHARSET_THAI", constant<THAI_CHARSET>::func, 0 },

	{ "SOUND_BGM", constant<SoundDivision::DIVISION_BGM>::func, 0 },
	{ "SOUND_SE", constant<SoundDivision::DIVISION_SE>::func, 0 },
	{ "SOUND_VOICE", constant<SoundDivision::DIVISION_VOICE>::func, 0 },

	{ "ALIGNMENT_LEFT", constant<DxText::ALIGNMENT_LEFT>::func, 0 },
	{ "ALIGNMENT_RIGHT", constant<DxText::ALIGNMENT_RIGHT>::func, 0 },
	{ "ALIGNMENT_CENTER", constant<DxText::ALIGNMENT_CENTER>::func, 0 },

	{ "CODE_ACP", constant<DxScript::CODE_ACP>::func, 0 },
	{ "CODE_UTF8", constant<DxScript::CODE_UTF8>::func, 0 },
	{ "CODE_UTF16LE", constant<DxScript::CODE_UTF16LE>::func, 0 },
	{ "CODE_UTF16BE", constant<DxScript::CODE_UTF16BE>::func, 0 },

	{ "ENDIAN_LITTLE", constant<ByteOrder::ENDIAN_LITTLE>::func, 0 },
	{ "ENDIAN_BIG", constant<ByteOrder::ENDIAN_BIG>::func, 0 },

	//DirectInput
	{ "KEY_FREE", constant<KEY_FREE>::func, 0 },
	{ "KEY_PUSH", constant<KEY_PUSH>::func, 0 },
	{ "KEY_PULL", constant<KEY_PULL>::func, 0 },
	{ "KEY_HOLD", constant<KEY_HOLD>::func, 0 },

	{ "MOUSE_LEFT", constant<DI_MOUSE_LEFT>::func, 0 },
	{ "MOUSE_RIGHT", constant<DI_MOUSE_RIGHT>::func, 0 },
	{ "MOUSE_MIDDLE", constant<DI_MOUSE_MIDDLE>::func, 0 },

	{ "KEY_ESCAPE", constant<DIK_ESCAPE>::func, 0 },
	{ "KEY_1", constant<DIK_1>::func, 0 },
	{ "KEY_2", constant<DIK_2>::func, 0 },
	{ "KEY_3", constant<DIK_3>::func, 0 },
	{ "KEY_4", constant<DIK_4>::func, 0 },
	{ "KEY_5", constant<DIK_5>::func, 0 },
	{ "KEY_6", constant<DIK_6>::func, 0 },
	{ "KEY_7", constant<DIK_7>::func, 0 },
	{ "KEY_8", constant<DIK_8>::func, 0 },
	{ "KEY_9", constant<DIK_9>::func, 0 },
	{ "KEY_0", constant<DIK_0>::func, 0 },
	{ "KEY_MINUS", constant<DIK_MINUS>::func, 0 },
	{ "KEY_EQUALS", constant<DIK_EQUALS>::func, 0 },
	{ "KEY_BACK", constant<DIK_BACK>::func, 0 },
	{ "KEY_TAB", constant<DIK_TAB>::func, 0 },
	{ "KEY_Q", constant<DIK_Q>::func, 0 },
	{ "KEY_W", constant<DIK_W>::func, 0 },
	{ "KEY_E", constant<DIK_E>::func, 0 },
	{ "KEY_R", constant<DIK_R>::func, 0 },
	{ "KEY_T", constant<DIK_T>::func, 0 },
	{ "KEY_Y", constant<DIK_Y>::func, 0 },
	{ "KEY_U", constant<DIK_U>::func, 0 },
	{ "KEY_I", constant<DIK_I>::func, 0 },
	{ "KEY_O", constant<DIK_O>::func, 0 },
	{ "KEY_P", constant<DIK_P>::func, 0 },
	{ "KEY_LBRACKET", constant<DIK_LBRACKET>::func, 0 },
	{ "KEY_RBRACKET", constant<DIK_RBRACKET>::func, 0 },
	{ "KEY_RETURN", constant<DIK_RETURN>::func, 0 },
	{ "KEY_LCONTROL", constant<DIK_LCONTROL>::func, 0 },
	{ "KEY_A", constant<DIK_A>::func, 0 },
	{ "KEY_S", constant<DIK_S>::func, 0 },
	{ "KEY_D", constant<DIK_D>::func, 0 },
	{ "KEY_F", constant<DIK_F>::func, 0 },
	{ "KEY_G", constant<DIK_G>::func, 0 },
	{ "KEY_H", constant<DIK_H>::func, 0 },
	{ "KEY_J", constant<DIK_J>::func, 0 },
	{ "KEY_K", constant<DIK_K>::func, 0 },
	{ "KEY_L", constant<DIK_L>::func, 0 },
	{ "KEY_SEMICOLON", constant<DIK_SEMICOLON>::func, 0 },
	{ "KEY_APOSTROPHE", constant<DIK_APOSTROPHE>::func, 0 },
	{ "KEY_GRAVE", constant<DIK_GRAVE>::func, 0 },
	{ "KEY_LSHIFT", constant<DIK_LSHIFT>::func, 0 },
	{ "KEY_BACKSLASH", constant<DIK_BACKSLASH>::func, 0 },
	{ "KEY_Z", constant<DIK_Z>::func, 0 },
	{ "KEY_X", constant<DIK_X>::func, 0 },
	{ "KEY_C", constant<DIK_C>::func, 0 },
	{ "KEY_V", constant<DIK_V>::func, 0 },
	{ "KEY_B", constant<DIK_B>::func, 0 },
	{ "KEY_N", constant<DIK_N>::func, 0 },
	{ "KEY_M", constant<DIK_M>::func, 0 },
	{ "KEY_COMMA", constant<DIK_COMMA>::func, 0 },
	{ "KEY_PERIOD", constant<DIK_PERIOD>::func, 0 },
	{ "KEY_SLASH", constant<DIK_SLASH>::func, 0 },
	{ "KEY_RSHIFT", constant<DIK_RSHIFT>::func, 0 },
	{ "KEY_MULTIPLY", constant<DIK_MULTIPLY>::func, 0 },
	{ "KEY_LMENU", constant<DIK_LMENU>::func, 0 },
	{ "KEY_SPACE", constant<DIK_SPACE>::func, 0 },
	{ "KEY_CAPITAL", constant<DIK_CAPITAL>::func, 0 },
	{ "KEY_F1", constant<DIK_F1>::func, 0 },
	{ "KEY_F2", constant<DIK_F2>::func, 0 },
	{ "KEY_F3", constant<DIK_F3>::func, 0 },
	{ "KEY_F4", constant<DIK_F4>::func, 0 },
	{ "KEY_F5", constant<DIK_F5>::func, 0 },
	{ "KEY_F6", constant<DIK_F6>::func, 0 },
	{ "KEY_F7", constant<DIK_F7>::func, 0 },
	{ "KEY_F8", constant<DIK_F8>::func, 0 },
	{ "KEY_F9", constant<DIK_F9>::func, 0 },
	{ "KEY_F10", constant<DIK_F10>::func, 0 },
	{ "KEY_NUMLOCK", constant<DIK_NUMLOCK>::func, 0 },
	{ "KEY_SCROLL", constant<DIK_SCROLL>::func, 0 },
	{ "KEY_NUMPAD7", constant<DIK_NUMPAD7>::func, 0 },
	{ "KEY_NUMPAD8", constant<DIK_NUMPAD8>::func, 0 },
	{ "KEY_NUMPAD9", constant<DIK_NUMPAD9>::func, 0 },
	{ "KEY_SUBTRACT", constant<DIK_SUBTRACT>::func, 0 },
	{ "KEY_NUMPAD4", constant<DIK_NUMPAD4>::func, 0 },
	{ "KEY_NUMPAD5", constant<DIK_NUMPAD5>::func, 0 },
	{ "KEY_NUMPAD6", constant<DIK_NUMPAD6>::func, 0 },
	{ "KEY_ADD", constant<DIK_ADD>::func, 0 },
	{ "KEY_NUMPAD1", constant<DIK_NUMPAD1>::func, 0 },
	{ "KEY_NUMPAD2", constant<DIK_NUMPAD2>::func, 0 },
	{ "KEY_NUMPAD3", constant<DIK_NUMPAD3>::func, 0 },
	{ "KEY_NUMPAD0", constant<DIK_NUMPAD0>::func, 0 },
	{ "KEY_DECIMAL", constant<DIK_DECIMAL>::func, 0 },
	{ "KEY_F11", constant<DIK_F11>::func, 0 },
	{ "KEY_F12", constant<DIK_F12>::func, 0 },
	{ "KEY_F13", constant<DIK_F13>::func, 0 },
	{ "KEY_F14", constant<DIK_F14>::func, 0 },
	{ "KEY_F15", constant<DIK_F15>::func, 0 },
	{ "KEY_KANA", constant<DIK_KANA>::func, 0 },
	{ "KEY_CONVERT", constant<DIK_CONVERT>::func, 0 },
	{ "KEY_NOCONVERT", constant<DIK_NOCONVERT>::func, 0 },
	{ "KEY_YEN", constant<DIK_YEN>::func, 0 },
	{ "KEY_NUMPADEQUALS", constant<DIK_NUMPADEQUALS>::func, 0 },
	{ "KEY_CIRCUMFLEX", constant<DIK_CIRCUMFLEX>::func, 0 },
	{ "KEY_AT", constant<DIK_AT>::func, 0 },
	{ "KEY_COLON", constant<DIK_COLON>::func, 0 },
	{ "KEY_UNDERLINE", constant<DIK_UNDERLINE>::func, 0 },
	{ "KEY_KANJI", constant<DIK_KANJI>::func, 0 },
	{ "KEY_STOP", constant<DIK_STOP>::func, 0 },
	{ "KEY_AX", constant<DIK_AX>::func, 0 },
	{ "KEY_UNLABELED", constant<DIK_UNLABELED>::func, 0 },
	{ "KEY_NUMPADENTER", constant<DIK_NUMPADENTER>::func, 0 },
	{ "KEY_RCONTROL", constant<DIK_RCONTROL>::func, 0 },
	{ "KEY_NUMPADCOMMA", constant<DIK_NUMPADCOMMA>::func, 0 },
	{ "KEY_DIVIDE", constant<DIK_DIVIDE>::func, 0 },
	{ "KEY_SYSRQ", constant<DIK_SYSRQ>::func, 0 },
	{ "KEY_RMENU", constant<DIK_RMENU>::func, 0 },
	{ "KEY_PAUSE", constant<DIK_PAUSE>::func, 0 },
	{ "KEY_HOME", constant<DIK_HOME>::func, 0 },
	{ "KEY_UP", constant<DIK_UP>::func, 0 },
	{ "KEY_PRIOR", constant<DIK_PRIOR>::func, 0 },
	{ "KEY_LEFT", constant<DIK_LEFT>::func, 0 },
	{ "KEY_RIGHT", constant<DIK_RIGHT>::func, 0 },
	{ "KEY_END", constant<DIK_END>::func, 0 },
	{ "KEY_DOWN", constant<DIK_DOWN>::func, 0 },
	{ "KEY_NEXT", constant<DIK_NEXT>::func, 0 },
	{ "KEY_INSERT", constant<DIK_INSERT>::func, 0 },
	{ "KEY_DELETE", constant<DIK_DELETE>::func, 0 },
	{ "KEY_LWIN", constant<DIK_LWIN>::func, 0 },
	{ "KEY_RWIN", constant<DIK_RWIN>::func, 0 },
	{ "KEY_APPS", constant<DIK_APPS>::func, 0 },
	{ "KEY_POWER", constant<DIK_POWER>::func, 0 },
	{ "KEY_SLEEP", constant<DIK_SLEEP>::func, 0 },
};


DxScript::DxScript() {
	_AddFunction(dxFunction, sizeof(dxFunction) / sizeof(function));
	objManager_ = std::shared_ptr<DxScriptObjectManager>(new DxScriptObjectManager);
}
DxScript::~DxScript() {
	_ClearResource();
}
void DxScript::_ClearResource() {
	mapTexture_.clear();
	mapMesh_.clear();

	std::map<std::wstring, gstd::ref_count_ptr<SoundPlayer>>::iterator itrSound;
	for (itrSound = mapSoundPlayer_.begin(); itrSound != mapSoundPlayer_.end(); ++itrSound) {
		SoundPlayer* player = (itrSound->second).GetPointer();
		player->Delete();
	}
	mapSoundPlayer_.clear();
}
int DxScript::AddObject(shared_ptr<DxScriptObjectBase> obj, bool bActivate) {
	obj->idScript_ = idScript_;
	return objManager_->AddObject(obj, bActivate);
}
shared_ptr<Texture> DxScript::_GetTexture(std::wstring name) {
	shared_ptr<Texture> res;
	auto itr = mapTexture_.find(name);
	if (itr != mapTexture_.end()) {
		res = itr->second;
	}
	return res;
}

gstd::value DxScript::Func_MatrixIdentity(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixInverse(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	IsMatrix(machine, arg0);

	D3DXMATRIX mat;
	float* ptrMat = &(mat._11);
	for (size_t i = 0; i < 16; ++i) {
		ptrMat[i] = arg0.index_as_array(i).as_real();
	}
	D3DXMatrixInverse(&mat, nullptr, &mat);

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixAdd(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	float* ptrMat = &(mat._11);
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() + v1.as_real();
	}

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixSubtract(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	float* ptrMat = &(mat._11);
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() - v1.as_real();
	}

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixMultiply(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	float* ptrMat = &(mat._11);
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() * v1.as_real();
	}

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixDivide(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	IsMatrix(machine, arg0);
	IsMatrix(machine, arg1);

	D3DXMATRIX mat;
	float* ptrMat = &(mat._11);
	for (size_t i = 0; i < 16; ++i) {
		const value& v0 = arg0.index_as_array(i);
		const value& v1 = arg1.index_as_array(i);
		ptrMat[i] = v0.as_real() / v1.as_real();
	}

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixTranspose(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	IsMatrix(machine, arg0);

	D3DXMATRIX mat;
	float* ptrMat = &(mat._11);
	for (size_t i = 0; i < 16; ++i) {
		ptrMat[i] = arg0.index_as_array(i).as_real();
	}
	D3DXMatrixTranspose(&mat, &mat);

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixDeterminant(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	IsMatrix(machine, arg0);

	D3DXMATRIX mat;
	float* ptrMat = &(mat._11);
	for (size_t i = 0; i < 16; ++i) {
		ptrMat[i] = arg0.index_as_array(i).as_real();
	}

	double det = D3DXMatrixDeterminant(&mat);

	return value(machine->get_engine()->get_real_type(), det);
}
gstd::value DxScript::Func_MatrixLookatLH(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	const value& arg2 = argv[2];
	IsVector(machine, arg0, 3);
	IsVector(machine, arg1, 3);
	IsVector(machine, arg2, 3);

	D3DXVECTOR3 eye = *(D3DXVECTOR3*)&arg0.index_as_array(0);
	D3DXVECTOR3 dest = *(D3DXVECTOR3*)&arg1.index_as_array(0);
	D3DXVECTOR3 up = *(D3DXVECTOR3*)&arg2.index_as_array(0);

	D3DXMATRIX mat;
	D3DXMatrixLookAtLH(&mat, &eye, &dest, &up);
	
	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}
gstd::value DxScript::Func_MatrixLookatRH(gstd::script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;

	const value& arg0 = argv[0];
	const value& arg1 = argv[1];
	const value& arg2 = argv[2];
	IsVector(machine, arg0, 3);
	IsVector(machine, arg1, 3);
	IsVector(machine, arg2, 3);

	D3DXVECTOR3 eye = *(D3DXVECTOR3*)&arg0.index_as_array(0);
	D3DXVECTOR3 dest = *(D3DXVECTOR3*)&arg1.index_as_array(0);
	D3DXVECTOR3 up = *(D3DXVECTOR3*)&arg2.index_as_array(0);

	D3DXMATRIX mat;
	D3DXMatrixLookAtRH(&mat, &eye, &dest, &up);

	std::vector<float> matVec;
	matVec.resize(16);
	memcpy(&matVec[0], &mat, sizeof(float) * 16);
	return script->CreateRealArrayValue(matVec);
}

//Dx関数：システム系系
gstd::value DxScript::Func_InstallFont(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	DxTextRenderer* renderer = DxTextRenderer::GetBase();
	bool res = false;
	try {
		res = renderer->AddFontFromFile(path);
	}
	catch (gstd::wexception e) {
		Logger::WriteTop(e.what());
	}

	return value(machine->get_engine()->get_boolean_type(), res);
}

//Dx関数：音声系
value DxScript::Func_LoadSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	if (script->mapSoundPlayer_.find(path) != script->mapSoundPlayer_.end())
		return value(machine->get_engine()->get_boolean_type(), true);

	gstd::ref_count_ptr<SoundPlayer> player = manager->GetPlayer(path, true);
	if (player) {
		script->mapSoundPlayer_[path] = player;
	}
	return value(machine->get_engine()->get_boolean_type(), player != nullptr);
}
value DxScript::Func_RemoveSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		itr->second->Delete();
		script->mapSoundPlayer_.erase(itr);
	}
	return value();
}
value DxScript::Func_PlayBGM(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		double loopStart = argv[1].as_real();
		double loopEnd = argv[2].as_real();

		gstd::ref_count_ptr<SoundPlayer> player = itr->second;
		player->SetSoundDivision(SoundDivision::DIVISION_BGM);

		SoundPlayer::PlayStyle style;
		style.SetLoopEnable(true);
		style.SetLoopStartTime(loopStart);
		style.SetLoopEndTime(loopEnd);
		//player->Play(style);
		script->GetObjectManager()->ReserveSound(player, style);
	}
	return value();
}
gstd::value DxScript::Func_PlaySE(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		gstd::ref_count_ptr<SoundPlayer> player = itr->second;
		player->SetSoundDivision(SoundDivision::DIVISION_SE);

		SoundPlayer::PlayStyle style;
		style.SetLoopEnable(false);
		//player->Play(style);
		script->GetObjectManager()->ReserveSound(player, style);
	}
	return value();
}
value DxScript::Func_StopSound(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	auto itr = script->mapSoundPlayer_.find(path);
	if (itr != script->mapSoundPlayer_.end()) {
		gstd::ref_count_ptr<SoundPlayer> player = itr->second;
		player->Stop();
		script->GetObjectManager()->DeleteReservedSound(player);
	}
	return value();
}
value DxScript::Func_SetSoundDivisionVolumeRate(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	int idDivision = (int)argv[0].as_real();
	auto division = manager->GetSoundDivision(idDivision);

	if (division) {
		double volume = argv[1].as_real();
		division->SetVolumeRate(volume);
	}

	return value();
}
value DxScript::Func_GetSoundDivisionVolumeRate(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	int idDivision = (int)argv[0].as_real();
	auto division = manager->GetSoundDivision(idDivision);

	double res = 0;
	if (division) 
		res = division->GetVolumeRate();

	return value(machine->get_engine()->get_real_type(), res);
}

//Dx関数：キー系
gstd::value DxScript::Func_GetKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectInput* input = DirectInput::GetBase();
	int key = (int)argv[0].as_real();
	double res = input->GetKeyState(key);
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_GetMouseX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetMousePosition().x;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_GetMouseY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetMousePosition().y;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_GetMouseMoveZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectInput* input = DirectInput::GetBase();
	double res = input->GetMouseMoveZ();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_GetMouseState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectInput* input = DirectInput::GetBase();
	double res = input->GetMouseState(argv[0].as_real());
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_GetVirtualKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = KEY_FREE;
	VirtualKeyManager* input = dynamic_cast<VirtualKeyManager*>(DirectInput::GetBase());
	if (input) {
		int id = (int)(argv[0].as_real());
		res = input->GetVirtualKeyState(id);
	}
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_SetVirtualKeyState(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	VirtualKeyManager* input = dynamic_cast<VirtualKeyManager*>(DirectInput::GetBase());
	if (input) {
		int id = (int)(argv[0].as_real());
		int state = (int)(argv[1].as_real());
		ref_count_ptr<VirtualKey> vkey = input->GetVirtualKey(id);
		if (vkey) {
			vkey->SetKeyState(state);
		}
	}
	return value();
}
//Dx関数：描画系
gstd::value DxScript::Func_GetScreenWidth(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int res = graphics->GetScreenWidth();
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_GetScreenHeight(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int res = graphics->GetScreenHeight();
	return value(machine->get_engine()->get_real_type(), (double)res);
}
value DxScript::Func_LoadTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		bool res = texture->CreateFromFile(path, false, false);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
value DxScript::Func_LoadTextureInLoadThread(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		bool res = texture->CreateFromFileInLoadThread(path, false, false);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
value DxScript::Func_LoadTextureEx(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	bool useMipMap = argv[1].as_boolean();
	bool useNonPowerOfTwo = argv[2].as_boolean();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		bool res = texture->CreateFromFile(path, useMipMap, useNonPowerOfTwo);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
value DxScript::Func_LoadTextureInLoadThreadEx(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = true;
	std::wstring path = argv[0].as_string();
	bool useMipMap = argv[1].as_boolean();
	bool useNonPowerOfTwo = argv[2].as_boolean();
	path = PathProperty::GetUnique(path);

	if (script->mapTexture_.find(path) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		bool res = texture->CreateFromFileInLoadThread(path, useMipMap, useNonPowerOfTwo);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[path] = texture;
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
value DxScript::Func_RemoveTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	{
		Lock lock(script->criticalSection_);
		script->mapTexture_.erase(path);
	}
	return value();
}
value DxScript::Func_GetTextureWidth(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	TextureManager* textureManager = TextureManager::GetBase();

	shared_ptr<TextureData> textureData = textureManager->GetTextureData(path);
	if (textureData) {
		D3DXIMAGE_INFO* imageInfo = textureData->GetImageInfo();
		res = imageInfo->Width;
	}

	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetTextureHeight(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	std::wstring path = argv[0].as_string();
	path = PathProperty::GetUnique(path);
	TextureManager* textureManager = TextureManager::GetBase();

	shared_ptr<TextureData> textureData = textureManager->GetTextureData(path);
	if (textureData) {
		D3DXIMAGE_INFO* imageInfo = textureData->GetImageInfo();
		res = imageInfo->Height;
	}

	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_SetFogEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool bEnable = argv[0].as_boolean();
	script->GetObjectManager()->SetFogParam(bEnable, 0, 0, 0);
	return value();
}
gstd::value DxScript::Func_SetFogParam(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	double start = argv[0].as_real();
	double end = argv[1].as_real();
	int r = (int)argv[2].as_real();
	int g = (int)argv[3].as_real();
	int b = (int)argv[4].as_real();
	D3DCOLOR color = D3DCOLOR_ARGB(255, r, g, b);
	script->GetObjectManager()->SetFogParam(true, color, start, end);
	return value();
}
gstd::value DxScript::Func_CreateRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;
	std::wstring name = argv[0].as_string();

	if (script->mapTexture_.find(name) == script->mapTexture_.end()) {
		shared_ptr<Texture> texture(new Texture());
		res = texture->CreateRenderTarget(name);
		if (res) {
			Lock lock(script->criticalSection_);
			script->mapTexture_[name] = texture;
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_CreateRenderTargetEx(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;
	std::wstring name = argv[0].as_string();
	double width = argv[1].as_real();
	double height = argv[2].as_real();

	if (width > 0 && height > 0) {
		if (script->mapTexture_.find(name) == script->mapTexture_.end()) {
			shared_ptr<Texture> texture(new Texture());
			res = texture->CreateRenderTarget(name, (size_t)width, (size_t)height);
			if (res) {
				Lock lock(script->criticalSection_);
				script->mapTexture_[name] = texture;
			}
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_SetRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
	if (texture == nullptr)
		script->RaiseError("The specified render target does not exist.");
	if (texture->GetType() != TextureData::TYPE_RENDER_TARGET)
		script->RaiseError("Target texture must be a render target.");

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetRenderTarget(texture);
	if (argv[1].as_boolean()) graphics->ClearRenderTarget();

	return value();
}
gstd::value DxScript::Func_ResetRenderTarget(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetRenderTarget(nullptr);

	return value();
}
gstd::value DxScript::Func_ClearRenderTargetA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
	if (texture == nullptr)
		texture = textureManager->GetTexture(name);
	if (texture == nullptr) 
		return value(machine->get_engine()->get_boolean_type(), false);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	shared_ptr<Texture> current = graphics->GetRenderTarget();

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture);
	device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	graphics->SetRenderTarget(current);

	return value(machine->get_engine()->get_boolean_type(), true);
}
gstd::value DxScript::Func_ClearRenderTargetA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
	if (texture == nullptr)
		texture = textureManager->GetTexture(name);
	if (texture == nullptr)
		return value(machine->get_engine()->get_boolean_type(), false);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	shared_ptr<Texture> current = graphics->GetRenderTarget();

	byte cr = (byte)argv[1].as_real();
	byte cg = (byte)argv[2].as_real();
	byte cb = (byte)argv[3].as_real();
	byte ca = (byte)argv[4].as_real();

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture);
	device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(ca, cr, cg, cb), 1.0f, 0);
	graphics->SetRenderTarget(current);

	return value(machine->get_engine()->get_boolean_type(), true);
}
gstd::value DxScript::Func_ClearRenderTargetA3(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	TextureManager* textureManager = TextureManager::GetBase();

	std::wstring name = argv[0].as_string();
	shared_ptr<Texture> texture = script->_GetTexture(name);
	if (texture == nullptr)
		texture = textureManager->GetTexture(name);
	if (texture == nullptr)
		return value(machine->get_engine()->get_boolean_type(), false);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	shared_ptr<Texture> current = graphics->GetRenderTarget();

	byte cr = (byte)argv[1].as_real();
	byte cg = (byte)argv[2].as_real();
	byte cb = (byte)argv[3].as_real();
	byte ca = (byte)argv[4].as_real();

	LONG rl = (LONG)argv[5].as_real();
	LONG rt = (LONG)argv[6].as_real();
	LONG rr = (LONG)argv[7].as_real();
	LONG rb = (LONG)argv[8].as_real();
	D3DRECT rc = { rl, rt, rr, rb };

	IDirect3DDevice9* device = graphics->GetDevice();
	graphics->SetRenderTarget(texture);
	device->Clear(1, &rc, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(ca, cr, cg, cb), 1.0f, 0);
	graphics->SetRenderTarget(current);

	return value(machine->get_engine()->get_boolean_type(), true);
}
gstd::value DxScript::Func_GetTransitionRenderTargetName(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	std::wstring res = TextureManager::TARGET_TRANSITION;
	return value(machine->get_engine()->get_string_type(), res);
}
gstd::value DxScript::Func_SaveRenderedTextureA1(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	shared_ptr<Texture> texture = script->_GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);

	if (texture) {
		//フォルダ生成
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateDirectory(dir);

		//保存
		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		RECT rect = { 0, 0, graphics->GetScreenWidth(), graphics->GetScreenHeight() };
		D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
			pSurface, nullptr, &rect);
	}

	return value();
}
gstd::value DxScript::Func_SaveRenderedTextureA2(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	int rcLeft = (int)argv[2].as_real();
	int rcTop = (int)argv[3].as_real();
	int rcRight = (int)argv[4].as_real();
	int rcBottom = (int)argv[5].as_real();

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	shared_ptr<Texture> texture = script->_GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);
	if (texture) {
		//フォルダ生成
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateDirectory(dir);

		//保存
		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		RECT rect = { rcLeft, rcTop, rcRight, rcBottom };
		D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_BMP,
			pSurface, nullptr, &rect);
	}

	return value();
}
gstd::value DxScript::Func_SaveRenderedTextureA3(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;

	std::wstring nameTexture = argv[0].as_string();
	std::wstring path = argv[1].as_string();
	path = PathProperty::GetUnique(path);

	int rcLeft = (int)argv[2].as_real();
	int rcTop = (int)argv[3].as_real();
	int rcRight = (int)argv[4].as_real();
	int rcBottom = (int)argv[5].as_real();
	int imgFormat = (int)argv[6].as_real();

	if (imgFormat < 0)
		imgFormat = 0;
	if (imgFormat > D3DXIFF_PPM)
		imgFormat = D3DXIFF_PPM;

	TextureManager* textureManager = TextureManager::GetBase();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	shared_ptr<Texture> texture = script->_GetTexture(nameTexture);
	if (texture == nullptr)
		texture = textureManager->GetTexture(nameTexture);
	if (texture) {
		//フォルダ生成
		std::wstring dir = PathProperty::GetFileDirectory(path);
		File::CreateDirectory(dir);

		//保存
		IDirect3DSurface9* pSurface = texture->GetD3DSurface();
		RECT rect = { rcLeft, rcTop, rcRight, rcBottom };
		D3DXSaveSurfaceToFile(path.c_str(), (D3DXIMAGE_FILEFORMAT)imgFormat,
			pSurface, nullptr, &rect);
	}

	return value();
}
gstd::value DxScript::Func_IsPixelShaderSupported(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	int major = (int)(argv[0].as_real() + 0.5);
	int minor = (int)(argv[1].as_real() + 0.5);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool res = graphics->IsPixelShaderSupported(major, minor);

	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_SetEnableAntiAliasing(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	bool enable = argv[0].as_boolean();

	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool res = SUCCEEDED(graphics->SetFullscreenAntiAliasing(enable));

	return value(machine->get_engine()->get_boolean_type(), res);
}

gstd::value DxScript::Func_SetShader(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();

		double min = argv[1].as_real();
		double max = argv[2].as_real();
		auto objectManager = script->GetObjectManager();
		int size = objectManager->GetRenderBucketCapacity();

		objectManager->SetShader(shader, min * size, max * size);
	}
	return value();
}
gstd::value DxScript::Func_SetShaderI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();

		int min = Math::Trunc(argv[1].as_real());
		int max = Math::Trunc(argv[2].as_real());

		auto objectManager = script->GetObjectManager();
		objectManager->SetShader(shader, min, max);
	}
	return value();
}
gstd::value DxScript::Func_ResetShader(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	shared_ptr<Shader> shader = nullptr;

	double min = (double)argv[0].as_real();
	double max = (double)argv[1].as_real();
	auto objectManager = script->GetObjectManager();
	int size = objectManager->GetRenderBucketCapacity();

	objectManager->SetShader(shader, min * size, max * size);

	return value();
}
gstd::value DxScript::Func_ResetShaderI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	shared_ptr<Shader> shader = nullptr;

	int min = Math::Trunc(argv[1].as_real());
	int max = Math::Trunc(argv[2].as_real());

	auto objectManager = script->GetObjectManager();
	objectManager->SetShader(shader, min, max);

	return value();
}

//Dx関数：カメラ3D
value DxScript::Func_SetCameraFocusX(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusX(x);
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraFocusY(script_machine* machine, int argc, const value* argv) {
	double y = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusY(y);
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraFocusZ(script_machine* machine, int argc, const value* argv) {
	double z = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusZ(z);
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraFocusXYZ(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	double z = argv[2].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetFocusX(x);
	camera->SetFocusY(y);
	camera->SetFocusZ(z);
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraRadius(script_machine* machine, int argc, const value* argv) {
	double r = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetRadius(r);
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraAzimuthAngle(script_machine* machine, int argc, const value* argv) {
	double angle = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetAzimuthAngle(Math::DegreeToRadian(angle));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraElevationAngle(script_machine* machine, int argc, const value* argv) {
	double angle = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetElevationAngle(Math::DegreeToRadian(angle));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraYaw(script_machine* machine, int argc, const value* argv) {
	double angle = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetYaw(Math::DegreeToRadian(angle));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraPitch(script_machine* machine, int argc, const value* argv) {
	double angle = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetPitch(Math::DegreeToRadian(angle));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraRoll(script_machine* machine, int argc, const value* argv) {
	double angle = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetRoll(Math::DegreeToRadian(angle));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraMode(script_machine* machine, int argc, const value* argv) {
	int mode = (int)(argv[0].as_real() + 0.01);
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetCameraMode(mode);
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_SetCameraPosLookAt(script_machine* machine, int argc, const value* argv) {
	double x = argv[0].as_real();
	double y = argv[1].as_real();
	double z = argv[2].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	camera->SetCameraLookAtVector(D3DXVECTOR3(x, y, z));
	camera->thisViewChanged_ = true;
	return value();
}
value DxScript::Func_GetCameraX(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetCameraPosition().x;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetCameraY(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetCameraPosition().y;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetCameraZ(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetCameraPosition().z;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetCameraFocusX(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetFocusPosition().x;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetCameraFocusY(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetFocusPosition().y;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetCameraFocusZ(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetFocusPosition().z;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetCameraRadius(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetRadius();
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_GetCameraAzimuthAngle(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetAzimuthAngle();
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraElevationAngle(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetElevationAngle();
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraYaw(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetYaw();
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraPitch(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetPitch();
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraRoll(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera()->GetRoll();
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
value DxScript::Func_GetCameraViewProjectionMatrix(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectGraphics* graphics = DirectGraphics::GetBase();

	D3DXMATRIX matViewProj = graphics->GetCamera()->GetViewProjectionMatrix();

	std::vector<float> mat;
	mat.resize(16);
	memcpy(&mat[0], &matViewProj, sizeof(float) * 16);

	return script->CreateRealArrayValue(mat);
}
value DxScript::Func_SetCameraPerspectiveClip(script_machine* machine, int argc, const value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double clipNear = argv[0].as_real();
	double clipFar = argv[1].as_real();
	int width = graphics->GetScreenWidth();
	int height = graphics->GetScreenHeight();

	auto camera = graphics->GetCamera();
	//camera->SetProjectionMatrix(width, height, clipNear, clipFar);
	camera->SetPerspectiveClip(clipNear, clipFar);
	camera->thisProjectionChanged_ = true;

	return value();
}

//Dx関数：カメラ2D
gstd::value DxScript::Func_Set2DCameraFocusX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double x = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetFocusX(x);
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraFocusY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double y = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetFocusY(y);
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double angle = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetAngleZ(Math::DegreeToRadian(angle));
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraRatio(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double ratio = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetRatio(ratio);
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraRatioX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double ratio = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetRatioX(ratio);
	return gstd::value();
}
gstd::value DxScript::Func_Set2DCameraRatioY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double ratio = argv[0].as_real();
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->SetRatioY(ratio);
	return gstd::value();
}
gstd::value DxScript::Func_Reset2DCamera(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->GetCamera2D()->Reset();
	return gstd::value();
}
gstd::value DxScript::Func_Get2DCameraX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetFocusX();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_Get2DCameraY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetFocusY();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_Get2DCameraAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetAngleZ();
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
gstd::value DxScript::Func_Get2DCameraRatio(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetRatio();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_Get2DCameraRatioX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetRatioX();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_Get2DCameraRatioY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	double res = graphics->GetCamera2D()->GetRatioY();
	return value(machine->get_engine()->get_real_type(), res);
}

//Dx関数：その他
gstd::value DxScript::Func_GetObjectDistance(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id1 = (int)argv[0].as_real();
	int id2 = (int)argv[1].as_real();

	double res = -1;
	DxScriptRenderObject* obj1 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id1));
	if (obj1) {
		DxScriptRenderObject* obj2 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id2));
		if (obj2) {
			int tx = obj1->GetPosition().x - obj2->GetPosition().x;
			int ty = obj1->GetPosition().y - obj2->GetPosition().y;
			int tz = obj1->GetPosition().z - obj2->GetPosition().z;

			res = sqrt(tx * tx + ty * ty + tz * tz);
		}
	}
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_GetObject2dPosition(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)script->RaiseError("Invalid object; object might not have been initialized.");

	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	D3DXVECTOR3 pos = obj->GetPosition();

	D3DXVECTOR2 point = camera->TransformCoordinateTo2D(pos);
	std::vector<float> listRes;
	listRes.push_back(point.x);
	listRes.push_back(point.y);

	return script->CreateRealArrayValue(listRes);
}
gstd::value DxScript::Func_Get2dPosition(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	double px = argv[0].as_real();
	double py = argv[1].as_real();
	double pz = argv[2].as_real();

	D3DXVECTOR3 pos(px, py, pz);

	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	D3DXVECTOR2 point = camera->TransformCoordinateTo2D(pos);
	std::vector<float> listRes;
	listRes.push_back(point.x);
	listRes.push_back(point.y);

	return script->CreateRealArrayValue(listRes);
}

gstd::value DxScript::Func_IsIntersected_Circle_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxCircle circle1(argv[0].as_real(), argv[1].as_real(), argv[2].as_real());
	DxCircle circle2(argv[3].as_real(), argv[4].as_real(), argv[5].as_real());

	bool res = DxMath::IsIntersected(circle1, circle2);
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_IsIntersected_Line_Circle(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line(
		argv[0].as_real(),
		argv[1].as_real(),
		argv[2].as_real(),
		argv[3].as_real(),
		argv[4].as_real()
	);

	DxCircle circle(argv[5].as_real(), argv[6].as_real(), argv[7].as_real());

	bool res = DxMath::IsIntersected(circle, line);
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_IsIntersected_Line_Line(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxWidthLine line1(
		argv[0].as_real(),
		argv[1].as_real(),
		argv[2].as_real(),
		argv[3].as_real(),
		argv[4].as_real()
	);
	DxWidthLine line2(
		argv[5].as_real(),
		argv[6].as_real(),
		argv[7].as_real(),
		argv[8].as_real(),
		argv[9].as_real()
	);

	bool res = DxMath::IsIntersected(line1, line2);
	return value(machine->get_engine()->get_boolean_type(), res);
}

//Dx関数：オブジェクト操作(共通)
value DxScript::Func_Obj_Delete(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	int id = (int)argv[0].as_real();
	script->DeleteObject(id);
	return value();
}
value DxScript::Func_Obj_IsDeleted(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	bool res = obj == nullptr;
	return value(machine->get_engine()->get_boolean_type(), res);
}
value DxScript::Func_Obj_SetVisible(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj == nullptr)return value();
	obj->bVisible_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_Obj_IsVisible(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	bool res = false;
	if (obj) res = obj->bVisible_;
	return value(machine->get_engine()->get_boolean_type(), res);
}
value DxScript::Func_Obj_SetRenderPriority(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		size_t maxPri = script->GetObjectManager()->GetRenderBucketCapacity() - 1U;
		double pri = argv[1].as_real();

		if (pri < 0) pri = 0;
		else if (pri > 1) pri = 1;

		obj->priRender_ = pri * maxPri;
	}
	return value();
}
value DxScript::Func_Obj_SetRenderPriorityI(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		int pri = Math::Trunc(argv[1].as_real());
		size_t maxPri = script->GetObjectManager()->GetRenderBucketCapacity() - 1U;

		if (pri < 0) pri = 0;
		else if (pri > maxPri) pri = maxPri;

		obj->priRender_ = pri;
	}
	return value();
}
gstd::value DxScript::Func_Obj_GetRenderPriority(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;

	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj)
		res = obj->GetRenderPriorityI() / (double)(script->GetObjectManager()->GetRenderBucketCapacity() - 1U);

	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_Obj_GetRenderPriorityI(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;

	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) res = obj->GetRenderPriorityI();

	return value(machine->get_engine()->get_real_type(), res);
}

gstd::value DxScript::Func_Obj_GetValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHash(key));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return value();
}
gstd::value DxScript::Func_Obj_GetValueD(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();
	gstd::value def = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHash(key));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return def;
}
gstd::value DxScript::Func_Obj_SetValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();
	gstd::value val = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) obj->SetObjectValue(key, val);

	return value();
}
gstd::value DxScript::Func_Obj_DeleteValue(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) obj->DeleteObjectValue(key);

	return value();
}
gstd::value DxScript::Func_Obj_IsValueExists(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::wstring key = argv[1].as_string();

	bool res = false;

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) res = obj->IsObjectValueExists(key);

	return value(machine->get_engine()->get_boolean_type(), res);
}

gstd::value DxScript::Func_Obj_GetValueR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHashReal(keyDbl));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return value();
}
gstd::value DxScript::Func_Obj_GetValueDR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();
	gstd::value def = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		auto itr = obj->mapObjectValue_.find(DxScriptObjectBase::GetKeyHashReal(keyDbl));
		if (itr != obj->mapObjectValue_.end()) return itr->second;
	}
	return def;
}
gstd::value DxScript::Func_Obj_SetValueR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();
	gstd::value val = argv[2];

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		obj->SetObjectValue(DxScriptObjectBase::GetKeyHashReal(keyDbl), val);
	}

	return value();
}
gstd::value DxScript::Func_Obj_DeleteValueR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		obj->DeleteObjectValue(DxScriptObjectBase::GetKeyHashReal(keyDbl));
	}

	return value();
}
gstd::value DxScript::Func_Obj_IsValueExistsR(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double keyDbl = argv[1].as_real();

	bool res = false;

	DxScriptObjectBase* obj = script->GetObjectPointer(id);
	if (obj) {
		res = obj->IsObjectValueExists(DxScriptObjectBase::GetKeyHashReal(keyDbl));
	}

	return value(machine->get_engine()->get_boolean_type(), res);
}

value DxScript::Func_Obj_GetType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	double res = (int)TypeObject::OBJ_INVALID;

	DxScriptObjectBase* obj = dynamic_cast<DxScriptObjectBase*>(script->GetObjectPointer(id));
	if (obj) res = (int)obj->GetObjectType();

	return value(machine->get_engine()->get_real_type(), res);
}


//Dx関数：オブジェクト操作(RenderObject)
value DxScript::Func_ObjRender_SetX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		obj->SetX(argv[1].as_real());
		obj->SetY(argv[2].as_real());
		obj->SetZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetAngleX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAngleX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAngleY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAngleZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetAngleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		obj->SetAngleX(argv[1].as_real());
		obj->SetAngleY(argv[2].as_real());
		obj->SetAngleZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetScaleX(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetScaleX(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleY(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetScaleY(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetScaleZ(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetScaleXYZ(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		obj->SetScaleX(argv[1].as_real());
		obj->SetScaleY(argv[2].as_real());
		obj->SetScaleZ(argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjRender_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetColor((int)argv[1].as_real(), (int)argv[2].as_real(), (int)argv[3].as_real());
	return value();
}
value DxScript::Func_ObjRender_SetColorHSV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		int hue = (int)argv[1].as_real();
		int sat = (int)argv[2].as_real();
		int val = (int)argv[3].as_real();

		D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255);
		color = ColorAccess::SetColorHSV(color, hue, sat, val);

		int red = ColorAccess::GetColorR(color);
		int green = ColorAccess::GetColorG(color);
		int blue = ColorAccess::GetColorB(color);

		obj->SetColor(red, green, blue);
	}
	return value();
}
value DxScript::Func_ObjRender_GetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::vector<float> vecColor;
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		D3DCOLOR color = obj->color_;
		vecColor.push_back(ColorAccess::GetColorR(color));
		vecColor.push_back(ColorAccess::GetColorG(color));
		vecColor.push_back(ColorAccess::GetColorB(color));
	}
	return script->CreateRealArrayValue(vecColor);
}
value DxScript::Func_ObjRender_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAlpha((int)argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_GetAlpha(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = ColorAccess::GetColorA(obj->color_);
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_ObjRender_SetBlendType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetBlendType((int)argv[1].as_real());
	return value();
}
value DxScript::Func_ObjRender_GetX(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->position_.x;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_ObjRender_GetY(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->position_.y;
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_ObjRender_GetZ(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->position_.z;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjRender_GetAngleX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->angle_.x;
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetAngleY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->angle_.y;
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetAngleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->angle_.z;
	return value(machine->get_engine()->get_real_type(), Math::RadianToDegree(res));
}
gstd::value DxScript::Func_ObjRender_GetScaleX(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->scale_.x;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjRender_GetScaleY(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->scale_.y;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjRender_GetScaleZ(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->scale_.z;
	return value(machine->get_engine()->get_real_type(), res);
}

value DxScript::Func_ObjRender_SetZWrite(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bZWrite_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetZTest(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bZTest_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetFogEnable(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->bFogEnable_ = argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetCullingMode(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->modeCulling_ = (int)argv[1].as_boolean();
	return value();
}
value DxScript::Func_ObjRender_SetRalativeObject(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		int idRelative = (int)argv[1].as_real();
		std::wstring nameBone = argv[2].as_string();
		DxScriptObjectBase* objRelative = dynamic_cast<DxScriptObjectBase*>(script->GetObjectPointer(idRelative));
		if (objRelative)
			obj->SetRelativeObject(idRelative, nameBone);
	}
	return value();
}
value DxScript::Func_ObjRender_SetPermitCamera(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool bEnable = argv[1].as_boolean();

	DxScriptObjectBase* pObj = script->GetObjectPointer(id);
	DxScriptPrimitiveObject2D* obj2D = dynamic_cast<DxScriptPrimitiveObject2D*>(pObj);
	DxScriptTextObject* objText = dynamic_cast<DxScriptTextObject*>(pObj);
	if (obj2D)
		obj2D->SetPermitCamera(bEnable);
	else if (objText)
		objText->SetPermitCamera(bEnable);

	return value();
}

gstd::value DxScript::Func_ObjRender_GetBlendType(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	int res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetBlendType();
	return value(machine->get_engine()->get_real_type(), (double)res);
}

value DxScript::Func_ObjRender_SetTextureFilterMin(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int type = (int)(argv[1].as_real() + 0.01);

	type = std::max(type, (int)D3DTEXF_NONE);
	type = std::min(type, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetFilteringMin((D3DTEXTUREFILTERTYPE)type);

	return value();
}
value DxScript::Func_ObjRender_SetTextureFilterMag(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int type = (int)(argv[1].as_real() + 0.01);

	type = std::max(type, (int)D3DTEXF_NONE);
	type = std::min(type, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetFilteringMag((D3DTEXTUREFILTERTYPE)type);

	return value();
}
value DxScript::Func_ObjRender_SetTextureFilterMip(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int type = (int)(argv[1].as_real() + 0.01);

	type = std::max(type, (int)D3DTEXF_NONE);
	type = std::min(type, (int)D3DTEXF_ANISOTROPIC);

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetFilteringMip((D3DTEXTUREFILTERTYPE)type);

	return value();
}
value DxScript::Func_ObjRender_SetVertexShaderRenderingMode(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexShaderRendering(argv[1].as_boolean());

	return value();
}

//Dx関数：オブジェクト操作(ShaderObject)
gstd::value DxScript::Func_ObjShader_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	shared_ptr<DxScriptShaderObject> obj = std::make_shared<DxScriptShaderObject>();

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return value(machine->get_engine()->get_real_type(), (double)id);
}
gstd::value DxScript::Func_ObjShader_SetShaderF(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);
		{
			ShaderManager* manager = ShaderManager::GetBase();

			shared_ptr<Shader> shader = manager->CreateFromFile(path);
			obj->SetShader(shader);

			res = shader != nullptr;

			if (!res) {
				std::wstring error = manager->GetLastError();
				script->RaiseError(error);
			}
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_ObjShader_SetShaderO(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	bool res = false;

	int id1 = (int)argv[0].as_real();
	DxScriptRenderObject* obj1 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id1));
	if (obj1) {
		int id2 = (int)argv[1].as_real();
		DxScriptRenderObject* obj2 = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id2));
		if (obj2) {
			shared_ptr<Shader> shader = obj2->GetShader();
			if (shader) {
				obj1->SetShader(shader);
				res = true;
			}
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_ObjShader_ResetShader(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetShader(nullptr);
	return value();
}
gstd::value DxScript::Func_ObjShader_SetTechnique(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string aPath = StringUtility::ConvertWideToMulti(argv[1].as_string());
			shader->SetTechnique(aPath);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetMatrix(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			const gstd::value& sMatrix = argv[2];
			type_data::type_kind kind = sMatrix.get_type()->get_kind();
			if (kind == type_data::type_kind::tk_array) {
				if (sMatrix.length_as_array() == 16) {
					D3DXMATRIX matrix;
					for (size_t iRow = 0; iRow < 4; iRow++) {
						for (size_t iCol = 0; iCol < 4; iCol++) {
							size_t index = iRow * 4 + iCol;
							const value& arrayValue = sMatrix.index_as_array(index);
							matrix.m[iRow][iCol] = arrayValue.as_real();
						}
					}
					shader->SetMatrix(name, matrix);
				}
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetMatrixArray(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			gstd::value array = argv[2];
			type_data::type_kind kind = array.get_type()->get_kind();
			if (kind == type_data::type_kind::tk_array) {
				size_t dataLength = array.length_as_array();
				std::vector<D3DXMATRIX> listMatrix;
				for (size_t iArray = 0; iArray < dataLength; ++iArray) {
					const value& sMatrix = array.index_as_array(iArray);
					type_data::type_kind kind = sMatrix.get_type()->get_kind();
					if (kind != type_data::type_kind::tk_array) {
						if (sMatrix.length_as_array() == 16) {
							D3DXMATRIX matrix;
							for (size_t iRow = 0; iRow < 4; ++iRow) {
								for (size_t iCol = 0; iCol < 4; ++iCol) {
									size_t index = iRow * 4 + iCol;
									const value& arrayValue = sMatrix.index_as_array(index);
									matrix.m[iRow][iCol] = arrayValue.as_real();
								}
							}
							listMatrix.push_back(matrix);
						}
					}
				}
				shader->SetMatrixArray(name, listMatrix);
			}
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetVector(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			D3DXVECTOR4 vect4;
			vect4.x = (FLOAT)argv[2].as_real();
			vect4.y = (FLOAT)argv[3].as_real();
			vect4.z = (FLOAT)argv[4].as_real();
			vect4.w = (FLOAT)argv[5].as_real();

			shader->SetVector(name, vect4);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			shader->SetFloat(name, (FLOAT)argv[2].as_real());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetFloatArray(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			const gstd::value& array = argv[2];
			type_data::type_kind kind = array.get_type()->get_kind();
			if (kind != type_data::type_kind::tk_array)return value();

			size_t dataLength = array.length_as_array();
			std::vector<FLOAT> listFloat;
			for (size_t iArray = 0; iArray < dataLength; ++iArray) {
				const value& aValue = array.index_as_array(iArray);
				listFloat.push_back((FLOAT)aValue.as_real());
			}
			shader->SetFloatArray(name, listFloat);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjShader_SetTexture(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Shader> shader = obj->GetShader();
		if (shader) {
			std::string name = StringUtility::ConvertWideToMulti(argv[1].as_string());
			std::wstring path = argv[2].as_string();
			path = PathProperty::GetUnique(path);

			auto itr = script->mapTexture_.find(path);
			if (itr != script->mapTexture_.end()) {
				shader->SetTexture(name, itr->second);
			}
			else {
				shared_ptr<Texture> texture(new Texture());
				texture->CreateFromFile(path, false, false);
				shader->SetTexture(name, texture);
			}
		}
	}
	return value();
}

//Dx関数：オブジェクト操作(PrimitiveObject)
value DxScript::Func_ObjPrimitive_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)((int)argv[0].as_real());

	shared_ptr<DxScriptPrimitiveObject> obj;
	if (type == TypeObject::OBJ_PRIMITIVE_2D) {
		obj = std::make_shared<DxScriptPrimitiveObject2D>();
	}
	else if (type == TypeObject::OBJ_SPRITE_2D) {
		obj = std::make_shared<DxScriptSpriteObject2D>();
	}
	else if (type == TypeObject::OBJ_SPRITE_LIST_2D) {
		obj = std::make_shared<DxScriptSpriteListObject2D>();
	}
	else if (type == TypeObject::OBJ_PRIMITIVE_3D) {
		obj = std::make_shared<DxScriptPrimitiveObject3D>();
	}
	else if (type == TypeObject::OBJ_SPRITE_3D) {
		obj = std::make_shared<DxScriptSpriteObject3D>();
	}
	else if (type == TypeObject::OBJ_TRAJECTORY_3D) {
		obj = std::make_shared<DxScriptTrajectoryObject3D>();
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return value(machine->get_engine()->get_real_type(), (double)id);
}
value DxScript::Func_ObjPrimitive_SetPrimitiveType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetPrimitiveType((D3DPRIMITIVETYPE)(int)argv[1].as_real());
	return value();
}
value DxScript::Func_ObjPrimitive_GetPrimitiveType(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetPrimitiveType();
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_ObjPrimitive_SetVertexCount(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexCount((int)argv[1].as_real());
	return value();
}
value DxScript::Func_ObjPrimitive_SetTexture(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		auto itr = script->mapTexture_.find(path);
		if (itr != script->mapTexture_.end()) {
			obj->SetTexture(itr->second);
		}
		else {
			shared_ptr<Texture> texture(new Texture());
			texture->CreateFromFile(path, false, false);
			obj->SetTexture(texture);
		}
	}
	return value();
}
value DxScript::Func_ObjPrimitive_GetVertexCount(script_machine* machine, int argc, const value* argv) {
	double res = 0;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetVertexCount();
	return value(machine->get_engine()->get_real_type(), res);
}
value DxScript::Func_ObjPrimitive_SetVertexPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexPosition((int)argv[1].as_real(), argv[2].as_real(), argv[3].as_real(), argv[4].as_real());
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexUV(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexUV((int)argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	return value();
}
gstd::value DxScript::Func_ObjPrimitive_SetVertexUVT(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<Texture> texture = obj->GetTexture();
		if (texture) {
			int width = texture->GetWidth();
			int height = texture->GetHeight();
			obj->SetVertexUV((int)argv[1].as_real(), argv[2].as_real() / width, argv[3].as_real() / height);
		}
	}
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexColor((int)argv[1].as_real(), (int)argv[2].as_real(), (int)argv[3].as_real(), (int)argv[4].as_real());
	return value();
}
value DxScript::Func_ObjPrimitive_SetVertexAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetVertexAlpha((int)argv[1].as_real(), (int)argv[2].as_real());
	return value();
}
value DxScript::Func_ObjPrimitive_GetVertexPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int index = (int)argv[1].as_real();

	D3DXVECTOR3 pos = D3DXVECTOR3(0, 0, 0);
	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj)
		pos = obj->GetVertexPosition(index);

	std::vector<float> listPos;
	listPos.resize(3);
	listPos[0] = pos.x;
	listPos[1] = pos.y;
	listPos[2] = pos.z;

	gstd::value res = script->CreateRealArrayValue(listPos);
	return res;
}
value DxScript::Func_ObjPrimitive_SetVertexIndex(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		::RenderObject* objRender = obj->GetObjectPointer();

		value valArr = argv[1];
		std::vector<uint16_t> vecIndex;
		vecIndex.resize(valArr.length_as_array());
		for (size_t i = 0; i < valArr.length_as_array(); ++i) {
			vecIndex[i] = (uint16_t)valArr.index_as_array(i).as_real();
		}
		objRender->SetVertexIndices(vecIndex);
	}
	return value();
}

//Dx関数：オブジェクト操作(Sprite2D)
value DxScript::Func_ObjSprite2D_SetSourceRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject2D* obj = dynamic_cast<DxScriptSpriteObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			(double)argv[1].as_real(),
			(double)argv[2].as_real(),
			(double)argv[3].as_real(),
			(double)argv[4].as_real()
		};
		obj->GetSpritePointer()->SetSourceRect(rcDest);
	}
	return value();
}
value DxScript::Func_ObjSprite2D_SetDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject2D* obj = dynamic_cast<DxScriptSpriteObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			(double)argv[1].as_real(),
			(double)argv[2].as_real(),
			(double)argv[3].as_real(),
			(double)argv[4].as_real()
		};
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
value DxScript::Func_ObjSprite2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject2D* obj = dynamic_cast<DxScriptSpriteObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->SetDestinationCenter();
	return value();
}

//Dx関数：オブジェクト操作(SpriteList2D)
gstd::value DxScript::Func_ObjSpriteList2D_SetSourceRect(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			(double)argv[1].as_real(),
			(double)argv[2].as_real(),
			(double)argv[3].as_real(),
			(double)argv[4].as_real()
		};
		obj->GetSpritePointer()->SetSourceRect(rcDest);
	}
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetDestRect(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			(double)argv[1].as_real(),
			(double)argv[2].as_real(),
			(double)argv[3].as_real(),
			(double)argv[4].as_real()
		};
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetDestCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->SetDestinationCenter();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_AddVertex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->AddVertex();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_CloseVertex(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->CloseVertex();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_ClearVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->ClearVertexCount();
	return value();
}
gstd::value DxScript::Func_ObjSpriteList2D_SetAutoClearVertexCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteListObject2D* obj = dynamic_cast<DxScriptSpriteListObject2D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetSpritePointer()->SetAutoClearVertex(argv[1].as_boolean());
	return value();
}

//Dx関数：オブジェクト操作(Sprite3D)
value DxScript::Func_ObjSprite3D_SetSourceRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcSrc = {
			(double)argv[1].as_real(),
			(double)argv[2].as_real(),
			(double)argv[3].as_real(),
			(double)argv[4].as_real()
		};
		obj->GetSpritePointer()->SetSourceRect(rcSrc);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcDest = {
			(double)argv[1].as_real(),
			(double)argv[2].as_real(),
			(double)argv[3].as_real(),
			(double)argv[4].as_real()
		};
		obj->GetSpritePointer()->SetDestinationRect(rcDest);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetSourceDestRect(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		RECT_D rcSrc = {
			(double)argv[1].as_real(),
			(double)argv[2].as_real(),
			(double)argv[3].as_real(),
			(double)argv[4].as_real()
		};
		obj->GetSpritePointer()->SetSourceDestRect(rcSrc);
	}
	return value();
}
value DxScript::Func_ObjSprite3D_SetBillboard(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptSpriteObject3D* obj = dynamic_cast<DxScriptSpriteObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		bool bEnable = argv[1].as_boolean();
		obj->GetSpritePointer()->SetBillboardEnable(bEnable);
	}
	return value();
}
//Dx関数：オブジェクト操作(TrajectoryObject3D)
value DxScript::Func_ObjTrajectory3D_SetInitialPoint(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTrajectoryObject3D* obj = dynamic_cast<DxScriptTrajectoryObject3D*>(script->GetObjectPointer(id));
	if (obj) {
		D3DXVECTOR3 pos1(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
		D3DXVECTOR3 pos2(argv[4].as_real(), argv[5].as_real(), argv[6].as_real());
		obj->GetObjectPointer()->SetInitialLine(pos1, pos2);
	}
	return value();
}
value DxScript::Func_ObjTrajectory3D_SetAlphaVariation(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTrajectoryObject3D* obj = dynamic_cast<DxScriptTrajectoryObject3D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetObjectPointer()->SetAlphaVariation(argv[1].as_real());
	return value();
}
value DxScript::Func_ObjTrajectory3D_SetComplementCount(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTrajectoryObject3D* obj = dynamic_cast<DxScriptTrajectoryObject3D*>(script->GetObjectPointer(id));
	if (obj)
		obj->GetObjectPointer()->SetComplementCount((int)argv[1].as_real());
	return value();
}

//DxScriptParticleListObject
value DxScript::Func_ObjParticleList_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)((int)argv[0].as_real());

	shared_ptr<DxScriptPrimitiveObject> obj;
	if (type == TypeObject::OBJ_PARTICLE_LIST_2D) {
		obj = std::make_shared<DxScriptParticleListObject2D>();
	}
	else if (type == TypeObject::OBJ_PARTICLE_LIST_3D) {
		obj = std::make_shared<DxScriptParticleListObject3D>();
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return value(machine->get_engine()->get_real_type(), (double)id);
}
value DxScript::Func_ObjParticleList_SetPosition(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstancePosition(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetScale(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceScale(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
template<size_t ID>
value DxScript::Func_ObjParticleList_SetAngleSingle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceAngleSingle(ID, argv[1].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetAngle(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceAngle(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceColorRGB(argv[1].as_real(), argv[2].as_real(), argv[3].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceAlpha(argv[1].as_real());
	}
	return value();
}
value DxScript::Func_ObjParticleList_SetExtraData(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->SetInstanceUserData(D3DXVECTOR3(argv[1].as_real(), argv[2].as_real(), argv[3].as_real()));
	}
	return value();
}
value DxScript::Func_ObjParticleList_AddInstance(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->AddInstance();
	}
	return value();
}
value DxScript::Func_ObjParticleList_ClearInstance(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();

	DxScriptPrimitiveObject* obj = dynamic_cast<DxScriptPrimitiveObject*>(script->GetObjectPointer(id));
	if (obj) {
		ParticleRendererBase* objParticle = dynamic_cast<ParticleRendererBase*>(obj->GetObjectPointer());
		if (objParticle) objParticle->ClearInstance();
	}
	return value();
}

//Dx関数：オブジェクト操作(DxMesh)
value DxScript::Func_ObjMesh_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	shared_ptr<DxScriptMeshObject> obj = shared_ptr<DxScriptMeshObject>(new DxScriptMeshObject());
	int id = ID_INVALID;
	if (obj) {
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return value(machine->get_engine()->get_real_type(), (double)id);
}
value DxScript::Func_ObjMesh_Load(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj) {
		shared_ptr<DxMesh> mesh;
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);
		std::wstring ext = PathProperty::GetFileExtension(path);

		
		if (ext == L".mqo") {
			mesh = std::make_shared<MetasequoiaMesh>();
			res = mesh->CreateFromFile(path);
		}
		/*
		else if (ext == L".elem") {
			mesh = std::make_shared<ElfreinaMesh>();
			res = mesh->CreateFromFile(path);
		}
		*/
		if (res) {
			obj->mesh_ = mesh;
			//script->AddMeshResource(path, mesh);
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
value DxScript::Func_ObjMesh_SetColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetColor((int)argv[1].as_real(), (int)argv[2].as_real(), (int)argv[3].as_real());
	return value();
}
value DxScript::Func_ObjMesh_SetAlpha(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj)
		obj->SetAlpha((int)argv[1].as_real());
	return value();
}
value DxScript::Func_ObjMesh_SetAnimation(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring anime = argv[1].as_string();
		obj->anime_ = anime;
		obj->time_ = (int)argv[2].as_real();

		//	D3DXMATRIX mat = obj->mesh_->GetAnimationMatrix(anime, obj->time_, "悠久前部");
		//	D3DXVECTOR3 pos;
		//	D3DXVec3TransformCoord(&pos, &D3DXVECTOR3(0,0,0), &mat);
	}
	return value();
}
gstd::value DxScript::Func_ObjMesh_SetCoordinate2D(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj) {
		bool bEnable = argv[1].as_boolean();
		obj->bCoordinate2D_ = bEnable;
	}
	return value();
}
value DxScript::Func_ObjMesh_GetPath(script_machine* machine, int argc, const value* argv) {
	std::wstring res;
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptMeshObject* obj = dynamic_cast<DxScriptMeshObject*>(script->GetObjectPointer(id));
	if (obj) {
		DxMesh* mesh = obj->mesh_.get();
		if (mesh) res = mesh->GetPath();
	}
	return value(machine->get_engine()->get_string_type(), res);
}

//Dx関数：オブジェクト操作(DxText)
value DxScript::Func_ObjText_Create(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	shared_ptr<DxScriptTextObject> obj = shared_ptr<DxScriptTextObject>(new DxScriptTextObject());
	int id = ID_INVALID;
	if (obj) {
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return value(machine->get_engine()->get_real_type(), (double)id);
}
value DxScript::Func_ObjText_SetText(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring wstr = argv[1].as_string();
		obj->SetText(wstr);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring wstr = argv[1].as_string();
		obj->SetFontType(wstr);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontSize(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int size = (int)argv[1].as_real();
		obj->SetFontSize(size);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBold(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		bool bBold = argv[1].as_boolean();
		obj->SetFontWeight(bBold ? FW_BOLD : FW_NORMAL);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontWeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int weight = argv[1].as_real();
		if (weight < 0) weight = 0;
		else if (weight > 1000) weight = 1000;
		obj->SetFontWeight(weight);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontColorTop(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int r = (int)argv[1].as_real();
		int g = (int)argv[2].as_real();
		int b = (int)argv[3].as_real();
		obj->SetFontColorTop(r, g, b);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontColorBottom(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int r = (int)argv[1].as_real();
		int g = (int)argv[2].as_real();
		int b = (int)argv[3].as_real();
		obj->SetFontColorBottom(r, g, b);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int width = (int)argv[1].as_real();
		obj->SetFontBorderWidth(width);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderType(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int type = (int)argv[1].as_real();
		obj->SetFontBorderType(type);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontBorderColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int r = (int)argv[1].as_real();
		int g = (int)argv[2].as_real();
		int b = (int)argv[3].as_real();
		obj->SetFontBorderColor(r, g, b);
	}
	return value();
}
value DxScript::Func_ObjText_SetFontCharacterSet(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int cSet = (int)(argv[1].as_real() + 0.01);
		if (cSet < 0) cSet = 0;
		else if (cSet > 0xff) cSet = 0xff;
		obj->SetCharset((BYTE)cSet);
	}
	return value();
}
value DxScript::Func_ObjText_SetMaxWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int width = (int)argv[1].as_real();
		obj->SetMaxWidth(width);
	}
	return value();
}
value DxScript::Func_ObjText_SetMaxHeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int height = (int)argv[1].as_real();
		obj->SetMaxHeight(height);
	}
	return value();
}
value DxScript::Func_ObjText_SetLinePitch(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int pitch = (int)argv[1].as_real();
		obj->SetLinePitch(pitch);
	}
	return value();
}
value DxScript::Func_ObjText_SetSidePitch(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int pitch = (int)argv[1].as_real();
		obj->SetSidePitch(pitch);
	}
	return value();
}
value DxScript::Func_ObjText_SetVertexColor(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int a = (int)argv[1].as_real();
		int r = (int)argv[2].as_real();
		int g = (int)argv[3].as_real();
		int b = (int)argv[4].as_real();
		obj->SetVertexColor(D3DCOLOR_ARGB(a, r, g, b));
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		float centerX = argv[1].as_real();
		float centerY = argv[2].as_real();
		obj->center_ = D3DXVECTOR2(centerX, centerY);
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetAutoTransCenter(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		bool bAutoCenter = argv[1].as_boolean();
		obj->bAutoCenter_ = bAutoCenter;
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetHorizontalAlignment(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		int align = (int)argv[1].as_real();
		obj->SetHorizontalAlignment(align);
	}
	return value();
}
gstd::value DxScript::Func_ObjText_SetSyntacticAnalysis(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		bool bEnable = argv[1].as_boolean();
		obj->SetSyntacticAnalysis(bEnable);
	}
	return value();
}
value DxScript::Func_ObjText_GetTextLength(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring text = obj->GetText();
		res = StringUtility::CountAsciiSizeCharacter(text);
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
value DxScript::Func_ObjText_GetTextLengthCU(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::vector<int> listCount = obj->GetTextCountCU();
		for (int iLine = 0; iLine < listCount.size(); ++iLine) {
			int count = listCount[iLine];
			res += count;
		}
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
value DxScript::Func_ObjText_GetTextLengthCUL(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	std::vector<float> listCountD;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::vector<int> listCount = obj->GetTextCountCU();
		for (int iLine = 0; iLine < listCount.size(); ++iLine) {
			int count = listCount[iLine];
			listCountD.push_back(count);
		}
	}
	return script->CreateRealArrayValue(listCountD);
}
value DxScript::Func_ObjText_GetTotalWidth(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetTotalWidth();
	return value(machine->get_engine()->get_real_type(), (double)res);
}
value DxScript::Func_ObjText_GetTotalHeight(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxScriptTextObject* obj = dynamic_cast<DxScriptTextObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetTotalHeight();
	return value(machine->get_engine()->get_real_type(), (double)res);
}

//Dx関数：音声操作(DxSoundObject)
gstd::value DxScript::Func_ObjSound_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DirectSoundManager* manager = DirectSoundManager::GetBase();

	shared_ptr<DxSoundObject> obj = shared_ptr<DxSoundObject>(new DxSoundObject());
	obj->manager_ = script->objManager_.get();

	int id = script->AddObject(obj);
	return value(machine->get_engine()->get_real_type(), (double)id);
}
gstd::value DxScript::Func_ObjSound_Load(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool bLoad = false;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);
		bLoad = obj->Load(path);
	}
	return value(machine->get_engine()->get_boolean_type(), bLoad);
}
gstd::value DxScript::Func_ObjSound_Play(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			//obj->Play();
			script->GetObjectManager()->ReserveSound(player, obj->GetStyle());
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_Stop(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			player->Stop();
			script->GetObjectManager()->DeleteReservedSound(player);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetVolumeRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double rate = argv[1].as_real();
			player->SetVolumeRate(rate);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetPanRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double rate = argv[1].as_real();
			player->SetPanRate(rate);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetFade(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double fade = argv[1].as_real();
			player->SetFade(fade);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			bool bLoop = (bool)argv[1].as_boolean();
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetLoopEnable(bLoop);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopTime(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double startTime = argv[1].as_real();
			double endTime = argv[2].as_real();
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetLoopStartTime(startTime);
			style.SetLoopEndTime(endTime);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetLoopSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double startSample = argv[1].as_real();
			double endSample = argv[2].as_real();

			WAVEFORMATEX fmt = obj->GetPlayer()->GetWaveFormat();
			double startTime = startSample / (double)fmt.nSamplesPerSec;
			double endTime = endSample / (double)fmt.nSamplesPerSec;;
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetLoopStartTime(startTime);
			style.SetLoopEndTime(endTime);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_Seek(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double seekTime = argv[1].as_real();
			if (obj->GetPlayer()->IsPlaying()) {
				player->Seek(seekTime);
				player->ResetStreamForSeek();
			}
			else
				obj->GetStyle().SetStartTime(seekTime);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SeekSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			double seekSample = argv[1].as_real();

			WAVEFORMATEX fmt = obj->GetPlayer()->GetWaveFormat();
			if (obj->GetPlayer()->IsPlaying()) {
				player->Seek((int64_t)seekSample);
				player->ResetStreamForSeek();
			}
			else
				obj->GetStyle().SetStartTime(seekSample / (double)fmt.nSamplesPerSec);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetRestartEnable(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			bool bRestart = (bool)argv[1].as_boolean();
			SoundPlayer::PlayStyle& style = obj->GetStyle();
			style.SetRestart(bRestart);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_SetSoundDivision(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			int div = (int)argv[1].as_real();
			player->SetSoundDivision(div);
		}
	}
	return value();
}
gstd::value DxScript::Func_ObjSound_IsPlaying(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool bPlay = false;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) bPlay = player->IsPlaying();
	}
	return value(machine->get_engine()->get_boolean_type(), bPlay);
}
gstd::value DxScript::Func_ObjSound_GetVolumeRate(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double rate = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) rate = player->GetVolumeRate();
	}
	return value(machine->get_engine()->get_real_type(), (double)rate);
}
gstd::value DxScript::Func_ObjSound_GetWavePosition(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double posSec = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetCurrentPosition();
			posSec = pos / player->GetWaveFormat().nAvgBytesPerSec;
		}
	}
	return value(machine->get_engine()->get_real_type(), posSec);
}
gstd::value DxScript::Func_ObjSound_GetWavePositionSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	size_t posSamp = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetCurrentPosition();
			posSamp = pos / player->GetWaveFormat().nBlockAlign;
		}
	}
	return value(machine->get_engine()->get_real_type(), (double)posSamp);
}
gstd::value DxScript::Func_ObjSound_GetTotalLength(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	double posSec = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetTotalAudioSize();
			posSec = pos / player->GetWaveFormat().nAvgBytesPerSec;
		}
	}
	return value(machine->get_engine()->get_real_type(), posSec);
}
gstd::value DxScript::Func_ObjSound_GetTotalLengthSampleCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	size_t posSamp = 0;
	DxSoundObject* obj = dynamic_cast<DxSoundObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<SoundPlayer> player = obj->GetPlayer();
		if (player) {
			DWORD pos = player->GetTotalAudioSize();
			posSamp = pos / player->GetWaveFormat().nBlockAlign;
		}
	}
	return value(machine->get_engine()->get_real_type(), (double)posSamp);
}

//Dx関数：ファイル操作(DxFileObject)
gstd::value DxScript::Func_ObjFile_Create(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	//	script->CheckRunInMainThread();
	TypeObject type = (TypeObject)((int)argv[0].as_real());

	shared_ptr<DxFileObject> obj;
	if (type == TypeObject::OBJ_FILE_TEXT) {
		obj = shared_ptr<DxFileObject>(new DxTextFileObject());
	}
	else if (type == TypeObject::OBJ_FILE_BINARY) {
		obj = shared_ptr<DxFileObject>(new DxBinaryFileObject());
	}

	int id = ID_INVALID;
	if (obj) {
		obj->Initialize();
		obj->manager_ = script->objManager_.get();
		id = script->AddObject(obj);
	}
	return value(machine->get_engine()->get_real_type(), (double)id);
}
gstd::value DxScript::Func_ObjFile_Open(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader && reader->Open()) {
			ManagedFileReader* ptrManaged = dynamic_cast<ManagedFileReader*>(reader.GetPointer());
			obj->isArchived_ = ptrManaged->IsArchived();

			res = obj->OpenR(path);
		}
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_ObjFile_OpenNW(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		std::wstring path = argv[1].as_string();
		path = PathProperty::GetUnique(path);

		obj->isArchived_ = false;

		//Cannot write to an archived file, fall back to read-only permission
		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader && reader->Open()) {
			ManagedFileReader* ptrManaged = dynamic_cast<ManagedFileReader*>(reader.GetPointer());
			if (ptrManaged->IsArchived()) {
				obj->isArchived_ = true;
				res = obj->OpenR(path);
			}
			else res = obj->OpenRW(path);
		}
		else res = obj->OpenRW(path);
	}
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_ObjFile_Store(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	bool res = false;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->isArchived_)
		res = obj->Store();
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_ObjFile_GetSize(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxFileObject* obj = dynamic_cast<DxFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<File> file = obj->GetFile();
		res = file != nullptr ? file->GetSize() : 0;
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}

//Dx関数：ファイル操作(DxTextFileObject)
gstd::value DxScript::Func_ObjFileT_GetLineCount(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	int res = 0;
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));
	if (obj)
		res = obj->GetLineCount();
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileT_GetLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_string_type(), std::wstring());

	int line = (int)argv[1].as_real();
	std::wstring res = obj->GetLineAsWString(line);
	return value(machine->get_engine()->get_string_type(), obj->GetLineAsWString(line));
}
gstd::value DxScript::Func_ObjFileT_SetLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));

	//Cannot write to an archived file
	if (obj && !obj->isArchived_) {
		int line = (int)argv[1].as_real();
		std::wstring text = argv[2].as_string();
		obj->SetLineAsWString(text, line);
	}

	return value();
}
gstd::value DxScript::Func_ObjFileT_SplitLineText(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_string_type(), std::wstring());

	int pos = (int)argv[1].as_real();
	std::wstring delim = argv[2].as_string();
	std::wstring line = obj->GetLineAsWString(pos);
	std::vector<std::wstring> list = StringUtility::Split(line, delim);

	return script->CreateStringArrayValue(list);
}
gstd::value DxScript::Func_ObjFileT_AddLine(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));

	//Cannot write to an archived file
	if (obj && !obj->isArchived_) obj->AddLine(argv[1].as_string());

	return value();
}
gstd::value DxScript::Func_ObjFileT_ClearLine(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxTextFileObject* obj = dynamic_cast<DxTextFileObject*>(script->GetObjectPointer(id));

	//Cannot write to an archived file
	if (obj && !obj->isArchived_) obj->ClearLine();

	return value();
}

//Dx関数：ファイル操作(DxBinalyFileObject)
gstd::value DxScript::Func_ObjFileB_SetByteOrder(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		int order = (int)argv[1].as_real();
		obj->SetByteOrder(order);
	}
	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_SetCharacterCode(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		unsigned int code = (unsigned int)argv[1].as_real();
		obj->SetCodePage(code);
	}
	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_GetPointer(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	size_t res = 0;
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
		res = buffer->GetOffset();
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileB_Seek(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj) {
		int pos = (int)argv[1].as_real();
		gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
		buffer->Seek(pos);
	}
	return gstd::value();
}
gstd::value DxScript::Func_ObjFileB_ReadBoolean(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_real_type(), (double)-1);
	if (!obj->IsReadableSize(1))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	bool res = buffer->ReadBoolean();
	return value(machine->get_engine()->get_boolean_type(), res);
}
gstd::value DxScript::Func_ObjFileB_ReadByte(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_real_type(), (double)-1);
	if (!obj->IsReadableSize(1))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	double res = buffer->ReadCharacter();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjFileB_ReadShort(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_real_type(), (double)0);
	if (!obj->IsReadableSize(2))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	short bv = buffer->ReadShort();
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
		ByteOrder::Reverse(&bv, sizeof(bv));

	double res = bv;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjFileB_ReadInteger(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_real_type(), (double)-1);
	if (!obj->IsReadableSize(4))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	int bv = buffer->ReadInteger();
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
		ByteOrder::Reverse(&bv, sizeof(bv));

	double res = bv;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjFileB_ReadLong(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_real_type(), (double)-1);
	if (!obj->IsReadableSize(8))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	int64_t bv = buffer->ReadInteger64();
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG)
		ByteOrder::Reverse(&bv, sizeof(bv));
	double res = bv;
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjFileB_ReadFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_real_type(), (double)-1);
	if (!obj->IsReadableSize(4))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	double res = 0;
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG) {
		int bv = buffer->ReadInteger();
		ByteOrder::Reverse(&bv, sizeof(bv));
		res = (double&)bv;
	}
	else
		res = buffer->ReadFloat();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjFileB_ReadDouble(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_real_type(), (double)-1);
	if (!obj->IsReadableSize(8))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	double res = 0;
	if (obj->GetByteOrder() == ByteOrder::ENDIAN_BIG) {
		int64_t bv = buffer->ReadInteger64();
		ByteOrder::Reverse(&bv, sizeof(bv));
		res = (double&)bv;
	}
	else
		res = buffer->ReadDouble();
	return value(machine->get_engine()->get_real_type(), res);
}
gstd::value DxScript::Func_ObjFileB_ReadString(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj == nullptr)return value(machine->get_engine()->get_string_type(), std::wstring());

	size_t readSize = (size_t)argv[1].as_real();
	if (!obj->IsReadableSize(readSize))
		script->RaiseError(gstd::ErrorUtility::GetErrorMessage(ErrorUtility::ERROR_END_OF_FILE));

	std::vector<byte> data;
	data.resize(readSize);

	gstd::ref_count_ptr<gstd::ByteBuffer> buffer = obj->GetBuffer();
	buffer->Read(&data[0], readSize);

	std::wstring res;
	int code = obj->GetCodePage();
	if (code == CODE_ACP || code == CODE_UTF8) {
		std::string str;
		str.resize(readSize);
		memcpy(&str[0], &data[0], readSize);

		res = StringUtility::ConvertMultiToWide(str, code == CODE_UTF8 ? CP_UTF8 : CP_ACP);
	}
	else if (code == CODE_UTF16LE || code == CODE_UTF16BE) {
		size_t strSize = readSize / 2 * 2;
		size_t wsize = strSize / 2;

		res.resize(wsize);
		memcpy(&res[0], &data[0], readSize);

		if (code == CODE_UTF16BE) {
			for (auto itr = res.begin(); itr != res.end(); ++itr) {
				wchar_t ch = *itr;
				*itr = (ch >> 8) | (ch << 8);
			}
		}
	}

	return value(machine->get_engine()->get_string_type(), res);
}

gstd::value DxScript::Func_ObjFileB_WriteBoolean(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		bool data = argv[1].as_boolean();
		res = obj->GetBuffer()->Write(&data, sizeof(bool));
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileB_WriteByte(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		byte data = (byte)argv[1].as_real();
		DWORD res = obj->GetBuffer()->Write(&data, sizeof(byte));
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileB_WriteShort(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		int16_t data = (int16_t)argv[1].as_real();
		DWORD res = obj->GetBuffer()->Write(&data, sizeof(int16_t));
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileB_WriteInteger(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		int32_t data = (int32_t)argv[1].as_real();
		DWORD res = obj->GetBuffer()->Write(&data, sizeof(int32_t));
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileB_WriteLong(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		int64_t data = (int64_t)argv[1].as_real();
		DWORD res = obj->GetBuffer()->Write(&data, sizeof(int64_t));
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileB_WriteFloat(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		float data = (float)argv[1].as_real();
		DWORD res = obj->GetBuffer()->Write(&data, sizeof(float));
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
gstd::value DxScript::Func_ObjFileB_WriteDouble(gstd::script_machine* machine, int argc, const gstd::value* argv) {
	DxScript* script = (DxScript*)machine->data;
	DWORD res = 0;

	int id = (int)argv[0].as_real();
	DxBinaryFileObject* obj = dynamic_cast<DxBinaryFileObject*>(script->GetObjectPointer(id));
	if (obj && !obj->IsArchived()) {
		double data = argv[1].as_real();
		DWORD res = obj->GetBuffer()->Write(&data, sizeof(double));
	}
	return value(machine->get_engine()->get_real_type(), (double)res);
}
