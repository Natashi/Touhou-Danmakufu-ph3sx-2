#pragma once

#include "../pch.h"

#include "DxConstant.hpp"
#include "DxUtility.hpp"
#include "Texture.hpp"
#include "Shader.hpp"

namespace directx {
	//****************************************************************************
	//DirectionalLightingState
	//	Contains lighting data for 3D lighting
	//****************************************************************************
	class DirectionalLightingState {
	public:
		DirectionalLightingState();
		~DirectionalLightingState();

		void SetDirection(const D3DXVECTOR3& dir) { light_.Direction = dir; }
		void SetColorDiffuse(const D3DCOLORVALUE& col) { light_.Diffuse = col; }
		void SetColorSpecular(const D3DCOLORVALUE& col) { light_.Specular = col; }
		void SetColorAmbient(const D3DCOLORVALUE& col) { light_.Ambient = col; }

		void SetLightEnable(bool b) { bLightEnable_ = b; }
		void SetSpecularEnable(bool b) { bSpecularEnable_ = b; }

		void Apply();
	private:
		bool bLightEnable_;
		bool bSpecularEnable_;
		D3DLIGHT9 light_;
	};

	class DxScriptRenderObject;
	//****************************************************************************
	//RenderObject
	//	Base class for ObjRender
	//****************************************************************************
	class RenderObject {
	protected:
		DxScriptRenderObject* dxObjParent_;

		D3DPRIMITIVETYPE typePrimitive_;

		DirectionalLightingState lightParameter_;

		size_t strideVertexStreamZero_;			//byte size per vertex data
		std::vector<byte> vertex_;				//vertex data
		std::vector<uint16_t> vertexIndices_;	//Index data
		shared_ptr<Texture> texture_;
		D3DXVECTOR3 posWeightCenter_;

		D3DXVECTOR3 position_;
		D3DXVECTOR3 angle_;
		D3DXVECTOR3 scale_;
		shared_ptr<D3DXMATRIX> matRelative_;
		shared_ptr<Shader> shader_;

		bool bCoordinate2D_;
		bool disableMatrixTransform_;
		bool bVertexShaderMode_;
		bool flgUseVertexBufferMode_;
	public:
		RenderObject();
		virtual ~RenderObject();

		virtual void Render() = 0;

		virtual void CalculateWeightCenter() {}
		D3DXVECTOR3 GetWeightCenter() { return posWeightCenter_; }
		shared_ptr<Texture> GetTexture() { return texture_; }

		size_t _GetPrimitiveCount();
		size_t _GetPrimitiveCount(size_t count);

		void SetRalativeMatrix(shared_ptr<D3DXMATRIX>& mat) { matRelative_ = mat; }

		static D3DXMATRIX CreateWorldMatrix(const D3DXVECTOR3& position, const D3DXVECTOR3& scale,
			const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ,
			const D3DXMATRIX* matRelative, bool bCoordinate2D = false);
		static D3DXMATRIX CreateWorldMatrix(const D3DXVECTOR3& position, const D3DXVECTOR3& scale, const D3DXVECTOR3& angle,
			const D3DXMATRIX* matRelative, bool bCoordinate2D = false);
		static D3DXMATRIX CreateWorldMatrixSprite3D(const D3DXVECTOR3& position, const D3DXVECTOR3& scale,
			const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ,
			const D3DXMATRIX* matRelative, bool bBillboard = false);
		static D3DXMATRIX CreateWorldMatrix2D(const D3DXVECTOR3& position, const D3DXVECTOR3& scale,
			const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ, const D3DXMATRIX* matCamera);
		static D3DXMATRIX CreateWorldMatrixText2D(const D3DXVECTOR2& centerPosition, const D3DXVECTOR3& scale,
			const D3DXVECTOR2& angleX, const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ,
			const D3DXVECTOR2& objectPosition, const D3DXVECTOR2& biasPosition, const D3DXMATRIX* matCamera);
		static void SetCoordinate2dDeviceMatrix();

		void SetDxObjectReference(DxScriptRenderObject* obj) { dxObjParent_ = obj; }

