#ifndef __DIRECTX_EVENTSCRIPT__
#define __DIRECTX_EVENTSCRIPT__

#include"RenderObject.hpp"
#include"DxText.hpp"
#include"DxWindow.hpp"
#include"DxScript.hpp"
#include"TransitionEffect.hpp"

namespace directx {
	class EventScriptSource;
	class EventScriptCompiler;
	class EventScriptCode;
	class EventScriptBlock;
	class EventScriptBlock_Main;

	class EventValueParser;
	class EventEngine;
	class EventImage;

	class DxScriptForEvent;

	/**********************************************************
	//EventScriptSource
	//コンパイルされたイベントスクリプトコード
	**********************************************************/
	class EventScriptSource {
		friend EventScriptCompiler;
	public:

	protected:
		std::vector<gstd::ref_count_ptr<EventScriptCode> > code_;
		std::map<std::string, gstd::ref_count_ptr<EventScriptBlock_Main> > event_;
	public:
		EventScriptSource();
		virtual ~EventScriptSource();

		int GetCodeCount() { return code_.size(); }
		void AddCode(gstd::ref_count_ptr<EventScriptCode> code);
		gstd::ref_count_ptr<EventScriptCode> GetCode(int index) { return code_[index]; }
		gstd::ref_count_ptr<EventScriptBlock_Main> GetEventBlock(std::string name);
	};

	/**********************************************************
	//EventScriptScanner
	**********************************************************/
	class EventScriptScanner;
	class EventScriptToken {
		friend EventScriptScanner;
	public:
		enum Type {
			TK_UNKNOWN, TK_EOF, TK_NEWLINE,
			TK_ID,
			TK_INT, TK_REAL, TK_STRING,

			TK_OPENP, TK_CLOSEP, TK_OPENB, TK_CLOSEB, TK_OPENC, TK_CLOSEC,
			TK_SHARP,

			TK_COMMA, TK_EQUAL,
			TK_ASTERISK, TK_SLASH, TK_COLON, TK_SEMICOLON, TK_TILDE, TK_EXCLAMATION,
			TK_PLUS, TK_MINUS,
			TK_LESS, TK_GREATER,

			TK_TEXT,
		};
	protected:
		Type type_;
		std::string element_;
	public:
		EventScriptToken() { type_ = TK_UNKNOWN; }
		EventScriptToken(Type type, std::string element) { type_ = type; element_ = element; }

		virtual ~EventScriptToken() {}
		Type GetType() { return type_; }
		std::string& GetElement() { return element_; }

		int GetInteger();
		double GetReal();
		bool GetBoolean();
		std::string GetString();
		std::string& GetIdentifier();
	};

	class EventScriptScanner {
	public:
		const static int TOKEN_TAG_START;
		const static int TOKEN_TAG_END;
		const static std::string TAG_START;
		const static std::string TAG_END;
		const static std::string TAG_NEW_LINE;
		const static std::string TAG_RUBY;
		const static std::string TAG_FONT;
	protected:
		int line_;
		std::vector<char> buffer_;
		std::vector<char>::iterator pointer_;//今の位置
		EventScriptToken token_;//現在のトークン
		boolean bTagScan_;

		char _NextChar();//ポインタを進めて次の文字を調べる
		virtual void _SkipComment();//コメントをとばす
		virtual void _SkipSpace();//空白をとばす
		virtual void _RaiseError(std::wstring str);//例外を投げます
		bool _IsTextStartSign();
		bool _IsTextScan();
	public:
		EventScriptScanner(char* str, int size);
		EventScriptScanner(std::string str);
		EventScriptScanner(std::vector<char>& buf);
		virtual ~EventScriptScanner();

		EventScriptToken& GetToken();//現在のトークンを取得
		EventScriptToken& Next();
		bool HasNext();
		void CheckType(EventScriptToken& tok, int type);
		void CheckIdentifer(EventScriptToken& tok, std::string id);
		int GetCurrentLine();
		int SearchCurrentLine();

		std::vector<char>::iterator GetCurrentPointer();
		void SetCurrentPointer(std::vector<char>::iterator pos);
		void SetPointerBegin() { pointer_ = buffer_.begin(); }
		int GetCurrentPosition();
		void SetTagScanEnable(bool bEnable) { bTagScan_ = bEnable; }
	};

	/**********************************************************
	//EventScriptCompiler
	**********************************************************/
	class EventScriptCompiler {
		std::wstring path_;
		gstd::ref_count_ptr<EventScriptScanner> scan_;
		gstd::ref_count_ptr<EventScriptSource> source_;
		std::list<std::vector<gstd::ref_count_ptr<EventScriptBlock> > >block_;

