#ifndef __DIRECTX_RENDEROBJECT__
#define __DIRECTX_RENDEROBJECT__

#include "../pch.h"

#include "DxConstant.hpp"
#include "DxUtility.hpp"
#include "Vertex.hpp"
#include "Texture.hpp"
#include "Shader.hpp"

namespace directx {
	class RenderObjectBase;
	class RenderManager;
	class RenderStateFunction;
	class RenderBlock;
	class RenderObject;

	/**********************************************************
	//RenderBlock
	**********************************************************/
	class RenderBlock {
	protected:
		float posSortKey_;
		gstd::ref_count_ptr<RenderStateFunction> func_;
		gstd::ref_count_ptr<RenderObject> obj_;

		D3DXVECTOR3 position_;//移動先座標
		D3DXVECTOR3 angle_;//回転角度
		D3DXVECTOR3 scale_;//拡大率

	public:
		RenderBlock();
		virtual ~RenderBlock();
		void SetRenderFunction(gstd::ref_count_ptr<RenderStateFunction> func) { func_ = func; }
		virtual void Render();

		virtual void CalculateZValue() = 0;
		float GetZValue() { return posSortKey_; }
		void SetZValue(float pos) { posSortKey_ = pos; }
		virtual bool IsTranslucent() = 0;//Zソート対象に使用

		void SetRenderObject(gstd::ref_count_ptr<RenderObject> obj) { obj_ = obj; }
		gstd::ref_count_ptr<RenderObject> GetRenderObject() { return obj_; }
		void SetPosition(D3DXVECTOR3& pos) { position_ = pos; }
		void SetAngle(D3DXVECTOR3& angle) { angle_ = angle; }
		void SetScale(D3DXVECTOR3& scale) { scale_ = scale; }
	};

	class RenderBlocks {
	protected:
		std::list<gstd::ref_count_ptr<RenderBlock> > listBlock_;
	public:
		RenderBlocks() {};
		virtual ~RenderBlocks() {};
		void Add(gstd::ref_count_ptr<RenderBlock> block) { listBlock_.push_back(block); }
		std::list<gstd::ref_count_ptr<RenderBlock> >& GetList() { return listBlock_; }

	};

	/**********************************************************
	//RenderManager
	//レンダリング管理
	//3D不透明オブジェクト
	//3D半透明オブジェクトZソート順
	//2Dオブジェクト
	//順に描画する
	**********************************************************/
	class RenderManager {
		class ComparatorRenderBlockTranslucent;
	protected:
		std::list<gstd::ref_count_ptr<RenderBlock> > listBlockOpaque_;
		std::list<gstd::ref_count_ptr<RenderBlock> > listBlockTranslucent_;
	public:
		RenderManager();
		virtual ~RenderManager();
		virtual void Render();
		void AddBlock(gstd::ref_count_ptr<RenderBlock> block);
		void AddBlock(gstd::ref_count_ptr<RenderBlocks> blocks);
	};

	class RenderManager::ComparatorRenderBlockTranslucent {
	public:
		bool operator()(gstd::ref_count_ptr<RenderBlock> l, gstd::ref_count_ptr<RenderBlock> r) {
			return l->GetZValue() > r->GetZValue();
		}
	};

	/**********************************************************
	//RenderStateFunction
	**********************************************************/
	class RenderStateFunction {
		friend RenderObjectBase;
		enum FUNC_TYPE {
			FUNC_LIGHTING,
			FUNC_CULLING,
			FUNC_ZBUFFER_ENABLE,
			FUNC_ZBUFFER_WRITE_ENABLE,
			FUNC_BLEND,
			FUNC_TEXTURE_FILTER,
		};

		std::map<FUNC_TYPE, gstd::ref_count_ptr<gstd::ByteBuffer> > mapFuncRenderState_;
	public:
		RenderStateFunction();
		virtual ~RenderStateFunction();
		void CallRenderStateFunction();

		//レンダリングステート設定(RenderManager用)
		void SetLightingEnable(bool bEnable);//ライティング
		void SetCullingMode(DWORD mode);//カリング
		void SetZBufferEnable(bool bEnable);//Zバッファ参照
		void SetZWriteEnable(bool bEnable);//Zバッファ書き込み
		void SetBlendMode(DWORD mode, int stage = 0);
		void SetTextureFilter(DWORD mode, int stage = 0);
	};