		void SetPrimitiveType(D3DPRIMITIVETYPE type) { typePrimitive_ = type; }
		D3DPRIMITIVETYPE GetPrimitiveType() { return typePrimitive_; }
		virtual void SetVertexCount(size_t count) {
			count = std::min(count, 65536U);
			vertex_.resize(count * strideVertexStreamZero_);
			ZeroMemory(vertex_.data(), vertex_.size());
		}
		virtual size_t GetVertexCount() { return vertex_.size() / strideVertexStreamZero_; }
		void SetVertexIndices(const std::vector<uint16_t>& indices) { vertexIndices_ = indices; }

		void SetPosition(const D3DXVECTOR3& pos) { SetPosition(pos.x, pos.y, pos.z); }
		void SetPosition(float x, float y, float z);
		void SetX(float x);
		void SetY(float y);
		void SetZ(float z);

		void SetAngle(const D3DXVECTOR3& angle) { angle_ = angle; }
		void SetAngleXYZ(float angx = 0.0f, float angy = 0.0f, float angz = 0.0f) { angle_.x = angx; angle_.y = angy; angle_.z = angz; }

		void SetScale(const D3DXVECTOR3& scale) { SetScaleXYZ(scale.x, scale.y, scale.z); }
		void SetScaleXYZ(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);

		void SetTexture(shared_ptr<Texture> texture) { texture_ = texture; }

		bool IsCoordinate2D() { return bCoordinate2D_; }
		void SetCoordinate2D(bool b) { bCoordinate2D_ = b; }

		void SetDisableMatrixTransformation(bool b) { disableMatrixTransform_ = b; }
		void SetVertexShaderRendering(bool b) { bVertexShaderMode_ = b; }

		DirectionalLightingState* GetLighting() { return &lightParameter_; }

		shared_ptr<Shader> GetShader() { return shader_; }
		void SetShader(shared_ptr<Shader> shader) { shader_ = shader; }
	};

	//****************************************************************************
	//RenderObjectTLX
	//	2D render object
	//****************************************************************************
	class RenderObjectTLX : public RenderObject {
	protected:
		bool bPermitCamera_;
		std::vector<byte> vertCopy_;
	public:
		RenderObjectTLX();
		~RenderObjectTLX();

		virtual void Render();
		virtual void Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ);
		virtual void Render(const D3DXMATRIX& matTransform);

		virtual void SetVertexCount(size_t count);

