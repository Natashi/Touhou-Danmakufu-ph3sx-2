#include "source/GcLib/pch.h"

#include "RenderObject.hpp"

#include "DirectGraphics.hpp"
#include "Shader.hpp"

#include "MetasequoiaMesh.hpp"
//#include "ElfreinaMesh.hpp"

#include "DxScript.hpp"

#include "HLSL.hpp"

using namespace gstd;
using namespace directx;

//****************************************************************************
//DirectionalLightingState
//****************************************************************************
DirectionalLightingState::DirectionalLightingState() {
	bLightEnable_ = false;
	bSpecularEnable_ = false;

	ZeroMemory(&light_, sizeof(D3DLIGHT9));
	light_.Type = D3DLIGHT_DIRECTIONAL;
	light_.Diffuse = { 0.5f, 0.5f, 0.5f, 0.0f };
	light_.Specular = { 0.0f, 0.0f, 0.0f, 0.0f };
	light_.Ambient = { 0.5f, 0.5f, 0.5f, 0.0f };
	light_.Direction = D3DXVECTOR3(-1, -1, -1);
}
DirectionalLightingState::~DirectionalLightingState() {

}
void DirectionalLightingState::Apply() {
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	device->SetRenderState(D3DRS_LIGHTING, bLightEnable_);
	device->SetRenderState(D3DRS_SPECULARENABLE, bLightEnable_ ? bSpecularEnable_ : false);
	device->LightEnable(0, bLightEnable_);
	if (bLightEnable_) device->SetLight(0, &light_);
}

//****************************************************************************
//RenderObject
//****************************************************************************
RenderObject::RenderObject() {
	typePrimitive_ = D3DPT_TRIANGLELIST;
	position_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	angle_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	posWeightCenter_ = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	bCoordinate2D_ = false;

	bVertexShaderMode_ = false;
	flgUseVertexBufferMode_ = true;

	disableMatrixTransform_ = false;
}
RenderObject::~RenderObject() {
}
size_t RenderObject::_GetPrimitiveCount() {
	return _GetPrimitiveCount(GetVertexCount());
}
size_t RenderObject::_GetPrimitiveCount(size_t count) {
	switch (typePrimitive_) {
	case D3DPT_POINTLIST:
		return count;
	case D3DPT_LINELIST:
		return count / 2U;
	case D3DPT_LINESTRIP:
		return (count > 0U ? count - 1U : 0U);
	case D3DPT_TRIANGLELIST:
		return count / 3U;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_TRIANGLEFAN:
		return (count > 1U ? count - 2U : 0U);
	}
	return 0U;
}

void RenderObject::SetPosition(float x, float y, float z) {
	position_ = D3DXVECTOR3(x, y, z);
	D3DXVec3Scale(&position_, &position_, DirectGraphics::g_dxCoordsMul_);
}
void RenderObject::SetX(float x) {
	position_.x = x * DirectGraphics::g_dxCoordsMul_;
}
void RenderObject::SetY(float y) {
	position_.y = y * DirectGraphics::g_dxCoordsMul_;
}
void RenderObject::SetZ(float z) {
	position_.z = z * DirectGraphics::g_dxCoordsMul_;
}
void RenderObject::SetScaleXYZ(float sx, float sy, float sz) {
	scale_ = D3DXVECTOR3(sx, sy, sz);
	D3DXVec3Scale(&scale_, &scale_, DirectGraphics::g_dxCoordsMul_);
}

//---------------------------------------------------------------------

D3DXMATRIX RenderObject::CreateWorldMatrix(const D3DXVECTOR3& position, const D3DXVECTOR3& scale,
	const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ,
	const D3DXMATRIX* matRelative, bool bCoordinate2D)
{
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	DxMath::ConstructRotationMatrix(&mat, angleX, angleY, angleZ);

	bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
	bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

	if (bScale || bPos) {
		if (bScale) {
			DxMath::MatrixApplyScaling(&mat, scale);
		}
		if (bPos) {
			mat._41 = position.x;
			mat._42 = position.y;
			mat._43 = position.z;
		}
	}
	if (matRelative) D3DXMatrixMultiply(&mat, &mat, matRelative);

	if (bCoordinate2D) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		D3DVIEWPORT9 viewPort;
		device->GetViewport(&viewPort);
		float width = viewPort.Width * 0.5f;
		float height = viewPort.Height * 0.5f;

		ref_count_ptr<DxCamera> camera = graphics->GetCamera();
		ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
		if (camera2D->IsEnable()) {
			D3DXMATRIX matCamera = camera2D->GetMatrix();
			mat = mat * matCamera;
		}

		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, -width, -height, 0);

		D3DXMATRIX matScale;
		D3DXMatrixScaling(&matScale, 200.0f, 200.0f, -0.002f);

		const D3DXMATRIX& matInvView = camera->GetViewInverseMatrix();
		const D3DXMATRIX& matInvProj = camera->GetProjectionInverseMatrix();
		D3DXMATRIX matInvViewPort;
		D3DXMatrixIdentity(&matInvViewPort);
		matInvViewPort._11 = 1.0f / width;
		matInvViewPort._22 = -1.0f / height;
		matInvViewPort._41 = -1.0f;
		matInvViewPort._42 = 1.0f;

		mat = mat * matTrans * matScale * matInvViewPort * matInvProj * matInvView;
		mat._43 *= -1;
	}

	return mat;
}
D3DXMATRIX RenderObject::CreateWorldMatrix(const D3DXVECTOR3& position, const D3DXVECTOR3& scale, 
	const D3DXVECTOR3& angle, const D3DXMATRIX* matRelative, bool bCoordinate2D)
{
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	{
		D3DXMATRIX matSRT;
		D3DXMatrixIdentity(&matSRT);

		D3DXMatrixRotationYawPitchRoll(&matSRT, angle.y, angle.x, angle.z);

		bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
		bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

		if (bScale) {
			DxMath::MatrixApplyScaling(&matSRT, scale);
		}
		if (bPos) {
			matSRT._41 = position.x;
			matSRT._42 = position.y;
			matSRT._43 = position.z;
		}

		D3DXMatrixMultiply(&mat, &mat, &matSRT);
	}
	if (matRelative) D3DXMatrixMultiply(&mat, &mat, matRelative);

	if (bCoordinate2D) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		IDirect3DDevice9* device = graphics->GetDevice();

		D3DVIEWPORT9 viewPort;
		device->GetViewport(&viewPort);
		float width = viewPort.Width * 0.5f;
		float height = viewPort.Height * 0.5f;

		ref_count_ptr<DxCamera> camera = graphics->GetCamera();
		ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
		if (camera2D->IsEnable()) {
			D3DXMatrixMultiply(&mat, &mat, &camera2D->GetMatrix());
		}

		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, -width, -height, 0);

		D3DXMATRIX matScale;
		D3DXMatrixScaling(&matScale, 200.0f, 200.0f, -0.002f);

		const D3DXMATRIX& matInvView = camera->GetViewInverseMatrix();
		const D3DXMATRIX& matInvProj = camera->GetProjectionInverseMatrix();
		D3DXMATRIX matInvViewPort;
		D3DXMatrixIdentity(&matInvViewPort);
		matInvViewPort._11 = 1.0f / width;
		matInvViewPort._22 = -1.0f / height;
		matInvViewPort._41 = -1.0f;
		matInvViewPort._42 = 1.0f;

		mat = mat * matTrans * matScale * matInvViewPort * matInvProj * matInvView;
		mat._43 *= -1;
	}

	return mat;
}

D3DXMATRIX RenderObject::CreateWorldMatrixSprite3D(const D3DXVECTOR3& position, const D3DXVECTOR3& scale,
	const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ,
	const D3DXMATRIX* matRelative, bool bBillboard)
{
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	DxMath::ConstructRotationMatrix(&mat, angleX, angleY, angleZ);

	bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
	bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

	if (bScale) {
		DxMath::MatrixApplyScaling(&mat, scale);
	}
	if (bBillboard) {
		DirectGraphics* graph = DirectGraphics::GetBase();
		D3DXMatrixMultiply(&mat, &mat, &graph->GetCamera()->GetViewTransposedMatrix());
	}
	if (bPos) {
		D3DXMATRIX matTrans;
		D3DXMatrixTranslation(&matTrans, position.x, position.y, position.z);
		D3DXMatrixMultiply(&mat, &mat, &matTrans);
	}

	if (matRelative) {
		if (bBillboard) {
			D3DXMATRIX matRelativeE;
			D3DXVECTOR3 pos;
			D3DXVec3TransformCoord(&pos, &D3DXVECTOR3(0, 0, 0), matRelative);
			D3DXMatrixTranslation(&matRelativeE, pos.x, pos.y, pos.z);
			D3DXMatrixMultiply(&mat, &mat, &matRelativeE);
		}
		else {
			D3DXMatrixMultiply(&mat, &mat, matRelative);
		}
	}
	return mat;
}

