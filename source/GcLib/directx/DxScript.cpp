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
	typeBlend_ = MODE_BLEND_ALPHA;
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
	objRender_->SetDxObjectReference(this);

	bZWrite_ = false;
	bZTest_ = false;
}
void DxScriptPrimitiveObject2D::Render() {
	if (RenderObjectTLX* obj = GetObjectPointer()) {
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
	graphics->SetTextureFilter(filterMin_, filterMag_, filterMip_);
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
D3DCOLOR DxScriptPrimitiveObject2D::GetVertexColor(size_t index) {
	RenderObjectTLX* obj = GetObjectPointer();
	return obj->GetVertexColor(index);
}
void DxScriptPrimitiveObject2D::SetPermitCamera(bool bPermit) {
	RenderObjectTLX* obj = GetObjectPointer();
	obj->SetPermitCamera(bPermit);
}
D3DXVECTOR3 DxScriptPrimitiveObject2D::GetVertexPosition(size_t index) {
	D3DXVECTOR3 res(0, 0, 0);
	if (!IsValidVertexIndex(index)) return res;
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
	objRender_->SetDxObjectReference(this);
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
	objRender_->SetDxObjectReference(this);
}
void DxScriptSpriteListObject2D::CleanUp() {
	GetSpritePointer()->CleanUp();
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
	objRender_->SetDxObjectReference(this);
	bZWrite_ = false;
	bZTest_ = true;
	bFogEnable_ = true;
}
void DxScriptPrimitiveObject3D::Render() {
	if (RenderObjectLX* obj = GetObjectPointer()) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		bool bEnvFogEnable = graphics->IsFogEnable();
		SetRenderState();
		//obj->Render();
		obj->Render(angX_, angY_, angZ_);

		if (bEnvFogEnable)
			graphics->SetFogEnable(true);
	}
}
void DxScriptPrimitiveObject3D::SetRenderState() {
	if (shared_ptr<DxScriptRenderObject> objRelative = objRelative_.lock()) {
		if (DxScriptMeshObject* objMesh = dynamic_cast<DxScriptMeshObject*>(objRelative.get())) {
			objRelative->SetRenderState();

			int frameAnime = objMesh->GetAnimeFrame();
			std::wstring nameAnime = objMesh->GetAnimeName();
			shared_ptr<DxMesh> mesh = objMesh->GetMesh();
			shared_ptr<D3DXMATRIX> mat = std::make_shared<D3DXMATRIX>();
			*mat = mesh->GetAnimationMatrix(nameAnime, frameAnime, nameRelativeBone_);
			objRender_->SetRalativeMatrix(mat);
		}
	}

	DirectGraphics* graphics = DirectGraphics::GetBase();
	RenderObjectLX* obj = GetObjectPointer();

	bool bEnvFogEnable = graphics->IsFogEnable();
	if (bEnvFogEnable)
		graphics->SetFogEnable(bFogEnable_);

	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	obj->GetLighting()->Apply();
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	graphics->SetTextureFilter(filterMin_, filterMag_, filterMip_);
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
	if (!IsValidVertexIndex(index)) return;
	RenderObjectLX* obj = GetObjectPointer();
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	obj->SetVertexColorRGB(index, r, g, b);
}
D3DCOLOR DxScriptPrimitiveObject3D::GetVertexColor(size_t index) {
	RenderObjectLX* obj = GetObjectPointer();
	return obj->GetVertexColor(index);
}
D3DXVECTOR3 DxScriptPrimitiveObject3D::GetVertexPosition(size_t index) {
	D3DXVECTOR3 res(0, 0, 0);
	if (!IsValidVertexIndex(index)) return res;
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
	objRender_->SetDxObjectReference(this);
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
	if (TrajectoryObject3D* obj = GetObjectPointer()) {
		if (shared_ptr<DxScriptRenderObject> objRelative = objRelative_.lock()) {
			if (DxScriptMeshObject* objMesh = dynamic_cast<DxScriptMeshObject*>(objRelative.get())) {
				objRelative->SetRenderState();
				int frameAnime = objMesh->GetAnimeFrame();
				std::wstring nameAnime = objMesh->GetAnimeName();
				shared_ptr<DxMesh> mesh = objMesh->GetMesh();
				D3DXMATRIX matAnime = mesh->GetAnimationMatrix(nameAnime, frameAnime, nameRelativeBone_);

				TrajectoryObject3D* objRender = GetObjectPointer();
				objRender->AddPoint(matAnime);
			}
		}

		obj->Work();
	}
}
void DxScriptTrajectoryObject3D::Render() {
	if (TrajectoryObject3D* obj = GetObjectPointer()) {
		SetRenderState();
		//obj->Render();
		obj->Render(angX_, angY_, angZ_);
	}
}
void DxScriptTrajectoryObject3D::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	TrajectoryObject3D* obj = GetObjectPointer();
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	graphics->SetTextureFilter(filterMin_, filterMag_, filterMip_);
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
	objRender_->SetDxObjectReference(this);
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
	graphics->SetTextureFilter(filterMin_, filterMag_, filterMip_);
}
void DxScriptParticleListObject2D::CleanUp() {
	GetParticlePointer()->ClearInstance();
}
/**********************************************************
//DxScriptParticleListObject3D
**********************************************************/
DxScriptParticleListObject3D::DxScriptParticleListObject3D() {
	typeObject_ = TypeObject::OBJ_PARTICLE_LIST_3D;
	objRender_ = std::make_shared<ParticleRenderer3D>();
	objRender_->SetDxObjectReference(this);
}
void DxScriptParticleListObject3D::Render() {
	ParticleRenderer3D* obj = GetParticlePointer();
	DirectGraphics* graphics = DirectGraphics::GetBase();

	bool bEnvFogEnable = graphics->IsFogEnable();

	SetRenderState();
	obj->Render();

	graphics->SetFogEnable(bEnvFogEnable);
}
void DxScriptParticleListObject3D::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	bool bEnvFogEnable = graphics->IsFogEnable();
	if (bEnvFogEnable)
		graphics->SetFogEnable(bFogEnable_);

	ParticleRenderer3D* obj = GetParticlePointer();
	obj->SetPosition(position_);
	obj->SetAngle(angle_);
	obj->SetScale(scale_);
	//obj->GetLighting()->Apply();
	graphics->SetLightingEnable(false);
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);
	graphics->SetTextureFilter(filterMin_, filterMag_, filterMip_);
}
void DxScriptParticleListObject3D::CleanUp() {
	GetParticlePointer()->ClearInstance();
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
}

