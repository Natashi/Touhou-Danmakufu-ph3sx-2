#include "source/GcLib/pch.h"

#include "ImGuiWindow.hpp"

#include "DxConstant.hpp"
#include "DirectGraphics.hpp"

using namespace gstd;
using namespace directx;
using namespace directx::imgui;

// ------------------------------------------------------------------------

ImGuiDirectGraphics::ImGuiDirectGraphics() {
	ZeroMemory(&d3dpp_, sizeof(d3dpp_));
}

void ImGuiDirectGraphics::_ReleaseDxResource() {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	DirectGraphicsBase::_ReleaseDxResource();
}
void ImGuiDirectGraphics::_RestoreDxResource() {
	ImGui_ImplDX9_CreateDeviceObjects();
	DirectGraphicsBase::_RestoreDxResource();
}
bool ImGuiDirectGraphics::_Restore() {
	deviceStatus_ = pDevice_->TestCooperativeLevel();
	if (deviceStatus_ == D3D_OK) {
		return true;
	}
	else if (deviceStatus_ == D3DERR_DEVICENOTRESET) {
		ResetDevice();
		return SUCCEEDED(deviceStatus_);
	}
	return false;
}

std::vector<std::wstring> ImGuiDirectGraphics::_GetRequiredModules() {
	return std::vector<std::wstring>({
		L"d3d9.dll",
	});
}

bool ImGuiDirectGraphics::Initialize(HWND hWnd) {
	_LoadModules();

	Logger::WriteTop("DirectGraphics: Initialize.");

	IDirect3D9* pDirect3D = EDirect3D9::GetInstance()->GetD3D();

	hAttachedWindow_ = hWnd;

	pDirect3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &deviceCaps_);
	_VerifyDeviceCaps();

	d3dpp_.hDeviceWindow = hWnd;
	d3dpp_.Windowed = TRUE;
	d3dpp_.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp_.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp_.EnableAutoDepthStencil = TRUE;
	d3dpp_.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp_.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	{
		HRESULT hrDevice = E_FAIL;
		auto _TryCreateDevice = [&](D3DDEVTYPE type, DWORD addFlag) {
			hrDevice = pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, type, hWnd,
				addFlag | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &d3dpp_, &pDevice_);
		};

		_TryCreateDevice(D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
		if (SUCCEEDED(hrDevice)) {
			Logger::WriteTop("DirectGraphics: Created device (D3DCREATE_HARDWARE_VERTEXPROCESSING)");
		}
		else {
			_TryCreateDevice(D3DDEVTYPE_HAL, D3DCREATE_SOFTWARE_VERTEXPROCESSING);
			if (SUCCEEDED(hrDevice))
				Logger::WriteTop("DirectGraphics: Created device (D3DCREATE_SOFTWARE_VERTEXPROCESSING)");
		}

		if (FAILED(hrDevice)) {
			std::wstring err = StringUtility::Format(
				L"Cannot create Direct3D device. [%s]\r\n  %s",
				DXGetErrorString(hrDevice), DXGetErrorDescription(hrDevice));
			throw wexception(err);
		}
	}

	pDevice_->GetRenderTarget(0, &pBackSurf_);
	pDevice_->GetDepthStencilSurface(&pZBuffer_);

	Logger::WriteTop("DirectGraphics: Initialized.");
	return true;
}
void ImGuiDirectGraphics::Release() {
	DirectGraphicsBase::Release();
}

bool ImGuiDirectGraphics::BeginScene(bool bClear) {
	pDevice_->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pDevice_->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	pDevice_->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	HRESULT hr = pDevice_->BeginScene();
	if (FAILED(hr)) {
		const wchar_t* str = DXGetErrorStringW(hr);
		const wchar_t* desc = DXGetErrorDescriptionW(hr);


	}
	return SUCCEEDED(hr);
}
void ImGuiDirectGraphics::EndScene(bool bPresent) {
	pDevice_->EndScene();

	deviceStatus_ = pDevice_->Present(nullptr, nullptr, nullptr, nullptr);
	if (FAILED(deviceStatus_)) {
		_Restore();
	}
}

void ImGuiDirectGraphics::ResetDevice() {
	_ReleaseDxResource();
	deviceStatus_ = pDevice_->Reset(&d3dpp_);
	if (SUCCEEDED(deviceStatus_)) {
		_RestoreDxResource();
	}
}

// ------------------------------------------------------------------------

ImGuiAddFont::ImGuiAddFont(const std::string& name_, const std::wstring& font_, float size_, const GlyphRange& ranges)
	: ImGuiAddFont(name_, font_, size_)
{
	for (auto& data : ranges) {
		rangesEx.push_back(data[0]);
		rangesEx.push_back(data[1]);
	}
	rangesEx.push_back(0);
}