D3DXMATRIX RenderObject::CreateWorldMatrix2D(const D3DXVECTOR3& position, const D3DXVECTOR3& scale,
	const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ, const D3DXMATRIX* matCamera)
{
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);

	DxMath::ConstructRotationMatrix(&mat, angleX, angleY, angleZ);

	bool bScale = scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f;
	bool bPos = position.x != 0.0f || position.y != 0.0f || position.z != 0.0f;

	if (bScale || bPos) {
		if (bScale) {
			DxMath::MatrixApplyScaling(&mat, scale);
		}
		if (bPos) {
			mat._41 = position.x;
			mat._42 = position.y;
			mat._43 = position.z;
		}
	}
	if (matCamera) D3DXMatrixMultiply(&mat, &mat, matCamera);

	return mat;
}
D3DXMATRIX RenderObject::CreateWorldMatrixText2D(const D3DXVECTOR2& centerPosition, const D3DXVECTOR3& scale,
	const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ,
	const D3DXVECTOR2& objectPosition, const D3DXVECTOR2& biasPosition, const D3DXMATRIX* matCamera)
{
	D3DXMATRIX mat;
	D3DXMatrixTranslation(&mat, -centerPosition.x, -centerPosition.y, 0.0f);

	{
		D3DXMATRIX matSRT;
		D3DXMatrixIdentity(&matSRT);

		DxMath::ConstructRotationMatrix(&matSRT, angleX, angleY, angleZ);

		if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
			DxMath::MatrixApplyScaling(&matSRT, scale);
		}
		if (objectPosition.x != 0.0f || objectPosition.y != 0.0f) {
			matSRT._41 = objectPosition.x;
			matSRT._42 = objectPosition.y;
			matSRT._43 = 0.0f;
		}

		D3DXMatrixMultiply(&matSRT, &mat, &matSRT);
		D3DXMatrixTranslation(&mat, centerPosition.x + biasPosition.x, centerPosition.y + biasPosition.y, 0.0f);
		D3DXMatrixMultiply(&mat, &matSRT, &mat);
	}
	if (matCamera) D3DXMatrixMultiply(&mat, &mat, matCamera);

	return mat;
}

//---------------------------------------------------------------------

void RenderObject::SetCoordinate2dDeviceMatrix() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	float width = graphics->GetScreenWidth();
	float height = graphics->GetScreenHeight();

	D3DVIEWPORT9 viewPort;
	device->GetViewport(&viewPort);

	D3DXMATRIX viewMat;
	D3DXMATRIX persMat;
	D3DVECTOR viewFrom = D3DXVECTOR3(0, 0, -100);
	D3DXMatrixLookAtLH(&viewMat, (D3DXVECTOR3*)&viewFrom, &D3DXVECTOR3(0, 0, 0), &D3DXVECTOR3(0, 1, 0));
	D3DXMatrixPerspectiveFovLH(&persMat, D3DXToRadian(180),
		(float)viewPort.Width / (float)viewPort.Height, 1, 2000);

	viewMat = viewMat * persMat;

	device->SetTransform(D3DTS_VIEW, &viewMat);
}

//****************************************************************************
//RenderObjectTLX
//座標3D変換済み、ライティング済み、テクスチャ有り
//****************************************************************************
RenderObjectTLX::RenderObjectTLX() {
	strideVertexStreamZero_ = sizeof(VERTEX_TLX);
	bPermitCamera_ = true;
}
RenderObjectTLX::~RenderObjectTLX() {

}
void RenderObjectTLX::Render() {
	RenderObjectTLX::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void RenderObjectTLX::Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera2D> camera = graphics->GetCamera2D();
	bool bCamera = camera->IsEnable() && bPermitCamera_;

	D3DXMATRIX matWorld;
	if (!disableMatrixTransform_) {
		matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
			angX, angY, angZ, bCamera ? &camera->GetMatrix() : nullptr);
	}
	else {
		matWorld = camera->GetMatrix();
	}

	RenderObjectTLX::Render(matWorld);
}
void RenderObjectTLX::Render(const D3DXMATRIX& matTransform) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera2D> camera = graphics->GetCamera2D();
	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();

	IDirect3DDevice9* device = graphics->GetDevice();
	device->SetTexture(0, texture_ ? texture_->GetD3DTexture() : nullptr);

	device->SetFVF(VERTEX_TLX::fvf);

	{
		bool bUseIndex = vertexIndices_.size() > 0;
		size_t countVertex = std::min(GetVertexCount(), 65536U);
		size_t countIndex = std::min(vertexIndices_.size(), 65536U);
		size_t countPrim = std::min(_GetPrimitiveCount(bUseIndex ? countIndex : countVertex), 65536U);

		if (!bVertexShaderMode_) {
			vertCopy_ = vertex_;
			for (size_t iVert = 0; iVert < countVertex; ++iVert) {
				size_t pos = iVert * strideVertexStreamZero_;
				VERTEX_TLX* vertex = (VERTEX_TLX*)&vertCopy_[pos];
				D3DXVECTOR4* vPos = &vertex->position;
				D3DXVec3TransformCoord((D3DXVECTOR3*)vPos, (D3DXVECTOR3*)vPos, &matTransform);
			}
		}

		RenderShaderLibrary* shaderLib = ShaderManager::GetBase()->GetRenderLib();

		VertexBufferManager* vbManager = VertexBufferManager::GetBase();
		FixedVertexBuffer* vertexBuffer = vbManager->GetVertexBufferTLX();
		FixedIndexBuffer* indexBuffer = vbManager->GetIndexBuffer();

		if (flgUseVertexBufferMode_ || bVertexShaderMode_) {
			BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

			lockParam.SetSource(bVertexShaderMode_ ? vertex_ : vertCopy_, countVertex, sizeof(VERTEX_TLX));
			vertexBuffer->UpdateBuffer(&lockParam);
			
			if (bUseIndex) {
				lockParam.SetSource(vertexIndices_, countIndex, sizeof(uint16_t));
				indexBuffer->UpdateBuffer(&lockParam);
			}
		}

		device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_TLX));
		device->SetIndices(indexBuffer->GetBuffer());

		{
			UINT countPass = 1;
			ID3DXEffect* effect = nullptr;
			if (shader_) {
				effect = shader_->GetEffect();

				if (shader_->LoadTechnique()) {
					shader_->LoadParameter();

					if (bVertexShaderMode_) {
						device->SetVertexDeclaration(shaderLib->GetVertexDeclarationTLX());

						D3DXHANDLE handle = nullptr;
						if (handle = effect->GetParameterBySemantic(nullptr, "WORLD"))
							effect->SetMatrix(handle, &matTransform);
						if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION")) {
							effect->SetMatrix(handle, &graphics->GetViewPortMatrix());
						}
					}
				}
				
				effect->Begin(&countPass, 0);
			}
			for (UINT iPass = 0; iPass < countPass; ++iPass) {
				if (effect) effect->BeginPass(iPass);

				if (flgUseVertexBufferMode_ || bVertexShaderMode_) {
					if (bUseIndex) device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
					else device->DrawPrimitive(typePrimitive_, 0, countPrim);
				}
				else {
					if (bUseIndex)
						device->DrawIndexedPrimitiveUP(typePrimitive_, 0, countVertex, countPrim,
							vertexIndices_.data(), D3DFMT_INDEX16, vertCopy_.data(), strideVertexStreamZero_);
					else
						device->DrawPrimitiveUP(typePrimitive_, countPrim, vertCopy_.data(), strideVertexStreamZero_);
				}

				if (effect) effect->EndPass();
			}
			if (effect) effect->End();
		}

		device->SetIndices(nullptr);
		device->SetVertexDeclaration(nullptr);
	}
}

void RenderObjectTLX::SetVertexCount(size_t count) {
	RenderObject::SetVertexCount(count);
	SetColorRGB(D3DCOLOR_ARGB(255, 255, 255, 255));
	SetAlpha(255);
}
VERTEX_TLX* RenderObjectTLX::GetVertex(size_t index) {
	size_t pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.size()) return nullptr;
	return (VERTEX_TLX*)&vertex_[pos];
}
void RenderObjectTLX::SetVertex(size_t index, const VERTEX_TLX& vertex) {
	size_t pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.size()) return;
	memcpy(&vertex_[pos], &vertex, strideVertexStreamZero_);
}
void RenderObjectTLX::SetVertexPosition(size_t index, float x, float y, float z, float w) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr) return;

	x *= DirectGraphics::g_dxCoordsMul_;
	y *= DirectGraphics::g_dxCoordsMul_;

	constexpr float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
	vertex->position.w = w;
}
void RenderObjectTLX::SetVertexUV(size_t index, float u, float v) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectTLX::SetVertexColor(size_t index, D3DCOLOR color) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = color;
}
void RenderObjectTLX::SetVertexColorARGB(size_t index, int a, int r, int g, int b) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = D3DCOLOR_ARGB(a, r, g, b);
}
void RenderObjectTLX::SetVertexAlpha(size_t index, int alpha) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = (vertex->diffuse_color & 0x00ffffff) | ((byte)alpha << 24);
}
void RenderObjectTLX::SetVertexColorRGB(size_t index, int r, int g, int b) {
	this->SetVertexColorRGB(index, D3DCOLOR_ARGB(0, r, g, b));
}
void RenderObjectTLX::SetVertexColorRGB(size_t index, D3DCOLOR rgb) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = (vertex->diffuse_color & 0xff000000) | (rgb & 0x00ffffff);
}
D3DCOLOR RenderObjectTLX::GetVertexColor(size_t index) {
	VERTEX_TLX* vertex = GetVertex(index);
	if (vertex == nullptr) return 0xffffffff;
	return vertex->diffuse_color;
}
void RenderObjectTLX::SetColorRGB(D3DCOLOR color) {
	D3DCOLOR dc = color & 0x00ffffff;
	for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
		VERTEX_TLX* vertex = GetVertex(iVert);
		if (vertex == nullptr) return;
		vertex->diffuse_color = (vertex->diffuse_color & 0xff000000) | dc;
	}
}
void RenderObjectTLX::SetAlpha(int alpha) {
	D3DCOLOR dc = (byte)alpha << 24;
	for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
		VERTEX_TLX* vertex = GetVertex(iVert);
		if (vertex == nullptr) return;
		vertex->diffuse_color = (vertex->diffuse_color & 0x00ffffff) | dc;
	}
}