	class Matrices {
		std::vector<D3DXMATRIX> matrix_;
	public:
		Matrices() {};
		virtual ~Matrices() {};
		void SetSize(size_t size) { 
			matrix_.resize(size); 
			for (size_t iMat = 0; iMat < size; iMat++)
				D3DXMatrixIdentity(&matrix_[iMat]);
		}
		size_t GetSize() { return matrix_.size(); }
		void SetMatrix(size_t index, D3DXMATRIX& mat) { matrix_[index] = mat; }
		D3DXMATRIX& GetMatrix(size_t index) { return matrix_[index]; }
	};

	class DirectionalLightingState {
	public:
		DirectionalLightingState();
		~DirectionalLightingState();

		void SetDirection(D3DXVECTOR3& dir) { light_.Direction = dir; }
		void SetColorDiffuse(D3DCOLORVALUE& col) { light_.Diffuse = col; }
		void SetColorSpecular(D3DCOLORVALUE& col) { light_.Specular = col; }
		void SetColorAmbient(D3DCOLORVALUE& col) { light_.Ambient = col; }

		void SetLightEnable(bool b) { bLightEnable_ = b; }
		void SetSpecularEnable(bool b) { bSpecularEnable_ = b; }

		void Apply();
	private:
		bool bLightEnable_;
		bool bSpecularEnable_;
		D3DLIGHT9 light_;
	};

	/**********************************************************
	//RenderObject
	//レンダリングオブジェクト
	//描画の最小単位
	//RenderManagerに登録して描画してもらう
	//(直接描画も可能)
	**********************************************************/
	class RenderObject {
	protected:
		D3DPRIMITIVETYPE typePrimitive_;
		D3DTEXTUREFILTERTYPE filterMin_;
		D3DTEXTUREFILTERTYPE filterMag_;
		D3DTEXTUREFILTERTYPE filterMip_;
		D3DCULL modeCulling_;

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

		static D3DXMATRIX CreateWorldMatrix(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
			D3DXMATRIX* matRelative, bool bCoordinate2D = false);
		static D3DXMATRIX CreateWorldMatrix(D3DXVECTOR3& position, D3DXVECTOR3& scale, D3DXVECTOR3& angle,
			D3DXMATRIX* matRelative, bool bCoordinate2D = false);
		static D3DXMATRIX CreateWorldMatrixSprite3D(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ,
			D3DXMATRIX* matRelative, bool bBillboard = false);
		static D3DXMATRIX CreateWorldMatrix2D(D3DXVECTOR3& position, D3DXVECTOR3& scale, 
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ, D3DXMATRIX* matCamera);
		static D3DXMATRIX CreateWorldMatrixText2D(D3DXVECTOR2& centerPosition, D3DXVECTOR3& scale,
			D3DXVECTOR2& angleX, D3DXVECTOR2& angleY, D3DXVECTOR2& angleZ, 
			D3DXVECTOR2& objectPosition, D3DXVECTOR2& biasPosition, D3DXMATRIX* matCamera);
		static void SetCoordinate2dDeviceMatrix();

		void SetPrimitiveType(D3DPRIMITIVETYPE type) { typePrimitive_ = type; }
		D3DPRIMITIVETYPE GetPrimitiveType() { return typePrimitive_; }
		virtual void SetVertexCount(size_t count) {
			count = std::min(count, 65536U);
			vertex_.resize(count * strideVertexStreamZero_);
			ZeroMemory(vertex_.data(), vertex_.size());
		}
		virtual size_t GetVertexCount() { return vertex_.size() / strideVertexStreamZero_; }
		void SetVertexIndices(std::vector<uint16_t>& indices) { vertexIndices_ = indices; }

		void SetPosition(D3DXVECTOR3& pos) { position_ = pos; }
		void SetPosition(float x, float y, float z) { position_.x = x; position_.y = y; position_.z = z; }
		void SetX(float x) { position_.x = x; }
		void SetY(float y) { position_.y = y; }
		void SetZ(float z) { position_.z = z; }
		void SetAngle(D3DXVECTOR3& angle) { angle_ = angle; }
		void SetAngleXYZ(float angx = 0.0f, float angy = 0.0f, float angz = 0.0f) { angle_.x = angx; angle_.y = angy; angle_.z = angz; }
		void SetScale(D3DXVECTOR3& scale) { scale_ = scale; }
		void SetScaleXYZ(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) { scale_.x = sx; scale_.y = sy; scale_.z = sz; }
		void SetTexture(Texture* texture) { 
			if (texture)
				texture_ = std::make_shared<Texture>(texture);
			else texture_ = nullptr;
		}
		void SetTexture(shared_ptr<Texture> texture) { texture_ = texture; }