// ------------------------------------------------------------------------

ImGuiBaseWindow::ImGuiBaseWindow() {
	bInitialized_ = false;
	bImGuiInitialized_ = false;

	pIo_ = nullptr;
	dpi_ = USER_DEFAULT_SCREEN_DPI;

	bRendering_ = false;
}
ImGuiBaseWindow::~ImGuiBaseWindow() {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	dxGraphics_->Release();
	::UnregisterClassW(winClassName_.c_str(), ::GetModuleHandleW(nullptr));
}

bool ImGuiBaseWindow::InitializeWindow(const std::wstring& className) {
	winClassName_ = className;

	HINSTANCE hInst = ::GetModuleHandleW(nullptr);

	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_CLASSDC;
		wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
		wcex.hInstance = hInst;
		wcex.lpszClassName = winClassName_.c_str();
		::RegisterClassExW(&wcex);

		hWnd_ = ::CreateWindowW(wcex.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
			0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
	}

	if (dxGraphics_) {
		if (!dxGraphics_->Initialize(hWnd_))
			return false;

		SetBounds(0, 0, dxGraphics_->GetWidth(), dxGraphics_->GetHeight());
	}

	this->Attach(hWnd_);
	::ShowWindow(hWnd_, SW_SHOWDEFAULT);

	bRun_ = true;

	return true;
}
bool ImGuiBaseWindow::InitializeImGui() {
	IMGUI_CHECKVERSION();

	if (ImGui::CreateContext() == nullptr)
		return false;

	pIo_ = &ImGui::GetIO();
	pIo_->IniFilename = nullptr;	//Disable default save/load of imgui.ini

	if (!ImGui_ImplWin32_Init(hWnd_))
		return false;
	if (!ImGui_ImplDX9_Init(dxGraphics_->GetDevice()))
		return false;

	ImGui_ImplWin32_EnableDpiAwareness();
	_InitializeSystemFonts();

	bImGuiInitialized_ = true;
	return true;
}

void ImGuiBaseWindow::_AddUserFont(const ImGuiAddFont& font) {
	userFontData_.push_back(font);
}
void ImGuiBaseWindow::_InitializeSystemFonts() {
	if (mapSystemFontPath_.size() == 0) {
		std::vector<std::wstring> fonts = { L"Arial" };
		for (auto& i : fonts) {
			auto path = SystemUtility::GetSystemFontFilePath(i);
			mapSystemFontPath_[i] = StringUtility::ConvertWideToMulti(path);
		}
	}
}

void ImGuiBaseWindow::_ResetDevice() {
	{
		//Lock lock(lock_);

		_ResetFont();
		dxGraphics_->ResetDevice();
	}
}
void ImGuiBaseWindow::_ResetFont() {
	_InitializeSystemFonts();

	if (pIo_ == nullptr || ImGui::GetCurrentContext() == nullptr) return;

	float scale = SystemUtility::DpiToScalingFactor(dpi_);

	{
		pIo_->Fonts->Clear();
		mapFont_.clear();

		for (const auto& iData : userFontData_) {
			auto baseFont = mapSystemFontPath_[iData.font].c_str();

			if (iData.rangesEx.size() == 0) {
				mapFont_[iData.name] = pIo_->Fonts->AddFontFromFileTTF(baseFont, iData.size * scale);
			}
			else {
				mapFont_[iData.name] = pIo_->Fonts->AddFontFromFileTTF(baseFont, iData.size * scale,
					nullptr, iData.rangesEx.data());
			}
		}

		pIo_->Fonts->Build();
	}
}

void ImGuiBaseWindow::_Resize(float scale) {
	RECT rc;
	::GetWindowRect(hWnd_, &rc);

	UINT wd = rc.right - rc.left;
	UINT ht = rc.bottom - rc.top;
	wd *= scale;
	ht *= scale;

	::MoveWindow(hWnd_, rc.left, rc.left, wd, ht, true);
	MoveWindowCenter();

	if (dxGraphics_) {
		dxGraphics_->SetSize(wd, ht);
		_ResetDevice();
	}
}

void ImGuiBaseWindow::_SetImguiStyle(const ImGuiStyle& style) {
	if (ImGui::GetCurrentContext() == nullptr)
		return;

	memcpy(defaultStyleColors, style.Colors, sizeof(defaultStyleColors));

	ImGui::GetStyle() = style;
}

void ImGuiBaseWindow::_Close() {
	bRun_ = false;
	::DestroyWindow(hWnd_);
}

void ImGuiBaseWindow::_Update() {
	if (!bInitialized_ || bRendering_) return;

	bRendering_ = true;

	{
		//Lock lock(lock_);

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		_ProcessGui();
		ImGui::EndFrame();

		if (dxGraphics_->BeginScene(true)) {
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		}
		dxGraphics_->EndScene(true);
	}

	bRendering_ = false;
}