//****************************************************************************
//RenderObjectLX
//****************************************************************************
RenderObjectLX::RenderObjectLX() {
	strideVertexStreamZero_ = sizeof(VERTEX_LX);
}
RenderObjectLX::~RenderObjectLX() {
}
void RenderObjectLX::Render() {
	RenderObjectLX::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void RenderObjectLX::Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	D3DXMATRIX matWorld;
	if (!disableMatrixTransform_) {
		matWorld = RenderObject::CreateWorldMatrix(position_, scale_,
			angX, angY, angZ, matRelative_.get(), false);
	}
	else {
		if (matRelative_ == nullptr)
			D3DXMatrixIdentity(&matWorld);
		else matWorld = *matRelative_;
	}

	RenderObjectLX::Render(matWorld);
}
void RenderObjectLX::Render(const D3DXMATRIX& matTransform) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	device->SetTexture(0, texture_ ? texture_->GetD3DTexture() : nullptr);

	device->SetTransform(D3DTS_WORLD, &matTransform);
	device->SetFVF(VERTEX_LX::fvf);

	{
		bool bUseIndex = vertexIndices_.size() > 0;
		size_t countVertex = std::min(GetVertexCount(), 65536U);
		size_t countIndex = std::min(vertexIndices_.size(), 65536U);
		size_t countPrim = std::min(_GetPrimitiveCount(bUseIndex ? countIndex : countVertex), 65536U);

		RenderShaderLibrary* shaderLib = ShaderManager::GetBase()->GetRenderLib();

		VertexBufferManager* vbManager = VertexBufferManager::GetBase();
		FixedVertexBuffer* vertexBuffer = vbManager->GetVertexBufferLX();
		FixedIndexBuffer* indexBuffer = vbManager->GetIndexBuffer();

		if (flgUseVertexBufferMode_ || bVertexShaderMode_) {
			BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

			lockParam.SetSource(vertex_, countVertex, sizeof(VERTEX_LX));
			vertexBuffer->UpdateBuffer(&lockParam);

			if (bUseIndex) {
				lockParam.SetSource(vertexIndices_, countIndex, sizeof(uint16_t));
				indexBuffer->UpdateBuffer(&lockParam);
			}
		}

		device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_LX));
		device->SetIndices(indexBuffer->GetBuffer());

		UINT countPass = 1;
		ID3DXEffect* effect = nullptr;
		if (shader_ != nullptr) {
			effect = shader_->GetEffect();

			if (shader_->LoadTechnique()) {
				shader_->LoadParameter();

				if (bVertexShaderMode_) {
					VertexFogState* fogParam = graphics->GetFogState();

					auto camera = DirectGraphics::GetBase()->GetCamera();

					bool bFog = graphics->IsFogEnable();
					graphics->SetFogEnable(false);
					device->SetVertexDeclaration(shaderLib->GetVertexDeclarationLX());

					D3DXHANDLE handle = nullptr;
					if (handle = effect->GetParameterBySemantic(nullptr, "WORLD"))
						effect->SetMatrix(handle, &matTransform);
					if (handle = effect->GetParameterBySemantic(nullptr, "VIEW"))
						effect->SetMatrix(handle, &camera->GetViewMatrix());
					if (handle = effect->GetParameterBySemantic(nullptr, "PROJECTION"))
						effect->SetMatrix(handle, &camera->GetProjectionMatrix());
					if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION"))
						effect->SetMatrix(handle, &camera->GetViewProjectionMatrix());
					if (handle = effect->GetParameterBySemantic(nullptr, "FOGENABLE"))
						effect->SetBool(handle, bFog);
					if (bFog) {
						if (handle = effect->GetParameterBySemantic(nullptr, "FOGCOLOR"))
							effect->SetFloatArray(handle, (FLOAT*)(&(fogParam->color)), 3);
						if (handle = effect->GetParameterBySemantic(nullptr, "FOGDIST"))
							effect->SetFloatArray(handle, (FLOAT*)(&(fogParam->fogDist)), 2);
					}
				}
			}

			effect->Begin(&countPass, 0);
		}
		for (UINT iPass = 0; iPass < countPass; ++iPass) {
			if (effect) effect->BeginPass(iPass);

			if (flgUseVertexBufferMode_ || bVertexShaderMode_) {
				if (bUseIndex) device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
				else device->DrawPrimitive(typePrimitive_, 0, countPrim);
			}
			else {
				if (bUseIndex)
					device->DrawIndexedPrimitiveUP(typePrimitive_, 0, countVertex, countPrim,
						vertexIndices_.data(), D3DFMT_INDEX16, vertex_.data(), strideVertexStreamZero_);
				else
					device->DrawPrimitiveUP(typePrimitive_, countPrim, vertex_.data(), strideVertexStreamZero_);
			}

			if (effect) effect->EndPass();
		}
		if (effect) effect->End();

		device->SetIndices(nullptr);
		device->SetVertexDeclaration(nullptr);
	}
}

void RenderObjectLX::SetVertexCount(size_t count) {
	RenderObject::SetVertexCount(count);
	SetColorRGB(D3DCOLOR_ARGB(255, 255, 255, 255));
	SetAlpha(255);
}
VERTEX_LX* RenderObjectLX::GetVertex(size_t index) {
	size_t pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.size()) return nullptr;
	return (VERTEX_LX*)&vertex_[pos];
}
void RenderObjectLX::SetVertex(size_t index, const VERTEX_LX& vertex) {
	size_t pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.size()) return;
	memcpy(&vertex_[pos], &vertex, strideVertexStreamZero_);
}
void RenderObjectLX::SetVertexPosition(size_t index, float x, float y, float z) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr) return;

	x *= DirectGraphics::g_dxCoordsMul_;
	y *= DirectGraphics::g_dxCoordsMul_;

	constexpr float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
}
void RenderObjectLX::SetVertexUV(size_t index, float u, float v) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectLX::SetVertexColor(size_t index, D3DCOLOR color) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = color;
}
void RenderObjectLX::SetVertexColorARGB(size_t index, int a, int r, int g, int b) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = D3DCOLOR_ARGB(a, r, g, b);
}
void RenderObjectLX::SetVertexAlpha(size_t index, int alpha) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = (vertex->diffuse_color & 0x00ffffff) | ((byte)alpha << 24);
}
void RenderObjectLX::SetVertexColorRGB(size_t index, int r, int g, int b) {
	this->SetVertexColorRGB(index, D3DCOLOR_ARGB(0, r, g, b));
}
void RenderObjectLX::SetVertexColorRGB(size_t index, D3DCOLOR rgb) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->diffuse_color = (vertex->diffuse_color & 0xff000000) | (rgb & 0x00ffffff);
}
D3DCOLOR RenderObjectLX::GetVertexColor(size_t index) {
	VERTEX_LX* vertex = GetVertex(index);
	if (vertex == nullptr) return 0xffffffff;
	return vertex->diffuse_color;
}
void RenderObjectLX::SetColorRGB(D3DCOLOR color) {
	D3DCOLOR dc = color & 0x00ffffff;
	for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
		VERTEX_LX* vertex = GetVertex(iVert);
		if (vertex == nullptr) return;
		vertex->diffuse_color = (vertex->diffuse_color & 0xff000000) | dc;
	}
}
void RenderObjectLX::SetAlpha(int alpha) {
	D3DCOLOR dc = (byte)alpha << 24;
	for (size_t iVert = 0; iVert < vertex_.size(); ++iVert) {
		VERTEX_LX* vertex = GetVertex(iVert);
		if (vertex == nullptr) return;
		vertex->diffuse_color = (vertex->diffuse_color & 0x00ffffff) | dc;
	}
}