		bool IsCoordinate2D() { return bCoordinate2D_; }
		void SetCoordinate2D(bool b) { bCoordinate2D_ = b; }

		void SetDisableMatrixTransformation(bool b) { disableMatrixTransform_ = b; }
		void SetVertexShaderRendering(bool b) { bVertexShaderMode_ = b; }

		void SetFilteringMin(D3DTEXTUREFILTERTYPE filter) { filterMin_ = filter; }
		void SetFilteringMag(D3DTEXTUREFILTERTYPE filter) { filterMag_ = filter; }
		void SetFilteringMip(D3DTEXTUREFILTERTYPE filter) { filterMip_ = filter; }
		D3DTEXTUREFILTERTYPE GetFilteringMin() { return filterMin_; }
		D3DTEXTUREFILTERTYPE GetFilteringMag() { return filterMag_; }
		D3DTEXTUREFILTERTYPE GetFilteringMip() { return filterMip_; }

		DirectionalLightingState* GetLighting() { return &lightParameter_; }

		shared_ptr<Shader> GetShader() { return shader_; }
		void SetShader(shared_ptr<Shader> shader) { shader_ = shader; }
	};

	/**********************************************************
	//RenderObjectTLX
	//座標3D変換済み、ライティング済み、テクスチャ有り
	//2D自由変形スプライト用
	**********************************************************/
	class RenderObjectTLX : public RenderObject {
	protected:
		bool bPermitCamera_;
		std::vector<byte> vertCopy_;
	public:
		RenderObjectTLX();
		~RenderObjectTLX();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		virtual void Render(D3DXMATRIX& matTransform);
		virtual void SetVertexCount(size_t count);

