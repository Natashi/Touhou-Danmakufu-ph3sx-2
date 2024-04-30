#pragma once

#include "../../GcLib/pch.h"

#include "DnhCommon.hpp"

//*******************************************************************
//ReplayInformation
//*******************************************************************
class ScriptCommonDataArea;
class ReplayInformation {
public:
	enum {
		INDEX_ACTIVE = 0,
		INDEX_DIGIT_MIN = 1,
		INDEX_DIGIT_MAX = 99,
		INDEX_USER = 100,
	};

	class StageData;
private:
	std::wstring path_;
	std::wstring playerScriptID_;
	std::wstring playerScriptFileName_;
	std::wstring playerScriptReplayName_;

	std::wstring comment_;
	std::wstring userName_;
	int64_t totalScore_;
	double fpsAverage_;
	SYSTEMTIME date_;

	unique_ptr<ScriptCommonDataArea> userData_;
	std::map<int, ref_count_ptr<StageData>> mapStageData_;
public:
	ReplayInformation();
	virtual ~ReplayInformation();

	std::wstring& GetPath() { return path_; }
	std::wstring& GetPlayerScriptID() { return playerScriptID_; }
	void SetPlayerScriptID(const std::wstring& id) { playerScriptID_ = id; }
	std::wstring& GetPlayerScriptFileName() { return playerScriptFileName_; }
	void SetPlayerScriptFileName(const std::wstring& name) { playerScriptFileName_ = name; }
	std::wstring& GetPlayerScriptReplayName() { return playerScriptReplayName_; }
	void SetPlayerScriptReplayName(const std::wstring& name) { playerScriptReplayName_ = name; }

	std::wstring& GetComment() { return comment_; }
	void SetComment(const std::wstring& comment) { comment_ = comment; }
	std::wstring& GetUserName() { return userName_; }
	void SetUserName(const std::wstring& name) { userName_ = name; }
	int64_t GetTotalScore() { return totalScore_; }
	void SetTotalScore(int64_t score) { totalScore_ = score; }
	double GetAverageFps() { return fpsAverage_; }
	void SetAverageFps(double fps) { fpsAverage_ = fps; }
	SYSTEMTIME& GetDate() { return date_; }
	void SetDate(SYSTEMTIME& date) { date_ = date; }
	std::wstring GetDateAsString();

	void SetUserData(const std::string& key, gstd::value val);
	gstd::value GetUserData(const std::string& key);
	bool IsUserDataExists(const std::string& key);

	ref_count_ptr<StageData> GetStageData(int stage) { return mapStageData_[stage]; }
	void SetStageData(int stage, ref_count_ptr<StageData> data) { mapStageData_[stage] = data; }
	std::vector<int> GetStageIndexList();

	bool SaveToFile(const std::wstring& scriptPath, int index);
	static ref_count_ptr<ReplayInformation> CreateFromFile(std::wstring scriptPath, std::wstring fileName);
	static ref_count_ptr<ReplayInformation> CreateFromFile(std::wstring path);
};

class ReplayInformation::StageData {
private:
	std::wstring mainScriptID_;
	std::wstring mainScriptName_;
	std::wstring mainScriptRelativePath_;

	int64_t scoreStart_;
	int64_t scoreLast_;
	int64_t graze_;
	int64_t point_;
	DWORD frameEnd_;

	uint32_t randSeed_;
	std::vector<float> listFramePerSecond_;

	gstd::RecordBuffer recordKey_;
	std::map<std::string, gstd::RecordBuffer> mapCommonData_;

	std::wstring playerScriptID_;
	std::wstring playerScriptFileName_;
	std::wstring playerScriptReplayName_;

	double playerLife_;
	double playerBombCount_;
	double playerPower_;
	int playerRebirthFrame_;	//Current deathbomb frame
public:
	StageData() = default;
	virtual ~StageData() {}

	std::wstring& GetMainScriptID() { return mainScriptID_; }
	void SetMainScriptID(const std::wstring& id) { mainScriptID_ = id; }
	std::wstring& GetMainScriptName() { return mainScriptName_; }
	void SetMainScriptName(const std::wstring& name) { mainScriptName_ = name; }
	std::wstring& GetMainScriptRelativePath() { return mainScriptRelativePath_; }
	void SetMainScriptRelativePath(const std::wstring& path) { mainScriptRelativePath_ = path; }
	int64_t GetStartScore() { return scoreStart_; }
	void SetStartScore(int64_t score) { scoreStart_ = score; }
	int64_t GetLastScore() { return scoreLast_; }
	void SetLastScore(int64_t score) { scoreLast_ = score; }
	int64_t GetGraze() { return graze_; }
	void SetGraze(int64_t graze) { graze_ = graze; }
	int64_t GetPoint() { return point_; }
	void SetPoint(int64_t point) { point_ = point; }
	DWORD GetEndFrame() { return frameEnd_; }
	void SetEndFrame(DWORD frame) { frameEnd_ = frame; }
	uint32_t GetRandSeed() { return randSeed_; }
	void SetRandSeed(uint32_t seed) { randSeed_ = seed; }
	float GetFramePerSecond(int frame) { int index = frame / 60; int res = index < listFramePerSecond_.size() ? listFramePerSecond_[index] : 0; return res; }
	void AddFramePerSecond(float frame) { listFramePerSecond_.push_back(frame); }
	double GetFramePerSecondAverage();

	gstd::RecordBuffer& GetReplayKeyRecord() { return recordKey_; }
	void SetReplayKeyRecord(gstd::RecordBuffer&& rec) { recordKey_ = MOVE(rec); }
	std::set<std::string> GetCommonDataAreaList();
	unique_ptr<ScriptCommonDataArea> GetCommonData(const std::string& area);
	void SetCommonData(const std::string& area, ScriptCommonDataArea* commonData);
	void SetCommonData(const std::string& area, unique_ptr<ScriptCommonDataArea>&& commonData) {
		SetCommonData(area, commonData.get());
	}

	std::wstring& GetPlayerScriptID() { return playerScriptID_; }
	void SetPlayerScriptID(const std::wstring& id) { playerScriptID_ = id; }
	std::wstring& GetPlayerScriptFileName() { return playerScriptFileName_; }
	void SetPlayerScriptFileName(const std::wstring& name) { playerScriptFileName_ = name; }
	std::wstring& GetPlayerScriptReplayName() { return playerScriptReplayName_; }
	void SetPlayerScriptReplayName(const std::wstring& name) { playerScriptReplayName_ = name; }
	double GetPlayerLife() { return playerLife_; }
	void SetPlayerLife(double life) { playerLife_ = life; }
	double GetPlayerBombCount() { return playerBombCount_; }
	void SetPlayerBombCount(double bomb) { playerBombCount_ = bomb; }
	double GetPlayerPower() { return playerPower_; }
	void SetPlayerPower(double power) { playerPower_ = power; }
	int GetPlayerRebirthFrame() { return playerRebirthFrame_; }
	void SetPlayerRebirthFrame(int frame) { playerRebirthFrame_ = frame; }

	void ReadRecord(gstd::RecordBuffer& record);
	void WriteRecord(gstd::RecordBuffer& record);
};

//*******************************************************************
//ReplayInformationManager
//*******************************************************************
class ReplayInformationManager {
protected:
	std::map<int, ref_count_ptr<ReplayInformation>> mapInfo_;
public:
	ReplayInformationManager();
	virtual ~ReplayInformationManager();

	void UpdateInformationList(std::wstring pathScript);
	std::vector<int> GetIndexList();
	ref_count_ptr<ReplayInformation> GetInformation(int index);
};