		void _ParseBlock(gstd::ref_count_ptr<EventScriptCode> blockStartCode);
		gstd::ref_count_ptr<EventScriptCode> _ParseTag(gstd::ref_count_ptr<EventScriptCode> blockStartCode);
		std::map<std::string, EventScriptToken> _GetAllTagElement();
		std::string _GetNextTagElement();
		std::string _GetNextTagElement(int type);
	public:
		EventScriptCompiler();
		virtual ~EventScriptCompiler();

		void SetPath(std::wstring path) { path_ = path; }
		gstd::ref_count_ptr<EventScriptSource> Compile();
	};


	/**********************************************************
	//EventScriptBlock
	**********************************************************/
	class EventScriptBlock {
		friend EventScriptSource;
	public:
		enum {
			POS_NULL = -1,
		};
		const static std::string BLOCK_GLOBAL;
	protected:
		bool bInner_;
		int posStart_;
		int posEnd_;
		int posReturn_;
	public:
		EventScriptBlock() { bInner_ = false; posStart_ = POS_NULL; posEnd_ = POS_NULL; posReturn_ = POS_NULL; }
		virtual ~EventScriptBlock() {}
		int GetStartPosition() { return posStart_; }
		void SetStartPosition(int pos) { posStart_ = pos; }
		int GetEndPosition() { return posEnd_; }
		void SetEndPosition(int pos) { posEnd_ = pos; }
		int GetReturnPosition() { return posReturn_; }
		void SetReturnPosition(int pos) { posReturn_ = pos; }
		bool IsInner() { return bInner_; }
		virtual bool IsGlobal() { return false; }
	};

	class EventScriptBlock_Main : public EventScriptBlock {
		friend EventScriptSource;
	private:
		std::string name_;
	public:
		EventScriptBlock_Main() { bInner_ = false; }
		virtual ~EventScriptBlock_Main() {}
		std::string GetName() { return name_; }
		void SetName(std::string name) { name_ = name; }
		bool IsGlobal() { return name_ == BLOCK_GLOBAL; }
	};

	/**********************************************************
	//EventScriptCode
	**********************************************************/
	class EventScriptCode {
	public:
		enum {
			TYPE_UNKNOWN,
			TYPE_TEXT,
			TYPE_NEXT_LINE,
			TYPE_WAIT_CLICK,
			TYPE_WAIT_NEXT_PAGE,
			TYPE_WAIT_TIME,
			TYPE_CLEAR_MESSAGE,
			TYPE_NAME,
			TYPE_TRANSITION,
			TYPE_VISIBLE_TEXT,

			TYPE_VAR,
			TYPE_EVAL,
			TYPE_SYSVAL,
			TYPE_OUTPUT,

			TYPE_IMAGE,
			TYPE_SOUND,

			TYPE_IF,
			TYPE_ELSEIF,
			TYPE_ENDIF,
			TYPE_LOOP,
			TYPE_JUMP,

			TYPE_SCRIPT,
			TYPE_END,
			TYPE_BATTLE,
		};
		const static std::string TAG_WAIT_CLICK;
		const static std::string TAG_WAIT_NEXT_PAGE;
		const static std::string TAG_WAIT_TIME;
		const static std::string TAG_CLEAR_MESSAGE;
		const static std::string TAG_NAME;
		const static std::string TAG_TRANSITION;
		const static std::string TAG_VISIBLE_TEXT;
		const static std::string TAG_VAR;
		const static std::string TAG_EVAL;
		const static std::string TAG_SYSVAL;
		const static std::string TAG_OUTPUT;
		const static std::string TAG_IMAGE;
		const static std::string TAG_BGM;
		const static std::string TAG_SE;
		const static std::string TAG_IF;
		const static std::string TAG_ELSEIF;
		const static std::string TAG_ENDIF;
		const static std::string TAG_GOSUB;
		const static std::string TAG_GOTO;
		const static std::string TAG_SCRIPT;
		const static std::string TAG_SCRIPT_END;
		const static std::string TAG_END;
		const static std::string TAG_BATTLE;

		const static std::string STRING_INVALID;
	protected:
		int type_;
		int line_;
	public:
		EventScriptCode();
		virtual ~EventScriptCode();

		int GetType() { return type_; }
		int GetLine() { return line_; }
		void SetLine(int line) { line_ = line; }
		virtual std::string GetCodeText() { return ""; }
	};

