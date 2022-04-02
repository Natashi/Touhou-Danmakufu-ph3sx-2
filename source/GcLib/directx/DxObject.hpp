#pragma once

#include "../pch.h"

#include "../gstd/ScriptClient.hpp"

#include "DxConstant.hpp"
#include "Texture.hpp"
#include "DxText.hpp"
#include "RenderObject.hpp"
#include "DirectSound.hpp"

namespace directx {
	class DxScript;
	class DxScriptObjectManager;
	class DxScriptObjectBase;

	//****************************************************************************
	//DxScriptObjectBase
	//****************************************************************************
	class DxScriptObjectBase {
		friend DxScript;
		friend DxScriptObjectManager;
	protected:
		int idObject_;
		TypeObject typeObject_;
		int64_t idScript_;
		DxScriptObjectManager* manager_;
		int priRender_;
		bool bVisible_;
		bool bDeleted_;
		bool bQueuedToDelete_;
		bool bActive_;
		bool bAutoDeleteOverride_;
		int frameExist_;

		std::unordered_map<std::wstring, gstd::value> mapObjectValue_;
		std::unordered_map<int64_t, gstd::value> mapObjectValueI_;
	public:
		DxScriptObjectBase();
		virtual ~DxScriptObjectBase();

		void SetObjectManager(DxScriptObjectManager* manager) { manager_ = manager; }
		
		virtual void Initialize() {}
		virtual void Work() {}
		virtual void Render() {}
		virtual void SetRenderState() {}
		virtual void CleanUp() {}

		virtual bool HasNormalRendering() { return false; }

		int GetObjectID() { return idObject_; }
		TypeObject GetObjectType() { return typeObject_; }
		int64_t GetScriptID() { return idScript_; }
		void QueueDelete() { bQueuedToDelete_ = true; }
		bool IsQueuedForDeletion() { return bQueuedToDelete_; }
		bool IsDeleted() { return bDeleted_; }
		bool IsActive() { return bActive_; }
		void SetActive(bool bActive) { bActive_ = bActive; }
		bool IsVisible() { return bVisible_; }

		double GetRenderPriority();
		int GetRenderPriorityI() { return priRender_; }
		void SetRenderPriority(double pri);
		void SetRenderPriorityI(int pri) { priRender_ = pri; }
		void SetAutoDeleteOverride(bool del) { bAutoDeleteOverride_ = del; }
		bool IsAutoDeleteOverride() { return bAutoDeleteOverride_; }

		int GetExistFrame() { return frameExist_; }

		std::unordered_map<std::wstring, gstd::value>* GetValueMap() { return &mapObjectValue_; }
		std::unordered_map<int64_t, gstd::value>* GetValueMapI() { return &mapObjectValueI_; }
	};

	//****************************************************************************
	//DxScriptRenderObject
	//****************************************************************************
	class DxScriptRenderObject : public DxScriptObjectBase {
		friend DxScript;
	protected:
		bool bZWrite_;
		bool bZTest_;
		bool bFogEnable_;
		D3DCULL modeCulling_;

		D3DXVECTOR3 position_;
		D3DXVECTOR3 angle_;
		D3DXVECTOR3 scale_;
		BlendMode typeBlend_;

		D3DCOLOR color_;

		D3DTEXTUREFILTERTYPE filterMin_;
		D3DTEXTUREFILTERTYPE filterMag_;
		D3DTEXTUREFILTERTYPE filterMip_;
		bool bVertexShaderMode_;
		bool bEnableMatrix_;

		gstd::ref_count_weak_ptr<DxScriptRenderObject, false> objRelative_;
	public:
		DxScriptRenderObject();

		virtual void Render() {}
		virtual void SetRenderState() {}

		virtual bool HasNormalRendering() { return true; }

		virtual void SetX(float x) { position_.x = x; }
		virtual void SetY(float y) { position_.y = y; }
		virtual void SetZ(float z) { position_.z = z; }
		virtual void SetAngleX(float x) { angle_.x = gstd::Math::DegreeToRadian(x); }
		virtual void SetAngleY(float y) { angle_.y = gstd::Math::DegreeToRadian(y); }
		virtual void SetAngleZ(float z) { angle_.z = gstd::Math::DegreeToRadian(z); }
		virtual void SetScaleX(float x) { scale_.x = x; }
		virtual void SetScaleY(float y) { scale_.y = y; }
		virtual void SetScaleZ(float z) { scale_.z = z; }
		virtual void SetColor(int r, int g, int b) = 0;
		virtual void SetAlpha(int alpha) = 0;

