#include "source/GcLib/pch.h"

#include "DnhReplay.hpp"
#include "DnhGcLibImpl.hpp"
#include "StgCommonData.hpp"

#include "../../GcLib/gstd/CompressorStream.hpp"

//*******************************************************************
//ReplayInformation
//*******************************************************************
ReplayInformation::ReplayInformation() {
	userData_ = make_unique<ScriptCommonDataArea>();
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
void ReplayInformation::SetUserData(const std::string& key, gstd::value val) {
	userData_->SetValue(key, val);
}
gstd::value ReplayInformation::GetUserData(const std::string& key) {
	if (auto v = userData_->GetValueRef(key))
		return *v;
	return {};
}
bool ReplayInformation::IsUserDataExists(const std::string& key) {
	return userData_->GetValue(key).has_value();
}

struct HeaderReplay {
	char magic[8];
	uint32_t version;
};

bool ReplayInformation::SaveToFile(const std::wstring& scriptPath, int index) {
	std::wstring dir = EPathProperty::GetReplaySaveDirectory(scriptPath);
	std::wstring scriptName = PathProperty::GetFileNameWithoutExtension(scriptPath);
	std::wstring path = dir + scriptName + StringUtility::Format(L"_replay%02d.dat", index);

	File::CreateFileDirectory(dir);

	RecordBuffer rec;

	rec.SetRecordAsStringW("playerScriptID", playerScriptID_);
	rec.SetRecordAsStringW("playerScriptFileName", playerScriptFileName_);
	rec.SetRecordAsStringW("playerScriptReplayName", playerScriptReplayName_);

	rec.SetRecordAsStringW("comment", comment_);
	rec.SetRecordAsStringW("userName", userName_);
	rec.SetRecord("totalScore", totalScore_);
	rec.SetRecordAsDouble("fpsAverage", fpsAverage_);
	rec.SetRecord("date", date_);

	RecordBuffer recUserData;
	userData_->WriteRecord(recUserData);
	rec.SetRecordAsRecordBuffer("userData", recUserData);

	std::vector<int> listStage = GetStageIndexList();
	rec.SetRecord<uint32_t>("stageCount", listStage.size());
	rec.SetRecord("stageIndexList", &listStage[0], sizeof(int) * listStage.size());
	for (const int iStage : listStage) {
		std::string key = StringUtility::Format("stage%d", iStage);

		auto& data = mapStageData_[iStage];

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
			HeaderReplay header = {
				"DNHRPY\0",
				DATA_VERSION_REPLAY,
			};
			replayFile.write((char*)&header, sizeof(HeaderReplay));

			if (!CompressorStream::Deflate(replayBase, replayFile, replayBase.GetSize())) {
				Logger::WriteError("ReplayInformation::SaveToFile: Failed to compress replay file");
			}
		}
		else {
			Logger::WriteError("ReplayInformation::SaveToFile: Failed to create replay file.");
		}
	}

	return true;
}
ref_count_ptr<ReplayInformation> ReplayInformation::CreateFromFile(std::wstring scriptPath, std::wstring fileName) {
	std::wstring dir = EPathProperty::GetReplaySaveDirectory(scriptPath);
	//	std::string scriptName = PathProperty::GetFileNameWithoutExtension(scriptPath);
	//	std::string path = dir + scriptName + StringUtility::Format("_replay%02d.dat", index);
	std::wstring path = dir + fileName;

	return CreateFromFile(path);
}
ref_count_ptr<ReplayInformation> ReplayInformation::CreateFromFile(std::wstring path) {
	RecordBuffer rec;
	try {
		std::stringstream data;
		size_t dataSize = 0;

		{
			shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
			if (reader == nullptr || !reader->Open())
				throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path, false));

			std::string source = reader->ReadAllString();
			data.str(source);

			dataSize = source.size();
		}
		data.seekg(0);

		//Let's be honest, there's no bloody way a replay can be that small.
		if (dataSize < sizeof(HeaderReplay) + 0x40)
			return nullptr;

		{
			HeaderReplay header{};
			data.read((char*)&header, sizeof(HeaderReplay));

			if (memcmp(header.magic, "DNHRPY\0\0", sizeof(header.magic)) != 0) {
				throw wexception("File is not a ph3sx replay file");
			}
			if (header.version != DATA_VERSION_REPLAY) {
				throw wexception("Replay version not compatible with engine version");
			}
		}

		size_t sizeFull = 0U;
		ByteBuffer bufDecomp;

		if (auto oSize = CompressorStream::Inflate(data, bufDecomp, dataSize - sizeof(HeaderReplay))) {
			sizeFull = *oSize;
		}
		else return nullptr;

		bufDecomp.Seek(0);
		rec.Read(bufDecomp);
	}
	catch (gstd::wexception& e) {
		std::wstring str = StringUtility::Format(
			L"LoadReplay: Failed to load replay file \"%s\"\r\n    %s", 
			path.c_str(), e.what());
		Logger::WriteError(str);

		return nullptr;
	}

	ref_count_ptr<ReplayInformation> res(new ReplayInformation());

	res->path_ = path;
	res->playerScriptID_ = *rec.GetRecordAsStringW("playerScriptID");
	res->playerScriptFileName_ = *rec.GetRecordAsStringW("playerScriptFileName");
	res->playerScriptReplayName_ = *rec.GetRecordAsStringW("playerScriptReplayName");

	res->comment_ = *rec.GetRecordAsStringW("comment");
	res->userName_ = *rec.GetRecordAsStringW("userName");
	rec.GetRecord("totalScore", res->totalScore_);
	res->fpsAverage_ = *rec.GetRecordAsDouble("fpsAverage");
	rec.GetRecord("date", res->date_);

	res->userData_->Clear();
	if (auto data = rec.GetRecordAsRecordBuffer("userData")) {
		res->userData_->ReadRecord(*data);
	}

	uint32_t stageCount = *rec.GetRecordAs<uint32_t>("stageCount");

	std::vector<int> listStage;
	listStage.resize(stageCount);
	rec.GetRecord("stageIndexList", &listStage[0], sizeof(int) * stageCount);
	for (const int iStage : listStage) {
		std::string key = StringUtility::Format("stage%d", iStage);

		ref_count_ptr<StageData> data(new StageData());

		gstd::RecordBuffer recStage = *rec.GetRecordAsRecordBuffer(key);
		data->ReadRecord(recStage);

		res->mapStageData_[iStage] = MOVE(data);
	}

	return res;
}