	class EventScriptCode_Text : public EventScriptCode {
	protected:
		std::string text_;
	public:
		EventScriptCode_Text();
		std::string GetText() { return text_; }
		std::string GetCodeText() { return GetText(); }
		void SetText(std::string text) { text_ = text; }
	};
	class EventScriptCode_NextLine : public EventScriptCode {
	public:
		EventScriptCode_NextLine();
		std::string GetCodeText();
	};
	class EventScriptCode_WaitClick : public EventScriptCode {
	public:
		EventScriptCode_WaitClick();
	};
	class EventScriptCode_WaitNextPage : public EventScriptCode {
	public:
		EventScriptCode_WaitNextPage();
	};
	class EventScriptCode_WaitTime : public EventScriptCode {
		std::string time_;
		bool bSkipEnable_;
	public:
		EventScriptCode_WaitTime();
		std::string GetTime() { return time_; }
		void SetTime(std::string time) { time_ = time; }
		bool IsSkipEnable() { return bSkipEnable_; }
		void SetSkipEnable(bool bEnable) { bSkipEnable_ = bEnable; }
	};
	class EventScriptCode_ClearMessage : public EventScriptCode {
	public:
		EventScriptCode_ClearMessage();
	};
	class EventScriptCode_Name : public EventScriptCode {
		std::string name_;
	public:
		EventScriptCode_Name();
		std::string& GetName() { return name_; }
		void SetName(std::string name) { name_ = name; }
	};
	class EventScriptCode_Transition : public EventScriptCode {
	public:
		enum {
			TRANS_NONE,
			TRANS_FADE,
			TRANS_SCRIPT,
		};
	protected:
		int typeTrans_;
		std::string frame_;
		std::string path_;
		std::string method_;
	public:
		EventScriptCode_Transition();
		int GetTransType() { return typeTrans_; }
		void SetTransType(int type) { typeTrans_ = type; }
		std::string GetFrame() { return frame_; }
		void SetFrame(std::string frame) { frame_ = frame; }
		std::string GetPath() { return path_; }
		void SetPath(std::string path) { path_ = path; }
		std::string GetMethod() { return method_; }
		void SetMethod(std::string method) { method_ = method; }

	};
	class EventScriptCode_VisibleText : public EventScriptCode {
		bool bVisible_;
	public:
		EventScriptCode_VisibleText();
		bool IsVisible() { return bVisible_; }
		void SetVisible(bool bVisible) { bVisible_ = bVisible; }
	};
	class EventScriptCode_Var : public EventScriptCode {
		std::string name_;
		std::string value_;
	public:
		EventScriptCode_Var();
		std::string& GetName() { return name_; }
		void SetName(std::string name) { name_ = name; }
		std::string& GetValue() { return value_; }
		void SetValue(std::string value) { value_ = value; }
	};
	class EventScriptCode_Eval : public EventScriptCode {
		std::string name_;
		std::string value_;
	public:
		EventScriptCode_Eval();
		std::string& GetName() { return name_; }
		void SetName(std::string name) { name_ = name; }
		std::string& GetValue() { return value_; }
		void SetValue(std::string value) { value_ = value; }
	};
	class EventScriptCode_SysVal : public EventScriptCode {
		bool bGlobal_;
		std::string name_;
		std::string value_;
	public:
		EventScriptCode_SysVal();
		std::string& GetName() { return name_; }
		void SetName(std::string name) { name_ = name; }
		std::string& GetValue() { return value_; }
		void SetValue(std::string value) { value_ = value; }
		bool IsGlobal() { return bGlobal_; }
		void SetGlobal(bool b) { bGlobal_ = b; }
	};
	class EventScriptCode_Output : public EventScriptCode {
		std::string value_;
	public:
		EventScriptCode_Output();
		std::string& GetValue() { return value_; }
		void SetValue(std::string value) { value_ = value; }
	};
	class EventScriptCode_Image : public EventScriptCode {
	public:
		enum {
			VALUE_INVALID = INT_MIN,
		};
	protected:
		std::string idObject_;
		std::string path_;
		int layer_;
		std::string pri_;
		std::string bVisible_;
		std::string posDestLeft_;
		std::string posDestTop_;
		std::string bTransition_;
		std::string bWaitEnd_;
	public:
		EventScriptCode_Image();
		std::string GetObjectIdentifier() { return idObject_; }
		void SetObjectIdentifier(std::string id) { idObject_ = id; }
		std::string GetPath() { return path_; }
		void SetPath(std::string path) { path_ = path; }
		int GetLayer() { return layer_; }
		void SetLayer(int layer) { layer_ = layer; }
		std::string GetPriority() { return pri_; }
		void SetPriority(std::string pri) { pri_ = pri; }
		std::string GetVisible() { return bVisible_; }
		void SetVisible(std::string bVisible) { bVisible_ = bVisible_; }
		std::string GetLeftDestPoint() { return posDestLeft_; }
		void SetLeftDestPoint(std::string left) { posDestLeft_ = left; }
		std::string GetTopDestPoint() { return posDestTop_; }
		void SetTopDestPoint(std::string top) { posDestTop_ = top; }
		std::string GetTransition() { return bTransition_; }
		void SetTransition(std::string bTrans) { bTransition_ = bTrans; }
		std::string GetWaitEnd() { return bWaitEnd_; }
		void SetWaitEnd(std::string bWaitEnd) { bWaitEnd_ = bWaitEnd; }
	};
	class EventScriptCode_Sound : public EventScriptCode {
		int typeSound_;
		std::string path_;
	public:
		EventScriptCode_Sound();
		int GetSoundType() { return typeSound_; }
		void SetSoundType(int type) { typeSound_ = type; }
		std::string GetPath() { return path_; }
		void SetPath(std::string path) { path_ = path; }
	};
	class EventScriptCode_If : public EventScriptCode, public EventScriptBlock {
	protected:
		std::string param_;
		int posNextElse_;
	public:
		EventScriptCode_If();
		std::string GetParameter() { return param_; }
		void SetParameter(std::string param) { param_ = param; }
		int GetNextElsePosition() { return posNextElse_; }
		void SetNextElsePosition(int pos) { posNextElse_ = pos; }
	};