		virtual DirectionalLightingState* GetLightPointer() { return nullptr; }

		D3DXVECTOR3& GetPosition() { return position_; }
		D3DXVECTOR3& GetAngle() { return angle_; }
		D3DXVECTOR3& GetScale() { return scale_; }
		void SetPosition(const D3DXVECTOR3& pos) { position_ = pos; }
		void SetAngle(const D3DXVECTOR3& angle) { angle_ = angle; }
		void SetScale(const D3DXVECTOR3& scale) { scale_ = scale; }

		void SetFilteringMin(D3DTEXTUREFILTERTYPE filter) { filterMin_ = filter; }
		void SetFilteringMag(D3DTEXTUREFILTERTYPE filter) { filterMag_ = filter; }
		void SetFilteringMip(D3DTEXTUREFILTERTYPE filter) { filterMip_ = filter; }
		void SetVertexShaderRendering(bool b) { bVertexShaderMode_ = b; }
		void SetLoadWorldMatrix(bool b) { bEnableMatrix_ = b; }

		void SetBlendType(BlendMode type) { typeBlend_ = type; }
		BlendMode GetBlendType() { return typeBlend_; }

		virtual void SetRenderTarget(shared_ptr<Texture> texture) {};

		virtual shared_ptr<Shader> GetShader() { return nullptr; }
		virtual void SetShader(shared_ptr<Shader> shader) {};

		void SetRelativeObject(gstd::ref_count_weak_ptr<DxScriptRenderObject, false> id) {
			objRelative_ = id;
		}
	};

	//****************************************************************************
	//DxScriptShaderObject
	//****************************************************************************
	class DxScriptShaderObject : public DxScriptRenderObject {
	protected:
		shared_ptr<Shader> shader_;
	public:
		DxScriptShaderObject();

		virtual shared_ptr<Shader> GetShader() { return shader_; }
		virtual void SetShader(shared_ptr<Shader> shader) { shader_ = shader; }

		virtual void SetColor(int r, int g, int b) {}
		virtual void SetAlpha(int alpha) {}
	};

	//****************************************************************************
	//DxScriptPrimitiveObject
	//****************************************************************************
	class DxScriptPrimitiveObject : public DxScriptRenderObject {
		friend DxScript;
	protected:
		shared_ptr<RenderObject> objRender_;

		D3DXVECTOR2 angX_;
		D3DXVECTOR2 angY_;
		D3DXVECTOR2 angZ_;
	public:
		DxScriptPrimitiveObject();

		virtual DirectionalLightingState* GetLightPointer() { return objRender_->GetLighting(); }
		RenderObject* GetObjectPointer() { return objRender_.get(); }

		void SetPrimitiveType(D3DPRIMITIVETYPE type) { objRender_->SetPrimitiveType(type); }
		D3DPRIMITIVETYPE GetPrimitiveType() { return objRender_->GetPrimitiveType(); }

		void SetVertexCount(size_t count) { objRender_->SetVertexCount(count); }
		size_t GetVertexCount() { return objRender_->GetVertexCount(); }

		shared_ptr<Texture> GetTexture() { return objRender_->GetTexture(); }
		virtual void SetTexture(shared_ptr<Texture> texture) { objRender_->SetTexture(texture); }

		virtual bool IsValidVertexIndex(size_t index) = 0;
		virtual void SetVertexPosition(size_t index, float x, float y, float z) = 0;
		virtual void SetVertexUV(size_t index, float u, float v) = 0;
		virtual void SetVertexAlpha(size_t index, int alpha) = 0;
		virtual void SetVertexColor(size_t index, int r, int g, int b) = 0;
		virtual D3DCOLOR GetVertexColor(size_t index) = 0;
		virtual D3DXVECTOR3 GetVertexPosition(size_t index) = 0;