//****************************************************************************
//RenderObjectNX
//****************************************************************************
RenderObjectNX::RenderObjectNX() {
	strideVertexStreamZero_ = sizeof(VERTEX_NX);

	color_ = 0xffffffff;

	pVertexBuffer_ = nullptr;
	pIndexBuffer_ = nullptr;
}
RenderObjectNX::~RenderObjectNX() {
	ptr_release(pVertexBuffer_);
	ptr_release(pIndexBuffer_);
}
void RenderObjectNX::Render() {
	RenderObjectNX::Render(nullptr);
}
void RenderObjectNX::Render(D3DXMATRIX* matTransform) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	device->SetTexture(0, texture_ ? texture_->GetD3DTexture() : nullptr);

	device->SetFVF(VERTEX_NX::fvf);

	{
		bool bUseIndex = vertexIndices_.size() > 0;
		size_t countVertex = GetVertexCount();
		size_t countIndex = vertexIndices_.size();
		size_t countPrim = _GetPrimitiveCount(bUseIndex ? countIndex : countVertex);

		RenderShaderLibrary* shaderLib = ShaderManager::GetBase()->GetRenderLib();

		VertexBufferManager* vbManager = VertexBufferManager::GetBase();
		FixedIndexBuffer* indexBuffer = vbManager->GetIndexBuffer();

		{
			if (bUseIndex) {
				BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

				lockParam.SetSource(vertexIndices_, countIndex, sizeof(uint16_t));
				indexBuffer->UpdateBuffer(&lockParam);
			}
		}

		device->SetStreamSource(0, pVertexBuffer_, 0, sizeof(VERTEX_NX));
		device->SetIndices(indexBuffer->GetBuffer());

		UINT countPass = 1;
		ID3DXEffect* effect = nullptr;
		if (shader_ != nullptr) {
			effect = shader_->GetEffect();

			if (shader_->LoadTechnique()) {
				shader_->LoadParameter();

				if (bVertexShaderMode_) {
					VertexFogState* fogParam = graphics->GetFogState();

					auto camera = graphics->GetCamera();

					bool bFog = graphics->IsFogEnable();
					graphics->SetFogEnable(false);
					device->SetVertexDeclaration(shaderLib->GetVertexDeclarationNX());

					D3DXHANDLE handle = nullptr;
					if (handle = effect->GetParameterBySemantic(nullptr, "WORLD"))
						effect->SetMatrix(handle, matTransform ? matTransform : &graphics->GetCamera()->GetIdentity());
					if (handle = effect->GetParameterBySemantic(nullptr, "VIEW"))
						effect->SetMatrix(handle, &camera->GetViewMatrix());
					if (handle = effect->GetParameterBySemantic(nullptr, "PROJECTION"))
						effect->SetMatrix(handle, &camera->GetProjectionMatrix());
					if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION"))
						effect->SetMatrix(handle, &camera->GetViewProjectionMatrix());
					if (handle = effect->GetParameterBySemantic(nullptr, "FOGENABLE"))
						effect->SetBool(handle, bFog);
					if (bFog) {
						if (handle = effect->GetParameterBySemantic(nullptr, "FOGCOLOR"))
							effect->SetFloatArray(handle, (FLOAT*)(&(fogParam->color)), 3);
						if (handle = effect->GetParameterBySemantic(nullptr, "FOGDIST"))
							effect->SetFloatArray(handle, (FLOAT*)(&(fogParam->fogDist)), 2);
					}
				}
			}

			effect->Begin(&countPass, 0);
		}
		for (UINT iPass = 0; iPass < countPass; ++iPass) {
			if (effect) effect->BeginPass(iPass);
			if (bUseIndex) {
				device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
			}
			else {
				device->DrawPrimitive(typePrimitive_, 0, countPrim);
			}
			if (effect) effect->EndPass();
		}
		if (effect) effect->End();

		device->SetIndices(nullptr);
		device->SetVertexDeclaration(nullptr);
	}
}

VERTEX_NX* RenderObjectNX::GetVertex(size_t index) {
	size_t pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.size()) return nullptr;
	return (VERTEX_NX*)&vertex_[pos];
}
void RenderObjectNX::SetVertex(size_t index, const VERTEX_NX& vertex) {
	size_t pos = index * strideVertexStreamZero_;
	if (pos >= vertex_.size()) return;
	memcpy(&vertex_[pos], &vertex, strideVertexStreamZero_);
}
void RenderObjectNX::SetVertexPosition(size_t index, float x, float y, float z) {
	VERTEX_NX* vertex = GetVertex(index);
	if (vertex == nullptr) return;

	x *= DirectGraphics::g_dxCoordsMul_;
	y *= DirectGraphics::g_dxCoordsMul_;

	constexpr float bias = -0.5f;
	vertex->position.x = x + bias;
	vertex->position.y = y + bias;
	vertex->position.z = z;
}
void RenderObjectNX::SetVertexUV(size_t index, float u, float v) {
	VERTEX_NX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->texcoord.x = u;
	vertex->texcoord.y = v;
}
void RenderObjectNX::SetVertexNormal(size_t index, float x, float y, float z) {
	VERTEX_NX* vertex = GetVertex(index);
	if (vertex == nullptr) return;
	vertex->normal.x = x;
	vertex->normal.y = y;
	vertex->normal.z = z;
}

//****************************************************************************
//Sprite2D
//****************************************************************************
Sprite2D::Sprite2D() {
	SetVertexCount(4);	//Top-left, top-right, bottom-left, bottom-right (Z pattern)
	SetPrimitiveType(D3DPT_TRIANGLESTRIP);

	flgUseVertexBufferMode_ = false;
}
Sprite2D::~Sprite2D() {

}
void Sprite2D::Copy(Sprite2D* src) {
	dxObjParent_ = src->dxObjParent_;
	
	typePrimitive_ = src->typePrimitive_;
	strideVertexStreamZero_ = src->strideVertexStreamZero_;

	vertex_ = src->vertex_;
	vertexIndices_ = src->vertexIndices_;

	texture_ = src->texture_;

	posWeightCenter_ = src->posWeightCenter_;

	matRelative_ = src->matRelative_;
}
void Sprite2D::SetSourceRect(const DxRect<int>& rcSrc) {
	if (texture_ == nullptr) return;
	float width = texture_->GetWidth();
	float height = texture_->GetHeight();

	SetVertexUV(0, rcSrc.left / width, rcSrc.top / height);
	SetVertexUV(1, rcSrc.right / width, rcSrc.top / height);
	SetVertexUV(2, rcSrc.left / width, rcSrc.bottom / height);
	SetVertexUV(3, rcSrc.right / width, rcSrc.bottom / height);
}
void Sprite2D::SetDestinationRect(const DxRect<double>& rcDest) {
	D3DXVECTOR4 pos = D3DXVECTOR4(rcDest.left, rcDest.top, rcDest.right, rcDest.bottom);
	//D3DXVec4Scale(&pos, &pos, DirectGraphics::g_dxCoordsMul_);

	SetVertexPosition(0, pos.x, pos.y);
	SetVertexPosition(1, pos.z, pos.y);
	SetVertexPosition(2, pos.x, pos.w);
	SetVertexPosition(3, pos.z, pos.w);
}
void Sprite2D::SetVertex(const DxRect<int>& rcSrc, const DxRect<double>& rcDest, D3DCOLOR color) {
	SetSourceRect(rcSrc);
	SetDestinationRect(rcDest);
	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}
DxRect<double> Sprite2D::GetDestinationRect() {
	constexpr float bias = -0.5f;

	VERTEX_TLX* vertexLeftTop = GetVertex(0);
	VERTEX_TLX* vertexRightBottom = GetVertex(3);

	DxRect<double> rect(vertexLeftTop->position.x - bias, vertexLeftTop->position.y - bias,
		vertexRightBottom->position.x - bias, vertexRightBottom->position.y - bias);

	return rect;
}
void Sprite2D::SetDestinationCenter() {
	if (texture_ == nullptr || GetVertexCount() < 4U) return;
	float width = texture_->GetWidth();
	float height = texture_->GetHeight();

	VERTEX_TLX* vertLT = GetVertex(0); //Top-left
	VERTEX_TLX* vertRB = GetVertex(3); //Bottom-right

	float vWidth = (vertRB->texcoord.x - vertLT->texcoord.x) * width / 2.0f;
	float vHeight = (vertRB->texcoord.y - vertLT->texcoord.y) * height / 2.0f;
	
	SetDestinationRect(DxRect<double>(-vWidth, -vHeight, vWidth, vHeight));
}