	class EventScriptCode_Jump : public EventScriptCode {
	protected:
		bool bGoSub_;
		std::string path_;
		std::string name_;
	public:
		EventScriptCode_Jump();

		bool IsGoSub() { return bGoSub_; }
		void SetGosub(bool b) { bGoSub_ = b; }

		std::string GetPath() { return path_; }
		void SetPath(std::string path) { path_ = path; }
		std::string GetName() { return name_; }
		void SetName(std::string name) { name_ = name; }
	};

	class EventScriptCode_Script : public EventScriptCode {
		std::string path_;
		std::string method_;
		std::string bWaitEnd_;
		std::string targetId_;
		std::string code_;
		std::string id_;
		std::vector<std::string> arg_;
		bool bEndScript_;
	public:
		EventScriptCode_Script();
		std::string GetPath() { return path_; }
		void SetPath(std::string path) { path_ = path; }
		std::string GetMethod() { return method_; }
		void SetMethod(std::string method) { method_ = method; }
		std::string GetWaitEnd() { return bWaitEnd_; }
		void SetWaitEnd(std::string bWait) { bWaitEnd_ = bWait; }
		std::string GetTargetId() { return targetId_; }
		void SetTargetId(std::string target) { targetId_ = target; }
		std::string GetCode() { return code_; }
		void SetCode(std::string code) { code_ = code; }
		std::string GetId() { return id_; }
		void SetId(std::string id) { id_ = id; }
		std::vector<std::string> GetArgumentList() { return arg_; }
		void SetArgument(int index, std::string arg) { if (index >= arg_.size())arg_.resize(index + 1); arg_[index] = arg; }

		bool IsEndScript() { return bEndScript_; }
		void SetEndScript(bool bEnd) { bEndScript_ = bEnd; }
	};

	class EventScriptCode_End : public EventScriptCode {
	protected:
		std::string arg_;
	public:
		EventScriptCode_End();
		std::string GetArgument() { return arg_; }
		void SetArgument(std::string arg) { arg_ = arg; }
	};

	class EventScriptCode_Battle : public EventScriptCode {
	protected:
		std::string path_;
	public:
		EventScriptCode_Battle();
		std::string GetPath() { return path_; }
		void SetPath(std::string path) { path_ = path; }
	};


	/**********************************************************
	//EventScriptCodeExecuter
	**********************************************************/
	class EventScriptCodeExecuter {
	protected:
		EventEngine* engine_;
		bool bEnd_;

		int _GetElementInteger(std::string value);
		double _GetElementReal(std::string value);
		bool _GetElementBoolean(std::string value);
		std::string _GetElementString(std::string value);
		bool _IsValieElement(std::string value);
	public:
		EventScriptCodeExecuter(EventEngine* engine);
		virtual ~EventScriptCodeExecuter();
		virtual void Execute() = 0;
		bool IsEnd() { return bEnd_; }
	};

	class EventScriptCodeExecuter_WaitClick : public EventScriptCodeExecuter {
	public:
		EventScriptCodeExecuter_WaitClick(EventEngine* engine) :EventScriptCodeExecuter(engine) {}
		virtual void Execute();
	};

