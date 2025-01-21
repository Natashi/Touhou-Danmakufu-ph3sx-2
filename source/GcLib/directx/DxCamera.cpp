#include "source/GcLib/pch.h"

#include "DxCamera.hpp"

#include "DirectGraphics.hpp"
#include "DxUtility.hpp"
#include "DxScript.hpp"

using namespace directx;

//*******************************************************************
//DxCamera
//*******************************************************************
DxCamera::DxCamera() {
	Reset();
}
DxCamera::~DxCamera() {
}
void DxCamera::Reset() {
	radius_ = 500;
	angleAzimuth_ = D3DXToRadian(15);
	angleElevation_ = D3DXToRadian(45);

	ZeroMemory(&pos_, sizeof(D3DXVECTOR3));
	ZeroMemory(&camPos_, sizeof(D3DXVECTOR3));
	ZeroMemory(&laPosLookAt_, sizeof(D3DXVECTOR3));

	yaw_ = 0;
	pitch_ = 0;
	roll_ = 0;

	projWidth_ = 384.0f;
	projHeight_ = 448.0f;
	clipNear_ = 10.0f;
	clipFar_ = 2000.0f;

	D3DXMatrixIdentity(&matView_);
	D3DXMatrixIdentity(&matProjection_);
	D3DXMatrixIdentity(&matViewProjection_);
	D3DXMatrixIdentity(&matViewInverse_);
	D3DXMatrixIdentity(&matViewTranspose_);
	D3DXMatrixIdentity(&matProjectionInverse_);

	D3DXMatrixIdentity(&matIdentity_);

	thisViewChanged_ = true;
	thisProjectionChanged_ = true;

	modeCamera_ = MODE_NORMAL;
}

D3DXMATRIX DxCamera::GetMatrixLookAtLH() {
	D3DXMATRIX res;

	D3DXVECTOR3 vCameraUp(0, 1, 0);
	{
		D3DXQUATERNION qRot(0, 0, 0, 1.0f);
		D3DXQuaternionRotationYawPitchRoll(&qRot, yaw_, pitch_, roll_);
		D3DXMATRIX matRot;
		D3DXMatrixRotationQuaternion(&matRot, &qRot);
		D3DXVec3TransformCoord((D3DXVECTOR3*)&vCameraUp, (D3DXVECTOR3*)&vCameraUp, &matRot);
	}

	if (modeCamera_ == MODE_LOOKAT) {
		D3DXMatrixLookAtLH(&res, &pos_, &laPosLookAt_, &vCameraUp);
	}
	else {
		camPos_.x = pos_.x + (float)(radius_ * cos(angleElevation_) * cos(angleAzimuth_));
		camPos_.y = pos_.y + (float)(radius_ * sin(angleElevation_));
		camPos_.z = pos_.z + (float)(radius_ * cos(angleElevation_) * sin(angleAzimuth_));

		D3DXVECTOR3 posTo = pos_;

		{
			D3DXMATRIX matTrans1;
			D3DXMatrixTranslation(&matTrans1, -camPos_.x, -camPos_.y, -camPos_.z);
			D3DXMATRIX matTrans2;
			D3DXMatrixTranslation(&matTrans2, camPos_.x, camPos_.y, camPos_.z);

			D3DXQUATERNION qRot(0, 0, 0, 1.0f);
			D3DXQuaternionRotationYawPitchRoll(&qRot, yaw_, pitch_, 0);
			D3DXMATRIX matRot;
			D3DXMatrixRotationQuaternion(&matRot, &qRot);

			D3DXMATRIX mat;
			mat = matTrans1 * matRot * matTrans2;
			D3DXVec3TransformCoord((D3DXVECTOR3*)&posTo, (D3DXVECTOR3*)&posTo, &mat);
		}

		D3DXMatrixLookAtLH(&res, &camPos_, &posTo, &vCameraUp);
	}

	return res;
}
void DxCamera::SetWorldViewMatrix() {
	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr) return;
	IDirect3DDevice9* device = graph->GetDevice();

	matView_ = GetMatrixLookAtLH();
	D3DXMatrixInverse(&matViewInverse_, nullptr, &matView_);

	{
		matViewTranspose_._11 = matView_._11;
		matViewTranspose_._12 = matView_._21;
		matViewTranspose_._13 = matView_._31;
		matViewTranspose_._21 = matView_._12;
		matViewTranspose_._22 = matView_._22;
		matViewTranspose_._23 = matView_._32;
		matViewTranspose_._31 = matView_._13;
		matViewTranspose_._32 = matView_._23;
		matViewTranspose_._33 = matView_._33;
		matViewTranspose_._14 = 0.0f;
		matViewTranspose_._24 = 0.0f;
		matViewTranspose_._34 = 0.0f;
		matViewTranspose_._44 = 1.0f;
	}

	//UpdateDeviceProjectionMatrix();
}
void DxCamera::SetProjectionMatrix() {
	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr) return;
	IDirect3DDevice9* device = graph->GetDevice();

	D3DXMatrixPerspectiveFovLH(&matProjection_, D3DXToRadian(45.0),
		projWidth_ / projHeight_, clipNear_, clipFar_);
	D3DXMatrixInverse(&matProjectionInverse_, nullptr, &matProjection_);

	//UpdateDeviceProjectionMatrix();
}
void DxCamera::UpdateDeviceViewProjectionMatrix() {
	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr) return;
	IDirect3DDevice9* device = graph->GetDevice();

	matViewProjection_ = matView_ * matProjection_;
	device->SetTransform(D3DTS_VIEW, &matViewProjection_);
	device->SetTransform(D3DTS_PROJECTION, &matIdentity_);
}
D3DXVECTOR2 DxCamera::TransformCoordinateTo2D(D3DXVECTOR3 pos) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	LONG width = graphics->GetScreenWidth();
	LONG height = graphics->GetScreenHeight();

	D3DXVECTOR4 vect;
	D3DXVec3Transform(&vect, &pos, &matViewProjection_);

	if (vect.w > 0) {
		vect.x = width / 2.0f + (vect.x / vect.w) * width / 2.0f;
		vect.y = height / 2.0f - (vect.y / vect.w) * height / 2.0f; // Ｙ方向は上が正となるため
		return D3DXVECTOR2(vect.x, vect.y);
	}

	return D3DXVECTOR2((FLOAT)DxScript::g_posInvalidX_, (FLOAT)DxScript::g_posInvalidY_);
}
void DxCamera::PushMatrixState() {
	listMatrixState_.push_back(matViewProjection_);
}
void DxCamera::PopMatrixState() {
	if (listMatrixState_.empty()) return;

	DirectGraphics* graph = DirectGraphics::GetBase();
	if (graph == nullptr) return;
	IDirect3DDevice9* device = graph->GetDevice();

	D3DXMATRIX* top = &listMatrixState_.back();

	matViewProjection_ = *top;
	device->SetTransform(D3DTS_VIEW, top);
	device->SetTransform(D3DTS_PROJECTION, &matIdentity_);

	listMatrixState_.pop_back();
}

