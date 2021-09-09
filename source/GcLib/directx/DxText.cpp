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

bool DxCharGlyph::Create(UINT code, const Font& winFont, const DxFont* dxFont) {
	code_ = code;

	static short colorTop[4];
	static short colorBottom[4];
	static short colorBorder[4];
	ColorAccess::ToByteArray(dxFont->GetTopColor(), colorTop);
	ColorAccess::ToByteArray(dxFont->GetBottomColor(), colorBottom);
	ColorAccess::ToByteArray(dxFont->GetBorderColor(), colorBorder);

	TextBorderType typeBorder = dxFont->GetBorderType();
	LONG widthBorder = typeBorder != TextBorderType::None ? dxFont->GetBorderWidth() : 0L;

	//--------------------------------------------------------------

	HDC hDC = ::GetDC(nullptr);
	HFONT oldFont = (HFONT)::SelectObject(hDC, winFont.GetHandle());

	//--------------------------------------------------------------

	const TEXTMETRIC& tm = winFont.GetMetrics();

	const UINT uFormat = GGO_GRAY8_BITMAP;
	const UINT BMP_LEVEL = 64 + 1;
	const UINT BMP_LEVEL_1 = BMP_LEVEL - 1;

	static constexpr const MAT2 mat = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
	DWORD size = ::GetGlyphOutline(hDC, code, uFormat, &glpMet_, 0, nullptr, &mat);

	UINT iBmp_w = Math::CeilBase(glpMet_.gmBlackBoxX, 4);
	UINT iBmp_h = glpMet_.gmBlackBoxY;

	size_.x = glpMet_.gmCellIncX + widthBorder * 2;
	size_.y = tm.tmHeight + widthBorder * 2;

	FLOAT iBmp_h_inv = 1.0f / iBmp_h;
	LONG glyphOriginX = glpMet_.gmptGlyphOrigin.x;
	LONG glyphOriginY = tm.tmAscent - glpMet_.gmptGlyphOrigin.y;
	sizeMax_.x = iBmp_w + widthBorder * 2 + glyphOriginX + (tm.tmItalic ? tm.tmOverhang : 0);
	sizeMax_.y = iBmp_h + widthBorder * 2 + glyphOriginY;

	//--------------------------------------------------------------

	UINT widthTexture = 1;
	UINT heightTexture = 1;
	while (widthTexture < sizeMax_.x) {
		widthTexture = widthTexture << 1;
		if (widthTexture >= 0x2000u) return false;
	}
	while (heightTexture < sizeMax_.y) {
		heightTexture = heightTexture << 1;
		if (heightTexture >= 0x2000u) return false;
	}

	//--------------------------------------------------------------

	IDirect3DTexture9* pTexture = nullptr;
	IDirect3DDevice9* device = DirectGraphics::GetBase()->GetDevice();
	HRESULT hr = device->CreateTexture(widthTexture, heightTexture, 1, 
		D3DPOOL_DEFAULT, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, nullptr);
	if (FAILED(hr)) return false;

	D3DLOCKED_RECT lock;
	if (FAILED(pTexture->LockRect(0, &lock, nullptr, D3DLOCK_DISCARD))) {
		ptr_release(pTexture);
		return false;
	}

	{
		BYTE* ptr = new BYTE[size];
		::GetGlyphOutline(hDC, code, uFormat, &glpMet_, size, ptr, &mat);

		//Restore previous font handle and discard the device context
		::SelectObject(hDC, oldFont);
		::ReleaseDC(nullptr, hDC);

		/*
		{
			auto _GenUnderline = [&](FLOAT ty, FLOAT sz) {
				FLOAT flStart = ty - sz;
				FLOAT flEnd = ty + sz;
				LONG llStart = std::floorf(flStart);
				LONG llEnd = std::ceilf(flEnd);

				for (LONG iy = std::max(llStart, 0L); iy < std::min(llEnd, sizeMax_.y); ++iy) {
					LONG yBmp = iy - glyphOriginY - widthBorder;
					for (LONG ix = 0; ix < sizeMax_.x; ++ix) {
						LONG xBmp = ix - glyphOriginX - widthBorder;
						if ((xBmp >= 0 && xBmp < iBmp_w) && (yBmp >= 0 && yBmp < iBmp_h))
							ptr[std::min(iBmp_w * yBmp + xBmp, size - 1)] = BMP_LEVEL;
					}
				}
			};

			FLOAT lineSize = std::max(tm.tmHeight * 0.02f, 0.5f);
			
			if (tm.tmUnderlined)
				_GenUnderline(tm.tmAscent, lineSize);
			if (tm.tmStruckOut)
				_GenUnderline(tm.tmInternalLeading + (tm.tmHeight - tm.tmAscent) + tm.tmAscent / 2L, lineSize);
		}
		*/

		FillMemory(lock.pBits, lock.Pitch * sizeMax_.y, 0);

		if (size > 0) {
			auto _GenRow = [&](LONG iy) {
				LONG yBmp = iy - glyphOriginY - widthBorder;

				float yColorLerp = yBmp * iBmp_h_inv;
				short colorR = Math::Lerp::Linear(colorTop[1], colorBottom[1], yColorLerp);
				short colorG = Math::Lerp::Linear(colorTop[2], colorBottom[2], yColorLerp);
				short colorB = Math::Lerp::Linear(colorTop[3], colorBottom[3], yColorLerp);

				for (LONG ix = 0; ix < sizeMax_.x; ++ix) {
					LONG xBmp = ix - glyphOriginX - widthBorder;
					bool bInsideBmp = (xBmp >= 0 && xBmp < iBmp_w) && (yBmp >= 0 && yBmp < iBmp_h);

					D3DCOLOR color = 0x00000000;

					short alpha = bInsideBmp ? (255 * ptr[iBmp_w * yBmp + xBmp] / BMP_LEVEL_1) : 0;

					if (typeBorder != TextBorderType::None && alpha != 255) {
						if (alpha == 0) {		//Generate external borders
							size_t count = 0;
							LONG minAlphaEnableDist = 255 * 255;

							const LONG D_MARGIN = widthBorder + 0;
							LONG bx = typeBorder == TextBorderType::Full ? xBmp + D_MARGIN : xBmp + 1;
							LONG by = typeBorder == TextBorderType::Full ? yBmp + D_MARGIN : yBmp + 1;

							for (LONG ax = xBmp - D_MARGIN; ax <= bx; ++ax) {
								for (LONG ay = yBmp - D_MARGIN; ay <= by; ++ay) {
									LONG dist = abs(ax - xBmp) + abs(ay - yBmp);
									if (dist > D_MARGIN || dist == 0) continue;

									bool _bInsideBmp = (ax >= 0 && ax < iBmp_w) && (ay >= 0 && ay < iBmp_h);

									LONG tAlpha = _bInsideBmp ? (255 * ptr[iBmp_w * ay + ax]) / BMP_LEVEL_1 : 0;
									if (tAlpha > 0 && dist < minAlphaEnableDist)
										minAlphaEnableDist = dist;

									LONG tCount = tAlpha / dist * 2L;
									if (typeBorder == TextBorderType::Shadow && (ax >= xBmp || ay >= yBmp))
										tCount /= 2L;
									count += tCount;
								}
							}

							short destAlpha = 0;
							if (minAlphaEnableDist < widthBorder)
								destAlpha = 255;
							else if (minAlphaEnableDist == widthBorder)
								destAlpha = static_cast<short>(count);
							else destAlpha = 0;

							//color = ColorAccess::SetColorA(color, ColorAccess::GetColorA(colorBorder)*count/255);
							byte c_a = ColorAccess::ClampColorRet(colorBorder[0] * destAlpha / 255);
							color = D3DCOLOR_ARGB(c_a, colorBorder[1], colorBorder[2], colorBorder[3]);
						}
						else {				//Generate internal borders + smooth outlines
							size_t count = 0;

							const LONG C_DIST = 1;
							LONG bx = typeBorder == TextBorderType::Full ? xBmp + C_DIST : xBmp + 1;
							LONG by = typeBorder == TextBorderType::Full ? yBmp + C_DIST : yBmp + 1;

							for (LONG ax = xBmp - C_DIST; ax <= bx; ++ax) {
								for (LONG ay = yBmp - C_DIST; ay <= by; ++ay) {
									bool _bInsideBmp = (ax >= 0 && ax < iBmp_w) && (ay >= 0 && ay < iBmp_h);
									if (!_bInsideBmp) continue;
									if (ptr[iBmp_w * ay + ax]) ++count;
								}
							}

							{
								LONG oAlpha = std::min(count >= 2 ? alpha : alpha + 64, 255);
								LONG bAlpha = 255 - oAlpha;
								short c_r = colorR * oAlpha / 255 + colorBorder[1] * bAlpha / 255;
								short c_g = colorG * oAlpha / 255 + colorBorder[2] * bAlpha / 255;
								short c_b = colorB * oAlpha / 255 + colorBorder[3] * bAlpha / 255;
								__m128i c = Vectorize::SetI(0, c_r, c_g, c_b);
								color = ColorAccess::ToD3DCOLOR(ColorAccess::ClampColorPacked(c));
								color = (color & 0x00ffffff) | 0xff000000;
							}
						}
					}
					else {		//Font outline
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
	}

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
void DxTextScanner::_SkipSpace() {
	wchar_t ch = *pointer_;
	while (true) {
		if (!std::iswspace(ch)) break;
		ch = _NextChar();
	}
}
void DxTextScanner::_RaiseError(const std::wstring& str) {
	throw gstd::wexception(str);
}
bool DxTextScanner::_IsTextStartSign() {
	if (bTagScan_) return false;

	wchar_t prev = pointer_ == buffer_.begin() ? '\0' : pointer_[-1];
	wchar_t ch = *pointer_;

	return prev == '\\' || ch != CHAR_TAG_START;
}
bool DxTextScanner::_IsTextScan() {
	wchar_t prev = *pointer_;
	wchar_t ch = _NextChar();
	if (!HasNext()) {
		return false;
	}
	else if (prev != '\\') {
		if (ch == CHAR_TAG_END)
			_RaiseError(L"DxTextScanner: Unexpected tag end token (]) in text.");
		return ch != CHAR_TAG_START;
	}
	return true;
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

	//_SkipComment();

	wchar_t ch = *pointer_;
	if (ch == L'\0') {
		token_ = DxTextToken(DxTextToken::Type::TK_EOF, L"\0");
		return token_;
	}

	DxTextToken::Type type = DxTextToken::Type::TK_UNKNOWN;
	std::vector<wchar_t>::iterator posStart = pointer_;

	if (_IsTextStartSign()) {	//Regular text
		ch = *pointer_;
		posStart = pointer_;

		while (_IsTextScan());

		ch = *pointer_;

		type = DxTextToken::Type::TK_TEXT;
		std::wstring text = std::wstring(posStart, pointer_);
		//text = StringUtility::ParseStringWithEscape(text);
		token_ = DxTextToken(type, text);
	}
	else {						//Text tag contents
		switch (ch) {
		case L'\0': type = DxTextToken::Type::TK_EOF; break;

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
			ch = _NextChar();
			while (true) {
				if (ch == L'\\') ch = _NextChar();	//Skip escaped characters
				else if (ch == L'\"') break;
				ch = _NextChar();
			}
			if (ch == L'\"')
				_NextChar();
			else 
				_RaiseError(L"Next(Text): Unexpected end-of-file while parsing string.");
			type = DxTextToken::Type::TK_STRING;
			break;
		}

		//Newlines
		case L'\r':
		case L'\n':
			//Skip other successive newlines
			while (ch == L'\r' || ch == L'\n') ch = _NextChar();
			type = DxTextToken::Type::TK_NEWLINE;
			break;

		case L'+':
		case L'-':
		{
			if (ch == L'+') {
				ch = _NextChar();
				type = DxTextToken::Type::TK_PLUS;
			}
			else if (ch == L'-') {
				ch = _NextChar();
				type = DxTextToken::Type::TK_MINUS;
			}
			if (!iswdigit(ch)) break;
		}

		default:
		{
			if (iswdigit(ch)) {
				while (iswdigit(ch)) ch = _NextChar();
				type = DxTextToken::Type::TK_INT;

				if (ch == L'.') {
					//Int -> Real
					ch = _NextChar();
					while (iswdigit(ch)) ch = _NextChar();
					type = DxTextToken::Type::TK_REAL;
				}

				if (ch == L'E' || ch == L'e') {
					//Exponent format
					ch = _NextChar();
					while (iswdigit(ch) || ch == L'-') ch = _NextChar();
					type = DxTextToken::Type::TK_REAL;
				}
			}
			else if (iswalpha(ch) || ch == L'_') {
				while (iswalpha(ch) || iswdigit(ch) || ch == L'_')
					ch = _NextChar();
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

		{
			std::wstring wstr = std::wstring(posStart, pointer_);
			if (type == DxTextToken::Type::TK_STRING)
				wstr = StringUtility::ParseStringWithEscape(wstr);
			token_ = DxTextToken(type, wstr);
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
	std::wstring tmp = L"";

	auto _AddStr = [&]() {
		tmp = StringUtility::Trim(tmp);
		if (tmp.size() > 0)
			list.push_back(tmp);
		tmp = L"";
	};

	if (tok.GetType() == DxTextToken::Type::TK_OPENP) {
		while (true) {
			tok = scan.Next();

			DxTextToken::Type type = tok.GetType();
			if (type == DxTextToken::Type::TK_CLOSEP || type == DxTextToken::Type::TK_COMMA) {
				_AddStr();
				if (type == DxTextToken::Type::TK_CLOSEP)
					break;
			}
			else tmp += tok.GetElement();
		}
	}
	else {
		tmp = tok.GetElement();
		_AddStr();
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
	HFONT oldFont = (HFONT)::SelectObject(hDC, winFont_.GetHandle());

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
					if (textLine->height_ == 0) {
						//Insert a dummy space if there is no text
						textLine = _GetTextInfoSub(L" ", dxText, res, textLine, hDC, totalWidth, totalHeight);
					}

					totalWidth = std::max(totalWidth, textLine->width_);
					totalHeight += textLine->height_ + linePitch;
					res->AddTextLine(textLine);

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

					//--------------------------------------------------------------

					auto _FuncSetText = [](Data* i, DxTextScanner& sc) {
						i->tag->SetText(sc.Next().GetString());
					};
					auto _FuncSetRuby = [](Data* i, DxTextScanner& sc) {
						i->tag->SetRuby(sc.Next().GetString());
					};
					auto _FuncSetSize = [](Data* i, DxTextScanner& sc) {
						i->sizeOff = sc.Next().GetInteger();
					};
					auto _FuncSetWeight = [](Data* i, DxTextScanner& sc) {
						i->weightRuby = sc.Next().GetInteger();
					};
					auto _FuncSetLeft = [](Data* i, DxTextScanner& sc) {
						i->leftOff = sc.Next().GetInteger();
					};
					auto _FuncSetPitch = [](Data* i, DxTextScanner& sc) {
						i->pitchOff = sc.Next().GetInteger();
					};

					//Do NOT use [&] lambdas
					static const std::unordered_map<std::wstring, std::function<void(Data*, DxTextScanner&)>> mapFunc = {
						{ L"rb", _FuncSetText },
						{ L"rt", _FuncSetRuby },
						{ L"size", _FuncSetSize },
						{ L"sz", _FuncSetSize },
						{ L"wg", _FuncSetWeight },
						{ L"ox", _FuncSetLeft },
						{ L"op", _FuncSetPitch },
					};

					//--------------------------------------------------------------

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

					

#define CHK_EQ scan.CheckType(scan.Next(), DxTextToken::Type::TK_EQUAL); \
					auto pointerBefore = scan.GetCurrentPointer(); \
					DxTextToken& arg = scan.Next();
#define INIT_LAMBDA(_name) auto _name = [](Data* i, DxTextScanner& scan)
#define INIT_LAMBDA2(_name) auto _name = [&](Data* i, DxTextScanner& scan)
					INIT_LAMBDA(_FuncClear) { i->bClear = true; };
					INIT_LAMBDA(_FuncSetSize) {
						CHK_EQ;
						i->logFont->lfHeight = arg.GetInteger();
					};
					INIT_LAMBDA(_FuncSetWeight) {
						CHK_EQ;
						i->logFont->lfWeight = arg.GetInteger();
					};
					INIT_LAMBDA(_FuncSetItalic) {
						CHK_EQ;
						i->fontData->italic = arg.GetBoolean();
					};
					INIT_LAMBDA(_FuncSetOffX) {
						CHK_EQ;
						i->tag->GetOffset().x = arg.GetInteger();
					};
					INIT_LAMBDA(_FuncSetOffY) {
						CHK_EQ;
						i->tag->GetOffset().y = arg.GetInteger();
					};

					auto __FuncSetColor_S = [](int col, D3DCOLOR* dst, DWORD mask, byte shift) {
						byte c = ColorAccess::ClampColorRet(col);
						*dst = ((*dst) & mask) | (c << shift);
					};
					INIT_LAMBDA2(_FuncSetColor_SB_R) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorBottom, 0xff00ffff, 16);
					};
					INIT_LAMBDA2(_FuncSetColor_SB_G) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorBottom, 0xffff00ff, 8);
					};
					INIT_LAMBDA2(_FuncSetColor_SB_B) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorBottom, 0xffffff00, 0);
					};
					INIT_LAMBDA2(_FuncSetColor_ST_R) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorTop, 0xff00ffff, 16);
					};
					INIT_LAMBDA2(_FuncSetColor_ST_G) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorTop, 0xffff00ff, 8);
					};
					INIT_LAMBDA2(_FuncSetColor_ST_B) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorTop, 0xffffff00, 0);
					};
					INIT_LAMBDA2(_FuncSetColor_SO_R) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorBorder, 0xff00ffff, 16);
					};
					INIT_LAMBDA2(_FuncSetColor_SO_G ) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorBorder, 0xffff00ff, 8);
					};
					INIT_LAMBDA2(_FuncSetColor_SO_B) {
						CHK_EQ;
						__FuncSetColor_S(arg.GetInteger(), &i->fontData->colorBorder, 0xffffff00, 0);
					};

					auto __FuncSetColor_P = [](DxTextScanner& scan, std::vector<wchar_t>::iterator before, D3DCOLOR* dst) {
						scan.SetCurrentPointer(before);
						std::vector<std::wstring> list = GetScannerStringArgumentList(scan, before);
						if (list.size() >= 3) {
							byte r = ColorAccess::ClampColorRet(StringUtility::ToInteger(list[0]));
							byte g = ColorAccess::ClampColorRet(StringUtility::ToInteger(list[1]));
							byte b = ColorAccess::ClampColorRet(StringUtility::ToInteger(list[2]));
							*dst = ((*dst) & 0xff000000) | (D3DCOLOR_XRGB(r, g, b) & 0x00ffffff);
						}
					};
					INIT_LAMBDA2(_FuncSetColor_P_B) {
						CHK_EQ; 
						__FuncSetColor_P(scan, pointerBefore, &i->fontData->colorBottom);
					};
					INIT_LAMBDA2(_FuncSetColor_P_T) {
						CHK_EQ;
						__FuncSetColor_P(scan, pointerBefore, &i->fontData->colorTop);
					};
					INIT_LAMBDA2(_FuncSetColor_P_O) {
						CHK_EQ;
						__FuncSetColor_P(scan, pointerBefore, &i->fontData->colorBorder);
					};