	class EventScriptCodeExecuter_WaitNextPage : public EventScriptCodeExecuter {
	public:
		EventScriptCodeExecuter_WaitNextPage(EventEngine* engine) :EventScriptCodeExecuter(engine) {}
		virtual void Execute();
	};

	class EventScriptCodeExecuter_WaitTime : public EventScriptCodeExecuter {
		int count_;
		int time_;
		bool bSkipEnable_;
	public:
		EventScriptCodeExecuter_WaitTime(EventEngine* engine, EventScriptCode_WaitTime* code);
		virtual void Execute();
	};

	class EventScriptCodeExecuter_Transition : public EventScriptCodeExecuter {
		class DxScriptRenderObject_Transition : public DxScriptObjectBase {
			TransitionEffect* effect_;
		public:
			DxScriptRenderObject_Transition(TransitionEffect* effect) { effect_ = effect; priRender_ = 1.0; }
			virtual void Render() { effect_->Render(); }
			virtual void SetRenderState() {}
		};
		EventScriptCode_Transition* code_;
		gstd::ref_count_ptr<DxScriptForEvent> script_;
		gstd::ref_count_ptr<TransitionEffect> effect_;
		int frame_;
	public:
		EventScriptCodeExecuter_Transition(EventEngine* engine, EventScriptCode_Transition* code);
		virtual void Execute();
	};

	class EventScriptCodeExecuter_Image : public EventScriptCodeExecuter {
		EventScriptCode_Image* code_;
		std::string pathImage_;
		DxScriptObjectManager* objManager_;
		gstd::ref_count_ptr<DxScriptSpriteObject2D>::unsync nowSprite_;
		gstd::ref_count_ptr<DxScriptSpriteObject2D>::unsync oldSprite_;
		int frame_;
		bool bTrans_;
		double priority_;

		void _Initialize();
	public:
		EventScriptCodeExecuter_Image(EventEngine* engine, EventScriptCode_Image* code);
		~EventScriptCodeExecuter_Image();
		virtual void Execute();
	};

	class EventScriptCodeExecuter_Script : public EventScriptCodeExecuter {
		DxScriptForEvent* script_;
	public:
		EventScriptCodeExecuter_Script(EventEngine* engine, DxScriptForEvent* script);
		virtual void Execute();
	};

	/**********************************************************
	//EventWindowManager
	**********************************************************/
	class EventMouseCaptureLayer;
	class EventTextWindow;
	class EventNameWindow;
	class EventLogWindow;
	class EventWindowManager : public DxWindowManager {
	protected:
		EventEngine* engine_;
		gstd::ref_count_ptr<EventMouseCaptureLayer> layerCapture_;
		gstd::ref_count_ptr<EventTextWindow> wndText_;
		gstd::ref_count_ptr<EventLogWindow> wndLog_;
		gstd::ref_count_ptr<EventNameWindow> wndName_;

		gstd::ref_count_ptr<DxButton> btnSave_;
		gstd::ref_count_ptr<DxButton> btnLoad_;

		bool bVisibleText_;

	public:
		EventWindowManager(EventEngine* engine);
		EventEngine* GetEngine() { return engine_; }
		virtual bool Initialize();
		virtual void Work();
		virtual void Render();

		gstd::ref_count_ptr<EventMouseCaptureLayer> GetMouseCaptureLayer() { return layerCapture_; }
		gstd::ref_count_ptr<EventTextWindow> GetTextWindow() { return wndText_; }
		gstd::ref_count_ptr<EventLogWindow> GetLogWindow() { return wndLog_; }
		gstd::ref_count_ptr<EventNameWindow> GetNameWindow() { return wndName_; }

		gstd::ref_count_ptr<DxButton> GetSaveButton() { return btnSave_; }
		gstd::ref_count_ptr<DxButton> GetLoadButton() { return btnLoad_; }

		bool IsTextVisible() { return bVisibleText_; }
		void SetTextVisible(bool bVisible) { bVisibleText_ = bVisible; }

		void Read(gstd::RecordBuffer& record);
		void Write(gstd::RecordBuffer& record);
	};

