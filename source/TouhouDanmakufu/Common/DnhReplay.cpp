#include "source/GcLib/pch.h"

#include "DnhReplay.hpp"
#include "DnhGcLibImpl.hpp"

/**********************************************************
//ReplayInformation
**********************************************************/
const std::string ReplayInformation::REC_HEADER = "DNHRPY";

ReplayInformation::ReplayInformation() {
	userData_ = std::make_shared<ScriptCommonData>();
}
ReplayInformation::~ReplayInformation() {}
std::vector<int> ReplayInformation::GetStageIndexList() {
	std::vector<int> res;

	for (auto itr = mapStageData_.begin(); itr != mapStageData_.end(); itr++) {
		int stage = itr->first;
		res.push_back(stage);
	}

	std::sort(res.begin(), res.end());
	return res;
}
std::wstring ReplayInformation::GetDateAsString() {
	std::wstring res = StringUtility::Format(
		L"%04d/%02d/%02d %02d:%02d",
		date_.wYear, date_.wMonth, date_.wDay,
		date_.wHour, date_.wMinute
	);

	return res;
}
void ReplayInformation::SetUserData(std::string key, gstd::value val) {
	userData_->SetValue(key, val);
}
gstd::value ReplayInformation::GetUserData(std::string key) {
	gstd::value res = userData_->GetValue(key);
	return res;
}
bool ReplayInformation::IsUserDataExists(std::string key) {
	return userData_->IsExists(key).first;
}

bool ReplayInformation::SaveToFile(std::wstring scriptPath, int index) {
	std::wstring dir = EPathProperty::GetReplaySaveDirectory(scriptPath);
	std::wstring scriptName = PathProperty::GetFileNameWithoutExtension(scriptPath);
	std::wstring path = dir + scriptName + StringUtility::Format(L"_replay%02d.dat", index);

	//フォルダ作成
	File::CreateFileDirectory(dir);

	RecordBuffer rec;
	rec.SetRecordAsInteger("version", DNH_VERSION_NUM);
	rec.SetRecordAsStringW("playerScriptID", playerScriptID_);
	rec.SetRecordAsStringW("playerScriptFileName", playerScriptFileName_);
	rec.SetRecordAsStringW("playerScriptReplayName", playerScriptReplayName_);

	rec.SetRecordAsStringW("comment", comment_);
	rec.SetRecordAsStringW("userName", userName_);
	rec.SetRecord("totalScore", totalScore_);
	rec.SetRecordAsDouble("fpsAverage", fpsAvarage_);
	rec.SetRecord("date", date_);

	RecordBuffer recUserData;
	userData_->WriteRecord(recUserData);
	rec.SetRecordAsRecordBuffer("userData", recUserData);

	std::vector<int> listStage = GetStageIndexList();
	rec.SetRecordAsInteger("stageCount", listStage.size());
	{
		std::string key = "stageIndexList";
		rec.SetRecord(key, &listStage[0], sizeof(int) * listStage.size());
	}
	for (size_t iStage = 0; iStage < listStage.size(); iStage++) {
		int stage = listStage[iStage];
		std::string key = StringUtility::Format("stage%d", stage);

		ref_count_ptr<StageData> data = mapStageData_[stage];
		gstd::RecordBuffer recStage;
		data->WriteRecord(recStage);
		rec.SetRecordAsRecordBuffer(key, recStage);
	}

	{
		ByteBuffer replayBase;
		rec.Write(replayBase);
		replayBase.Seek(0);

		std::ofstream replayFile;
		replayFile.open(path, std::ios::binary | std::ios::trunc);
		if (replayFile.is_open()) {
			replayFile.write(REC_HEADER.c_str(), 0x6);
			Compressor::DeflateStream(replayBase, replayFile, replayBase.GetSize(), nullptr);
		}
		else {
			Logger::WriteTop("ReplayInformation::SaveToFile: Failed to create replay file.");
		}

		replayFile.close();
	}

	return true;
}
ref_count_ptr<ReplayInformation> ReplayInformation::CreateFromFile(std::wstring scriptPath, std::wstring fileName) {
	std::wstring dir = EPathProperty::GetReplaySaveDirectory(scriptPath);
	//	std::string scriptName = PathProperty::GetFileNameWithoutExtension(scriptPath);
	//	std::string path = dir + scriptName + StringUtility::Format("_replay%02d.dat", index);
	std::wstring path = dir + fileName;

	ref_count_ptr<ReplayInformation> res = CreateFromFile(path);
	return res;
}
ref_count_ptr<ReplayInformation> ReplayInformation::CreateFromFile(std::wstring path) {
	RecordBuffer rec;
	{
		std::ifstream file;
		file.open(path, std::ios::binary);
		if (!file.is_open()) return nullptr;

		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		//Let's be honest, there's no bloody way a replay can be that small.
		if (size < 0x6 + 0x40) return nullptr;
		char header[0x6];
		file.read(header, 0x6);
		if (memcmp(header, REC_HEADER.c_str(), 0x6) != 0) return nullptr;

		size_t sizeFull = 0U;
		ByteBuffer bufDecomp;
		if (!Compressor::InflateStream(file, bufDecomp, size - 6U, &sizeFull)) return nullptr;

		bufDecomp.Seek(0);
		rec.Read(bufDecomp);

		file.close();
	}

	int version = rec.GetRecordAsInteger("version");
	if (version != DNH_VERSION_NUM) return nullptr;

	ref_count_ptr<ReplayInformation> res = new ReplayInformation();
	res->path_ = path;
	res->playerScriptID_ = rec.GetRecordAsStringW("playerScriptID");
	res->playerScriptFileName_ = rec.GetRecordAsStringW("playerScriptFileName");
	res->playerScriptReplayName_ = rec.GetRecordAsStringW("playerScriptReplayName");

	res->comment_ = rec.GetRecordAsStringW("comment");
	res->userName_ = rec.GetRecordAsStringW("userName");
	rec.GetRecord("totalScore", res->totalScore_);
	res->fpsAvarage_ = rec.GetRecordAsDouble("fpsAverage");
	rec.GetRecord("date", res->date_);

	res->userData_->Clear();
	if (rec.IsExists("userData")) {
		RecordBuffer recUserData;
		rec.GetRecordAsRecordBuffer("userData", recUserData);
		res->userData_->ReadRecord(recUserData);
	}

	int stageCount = rec.GetRecordAsInteger("stageCount");
	std::vector<int> listStage;
	listStage.resize(stageCount);
	{
		std::string key = "stageIndexList";
		rec.GetRecord(key, &listStage[0], sizeof(int) * stageCount);
	}
	for (size_t iStage = 0; iStage < listStage.size(); iStage++) {
		int stage = listStage[iStage];
		std::string key = StringUtility::Format("stage%d", stage);
		ref_count_ptr<StageData> data = new StageData();
		gstd::RecordBuffer recStage;
		rec.GetRecordAsRecordBuffer(key, recStage);
		data->ReadRecord(recStage);
		res->mapStageData_[stage] = data;
	}

	return res;
}