		//頂点設定
		VERTEX_TLX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_TLX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z = 1.0f, float w = 1.0f);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexColor(size_t index, D3DCOLOR color);
		void SetVertexColorARGB(size_t index, int a, int r, int g, int b);
		void SetVertexAlpha(size_t index, int alpha);
		void SetVertexColorRGB(size_t index, int r, int g, int b);
		D3DCOLOR GetVertexColor(size_t index);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);

		//カメラ
		bool IsPermitCamera() { return bPermitCamera_; }
		void SetPermitCamera(bool bPermit) { bPermitCamera_ = bPermit; }
	};

	/**********************************************************
	//RenderObjectLX
	//ライティング済み、テクスチャ有り
	//3Dエフェクト用
	**********************************************************/
	class RenderObjectLX : public RenderObject {
	public:
		RenderObjectLX();
		~RenderObjectLX();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		virtual void Render(D3DXMATRIX& matTransform);
		virtual void SetVertexCount(size_t count);

		//頂点設定
		VERTEX_LX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_LX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexColor(size_t index, D3DCOLOR color);
		void SetVertexColorARGB(size_t index, int a, int r, int g, int b);
		void SetVertexAlpha(size_t index, int alpha);
		void SetVertexColorRGB(size_t index, int r, int g, int b);
		D3DCOLOR GetVertexColor(size_t index);
		void SetColorRGB(D3DCOLOR color);
		void SetAlpha(int alpha);
	};

	/**********************************************************
	//RenderObjectNX
	//法線有り、テクスチャ有り
	**********************************************************/
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

		//頂点設定
		VERTEX_NX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_NX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexNormal(size_t index, float x, float y, float z);
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	/**********************************************************
	//RenderObjectBNX
	//頂点ブレンド
	//法線有り
	//テクスチャ有り
	**********************************************************/
	class RenderObjectBNX : public RenderObject {
	protected:
		gstd::ref_count_ptr<Matrices> matrix_;

		D3DCOLOR color_;
		D3DMATERIAL9 materialBNX_;

		IDirect3DVertexBuffer9* pVertexBuffer_;
		IDirect3DIndexBuffer9* pIndexBuffer_;

		virtual void _CopyVertexBufferOnInitialize() = 0;
	public:
		RenderObjectBNX();
		~RenderObjectBNX();

		void InitializeVertexBuffer();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);

		//描画用設定
		void SetMatrix(gstd::ref_count_ptr<Matrices> matrix) { matrix_ = matrix; }
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	class RenderObjectBNXBlock : public RenderBlock {
	protected:
		gstd::ref_count_ptr<Matrices> matrix_;
		D3DCOLOR color_;

	public:
		void SetMatrix(gstd::ref_count_ptr<Matrices> matrix) { matrix_ = matrix; }
		void SetColor(D3DCOLOR color) { color_ = color; }
		bool IsTranslucent() { return ColorAccess::GetColorA(color_) != 255; }
	};

	/**********************************************************
	//RenderObjectB2NX
	//頂点ブレンド2
	//法線有り
	//テクスチャ有り
	**********************************************************/
	class RenderObjectB2NX : public RenderObjectBNX {
	protected:
		virtual void _CopyVertexBufferOnInitialize();
	public:
		RenderObjectB2NX();
		~RenderObjectB2NX();

		virtual void CalculateWeightCenter();

		//頂点設定
		VERTEX_B2NX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_B2NX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexBlend(size_t index, int pos, BYTE indexBlend, float rate);
		void SetVertexNormal(size_t index, float x, float y, float z);
	};

	class RenderObjectB2NXBlock : public RenderObjectBNXBlock {
	public:
		RenderObjectB2NXBlock();
		virtual ~RenderObjectB2NXBlock();
		virtual void Render();
	};

	/**********************************************************
	//RenderObjectB4NX
	//頂点ブレンド4
	//法線有り
	//テクスチャ有り
	**********************************************************/
	class RenderObjectB4NX : public RenderObjectBNX {
	protected:
		virtual void _CopyVertexBufferOnInitialize();
	public:
		RenderObjectB4NX();
		~RenderObjectB4NX();

		virtual void CalculateWeightCenter();

		//頂点設定
		VERTEX_B4NX* GetVertex(size_t index);
		void SetVertex(size_t index, VERTEX_B4NX& vertex);
		void SetVertexPosition(size_t index, float x, float y, float z);
		void SetVertexUV(size_t index, float u, float v);
		void SetVertexBlend(size_t index, int pos, BYTE indexBlend, float rate);
		void SetVertexNormal(size_t index, float x, float y, float z);
	};

	class RenderObjectB4NXBlock : public RenderObjectBNXBlock {
	public:
		RenderObjectB4NXBlock();
		virtual ~RenderObjectB4NXBlock();
		virtual void Render();
	};

	/**********************************************************
	//Sprite2D
	//矩形スプライト
	**********************************************************/
	class Sprite2D : public RenderObjectTLX {
	public:
		Sprite2D();
		~Sprite2D();
		void Copy(Sprite2D* src);
		void SetSourceRect(RECT_D &rcSrc);
		void SetDestinationRect(RECT_D &rcDest);
		void SetDestinationCenter();
		void SetVertex(RECT_D &rcSrc, RECT_D &rcDest, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));

		RECT_D GetDestinationRect();
	};

	/**********************************************************
	//SpriteList2D
	//矩形スプライトリスト
	**********************************************************/
	class SpriteList2D : public RenderObjectTLX {
		size_t countRenderVertex_;
		size_t countRenderVertexPrev_;
		RECT_D rcSrc_;
		RECT_D rcDest_;
		D3DCOLOR color_;
		bool bCloseVertexList_;
		bool autoClearVertexList_;
		void _AddVertex(VERTEX_TLX& vertex);
	public:
		SpriteList2D();

		virtual size_t GetVertexCount() { return GetVertexCount(countRenderVertex_); }
		virtual size_t GetVertexCount(size_t count) {
			return std::min(count, vertex_.size() / strideVertexStreamZero_);
		}

		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);

		void ClearVertexCount() { countRenderVertex_ = 0; bCloseVertexList_ = false; }
		void CleanUp();

		void AddVertex();
		void AddVertex(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		void SetSourceRect(RECT_D &rcSrc) { rcSrc_ = rcSrc; }
		void SetDestinationRect(RECT_D &rcDest) { rcDest_ = rcDest; }
		void SetDestinationCenter();
		D3DCOLOR GetColor() { return color_; }
		void SetColor(D3DCOLOR color) { color_ = color; }
		void CloseVertex();

		void SetAutoClearVertex(bool clear) { autoClearVertexList_ = clear; }
	};

	/**********************************************************
	//Sprite3D
	//矩形スプライト
	**********************************************************/
	class Sprite3D : public RenderObjectLX {
	protected:
		bool bBillboard_;
	public:
		Sprite3D();
		~Sprite3D();

		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);

		void SetSourceRect(RECT_D &rcSrc);
		void SetDestinationRect(RECT_D &rcDest);
		void SetVertex(RECT_D &rcSrc, RECT_D &rcDest, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));
		void SetSourceDestRect(RECT_D &rcSrc);
		void SetVertex(RECT_D &rcSrc, D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255));
		void SetBillboardEnable(bool bEnable) { bBillboard_ = bEnable; }
	};

	/**********************************************************
	//TrajectoryObject3D
	//3D軌跡
	**********************************************************/
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
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ);
		void SetInitialLine(D3DXVECTOR3 pos1, D3DXVECTOR3 pos2) {
			dataInit_.pos1 = pos1;
			dataInit_.pos2 = pos2;
		}
		void AddPoint(D3DXMATRIX mat);
		void SetAlphaVariation(int diff) { diffAlpha_ = diff; }
		void SetComplementCount(int count) { countComplement_ = count; }
		void SetColor(D3DCOLOR color) { color_ = color; }
	};

	/**********************************************************
	//ParticleRendererBase
	**********************************************************/
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
		void SetInstancePosition(D3DXVECTOR3 pos) { instPosition_ = pos; }

		void SetInstanceScaleSingle(size_t index, float sc);
		void SetInstanceScale(float x, float y, float z) { SetInstanceScale(D3DXVECTOR3(x, y, z)); }
		void SetInstanceScale(D3DXVECTOR3 scale) { instScale_ = scale; }

		void SetInstanceAngleSingle(size_t index, float ang);
		void SetInstanceAngle(float x, float y, float z) { SetInstanceAngle(D3DXVECTOR3(x, y, z)); }
		void SetInstanceAngle(D3DXVECTOR3 angle) {
			SetInstanceAngleSingle(0, angle.x);
			SetInstanceAngleSingle(1, angle.y);
			SetInstanceAngleSingle(2, angle.z);
		}

		void SetInstanceUserData(D3DXVECTOR3 data) { instUserData_ = data; }
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
	/**********************************************************
	//ParticleRenderer2D
	**********************************************************/
	class ParticleRenderer2D : public ParticleRendererBase, public Sprite2D {
	public:
		ParticleRenderer2D();
		virtual void Render();
	};
	/**********************************************************
	//ParticleRenderer3D
	**********************************************************/
	class ParticleRenderer3D : public ParticleRendererBase, public Sprite3D {
	private:
		bool bUseFog_;
	public:
		ParticleRenderer3D();
		virtual void Render();

		void SetFog(bool bFog) { bUseFog_ = bFog; }
	};

	/**********************************************************
	//DxMesh
	**********************************************************/
	enum {
		MESH_ELFREINA,
		MESH_METASEQUOIA,
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
		void SetName(std::wstring name) { name_ = name; }
		std::wstring& GetName() { return name_; }
		virtual bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader) = 0;
	};
	class DxMesh : public gstd::FileManager::LoadObject {
	public:
		friend DxMeshManager;
	protected:
		D3DTEXTUREFILTERTYPE filterMin_;
		D3DTEXTUREFILTERTYPE filterMag_;
		D3DTEXTUREFILTERTYPE filterMip_;
		bool bVertexShaderMode_;
		bool bCoordinate2D_;

		DirectionalLightingState lightParameter_;

		D3DXVECTOR3 position_;
		D3DXVECTOR3 angle_;
		D3DXVECTOR3 scale_;
		D3DCOLOR color_;
		
		shared_ptr<Shader> shader_;

		shared_ptr<DxMeshData> data_;
		shared_ptr<DxMeshData> _GetFromManager(std::wstring name);
		void _AddManager(std::wstring name, shared_ptr<DxMeshData> data);
	public:
		DxMesh();
		virtual ~DxMesh();
		virtual void Release();
		bool CreateFromFile(std::wstring path);
		virtual bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader) = 0;
		virtual bool CreateFromFileInLoadThread(std::wstring path, int type);
		virtual bool CreateFromFileInLoadThread(std::wstring path) = 0;
		virtual std::wstring GetPath() = 0;

		virtual void Render() = 0;
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) = 0;
		virtual inline void Render(std::wstring nameAnime, int time) { Render(); }
		virtual inline void Render(std::wstring nameAnime, int time,
			D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) { Render(angX, angY, angZ); }

		void SetPosition(D3DXVECTOR3 pos) { position_ = pos; }
		void SetPosition(float x, float y, float z) { position_.x = x; position_.y = y; position_.z = z; }
		void SetX(float x) { position_.x = x; }
		void SetY(float y) { position_.y = y; }
		void SetZ(float z) { position_.z = z; }
		void SetAngle(D3DXVECTOR3 angle) { angle_ = angle; }
		void SetAngleXYZ(float angx = 0.0f, float angy = 0.0f, float angz = 0.0f) { angle_.x = angx; angle_.y = angy; angle_.z = angz; }
		void SetScale(D3DXVECTOR3 scale) { scale_ = scale; }
		void SetScaleXYZ(float sx = 1.0f, float sy = 1.0f, float sz = 1.0f) { scale_.x = sx; scale_.y = sy; scale_.z = sz; }

		void SetColor(D3DCOLOR color) { color_ = color; }
		inline void SetColorRGB(D3DCOLOR color) {
			color_ = (color_ & 0xff000000) | (color & 0x00ffffff);
		}
		inline void SetAlpha(int alpha) {
			color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
		}

		void SetFilteringMin(D3DTEXTUREFILTERTYPE filter) { filterMin_ = filter; }
		void SetFilteringMag(D3DTEXTUREFILTERTYPE filter) { filterMag_ = filter; }
		void SetFilteringMip(D3DTEXTUREFILTERTYPE filter) { filterMip_ = filter; }
		void SetVertexShaderRendering(bool b) { bVertexShaderMode_ = b; }

		DirectionalLightingState* GetLighting() { return &lightParameter_; }

		bool IsCoordinate2D() { return bCoordinate2D_; }
		void SetCoordinate2D(bool b) { bCoordinate2D_ = b; }

		gstd::ref_count_ptr<RenderBlocks> CreateRenderBlocks() { return NULL; }
		virtual D3DXMATRIX GetAnimationMatrix(std::wstring nameAnime, double time, std::wstring nameBone) { 
			D3DXMATRIX mat; 
			D3DXMatrixIdentity(&mat); 
			return mat; 
		}
		shared_ptr<Shader> GetShader() { return shader_; }
		void SetShader(shared_ptr<Shader> shader) { shader_ = shader; }
	};

	/**********************************************************
	//DxMeshManager
	**********************************************************/
	class DxMeshInfoPanel;
	class DxMeshManager : public gstd::FileManager::LoadThreadListener {
		friend DxMeshData;
		friend DxMesh;
		friend DxMeshInfoPanel;
		static DxMeshManager* thisBase_;
	protected:
		gstd::CriticalSection lock_;
		std::map<std::wstring, shared_ptr<DxMesh>> mapMesh_;
		std::map<std::wstring, shared_ptr<DxMeshData>> mapMeshData_;
		gstd::ref_count_ptr<DxMeshInfoPanel> panelInfo_;

		void _AddMeshData(std::wstring name, shared_ptr<DxMeshData> data);
		shared_ptr<DxMeshData> _GetMeshData(std::wstring name);
		void _ReleaseMeshData(std::wstring name);
	public:
		DxMeshManager();
		virtual ~DxMeshManager();
		static DxMeshManager* GetBase() { return thisBase_; }
		bool Initialize();
		gstd::CriticalSection& GetLock() { return lock_; }

		virtual void Clear();
		virtual void Add(std::wstring name, shared_ptr<DxMesh> mesh);//参照を保持します
		virtual void Release(std::wstring name);//保持している参照を解放します
		virtual bool IsDataExists(std::wstring name);

		shared_ptr<DxMesh> CreateFromFileInLoadThread(std::wstring path, int type);
		virtual void CallFromLoadThread(shared_ptr<gstd::FileManager::LoadThreadEvent> event);

		void SetInfoPanel(gstd::ref_count_ptr<DxMeshInfoPanel> panel) { panelInfo_ = panel; }
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

#endif