		virtual void SetRenderTarget(shared_ptr<Texture> texture) { objRender_->SetRenderTarget(texture); }

		virtual shared_ptr<Shader> GetShader() { return objRender_->GetShader(); }
		virtual void SetShader(shared_ptr<Shader> shader) { objRender_->SetShader(shader); }

		virtual void SetAngleX(float x);
		virtual void SetAngleY(float y);
		virtual void SetAngleZ(float z);
		virtual void SetAngle(const D3DXVECTOR3& angle) {
			SetAngleX(angle.x);
			SetAngleY(angle.y);
			SetAngleZ(angle.z);
		}
	};

	//****************************************************************************
	//DxScriptPrimitiveObject2D
	//****************************************************************************
	class DxScriptPrimitiveObject2D : public DxScriptPrimitiveObject {
	public:
		DxScriptPrimitiveObject2D();

		virtual void Render();
		virtual void SetRenderState();

		RenderObjectTLX* GetObjectPointer() { return dynamic_cast<RenderObjectTLX*>(objRender_.get()); }

		virtual void SetColor(int r, int g, int b);
		virtual void SetAlpha(int alpha);

		virtual bool IsValidVertexIndex(size_t index);
		virtual void SetVertexPosition(size_t index, float x, float y, float z);
		virtual void SetVertexUV(size_t index, float u, float v);
		virtual void SetVertexAlpha(size_t index, int alpha);
		virtual void SetVertexColor(size_t index, int r, int g, int b);
		virtual D3DCOLOR GetVertexColor(size_t index);

		void SetPermitCamera(bool bPermit);
		virtual D3DXVECTOR3 GetVertexPosition(size_t index);
	};

	//****************************************************************************
	//DxScriptSpriteObject2D
	//****************************************************************************
	class DxScriptSpriteObject2D : public DxScriptPrimitiveObject2D {
	public:
		DxScriptSpriteObject2D();
		void Copy(DxScriptSpriteObject2D* src);
		Sprite2D* GetSpritePointer() { return dynamic_cast<Sprite2D*>(objRender_.get()); }
	};

	//****************************************************************************
	//DxScriptSpriteListObject2D
	//****************************************************************************
	class DxScriptSpriteListObject2D : public DxScriptPrimitiveObject2D {
	public:
		DxScriptSpriteListObject2D();

		virtual void CleanUp();

		SpriteList2D* GetSpritePointer() { return (SpriteList2D*)objRender_.get(); }

		virtual void SetColor(int r, int g, int b);
		virtual void SetAlpha(int alpha);

		void AddVertex();
		void CloseVertex();
	};

	//****************************************************************************
	//DxScriptPrimitiveObject3D
	//****************************************************************************
	class DxScriptPrimitiveObject3D : public DxScriptPrimitiveObject {
		friend DxScript;
	public:
		DxScriptPrimitiveObject3D();

		virtual void Render();
		virtual void SetRenderState();

		RenderObjectLX* GetObjectPointer() { return dynamic_cast<RenderObjectLX*>(objRender_.get()); }

		virtual void SetColor(int r, int g, int b);
		virtual void SetAlpha(int alpha);

		virtual bool IsValidVertexIndex(size_t index);
		virtual void SetVertexPosition(size_t index, float x, float y, float z);
		virtual void SetVertexUV(size_t index, float u, float v);
		virtual void SetVertexAlpha(size_t index, int alpha);
		virtual void SetVertexColor(size_t index, int r, int g, int b);
		virtual D3DCOLOR GetVertexColor(size_t index);
		virtual D3DXVECTOR3 GetVertexPosition(size_t index);
	};
	//****************************************************************************
	//DxScriptSpriteObject3D
	//****************************************************************************
	class DxScriptSpriteObject3D : public DxScriptPrimitiveObject3D {
	public:
		DxScriptSpriteObject3D();

		Sprite3D* GetSpritePointer() { return dynamic_cast<Sprite3D*>(objRender_.get()); }
	};

	//****************************************************************************
	//DxScriptTrajectoryObject3D
	//****************************************************************************
	class DxScriptTrajectoryObject3D : public DxScriptPrimitiveObject {
	public:
		DxScriptTrajectoryObject3D();