//ReplayInformation::StageData
double ReplayInformation::StageData::GetFramePerSecondAvarage() {
	double res = 0;
	for (size_t iFrame = 0; iFrame < listFramePerSecond_.size(); iFrame++) {
		res += listFramePerSecond_[iFrame];
	}

	if (listFramePerSecond_.size() > 0)
		res /= listFramePerSecond_.size();
	return res;
}
std::set<std::string> ReplayInformation::StageData::GetCommonDataAreaList() {
	std::set<std::string> res;
	for (auto itrCommonData = mapCommonData_.begin(); itrCommonData != mapCommonData_.end(); itrCommonData++) {
		std::string key = itrCommonData->first;
		res.insert(key);
	}
	return res;
}
shared_ptr<ScriptCommonData> ReplayInformation::StageData::GetCommonData(std::string area) {
	shared_ptr<ScriptCommonData> res(new ScriptCommonData());
	auto itr = mapCommonData_.find(area);
	if (itr != mapCommonData_.end()) {
		res->ReadRecord(*itr->second);
	}
	return res;
}
void ReplayInformation::StageData::SetCommonData(std::string area, shared_ptr<ScriptCommonData> commonData) {
	ref_count_ptr<RecordBuffer> record = new RecordBuffer();
	if (commonData)
		commonData->WriteRecord(*record);
	mapCommonData_[area] = record;
}

