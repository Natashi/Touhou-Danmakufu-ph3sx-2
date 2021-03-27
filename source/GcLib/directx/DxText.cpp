#include "source/GcLib/pch.h"

#include "DxText.hpp"
#include "DxUtility.hpp"
#include "DirectGraphics.hpp"
#include "RenderObject.hpp"

using namespace gstd;
using namespace directx;

//*******************************************************************
//DxFont
//*******************************************************************
DxFont::DxFont() {
	ZeroMemory(&info_, sizeof(LOGFONT));
	colorTop_ = D3DCOLOR_ARGB(255, 128, 128, 255);
	colorBottom_ = D3DCOLOR_ARGB(255, 32, 32, 255);
	typeBorder_ = TextBorderType::None;
	widthBorder_ = 2;
	colorBorder_ = D3DCOLOR_ARGB(128, 255, 255, 255);
}
DxFont::~DxFont() {

}

//*******************************************************************
//DxCharGlyph
//*******************************************************************
DxCharGlyph::DxCharGlyph() {

}
DxCharGlyph::~DxCharGlyph() {}

bool DxCharGlyph::Create(UINT code, Font& winFont, DxFont* dxFont) {
	code_ = code;

	short colorTop[4];
	short colorBottom[4];
	short colorBorder[4];
	ColorAccess::ToByteArray(dxFont->GetTopColor(), colorTop);
	ColorAccess::ToByteArray(dxFont->GetBottomColor(), colorBottom);
	ColorAccess::ToByteArray(dxFont->GetBorderColor(), colorBorder);

	TextBorderType typeBorder = dxFont->GetBorderType();
	LONG widthBorder = typeBorder != TextBorderType::None ? dxFont->GetBorderWidth() : 0L;

	HDC hDC = ::GetDC(nullptr);
	HFONT oldFont = (HFONT)SelectObject(hDC, winFont.GetHandle());

	const TEXTMETRIC& tm = winFont.GetMetrics();

	UINT uFormat = GGO_GRAY2_BITMAP;	//typeBorder == DxFont::BORDER_FULL ? GGO_BITMAP : GGO_GRAY2_BITMAP;
	if (dxFont->GetLogFont().lfHeight <= 12)
		uFormat = GGO_BITMAP;

	const MAT2 mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
	DWORD size = ::GetGlyphOutline(hDC, code, uFormat, &glpMet_, 0, nullptr, &mat);

	UINT iBmp_w = uFormat != GGO_BITMAP ? glpMet_.gmBlackBoxX + (4 - (glpMet_.gmBlackBoxX % 4)) % 4 : glpMet_.gmBlackBoxX;
	UINT iBmp_h = glpMet_.gmBlackBoxY;
	LONG level = 5;		//GGO_GRAY4_BITMAP;

	size_.x = glpMet_.gmCellIncX + widthBorder * 2;
	size_.y = tm.tmHeight + widthBorder * 2;

	float iBmp_h_inv = 1 / (float)iBmp_h;
	LONG glyphOriginX = glpMet_.gmptGlyphOrigin.x;
	LONG glyphOriginY = tm.tmAscent - glpMet_.gmptGlyphOrigin.y;
	sizeMax_.x = iBmp_w + widthBorder * 2 + glyphOriginX + (tm.tmItalic ? tm.tmOverhang : 0);
	sizeMax_.y = iBmp_h + widthBorder * 2 + glyphOriginY;

	UINT widthTexture = 1;
	UINT heightTexture = 1;
	while (widthTexture < sizeMax_.x) {
		widthTexture = widthTexture << 1;
		if (widthTexture >= 0x4000u) return false;
	}
	while (heightTexture < sizeMax_.y) {
		heightTexture = heightTexture << 1;
		if (heightTexture >= 0x4000u) return false;
	}

	//if (sizeMax_.x > widthTexture) sizeMax_.x = widthTexture;
	//if (sizeMax_.y > heightTexture) sizeMax_.y = heightTexture;

	IDirect3DTexture9* pTexture = nullptr;
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	HRESULT hr = device->CreateTexture(widthTexture, heightTexture, 1, 
		D3DPOOL_DEFAULT, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, nullptr);
	if (FAILED(hr)) return false;

	//RECT lockingRect = { 0, 0, sizeMax_.x, sizeMax_.y };
	D3DLOCKED_RECT lock;
	if (FAILED(pTexture->LockRect(0, &lock, nullptr, D3DLOCK_DISCARD))) {
		ptr_release(pTexture);
		return false;
	}

	BYTE* ptr = new BYTE[size];
	::GetGlyphOutline(hDC, code, uFormat, &glpMet_, size, ptr, &mat);

	//Restore previous font handle and discard the device context
	::SelectObject(hDC, oldFont);
	::ReleaseDC(nullptr, hDC);

	FillMemory(lock.pBits, lock.Pitch * sizeMax_.y, 0);

	if (size > 0) {
		auto _GenRow = [&](LONG iy) {
			LONG yBmp = iy - glyphOriginY - widthBorder;

			float iRateY = yBmp * iBmp_h_inv;
			short colorR = Math::Lerp::Linear(colorTop[1], colorBottom[1], iRateY);
			short colorG = Math::Lerp::Linear(colorTop[2], colorBottom[2], iRateY);
			short colorB = Math::Lerp::Linear(colorTop[3], colorBottom[3], iRateY);

			for (LONG ix = 0; ix < sizeMax_.x; ++ix) {
				LONG xBmp = ix - glyphOriginX - widthBorder;

				short alpha = 255;
				if (uFormat != GGO_BITMAP) {
					LONG posBmp = xBmp + iBmp_w * yBmp;
					alpha = xBmp >= 0 && xBmp < iBmp_w&& yBmp >= 0 && yBmp < iBmp_h ?
						(255 * ptr[posBmp]) / (level - 1) : 0;
				}
				else {
					if (xBmp >= 0 && xBmp < iBmp_w && yBmp >= 0 && yBmp < iBmp_h) {
						UINT lineByte = (1 + (iBmp_w / 32)) * 4;
						LONG posBmp = xBmp / 8 + lineByte * yBmp;
						alpha = ((ptr[posBmp] >> (7 - xBmp % 8)) & 0b1) ? 255 : 0;
					}
					else alpha = 0;
				}

				D3DCOLOR color = 0x00000000;

				if (typeBorder != TextBorderType::None && alpha != 255) {
					//Generate borders
					if (alpha == 0) {
						size_t count = 0;
						LONG antiDist = 0;
						LONG bx = typeBorder == TextBorderType::Full ? xBmp + widthBorder + antiDist : xBmp + 1;
						LONG by = typeBorder == TextBorderType::Full ? yBmp + widthBorder + antiDist : yBmp + 1;
						LONG minAlphaEnableDist = 255 * 255;
						for (LONG ax = xBmp - widthBorder - antiDist; ax <= bx; ++ax) {
							for (LONG ay = yBmp - widthBorder - antiDist; ay <= by; ++ay) {
								LONG dist = abs(ax - xBmp) + abs(ay - yBmp);
								if (dist > widthBorder + antiDist || dist == 0) continue;

								LONG tAlpha = 255;
								if (uFormat != GGO_BITMAP) {
									tAlpha = ax >= 0 && ax < iBmp_w&& ay >= 0 && ay < iBmp_h ?
										(255 * ptr[ax + iBmp_w * ay]) / (level - 1) : 0;
								}
								else {
									if (ax >= 0 && ax < iBmp_w && ay >= 0 && ay < iBmp_h) {
										UINT lineByte = (1 + (iBmp_w / 32)) * 4;
										LONG tPos = ax / 8 + lineByte * ay;
										tAlpha = ((ptr[tPos] >> (7 - ax % 8)) & 0b1) ? 255 : 0;
									}
									else tAlpha = 0;
								}

								if (tAlpha > 0)
									minAlphaEnableDist = std::min(minAlphaEnableDist, dist);

								LONG tCount = tAlpha / dist * 2L;
								if (typeBorder == TextBorderType::Shadow && (ax >= xBmp || ay >= yBmp))
									tCount /= 2L;
								count += tCount;
							}
						}

						int destAlpha = 0;
						if (minAlphaEnableDist < widthBorder)
							destAlpha = 255;
						else if (minAlphaEnableDist == widthBorder)
							destAlpha = count;
						else destAlpha = 0;
						//color = ColorAccess::SetColorA(color, ColorAccess::GetColorA(colorBorder)*count/255);
						byte c_a = ColorAccess::ClampColorRet(colorBorder[0] * destAlpha / 255);
						color = D3DCOLOR_ARGB(c_a, colorBorder[1], colorBorder[2], colorBorder[3]);
					}
					else {
						short oAlpha = alpha + 64;
						if (alpha > 255) alpha = 255;

						size_t count = 0;
						LONG cDist = 1;
						LONG bx = typeBorder == TextBorderType::Full ? xBmp + cDist : xBmp + 1;
						LONG by = typeBorder == TextBorderType::Full ? yBmp + cDist : yBmp + 1;
						for (LONG ax = xBmp - cDist; ax <= bx; ++ax) {
							for (LONG ay = yBmp - cDist; ay <= by; ++ay) {
								LONG tAlpha = 255;
								if (uFormat != GGO_BITMAP) {
									tAlpha = tAlpha = ax >= 0 && ax < iBmp_w&& ay >= 0 && ay < iBmp_h ?
										(255 * ptr[ax + iBmp_w * ay]) / (level - 1) : 0;
								}
								else {
									if (ax >= 0 && ax < iBmp_w && ay >= 0 && ay < iBmp_h) {
										UINT lineByte = (1 + (iBmp_w / 32)) * 4;
										LONG tPos = ax / 8 + lineByte * ay;
										tAlpha = ((ptr[tPos] >> (7 - ax % 8)) & 0b1) ? 255 : 0;
									}
									else tAlpha = 0;
								}

								if (tAlpha > 0) ++count;
							}
						}
						if (count >= 2) oAlpha = alpha;

						{
							LONG bAlpha = 255 - oAlpha;
							short c_r = colorR * oAlpha / 255 + colorBorder[1] * bAlpha / 255;
							short c_g = colorG * oAlpha / 255 + colorBorder[2] * bAlpha / 255;
							short c_b = colorB * oAlpha / 255 + colorBorder[3] * bAlpha / 255;
							__m128i c = Vectorize::Set(0, c_r, c_g, c_b);
							color = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));
							color = (color & 0x00ffffff) | 0xff000000;
						}
					}
				}
				else {
					if (typeBorder != TextBorderType::None && alpha > 0) alpha = 255;
					color = (D3DCOLOR_XRGB(colorR, colorG, colorB) & 0x00ffffff) | (alpha << 24);
				}

				memcpy((BYTE*)lock.pBits + lock.Pitch * iy + 4 * ix, &color, sizeof(D3DCOLOR));
			}
		};

		ParallelFor(sizeMax_.y, _GenRow);
	}
	pTexture->UnlockRect(0);

	delete[] ptr;

	texture_ = std::make_shared<Texture>();
	texture_->SetTexture(pTexture);

	return true;
}