//ReplayInformation::StageData

double ReplayInformation::StageData::GetFramePerSecondAverage() {
	double totalFps = 0;
	for (const FLOAT iFps : listFramePerSecond_)
		totalFps += iFps;

	if (listFramePerSecond_.size() > 0)
		totalFps /= listFramePerSecond_.size();
	return totalFps;
}
std::set<std::string> ReplayInformation::StageData::GetCommonDataAreaList() {
	std::set<std::string> res;
	for (auto& [name, _] : mapCommonData_) {
		res.insert(name);
	}
	return res;
}
unique_ptr<ScriptCommonDataArea> ReplayInformation::StageData::GetCommonData(const std::string& area) {
	auto commonData = make_unique<ScriptCommonDataArea>();

	auto itr = mapCommonData_.find(area);
	if (itr != mapCommonData_.end()) {
		commonData->ReadRecord(itr->second);
	}

	return MOVE(commonData);
}
void ReplayInformation::StageData::SetCommonData(
	const std::string& area, ScriptCommonDataArea* commonData)
{
	RecordBuffer record;
	if (commonData)
		commonData->WriteRecord(record);
	mapCommonData_[area] = MOVE(record);
}

void ReplayInformation::StageData::ReadRecord(gstd::RecordBuffer& record) {
	mainScriptID_ = *record.GetRecordAsStringW("mainScriptID");
	mainScriptName_ = *record.GetRecordAsStringW("mainScriptName");
	mainScriptRelativePath_ = *record.GetRecordAsStringW("mainScriptRelativePath");

	record.GetRecord<int64_t>("scoreStart", scoreStart_);
	record.GetRecord<int64_t>("scoreLast", scoreLast_);
	record.GetRecord<int64_t>("graze", graze_);
	record.GetRecord<int64_t>("point", point_);
	frameEnd_ = *record.GetRecordAs<uint32_t>("frameEnd");
	record.GetRecord<uint32_t>("randSeed", randSeed_);
	recordKey_ = *record.GetRecordAsRecordBuffer("recordKey");

	//FPS list
	size_t countFramePerSecond = *record.GetRecordAs<uint32_t>("countFramePerSecond");
	listFramePerSecond_.resize(countFramePerSecond);
	record.GetRecord("listFramePerSecond", &listFramePerSecond_[0], sizeof(FLOAT) * listFramePerSecond_.size());

	//Common data
	{
		gstd::RecordBuffer recComMap = *record.GetRecordAsRecordBuffer("mapCommonData");
		for (auto& iCommonData : recComMap.GetKeyList()) {
			auto record = *recComMap.GetRecordAsRecordBuffer(iCommonData);
			mapCommonData_[iCommonData] = MOVE(record);
		}
	}

	//Player information
	playerScriptID_ = *record.GetRecordAsStringW("playerScriptID");
	playerScriptFileName_ = *record.GetRecordAsStringW("playerScriptFileName");
	playerScriptReplayName_ = *record.GetRecordAsStringW("playerScriptReplayName");
	playerLife_ = *record.GetRecordAsDouble("playerLife");
	playerBombCount_ = *record.GetRecordAsDouble("playerBombCount");
	playerPower_ = *record.GetRecordAsDouble("playerPower");
	playerRebirthFrame_ = *record.GetRecordAsInteger("playerRebirthFrame");
}
void ReplayInformation::StageData::WriteRecord(gstd::RecordBuffer& record) {
	record.SetRecordAsStringW("mainScriptID", mainScriptID_);
	record.SetRecordAsStringW("mainScriptName", mainScriptName_);
	record.SetRecordAsStringW("mainScriptRelativePath", mainScriptRelativePath_);

	record.SetRecord<int64_t>("scoreStart", scoreStart_);
	record.SetRecord<int64_t>("scoreLast", scoreLast_);
	record.SetRecord<int64_t>("graze", graze_);
	record.SetRecord<int64_t>("point", point_);
	record.SetRecord<uint32_t>("frameEnd", frameEnd_);
	record.SetRecord<uint32_t>("randSeed", randSeed_);
	record.SetRecordAsRecordBuffer("recordKey", recordKey_);

	//FPS list
	size_t countFramePerSecond = listFramePerSecond_.size();
	record.SetRecord<uint32_t>("countFramePerSecond", countFramePerSecond);
	record.SetRecord("listFramePerSecond", &listFramePerSecond_[0], sizeof(FLOAT) * listFramePerSecond_.size());

	//Common data
	{
		gstd::RecordBuffer recComMap;

		for (auto& [name, areaRec] : mapCommonData_) {
			recComMap.SetRecordAsRecordBuffer(name, areaRec);
		}

		record.SetRecordAsRecordBuffer("mapCommonData", recComMap);
	}

	//Player information
	record.SetRecordAsStringW("playerScriptID", playerScriptID_);
	record.SetRecordAsStringW("playerScriptFileName", playerScriptFileName_);
	record.SetRecordAsStringW("playerScriptReplayName", playerScriptReplayName_);
	record.SetRecordAsDouble("playerLife", playerLife_);
	record.SetRecordAsDouble("playerBombCount", playerBombCount_);
	record.SetRecordAsDouble("playerPower", playerPower_);
	record.SetRecordAsInteger("playerRebirthFrame", playerRebirthFrame_);
}

//*******************************************************************
//ReplayInformationManager
//*******************************************************************
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
		const std::wstring& path = *itr;
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
		mapInfo_[key] = MOVE(info);
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