void DxScriptMeshObject::Render() {
	if (mesh_ == nullptr) return;
	DirectGraphics* graphics = DirectGraphics::GetBase();
	bool bEnvFogEnable = graphics->IsFogEnable();

	SetRenderState();
	mesh_->Render(anime_, time_, angX_, angY_, angZ_);

	graphics->SetFogEnable(bEnvFogEnable);
}
void DxScriptMeshObject::SetRenderState() {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	if (graphics->IsFogEnable())
		graphics->SetFogEnable(bFogEnable_);

	mesh_->SetPosition(position_);
	mesh_->SetAngle(angle_);
	mesh_->SetScale(scale_);
	mesh_->SetColor(color_);
	mesh_->SetCoordinate2D(bCoordinate2D_);

	mesh_->GetLighting()->Apply();
	graphics->SetZWriteEnable(bZWrite_);
	graphics->SetZBufferEnable(bZTest_);
	
	graphics->SetBlendMode(typeBlend_);
	graphics->SetCullingMode(modeCulling_);

	graphics->SetTextureFilter(filterMin_, filterMag_, filterMip_);
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
void DxScriptMeshObject::SetShader(shared_ptr<Shader> shader) {
	if (mesh_ == nullptr) return;
	mesh_->SetShader(shader);
}
void DxScriptMeshObject::SetAngleX(float x) {
	x = gstd::Math::DegreeToRadian(x);
	if (angle_.x != x) {
		angle_.x = x;
		angX_ = D3DXVECTOR2(cosf(-x), sinf(-x));
	}
}
void DxScriptMeshObject::SetAngleY(float y) {
	y = gstd::Math::DegreeToRadian(y);
	if (angle_.y != y) {
		angle_.y = y;
		angY_ = D3DXVECTOR2(cosf(-y), sinf(-y));
	}
}
void DxScriptMeshObject::SetAngleZ(float z) {
	z = gstd::Math::DegreeToRadian(z);
	if (angle_.z != z) {
		angle_.z = z;
		angZ_ = D3DXVECTOR2(cosf(-z), sinf(-z));
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
	graphics->SetTextureFilter(filterMin_, filterMag_, D3DTEXF_NONE);
}
void DxScriptTextObject::_UpdateRenderer() {
	if (bChange_) {
		textInfo_ = text_.GetTextInfo();
		objRender_ = text_.CreateRenderObject(textInfo_);
	}
	bChange_ = false;
}
void DxScriptTextObject::SetCharset(BYTE set) {
	/*
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
	*/
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
	if (player_ == nullptr) return;
	player_->Delete();
}
bool DxSoundObject::Load(std::wstring path) {
	DirectSoundManager* manager = DirectSoundManager::GetBase();
	player_ = manager->GetPlayer(path);
	return player_ != nullptr;
}
void DxSoundObject::Play() {
	if (player_)
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
	bool bDir = File::CreateFileDirectory(dir);
	if (!bDir) return false;

	//Security; to prevent scripts from being able to access external files
	std::wstring dirModule = PathProperty::GetModuleDirectory();
	if (dir.find(dirModule) == std::wstring::npos) {
		Logger::WriteTop(StringUtility::Format("DxFileObject: OpenNW cannot open external files. [%s]", path.c_str()));
		return false;
	}

	file_ = new File(path);
	bool res = file_->Open(File::WRITE);
	if (!res) file_ = nullptr;
	return res;
}
void DxFileObject::Close() {
	if (file_ == nullptr) return;
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
	if (size == 0) return true;

	std::vector<char> text;
	text.resize(size);
	reader_->SetFilePointerBegin();
	reader_->Read(&text[0], size);

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
	file_->SetFilePointerBegin();
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
	if (file_ == nullptr) return false;

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
	if (!res) return false;

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
	listObjRender_.resize(capacity);
	listShader_.resize(capacity);
}
void DxScriptObjectManager::_ArrangeActiveObjectList() {
	for (auto itr = listActiveObject_.begin(); itr != listActiveObject_.end();) {
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
		if (res) Logger::WriteTop(StringUtility::Format("DxScriptObjectManager: Object pool expansion. [%d->%d]",
			oldSize, obj_.size()));
		return res;
	};

	{
		/*
		do {
			if (listUnusedIndex_.size() == 0U) {
				if (!ExpandContainerCapacity()) break;
			}
			res = listUnusedIndex_.front();
			listUnusedIndex_.pop_front();
		} while (obj_[res] != nullptr);
		*/
		if (listUnusedIndex_.size() == 0U) {
			if (!ExpandContainerCapacity()) return res;
		}
		res = listUnusedIndex_.front();
		listUnusedIndex_.pop_front();

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
	if (id < 0 || id >= obj_.size()) return nullptr;
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
	if (index < 0 || index >= listShader_.size()) return nullptr;
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
void DxScriptObjectManager::WorkObject() {
	for (auto itr = listActiveObject_.begin(); itr != listActiveObject_.end();) {
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
	for (auto itrSound = mapReservedSound_.begin(); itrSound != mapReservedSound_.end(); ++itrSound) {
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

	for (size_t iPri = 0; iPri < listObjRender_.size(); ++iPri) {
		ID3DXEffect* effect = nullptr;
		UINT cPass = 1;
		if (shared_ptr<Shader> shader = listShader_[iPri]) {
			effect = shader->GetEffect();
			shader->LoadParameter();
			effect->Begin(&cPass, 0);
		}

		RenderList& renderList = listObjRender_[iPri];

		for (UINT iPass = 0; iPass < cPass; ++iPass) {
			if (effect) effect->BeginPass(iPass);
			for (auto itr = renderList.begin(); itr != renderList.end(); ++itr) {
				if (DxScriptRenderObject* obj = dynamic_cast<DxScriptRenderObject*>(itr->get()))
					obj->Render();
			}
			if (effect) effect->EndPass();
		}
		renderList.Clear();

		if (effect) effect->End();
	}
}
void DxScriptObjectManager::CleanupObject() {
	//No need to check for object validity here, only nullptr check is enough
	for (auto itr = listActiveObject_.begin(); itr != listActiveObject_.end(); ++itr) {
		if (shared_ptr<DxScriptObjectBase> obj = *itr)
			obj->CleanUp();
	}
}

void DxScriptObjectManager::RenderList::Add(shared_ptr<DxScriptObjectBase>& ptr) {
	if (size >= list.size()) list.push_back(ptr);
	else list[size++] = ptr;
}
void DxScriptObjectManager::RenderList::Clear() {
	size = 0U;
	std::fill(list.begin(), list.end(), nullptr);
}
void DxScriptObjectManager::PrepareRenderObject() {
	for (auto itr = listActiveObject_.begin(); itr != listActiveObject_.end(); ++itr) {
		shared_ptr<DxScriptObjectBase> obj = (*itr);
		if (obj == nullptr || obj->IsDeleted()) continue;
		if (!obj->IsVisible()) continue;
		AddRenderObject(obj);
	}
}
void DxScriptObjectManager::AddRenderObject(shared_ptr<DxScriptObjectBase> obj) {
	size_t renderSize = listObjRender_.size();

	int tPri = obj->priRender_;
	if (tPri < 0) tPri = 0;
	else if (tPri > renderSize - 1) tPri = renderSize - 1;
	listObjRender_[tPri].Add(obj);
}
void DxScriptObjectManager::ClearRenderObject() {
	for (size_t iPri = 0; iPri < listObjRender_.size(); ++iPri) {
		listObjRender_[iPri].Clear();
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