//*******************************************************************
//DxCharCache
//*******************************************************************
DxCharCache::DxCharCache() {
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
	for (auto itr = mapPriKeyLast.begin(); itr != mapPriKeyLast.end(); itr++) {
		DxCharCacheKey& key = itr->second;
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
shared_ptr<DxCharGlyph> DxCharCache::GetChar(DxCharCacheKey& key) {
	shared_ptr<DxCharGlyph> res;

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

void DxCharCache::AddChar(DxCharCacheKey& key, shared_ptr<DxCharGlyph> value) {
	//bool bExist = mapCache_.find(key) != mapCache_.end();
	//if (bExist) return;
	mapCache_[key] = value;

	if (mapCache_.size() >= MAX) {
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


//*******************************************************************
//DxTextScanner
//*******************************************************************
static const DxTextToken::Type TOKEN_TAG_START = DxTextToken::Type::TK_OPENB;
static const DxTextToken::Type TOKEN_TAG_END = DxTextToken::Type::TK_CLOSEB;
static const std::wstring TAG_START = L"[";
static const std::wstring TAG_END = L"]";
static const std::wstring TAG_NEW_LINE = L"r";
static const std::wstring TAG_RUBY = L"ruby";
static const std::wstring TAG_FONT = L"font";
static const std::wstring TAG_FONT2 = L"f";
static const wchar_t CHAR_TAG_START = L'[';
static const wchar_t CHAR_TAG_END = L']';

DxTextScanner::DxTextScanner(wchar_t* str, size_t charCount) {
	std::vector<wchar_t> buf;
	buf.resize(charCount);
	if (buf.size() != 0) {
		memcpy(&buf[0], str, charCount * sizeof(wchar_t));
	}

	buf.push_back(L'\0');
	this->DxTextScanner::DxTextScanner(buf);
}
DxTextScanner::DxTextScanner(const std::wstring& str) {
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
		std::wstring log = StringUtility::Format(L"_NextChar(Text): Unexpected end-of-file while parsing text. -> %s", 
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
		if (bTagScan_) _SkipSpace();

		wchar_t ch = *pointer_;

		if (ch == L'/') {//コメントアウト処理
			std::vector<wchar_t>::iterator tPos = pointer_;
			ch = _NextChar();
			if (ch == L'/') {// "//"
				while (true) {
					ch = _NextChar();
					if (ch == L'\r' || ch == L'\n') break;
				}
			}
			else if (ch == L'*') {// "/*"-"*/"
				while (true) {
					ch = _NextChar();
					if (ch == L'*') {
						ch = _NextChar();
						if (ch == L'/') break;
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
		if (posStart == pointer_) break;
	}
}
void DxTextScanner::_SkipSpace() {
	wchar_t ch = *pointer_;
	while (true) {
		if (ch != L' ' && ch != L'\t') break;
		ch = _NextChar();
	}
}
void DxTextScanner::_RaiseError(const std::wstring& str) {
	throw gstd::wexception(str);
}
bool DxTextScanner::_IsTextStartSign() {
	if (bTagScan_) return false;

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
		if (ch == L'/' || ch == L'*') res = false;
	}
	else {
		bool bGreater = ch == CHAR_TAG_END;
		if (ch == CHAR_TAG_END) {
			_RaiseError(L"DxTextScanner: Unexpected tag end token (]) in text.");
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
		token_ = DxTextToken(DxTextToken::Type::TK_EOF, L"\0");
		return token_;
	}

	DxTextToken::Type type = DxTextToken::Type::TK_UNKNOWN;
	std::vector<wchar_t>::iterator posStart = pointer_;//先頭を保存

	if (_IsTextStartSign()) {
		ch = *pointer_;

		posStart = pointer_;
		while (_IsTextScan()) {

		}

		ch = *pointer_;

		type = DxTextToken::Type::TK_TEXT;
		std::wstring text = std::wstring(posStart, pointer_);
		text = StringUtility::ReplaceAll(text, L'\\', L'\0');
		token_ = DxTextToken(type, text);
	}
	else {
		switch (ch) {
		case L'\0': type = DxTextToken::Type::TK_EOF; break;//終端
		case L',': _NextChar(); type = DxTextToken::Type::TK_COMMA;  break;
		case L'=': _NextChar(); type = DxTextToken::Type::TK_EQUAL;  break;
		case L'(': _NextChar(); type = DxTextToken::Type::TK_OPENP; break;
		case L')': _NextChar(); type = DxTextToken::Type::TK_CLOSEP; break;
		case L'[': _NextChar(); type = DxTextToken::Type::TK_OPENB; break;
		case L']': _NextChar(); type = DxTextToken::Type::TK_CLOSEB; break;
		case L'{': _NextChar(); type = DxTextToken::Type::TK_OPENC; break;
		case L'}': _NextChar(); type = DxTextToken::Type::TK_CLOSEC; break;
		case L'*': _NextChar(); type = DxTextToken::Type::TK_ASTERISK; break;
		case L'/': _NextChar(); type = DxTextToken::Type::TK_SLASH; break;
		case L':': _NextChar(); type = DxTextToken::Type::TK_COLON; break;
		case L';': _NextChar(); type = DxTextToken::Type::TK_SEMICOLON; break;
		case L'~': _NextChar(); type = DxTextToken::Type::TK_TILDE; break;
		case L'!': _NextChar(); type = DxTextToken::Type::TK_EXCLAMATION; break;
		case L'#': _NextChar(); type = DxTextToken::Type::TK_SHARP; break;
		case L'<': _NextChar(); type = DxTextToken::Type::TK_LESS; break;
		case L'>': _NextChar(); type = DxTextToken::Type::TK_GREATER; break;

		case L'\"':
		{
			ch = _NextChar();	//1つ進めて
			
			while (true) {
				ch = _NextChar();
				if (ch == L'\\') ch = _NextChar();
				else if (ch == L'\"') break;
			}

			if (ch == L'\"')	//ダブルクオーテーションだったら1つ進める
				_NextChar();
			else 
				_RaiseError(L"Next(Text): Unexpected end-of-file while parsing string.");
			type = DxTextToken::Type::TK_STRING;
			break;
		}

		case L'\r':
		case L'\n'://改行
			//改行がいつまでも続くようなのも1つの改行として扱う
			while (ch == L'\r' || ch == L'\n') ch = _NextChar();
			type = DxTextToken::Type::TK_NEWLINE;
			break;

		case L'+':
		case L'-':
		{
			if (ch == L'+') {
				ch = _NextChar(); type = DxTextToken::Type::TK_PLUS;

			}
			else if (ch == L'-') {
				ch = _NextChar(); type = DxTextToken::Type::TK_MINUS;
			}

			if (!iswdigit(ch)) break;//次が数字でないなら抜ける
		}


		default:
		{
			if (iswdigit(ch)) {
				//整数か実数
				while (iswdigit(ch))ch = _NextChar();//数字だけの間ポインタを進める
				type = DxTextToken::Type::TK_INT;
				if (ch == L'.') {
					//実数か整数かを調べる。小数点があったら実数
					ch = _NextChar();
					while (iswdigit(ch))ch = _NextChar();//数字だけの間ポインタを進める
					type = DxTextToken::Type::TK_REAL;
				}

				if (ch == L'E' || ch == L'e') {
					//1E-5みたいなケース
					std::vector<wchar_t>::iterator pos = pointer_;
					ch = _NextChar();
					while (iswdigit(ch) || ch == L'-')ch = _NextChar();//数字だけの間ポインタを進める
					type = DxTextToken::Type::TK_REAL;
				}

			}
			else if (iswalpha(ch) || ch == L'_') {
				//たぶん識別子
				while (iswalpha(ch) || iswdigit(ch) || ch == L'_')ch = _NextChar();//たぶん識別子な間ポインタを進める
				type = DxTextToken::Type::TK_ID;
			}
			else {
				_NextChar();
				type = DxTextToken::Type::TK_UNKNOWN;
			}
			break;
		}
		}
		if (type == TOKEN_TAG_START) bTagScan_ = true;
		else if (type == TOKEN_TAG_END) bTagScan_ = false;

		if (type == DxTextToken::Type::TK_STRING) {
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
	return pointer_ != buffer_.end() && *pointer_ != L'\0' && token_.GetType() != DxTextToken::Type::TK_EOF;
}
void DxTextScanner::CheckType(DxTextToken& tok, int type) {
	if (tok.type_ != type) {
		std::wstring str = StringUtility::Format(L"CheckType error[%s]:", tok.element_.c_str());
		_RaiseError(str);
	}
}
void DxTextScanner::CheckIdentifer(DxTextToken& tok, const std::wstring& id) {
	if (tok.type_ != DxTextToken::Type::TK_ID || tok.GetIdentifier() != id) {
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
		if (pbuf >= ebuf) break;
		else if (*pbuf == L'\n') line++;

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
	if (buffer_.size() == 0) return 0;
	wchar_t* pos = (wchar_t*)&*pointer_;
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

//*******************************************************************
//DxTextRenderer
//テキスト描画エンジン
//*******************************************************************
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
void DxTextRenderObject::Render(const D3DXVECTOR2& angX, const D3DXVECTOR2& angY, const D3DXVECTOR2& angZ) {
	D3DXVECTOR2 position = D3DXVECTOR2(position_.x, position_.y);

	auto camera = DirectGraphics::GetBase()->GetCamera2D();
	bool bCamera = camera->IsEnable() && bPermitCamera_;

	D3DXVECTOR2 center = center_;
	if (bAutoCenter_) {
		DxRect<int> rect;

		for (auto itr = listData_.begin(); itr != listData_.end(); ++itr) {
			ObjectData& obj = *itr;
			DxRect<double> rcDest = obj.sprite->GetDestinationRect();
			rect.left = std::min(rect.left, (int)rcDest.left);
			rect.top = std::min(rect.top, (int)rcDest.top);
			rect.right = std::max(rect.right, (int)rcDest.right);
			rect.bottom = std::max(rect.bottom, (int)rcDest.bottom);
		}

		center.x = (rect.right + rect.left) / 2L;
		center.y = (rect.bottom + rect.top) / 2L;
	}

	for (auto itr = listData_.begin(); itr != listData_.end(); ++itr) {
		ObjectData& obj = *itr;

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
void DxTextRenderObject::AddRenderObject(shared_ptr<DxTextRenderObject> obj, const POINT& bias) {
	for (auto itr = obj->listData_.begin(); itr != obj->listData_.end(); ++itr) {
		itr->bias = bias;
		listData_.push_back(*itr);
	}
}

//DxTextRenderer
DxTextRenderer* DxTextRenderer::thisBase_ = nullptr;
DxTextRenderer::DxTextRenderer() {
	colorVertex_ = D3DCOLOR_ARGB(255, 255, 255, 255);
}
DxTextRenderer::~DxTextRenderer() {

}
bool DxTextRenderer::Initialize() {
	if (thisBase_) return false;

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

shared_ptr<DxTextLine> DxTextRenderer::_GetTextInfoSub(const std::wstring& text, DxText* dxText, DxTextInfo* textInfo,
	shared_ptr<DxTextLine> textLine, HDC& hDC, LONG& totalWidth, LONG& totalHeight)
{
	DxFont& dxFont = dxText->GetFont();
	float sidePitch = dxText->GetSidePitch();
	float linePitch = dxText->GetLinePitch();
	LONG widthMax = dxText->GetMaxWidth();
	LONG heightMax = dxText->GetMaxHeight();
	LONG widthBorder = dxFont.GetBorderType() != TextBorderType::None ? dxFont.GetBorderWidth() : 0L;
	textLine->SetSidePitch(sidePitch);

	if (widthMax < dxText->GetFontSize())
		return nullptr;

	const std::wstring strFirstForbid = L"」、。";

	wchar_t* pText = const_cast<wchar_t*>(text.data());
	wchar_t* eText = const_cast<wchar_t*>(text.data() + text.size());
	while (true) {
		if (*pText == L'\0' || pText >= eText) break;

		//文字コード
		int charCount = 1;
		UINT code = *pText;

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
		LONG lw = size.cx + widthBorder + sidePitch;
		LONG lh = size.cy;
		if (totalHeight + size.cy > heightMax) {
			textLine = nullptr;
			break;
		}
		if (textLine->width_ + lw + sizeNext.cx >= widthMax) {
			//改行
			totalWidth = std::max(totalWidth, textLine->width_);
			totalHeight += textLine->height_ + linePitch;
			textInfo->AddTextLine(textLine);
			textLine = std::make_shared<DxTextLine>();
			textLine->SetSidePitch(sidePitch);
			continue;
		}
		
		textLine->width_ += lw;
		textLine->height_ = std::max(textLine->height_, lh);
		textLine->code_.push_back(code);

		pText += charCount;
	}
	return textLine;
}

std::vector<std::wstring> GetScannerStringArgumentList(DxTextScanner& scan, std::vector<wchar_t>::iterator beg) {
	std::vector<std::wstring> list;

	scan.SetCurrentPointer(beg);
	DxTextToken& tok = scan.Next();
	if (tok.GetType() == DxTextToken::Type::TK_OPENP) {
		while (true) {
			tok = scan.Next();
			DxTextToken::Type type = tok.GetType();
			if (type == DxTextToken::Type::TK_CLOSEP) break;
			else if (type != DxTextToken::Type::TK_COMMA) {
				std::wstring& str = tok.GetElement();
				list.push_back(str);
			}
		}
	}
	else {
		list.push_back(tok.GetElement());
	}

	return list;
}
shared_ptr<DxTextInfo> DxTextRenderer::GetTextInfo(DxText* dxText) {
	SetFont(dxText->dxFont_.GetLogFont());
	DxTextInfo* res = new DxTextInfo();
	const std::wstring& text = dxText->GetText();
	DxFont& dxFont = dxText->GetFont();
	LONG linePitch = dxText->GetLinePitch();
	LONG widthMax = dxText->GetMaxWidth();
	LONG heightMax = dxText->GetMaxHeight();
	DxRect<LONG>& margin = dxText->GetMargin();

	shared_ptr<Font> fontTemp;

	HDC hDC = ::GetDC(nullptr);
	HFONT oldFont = (HFONT)SelectObject(hDC, winFont_.GetHandle());

	bool bEnd = false;
	LONG totalWidth = 0;
	LONG totalHeight = 0;
	LONG widthBorder = dxFont.GetBorderType() != TextBorderType::None ? dxFont.GetBorderWidth() : 0L;

	struct TempFontData {
		D3DCOLOR colorTop;
		D3DCOLOR colorBottom;
		D3DCOLOR colorBorder;
		bool italic;
		bool underl;
		bool strike;
	};

	const TempFontData orgFontData = {
		dxFont.GetTopColor(),
		dxFont.GetBottomColor(),
		dxFont.GetBorderColor(),
		(bool)dxFont.GetLogFont().lfItalic,
		(bool)dxFont.GetLogFont().lfUnderline,
		(bool)dxFont.GetLogFont().lfStrikeOut
	};
	TempFontData curFontData = orgFontData;

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
			DxTextToken::Type typeToken = tok.GetType();
			if (typeToken == DxTextToken::Type::TK_TEXT) {
				std::wstring text = tok.GetElement();
				text = _ReplaceRenderText(text);
				if (text.size() == 0 || text == L"") continue;

				textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);
				if (textLine == nullptr) bEnd = true;
			}
			else if (typeToken == TOKEN_TAG_START) {
				size_t indexTag = textLine->code_.size();
				tok = scan.Next();
				const std::wstring& element = tok.GetElement();
				if (element == TAG_NEW_LINE) {
					//改行
					if (textLine->height_ == 0) {
						//空文字の場合も空白文字で高さを計算する
						textLine = _GetTextInfoSub(L" ", dxText, res, textLine, hDC, totalWidth, totalHeight);
					}

					if (textLine) {
						totalWidth = std::max(totalWidth, textLine->width_);
						totalHeight += textLine->height_ + linePitch;
						res->AddTextLine(textLine);
					}

					textLine = std::make_shared<DxTextLine>();
				}
				else if (element == TAG_RUBY) {
					struct Data {
						shared_ptr<DxTextTag_Ruby> tag;
						LONG sizeOff = 0;
						LONG weightRuby = FW_BOLD;
						LONG leftOff = 0;
						LONG pitchOff = 0;
					} data;

					data.tag.reset(new DxTextTag_Ruby());
					data.tag->SetTagIndex(indexTag);

#define LAMBDA_SET(m, f) [](Data* i, DxTextScanner& sc) { i->m = sc.Next().f(); }
					//Do NOT use [&] lambdas
					static const std::unordered_map<std::wstring, std::function<void(Data*, DxTextScanner&)>> mapFunc = {
						{ L"rb", [&](Data* i, DxTextScanner& sc) { i->tag->SetText(sc.Next().GetString()); } },
						{ L"rt", [&](Data* i, DxTextScanner& sc) { i->tag->SetRuby(sc.Next().GetString()); } },
						{ L"size", LAMBDA_SET(sizeOff, GetInteger) },
						{ L"sz", LAMBDA_SET(sizeOff, GetInteger) },
						{ L"wg", LAMBDA_SET(weightRuby, GetInteger) },
						{ L"ox", LAMBDA_SET(leftOff, GetInteger) },
						{ L"op", LAMBDA_SET(pitchOff, GetInteger) },
					};
#undef LAMBDA_SET

					while (true) {
						tok = scan.Next();
						if (tok.GetType() == TOKEN_TAG_END) break;
						const std::wstring& str = tok.GetElement();

						auto itrFind = mapFunc.find(str);
						if (itrFind != mapFunc.end()) {
							scan.CheckType(scan.Next(), DxTextToken::Type::TK_EQUAL);
							itrFind->second(&data, scan);
						}
					}

					size_t linePos = res->GetLineCount();
					size_t codeCount = textLine->GetTextCodes().size();
					const std::wstring& text = data.tag->GetText();
					shared_ptr<DxTextLine> textLineRuby = textLine;
					textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);

					SIZE sizeTextBase;
					::GetTextExtentPoint32(hDC, &text[0], text.size(), &sizeTextBase);

					LONG rubyFontWidth = dxText->GetFontSize() / 2L + data.sizeOff;
					size_t rubyCount = StringUtility::CountAsciiSizeCharacter(data.tag->GetRuby());
					if (rubyCount > 0) {
						LONG rubySpace = std::max(sizeTextBase.cx - rubyFontWidth / 2L, 0L);
						LONG rubyGap = rubySpace / std::max((LONG)rubyCount, 1L) - rubyFontWidth / 2L;
						LONG rubyPitch = std::clamp(rubyGap, 0L, (LONG)(rubyFontWidth * 1.2f));

						data.tag->SetLeftMargin(data.leftOff);

						shared_ptr<DxText> dxTextRuby(new DxText());
						dxTextRuby->SetText(data.tag->GetRuby());
						dxTextRuby->SetFont(dxFont);
						dxTextRuby->SetPosition(dxText->GetPosition());
						dxTextRuby->SetMaxWidth(dxText->GetMaxWidth());
						dxTextRuby->SetSidePitch(rubyPitch + data.pitchOff);
						dxTextRuby->SetLinePitch(linePitch + dxText->GetFontSize() - rubyFontWidth);
						dxTextRuby->SetFontWeight(data.weightRuby);
						dxTextRuby->SetFontItalic(false);
						dxTextRuby->SetFontUnderLine(false);
						dxTextRuby->SetFontSize(rubyFontWidth);
						dxTextRuby->SetFontBorderWidth(dxFont.GetBorderWidth() / 2);
						data.tag->SetRenderText(dxTextRuby);

						size_t currentCodeCount = textLineRuby->GetTextCodes().size();
						if (codeCount == currentCodeCount) {
							//タグが完全に次の行に回る場合
							data.tag->SetTagIndex(0);
							textLine->tag_.push_back(data.tag);
						}
						else {
							textLineRuby->tag_.push_back(data.tag);
						}
					}
				}
				else if (element == TAG_FONT || element == TAG_FONT2) {
					DxFont font = dxText->GetFont();
					LOGFONT& logFont = font.GetLogFont();

					struct Data {
						shared_ptr<DxTextTag_Font> tag;
						bool bClear = false;
						TempFontData* fontData = nullptr;
						LOGFONT* logFont = nullptr;
					} data;
					data.tag.reset(new DxTextTag_Font());
					data.tag->SetTagIndex(indexTag);
					data.fontData = &curFontData;
					data.logFont = &logFont;

					//--------------------------------------------------------------

					auto funcClear = [](Data* i, DxTextScanner&) { i->bClear = true; };

#define CHK_EQ scan.CheckType(scan.Next(), DxTextToken::Type::TK_EQUAL); \
					auto pointerBefore = scan.GetCurrentPointer(); \
					DxTextToken& arg = scan.Next();
#define LAMBDA_SET_I(m) [](Data* i, DxTextScanner& scan) { CHK_EQ; i->m = arg.GetInteger(); }
#define LAMBDA_SET_B(m) [](Data* i, DxTextScanner& scan) { CHK_EQ; i->m = arg.GetBoolean(); }
#define LAMBDA_SET_COLOR_S(mask, shift, dst) [](Data* i, DxTextScanner& scan) { \
						CHK_EQ; \
						byte c = ColorAccess::ClampColorRet(arg.GetInteger()); \
						i->dst = ((i->dst) & (DWORD)(mask)) | (c << (byte)(shift)); \
					}
#define LAMBDA_SET_COLOR_PK(dst) [](Data* i, DxTextScanner& scan) { \
						CHK_EQ; \
						scan.SetCurrentPointer(pointerBefore); \
						std::vector<std::wstring> list = GetScannerStringArgumentList(scan, pointerBefore); \
						if (list.size() >= 3) { \
							byte r = ColorAccess::ClampColorRet(StringUtility::ToInteger(list[0])); \
							byte g = ColorAccess::ClampColorRet(StringUtility::ToInteger(list[1])); \
							byte b = ColorAccess::ClampColorRet(StringUtility::ToInteger(list[2])); \
							i->dst = ((i->dst) & 0xff000000) | (D3DCOLOR_XRGB(r, g, b) & 0x00ffffff); \
						} \
					}

					//Do NOT use [&] lambdas
					static const std::unordered_map<std::wstring, std::function<void(Data*, DxTextScanner&)>> mapFunc = {
						{ L"r", funcClear },
						{ L"rs", funcClear },
						{ L"reset", funcClear },
						{ L"c", funcClear },
						{ L"clr", funcClear },
						{ L"clear", funcClear },

						{ L"sz", LAMBDA_SET_I(logFont->lfHeight) },
						{ L"size", LAMBDA_SET_I(logFont->lfHeight) },
						{ L"wg", LAMBDA_SET_I(logFont->lfWeight) },
						{ L"it", LAMBDA_SET_B(fontData->italic) },
						{ L"un", LAMBDA_SET_B(fontData->underl) },		//doesn't work
						{ L"st", LAMBDA_SET_B(fontData->strike) },		//doesn't work
						{ L"ox", LAMBDA_SET_I(tag->GetOffset().x) },
						{ L"oy", LAMBDA_SET_I(tag->GetOffset().y) },

						{ L"br", LAMBDA_SET_COLOR_S(0xff00ffff, 16, fontData->colorBottom) },
						{ L"bg", LAMBDA_SET_COLOR_S(0xffff00ff, 8, fontData->colorBottom) },
						{ L"bb", LAMBDA_SET_COLOR_S(0xffffff00, 0, fontData->colorBottom) },
						{ L"tr", LAMBDA_SET_COLOR_S(0xff00ffff, 16, fontData->colorTop) },
						{ L"tg", LAMBDA_SET_COLOR_S(0xffff00ff, 8, fontData->colorTop) },
						{ L"tb", LAMBDA_SET_COLOR_S(0xffffff00, 0, fontData->colorTop) },
						{ L"or", LAMBDA_SET_COLOR_S(0xff00ffff, 16, fontData->colorBorder) },
						{ L"og", LAMBDA_SET_COLOR_S(0xffff00ff, 8, fontData->colorBorder) },
						{ L"ob", LAMBDA_SET_COLOR_S(0xffffff00, 0, fontData->colorBorder) },

						{ L"bc", LAMBDA_SET_COLOR_PK(fontData->colorBottom) },
						{ L"tc", LAMBDA_SET_COLOR_PK(fontData->colorTop) },
						{ L"oc", LAMBDA_SET_COLOR_PK(fontData->colorBorder) },
					};

#undef CHK_EQ
#undef LAMBDA_SET_I
#undef LAMBDA_SET_B
#undef LAMBDA_SET_COLOR_S
#undef LAMBDA_SET_COLOR_PK

					//--------------------------------------------------------------
					
					while (true) {
						tok = scan.Next();
						if (tok.GetType() == TOKEN_TAG_END) break;
						//else if (tok.GetType() == DxTextToken::Type::TK_COMMA) continue;
						std::wstring command = tok.GetElement();

						auto itrFind = mapFunc.find(command);
						if (itrFind != mapFunc.end())
							itrFind->second(&data, scan);
					}

					if (data.bClear) {
						widthBorder = dxFont.GetBorderType() != TextBorderType::None ? dxFont.GetBorderWidth() : 0L;

						fontTemp = nullptr;
						SelectObject(hDC, oldFont);

						curFontData = orgFontData;
					}
					else {
						widthBorder = font.GetBorderType() != TextBorderType::None ? font.GetBorderWidth() : 0L;
						fontTemp = std::make_shared<Font>();
						fontTemp->CreateFontIndirect(logFont);
						SelectObject(hDC, fontTemp->GetHandle());
					}

					font.SetBottomColor(curFontData.colorBottom);
					font.SetTopColor(curFontData.colorTop);
					font.SetBorderColor(curFontData.colorBorder);
					logFont.lfItalic = curFontData.italic;
					logFont.lfUnderline = curFontData.underl;
					logFont.lfStrikeOut = curFontData.strike;

					data.tag->SetFont(font);
					textLine->tag_.push_back(data.tag);
				}
				else {
					std::wstring text = TAG_START;
					text += tok.GetElement();
					while (true) {
						if (tok.GetType() == TOKEN_TAG_END)
							break;
						tok = scan.Next();
						text += tok.GetElement();
					}
					text = _ReplaceRenderText(text);
					if (text.size() == 0 || text == L"") continue;

					textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);
					if (textLine == nullptr) bEnd = true;
				}
			}
		}
	}
	else {
		std::wstring text = dxText->GetText();
		text = _ReplaceRenderText(text);
		if (text.size() > 0) {
			textLine = _GetTextInfoSub(text, dxText, res, textLine, hDC, totalWidth, totalHeight);
			res->AddTextLine(textLine);
		}
	}

	res->totalWidth_ = totalWidth + widthBorder;
	res->totalHeight_ = totalHeight + widthBorder;
	SelectObject(hDC, oldFont);
	ReleaseDC(nullptr, hDC);

	return shared_ptr<DxTextInfo>(res);
}
std::wstring DxTextRenderer::_ReplaceRenderText(std::wstring text) {
	text = StringUtility::ReplaceAll(text, L'\r', L'\0');
	text = StringUtility::ReplaceAll(text, L'\n', L'\0');
	text = StringUtility::ReplaceAll(text, L'\t', L'\0');
	text = StringUtility::ReplaceAll(text, L"&nbsp;", L" ");
	text = StringUtility::ReplaceAll(text, L"&quot;", L"\"");
	text = StringUtility::ReplaceAll(text, L"&osb;", L"[");
	text = StringUtility::ReplaceAll(text, L"&csb;", L"]");
	return text;
}

void DxTextRenderer::_CreateRenderObject(shared_ptr<DxTextRenderObject> objRender, const POINT& pos, DxFont dxFont,
	shared_ptr<DxTextLine> textLine)
{
	SetFont(dxFont.GetLogFont());
	DxCharCacheKey keyFont;
	keyFont.font_ = dxFont;
	LONG textHeight = textLine->GetHeight();

	LONG xRender = pos.x;
	LONG yRender = pos.y;
	LONG xOffset = 0;
	LONG yOffset = 0;

	size_t countTag = textLine->GetTagCount();
	size_t indexTag = 0;
	size_t countCode = textLine->code_.size();
	for (size_t iCode = 0; iCode < countCode; ++iCode) {
		for (; indexTag < countTag;) {
			shared_ptr<DxTextTag> tag = textLine->GetTag(indexTag);
			size_t tagNo = tag->GetTagIndex();
			if (tagNo != iCode) break;

			TextTagType type = tag->GetTagType();
			if (type == TextTagType::Font) {
				DxTextTag_Font* font = (DxTextTag_Font*)tag.get();
				dxFont = font->GetFont();
				keyFont.font_ = dxFont;
				xOffset = font->GetOffset().x;
				yOffset = font->GetOffset().y;
				winFont_.CreateFontIndirect(dxFont.GetLogFont());
				indexTag++;
			}
			else if (type == TextTagType::Ruby) {
				DxTextTag_Ruby* ruby = (DxTextTag_Ruby*)tag.get();
				shared_ptr<DxText> textRuby = ruby->GetRenderText();

				DxRect<LONG> margin;
				margin.left = xRender + ruby->GetLeftMargin();
				margin.top = pos.y - textRuby->GetFontSize();
				textRuby->SetMargin(margin);

				POINT bias = { 0, 0 };

				objRender->AddRenderObject(textRuby->CreateRenderObject(), bias);

				SetFont(dxFont.GetLogFont());

				indexTag++;
			}
			else break;
		}

		//文字コード
		UINT code = textLine->code_[iCode];

		//キャッシュに存在するか確認
		keyFont.code_ = code;
		keyFont.font_ = dxFont;

		shared_ptr<DxCharGlyph> dxChar = cache_.GetChar(keyFont);
		if (dxChar == nullptr) {
			//キャッシュにない場合、作成して追加
			dxChar = std::make_shared<DxCharGlyph>();
			dxChar->Create(code, winFont_, &dxFont);
			cache_.AddChar(keyFont, dxChar);
		}

		//描画
		shared_ptr<Sprite2D> spriteText(new Sprite2D());

		LONG yGap = 0L;
		yRender = pos.y + yGap;
		shared_ptr<Texture> texture = dxChar->GetTexture();
		spriteText->SetTexture(texture);

		//		int objWidth = texture->GetWidth();//dxChar->GetWidth();
		//		int objHeight = texture->GetHeight();//dxChar->GetHeight();
		POINT* ptrCharSize = &dxChar->GetMaxSize();
		LONG charWidth = ptrCharSize->x;
		LONG charHeight = ptrCharSize->y;
		DxRect<LONG> rcDest(xRender + xOffset, yRender + yOffset,
			charWidth + xRender + xOffset, charHeight + yRender + yOffset) ;
		DxRect<LONG> rcSrc(0, 0, charWidth, charHeight);
		spriteText->SetVertex(rcSrc, rcDest, colorVertex_);
		objRender->AddRenderObject(shared_ptr<Sprite2D>(spriteText));

		//次の文字
		xRender += dxChar->GetSize().x - dxFont.GetBorderWidth() + textLine->GetSidePitch();
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
		LONG linePitch = dxText->GetLinePitch();
		LONG widthMax = dxText->GetMaxWidth();
		LONG heightMax = dxText->GetMaxHeight();
		DxRect<LONG>& margin = dxText->GetMargin();
		TextAlignment alignmentHorizontal = dxText->GetHorizontalAlignment();
		TextAlignment alignmentVertical = dxText->GetVerticalAlignment();

		{
			POINT pos = { 0, 0 };

			bool bAutoIndent = textInfo->IsAutoIndent();

			switch (alignmentVertical) {
			case TextAlignment::Center:
			{
				LONG cy = pos.y + heightMax / 2L;
				pos.y = cy - textInfo->totalHeight_ / 2L;
				break;
			}
			case TextAlignment::Bottom:
			{
				LONG by = pos.y + heightMax;
				pos.y = by - textInfo->totalHeight_;
				break;
			}
			case TextAlignment::Top:
			default:
				break;
			}
			pos.y += margin.top;

			LONG heightTotal = 0L;
			size_t countLine = textInfo->textLine_.size();
			int lineStart = textInfo->GetValidStartLine() - 1;
			int lineEnd = textInfo->GetValidEndLine() - 1;
			for (int iLine = lineStart; iLine <= lineEnd; iLine++) {
				shared_ptr<DxTextLine> textLine = textInfo->GetTextLine(iLine);
				pos.x = 0;//dxText->GetPosition().x;
				if (iLine == 0) pos.x += margin.left;

				switch (alignmentHorizontal) {
				case TextAlignment::Center:
				{
					LONG cx = pos.x + widthMax / 2L;
					pos.x = cx - textLine->width_ / 2L;
					break;
				}
				case TextAlignment::Right:
				{
					LONG rx = pos.x + widthMax;
					pos.x = rx - textLine->width_;
					break;
				}
				case TextAlignment::Left:
				default:
					if (iLine >= 1 && bAutoIndent)
						pos.x = dxText->GetFontSize();
					break;
				}

				heightTotal += textLine->height_ + linePitch;
				if (heightTotal > heightMax) break;

				_CreateRenderObject(objRender, pos, dxFont, textLine);

				pos.y += textLine->height_ + linePitch;
			}
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
bool DxTextRenderer::AddFontFromFile(const std::wstring& path) {
	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open())
		throw gstd::wexception(L"AddFontFromFile: " + ErrorUtility::GetFileNotFoundErrorMessage(path, true));

	std::string source = reader->ReadAllString();

	DWORD count = 0;
	HANDLE hFont = ::AddFontMemResourceEx((LPVOID)source.c_str(), source.size(), nullptr, &count);

	Logger::WriteTop(StringUtility::Format(L"AddFontFromFile: Font loaded. [%s]", path.c_str()));
	return hFont != 0;
}

//*******************************************************************
//DxText
//テキスト描画
//*******************************************************************
DxText::DxText() {
	dxFont_.SetTopColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxFont_.SetBottomColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	dxFont_.SetBorderType(TextBorderType::None);
	dxFont_.SetBorderWidth(0);
	dxFont_.SetBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));

	LOGFONT logFont;
	ZeroMemory(&logFont, sizeof(LOGFONT));
	logFont.lfEscapement = 0;
	logFont.lfWidth = 0;
	//logFont.lfStrikeOut = 1;
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
	alignmentHorizontal_ = TextAlignment::Left;
	alignmentVertical_ = TextAlignment::Top;

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
	LOGFONT& info = dxFont_.GetLogFont();
	lstrcpy(info.lfFaceName, type);

	info.lfCharSet = Font::DetectCharset(type);
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