	class EventWindow : public DxWindow {
	protected:
		bool bApplyVisibleText_;
		EventWindowManager* _GetManager() { return (EventWindowManager*)manager_; }
	public:
		EventWindow() { bApplyVisibleText_ = true; }
		virtual bool IsWindowVisible();
	};
	class EventButton : public DxButton {
	protected:
		bool bApplyVisibleText_;
		EventWindowManager* _GetManager() { return (EventWindowManager*)manager_; }
	public:
		EventButton() { bApplyVisibleText_ = true; }
		virtual bool IsWindowVisible();
	};
	class EventMouseCaptureLayer : public EventWindow {
	protected:
		gstd::ref_count_ptr<DxWindowEvent> event_;
	public:
		virtual void AddedManager();
		virtual void DispatchedEvent(gstd::ref_count_ptr<DxWindowEvent> event);
		void ClearEvent();
		gstd::ref_count_ptr<DxWindowEvent> GetEvent() { return event_; }
	};
	class EventTextWindow : public EventWindow {
	protected:
		RECT rcMargin_;
		gstd::ref_count_ptr<DxText> dxText_;
	public:
		EventTextWindow();
		virtual void AddedManager();
		virtual void Render();
		bool IsWait();
	};
	class EventNameWindow : public EventWindow {
		gstd::ref_count_ptr<DxText> text_;
	public:
		EventNameWindow();
		void Work();
		void Render();
		virtual void RenderText();
		std::wstring GetText() { return text_->GetText(); }
		void SetText(std::wstring text) { text_->SetText(text); }
	};
	class EventLogWindow : public EventWindow {
		int posMin_;//最低表示位置
		int pos_;
		gstd::ref_count_ptr<DxText> dxText_;
	public:
		EventLogWindow();
		virtual void AddedManager();
		virtual void Work();
		virtual void Render();

		gstd::ref_count_ptr<DxText> GetRenderer() { return dxText_; }
		void ResetPosition();
	};

	/**********************************************************
	//EventEngine
	**********************************************************/
	class EventScriptObjectManager : public DxScriptObjectManager, public gstd::Recordable {
		enum {
			INDEX_FREE_START = 256,
		};
	public:
		virtual int AddObject(gstd::ref_count_ptr<DxScriptObjectBase>::unsync obj);

		void Read(gstd::RecordBuffer& record);
		void Write(gstd::RecordBuffer& record);
	};
	class EventText : public DxTextStepper {
	public:
		bool IsVoiceText();
	};
	class EventLogText {
		EventEngine* engine_;
		int max_;
		std::vector<gstd::ref_count_ptr<DxTextInfo> > listInfo_;
	public:
		EventLogText(EventEngine* engine);
		virtual ~EventLogText();
		void Add(std::string text, std::string name);

		int GetInfoCount() { return listInfo_.size(); }
		gstd::ref_count_ptr<DxTextInfo> GetTextInfo(int pos) { return listInfo_[pos]; }
	};
	class EventValue : public gstd::Recordable {
		friend EventValueParser;
	public:
		enum {
			TYPE_UNKNOWN,
			TYPE_REAL,
			TYPE_BOOLEAN,
			TYPE_STRING,
		};

	protected:
		int type_;
		std::string valueString_;
		union {
			double valueReal_;
			bool valueBoolean_;
		};
	public:
		EventValue() { type_ = TYPE_UNKNOWN; }
		virtual ~EventValue() {};
		int GetType() { return type_; }
		double GetReal() {
			double res = valueReal_;
			if (IsBoolean())res = valueBoolean_ ? 1.0 : 0.0;
			if (IsString())res = atof(valueString_.c_str());
			return res;
		}
		bool GetBoolean() {
			bool res = valueBoolean_;
			if (IsReal())res = (valueReal_ != 0.0 ? true : false);
			if (IsString())res = (valueString_ == "true" ? true : false);
			return res;
		}
		std::string GetString() {
			std::string res = valueString_;
			if (IsReal())res = gstd::StringUtility::Format("%f", valueReal_);
			if (IsBoolean())res = (valueBoolean_ ? "true" : "false");
			return res;
		}
		bool IsReal() { return type_ == TYPE_REAL; }
		bool IsBoolean() { return type_ == TYPE_BOOLEAN; }
		bool IsString() { return type_ == TYPE_STRING; }

		void SetReal(double v) { type_ = TYPE_REAL; valueReal_ = v; }
		void SetBoolean(bool v) { type_ = TYPE_BOOLEAN; valueBoolean_ = v; }
		void SetString(std::string str) { type_ = TYPE_STRING; valueString_ = str; }

		gstd::TextParser::Result ConvertToTextParserResult();
		void Copy(gstd::TextParser::Result& val);
		void Copy(EventValue& val);

