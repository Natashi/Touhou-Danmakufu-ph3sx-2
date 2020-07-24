#ifndef __DIRECTX_ElfreinaMeshData__
#define __DIRECTX_ElfreinaMeshData__

#include "../pch.h"
#include "RenderObject.hpp"

namespace directx {
	class RenderObjectElfreinaBlock;
	/**********************************************************
	//ElfreinaMesh
	**********************************************************/
	class ElfreinaMesh;
	class ElfreinaMeshData : public DxMeshData {
		friend ElfreinaMesh;
	public:
		class Bone;
		class Material;
		class Mesh;
		class AnimationData;
		class BoneAnimationPart;
	protected:
		std::wstring path_;
		std::vector<gstd::ref_count_ptr<Mesh>> mesh_;
		std::vector<gstd::ref_count_ptr<Bone>> bone_;
		std::vector<gstd::ref_count_ptr<Material>> material_;
		std::map<std::wstring, gstd::ref_count_ptr<AnimationData>> anime_;

		std::map<std::wstring, int> mapBoneNameIndex_;

		void _ReadMeshContainer(gstd::Scanner& scanner);
		void _ReadMaterials(gstd::Scanner& scanner, Material& material);
		void _ReadMesh(gstd::Scanner& scanner, Mesh& mesh);

		void _ReadHierarchyList(gstd::Scanner& scanner);
		int _ReadNode(gstd::Scanner& scanner, int parent);

		void _ReadAnimationList(gstd::Scanner& scanner);
		gstd::ref_count_ptr<AnimationData> _ReadAnimationData(gstd::Scanner& scanner);
		void _ReadBoneAnimation(gstd::Scanner& scanner, AnimationData& anime);
		void _ReadBoneAnimationPart(gstd::Scanner& scanner, AnimationData& anime);
	public:
		ElfreinaMeshData();
		~ElfreinaMeshData();
		bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader);
		std::vector<gstd::ref_count_ptr<Bone>>& GetBones() { return bone_; }
	};

	class ElfreinaMeshData::Bone {
		friend ElfreinaMesh;
		friend ElfreinaMeshData;
	public:
		enum {
			NO_PARENT = -1,
		};
	protected:
		std::wstring name_;
		D3DXMATRIX matOffset_;
		D3DXMATRIX matInitPosture_;

		int indexParent_;
		std::vector<int> indexChild_;

	public:
		Bone() {};
		virtual ~Bone() {};
		int GetParentIndex() { return indexParent_; }
		std::vector<int>& GetChildIndex() { return indexChild_; }
		D3DXMATRIX& GetOffsetMatrix() { return matOffset_; }
		D3DXMATRIX& GetInitPostureMatrix() { return matInitPosture_; }
	};
	class ElfreinaMeshData::Material {
		friend ElfreinaMeshData;
		friend ElfreinaMeshData::Mesh;
		friend RenderObjectElfreinaBlock;
	protected:
		std::wstring name_;
		D3DMATERIAL9 mat_;
		shared_ptr<Texture> texture_;
	public:
		Material() {};
		virtual ~Material() {};

	};
	class ElfreinaMeshData::Mesh : public RenderObjectB4NX {
		//頂点ボーンデータ読み込みに使う一時データ
		struct BoneWeightData {
			int index;
			float weight;
		};
		friend ElfreinaMeshData;
		friend RenderObjectElfreinaBlock;
	protected:
		std::wstring name_;
		gstd::ref_count_ptr<Material> material_;
		int indexWeightForCalucZValue_;
	public:
		Mesh();
		virtual ~Mesh();
		virtual void Render();
		gstd::ref_count_ptr<RenderBlock> CreateRenderBlock();
	};
	class ElfreinaMeshData::AnimationData {
		friend ElfreinaMesh;
		friend ElfreinaMeshData;
	protected:
		std::wstring name_;
		int timeTotal_;
		int framePerSecond_;
		bool bLoop_;
		std::vector<gstd::ref_count_ptr<ElfreinaMeshData::BoneAnimationPart>> animeBone_;

		void _CreateBoneAnimationMatrix(int time, ElfreinaMeshData* mesh, gstd::ref_count_ptr<Matrices> matrix, int indexOwn, D3DXMATRIX& matrixParentAnime);
		D3DXMATRIX _CalculateMatrix(double time, int index);
	public:
		AnimationData() {};
		virtual ~AnimationData() {};
		gstd::ref_count_ptr<Matrices> CreateBoneAnimationMatrix(double time, ElfreinaMeshData* mesh);
	};
	class ElfreinaMeshData::BoneAnimationPart {
		friend ElfreinaMeshData;
	protected:
		std::vector<float> keyTime_;
		std::vector<D3DXVECTOR3> keyTrans_;
		std::vector<D3DXQUATERNION> keyRotate_;
		std::vector<D3DXVECTOR3> keyScale_;
	public:
		BoneAnimationPart() {};
		virtual ~BoneAnimationPart() {};
		std::vector<float>& GetTimeKey() { return keyTime_; }
		std::vector<D3DXVECTOR3>& GetTransKey() { return keyTrans_; }
		std::vector<D3DXQUATERNION>& GetRotateKey() { return keyRotate_; }
		std::vector<D3DXVECTOR3>& GetScaleKey() { return keyScale_; }
	};

	class RenderObjectElfreinaBlock : public RenderObjectB2NXBlock {
	public:
		~RenderObjectElfreinaBlock();
		virtual bool IsTranslucent();
		virtual void CalculateZValue();
	};

	class ElfreinaMesh : public DxMesh {
	protected:
		double _CalcFrameToTime(double time, gstd::ref_count_ptr<ElfreinaMeshData::AnimationData> anime);
	public:
		ElfreinaMesh() {}
		virtual ~ElfreinaMesh() {}
		virtual bool CreateFromFileReader(gstd::ref_count_ptr<gstd::FileReader> reader);
		virtual bool CreateFromFileInLoadThread(std::wstring path);
		virtual std::wstring GetPath();
		virtual void Render();
		virtual void Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {};
		virtual void Render(std::wstring nameAnime, int time);
		virtual void Render(std::wstring nameAnime, int time, D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {};

		gstd::ref_count_ptr<RenderBlocks> CreateRenderBlocks();
		gstd::ref_count_ptr<RenderBlocks> CreateRenderBlocks(std::wstring nameAnime, double time);

		gstd::ref_count_ptr<Matrices> CreateAnimationMatrix(std::wstring nameAnime, double time);
		virtual D3DXMATRIX GetAnimationMatrix(std::wstring nameAnime, double time, std::wstring nameBone);
	};
}

#endif