//****************************************************************************
//SpriteList2D
//****************************************************************************
SpriteList2D::SpriteList2D() {
	countRenderIndex_ = 0;
	countRenderIndexPrev_ = 0;
	countRenderVertex_ = 0;
	countRenderVertexPrev_ = 0;
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	bCloseVertexList_ = false;
	autoClearVertexList_ = false;

	vertexIndices_.resize(64U);
	vertex_.resize(64U * strideVertexStreamZero_);
}
void SpriteList2D::Render() {
	SpriteList2D::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void SpriteList2D::Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	size_t countRenderIndex = countRenderIndex_;
	size_t countRenderVertex = countRenderVertex_;

	if (!graphics->IsMainRenderLoop()) {
		countRenderIndex = countRenderIndexPrev_;
		countRenderVertex = countRenderVertexPrev_;
	}
	if (countRenderIndex == 0U || countRenderVertex == 0U) return;

	IDirect3DDevice9* device = graphics->GetDevice();
	ref_count_ptr<DxCamera2D> camera = graphics->GetCamera2D();
	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	
	device->SetTexture(0, texture_ ? texture_->GetD3DTexture() : nullptr);
	device->SetFVF(VERTEX_TLX::fvf);

	bool bCamera = camera->IsEnable() && bPermitCamera_;
	{
		bool bUseIndex = vertexIndices_.size() > 0;
		size_t countVertex = std::min(countRenderVertex, 43688U);	//10922 * 4
		size_t countIndex = std::min(countRenderIndex, 65532U);		//10922 * 6
		size_t countPrim = _GetPrimitiveCount(countIndex);			//Max = 10922 quads

		D3DXMATRIX matWorld;
		if (bCloseVertexList_)
			matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
				angX, angY, angZ, bCamera ? &camera->GetMatrix() : nullptr);

		vertCopy_ = vertex_;
		{
			byte mulAlpha = color_ >> 24;
			float rMulAlpha = 1.0f;
			bool bMulAlpha = mulAlpha != 0xff;
			if (bMulAlpha)
				rMulAlpha = mulAlpha / 255.0f;

			for (size_t iVert = 0; iVert < countVertex; ++iVert) {
				size_t pos = iVert * strideVertexStreamZero_;
				VERTEX_TLX* vertex = (VERTEX_TLX*)&vertCopy_[pos];
				D3DXVECTOR3* vPos = (D3DXVECTOR3*)(&vertex->position);

				if (!bVertexShaderMode_) {
					if (bCloseVertexList_)
						D3DXVec3TransformCoord(vPos, vPos, &matWorld);
					else if (bCamera)
						D3DXVec3TransformCoord(vPos, vPos, &camera->GetMatrix());
				}
				if (bCloseVertexList_ && bMulAlpha) {
					//Multiply alpha
					ColorAccess::SetColorA(vertex->diffuse_color,
						(vertex->diffuse_color >> 24) * rMulAlpha);
				}
			}
		}

		{
			RenderShaderLibrary* shaderLib = ShaderManager::GetBase()->GetRenderLib();

			VertexBufferManager* vbManager = VertexBufferManager::GetBase();
			FixedVertexBuffer* vertexBuffer = vbManager->GetVertexBufferTLX();
			FixedIndexBuffer* indexBuffer = vbManager->GetIndexBuffer();

			{
				BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

				lockParam.SetSource(vertCopy_, countVertex, sizeof(VERTEX_TLX));
				vertexBuffer->UpdateBuffer(&lockParam);
				
				lockParam.SetSource(vertexIndices_, countIndex, sizeof(uint16_t));
				indexBuffer->UpdateBuffer(&lockParam);
			}

			device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_TLX));
			device->SetIndices(indexBuffer->GetBuffer());

			UINT countPass = 1;
			ID3DXEffect* effect = nullptr;
			if (shader_ != nullptr) {
				effect = shader_->GetEffect();

				if (shader_->LoadTechnique()) {
					shader_->LoadParameter();

					if (bVertexShaderMode_) {
						device->SetVertexDeclaration(shaderLib->GetVertexDeclarationTLX());

						D3DXHANDLE handle = nullptr;
						if (handle = effect->GetParameterBySemantic(nullptr, "WORLD")) {
							if (bCloseVertexList_)
								effect->SetMatrix(handle, &matWorld);
							else if (bCamera)
								effect->SetMatrix(handle, &camera->GetMatrix());
							else
								effect->SetMatrix(handle, &camera3D->GetIdentity());
						}
						if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION")) {
							effect->SetMatrix(handle, &graphics->GetViewPortMatrix());
						}
					}
				}

				effect->Begin(&countPass, 0);
			}
			for (UINT iPass = 0; iPass < countPass; ++iPass) {
				if (effect) effect->BeginPass(iPass);
				device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
				if (effect) effect->EndPass();
			}
			if (effect) effect->End();

			device->SetIndices(nullptr);
			device->SetVertexDeclaration(nullptr);
		}
	}
}
void SpriteList2D::CleanUp() {
	countRenderIndexPrev_ = countRenderIndex_;
	countRenderVertexPrev_ = countRenderVertex_;
	if (autoClearVertexList_ && (countRenderIndex_ >= 6))
		ClearVertexCount();
}
void SpriteList2D::_AddVertex(VERTEX_TLX(&verts)[4]) {
	if (countRenderIndex_ >= 65532U) return;

	size_t indexCount = vertexIndices_.size();
	size_t vertexCount = vertex_.size() / strideVertexStreamZero_;

	//10922 quads
	if (countRenderIndex_ + 6 >= indexCount)
		vertexIndices_.resize(std::min(65532U, indexCount * 2U));
	if (countRenderVertex_ + 4 >= vertexCount)
		vertex_.resize(std::min(43688U * strideVertexStreamZero_, vertex_.size() * 2U));

	uint16_t baseIndex = (uint16_t)countRenderVertex_;
	uint16_t indices[] = { 
		baseIndex + 0u, baseIndex + 1u, baseIndex + 2u,
		baseIndex + 1u, baseIndex + 2u, baseIndex + 3u
	};
	memcpy(&vertexIndices_[countRenderIndex_], indices, 6U * sizeof(uint16_t));
	memcpy(&vertex_[countRenderVertex_ * strideVertexStreamZero_], verts, 4U * strideVertexStreamZero_);
	countRenderIndex_ += 6U;
	countRenderVertex_ += 4U;
}
void SpriteList2D::AddVertex() {
	SpriteList2D::AddVertex(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void SpriteList2D::AddVertex(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	if (bCloseVertexList_ || countRenderVertex_ > 65536 / 6) return;

	if (texture_ == nullptr) return;
	float width = texture_->GetWidth();
	float height = texture_->GetHeight();

	D3DXMATRIX matWorld = RenderObject::CreateWorldMatrix2D(position_, scale_,
		angX, angY, angZ, nullptr);

	int* ptrSrc = reinterpret_cast<int*>(&rcSrc_);
	double* ptrDst = reinterpret_cast<double*>(&rcDest_);

	VERTEX_TLX verts[4];
	for (size_t iVert = 0; iVert < 4; ++iVert) {
		VERTEX_TLX vt;
		vt.texcoord.x = (float)ptrSrc[(iVert & 0b1) << 1] / width;
		vt.texcoord.y = (float)ptrSrc[iVert | 0b1] / height;

		D3DXVECTOR4 vPos;

		constexpr float bias = -0.5f;
		vPos.x = (float)ptrDst[(iVert & 0b1) << 1];
		vPos.y = (float)ptrDst[iVert | 0b1];
		vPos.z = 1.0f;
		vPos.w = 1.0f;

		vPos.x = vPos.x * DirectGraphics::g_dxCoordsMul_ + bias;
		vPos.y = vPos.y * DirectGraphics::g_dxCoordsMul_ + bias;

		D3DXVec3TransformCoord((D3DXVECTOR3*)&vPos, (D3DXVECTOR3*)&vPos, &matWorld);
		vt.position = vPos;

		vt.diffuse_color = color_;
		verts[iVert] = vt;
	}

	_AddVertex(verts);
}
void SpriteList2D::SetDestinationCenter() {
	if (texture_ == nullptr) return;

	double vWidth = (rcSrc_.right - rcSrc_.left) / 2.0;
	double vHeight = (rcSrc_.bottom - rcSrc_.top) / 2.0;

	SetDestinationRect(DxRect<double>(-vWidth, -vHeight, vWidth, vHeight));
}
void SpriteList2D::CloseVertex() {
	bCloseVertexList_ = true;
	SetColor(0xffffffff);
}
void SpriteList2D::SetColor(D3DCOLOR color) {
	color_ = color;
}

//****************************************************************************
//Sprite3D
//****************************************************************************
Sprite3D::Sprite3D() {
	SetVertexCount(4);	//Top-left, top-right, bottom-left, bottom-right (Z pattern)
	SetPrimitiveType(D3DPT_TRIANGLESTRIP);
	bBillboard_ = false;

	flgUseVertexBufferMode_ = false;
}
Sprite3D::~Sprite3D() {}

void Sprite3D::Render() {
	Sprite3D::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void Sprite3D::Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	D3DXMATRIX matWorld;
	if (!disableMatrixTransform_) {
		matWorld = RenderObject::CreateWorldMatrixSprite3D(position_, scale_,
			angX, angY, angZ, matRelative_.get(), bBillboard_);
	}

	RenderObjectLX::Render(matWorld);
}
void Sprite3D::SetSourceDestRect(const DxRect<double>& rcSrc) {
	DxRect<int> rcSrcCopy(rcSrc.left, rcSrc.top,
		(int)rcSrc.right - 1, (int)rcSrc.bottom - 1);

	double width = (rcSrc.right - rcSrc.left) / 2.0;
	double height = (rcSrc.bottom - rcSrc.top) / 2.0;
	DxRect<double> rcDest(-width, -height, width, height);

	SetSourceRect(rcSrcCopy);
	SetDestinationRect(rcDest);
}
void Sprite3D::SetSourceRect(const DxRect<int>& rcSrc) {
	if (texture_ == nullptr) return;
	float width = texture_->GetWidth();
	float height = texture_->GetHeight();

	SetVertexUV(0, rcSrc.left / width, rcSrc.top / height);
	SetVertexUV(1, rcSrc.left / width, rcSrc.bottom / height);
	SetVertexUV(2, rcSrc.right / width, rcSrc.top / height);
	SetVertexUV(3, rcSrc.right / width, rcSrc.bottom / height);
}
void Sprite3D::SetDestinationRect(const DxRect<double>& rcDest) {
	D3DXVECTOR4 pos = D3DXVECTOR4(rcDest.left, rcDest.top, rcDest.right, rcDest.bottom);
	//D3DXVec4Scale(&pos, &pos, DirectGraphics::g_dxCoordsMul_);

	SetVertexPosition(0, pos.x, pos.y, 0);
	SetVertexPosition(1, pos.x, pos.w, 0);
	SetVertexPosition(2, pos.z, pos.y, 0);
	SetVertexPosition(3, pos.z, pos.w, 0);
}
void Sprite3D::SetVertex(const DxRect<int>& rcSrc, const DxRect<double>& rcDest, D3DCOLOR color) {
	SetSourceRect(rcSrc);
	SetDestinationRect(rcDest);

	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}
void Sprite3D::SetVertex(const DxRect<double>& rcSrcDst, D3DCOLOR color) {
	SetSourceDestRect(rcSrcDst);

	//頂点色
	SetColorRGB(color);
	SetAlpha(ColorAccess::GetColorA(color));
}

//****************************************************************************
//TrajectoryObject3D
//****************************************************************************
TrajectoryObject3D::TrajectoryObject3D() {
	SetPrimitiveType(D3DPT_TRIANGLESTRIP);
	diffAlpha_ = 20;
	countComplement_ = 8;
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
}
TrajectoryObject3D::~TrajectoryObject3D() {}
D3DXMATRIX TrajectoryObject3D::_CreateWorldTransformMatrix() {
	D3DXMATRIX mat;
	D3DXMatrixIdentity(&mat);
	return mat;
}
void TrajectoryObject3D::Work() {
	for (auto itr = listData_.begin(); itr != listData_.end();) {
		Data& data = (*itr);
		data.alpha -= diffAlpha_;
		if (data.alpha < 0) itr = listData_.erase(itr);
		else ++itr;
	}
}
void TrajectoryObject3D::Render() {
	TrajectoryObject3D::Render(D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0), D3DXVECTOR2(1, 0));
}
void TrajectoryObject3D::Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	size_t size = listData_.size() * 2;
	SetVertexCount(size);

	int width = 1;
	if (texture_) width = texture_->GetWidth();

	float dWidth = 1.0 / width / listData_.size();
	size_t iData = 0;
	for (auto itr = listData_.begin(); itr != listData_.end(); ++itr, ++iData) {
		Data data = (*itr);
		int alpha = data.alpha;
		for (size_t iPos = 0; iPos < 2; ++iPos) {
			size_t index = iData * 2 + iPos;
			D3DXVECTOR3& pos = iPos == 0 ? data.pos1 : data.pos2;
			float u = dWidth * iData;
			float v = iPos == 0 ? 0 : 1;

			SetVertexPosition(index, pos.x, pos.y, pos.z);
			SetVertexUV(index, u, v);

			float r = ColorAccess::GetColorR(color_) * alpha / 255;
			float g = ColorAccess::GetColorG(color_) * alpha / 255;
			float b = ColorAccess::GetColorB(color_) * alpha / 255;
			SetVertexColorARGB(index, alpha, r, g, b);
		}
	}
	RenderObjectLX::Render(angX, angY, angZ);
}
void TrajectoryObject3D::AddPoint(const D3DXMATRIX& mat) {
	Data data;
	data.alpha = 255;
	data.pos1 = dataInit_.pos1;
	data.pos2 = dataInit_.pos2;
	D3DXVec3TransformCoord((D3DXVECTOR3*)&data.pos1, (D3DXVECTOR3*)&data.pos1, &mat);
	D3DXVec3TransformCoord((D3DXVECTOR3*)&data.pos2, (D3DXVECTOR3*)&data.pos2, &mat);

	if (listData_.size() <= 1) {
		listData_.push_back(data);
		dataLast2_ = dataLast1_;
		dataLast1_ = data;
	}
	else {
		float cDiff = 1.0 / (countComplement_);
		float diffAlpha = diffAlpha_ / countComplement_;
		for (size_t iCount = 0; iCount < countComplement_ - 1; ++iCount) {
			Data cData;
			float flame = cDiff * (iCount + 1);
			for (size_t iPos = 0; iPos < 2; ++iPos) {
				D3DXVECTOR3& outPos = iPos == 0 ? cData.pos1 : cData.pos2;
				D3DXVECTOR3& cPos = iPos == 0 ? data.pos1 : data.pos2;
				D3DXVECTOR3& lPos1 = iPos == 0 ? dataLast1_.pos1 : dataLast1_.pos2;
				D3DXVECTOR3& lPos2 = iPos == 0 ? dataLast2_.pos1 : dataLast2_.pos2;

				D3DXVECTOR3 vPos1 = lPos1 - lPos2;
				D3DXVECTOR3 vPos2 = lPos2 - cPos + vPos1;

				//				D3DXVECTOR3 vPos1 = lPos2 - lPos1;
				//				D3DXVECTOR3 vPos2 = lPos2 - cPos + vPos1;

								//D3DXVECTOR3 vPos1 = lPos1 - lPos2;
								//D3DXVECTOR3 vPos2 = cPos - lPos1;

				D3DXVec3Hermite(&outPos, &lPos1, &vPos1, &cPos, &vPos2, flame);
			}

			cData.alpha = 255 - diffAlpha * (countComplement_ - iCount - 1);
			listData_.push_back(cData);
		}
		dataLast2_ = dataLast1_;
		dataLast1_ = data;
	}

}

