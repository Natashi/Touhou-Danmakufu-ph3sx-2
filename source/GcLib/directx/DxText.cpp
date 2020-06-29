#include "source/GcLib/pch.h"

#include "DxText.hpp"
#include "DxUtility.hpp"
#include "DirectGraphics.hpp"
#include "RenderObject.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//DxFont
**********************************************************/
DxFont::DxFont() {
	ZeroMemory(&info_, sizeof(LOGFONT));
	colorTop_ = D3DCOLOR_ARGB(255, 128, 128, 255);
	colorBottom_ = D3DCOLOR_ARGB(255, 32, 32, 255);
	typeBorder_ = BORDER_NONE;
	widthBorder_ = 2;
	colorBorder_ = D3DCOLOR_ARGB(128, 255, 255, 255);
}
DxFont::~DxFont() {

}

/**********************************************************
//DxChar
**********************************************************/
DxChar::DxChar() {

}
DxChar::~DxChar() {}

bool DxChar::Create(int code, Font& winFont, DxFont& dxFont) {
	code_ = code;
	font_ = dxFont;

	int typeBorder = font_.GetBorderType();
	D3DCOLOR colorTop = font_.GetTopColor();
	D3DCOLOR colorBottom = font_.GetBottomColor();
	D3DCOLOR colorBorder = font_.GetBorderColor();
	int widthBorder = typeBorder != DxFont::BORDER_NONE ? font_.GetBorderWidth() : 0;

	HDC hDC = ::GetDC(NULL);
	HFONT oldFont = (HFONT)SelectObject(hDC, winFont.GetHandle());

	// フォントビットマップ取得
	TEXTMETRIC tm;
	::GetTextMetrics(hDC, &tm);
	GLYPHMETRICS gm;
	CONST MAT2 mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
	int uFormat = GGO_GRAY2_BITMAP;//typeBorder == DxFont::BORDER_FULL ? GGO_BITMAP : GGO_GRAY2_BITMAP;

	if (dxFont.GetLogFont().lfHeight <= 12)
		uFormat = GGO_BITMAP;
	DWORD size = ::GetGlyphOutline(hDC, code, uFormat, &gm, 0, NULL, &mat);

	BYTE* ptr = new BYTE[size];
	::GetGlyphOutline(hDC, code, uFormat, &gm, size, ptr, &mat);

	// デバイスコンテキストとフォントハンドルの解放
	::SelectObject(hDC, oldFont);
	::ReleaseDC(NULL, hDC);

	//テクスチャ作成
	int tex_x = gm.gmCellIncX + widthBorder * 2;
	int tex_y = tm.tmHeight + widthBorder * 2;
	int widthTexture = 1;
	int heightTexture = 1;
	while (widthTexture < tex_x) {
		widthTexture = widthTexture << 1;
	}
	while (heightTexture < tex_y) {
		heightTexture = heightTexture << 1;
	}

	IDirect3DTexture9* pTexture = nullptr;
	HRESULT hr = DirectGraphics::GetBase()->GetDevice()->CreateTexture(
		widthTexture, heightTexture,
		1, D3DPOOL_DEFAULT, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
	if (FAILED(hr))return false;

	D3DLOCKED_RECT lock;
	if (FAILED(pTexture->LockRect(0, &lock, NULL, D3DLOCK_DISCARD))) {
		return false;
	}
	int iOfs_x = gm.gmptGlyphOrigin.x;
	int iOfs_y = tm.tmAscent - gm.gmptGlyphOrigin.y;
	int iBmp_w = uFormat != GGO_BITMAP ? gm.gmBlackBoxX + (4 - (gm.gmBlackBoxX % 4)) % 4 : gm.gmBlackBoxX;
	int iBmp_h = gm.gmBlackBoxY;
	int level = 5;//GGO_GRAY4_BITMAP;
	FillMemory(lock.pBits, lock.Pitch * tm.tmHeight, 0);

	double gapColor = 1.0 / (double)iBmp_h;
	double gapColorR = (double)(ColorAccess::GetColorR(colorTop) - ColorAccess::GetColorR(colorBottom)) * gapColor;
	double gapColorG = (double)(ColorAccess::GetColorG(colorTop) - ColorAccess::GetColorG(colorBottom)) * gapColor;
	double gapColorB = (double)(ColorAccess::GetColorB(colorTop) - ColorAccess::GetColorB(colorBottom)) * gapColor;

	int xMax = iBmp_w + iOfs_x + widthBorder * 2;
	int yMax = iBmp_h + iOfs_y + widthBorder * 2;

	if (size > 0) {
#pragma omp for
		for (int iy = 0; iy < yMax; iy++) {
			int yBmp = iy - iOfs_y - widthBorder;
			int colorR = ColorAccess::GetColorR(colorTop) - gapColorR * yBmp;
			int colorG = ColorAccess::GetColorG(colorTop) - gapColorG * yBmp;
			int colorB = ColorAccess::GetColorB(colorTop) - gapColorB * yBmp;

			for (int ix = 0; ix < xMax; ix++) {
				int xBmp = ix - iOfs_x - widthBorder;

				DWORD alpha = 255;
				if (uFormat != GGO_BITMAP) {
					int posBmp = xBmp + iBmp_w * yBmp;
					alpha = xBmp >= 0 && xBmp < iBmp_w && yBmp >= 0 && yBmp < iBmp_h ?
						(255 * ptr[posBmp]) / (level - 1) : 0;
				}
				else {
					if (xBmp >= 0 && xBmp < iBmp_w && yBmp >= 0 && yBmp < iBmp_h) {
						int lineByte = (1 + (iBmp_w / 32)) * 4; // 1行に使用しているBYTE数（4バイト境界あり）
						int posBmp = xBmp / 8 + lineByte * yBmp;
						alpha = BitAccess::GetBit(ptr[posBmp], 7 - xBmp % 8) ? 255 : 0;
					}
					else alpha = 0;
				}

				DWORD color = 0;
				if (typeBorder != DxFont::BORDER_NONE && alpha != 255) {
					if (alpha == 0) {
						int count = 0;
						int antiDist = 0;
						int bx = typeBorder == DxFont::BORDER_FULL ? xBmp + widthBorder + antiDist : xBmp + 1;
						int by = typeBorder == DxFont::BORDER_FULL ? yBmp + widthBorder + antiDist : yBmp + 1;
						int minAlphaEnableDist = 255 * 255;
						for (int ax = xBmp - widthBorder - antiDist; ax <= bx; ax++) {
							for (int ay = yBmp - widthBorder - antiDist; ay <= by; ay++) {
								int dist = abs(ax - xBmp) + abs(ay - yBmp);
								if (dist > widthBorder + antiDist || dist == 0)continue;

								DWORD tAlpha = 255;
								if (uFormat != GGO_BITMAP) {
									tAlpha = ax >= 0 && ax < iBmp_w && ay >= 0 && ay < iBmp_h ?
										(255 * ptr[ax + iBmp_w * ay]) / (level - 1) : 0;
								}
								else {
									if (ax >= 0 && ax < iBmp_w && ay >= 0 && ay < iBmp_h) {
										int lineByte = (1 + (iBmp_w / 32)) * 4; // 1行に使用しているBYTE数（4バイト境界あり）
										int tPos = ax / 8 + lineByte * ay;
										tAlpha = BitAccess::GetBit(ptr[tPos], 7 - ax % 8) ? 255 : 0;
									}
									else tAlpha = 0;
								}

								if (tAlpha > 0)
									minAlphaEnableDist = std::min(minAlphaEnableDist, dist);

								int tCount = tAlpha /= dist;
								tCount *= 2;
								if (typeBorder == DxFont::BORDER_SHADOW && (ax >= xBmp || ay >= yBmp))
									tCount /= 2;
								count += tCount;

							}
						}
						color = colorBorder;

						int destAlpha = 0;
						if (minAlphaEnableDist < widthBorder)
							destAlpha = 255;
						else if (minAlphaEnableDist == widthBorder)
							destAlpha = count;
						else destAlpha = 0;
						//color = ColorAccess::SetColorA(color, ColorAccess::GetColorA(colorBorder)*count/255);
						color = ColorAccess::SetColorA(color, ColorAccess::GetColorA(colorBorder) * destAlpha / 255);
					}
					else {
						int oAlpha = alpha + 64;
						if (alpha > 255)alpha = 255;

						int count = 0;
						int cDist = 1;
						int bx = typeBorder == DxFont::BORDER_FULL ? xBmp + cDist : xBmp + 1;
						int by = typeBorder == DxFont::BORDER_FULL ? yBmp + cDist : yBmp + 1;
						for (int ax = xBmp - cDist; ax <= bx; ax++) {
							for (int ay = yBmp - cDist; ay <= by; ay++) {
								DWORD tAlpha = 255;
								if (uFormat != GGO_BITMAP) {
									tAlpha = tAlpha = ax >= 0 && ax < iBmp_w && ay >= 0 && ay < iBmp_h ?
										(255 * ptr[ax + iBmp_w * ay]) / (level - 1) : 0;
								}
								else {
									if (ax >= 0 && ax < iBmp_w && ay >= 0 && ay < iBmp_h) {
										int lineByte = (1 + (iBmp_w / 32)) * 4; // 1行に使用しているBYTE数（4バイト境界あり）
										int tPos = ax / 8 + lineByte * ay;
										tAlpha = BitAccess::GetBit(ptr[tPos], 7 - ax % 8) ? 255 : 0;
									}
									else tAlpha = 0;
								}

								if (tAlpha > 0)count++;
							}
						}
						if (count >= 2)oAlpha = alpha;

						int bAlpha = 255 - oAlpha;
						color = ColorAccess::SetColorA(color, 255);
						color = ColorAccess::SetColorR(color, colorR * oAlpha / 255 + ColorAccess::GetColorR(colorBorder) * bAlpha / 255);
						color = ColorAccess::SetColorG(color, colorG * oAlpha / 255 + ColorAccess::GetColorG(colorBorder) * bAlpha / 255);
						color = ColorAccess::SetColorB(color, colorB * oAlpha / 255 + ColorAccess::GetColorB(colorBorder) * bAlpha / 255);
					}
				}
				else {
					if (typeBorder != DxFont::BORDER_NONE && alpha > 0)alpha = 255;
					color = 0x00ffffff | (alpha << 24);
					color = ColorAccess::SetColorR(color, colorR);
					color = ColorAccess::SetColorG(color, colorG);
					color = ColorAccess::SetColorB(color, colorB);
				}
				memcpy((BYTE*)lock.pBits + lock.Pitch * iy + 4 * ix, &color, sizeof(DWORD));
			}
		}
	}
	pTexture->UnlockRect(0);

	delete[] ptr;

	texture_ = std::make_shared<Texture>();
	texture_->SetTexture(pTexture);

	width_ = gm.gmCellIncX + widthBorder * 2;
	height_ = tm.tmHeight + widthBorder * 2;

	return true;
}

/**********************************************************
//DxCharCache
**********************************************************/
DxCharCache::DxCharCache() {
	sizeMax_ = 512;
	countPri_ = 0;
}
DxCharCache::~DxCharCache() {
	Clear();
}
void DxCharCache::_arrange() {
	countPri_ = 0;
	std::map<int, DxCharCacheKey> mapPriKeyLast = mapPriKey_;
	mapPriKey_.clear();
	mapKeyPri_.clear();
	std::map<int, DxCharCacheKey>::iterator itr;
	for (itr = mapPriKeyLast.begin(); itr != mapPriKeyLast.end(); itr++) {
		DxCharCacheKey key = itr->second;
		int pri = countPri_;

		mapPriKey_[pri] = key;
		mapKeyPri_[key] = pri;

		countPri_++;
	}
}
void DxCharCache::Clear() {
	mapPriKey_.clear();
	mapKeyPri_.clear();
	mapCache_.clear();
}
shared_ptr<DxChar> DxCharCache::GetChar(DxCharCacheKey& key) {
	shared_ptr<DxChar> res;

	auto itr = mapCache_.find(key);
	if (itr != mapCache_.end()) {
		res = itr->second;
		/*
				//キーの優先順位をトップにする
				int tPri = mapKeyPri_[key];
				mapPriKey_.erase(tPri);

				countPri_++;
				int nPri = countPri_;
				mapKeyPri_[key] = nPri;
				mapPriKey_[nPri] = key;

				if(countPri_ >= INT_MAX)
				{
					//再配置
					_arrange();
				}
		*/
	}
	return res;
}

void DxCharCache::AddChar(DxCharCacheKey& key, shared_ptr<DxChar> value) {
	bool bExist = mapCache_.find(key) != mapCache_.end();
	if (bExist) return;
	mapCache_[key] = value;

	if (mapCache_.size() >= sizeMax_) {
		mapCache_.clear();
		/*
				//優先度の低いキャッシュを削除
				std::map<int, DxCharCacheKey>::iterator itrMinPri = mapPriKey_.begin();
				int minPri = itrMinPri->first;
				DxCharCacheKey keyMinPri = itrMinPri->second;

				mapCache_.erase(keyMinPri);
				mapKeyPri_.erase(keyMinPri);
				mapPriKey_.erase(minPri);
		*/
	}
}


/**********************************************************
//DxTextScanner
**********************************************************/
const int DxTextScanner::TOKEN_TAG_START = DxTextToken::TK_OPENB;
const int DxTextScanner::TOKEN_TAG_END = DxTextToken::TK_CLOSEB;
const std::wstring DxTextScanner::TAG_START = L"[";
const std::wstring DxTextScanner::TAG_END = L"]";
const std::wstring DxTextScanner::TAG_NEW_LINE = L"r";
const std::wstring DxTextScanner::TAG_RUBY = L"ruby";
const std::wstring DxTextScanner::TAG_FONT = L"font";
const wchar_t CHAR_TAG_START = L'[';
const wchar_t CHAR_TAG_END = L']';
DxTextScanner::DxTextScanner(wchar_t* str, int charCount) {
	std::vector<wchar_t> buf;
	buf.resize(charCount);
	if (buf.size() != 0) {
		memcpy(&buf[0], str, charCount * sizeof(wchar_t));
	}

	buf.push_back(L'\0');
	this->DxTextScanner::DxTextScanner(buf);
}
DxTextScanner::DxTextScanner(std::wstring str) {
	std::vector<wchar_t> buf;
	buf.resize(str.size() + 1);
	memcpy(&buf[0], str.c_str(), (str.size() + 1) * sizeof(wchar_t));
	this->DxTextScanner::DxTextScanner(buf);
}
DxTextScanner::DxTextScanner(std::vector<wchar_t>& buf) {
	buffer_ = buf;
	pointer_ = buffer_.begin();

	line_ = 1;
	bTagScan_ = false;
}
DxTextScanner::~DxTextScanner() {

}

wchar_t DxTextScanner::_NextChar() {
	if (HasNext() == false) {
		std::wstring source;
		source.resize(buffer_.size());
		memcpy(&source[0], &buffer_[0], source.size() * sizeof(wchar_t));
		std::wstring log = StringUtility::Format(L"_NextChar(Text): Unexpected end-of-file while parsing string. -> %s", 
			source.c_str());
		_RaiseError(log);
	}

	pointer_++;

	if (*pointer_ == L'\n')
		line_++;
	return *pointer_;
}
void DxTextScanner::_SkipComment() {
	while (true) {
		std::vector<wchar_t>::iterator posStart = pointer_;
		_SkipSpace();

		wchar_t ch = *pointer_;

		if (ch == L'/') {//コメントアウト処理
			std::vector<wchar_t>::iterator tPos = pointer_;
			ch = _NextChar();
			if (ch == L'/') {// "//"
				while (true) {
					ch = _NextChar();
					if (ch == L'\r' || ch == L'\n')break;
				}
			}
			else if (ch == L'*') {// "/*"-"*/"
				while (true) {
					ch = _NextChar();
					if (ch == L'*') {
						ch = _NextChar();
						if (ch == L'/')
							break;
					}
				}
				ch = _NextChar();
			}
			else {
				pointer_ = tPos;
				ch = L'/';
			}
		}

		//スキップも空白飛ばしも無い場合、終了
		if (posStart == pointer_)break;
	}
}
void DxTextScanner::_SkipSpace() {
	wchar_t ch = *pointer_;
	while (true) {
		if (ch != L' ' && ch != L'\t')break;
		ch = _NextChar();
	}
}
void DxTextScanner::_RaiseError(std::wstring str) {
	throw gstd::wexception(str.c_str());
}
bool DxTextScanner::_IsTextStartSign() {
	if (bTagScan_)return false;

	bool res = false;
	wchar_t ch = *pointer_;

	if (false && ch == L'\\') {
		std::vector<wchar_t>::iterator pos = pointer_;
		ch = _NextChar();//次のタグまで進める
		bool bLess = ch == CHAR_TAG_START;
		if (!bLess) {
			res = false;
			SetCurrentPointer(pos);
		}
		else {
			res = !bLess;
		}
	}
	else {
		bool bLess = ch == CHAR_TAG_START;
		res = !bLess;
	}

	return res;
}
bool DxTextScanner::_IsTextScan() {
	bool res = false;
	wchar_t ch = _NextChar();
	if (!HasNext()) {
		return false;
	}
	else if (ch == L'/') {
		ch = *(pointer_ + 1);
		if (ch == L'/' || ch == L'*')res = false;
	}
	else if (false && ch == L'\\') {
		ch = _NextChar();//次のタグまで進める
		res = true;
	}
	else {
		bool bGreater = ch == CHAR_TAG_END;
		if (bGreater) {
			_RaiseError(L"テキスト中にタグ終了文字が存在しました");
		}
		bool bNotLess = ch != CHAR_TAG_START;
		res = bNotLess;
	}
	return res;
}
DxTextToken& DxTextScanner::GetToken() {
	return token_;
}
DxTextToken& DxTextScanner::Next() {
	if (!HasNext()) {
		std::wstring source;
		source.resize(buffer_.size());
		memcpy(&source[0], &buffer_[0], source.size() * sizeof(wchar_t));
		std::wstring log = StringUtility::Format(L"Next(Text): Unexpected end-of-file -> %s", source.c_str());
		_RaiseError(log);
	}

	_SkipComment();//コメントをとばします

	wchar_t ch = *pointer_;
	if (ch == L'\0') {
		token_ = DxTextToken(DxTextToken::TK_EOF, L"\0");
		return token_;
	}

	DxTextToken::Type type = DxTextToken::TK_UNKNOWN;
	std::vector<wchar_t>::iterator posStart = pointer_;//先頭を保存

	if (_IsTextStartSign()) {
		ch = *pointer_;

		posStart = pointer_;
		while (_IsTextScan()) {

		}

		ch = *pointer_;
		if (ch == CHAR_TAG_START) {
		}
		else if (!HasNext()) {
		}
		//		else _RaiseError("Next:すでに文字列終端です");

		type = DxTextToken::TK_TEXT;
		std::wstring text = std::wstring(posStart, pointer_);
		text = StringUtility::ReplaceAll(text, L"\\", L"");
		token_ = DxTextToken(type, text);
	}
	else {
		switch (ch) {
		case L'\0': type = DxTextToken::TK_EOF; break;//終端
		case L',': _NextChar(); type = DxTextToken::TK_COMMA;  break;
		case L'=': _NextChar(); type = DxTextToken::TK_EQUAL;  break;
		case L'(': _NextChar(); type = DxTextToken::TK_OPENP; break;
		case L')': _NextChar(); type = DxTextToken::TK_CLOSEP; break;
		case L'[': _NextChar(); type = DxTextToken::TK_OPENB; break;
		case L']': _NextChar(); type = DxTextToken::TK_CLOSEB; break;
		case L'{': _NextChar(); type = DxTextToken::TK_OPENC; break;
		case L'}': _NextChar(); type = DxTextToken::TK_CLOSEC; break;
		case L'*': _NextChar(); type = DxTextToken::TK_ASTERISK; break;
		case L'/': _NextChar(); type = DxTextToken::TK_SLASH; break;
		case L':': _NextChar(); type = DxTextToken::TK_COLON; break;
		case L';': _NextChar(); type = DxTextToken::TK_SEMICOLON; break;
		case L'~': _NextChar(); type = DxTextToken::TK_TILDE; break;
		case L'!': _NextChar(); type = DxTextToken::TK_EXCLAMATION; break;
		case L'#': _NextChar(); type = DxTextToken::TK_SHARP; break;
		case L'<': _NextChar(); type = DxTextToken::TK_LESS; break;
		case L'>': _NextChar(); type = DxTextToken::TK_GREATER; break;

		case L'"':
		{
			ch = _NextChar();//1つ進めて
			wchar_t pre = ch;
			while (true) {
				if (ch == L'"' && pre != L'\\')break;
				pre = ch;
				ch = _NextChar();//次のダブルクオーテーションまで進める
			}

			if (ch == L'"') _NextChar();//ダブルクオーテーションだったら1つ進める
			else _RaiseError(L"Next(Text): Unexpected end-of-file while parsing string.");
			type = DxTextToken::TK_STRING;
			break;
		}

		case L'\r':case L'\n'://改行
			//改行がいつまでも続くようなのも1つの改行として扱う
			while (ch == L'\r' || ch == L'\n') ch = _NextChar();
			type = DxTextToken::TK_NEWLINE;
			break;

		case L'+':case L'-':
		{
			if (ch == L'+') {
				ch = _NextChar(); type = DxTextToken::TK_PLUS;

			}
			else if (ch == L'-') {
				ch = _NextChar(); type = DxTextToken::TK_MINUS;
			}

			if (!iswdigit(ch))break;//次が数字でないなら抜ける
		}


		default:
		{
			if (iswdigit(ch)) {
				//整数か実数
				while (iswdigit(ch))ch = _NextChar();//数字だけの間ポインタを進める
				type = DxTextToken::TK_INT;
				if (ch == L'.') {
					//実数か整数かを調べる。小数点があったら実数
					ch = _NextChar();
					while (iswdigit(ch))ch = _NextChar();//数字だけの間ポインタを進める
					type = DxTextToken::TK_REAL;
				}

				if (ch == L'E' || ch == L'e') {
					//1E-5みたいなケース
					std::vector<wchar_t>::iterator pos = pointer_;
					ch = _NextChar();
					while (iswdigit(ch) || ch == L'-')ch = _NextChar();//数字だけの間ポインタを進める
					type = DxTextToken::TK_REAL;
				}

			}
			else if (iswalpha(ch) || ch == L'_') {
				//たぶん識別子
				while (iswalpha(ch) || iswdigit(ch) || ch == L'_')ch = _NextChar();//たぶん識別子な間ポインタを進める
				type = DxTextToken::TK_ID;
			}
			else {
				_NextChar();
				type = DxTextToken::TK_UNKNOWN;
			}
			break;
		}
		}
		if (type == DxTextScanner::TOKEN_TAG_START)bTagScan_ = true;
		else if (type == DxTextScanner::TOKEN_TAG_END)bTagScan_ = false;

		if (type == DxTextToken::TK_STRING) {
			//\を除去
			std::wstring tmpStr(posStart, pointer_);
			std::wstring str = StringUtility::ReplaceAll(tmpStr, L"\\\"", L"\"");
			token_ = DxTextToken(type, str);
		}
		else {
			token_ = DxTextToken(type, std::wstring(posStart, pointer_));
		}

	}

	return token_;
}
bool DxTextScanner::HasNext() {
	return pointer_ != buffer_.end() && *pointer_ != L'\0' && token_.GetType() != DxTextToken::TK_EOF;
}
void DxTextScanner::CheckType(DxTextToken& tok, int type) {
	if (tok.type_ != type) {
		std::wstring str = StringUtility::Format(L"CheckType error[%s]:", tok.element_.c_str());
		_RaiseError(str);
	}
}
void DxTextScanner::CheckIdentifer(DxTextToken& tok, std::wstring id) {
	if (tok.type_ != DxTextToken::TK_ID || tok.GetIdentifier() != id) {
		std::wstring str = StringUtility::Format(L"CheckID error[%s]:", tok.element_.c_str());
		_RaiseError(str);
	}
}
int DxTextScanner::GetCurrentLine() {
	return line_;
}
int DxTextScanner::SearchCurrentLine() {
	int line = 1;
	wchar_t* pbuf = &(*buffer_.begin());
	wchar_t* ebuf = &(*pointer_);
	while (true) {
		if (pbuf >= ebuf)break;
		else if (*pbuf == L'\n')line++;

		pbuf++;
	}
	return line;
}
std::vector<wchar_t>::iterator DxTextScanner::GetCurrentPointer() {
	return pointer_;
}
void DxTextScanner::SetCurrentPointer(std::vector<wchar_t>::iterator pos) {
	pointer_ = pos;
}
int DxTextScanner::GetCurrentPosition() {
	if (buffer_.size() == 0)return 0;
	wchar_t* pos = (wchar_t*) & *pointer_;
	return pos - &buffer_[0];
}

//DxTextToken
std::wstring& DxTextToken::GetIdentifier() {
	if (type_ != TK_ID) {
		throw gstd::wexception(L"DxTextToken::GetIdentifier:データのタイプが違います");
	}
	return element_;
}
std::wstring DxTextToken::GetString() {
	if (type_ != TK_STRING) {
		throw gstd::wexception(L"DxTextToken::GetString:データのタイプが違います");
	}
	return element_.substr(1, element_.size() - 2);
}
int DxTextToken::GetInteger() {
	if (type_ != TK_INT) {
		throw gstd::wexception(L"DxTextToken::GetInterger:データのタイプが違います");
	}
	int res = StringUtility::ToInteger(element_);
	return res;
}
double DxTextToken::GetReal() {
	if (type_ != TK_REAL && type_ != TK_INT) {
		throw gstd::wexception(L"DxTextToken::GetReal:データのタイプが違います");
	}

	double res = StringUtility::ToDouble(element_);
	return res;
}
bool DxTextToken::GetBoolean() {
	bool res = false;
	if (type_ == TK_REAL || type_ == TK_INT) {
		res = GetReal() == 1;
	}
	else {
		res = element_ == L"true";
	}
	return res;
}

/**********************************************************
//DxTextRenderer
//テキスト描画エンジン
**********************************************************/
//Tag

//DxTextTag_Ruby


//DxTextRenderObject
DxTextRenderObject::DxTextRenderObject() {
	position_.x = 0;
	position_.y = 0;

	scale_ = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	center_ = D3DXVECTOR2(0.0f, 0.0f);
	bAutoCenter_ = true;
	bPermitCamera_ = true;
}
DxTextRenderObject::~DxTextRenderObject() {
	listData_.clear();
}
void DxTextRenderObject::Render() {
	D3DXVECTOR2 angZero(1, 0);
	DxTextRenderObject::Render(angZero, angZero, angZero);
}
void DxTextRenderObject::Render(D3DXVECTOR2& angX, D3DXVECTOR2& angY, D3DXVECTOR2& angZ) {
	D3DXVECTOR2 position = D3DXVECTOR2(position_.x, position_.y);

	auto camera = DirectGraphics::GetBase()->GetCamera2D();
	bool bCamera = camera->IsEnable() && bPermitCamera_;

	D3DXVECTOR2 center = center_;
	if (bAutoCenter_) {
		RECT rect;
		ZeroMemory(&rect, sizeof(RECT));

		std::list<DxTextRenderObject::ObjectData>::iterator itr = listData_.begin();
		for (; itr != listData_.end(); itr++) {
			ObjectData obj = *itr;
			RECT_D rcDest = obj.sprite->GetDestinationRect();
			rect.left = std::min(rect.left, (LONG)rcDest.left);
			rect.top = std::min(rect.top, (LONG)rcDest.top);
			rect.right = std::max(rect.right, (LONG)rcDest.right);
			rect.bottom = std::max(rect.bottom, (LONG)rcDest.bottom);
		}
		center.x = (rect.right + rect.left) / 2;
		center.y = (rect.bottom + rect.top) / 2;
	}

	auto itr = listData_.begin();
	for (; itr != listData_.end(); itr++) {
		ObjectData obj = *itr;

		D3DXVECTOR2 bias = D3DXVECTOR2(obj.bias.x, obj.bias.y);
		shared_ptr<Sprite2D> sprite = obj.sprite;

		sprite->SetColorRGB(color_);
		sprite->SetAlpha(ColorAccess::GetColorA(color_));

		D3DXMATRIX matWorld = RenderObject::CreateWorldMatrixText2D(center, scale_, angX, angY, angZ,
			position, bias, bCamera ? &DirectGraphics::GetBase()->GetCamera2D()->GetMatrix() : nullptr);

		sprite->SetPermitCamera(false);
		sprite->SetShader(shader_);
		//sprite->SetPosition(pos);
		//sprite->SetScale(scale_);
		sprite->SetDisableMatrixTransformation(true);
		sprite->Render(matWorld);
	}
}
void DxTextRenderObject::AddRenderObject(shared_ptr<Sprite2D> obj) {
	ObjectData data;
	ZeroMemory(&data.bias, sizeof(POINT));
	data.sprite = obj;
	listData_.push_back(data);
}
void DxTextRenderObject::AddRenderObject(shared_ptr<DxTextRenderObject> obj, POINT bias) {
	std::list<ObjectData>::iterator itr = obj->listData_.begin();
	for (; itr != obj->listData_.end(); itr++) {
		(*itr).bias = bias;
		listData_.push_back(*itr);
	}
}

//DxTextRenderer
DxTextRenderer* DxTextRenderer::thisBase_ = NULL;
DxTextRenderer::DxTextRenderer() {
	colorVertex_ = D3DCOLOR_ARGB(255, 255, 255, 255);
}
DxTextRenderer::~DxTextRenderer() {

}
bool DxTextRenderer::Initialize() {
	if (thisBase_ != NULL)return false;

	winFont_.CreateFont(Font::GOTHIC, 20, true);

	thisBase_ = this;
	return true;
}
SIZE DxTextRenderer::_GetTextSize(HDC hDC, wchar_t* pText) {
	//文字コード
	int charCount = 1;
	int code = 0;

	//文字サイズ計算
	SIZE size;
	::GetTextExtentPoint32(hDC, pText, charCount, &size);
	return size;
}
shared_ptr<DxTextLine> DxTextRenderer::_GetTextInfoSub(std::wstring text, DxText* dxText, DxTextInfo* textInfo, 
	shared_ptr<DxTextLine> textLine, HDC& hDC, int& totalWidth, int& totalHeight) 
{
	DxFont& dxFont = dxText->GetFont();
	int sidePitch = dxText->GetSidePitch();
	int linePitch = dxText->GetLinePitch();
	int widthMax = dxText->GetMaxWidth();
	int heightMax = dxText->GetMaxHeight();
	int widthBorder = dxFont.GetBorderType() != DxFont::BORDER_NONE ? dxFont.GetBorderWidth() : 0;
	textLine->SetSidePitch(sidePitch);

	const std::wstring strFirstForbid = L"」、。";

	wchar_t* pText = const_cast<wchar_t*>(text.data());
	wchar_t* eText = const_cast<wchar_t*>(text.data() + text.size());
	while (true) {
		if (*pText == L'\0' || pText >= eText)break;

		//文字コード
		int charCount = 1;
		int code = *pText;

		//禁則処理
		SIZE sizeNext;
		ZeroMemory(&sizeNext, sizeof(SIZE));
		wchar_t* pNextChar = pText + charCount;
		if (pNextChar < eText) {
			//次の文字
			std::wstring strNext = L"";
			strNext.resize(1);
			memcpy(&strNext[0], pNextChar, strNext.size() * sizeof(wchar_t));

			bool bFirstForbid = strFirstForbid.find(strNext) != std::wstring::npos;
			if (bFirstForbid)
				sizeNext = _GetTextSize(hDC, pNextChar);
		}

		//文字サイズ計算
		SIZE size = _GetTextSize(hDC, pText);
		int lw = size.cx + widthBorder + sidePitch;
		int lh = size.cy;
		if (textLine->width_ + lw + sizeNext.cx >= widthMax) {
			//改行
			totalWidth = std::max(totalWidth, textLine->width_);
			totalHeight += textLine->height_ + linePitch;
			textInfo->AddTextLine(textLine);
			textLine = std::make_shared<DxTextLine>();
			textLine->SetSidePitch(sidePitch);
			continue;
		}
		if (totalHeight + size.cy > heightMax) {
			textLine = NULL;
			break;
		}
		textLine->width_ += lw;
		textLine->height_ = std::max(textLine->height_, lh);
		textLine->code_.push_back(code);

		pText += charCount;
	}
	return textLine;
}
shared_ptr<DxTextInfo> DxTextRenderer::GetTextInfo(DxText* dxText) {
	SetFont(dxText->dxFont_.GetLogFont());
	DxTextInfo* res = new DxTextInfo();
	std::wstring& text = dxText->GetText();
	DxFont& dxFont = dxText->GetFont();
	int linePitch = dxText->GetLinePitch();
	int widthMax = dxText->GetMaxWidth();
	int heightMax = dxText->GetMaxHeight();
	RECT margin = dxText->GetMargin();

	shared_ptr<Font> fontTemp;

	HDC hDC = ::GetDC(NULL);
	HFONT oldFont = (HFONT)SelectObject(hDC, winFont_.GetHandle());

	bool bEnd = false;
	int totalWidth = 0;
	int totalHeight = 0;
	int widthBorder = dxFont.GetBorderType() != DxFont::BORDER_NONE ? dxFont.GetBorderWidth() : 0;

	shared_ptr<DxTextLine> textLine(new DxTextLine());
	textLine->width_ = margin.left;

	if (dxText->IsSyntacticAnalysis()) {
		DxTextScanner scan(text);
		while (!bEnd) {
			if (!scan.HasNext()) {
				//残りを加える
				if (textLine->code_.size() > 0) {
					totalWidth = std::max(totalWidth, textLine->width_);
					totalHeight += textLine->height_;
					res->AddTextLine(textLine);
				}
				break;
			}

			DxTextToken& tok = scan.Next();
			int typeToken = tok.GetType();
			if (typeToken == DxTextToken::TK_TEXT) {
				std::wstring text = tok.GetElement();
				text = _ReplaceRenderText(text);
				if (text.size() == 0 || text == L"")continue;

				textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);
				if (textLine == NULL)bEnd = true;
			}
			else if (typeToken == DxTextScanner::TOKEN_TAG_START) {
				int indexTag = textLine->code_.size();
				tok = scan.Next();
				std::wstring element = tok.GetElement();
				if (element == DxTextScanner::TAG_NEW_LINE) {
					//改行
					if (textLine->height_ == 0) {
						//空文字の場合も空白文字で高さを計算する
						textLine = _GetTextInfoSub(L" ", dxText, res, textLine, hDC, totalWidth, totalHeight);
					}

					if (textLine != NULL) {
						totalWidth = std::max(totalWidth, textLine->width_);
						totalHeight += textLine->height_ + linePitch;
						res->AddTextLine(textLine);
					}

					textLine = std::make_shared<DxTextLine>();
				}
				else if (element == DxTextScanner::TAG_RUBY) {
					shared_ptr<DxTextTag_Ruby> tag(new DxTextTag_Ruby());
					tag->SetTagIndex(indexTag);

					int weightRuby = FW_BOLD;

					while (true) {
						tok = scan.Next();
						if (tok.GetType() == DxTextScanner::TOKEN_TAG_END)break;
						std::wstring str = tok.GetElement();
						if (str == L"rb") {
							scan.CheckType(scan.Next(), DxTextToken::TK_EQUAL);
							std::wstring text = scan.Next().GetString();
							tag->SetText(text);
						}
						else if (str == L"rt") {
							scan.CheckType(scan.Next(), DxTextToken::TK_EQUAL);
							std::wstring text = scan.Next().GetString();
							tag->SetRuby(text);
						}
						else if (str == L"wg") {
							scan.CheckType(scan.Next(), DxTextToken::TK_EQUAL);
							weightRuby = scan.Next().GetInteger();
						}
					}

					int linePos = res->GetLineCount();
					int codeCount = textLine->GetTextCodes().size();
					std::wstring text = tag->GetText();
					shared_ptr<DxTextLine> textLineRuby = textLine;
					textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);

					SIZE sizeText;
					::GetTextExtentPoint32(hDC, &text[0], text.size(), &sizeText);
					int rubyWidth = dxText->GetFontSize() / 2;
					std::wstring sRuby = tag->GetRuby();
					int rubyCount = StringUtility::CountAsciiSizeCharacter(sRuby);
					if (rubyCount > 0) {
						int rubyPitch = std::max(sizeText.cx / rubyCount - rubyWidth, 0L);
						int rubyMarginLeft = rubyPitch / 2;
						tag->SetLeftMargin(rubyMarginLeft);

						shared_ptr<DxText> dxTextRuby(new DxText());
						dxTextRuby->SetText(tag->GetRuby());
						dxTextRuby->SetFont(dxFont);
						dxTextRuby->SetPosition(dxText->GetPosition());
						dxTextRuby->SetMaxWidth(dxText->GetMaxWidth());
						dxTextRuby->SetSidePitch(rubyPitch);
						dxTextRuby->SetLinePitch(linePitch + dxText->GetFontSize() - rubyWidth);
						dxTextRuby->SetFontWeight(weightRuby);
						dxTextRuby->SetFontItalic(false);
						dxTextRuby->SetFontUnderLine(false);
						dxTextRuby->SetFontSize(rubyWidth);
						dxTextRuby->SetFontBorderWidth(dxFont.GetBorderWidth() / 2);
						tag->SetRenderText(dxTextRuby);

						int currentCodeCount = textLineRuby->GetTextCodes().size();
						if (codeCount == currentCodeCount) {
							//タグが完全に次の行に回る場合
							tag->SetTagIndex(0);
							textLine->tag_.push_back(tag);
						}
						else {
							textLineRuby->tag_.push_back(tag);
						}


					}

				}
				else if (element == DxTextScanner::TAG_FONT) {
					DxTextTag_Font* tag = new DxTextTag_Font();
					DxFont font = dxText->GetFont();
					LOGFONT logFont = font.GetLogFont();
					tag->SetTagIndex(indexTag);

					bool bClear = false;
					while (true) {
						tok = scan.Next();
						if (tok.GetType() == DxTextScanner::TOKEN_TAG_END)break;
						std::wstring str = tok.GetElement();
						if (str == L"clear") {
							bClear = true;
						}
						else if (str == L"size") {
							scan.CheckType(scan.Next(), DxTextToken::TK_EQUAL);
							int size = scan.Next().GetInteger();
							logFont.lfHeight = size;
						}
					}

					if (bClear) {
						widthBorder = dxFont.GetBorderType() != DxFont::BORDER_NONE ? dxFont.GetBorderWidth() : 0;
						fontTemp = NULL;
						oldFont = (HFONT)SelectObject(hDC, winFont_.GetHandle());
					}
					else {
						widthBorder = font.GetBorderType() != DxFont::BORDER_NONE ? font.GetBorderWidth() : 0;
						fontTemp = std::make_shared<Font>();
						fontTemp->CreateFontIndirect(logFont);
						font.SetLogFont(logFont);
						oldFont = (HFONT)SelectObject(hDC, fontTemp->GetHandle());
					}
					tag->SetFont(font);

					textLine->tag_.push_back(shared_ptr<DxTextTag_Font>(tag));
				}
				else {
					std::wstring text = DxTextScanner::TAG_START;
					text += tok.GetElement();
					while (true) {
						if (tok.GetType() == DxTextScanner::TOKEN_TAG_END)
							break;
						tok = scan.Next();
						text += tok.GetElement();
					}
					text = _ReplaceRenderText(text);
					if (text.size() == 0 || text == L"")continue;

					textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);
					if (textLine == NULL)bEnd = true;
				}
			}
		}
	}
	else {
		std::wstring& text = dxText->GetText();
		text = _ReplaceRenderText(text);
		if (text.size() > 0) {
			textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);
			res->AddTextLine(textLine);
		}
	}

	res->totalWidth_ = totalWidth + widthBorder;
	res->totalHeight_ = totalHeight + widthBorder;
	::SelectObject(hDC, oldFont);
	::ReleaseDC(NULL, hDC);

	return shared_ptr<DxTextInfo>(res);
}
std::wstring DxTextRenderer::_ReplaceRenderText(std::wstring& text) {
	text = StringUtility::ReplaceAll(text, L"\r", L"");
	text = StringUtility::ReplaceAll(text, L"\n", L"");
	text = StringUtility::ReplaceAll(text, L"\t", L"");
	text = StringUtility::ReplaceAll(text, L"&nbsp;", L" ");
	text = StringUtility::ReplaceAll(text, L"&quot;", L"\"");
	text = StringUtility::ReplaceAll(text, L"&osb;", L"[");
	text = StringUtility::ReplaceAll(text, L"&csb;", L"]");
	return text;
}

