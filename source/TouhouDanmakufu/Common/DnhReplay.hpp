#ifndef __TOUHOUDANMAKUFU_DNHREPLAY__
#define __TOUHOUDANMAKUFU_DNHREPLAY__

#include "../../GcLib/pch.h"

#include "DnhCommon.hpp"

/**********************************************************
//ReplayInformation
**********************************************************/
class ReplayInformation {
public:
	enum {
		INDEX_ACTIVE = 0,
		INDEX_DIGIT_MIN = 1,
		INDEX_DIGIT_MAX = 99,
		INDEX_USER = 100,
	};

	static const std::string REC_HEADER;

	class StageData;
private:
	std::wstring path_;
	std::wstring playerScriptID_;
	std::wstring playerScriptFileName_;
	std::wstring playerScriptReplayName_;

	std::wstring comment_;
	std::wstring userName_;
	int64_t totalScore_;
	double fpsAvarage_;
	SYSTEMTIME date_;

	shared_ptr<ScriptCommonData> userData_;
	std::map<int, ref_count_ptr<StageData>> mapStageData_;
public:
	ReplayInformation();
	virtual ~ReplayInformation();

	std::wstring GetPath() { return path_; }
	std::wstring GetPlayerScriptID() { return playerScriptID_; }
	void SetPlayerScriptID(std::wstring id) { playerScriptID_ = id; }
	std::wstring GetPlayerScriptFileName() { return playerScriptFileName_; }
	void SetPlayerScriptFileName(std::wstring name) { playerScriptFileName_ = name; }
	std::wstring GetPlayerScriptReplayName() { return playerScriptReplayName_; }
	void SetPlayerScriptReplayName(std::wstring name) { playerScriptReplayName_ = name; }

	std::wstring GetComment() { return comment_; }
	void SetComment(std::wstring comment) { comment_ = comment; }
	std::wstring GetUserName() { return userName_; }
	void SetUserName(std::wstring name) { userName_ = name; }
	int64_t GetTotalScore() { return totalScore_; }
	void SetTotalScore(int64_t score) { totalScore_ = score; }
	double GetAvarageFps() { return fpsAvarage_; }
	void SetAvarageFps(double fps) { fpsAvarage_ = fps; }
	SYSTEMTIME GetDate() { return date_; }
	void SetDate(SYSTEMTIME date) { date_ = date; }
	std::wstring GetDateAsString();

	void SetUserData(std::string key, gstd::value val);
	gstd::value GetUserData(std::string key);
	bool IsUserDataExists(std::string key);

	ref_count_ptr<StageData> GetStageData(int stage) { return mapStageData_[stage]; }
	void SetStageData(int stage, ref_count_ptr<StageData> data) { mapStageData_[stage] = data; }
	std::vector<int> GetStageIndexList();

	bool SaveToFile(std::wstring scriptPath, int index);
	static ref_count_ptr<ReplayInformation> CreateFromFile(std::wstring scriptPath, std::wstring fileName);
	static ref_count_ptr<ReplayInformation> CreateFromFile(std::wstring path);
};

class ReplayInformation::StageData {
private:
	//ステージ情報
	std::wstring mainScriptID_;
	std::wstring mainScriptName_;
	std::wstring mainScriptRelativePath_;

	int64_t scoreStart_;
	int64_t scoreLast_;
	int64_t graze_;
	int64_t point_;
	int frameEnd_;
	uint32_t randSeed_;
	std::vector<float> listFramePerSecond_;
	ref_count_ptr<gstd::RecordBuffer> recordKey_;
	std::map<std::string, ref_count_ptr<gstd::RecordBuffer> > mapCommonData_;

	//自機情報
	std::wstring playerScriptID_;
	std::wstring playerScriptFileName_;
	std::wstring playerScriptReplayName_;

	double playerLife_;
	double playerBombCount_;
	double playerPower_;
	int playerRebirthFrame_;//くらいボム有効フレーム

public:
	StageData() { recordKey_ = new gstd::RecordBuffer(); scoreStart_ = 0; scoreLast_ = 0; }
	virtual ~StageData() {}

	std::wstring GetMainScriptID() { return mainScriptID_; }
	void SetMainScriptID(std::wstring id) { mainScriptID_ = id; }
	std::wstring GetMainScriptName() { return mainScriptName_; }
	void SetMainScriptName(std::wstring name) { mainScriptName_ = name; }
	std::wstring GetMainScriptRelativePath() { return mainScriptRelativePath_; }
	void SetMainScriptRelativePath(std::wstring path) { mainScriptRelativePath_ = path; }
	int64_t GetStartScore() { return scoreStart_; }
	void SetStartScore(int64_t score) { scoreStart_ = score; }
	int64_t GetLastScore() { return scoreLast_; }
	void SetLastScore(int64_t score) { scoreLast_ = score; }
	int64_t GetGraze() { return graze_; }
	void SetGraze(int64_t graze) { graze_ = graze; }
	int64_t GetPoint() { return point_; }
	void SetPoint(int64_t point) { point_ = point; }
	int GetEndFrame() { return frameEnd_; }
	void SetEndFrame(int frame) { frameEnd_ = frame; }
	uint32_t GetRandSeed() { return randSeed_; }
	void SetRandSeed(uint32_t seed) { randSeed_ = seed; }
	float GetFramePerSecond(int frame) { int index = frame / 60; int res = index < listFramePerSecond_.size() ? listFramePerSecond_[index] : 0; return res; }
	void AddFramePerSecond(float frame) { listFramePerSecond_.push_back(frame); }
	double GetFramePerSecondAvarage();
	ref_count_ptr<gstd::RecordBuffer> GetReplayKeyRecord() { return recordKey_; }
	void SetReplayKeyRecord(ref_count_ptr<gstd::RecordBuffer> rec) { recordKey_ = rec; }
	std::set<std::string> GetCommonDataAreaList();
	shared_ptr<ScriptCommonData> GetCommonData(std::string area);
	void SetCommonData(std::string area, shared_ptr<ScriptCommonData> commonData);

	std::wstring GetPlayerScriptID() { return playerScriptID_; }
	void SetPlayerScriptID(std::wstring id) { playerScriptID_ = id; }
	std::wstring GetPlayerScriptFileName() { return playerScriptFileName_; }
	void SetPlayerScriptFileName(std::wstring name) { playerScriptFileName_ = name; }
	std::wstring GetPlayerScriptReplayName() { return playerScriptReplayName_; }
	void SetPlayerScriptReplayName(std::wstring name) { playerScriptReplayName_ = name; }
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

/**********************************************************
//ReplayInformationManager
**********************************************************/
class ReplayInformationManager {
public:

protected:
	std::map<int, ref_count_ptr<ReplayInformation> > mapInfo_;
public:
	ReplayInformationManager();
	virtual ~ReplayInformationManager();

	void UpdateInformationList(std::wstring pathScript);
	std::vector<int> GetIndexList();
	ref_count_ptr<ReplayInformation> GetInformation(int index);
};

#endif