		virtual void Work();
		virtual void Render();
		virtual void SetRenderState();

		TrajectoryObject3D* GetObjectPointer() { return dynamic_cast<TrajectoryObject3D*>(objRender_.get()); }

		virtual void SetColor(int r, int g, int b);
		virtual void SetAlpha(int alpha) {};

		virtual bool IsValidVertexIndex(size_t index) { return false; }
		virtual void SetVertexPosition(size_t index, float x, float y, float z) {};
		virtual void SetVertexUV(size_t index, float u, float v) {};
		virtual void SetVertexAlpha(size_t index, int alpha) {};
		virtual void SetVertexColor(size_t index, int r, int g, int b) {};
		virtual D3DCOLOR GetVertexColor(size_t index) { return 0xffffffff; };
		virtual D3DXVECTOR3 GetVertexPosition(size_t index) { return D3DXVECTOR3(0, 0, 0); }
	};

	//****************************************************************************
	//DxScriptParticleListObject2D
	//****************************************************************************
	class DxScriptParticleListObject2D : public DxScriptSpriteObject2D {
	public:
		DxScriptParticleListObject2D();

		virtual void Render();
		virtual void SetRenderState();

		virtual void CleanUp();

		virtual void SetAngleX(float x) { angle_.x = x; }
		virtual void SetAngleY(float y) { angle_.y = y; }
		virtual void SetAngleZ(float z) { angle_.z = z; }

		ParticleRenderer2D* GetParticlePointer() { return (ParticleRenderer2D*)objRender_.get(); }
	};
	//****************************************************************************
	//DxScriptParticleListObject3D
	//****************************************************************************
	class DxScriptParticleListObject3D : public DxScriptSpriteObject3D {
	public:
		DxScriptParticleListObject3D();

		virtual void Render();
		virtual void SetRenderState();

		virtual void CleanUp();

		virtual void SetAngleX(float x) { angle_.x = x; }
		virtual void SetAngleY(float y) { angle_.y = y; }
		virtual void SetAngleZ(float z) { angle_.z = z; }

		ParticleRenderer3D* GetParticlePointer() { return (ParticleRenderer3D*)objRender_.get(); }
	};

	//****************************************************************************
	//DxScriptMeshObject
	//****************************************************************************
	class DxScriptMeshObject : public DxScriptRenderObject {
		friend DxScript;
	protected:
		shared_ptr<DxMesh> mesh_;
		int time_;
		std::wstring anime_;
		bool bCoordinate2D_;

		D3DXVECTOR2 angX_;
		D3DXVECTOR2 angY_;
		D3DXVECTOR2 angZ_;
	public:
		DxScriptMeshObject();

		virtual void Render();
		virtual void SetRenderState();

		virtual DirectionalLightingState* GetLightPointer() { return mesh_->GetLighting(); }
		
		virtual void SetColor(int r, int g, int b);
		virtual void SetAlpha(int alpha);

		void SetMesh(shared_ptr<DxMesh> mesh) { mesh_ = mesh; }
		shared_ptr<DxMesh> GetMesh() { return mesh_; }

		int GetAnimeFrame() { return time_; }
		std::wstring& GetAnimeName() { return anime_; }

		virtual void SetX(float x) { position_.x = x; }
		virtual void SetY(float y) { position_.y = y; }
		virtual void SetZ(float z) { position_.z = z; }

		virtual void SetAngleX(float x);
		virtual void SetAngleY(float y);
		virtual void SetAngleZ(float z);
		virtual void SetAngle(const D3DXVECTOR3& angle) {
			DxScriptMeshObject::SetAngleX(angle.x);
			DxScriptMeshObject::SetAngleY(angle.y);
			DxScriptMeshObject::SetAngleZ(angle.z);
		}

		virtual void SetScaleX(float x) { scale_.x = x; }
		virtual void SetScaleY(float y) { scale_.y = y; }
		virtual void SetScaleZ(float z) { scale_.z = z; }
		virtual void SetShader(shared_ptr<Shader> shader);
	};