void DxTextRenderer::_CreateRenderObject(shared_ptr<DxTextRenderObject> objRender, POINT pos, DxFont& dxFont, 
	shared_ptr<DxTextLine> textLine)
{
	SetFont(dxFont.GetLogFont());
	DxCharCacheKey keyFont;
	keyFont.font_ = dxFont;
	int textHeight = textLine->GetHeight();

	int xRender = pos.x;
	int yRender = pos.y;

	int countTag = textLine->GetTagCount();
	int indexTag = 0;
	int countCode = textLine->code_.size();
	for (int iCode = 0; iCode < countCode; iCode++) {
		for (; indexTag < countTag;) {
			shared_ptr<DxTextTag> tag = textLine->GetTag(indexTag);
			int tagNo = tag->GetTagIndex();
			if (tagNo != iCode)break;
			int type = tag->GetTagType();
			if (type == DxTextTag::TYPE_FONT) {
				DxTextTag_Font* font = (DxTextTag_Font*)tag.get();
				dxFont = font->GetFont();
				keyFont.font_ = dxFont;
				winFont_.CreateFontIndirect(dxFont.GetLogFont());
				indexTag++;
			}
			else if (type == DxTextTag::TYPE_RUBY) {
				DxTextTag_Ruby* ruby = (DxTextTag_Ruby*)tag.get();
				shared_ptr<DxText> textRuby = ruby->GetRenderText();

				RECT margin;
				ZeroMemory(&margin, sizeof(RECT));
				margin.left = xRender + ruby->GetLeftMargin();
				margin.top = pos.y - textRuby->GetFontSize();
				textRuby->SetMargin(margin);

				POINT bias;
				ZeroMemory(&bias, sizeof(POINT));

				objRender->AddRenderObject(textRuby->CreateRenderObject(), bias);

				SetFont(dxFont.GetLogFont());

				indexTag++;
			}
			else break;
		}

		//文字コード
		int code = textLine->code_[iCode];

		//キャッシュに存在するか確認
		keyFont.code_ = code;
		keyFont.font_ = dxFont;

		shared_ptr<DxChar> dxChar = cache_.GetChar(keyFont);
		if (dxChar == nullptr) {
			//キャッシュにない場合、作成して追加
			dxChar = std::make_shared<DxChar>();
			dxChar->Create(code, winFont_, dxFont);
			cache_.AddChar(keyFont, dxChar);
		}

		//描画
		Sprite2D* spriteText = new Sprite2D();

		int yGap = 0;
		yRender = pos.y + yGap;
		shared_ptr<Texture> texture = dxChar->GetTexture();
		spriteText->SetTexture(texture);

		//		int objWidth = texture->GetWidth();//dxChar->GetWidth();
		//		int objHeight = texture->GetHeight();//dxChar->GetHeight();
		int objWidth = dxChar->GetWidth();
		int objHeight = dxChar->GetHeight();
		RECT_D rcDest = { (double)xRender, (double)yRender, (double)(objWidth + xRender), (double)(objHeight + yRender) };
		RECT_D rcSrc = { 0., 0., (double)objWidth, (double)objHeight };
		spriteText->SetVertex(rcSrc, rcDest, colorVertex_);
		objRender->AddRenderObject(shared_ptr<Sprite2D>(spriteText));

		//次の文字
		xRender += dxChar->GetWidth() - dxFont.GetBorderWidth() + textLine->GetSidePitch();
	}
}