//*******************************************************************
//DxCamera2D
//*******************************************************************
DxCamera2D::DxCamera2D() {
	pos_.x = 400;
	pos_.y = 300;
	ratioX_ = 1.0f;
	ratioY_ = 1.0f;
	angleZ_ = 0;
	bEnable_ = false;

	posReset_ = { 0, 0 };

	D3DXMatrixIdentity(&matIdentity_);
}
DxCamera2D::~DxCamera2D() {}

void DxCamera2D::Reset() {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	LONG width = graphics->GetScreenWidth();
	LONG height = graphics->GetScreenHeight();

	if (!posReset_.has_value()) {
		pos_.x = width / 2;
		pos_.y = height / 2;
	}
	else {
		pos_.x = posReset_->x;
		pos_.y = posReset_->y;
	}
	ratioX_ = 1.0f;
	ratioY_ = 1.0f;

	rcClip_.Set(0, 0, width, height);

	angleZ_ = 0;
}
void DxCamera2D::ResetAll() {
	posReset_ = {};
	Reset();
}

D3DXVECTOR2 DxCamera2D::GetLeftTopPosition() {
	return GetLeftTopPosition(pos_, ratioX_, ratioY_, rcClip_);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(const D3DXVECTOR2& focus, float ratio) {
	return GetLeftTopPosition(focus, ratio, ratio);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	LONG width = graphics->GetScreenWidth();
	LONG height = graphics->GetScreenHeight();
	DxRect<LONG> rcClip;
	rcClip.right = width;
	rcClip.bottom = height;
	return GetLeftTopPosition(focus, ratioX, ratioY, rcClip);
}
D3DXVECTOR2 DxCamera2D::GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY, const DxRect<LONG>& rcClip) {
	LONG width_2 = (rcClip.right - rcClip.left) / 2L;
	LONG height_2 = (rcClip.bottom - rcClip.top) / 2L;

	LONG cen_x = rcClip.left + width_2;
	LONG cen_y = rcClip.top + height_2;

	LONG dx = focus.x - cen_x;
	LONG dy = focus.y - cen_y;

	D3DXVECTOR2 res;
	res.x = cen_x - dx * ratioX;
	res.y = cen_y - dy * ratioY;

	res.x -= width_2 * ratioX;
	res.y -= height_2 * ratioY;

	return res;
}

void DxCamera2D::UpdateMatrix() {
	D3DXVECTOR2 pos = GetLeftTopPosition();

	D3DXMatrixIdentity(&matCamera_);

	if (angleZ_ != 0) {
		float c = cosf(angleZ_);
		float s = sinf(angleZ_);
		float x = GetFocusX() - pos.x;
		float y = GetFocusY() - pos.y;

		matCamera_._11 = c;
		matCamera_._12 = -s;
		matCamera_._21 = s;
		matCamera_._22 = c;
		matCamera_._41 = -(x * c) - (y * s) + x;
		matCamera_._42 = x* s - y * c + y;
	}

	matCamera_._11 *= ratioX_;
	matCamera_._22 *= ratioY_;
	matCamera_._41 += pos.x;
	matCamera_._42 += pos.y;
}