//****************************************************************************
//ParticleRendererBase
//****************************************************************************
ParticleRendererBase::ParticleRendererBase() {
	countInstance_ = 0U;
	countInstancePrev_ = 0U;
	autoClearInstance_ = true;
	instColor_ = 0xffffffff;
	instPosition_ = D3DXVECTOR3(0, 0, 0);
	instScale_ = D3DXVECTOR3(1, 1, 1);
	instAngle_ = D3DXVECTOR3(0, 0, 0);
	instUserData_ = D3DXVECTOR3(0, 0, 0);
	instanceData_.resize(8U);
}
ParticleRendererBase::~ParticleRendererBase() {
}
void ParticleRendererBase::AddInstance() {
	if (countInstance_ == 32768U) return;
	if (instanceData_.size() == countInstance_) {
		size_t newSize = std::min(countInstance_ * 2U, 32768U);
		instanceData_.resize(newSize);
	}
	VERTEX_INSTANCE instance;
	instance.diffuse_color = instColor_;
	instance.xyz_pos_x_scale = D3DXVECTOR4(instPosition_.x, instPosition_.y, instPosition_.z, instScale_.x);
	instance.yz_scale_xy_ang = D3DXVECTOR4(instScale_.y, instScale_.z, -instAngle_.x, -instAngle_.y);
	instance.z_ang_extra = D3DXVECTOR4(-instAngle_.z, instUserData_.x, instUserData_.y, instUserData_.z);
	instanceData_[countInstance_++] = instance;
}
void ParticleRendererBase::ClearInstance() {
	countInstancePrev_ = countInstance_;
	countInstance_ = 0U;
	//countInstancePrev_ = 0U;
}
void ParticleRendererBase::SetInstanceColorRGB(int r, int g, int b) {
	__m128i c = Vectorize::Set(instColor_ >> 24, r, g, b);
	D3DCOLOR color = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));
	SetInstanceColor(color);
}
void ParticleRendererBase::SetInstanceColorRGB(D3DCOLOR color) {
	SetInstanceColor((color & 0x00ffffff) | (instColor_ & 0xff000000));
}
void ParticleRendererBase::SetInstanceAlpha(int alpha) {
	ColorAccess::ClampColor(alpha);
	SetInstanceColor((instColor_ & 0x00ffffff) | ((byte)alpha << 24));
}

void ParticleRendererBase::SetInstancePosition(const D3DXVECTOR3& pos) {
	instPosition_ = pos;
	D3DXVec3Scale(&instPosition_, &instPosition_, DirectGraphics::g_dxCoordsMul_);
}
void ParticleRendererBase::SetInstanceScale(const D3DXVECTOR3& scale) {
	instScale_ = scale;
	D3DXVec3Scale(&instScale_, &instScale_, DirectGraphics::g_dxCoordsMul_);
}
void ParticleRendererBase::SetInstanceScaleSingle(size_t index, float sc) {
	float* pVec = (float*)&instScale_;
	pVec[index] = sc * DirectGraphics::g_dxCoordsMul_;
}
void ParticleRendererBase::SetInstanceAngleSingle(size_t index, float ang) {
	float* pVec = (float*)&instAngle_;
	pVec[index] = D3DXToRadian(ang);
}

ParticleRenderer2D::ParticleRenderer2D() {
	
}
void ParticleRenderer2D::Render() {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	size_t countRenderInstance = countInstance_;

	if (!graphics->IsMainRenderLoop()) countRenderInstance = countInstancePrev_;
	if (countRenderInstance == 0U) return;

	size_t countIndex = std::min(vertexIndices_.size(), 65536U);
	if (countIndex == 0U) return;
	
	IDirect3DDevice9* device = graphics->GetDevice();
	ref_count_ptr<DxCamera2D> camera = graphics->GetCamera2D();

	bool bCamera = camera->IsEnable() && bPermitCamera_;

	device->SetTexture(0, texture_ ? texture_->GetD3DTexture() : nullptr);

	{
		size_t countVertex = std::min(GetVertexCount(), 65536U);
		size_t countPrim = _GetPrimitiveCount(countIndex);

		VertexBufferManager* bufferManager = VertexBufferManager::GetBase();
		RenderShaderLibrary* shaderManager = ShaderManager::GetBase()->GetRenderLib();

		FixedVertexBuffer* vertexBuffer = bufferManager->GetVertexBufferTLX();
		GrowableVertexBuffer* instanceBuffer = bufferManager->GetInstancingVertexBuffer();
		FixedIndexBuffer* indexBuffer = bufferManager->GetIndexBuffer();

		instanceBuffer->Expand(countRenderInstance);

		{
			BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

			lockParam.SetSource(vertex_, countVertex, sizeof(VERTEX_TLX));
			vertexBuffer->UpdateBuffer(&lockParam);

			lockParam.SetSource(instanceData_, countRenderInstance, sizeof(VERTEX_INSTANCE));
			instanceBuffer->UpdateBuffer(&lockParam);

			lockParam.SetSource(vertexIndices_, countIndex, sizeof(uint16_t));
			indexBuffer->UpdateBuffer(&lockParam);
		}

		device->SetVertexDeclaration(shaderManager->GetVertexDeclarationInstancedTLX());

		device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_TLX));