shared_ptr<DxTextRenderObject> DxTextRenderer::CreateRenderObject(DxText* dxText, shared_ptr<DxTextInfo> textInfo) {
	{
		Lock lock(lock_);

		shared_ptr<DxTextRenderObject> objRender(new DxTextRenderObject());
		objRender->SetPosition(dxText->GetPosition());
		objRender->SetVertexColor(dxText->GetVertexColor());
		objRender->SetPermitCamera(dxText->IsPermitCamera());
		objRender->SetShader(dxText->GetShader());

		DxFont& dxFont = dxText->GetFont();
		int linePitch = dxText->GetLinePitch();
		int widthMax = dxText->GetMaxWidth();
		int heightMax = dxText->GetMaxHeight();
		RECT margin = dxText->GetMargin();
		int alignmentHorizontal = dxText->GetHorizontalAlignment();
		int alignmentVertical = dxText->GetVerticalAlignment();
		POINT pos;
		ZeroMemory(&pos, sizeof(POINT));
		bool bAutoIndent = textInfo->IsAutoIndent();

		switch (alignmentVertical) {
		case ALIGNMENT_TOP:
			break;
		case ALIGNMENT_CENTER:
		{
			int  cy = pos.y + heightMax / 2;
			pos.y = cy - textInfo->totalHeight_ / 2;
			break;
		}
		case ALIGNMENT_BOTTOM:
		{
			int by = pos.y + heightMax;
			pos.y = by - textInfo->totalHeight_;
			break;
		}
		}
		pos.y += margin.top;

		int heightTotal = 0;
		int countLine = textInfo->textLine_.size();
		int lineStart = textInfo->GetValidStartLine() - 1;
		int lineEnd = textInfo->GetValidEndLine() - 1;
		for (int iLine = lineStart; iLine <= lineEnd; iLine++) {
			shared_ptr<DxTextLine> textLine = textInfo->GetTextLine(iLine);
			pos.x = 0;//dxText->GetPosition().x;
			if (iLine == 0) pos.x += margin.left;
			switch (alignmentHorizontal) {
			case ALIGNMENT_LEFT:
				if (iLine >= 1 && bAutoIndent)
					pos.x = dxText->GetFontSize();
				break;
			case ALIGNMENT_CENTER:
			{
				int cx = pos.x + widthMax / 2;
				pos.x = cx - textLine->width_ / 2;
				break;
			}
			case ALIGNMENT_RIGHT:
			{
				int rx = pos.x + widthMax;
				pos.x = rx - textLine->width_;
				break;
			}
			}

			heightTotal += textLine->height_ + linePitch;
			if (heightTotal > heightMax)break;

			_CreateRenderObject(objRender, pos, dxFont, textLine);

			pos.y += textLine->height_ + linePitch;
		}
		return objRender;
	}

}