		void Read(gstd::RecordBuffer& record);
		void Write(gstd::RecordBuffer& record);
	};
	class EventFrame {
	protected:
		bool bEnd_;
		int posCode_;
		int posReturn_;
		bool bAutoGlobal_;
		gstd::ref_count_ptr<EventScriptSource> sourceActive_;
		gstd::ref_count_ptr<EventScriptBlock> block_;
		std::map<std::string, gstd::ref_count_ptr<EventValue> > mapValue_;
	public:
		EventFrame();
		virtual ~EventFrame();
		void SetActiveSource(gstd::ref_count_ptr<EventScriptSource> source) { sourceActive_ = source; }
		gstd::ref_count_ptr<EventScriptBlock> GetBlock() { return block_; }
		void SetBlock(gstd::ref_count_ptr<EventScriptBlock> block);
		gstd::ref_count_ptr<EventScriptSource> GetActiveSource() { return sourceActive_; }

		int GetCurrentPosition() { return posCode_; }
		void SetCurrentPosition(int pos) { posCode_ = pos; }
		gstd::ref_count_ptr<EventScriptCode> NextCode();
		gstd::ref_count_ptr<EventScriptCode> GetCurrentCode();
		bool HasNextCode();
		void SetEnd() { bEnd_ = true; }
		bool IsEnd() { return bEnd_; }
		gstd::ref_count_ptr<EventValue> GetValue(std::string key);
		void AddValue(std::string key, gstd::ref_count_ptr<EventValue> val);
		void SetValue(std::string key, gstd::ref_count_ptr<EventValue> val);

		int GetReturnPosition() { return posReturn_; }
		void SetReturnPosition(int pos) { posReturn_ = pos; }
		bool IsInnerBlock();

		bool IsAutoGlobal() { return bAutoGlobal_; }
		void SetAutoGlobal(bool bAuto) { bAutoGlobal_ = bAuto; }

		//保存
		void ReadRecord(gstd::RecordBuffer& record, EventEngine* engine);
		void WriteRecord(gstd::RecordBuffer& record, EventEngine* engine);
	};

	class EventValueParser : public gstd::TextParser {
	protected:
		EventEngine* engine_;
		virtual Result _ParseIdentifer(std::vector<char>::iterator pos);
		std::vector<std::string> _GetFuctionArgument();
	public:
		EventValueParser(EventEngine* engine);
		gstd::ref_count_ptr<EventValue> GetEventValue(std::string text);
	};

	class EventImage : public gstd::Recordable {
	public:
		enum {
			LAYER_FOREGROUND,
			LAYER_BACKGROUND,
			MAX_OBJECT = 256 * 4,
			INDEX_OLD_START = 100,
			ID_TRANSITION = 255,
		};
	protected:
		std::vector<EventScriptObjectManager*> objManager_;
		int indexForeground_;
	public:
		EventImage();
		virtual ~EventImage();
		void Render(int layer);
		int GetForegroundLayerIndex();
		int GetBackgroundLayerIndex();
		void SwapForeBackLayerIndex();
		EventScriptObjectManager* GetObjectManager(int layer) { return objManager_[layer]; }

		void Read(gstd::RecordBuffer& record);
		void Write(gstd::RecordBuffer& record);
	};

	class EventKeyState {
	protected:
		EventEngine* engine_;
		bool bNextEnable_;
	public:
		EventKeyState(EventEngine* engine);
		virtual ~EventKeyState();
		void Work();

		void SetNextEnable(bool bEnable) { bNextEnable_ = bEnable; }
		bool IsNext();
		bool IsSkip();
	};

	class EventSound : public gstd::Recordable {
	public:
		enum {
			TYPE_BGM,
			TYPE_SE,
		};
	private:
		gstd::ref_count_ptr<SoundPlayer> playerBgm_;
		gstd::ref_count_ptr<SoundPlayer> playerSe_;
	public:
		EventSound();
		virtual ~EventSound();

		void Play(int type, std::string path);
		void Delete(int type);

		void Read(gstd::RecordBuffer& record);
		void Write(gstd::RecordBuffer& record);
	};

	class EventEngine : public gstd::TaskBase, public gstd::Recordable {
	public:
		enum {
			STATE_RUN,
			STATE_LOG,
			STATE_HIDE_TEXT,
			STATE_STOP,
		};

	protected:
		enum {
			RUN_RETURN_NONE,
			RUN_RETURN_DO,
		};

		int state_;
		gstd::ref_count_ptr<EventWindowManager> windowManager_;
		gstd::ref_count_ptr<EventText> textEvent_;
		gstd::ref_count_ptr<EventLogText> logEvent_;
		std::map<std::wstring, gstd::ref_count_ptr<EventScriptSource> > mapSource_;
		std::vector<gstd::ref_count_ptr<EventFrame> > frame_;
		gstd::ref_count_ptr<EventImage> image_;
		gstd::ref_count_ptr<EventKeyState> keyState_;
		gstd::ref_count_ptr<EventSound> sound_;
		bool bCriticalFrame_;
		gstd::ref_count_ptr<EventFrame> frameGlobal_;//グローバル変数用