bool ImGuiBaseWindow::Loop() {
	if (!bRun_) return false;

	MSG msg;
	while (::PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);

		if (msg.message == WM_QUIT)
			return false;
	}

	if (dxGraphics_) {
		_Update();
	}

	return bRun_;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT ImGuiBaseWindow::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;
	
	if (_SubWindowProcedure(hWnd, uMsg, wParam, lParam))
		return true;

	switch (uMsg) {
	case WM_CLOSE:
		_Close();
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;

	/*case WM_PAINT:
	{
		if (dxGraphics_) {
			_Update();
		}
		return 0;
	}*/
	
	case WM_SIZE:
	{
		if (dxGraphics_) {
			if (dxGraphics_->GetDevice() != nullptr && wParam != SIZE_MINIMIZED) {
				dxGraphics_->SetSize(LOWORD(lParam), HIWORD(lParam));
				_ResetDevice();
			}

			// Immediately re-render after resizing action
			_Update();
		}
		return 0;
	}
	case WM_DPICHANGED:
	{
		dpi_ = LOWORD(wParam);
		_Resize(SystemUtility::DpiToScalingFactor(dpi_));
		return 0;
	}
	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xfff0) == SC_KEYMENU)	// Disable default alt menu
			return 0;
		break;
	}
	}
	
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}

// ------------------------------------------------------------------------

#pragma push_macro("new")
#undef new
#include <imgui_internal.h>
#pragma pop_macro("new")

// https://github.com/ocornut/imgui/issues/1496#issuecomment-655048353
void ImGuiExt::BeginGroupPanel(ImVector<ImRect>* stack, const char* name, const ImVec2& size) {
	ImGui::BeginGroup();

	auto cursorPos = ImGui::GetCursorScreenPos();
	auto itemSpacing = ImGui::GetStyle().ItemSpacing;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	auto frameHeight = ImGui::GetFrameHeight();
	ImGui::BeginGroup();

	ImVec2 effectiveSize = size;
	effectiveSize.x = size.x < 0.0f ? ImGui::GetContentRegionAvail().x : size.x;

	ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::BeginGroup();
	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextUnformatted(name);
	auto labelMin = ImGui::GetItemRectMin();
	auto labelMax = ImGui::GetItemRectMax();
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
	ImGui::BeginGroup();

	//ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

	ImGui::PopStyleVar(2);

	ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;

	ImGui::GetCurrentWindow()->Size.x -= frameHeight;

	auto itemWidth = ImGui::CalcItemWidth();
	ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

	stack->push_back(ImRect(labelMin, labelMax));
}
void ImGuiExt::EndGroupPanel(ImVector<ImRect>* stack) {
	ImGui::PopItemWidth();

	auto itemSpacing = ImGui::GetStyle().ItemSpacing;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	auto frameHeight = ImGui::GetFrameHeight();

	ImGui::EndGroup();

	//ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

	ImGui::EndGroup();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

	ImGui::EndGroup();

	auto itemMin = ImGui::GetItemRectMin();
	auto itemMax = ImGui::GetItemRectMax();
	//ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

	auto labelRect = stack->back();
	stack->pop_back();

	auto _ImVec2_Add = [](const ImVec2& v1, const ImVec2& v2) -> ImVec2 {
		return ImVec2(v1.x + v2.x, v1.y + v2.y);
	};
	auto _ImVec2_Sub = [](const ImVec2& v1, const ImVec2& v2) -> ImVec2 {
		return ImVec2(v1.x - v2.x, v1.y - v2.y);
	};

	ImVec2 halfFrame = ImVec2(frameHeight * 0.25f * 0.5f, frameHeight * 0.5f);
	ImRect frameRect = ImRect(
		_ImVec2_Add(itemMin, halfFrame),
		_ImVec2_Sub(itemMax, ImVec2(halfFrame.x, 0.0f)));
	labelRect.Min.x -= itemSpacing.x;
	labelRect.Max.x += itemSpacing.x;

	for (int i = 0; i < 4; ++i) {
		switch (i) {
			// left half-plane
		case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
			// right half-plane
		case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
			// top
		case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true); break;
			// bottom
		case 3: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
		}

		ImGui::GetWindowDrawList()->AddRect(
			frameRect.Min, frameRect.Max,
			ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)),
			halfFrame.x);

		ImGui::PopClipRect();
	}

	ImGui::PopStyleVar(2);

	ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;

	ImGui::GetCurrentWindow()->Size.x += frameHeight;

	ImGui::Dummy(ImVec2(0.0f, 0.0f));

	ImGui::EndGroup();
}