	//****************************************************************************
	//DxScriptTextObject
	//****************************************************************************
	class DxScriptTextObject : public DxScriptRenderObject {
		friend DxScript;
	private:
		enum : byte {
			CHANGE_INFO = 0x01,
			CHANGE_RENDERER = 0x02,
			CHANGE_ALL = CHANGE_INFO | CHANGE_RENDERER,
		};
	protected:
		byte change_;

		DxText text_;
		shared_ptr<DxTextInfo> textInfo_;
		shared_ptr<DxTextRenderObject> objRender_;

		D3DXVECTOR2 center_;	//Transformation center
		bool bAutoCenter_;

		DWORD charSet_;

		D3DXVECTOR2 angX_;
		D3DXVECTOR2 angY_;
		D3DXVECTOR2 angZ_;

		void _UpdateRenderer();
	public:
		DxScriptTextObject();

		virtual void Render();
		virtual void SetRenderState();

		void SetText(const std::wstring& text);
		std::wstring& GetText() { return text_.GetText(); }
		std::vector<size_t> GetTextCountCU();
		LONG GetTotalWidth();
		LONG GetTotalHeight();

		void SetFontType(const std::wstring& type) { 
			text_.SetFontType(type.c_str()); 
			change_ = CHANGE_ALL;
		}
		void SetFontSize(LONG size) {
			if (size == text_.GetFontSize()) return;
			text_.SetFontSize(size); change_ = CHANGE_ALL;
		}
		void SetFontWeight(LONG weight) {
			if (weight == text_.GetFontWeight()) return;
			text_.SetFontWeight(weight); change_ = CHANGE_ALL;
		}
		void SetFontItalic(bool bItalic) { 
			if (bItalic == text_.GetFontItalic()) return;
			text_.SetFontItalic(bItalic); change_ = CHANGE_ALL;
		}
		void SetFontUnderLine(bool bLine) { 
			if (bLine == text_.GetFontUnderLine()) return;
			text_.SetFontUnderLine(bLine); change_ = CHANGE_ALL;
		}

		void SetFontColorTop(byte r, byte g, byte b) { 
			text_.SetFontColorTop(D3DCOLOR_ARGB(255, r, g, b)); change_ = CHANGE_ALL;
		}
		void SetFontColorBottom(byte r, byte g, byte b) { 
			text_.SetFontColorBottom(D3DCOLOR_ARGB(255, r, g, b)); change_ = CHANGE_ALL;
		}
		void SetFontBorderWidth(LONG width) { 
			text_.SetFontBorderWidth(width); change_ = CHANGE_ALL;
		}
		void SetFontBorderType(TextBorderType type) {
			text_.SetFontBorderType(type); change_ = CHANGE_ALL;
		}
		void SetFontBorderColor(byte r, byte g, byte b) { 
			text_.SetFontBorderColor(D3DCOLOR_ARGB(255, r, g, b)); change_ = CHANGE_ALL;
		}

		void SetCharset(BYTE set);

		void SetMaxWidth(LONG width) { text_.SetMaxWidth(width); change_ = CHANGE_ALL; }
		void SetMaxHeight(LONG height) { text_.SetMaxHeight(height); change_ = CHANGE_ALL; }
		void SetLinePitch(float pitch) { text_.SetLinePitch(pitch); change_ = CHANGE_ALL; }
		void SetSidePitch(float pitch) { text_.SetSidePitch(pitch); change_ = CHANGE_ALL; }
		void SetFixedWidth(float width) { text_.SetFixedWidth(width); change_ = CHANGE_ALL; }
		void SetHorizontalAlignment(TextAlignment value) { text_.SetHorizontalAlignment(value); change_ = CHANGE_ALL; }
		void SetVerticalAlignment(TextAlignment value) { text_.SetVerticalAlignment(value); change_ = CHANGE_ALL; }
		void SetPermitCamera(bool bPermit) { text_.SetPermitCamera(bPermit); }
		void SetSyntacticAnalysis(bool bEnable) { text_.SetSyntacticAnalysis(bEnable); }

		virtual void SetAlpha(int alpha);
		virtual void SetColor(int r, int g, int b);
		void SetVertexColor(D3DCOLOR color) { text_.SetVertexColor(color); }
		virtual void SetShader(shared_ptr<Shader> shader);