void DxTextRenderer::Render(DxText* dxText) {
	{
		Lock lock(lock_);
		shared_ptr<DxTextInfo> textInfo = GetTextInfo(dxText);
		Render(dxText, textInfo);
	}
}
void DxTextRenderer::Render(DxText* dxText, shared_ptr<DxTextInfo> textInfo) {
	{
		Lock lock(lock_);
		shared_ptr<DxTextRenderObject> objRender = CreateRenderObject(dxText, textInfo);
		objRender->Render();
	}
}
bool DxTextRenderer::AddFontFromFile(std::wstring path) {
	ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == NULL)throw gstd::wexception(StringUtility::Format(L"フォントファイルが見つかりません(%s)", path.c_str()).c_str());
	if (!reader->Open())throw gstd::wexception(StringUtility::Format(L"フォントファイルを開けません(%s)", path.c_str()).c_str());

	size_t size = reader->GetFileSize();
	ByteBuffer buf;
	buf.SetSize(size);
	reader->Read(buf.GetPointer(), size);

	DWORD  count = 0;
	HANDLE handle = ::AddFontMemResourceEx(buf.GetPointer(), size, NULL, &count);

	Logger::WriteTop(L"Font loaded: " + path);
	return handle != 0;
}

/**********************************************************
//DxText
//テキスト描画
**********************************************************/
DxText::DxText() {
	dxFont_.SetTopColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxFont_.SetBottomColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxFont_.SetBorderType(DxFont::BORDER_NONE);
	dxFont_.SetBorderWidth(0);
	dxFont_.SetBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));

	LOGFONT logFont;
	ZeroMemory(&logFont, sizeof(LOGFONT));
	logFont.lfEscapement = 0;
	logFont.lfWidth = 0;
	logFont.lfCharSet = ANSI_CHARSET;
	SetFont(logFont);
	SetFontSize(20);
	SetFontType(Font::GOTHIC);
	SetFontWeight(FW_DONTCARE);
	SetFontItalic(false);
	SetFontUnderLine(false);

	pos_.x = 0;
	pos_.y = 0;
	widthMax_ = INT_MAX;
	heightMax_ = INT_MAX;
	sidePitch_ = 0;
	linePitch_ = 4;
	alignmentHorizontal_ = ALIGNMENT_LEFT;
	alignmentVertical_ = ALIGNMENT_TOP;
	ZeroMemory(&margin_, sizeof(RECT));

	colorVertex_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	bPermitCamera_ = true;
	bSyntacticAnalysis_ = true;

	textHash_ = 0;
}
DxText::~DxText() {}
void DxText::Copy(const DxText& src) {
	dxFont_ = src.dxFont_;
	pos_ = src.pos_;
	widthMax_ = src.widthMax_;
	heightMax_ = src.heightMax_;
	sidePitch_ = src.sidePitch_;
	linePitch_ = src.linePitch_;
	margin_ = src.margin_;
	alignmentHorizontal_ = src.alignmentHorizontal_;
	alignmentVertical_ = src.alignmentVertical_;
	colorVertex_ = src.colorVertex_;
	text_ = src.text_;
}
void DxText::SetFontType(const wchar_t* type) {
	LOGFONT info = dxFont_.GetLogFont();
	lstrcpy(info.lfFaceName, type);
	info.lfCharSet = DEFAULT_CHARSET;

	for (int i = 0; i < (int)wcslen(type); i++) {
		if (!(IsCharAlphaNumeric(type[i]) || type[i] == L' ' || type[i] == L'-')) {
			info.lfCharSet = SHIFTJIS_CHARSET;
			break;
		}
	}
	SetFont(info);
}
void DxText::Render() {
	DxTextRenderer* renderer = DxTextRenderer::GetBase();
	{
		Lock lock(renderer->GetLock());
		renderer->SetVertexColor(colorVertex_);
		renderer->Render(this);
	}
}
void DxText::Render(shared_ptr<DxTextInfo> textInfo) {
	DxTextRenderer* renderer = DxTextRenderer::GetBase();
	{
		Lock lock(renderer->GetLock());
		renderer->SetVertexColor(colorVertex_);
		renderer->Render(this, textInfo);
	}
}
shared_ptr<DxTextInfo> DxText::GetTextInfo() {
	DxTextRenderer* renderer = DxTextRenderer::GetBase();
	{
		Lock lock(renderer->GetLock());
		renderer->SetVertexColor(colorVertex_);
		return renderer->GetTextInfo(this);
	}
}
shared_ptr<DxTextRenderObject> DxText::CreateRenderObject() {
	DxTextRenderer* renderer = DxTextRenderer::GetBase();
	{
		Lock lock(renderer->GetLock());
		renderer->SetVertexColor(colorVertex_);
		shared_ptr<DxTextInfo> textInfo = renderer->GetTextInfo(this);
		return renderer->CreateRenderObject(this, textInfo);
	}
}
shared_ptr<DxTextRenderObject> DxText::CreateRenderObject(shared_ptr<DxTextInfo> textInfo) {
	DxTextRenderer* renderer = DxTextRenderer::GetBase();
	{
		Lock lock(renderer->GetLock());
		renderer->SetVertexColor(colorVertex_);
		return renderer->CreateRenderObject(this, textInfo);
	}
}