#ifdef __L_USE_HWINSTANCING
		device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | countRenderInstance);
		device->SetStreamSource(1, instanceBuffer->GetBuffer(), 0, sizeof(VERTEX_INSTANCE));
		device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1U);
#endif

		device->SetIndices(indexBuffer->GetBuffer());

		{
			UINT countPass = 1;
			ID3DXEffect* effect = nullptr;

			if (shader_) {
				effect = shader_->GetEffect();
			}
			else {
				effect = shaderManager->GetInstancing2DShader();
				effect->SetTechnique(texture_ ? (dxObjParent_->GetBlendType() == MODE_BLEND_ALPHA_INV ?
					"RenderInv" : "Render") : "RenderNoTexture");
			}

			if (effect == nullptr) return;

			auto _SetParam = [&]() {
				D3DXHANDLE handle = nullptr;
				if (handle = effect->GetParameterBySemantic(nullptr, "WORLDVIEWPROJ")) {
					D3DXMATRIX mat;
					D3DXMatrixMultiply(&mat, &camera->GetMatrix(), &graphics->GetViewPortMatrix());
					effect->SetMatrix(handle, &mat);
				}
			};
			
			if (shader_) {
				if (shader_->LoadTechnique()) {
					shader_->LoadParameter();
					_SetParam();
				}
			}
			else {
				_SetParam();
			}

			effect->Begin(&countPass, 0);
			for (UINT iPass = 0; iPass < countPass; ++iPass) {
				effect->BeginPass(iPass);

#ifdef __L_USE_HWINSTANCING
				device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
#else
				for (UINT nInst = 0; nInst < countRenderInstance; ++nInst) {
					device->SetStreamSource(1, instanceBuffer->GetBuffer(), 
						nInst * sizeof(VERTEX_INSTANCE), 0);
					device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
				}
#endif

				effect->EndPass();
			}
			effect->End();
		}

#ifdef __L_USE_HWINSTANCING
		device->SetStreamSourceFreq(0, 1);
		device->SetStreamSourceFreq(1, 1);
#endif
	}
}
ParticleRenderer3D::ParticleRenderer3D() {
}
void ParticleRenderer3D::Render() {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	size_t countRenderInstance = countInstance_;

	if (!graphics->IsMainRenderLoop()) countRenderInstance = countInstancePrev_;
	if (countRenderInstance == 0U) return;

	size_t countIndex = std::min(vertexIndices_.size(), 65536U);
	if (countIndex == 0U) return;

	IDirect3DDevice9* device = graphics->GetDevice();

	ref_count_ptr<DxCamera> camera = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();

	device->SetTexture(0, texture_ ? texture_->GetD3DTexture() : nullptr);

	{
		size_t countVertex = std::min(GetVertexCount(), 65536U);
		size_t countPrim = _GetPrimitiveCount(countIndex);

		VertexBufferManager* bufferManager = VertexBufferManager::GetBase();
		RenderShaderLibrary* shaderManager = ShaderManager::GetBase()->GetRenderLib();

		FixedVertexBuffer* vertexBuffer = bufferManager->GetVertexBufferLX();
		GrowableVertexBuffer* instanceBuffer = bufferManager->GetInstancingVertexBuffer();
		FixedIndexBuffer* indexBuffer = bufferManager->GetIndexBuffer();

		instanceBuffer->Expand(countRenderInstance);

		{
			BufferLockParameter lockParam = BufferLockParameter(D3DLOCK_DISCARD);

			lockParam.SetSource(vertex_, countVertex, sizeof(VERTEX_LX));
			vertexBuffer->UpdateBuffer(&lockParam);

			lockParam.SetSource(instanceData_, countRenderInstance, sizeof(VERTEX_INSTANCE));
			instanceBuffer->UpdateBuffer(&lockParam);

			lockParam.SetSource(vertexIndices_, countIndex, sizeof(uint16_t));
			indexBuffer->UpdateBuffer(&lockParam);
		}

		device->SetVertexDeclaration(shaderManager->GetVertexDeclarationInstancedLX());

		device->SetStreamSource(0, vertexBuffer->GetBuffer(), 0, sizeof(VERTEX_LX));
#ifdef __L_USE_HWINSTANCING
		device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | countRenderInstance);
		device->SetStreamSource(1, instanceBuffer->GetBuffer(), 0, sizeof(VERTEX_INSTANCE));
		device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1U);
#endif

		device->SetIndices(indexBuffer->GetBuffer());

		{
			UINT countPass = 1;
			ID3DXEffect* effect = nullptr;

			if (shader_) {
				effect = shader_->GetEffect();
			}
			else {
				effect = shaderManager->GetInstancing3DShader();
				effect->SetTechnique(texture_ ? (dxObjParent_->GetBlendType() == MODE_BLEND_ALPHA_INV ?
					"RenderInv" : "Render") : "RenderNoTexture");
			}

			if (effect == nullptr) return;

			auto _SetParam = [&]() {
				VertexFogState* fogParam = graphics->GetFogState();

				bool bFog = graphics->IsFogEnable();
				graphics->SetFogEnable(false);

				D3DXHANDLE handle = nullptr;
				if (handle = effect->GetParameterBySemantic(nullptr, "WORLD")) {
					if (bBillboard_)
						effect->SetMatrix(handle, &camera->GetViewTransposedMatrix());
					else effect->SetMatrix(handle, &camera->GetIdentity());
				}
				if (handle = effect->GetParameterBySemantic(nullptr, "VIEW"))
					effect->SetMatrix(handle, &camera->GetViewMatrix());
				if (handle = effect->GetParameterBySemantic(nullptr, "PROJECTION"))
					effect->SetMatrix(handle, &camera->GetProjectionMatrix());
				if (handle = effect->GetParameterBySemantic(nullptr, "VIEWPROJECTION"))
					effect->SetMatrix(handle, &camera->GetViewProjectionMatrix());
				if (handle = effect->GetParameterBySemantic(nullptr, "FOGENABLE"))
					effect->SetBool(handle, bFog);
				if (bFog) {
					if (handle = effect->GetParameterBySemantic(nullptr, "FOGCOLOR"))
						effect->SetFloatArray(handle, (FLOAT*)(&(fogParam->color)), 3);
					if (handle = effect->GetParameterBySemantic(nullptr, "FOGDIST"))
						effect->SetFloatArray(handle, (FLOAT*)(&(fogParam->fogDist)), 2);
				}
			};

			if (shader_) {
				if (shader_->LoadTechnique()) {
					shader_->LoadParameter();
					_SetParam();
				}
			}
			else {
				_SetParam();
			}

			effect->Begin(&countPass, 0);
			for (UINT iPass = 0; iPass < countPass; ++iPass) {
				effect->BeginPass(iPass);

#ifdef __L_USE_HWINSTANCING
				device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
#else
				for (UINT nInst = 0; nInst < countRenderInstance; ++nInst) {
					device->SetStreamSource(1, instanceBuffer->GetBuffer(),
						nInst * sizeof(VERTEX_INSTANCE), 0);
					device->DrawIndexedPrimitive(typePrimitive_, 0, 0, countVertex, 0, countPrim);
				}
#endif

				effect->EndPass();
			}
			effect->End();
		}

#ifdef __L_USE_HWINSTANCING
		device->SetStreamSourceFreq(0, 1);
		device->SetStreamSourceFreq(1, 1);
#endif
	}
}

//****************************************************************************
//DxMesh
//****************************************************************************
DxMeshData::DxMeshData() {
	manager_ = DxMeshManager::GetBase();
	bLoad_ = true;
}
DxMeshData::~DxMeshData() {}

DxMesh::DxMesh() {
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	bCoordinate2D_ = false;

	lightParameter_.SetLightEnable(true);
}
DxMesh::~DxMesh() {
	Release();
}

void DxMesh::SetPosition(float x, float y, float z) {
	position_ = D3DXVECTOR3(x, y, z);
	D3DXVec3Scale(&position_, &position_, DirectGraphics::g_dxCoordsMul_);
}
void DxMesh::SetX(float x) {
	position_.x = x * DirectGraphics::g_dxCoordsMul_;
}
void DxMesh::SetY(float y) {
	position_.y = y * DirectGraphics::g_dxCoordsMul_;
}
void DxMesh::SetZ(float z) {
	position_.z = z * DirectGraphics::g_dxCoordsMul_;
}
void DxMesh::SetScaleXYZ(float sx, float sy, float sz) {
	scale_ = D3DXVECTOR3(sx, sy, sz);
	D3DXVec3Scale(&scale_, &scale_, DirectGraphics::g_dxCoordsMul_);
}