		virtual void SetAngleX(float x);
		virtual void SetAngleY(float y);
		virtual void SetAngleZ(float z);
		virtual void SetAngle(const D3DXVECTOR3& angle) {
			SetAngleX(angle.x);
			SetAngleY(angle.y);
			SetAngleZ(angle.z);
		}
	};

	//****************************************************************************
	//DxSoundObject
	//****************************************************************************
	class DxSoundObject : public DxScriptObjectBase {
		friend DxScript;
	protected:
		std::unordered_map<SoundSourceData*, weak_ptr<SoundPlayer>> mapCachedPlayers_;
		shared_ptr<SoundPlayer> player_;
		SoundPlayer::PlayStyle style_;
	public:
		DxSoundObject();
		~DxSoundObject();

		virtual void Render() {}
		virtual void SetRenderState() {}

		bool Load(const std::wstring& path);
		void Play();

		shared_ptr<SoundPlayer> GetPlayer() { return player_; }
		SoundPlayer::PlayStyle& GetStyle() { return style_; }
	};

	//****************************************************************************
	//DxFileObject
	//****************************************************************************
	class DxFileObject : public DxScriptObjectBase {
		friend DxScript;
	protected:
		shared_ptr<gstd::File> file_;
		shared_ptr<gstd::FileReader> reader_;
		bool bWritable_;
	public:
		DxFileObject();
		~DxFileObject();

		shared_ptr<gstd::File> GetFile() { return file_; }

		virtual void Render() {}
		virtual void SetRenderState() {}

		virtual bool OpenR(const std::wstring& path);
		virtual bool OpenR(shared_ptr<gstd::FileReader> reader);
		virtual bool OpenRW(const std::wstring& path);
		virtual bool Store() = 0;
		virtual void Close();

		virtual bool IsWritable() { return bWritable_; }
	};

	//****************************************************************************
	//DxTextFileObject
	//****************************************************************************
	class DxTextFileObject : public DxFileObject {
	protected:
		std::vector<std::vector<char>> listLine_;

		gstd::Encoding::Type encoding_;
		size_t bomSize_;
		byte bomHead_[3];
		size_t bytePerChar_;
		char lineEnding_[4];
		size_t lineEndingSize_;

		bool _ParseLines(std::vector<char>& src);
		void _AddLine(const char* pChar, size_t count);
	public:
		DxTextFileObject();
		virtual ~DxTextFileObject();

		virtual bool OpenR(const std::wstring& path);
		virtual bool OpenR(shared_ptr<gstd::FileReader> reader);
		virtual bool OpenRW(const std::wstring& path);
		virtual bool Store();

		size_t GetLineCount() { return listLine_.size(); }
		std::string GetLineAsString(size_t line);
		std::wstring GetLineAsWString(size_t line);
		void SetLineAsString(const std::string& text, size_t line);
		void SetLineAsWString(const std::wstring& text, size_t line);

		void AddLine(const std::string& line);
		void AddLine(const std::wstring& line);
		void ClearLine() { listLine_.clear(); }
	};

	//****************************************************************************
	//DxBinaryFileObject
	//****************************************************************************
	class DxBinaryFileObject : public DxFileObject {
	protected:
		byte byteOrder_;
		byte codePage_;

		gstd::ByteBuffer* buffer_;
		size_t lastRead_;
	public:
		DxBinaryFileObject();
		virtual ~DxBinaryFileObject();

		virtual bool OpenR(const std::wstring& path);
		virtual bool OpenR(shared_ptr<gstd::FileReader> reader);
		virtual bool OpenRW(const std::wstring& path);
		virtual bool Store();

		gstd::ByteBuffer* const GetBuffer() { return buffer_; }

		byte GetCodePage() { return codePage_; }
		void SetCodePage(byte page) { codePage_ = page; }

		void SetByteOrder(byte order) { byteOrder_ = order; }
		byte GetByteOrder() { return byteOrder_; }

		bool IsReadableSize(size_t size);
		DWORD Read(LPVOID data, size_t size);
		DWORD Write(LPVOID data, size_t size);
		size_t GetLastReadSize() { return lastRead_; }
	};