/**********************************************************
//DxTextStepper
**********************************************************/
DxTextStepper::DxTextStepper() {
	posNext_ = 0;
	framePerSec_ = 60;
	countNextPerFrame_ = 60;
	countFrame_ = 0;
	Clear();
}
DxTextStepper::~DxTextStepper() {}
void DxTextStepper::Clear() {
	posNext_ = 0;
	text_ = L"";
	source_ = L"";
}
void DxTextStepper::_Next() {
	if (!HasNext())return;

	if (source_[posNext_] == DxTextScanner::TAG_START[0]) {
		text_ += source_[posNext_];
		posNext_++;
		while (true) {
			bool bBreak = (source_[posNext_] == DxTextScanner::TAG_END[0]);
			text_ += source_[posNext_];
			posNext_++;
			if (bBreak)break;
		}
	}
	else {
		text_ += source_[posNext_];
		posNext_++;
	}
}
void DxTextStepper::Next() {
	double ratioFrame = (double)countNextPerFrame_ / (double)framePerSec_;
	int lastCountFrame = (int)countFrame_;
	countFrame_ += ratioFrame;
	while (true) {
		if (countFrame_ < 1.0)break;
		countFrame_ -= 1.0;
		_Next();
	}
}
void DxTextStepper::NextSkip() {
	while (HasNext())_Next();
}
bool DxTextStepper::HasNext() {
	return posNext_ < source_.size();
}
void DxTextStepper::SetSource(std::wstring text) {
	posNext_ = 0;
	source_ = text;
	//	text_ = "";
}