shared_ptr<DxMeshData> DxMesh::_GetFromManager(const std::wstring& name) {
	return DxMeshManager::GetBase()->_GetMeshData(name);
}
void DxMesh::_AddManager(const std::wstring& name, shared_ptr<DxMeshData> data) {
	DxMeshManager::GetBase()->_AddMeshData(name, data);
}
void DxMesh::Release() {
	{
		DxMeshManager* manager = DxMeshManager::GetBase();
		Lock lock(manager->GetLock());
		if (data_) {
			if (manager->IsDataExists(data_->GetName())) {
				long countRef = data_.use_count();
				if (countRef == 2) {
					manager->_ReleaseMeshData(data_->GetName());
				}
			}
			data_ = nullptr;
		}
	}
}
bool DxMesh::CreateFromFile(const std::wstring& path) {
	try {
		shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr || !reader->Open())
			throw wexception(ErrorUtility::GetFileNotFoundErrorMessage(PathProperty::ReduceModuleDirectory(path), true));

		return CreateFromFileReader(reader);
	}
	catch (gstd::wexception& e) {
		std::wstring str = StringUtility::Format(L"DxMesh: Mesh load failed. [%s]\r\n\t%s", path.c_str(), e.what());
		Logger::WriteTop(str);
	}
	return false;
}
bool DxMesh::CreateFromFileInLoadThread(const std::wstring& path, int type) {
	bool res = false;
	{
		Lock lock(DxMeshManager::GetBase()->GetLock());
		if (data_) Release();
		DxMeshManager* manager = DxMeshManager::GetBase();
		shared_ptr<DxMesh> mesh = manager->CreateFromFileInLoadThread(path, type);
		if (mesh)
			data_ = mesh->data_;
		res = data_ != nullptr;
	}

	return res;
}

//****************************************************************************
//DxMeshManager
//****************************************************************************
DxMeshManager* DxMeshManager::thisBase_ = nullptr;
DxMeshManager::DxMeshManager() {}
DxMeshManager::~DxMeshManager() {
	this->Clear();
	FileManager::GetBase()->RemoveLoadThreadListener(this);
	panelInfo_ = nullptr;
	thisBase_ = nullptr;
}
bool DxMeshManager::Initialize() {
	thisBase_ = this;
	FileManager::GetBase()->AddLoadThreadListener(this);
	return true;
}

void DxMeshManager::Clear() {
	{
		Lock lock(lock_);
		mapMesh_.clear();
		mapMeshData_.clear();
	}
}

void DxMeshManager::_AddMeshData(const std::wstring& name, shared_ptr<DxMeshData> data) {
	{
		Lock lock(lock_);
		if (!IsDataExists(name)) {
			mapMeshData_[name] = data;
		}
	}
}
shared_ptr<DxMeshData> DxMeshManager::_GetMeshData(const std::wstring& name) {
	shared_ptr<DxMeshData> res = nullptr;
	{
		Lock lock(lock_);

		auto itr = mapMeshData_.find(name);
		if (itr != mapMeshData_.end()) {
			res = itr->second;
		}
	}
	return res;
}
void DxMeshManager::_ReleaseMeshData(const std::wstring& name) {
	{
		Lock lock(lock_);

		auto itr = mapMeshData_.find(name);
		if (itr != mapMeshData_.end()) {
			mapMeshData_.erase(itr);
			Logger::WriteTop(StringUtility::Format(L"DxMeshManager: Mesh released. [%s]", 
				PathProperty::ReduceModuleDirectory(name).c_str()));
		}
	}
}
void DxMeshManager::Add(const std::wstring& name, shared_ptr<DxMesh> mesh) {
	{
		Lock lock(lock_);
		bool bExist = mapMesh_.find(name) != mapMesh_.end();
		if (!bExist) {
			mapMesh_[name] = mesh;
		}
	}
}
void DxMeshManager::Release(const std::wstring& name) {
	{
		Lock lock(lock_);
		mapMesh_.erase(name);
	}
}
bool DxMeshManager::IsDataExists(const std::wstring& name) {
	bool res = false;
	{
		Lock lock(lock_);
		res = mapMeshData_.find(name) != mapMeshData_.end();
	}
	return res;
}
shared_ptr<DxMesh> DxMeshManager::CreateFromFileInLoadThread(const std::wstring& path, int type) {
	shared_ptr<DxMesh> res;
	{
		Lock lock(lock_);

		auto itrMesh = mapMesh_.find(path);
		if (itrMesh != mapMesh_.end()) {
			res = itrMesh->second;
		}
		else {
			//if (type == MESH_ELFREINA) res = std::make_shared<ElfreinaMesh>();
			//else if (type == MESH_METASEQUOIA) res = std::make_shared<MetasequoiaMesh>();
			res = std::make_shared<MetasequoiaMesh>();

			auto itrData = mapMeshData_.find(path);
			if (itrData == mapMeshData_.end()) {
				shared_ptr<DxMeshData> data = nullptr;
				//if (type == MESH_ELFREINA) data = std::make_shared<ElfreinaMeshData>();
				//else if (type == MESH_METASEQUOIA) data = std::make_shared<MetasequoiaMeshData>();
				data = std::make_shared<MetasequoiaMeshData>();

				itrData = mapMeshData_.insert(std::pair<std::wstring, shared_ptr<DxMeshData>>(path, data)).first;

				data->manager_ = this;
				data->name_ = path;
				data->bLoad_ = false;

				shared_ptr<FileManager::LoadObject> source = res;
				shared_ptr<FileManager::LoadThreadEvent> event(new FileManager::LoadThreadEvent(this, path, res));
				FileManager::GetBase()->AddLoadThreadEvent(event);
			}

			res->data_ = itrData->second;
		}
	}
	return res;
}
void DxMeshManager::CallFromLoadThread(shared_ptr<FileManager::LoadThreadEvent> event) {
	const std::wstring& path = event->GetPath();
	{
		Lock lock(lock_);
		shared_ptr<DxMesh> mesh = std::dynamic_pointer_cast<DxMesh>(event->GetSource());
		if (mesh == nullptr) return;

		shared_ptr<DxMeshData> data = mesh->data_;
		if (data->bLoad_) return;

		std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);

		bool res = false;
		shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader != nullptr && reader->Open())
			res = data->CreateFromFileReader(reader);

		if (res) {
			Logger::WriteTop(StringUtility::Format(L"DxMeshManager(LT): Mesh loaded. [%s]", pathReduce.c_str()));
		}
		else {
			Logger::WriteTop(StringUtility::Format(L"DxMeshManager(LT): Failed to load mesh \"%s\"", pathReduce.c_str()));
		}
		data->bLoad_ = true;
	}
}

//DxMeshInfoPanel
DxMeshInfoPanel::DxMeshInfoPanel() {
	timeUpdateInterval_ = 500;
}
DxMeshInfoPanel::~DxMeshInfoPanel() {
	Stop();
	Join(1000);
}
bool DxMeshInfoPanel::_AddedLogger(HWND hTab) {
	Create(hTab);

	gstd::WListView::Style styleListView;
	styleListView.SetStyle(WS_CHILD | WS_VISIBLE |
		LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_NOSORTHEADER);
	styleListView.SetStyleEx(WS_EX_CLIENTEDGE);
	styleListView.SetListViewStyleEx(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	wndListView_.Create(hWnd_, styleListView);

	wndListView_.AddColumn(64, ROW_ADDRESS, L"Address");
	wndListView_.AddColumn(128, ROW_NAME, L"Name");
	wndListView_.AddColumn(128, ROW_FULLNAME, L"FullName");
	wndListView_.AddColumn(32, ROW_COUNT_REFFRENCE, L"Ref");

	SetWindowVisible(false);
	Start();

	return true;
}
void DxMeshInfoPanel::LocateParts() {
	int wx = GetClientX();
	int wy = GetClientY();
	int wWidth = GetClientWidth();
	int wHeight = GetClientHeight();

	wndListView_.SetBounds(wx, wy, wWidth, wHeight);
}
void DxMeshInfoPanel::_Run() {
	while (GetStatus() == RUN) {
		DxMeshManager* manager = DxMeshManager::GetBase();
		if (manager)
			Update(manager);
		Sleep(timeUpdateInterval_);
	}
}
void DxMeshInfoPanel::Update(DxMeshManager* manager) {
	if (!IsWindowVisible()) return;
	std::set<std::wstring> setKey;
	std::map<std::wstring, shared_ptr<DxMeshData>>& mapData = manager->mapMeshData_;
	std::map<std::wstring, shared_ptr<DxMeshData>>::iterator itrMap;
	{
		Lock lock(manager->GetLock());
		for (itrMap = mapData.begin(); itrMap != mapData.end(); ++itrMap) {
			std::wstring name = itrMap->first;
			DxMeshData* data = (itrMap->second).get();

			int address = (int)data;
			std::wstring key = StringUtility::Format(L"%08x", address);
			int index = wndListView_.GetIndexInColumn(key, ROW_ADDRESS);
			if (index == -1) {
				index = wndListView_.GetRowCount();
				wndListView_.SetText(index, ROW_ADDRESS, key);
			}

			int countRef = (itrMap->second).use_count();

			wndListView_.SetText(index, ROW_NAME, PathProperty::GetFileName(name));
			wndListView_.SetText(index, ROW_FULLNAME, name);
			wndListView_.SetText(index, ROW_COUNT_REFFRENCE, StringUtility::Format(L"%d", countRef));

			setKey.insert(key);
		}
	}

	for (int iRow = 0; iRow < wndListView_.GetRowCount();) {
		std::wstring key = wndListView_.GetText(iRow, ROW_ADDRESS);
		if (setKey.find(key) != setKey.end())
			++iRow;
		else
			wndListView_.DeleteRow(iRow);
	}
}