		gstd::ref_count_ptr<EventScriptCodeExecuter> activeCodeExecuter_;
		std::list<gstd::ref_count_ptr<EventScriptCodeExecuter> > parallelCodeExecuter_;
		std::list<gstd::ref_count_ptr<DxScriptForEvent> > listScript_;
		gstd::ref_count_ptr<gstd::ScriptEngineCache> cacheScriptEngine_;

		void _RaiseError(std::wstring msg);
		virtual void _RunCode();
		virtual int _RunCode(gstd::ref_count_ptr<EventFrame> frameActive, gstd::ref_count_ptr<EventScriptCode> code) { return RUN_RETURN_NONE; }
		virtual void _RunScript();
		virtual void _WorkWindow();
		gstd::ref_count_ptr<EventScriptSource> _GetSource(std::wstring path);
	public:
		EventEngine();
		virtual ~EventEngine();
		virtual bool Initialize();

		virtual void Work();
		virtual void Render();
		void SetSource(std::wstring path);

		gstd::ref_count_ptr<EventScriptSource> GetSource(std::wstring path);
		std::wstring GetSourcePath(gstd::ref_count_ptr<EventScriptSource> source);
		virtual bool IsEnd();

		gstd::ref_count_ptr<EventText> GetEventText() { return textEvent_; }
		gstd::ref_count_ptr<EventLogText> GetEventLogText() { return logEvent_; }
		gstd::ref_count_ptr<EventWindowManager> GetWindowManager() { return windowManager_; }

		gstd::ref_count_ptr<EventValue> GetEventValue(std::string key);
		gstd::ref_count_ptr<EventImage> GetEventImage() { return image_; }
		gstd::ref_count_ptr<EventKeyState> GetEventKeyState() { return keyState_; }

		gstd::ref_count_ptr<EventScriptCodeExecuter> GetActiveCodeExecuter() { return activeCodeExecuter_; }
		gstd::ref_count_ptr<EventFrame> GetGlobalFrame() { return frameGlobal_; }

		gstd::ref_count_ptr<gstd::ScriptEngineCache> GetScriptEngineCache() { return cacheScriptEngine_; }

		bool IsCriticalFrame() { return bCriticalFrame_; }

		void SetState(int state);
		int GetState() { return state_; }
		virtual void CheckStateChenge();


		bool IsSaveEnable();
		bool Load(std::wstring path);
		bool Load(gstd::RecordBuffer& record);
		bool Save(std::wstring path);
		void Read(gstd::RecordBuffer& record);
		void Write(gstd::RecordBuffer& record);
	};

	/**********************************************************
	//DxScriptForEvent
	**********************************************************/
	class DxScriptForEvent : public DxScript, public gstd::Recordable {
		EventEngine* engine_;
		bool bScriptEnd_;
		std::string method_;
		int targetId_;
		std::string scriptId_;
		std::set<int> listObj_;
		std::string code_;

		virtual std::vector<char> _Include(std::vector<char>& source);
	public:
		DxScriptForEvent(EventEngine* engine);
		~DxScriptForEvent();
		void Clear();
		virtual bool SetSourceFromFile(std::wstring path);
		virtual void SetSource(std::string source);
		virtual int AddObject(gstd::ref_count_ptr<DxScriptObjectBase>::unsync obj);
		virtual void DeleteObject(int id);

		std::string GetMethod() { return method_; }
		void SetMethod(std::string method) { method_ = method; }

		void EndScript() { bScriptEnd_ = true; }
		bool IsScriptEnd() { return bScriptEnd_; }

		void SetTargetId(int target) { targetId_ = target; }
		std::string GetScriptId() { return scriptId_; }
		void SetScriptId(std::string id) { scriptId_ = id; }
		std::string GetCode() { return code_; }

		void AddArgumentValue(gstd::ref_count_ptr<EventValue> arg);

		void Read(gstd::RecordBuffer& record);
		void Write(gstd::RecordBuffer& record);

		//関数：スクリプト操作
		static gstd::value Func_EndScript(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetTarget(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_GetEventValue(gstd::script_machine* machine, int argc, const gstd::value* argv);
		static gstd::value Func_SetEventValue(gstd::script_machine* machine, int argc, const gstd::value* argv);

		//関数：キー入力
		static gstd::value Func_IsSkip(gstd::script_machine* machine, int argc, const gstd::value* argv);

	};
}


#endif