void ReplayInformation::StageData::ReadRecord(gstd::RecordBuffer& record) {
	mainScriptID_ = record.GetRecordAsStringW("mainScriptID");
	mainScriptName_ = record.GetRecordAsStringW("mainScriptName");
	mainScriptRelativePath_ = record.GetRecordAsStringW("mainScriptRelativePath");
	record.GetRecord<int64_t>("scoreStart", scoreStart_);
	record.GetRecord<int64_t>("scoreLast", scoreLast_);
	record.GetRecord<int64_t>("graze", graze_);
	record.GetRecord<int64_t>("point", point_);
	frameEnd_ = record.GetRecordAsInteger("frameEnd");
	record.GetRecord<uint32_t>("randSeed", randSeed_);
	record.GetRecordAsRecordBuffer("recordKey", *recordKey_);

	int countFramePerSecond = record.GetRecordAsInteger("countFramePerSecond");
	listFramePerSecond_.resize(countFramePerSecond);
	{
		std::string key = "listFramePerSecond";
		record.GetRecord(key, &listFramePerSecond_[0], sizeof(FLOAT) * listFramePerSecond_.size());
	}

	//共通データ
	gstd::RecordBuffer recComMap;
	record.GetRecordAsRecordBuffer("mapCommonData", recComMap);
	std::vector<std::string> listKeyCommonData = recComMap.GetKeyList();
	for (size_t iCommonData = 0; iCommonData < listKeyCommonData.size(); iCommonData++) {
		std::string key = listKeyCommonData[iCommonData];
		ref_count_ptr<RecordBuffer> recComData = new RecordBuffer();
		recComMap.GetRecordAsRecordBuffer(key, *recComData);
		mapCommonData_[key] = recComData;
	}

	//自機情報
	playerScriptID_ = record.GetRecordAsStringW("playerScriptID");
	playerScriptFileName_ = record.GetRecordAsStringW("playerScriptFileName");
	playerScriptReplayName_ = record.GetRecordAsStringW("playerScriptReplayName");
	playerLife_ = record.GetRecordAsDouble("playerLife");
	playerBombCount_ = record.GetRecordAsDouble("playerBombCount");
	playerPower_ = record.GetRecordAsDouble("playerPower");
	playerRebirthFrame_ = record.GetRecordAsInteger("playerRebirthFrame");
}
void ReplayInformation::StageData::WriteRecord(gstd::RecordBuffer& record) {
	record.SetRecordAsStringW("mainScriptID", mainScriptID_);
	record.SetRecordAsStringW("mainScriptName", mainScriptName_);
	record.SetRecordAsStringW("mainScriptRelativePath", mainScriptRelativePath_);
	record.SetRecord<int64_t>("scoreStart", scoreStart_);
	record.SetRecord<int64_t>("scoreLast", scoreLast_);
	record.SetRecord<int64_t>("graze", graze_);
	record.SetRecord<int64_t>("point", point_);
	record.SetRecordAsInteger("frameEnd", frameEnd_);
	record.SetRecord<uint32_t>("randSeed", randSeed_);
	record.SetRecordAsRecordBuffer("recordKey", *recordKey_);

	size_t countFramePerSecond = listFramePerSecond_.size();
	record.SetRecordAsInteger("countFramePerSecond", countFramePerSecond);
	{
		std::string key = "listFramePerSecond";
		record.SetRecord(key, &listFramePerSecond_[0], sizeof(FLOAT) * listFramePerSecond_.size());
	}

	//共通データ
	gstd::RecordBuffer recComMap;
	std::map<std::string, ref_count_ptr<RecordBuffer>>::iterator itrCommonData;
	for (itrCommonData = mapCommonData_.begin(); itrCommonData != mapCommonData_.end(); itrCommonData++) {
		std::string key = itrCommonData->first;
		ref_count_ptr<RecordBuffer> recComData = itrCommonData->second;
		recComMap.SetRecordAsRecordBuffer(key, *recComData);
	}
	record.SetRecordAsRecordBuffer("mapCommonData", recComMap);

	//自機情報
	record.SetRecordAsStringW("playerScriptID", playerScriptID_);
	record.SetRecordAsStringW("playerScriptFileName", playerScriptFileName_);
	record.SetRecordAsStringW("playerScriptReplayName", playerScriptReplayName_);
	record.SetRecordAsDouble("playerLife", playerLife_);
	record.SetRecordAsDouble("playerBombCount", playerBombCount_);
	record.SetRecordAsDouble("playerPower", playerPower_);
	record.SetRecordAsInteger("playerRebirthFrame", playerRebirthFrame_);
}

/**********************************************************
//ReplayInformationManager
**********************************************************/
ReplayInformationManager::ReplayInformationManager() {

}
ReplayInformationManager::~ReplayInformationManager() {}
void ReplayInformationManager::UpdateInformationList(std::wstring pathScript) {
	mapInfo_.clear();

	std::wstring scriptName = PathProperty::GetFileNameWithoutExtension(pathScript);
	std::wstring fileNameHead = scriptName + L"_replay";
	std::wstring dir = EPathProperty::GetReplaySaveDirectory(pathScript);
	std::vector<std::wstring> listPath = File::GetFilePathList(dir);

	int indexFree = ReplayInformation::INDEX_USER;
	std::vector<std::wstring>::iterator itr;
	for (itr = listPath.begin(); itr != listPath.end(); itr++) {
		std::wstring path = *itr;
		std::wstring fileName = PathProperty::GetFileName(path);

		if (fileName.find(fileNameHead) == std::wstring::npos) continue;

		ref_count_ptr<ReplayInformation> info = ReplayInformation::CreateFromFile(pathScript, fileName);
		if (info == nullptr) continue;

		std::wstring strKey = fileName.substr(fileNameHead.size());
		strKey = PathProperty::GetFileNameWithoutExtension(strKey);
		int index = StringUtility::ToInteger(strKey);
		if (index > 0) {
			strKey = StringUtility::Format(L"%d", index);
		}
		else {
			index = indexFree;
			indexFree++;
			strKey = StringUtility::Format(L"%d", index);
		}

		int key = StringUtility::ToInteger(strKey);
		mapInfo_[key] = info;
	}

}
std::vector<int> ReplayInformationManager::GetIndexList() {
	std::vector<int> res;
	for (auto itr = mapInfo_.begin(); itr != mapInfo_.end(); ++itr) {
		res.push_back(itr->first);
	}
	return res;
}
ref_count_ptr<ReplayInformation> ReplayInformationManager::GetInformation(int index) {
	auto itr = mapInfo_.find(index);
	if (itr == mapInfo_.end()) return nullptr;
	return itr->second;
}