#undef CHK_EQ
#undef INIT_LAMBDA
#undef INIT_LAMBDA2

					//Do NOT use [&] lambdas
					static const std::unordered_map<std::wstring, std::function<void(Data*, DxTextScanner&)>> mapFunc = {
						{ L"r", _FuncClear },
						{ L"rs", _FuncClear },
						{ L"reset", _FuncClear },
						{ L"c", _FuncClear },
						{ L"clr", _FuncClear },
						{ L"clear", _FuncClear },

						{ L"sz", _FuncSetSize },
						{ L"size", _FuncSetSize },
						{ L"wg", _FuncSetWeight },
						{ L"it", _FuncSetItalic },
						//{ L"un", LAMBDA_SET_B(fontData->underl) },		//doesn't work
						//{ L"st", LAMBDA_SET_B(fontData->strike) },		//doesn't work
						{ L"ox", _FuncSetOffX },
						{ L"oy", _FuncSetOffY },

						{ L"br", _FuncSetColor_SB_R },
						{ L"bg", _FuncSetColor_SB_G },
						{ L"bb", _FuncSetColor_SB_B },
						{ L"tr", _FuncSetColor_ST_R },
						{ L"tg", _FuncSetColor_ST_G },
						{ L"tb", _FuncSetColor_ST_B },
						{ L"or", _FuncSetColor_SO_R },
						{ L"og", _FuncSetColor_SO_G },
						{ L"ob", _FuncSetColor_SO_B },

						{ L"bc", _FuncSetColor_P_B },
						{ L"tc", _FuncSetColor_P_T },
						{ L"oc", _FuncSetColor_P_O },
					};

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
						::SelectObject(hDC, oldFont);
						curFontData = orgFontData;
					}
					else {
						widthBorder = font.GetBorderType() != TextBorderType::None ? font.GetBorderWidth() : 0L;
						fontTemp = std::make_shared<Font>();
						fontTemp->CreateFontIndirect(logFont);
						::SelectObject(hDC, fontTemp->GetHandle());
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
	::SelectObject(hDC, oldFont);
	::ReleaseDC(nullptr, hDC);

	return shared_ptr<DxTextInfo>(res);
}
std::wstring DxTextRenderer::_ReplaceRenderText(std::wstring text) {
	//StringUtility::ReplaceAll(text, x, L'\0') is "erase all occurences of x"
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

		LONG yGap = 0L;
		yRender = pos.y + yGap;

		keyFont.code_ = textLine->code_[iCode];
		shared_ptr<DxCharGlyph> dxChar = cache_.GetChar(keyFont);
		if (dxChar == nullptr) {
			dxChar = std::make_shared<DxCharGlyph>();
			dxChar->Create(keyFont.code_, winFont_, &dxFont);
			cache_.AddChar(keyFont, dxChar);
		}

		shared_ptr<Sprite2D> spriteText(new Sprite2D());
		spriteText->SetTexture(dxChar->GetTexture());

		//		int objWidth = texture->GetWidth();//dxChar->GetWidth();
		//		int objHeight = texture->GetHeight();//dxChar->GetHeight();
		POINT* ptrCharSize = &dxChar->GetMaxSize();
		LONG charWidth = ptrCharSize->x;
		LONG charHeight = ptrCharSize->y;
		DxRect<LONG> rcDest(xRender + xOffset, yRender + yOffset,
			charWidth + xRender + xOffset, charHeight + yRender + yOffset);
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
	std::wstring pathReduce = PathProperty::ReduceModuleDirectory(path);

	shared_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr || !reader->Open())
		throw gstd::wexception(L"AddFontFromFile: " + ErrorUtility::GetFileNotFoundErrorMessage(pathReduce, true));

	std::string source = reader->ReadAllString();

	DWORD count = 0;
	HANDLE hFont = ::AddFontMemResourceEx((LPVOID)source.c_str(), source.size(), nullptr, &count);

	Logger::WriteTop(StringUtility::Format(L"AddFontFromFile: Font loaded. [%s]", pathReduce.c_str()));
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