	//****************************************************************************
	//DxScriptObjectManager
	//****************************************************************************
	class DxScriptObjectManager {
		friend DxScriptObjectBase;
	public:
		struct RenderList {
			std::vector<ref_unsync_ptr<DxScriptObjectBase>> list;
			size_t size = 0;

			void Add(ref_unsync_ptr<DxScriptObjectBase>& ptr);
			void Clear();

			std::vector<ref_unsync_ptr<DxScriptObjectBase>>::const_iterator begin() { return list.cbegin(); }
			std::vector<ref_unsync_ptr<DxScriptObjectBase>>::const_iterator end() { return begin() + size; }
		};
		struct FogData {
			bool enable;
			D3DCOLOR color;
			float start;
			float end;
		};

		enum : size_t {
			DEFAULT_CONTAINER_CAPACITY = 16384U,
		};
	protected:
		static FogData fogData_;
	protected:
		size_t totalObjectCreateCount_;
		std::list<int> listUnusedIndex_;

		std::vector<ref_unsync_ptr<DxScriptObjectBase>> obj_;
		std::list<ref_unsync_ptr<DxScriptObjectBase>> listActiveObject_;

		std::unordered_map<std::wstring, shared_ptr<SoundPlayer>> mapReservedSound_;

		std::vector<RenderList> listObjRender_;
		std::vector<shared_ptr<Shader>> listShader_;

		void _SetObjectID(DxScriptObjectBase* obj, int index) { obj->idObject_ = index; obj->manager_ = this; }
	public:
		DxScriptObjectManager();
		virtual ~DxScriptObjectManager();

		size_t GetMaxObject() { return obj_.size(); }
		bool SetMaxObject(size_t size);
		size_t GetAliveObjectCount() { return listActiveObject_.size(); }
		size_t GetRenderBucketCapacity() { return listObjRender_.size(); }
		void SetRenderBucketCapacity(size_t capacity);

		virtual int AddObject(ref_unsync_ptr<DxScriptObjectBase> obj, bool bActivate = true);
		//void AddObject(int id, shared_ptr<DxScriptObjectBase> obj, bool bActivate = true);
		void ActivateObject(int id, bool bActivate);
		void ActivateObject(ref_unsync_ptr<DxScriptObjectBase> obj, bool bActivate);

		ref_unsync_ptr<DxScriptObjectBase> GetObject(int id) {
			return ((id < 0 || id >= obj_.size()) ? nullptr : obj_[id]); 
		}

		std::vector<int> GetValidObjectIdentifier();

		DxScriptObjectBase* GetObjectPointer(int id);
		virtual void DeleteObject(int id);
		virtual void DeleteObject(ref_unsync_ptr<DxScriptObjectBase> obj);
		virtual void DeleteObject(DxScriptObjectBase* obj);

		void ClearObject();
		void DeleteObjectByScriptID(int64_t idScript);
		void OrphanObjectByScriptID(int64_t idScript);
		std::vector<int> GetObjectByScriptID(int64_t idScript);

		void AddRenderObject(ref_unsync_ptr<DxScriptObjectBase> obj);
		void WorkObject();
		virtual void RenderObject();
		void CleanupObject();

		virtual void PrepareRenderObject();
		void ClearRenderObject();
		std::vector<DxScriptObjectManager::RenderList>* GetRenderObjectListPointer() { return &listObjRender_; }

		void SetShader(shared_ptr<Shader> shader, int min, int max);
		void ResetShader();
		void ResetShader(int min, int max);
		shared_ptr<Shader> GetShader(int index);

		void ReserveSound(shared_ptr<SoundPlayer> player);
		void DeleteReservedSound(shared_ptr<SoundPlayer> player);
		shared_ptr<SoundPlayer> GetReservedSound(shared_ptr<SoundPlayer> player);

		size_t GetTotalObjectCreateCount() { return totalObjectCreateCount_; }

		static void SetFogParam(bool bEnable, D3DCOLOR fogColor, float start, float end);
		static FogData* GetFogData() { return &fogData_; }
		static bool IsFogEnable() { return fogData_.enable; }
		static D3DCOLOR GetFogColor() { return fogData_.color; }
		static float GetFogStart() { return fogData_.start; }
		static float GetFogEnd() { return fogData_.end; }
	};
}