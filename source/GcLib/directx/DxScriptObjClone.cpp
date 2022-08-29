#include "source/GcLib/pch.h"

#include "DxScript.hpp"
#include "DxObject.hpp"

#include "../../TouhouDanmakufu/Common/StgPlayer.hpp"
#include "../../TouhouDanmakufu/Common/StgEnemy.hpp"
#include "../../TouhouDanmakufu/Common/StgShot.hpp"
#include "../../TouhouDanmakufu/Common/StgItem.hpp"
#include "../../TouhouDanmakufu/Common/StgSystem.hpp"
#include "../../TouhouDanmakufu/Common/StgStageScript.hpp"

value DxScript::Func_Obj_Clone(script_machine* machine, int argc, const value* argv) {
	DxScript* script = (DxScript*)machine->data;
	script->CheckRunInMainThread();

	int idSrc = argv[0].as_int();
	DxScriptObjectBase* objSrc = script->GetObjectPointerAs<DxScriptObjectBase>(idSrc);

	int idDst = ID_INVALID;
	if (objSrc) {
		ref_unsync_ptr<DxScriptObjectBase> obj;

		if (dynamic_cast<StgObjectBase*>(objSrc) == nullptr) {
			//Not an STG-exclusive object

#define DEF_CASE(_type, _class) case _type: obj = new _class(); break;
			switch (objSrc->typeObject_) {
				DEF_CASE(TypeObject::Base, DxScriptObjectBase);

				DEF_CASE(TypeObject::Primitive2D, DxScriptPrimitiveObject2D);
				DEF_CASE(TypeObject::Sprite2D, DxScriptSpriteObject2D);
				DEF_CASE(TypeObject::SpriteList2D, DxScriptSpriteListObject2D);
				DEF_CASE(TypeObject::Primitive3D, DxScriptPrimitiveObject3D);
				DEF_CASE(TypeObject::Sprite3D, DxScriptSpriteObject3D);
				DEF_CASE(TypeObject::Trajectory3D, DxScriptTrajectoryObject3D);

				DEF_CASE(TypeObject::ParticleList2D, DxScriptParticleListObject2D);
				DEF_CASE(TypeObject::ParticleList3D, DxScriptParticleListObject3D);

				DEF_CASE(TypeObject::Shader, DxScriptShaderObject);

				DEF_CASE(TypeObject::Mesh, DxScriptMeshObject);
				DEF_CASE(TypeObject::Text, DxScriptTextObject);
				DEF_CASE(TypeObject::Sound, DxSoundObject);

				DEF_CASE(TypeObject::FileText, DxTextFileObject);
				DEF_CASE(TypeObject::FileBinary, DxBinaryFileObject);
			}
#undef DEF_CASE
		}
		else if (StgStageScript* scriptStg = dynamic_cast<StgStageScript*>(script)) {
			//STG-exclusive objects

			StgStageController* stageController = scriptStg->GetStageController();

#define DEF_CASE(_type, _class) case _type: obj = new _class(stageController); break;
			switch (objSrc->typeObject_) {
				DEF_CASE(TypeObject::Player, StgPlayerObject);
				DEF_CASE(TypeObject::SpellManage, StgPlayerSpellManageObject);
				DEF_CASE(TypeObject::Spell, StgPlayerSpellObject);

				DEF_CASE(TypeObject::Enemy, StgEnemyObject);
				DEF_CASE(TypeObject::EnemyBoss, StgEnemyBossObject);
				DEF_CASE(TypeObject::EnemyBossScene, StgEnemyBossSceneObject);

				DEF_CASE(TypeObject::Shot, StgNormalShotObject);
				DEF_CASE(TypeObject::LooseLaser, StgLooseLaserObject);
				DEF_CASE(TypeObject::StraightLaser, StgStraightLaserObject);
				DEF_CASE(TypeObject::CurveLaser, StgCurveLaserObject);
				DEF_CASE(TypeObject::ShotPattern, StgShotPatternGeneratorObject);
			
			case TypeObject::Item:
			{
				if (StgItemObject_User* pItem = dynamic_cast<StgItemObject_User*>(objSrc)) {
					obj = new StgItemObject_User(stageController);
				}
				break;
			}
			}
#undef DEF_CASE
		}

		if (obj) {
			obj->Initialize();
			obj->manager_ = script->objManager_.get();
			idDst = script->AddObject(obj);
		}
		if (idDst != ID_INVALID)
			obj->Clone(objSrc);
	}
	return script->CreateIntValue(idDst);
}