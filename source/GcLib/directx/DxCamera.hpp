#pragma once

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
	//*******************************************************************
	//DxCamera
	//*******************************************************************
	class DxCamera {
		D3DXVECTOR3 pos_;
		D3DXVECTOR3 camPos_;

		//D3DXVECTOR3 laPosEye_;
		D3DXVECTOR3 laPosLookAt_;

		float radius_;
		float angleAzimuth_;
		float angleElevation_;

		float yaw_;
		float pitch_;
		float roll_;

		float projWidth_;
		float projHeight_;
		float clipNear_;
		float clipFar_;

		D3DXMATRIX matView_;
		D3DXMATRIX matProjection_;

		D3DXMATRIX matViewProjection_;
		D3DXMATRIX matViewInverse_;
		D3DXMATRIX matViewTranspose_;
		D3DXMATRIX matProjectionInverse_;

		D3DXMATRIX matIdentity_;

		int modeCamera_;

		std::list<D3DXMATRIX> listMatrixState_;
	public:
		enum {
			MODE_NORMAL,
			MODE_LOOKAT
		};

		bool thisViewChanged_;
		bool thisProjectionChanged_;

		DxCamera();
		virtual ~DxCamera();

		void Reset();

		const D3DXVECTOR3& GetCameraPosition() { return camPos_; }
		const D3DXVECTOR3& GetFocusPosition() { return pos_; }
		void SetFocus(float x, float y, float z) { pos_.x = x; pos_.y = y; pos_.z = z; }
		void SetFocusX(float x) { pos_.x = x; }
		void SetFocusY(float y) { pos_.y = y; }
		void SetFocusZ(float z) { pos_.z = z; }
		float GetRadius() { return radius_; }
		void SetRadius(float r) { radius_ = r; }
		float GetAzimuthAngle() { return angleAzimuth_; }
		void SetAzimuthAngle(float angle) { angleAzimuth_ = angle; }
		float GetElevationAngle() { return angleElevation_; }
		void SetElevationAngle(float angle) { angleElevation_ = angle; }

		void SetCameraLookAtVector(D3DXVECTOR3 vec) { laPosLookAt_ = vec; }
		//void SetCameraEyeVector(D3DXVECTOR3 vec) { laPosEye_ = vec; }

		float GetYaw() { return yaw_; }
		void SetYaw(float yaw) { yaw_ = yaw; }
		float GetPitch() { return pitch_; }
		void SetPitch(float pitch) { pitch_ = pitch; }
		float GetRoll() { return roll_; }
		void SetRoll(float roll) { roll_ = roll; }

		void SetPerspectiveWidth(float w) { projWidth_ = w; }
		void SetPerspectiveHeight(float h) { projHeight_ = h; }
		void SetPerspectiveClip(float pNear, float pFar) {
			if (pNear < 1.0f) pNear = 1.0f;
			if (pFar < 1.0f) pFar = 1.0f;
			clipNear_ = pNear;
			clipFar_ = pFar;
		}
		float GetNearClip() { return clipNear_; }
		float GetFarClip() { return clipFar_; }

		void SetCameraMode(int mode) { modeCamera_ = mode; }
		int GetCameraMode() { return modeCamera_; }

		D3DXMATRIX GetMatrixLookAtLH();
		void SetWorldViewMatrix();
		void SetProjectionMatrix();
		void UpdateDeviceViewProjectionMatrix();

		D3DXVECTOR2 TransformCoordinateTo2D(D3DXVECTOR3 pos);

		const D3DXMATRIX& GetViewMatrix() { return matView_; }
		const D3DXMATRIX& GetViewProjectionMatrix() { return matViewProjection_; }
		const D3DXMATRIX& GetViewInverseMatrix() { return matViewInverse_; }
		const D3DXMATRIX& GetViewTransposedMatrix() { return matViewTranspose_; }
		const D3DXMATRIX& GetProjectionMatrix() { return matProjection_; }
		const D3DXMATRIX& GetProjectionInverseMatrix() { return matProjectionInverse_; }

		const D3DXMATRIX& GetIdentity() { return matIdentity_; }

		void PushMatrixState();
		void PopMatrixState();
	};

	//*******************************************************************
	//DxCamera2D
	//*******************************************************************
	class DxCamera2D {
	private:
		bool bEnable_;

		D3DXVECTOR2 pos_;

		float ratioX_;
		float ratioY_;
		double angleZ_;

		DxRect<LONG> rcClip_;

		optional<D3DXVECTOR2> posReset_;

		D3DXMATRIX matCamera_;
		D3DXMATRIX matIdentity_;
	public:
		DxCamera2D();
		virtual ~DxCamera2D();

		bool IsEnable() { return bEnable_; }
		void SetEnable(bool bEnable) { bEnable_ = bEnable; }

		const D3DXVECTOR2& GetFocusPosition() { return pos_; }
		float GetFocusX() { return pos_.x; }
		float GetFocusY() { return pos_.y; }
		void SetFocus(float x, float y) { pos_.x = x; pos_.y = y; }
		void SetFocus(const D3DXVECTOR2& pos) { pos_ = pos; }
		void SetFocusX(float x) { pos_.x = x; }
		void SetFocusY(float y) { pos_.y = y; }
		float GetRatio() { return std::max(ratioX_, ratioY_); }
		void SetRatio(float ratio) { ratioX_ = ratio; ratioY_ = ratio; }
		float GetRatioX() { return ratioX_; }
		void SetRatioX(float ratio) { ratioX_ = ratio; }
		float GetRatioY() { return ratioY_; }
		void SetRatioY(float ratio) { ratioY_ = ratio; }
		double GetAngleZ() { return angleZ_; }
		void SetAngleZ(double angle) { angleZ_ = angle; }

		const DxRect<LONG>& GetClip() { return rcClip_; }
		void SetClip(const DxRect<LONG>& rect) { rcClip_ = rect; }

		void SetResetFocus(const D3DXVECTOR2& pos) { posReset_ = pos; }
		void Reset();
		void ResetAll();

		inline D3DXVECTOR2 GetLeftTopPosition();
		inline static D3DXVECTOR2 GetLeftTopPosition(const D3DXVECTOR2& focus, float ratio);
		inline static D3DXVECTOR2 GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY);
		inline static D3DXVECTOR2 GetLeftTopPosition(const D3DXVECTOR2& focus, float ratioX, float ratioY, const DxRect<LONG>& rcClip);

		void UpdateMatrix();
		const D3DXMATRIX& GetMatrix() { return bEnable_ ? matCamera_ : matIdentity_; }
	};
}