		VERTEX_TLX* GetVertex(size_t index);
		void SetVertex(size_t index, const VERTEX_TLX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z = 1.0f, float w = 1.0f);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexColor(size_t index, D3DCOLOR color);
		void SetVertexColorARGB(size_t index, int a, int r, int g, int b);
		void SetVertexAlpha(size_t index, int alpha);
		void SetVertexColorRGB(size_t index, int r, int g, int b);
		void SetVertexColorRGB(size_t index, D3DCOLOR rgb);
		D3DCOLOR GetVertexColor(size_t index);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);

		bool IsPermitCamera() { return bPermitCamera_; }
		void SetPermitCamera(bool bPermit) { bPermitCamera_ = bPermit; }
	};

	//****************************************************************************
	//RenderObjectLX
	//	3D render object
	//****************************************************************************
	class RenderObjectLX : public RenderObject {
	public:
		RenderObjectLX();
		~RenderObjectLX();

		virtual void Render();
		virtual void Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ);
		virtual void Render(const D3DXMATRIX& matTransform);
		
		virtual void SetVertexCount(size_t count);

		VERTEX_LX* GetVertex(size_t index);
		void SetVertex(size_t index, const VERTEX_LX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexColor(size_t index, D3DCOLOR color);
		void SetVertexColorARGB(size_t index, int a, int r, int g, int b);
		void SetVertexAlpha(size_t index, int alpha);
		void SetVertexColorRGB(size_t index, int r, int g, int b);
		void SetVertexColorRGB(size_t index, D3DCOLOR rgb);
		D3DCOLOR GetVertexColor(size_t index);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);
	};

	//****************************************************************************
	//RenderObjectNX
	//	3D render object with vertex normal data
	//	For meshes
	//****************************************************************************
	class RenderObjectNX : public RenderObject {
	protected:
		D3DCOLOR color_;

		IDirect3DVertexBuffer9* pVertexBuffer_;
		IDirect3DIndexBuffer9* pIndexBuffer_;
	public:
		RenderObjectNX();
		~RenderObjectNX();
		
		virtual void Render();
		virtual void Render(D3DXMATRIX* matTransform);
		
		VERTEX_NX* GetVertex(size_t index);
		void SetVertex(size_t index, const VERTEX_NX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexNormal(size_t index, float x, float y, float z);
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	//****************************************************************************
	//Sprite2D
	//	RenderObjectTLX with pre-defined 4-vertex layout
	//****************************************************************************
	class Sprite2D : public RenderObjectTLX {
	public:
		Sprite2D();
		~Sprite2D();
		
		void Copy(Sprite2D* src);
		void SetSourceRect(const DxRect<int>& rcSrc);
		void SetDestinationRect(const DxRect<double>& rcDest);
		void SetDestinationCenter();
		void SetVertex(const DxRect<int>& rcSrc, const DxRect<double>& rcDest, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));

		DxRect<double> GetDestinationRect();
	};

	//****************************************************************************
	//SpriteList2D
	//	Render list of 2D sprites
	//****************************************************************************
	class SpriteList2D : public RenderObjectTLX {
		size_t countRenderIndex_;
		size_t countRenderIndexPrev_;
		size_t countRenderVertex_;
		size_t countRenderVertexPrev_;

		DxRect<int> rcSrc_;
		DxRect<double> rcDest_;

		D3DCOLOR color_;

		bool bCloseVertexList_;
		bool autoClearVertexList_;

		void _AddVertex(VERTEX_TLX(&verts)[4]);
	public:
		SpriteList2D();

		virtual size_t GetVertexCount() { return GetVertexCount(countRenderIndex_); }
		virtual size_t GetVertexCount(size_t count) {
			return std::min(count, vertexIndices_.size());
		}

		virtual void Render();
		virtual void Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ);

		void ClearVertexCount() { 
			countRenderIndex_ = 0;
			countRenderVertex_ = 0;
			bCloseVertexList_ = false; 
		}
		void CleanUp();

		void AddVertex();
		void AddVertex(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ);
		void SetSourceRect(const DxRect<int>& rcSrc) { rcSrc_ = rcSrc; }
		void SetDestinationRect(const DxRect<double>& rcDest) { rcDest_ = rcDest; }
		void SetDestinationCenter();
		D3DCOLOR GetColor() { return color_; }
		void SetColor(D3DCOLOR color);
		void CloseVertex();

		void SetAutoClearVertex(bool clear) { autoClearVertexList_ = clear; }
	};

	//****************************************************************************
	//Sprite3D
	//	RenderObjectLX with pre-defined 4-vertex layout
	//****************************************************************************
	class Sprite3D : public RenderObjectLX {
	protected:
		bool bBillboard_;
	public:
		Sprite3D();
		~Sprite3D();

		virtual void Render();
		virtual void Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ);

		void SetSourceRect(const DxRect<int>& rcSrc);
		void SetDestinationRect(const DxRect<double>& rcDest);
		void SetVertex(const DxRect<int>& rcSrc, const DxRect<double>& rcDest, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));
		void SetSourceDestRect(const DxRect<double>& rcSrc);
		void SetVertex(const DxRect<double>& rcSrcDst, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));
		void SetBillboardEnable(bool bEnable) { bBillboard_ = bEnable; }
	};

	//****************************************************************************
	//TrajectoryObject3D
	//	Fuck?
	//****************************************************************************
	class TrajectoryObject3D : public RenderObjectLX {
		struct Data {
			int alpha;
			D3DXVECTOR3 pos1;
			D3DXVECTOR3 pos2;
		};
	protected:
		D3DCOLOR color_;
		int diffAlpha_;
		int countComplement_;
		Data dataInit_;
		Data dataLast1_;
		Data dataLast2_;
		std::list<Data> listData_;

		virtual D3DXMATRIX _CreateWorldTransformMatrix();
	public:
		TrajectoryObject3D();
		~TrajectoryObject3D();
		
		virtual void Work();
		virtual void Render();
		virtual void Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ);
		
		void SetInitialLine(const D3DXVECTOR3& pos1, const D3DXVECTOR3& pos2) {
			dataInit_.pos1 = pos1;
			dataInit_.pos2 = pos2;
		}
		void AddPoint(const D3DXMATRIX& mat);
		void SetAlphaVariation(int diff) { diffAlpha_ = diff; }
		void SetComplementCount(int count) { countComplement_ = count; }
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	//****************************************************************************
	//ParticleRendererBase
	//	Base class for instanced render objects
	//****************************************************************************
	class ParticleRendererBase {
	public:
		ParticleRendererBase();
		virtual ~ParticleRendererBase();

		void AddInstance();
		void ClearInstance();

		void SetInstanceColor(D3DCOLOR color) { instColor_ = color; }
		void SetInstanceColorRGB(int r, int g, int b);
		void SetInstanceColorRGB(D3DCOLOR color);
		void SetInstanceAlpha(int alpha);

		void SetInstancePosition(float x, float y, float z) { SetInstancePosition(D3DXVECTOR3(x, y, z)); }
		void SetInstancePosition(const D3DXVECTOR3& pos);

		void SetInstanceScaleSingle(size_t index, float sc);
		void SetInstanceScale(float x, float y, float z) { SetInstanceScale(D3DXVECTOR3(x, y, z)); }
		void SetInstanceScale(const D3DXVECTOR3& scale);

		void SetInstanceAngleSingle(size_t index, float ang);
		void SetInstanceAngle(float x, float y, float z) { SetInstanceAngle(D3DXVECTOR3(x, y, z)); }
		void SetInstanceAngle(const D3DXVECTOR3& angle) {
			SetInstanceAngleSingle(0, angle.x);
			SetInstanceAngleSingle(1, angle.y);
			SetInstanceAngleSingle(2, angle.z);
		}

		void SetInstanceUserData(const D3DXVECTOR3& data) { instUserData_ = data; }
	protected:
		size_t countInstance_;
		size_t countInstancePrev_;
		std::vector<VERTEX_INSTANCE> instanceData_;
		
		D3DCOLOR instColor_;
		D3DXVECTOR3 instPosition_;
		D3DXVECTOR3 instScale_;
		D3DXVECTOR3 instAngle_;
		D3DXVECTOR3 instUserData_;
	};

	//****************************************************************************
	//ParticleRenderer2D
	//	2D render object utilizing hardware instancing
	//****************************************************************************
	class ParticleRenderer2D : public ParticleRendererBase, public Sprite2D {
	public:
		ParticleRenderer2D();

		virtual void Render();
	};

	//****************************************************************************
	//ParticleRenderer3D
	//	3D render object utilizing hardware instancing
	//****************************************************************************
	class ParticleRenderer3D : public ParticleRendererBase, public Sprite3D {
	public:
		ParticleRenderer3D();

		virtual void Render();
	};

	class DxMeshManager;
	class DxMeshData {
	public:
		friend DxMeshManager;
	protected:
		std::wstring name_;
		DxMeshManager* manager_;
		volatile bool bLoad_;
	public:
		DxMeshData();
		virtual ~DxMeshData();

		void SetName(const std::wstring& name) { name_ = name; }
		std::wstring& GetName() { return name_; }

		virtual bool CreateFromFileReader(shared_ptr<gstd::FileReader> reader) = 0;
	};
	class DxMesh : public gstd::FileManager::LoadObject {
	public:
		friend DxMeshManager;
		enum {
			MESH_METASEQUOIA,
		};
	protected:
		DxScriptRenderObject* dxObjParent_;

		bool bVertexShaderMode_;
		bool bCoordinate2D_;

		DirectionalLightingState lightParameter_;

		D3DXVECTOR3 position_;
		D3DXVECTOR3 angle_;
		D3DXVECTOR3 scale_;
		D3DCOLOR color_;
		
		shared_ptr<Shader> shader_;

		shared_ptr<DxMeshData> data_;
		shared_ptr<DxMeshData> _GetFromManager(const std::wstring& name);
		void _AddManager(const std::wstring& name, shared_ptr<DxMeshData> data);
	public:
		DxMesh();
		virtual ~DxMesh();

		virtual void Release();
		bool CreateFromFile(const std::wstring& path);
		virtual bool CreateFromFileReader(shared_ptr<gstd::FileReader> reader) = 0;
		virtual bool CreateFromFileInLoadThread(const std::wstring& path, int type);
		virtual bool CreateFromFileInLoadThread(const std::wstring& path) = 0;
		virtual std::wstring GetPath() = 0;

		virtual void Render() = 0;
		virtual void Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) = 0;
		virtual inline void Render(const std::wstring& nameAnime, int time) { Render(); }
		virtual inline void Render(const std::wstring& nameAnime, int time,
			const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) { Render(angX, angY, angZ); }

		void SetPosition(const D3DXVECTOR3& pos) { SetPosition(pos.x, pos.y, pos.z); }
		void SetPosition(float x, float y, float z);
		void SetX(float x);
		void SetY(float y);
		void SetZ(float z);

		void SetAngle(const D3DXVECTOR3& angle) { angle_ = angle; }
		void SetAngleXYZ(float angx = 0.0f, float angy = 0.0f, float angz = 0.0f) { angle_.x = angx; angle_.y = angy; angle_.z = angz; }

		void SetScale(const D3DXVECTOR3& scale) { SetScaleXYZ(scale.x, scale.y, scale.z); }
		void SetScaleXYZ(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);
		
		void SetColor(D3DCOLOR color) { color_ = color; }
		inline void SetColorRGB(D3DCOLOR color) {
			color_ = (color_ & 0xff000000) | (color & 0x00ffffff);
		}
		inline void SetAlpha(int alpha) {
			color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
		}

		void SetVertexShaderRendering(bool b) { bVertexShaderMode_ = b; }

		DirectionalLightingState* GetLighting() { return &lightParameter_; }

		bool IsCoordinate2D() { return bCoordinate2D_; }
		void SetCoordinate2D(bool b) { bCoordinate2D_ = b; }

		void SetDxObjectReference(DxScriptRenderObject* obj) { dxObjParent_ = obj; }

		//gstd::ref_count_ptr<RenderBlocks> CreateRenderBlocks() { return nullptr; }
		virtual D3DXMATRIX GetAnimationMatrix(const std::wstring& nameAnime, double time, const std::wstring& nameBone) {
			D3DXMATRIX mat; 
			D3DXMatrixIdentity(&mat); 
			return mat; 
		}
		shared_ptr<Shader> GetShader() { return shader_; }
		void SetShader(shared_ptr<Shader> shader) { shader_ = shader; }
	};

	class DxMeshInfoPanel;
	//****************************************************************************
	//DxMeshManager
	//	Manager class for all mesh resources
	//****************************************************************************
	class DxMeshManager : public gstd::FileManager::LoadThreadListener {
		friend DxMeshData;
		friend DxMesh;
		friend DxMeshInfoPanel;
		static DxMeshManager* thisBase_;
	protected:
		gstd::CriticalSection lock_;

		std::map<std::wstring, shared_ptr<DxMesh>> mapMesh_;
		std::map<std::wstring, shared_ptr<DxMeshData>> mapMeshData_;

		shared_ptr<DxMeshInfoPanel> panelInfo_;

		void _AddMeshData(const std::wstring& name, shared_ptr<DxMeshData> data);
		shared_ptr<DxMeshData> _GetMeshData(const std::wstring& name);
		void _ReleaseMeshData(const std::wstring& name);
	public:
		DxMeshManager();
		virtual ~DxMeshManager();

		static DxMeshManager* GetBase() { return thisBase_; }

		bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }

		virtual void Clear();
		virtual void Add(const std::wstring& name, shared_ptr<DxMesh> mesh);
		virtual void Release(const std::wstring& name);
		virtual bool IsDataExists(const std::wstring& name);

		shared_ptr<DxMesh> CreateFromFileInLoadThread(const std::wstring& path, int type);
		virtual void CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event);

		void SetInfoPanel(shared_ptr<DxMeshInfoPanel> panel) { panelInfo_ = panel; }
	};

	class DxMeshInfoPanel : public gstd::WindowLogger::Panel, public gstd::Thread {
	protected:
		enum {
			ROW_ADDRESS,
			ROW_NAME,
			ROW_FULLNAME,
			ROW_COUNT_REFFRENCE,
		};
		int timeUpdateInterval_;
		gstd::WListView wndListView_;
		virtual bool _AddedLogger(HWND hTab);
		void _Run();
	public:
		DxMeshInfoPanel();
		~DxMeshInfoPanel();

		virtual void LocateParts();
		virtual void Update(DxMeshManager* manager);
	};
}