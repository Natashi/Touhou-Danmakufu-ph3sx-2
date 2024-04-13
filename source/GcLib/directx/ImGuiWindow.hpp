#pragma once

#include "../pch.h"

#pragma push_macro("new")
#undef new

#include "imgui.h"
#include "backends/imgui_impl_dx9.h"
#include "backends/imgui_impl_win32.h"
#include "imgui_internal.h"

#pragma pop_macro("new")

#include "../gstd/Window.hpp"

#include "DirectGraphicsBase.hpp"

namespace directx::imgui {
	class ImGuiDirectGraphics : public DirectGraphicsBase {
	protected:
		D3DPRESENT_PARAMETERS d3dpp_;
	protected:
		virtual void _ReleaseDxResource();
		virtual void _RestoreDxResource();
		virtual bool _Restore();

		virtual std::vector<std::wstring> _GetRequiredModules();
	public:
		ImGuiDirectGraphics();

		virtual bool Initialize(HWND hWnd);
		virtual void Release();

		virtual bool BeginScene(bool bClear);
		virtual void EndScene(bool bPresent);

		virtual void ResetDeviceState() {};

		virtual void SetSize(UINT wd, UINT ht) {
			d3dpp_.BackBufferWidth = wd;
			d3dpp_.BackBufferHeight = ht;
		}
		virtual UINT GetWidth() { return d3dpp_.BackBufferWidth; }
		virtual UINT GetHeight() { return d3dpp_.BackBufferHeight; }

		virtual void ResetDevice();
	};

	struct ImGuiAddFont {
		using GlyphRange = std::vector<std::array<ImWchar, 2>>;

		std::string name;

		std::wstring font;
		float size;

		std::vector<ImWchar> rangesEx;

		ImGuiAddFont(const std::string& name_, const std::wstring& font_, float size_) : name(name_), font(font_), size(size_) { }
		ImGuiAddFont(const std::string& name_, const std::wstring& font_, float size_, const GlyphRange& ranges);
	};

	// ------------------------------------------------------------------------

	class ImGuiBaseWindow : public gstd::WindowBase {
		gstd::CriticalSection lock_;
	protected:
		virtual LRESULT _WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT _SubWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	protected:
		unique_ptr<ImGuiDirectGraphics> dxGraphics_;
		ImGuiIO* pIo_;

		std::wstring winClassName_;

		std::unordered_map<std::wstring, std::string> mapSystemFontPath_;
		std::vector<ImGuiAddFont> userFontData_;
		std::unordered_map<std::string, ImFont*> mapFont_;
		UINT dpi_;

		bool bInitialized_;
		bool bImGuiInitialized_;
		volatile bool bRun_;

		volatile bool bRendering_;
	public:
		ImVec4 defaultStyleColors[ImGuiCol_COUNT];
	protected:
		void _AddUserFont(const ImGuiAddFont& font);
		void _InitializeSystemFonts();
	protected:
		virtual void _ResetDevice();
		virtual void _ResetFont();

		virtual void _Resize(float scale);
		virtual void _SetImguiStyle(float scale) = 0;

		void _Close();
	protected:
		void _SetImguiStyle(const ImGuiStyle& style);
	protected:
		virtual void _ProcessGui() = 0;
		virtual void _Update();
	public:
		ImGuiBaseWindow();
		virtual ~ImGuiBaseWindow();

		bool InitializeWindow(const std::wstring& className);
		bool InitializeImGui();
		virtual bool Initialize() = 0;

		bool Loop();

		gstd::CriticalSection& GetLock() { return lock_; }

		DirectGraphicsBase* GetDxGraphics() { return dxGraphics_.get(); }
		std::unordered_map<std::string, ImFont*>& GetFontMap() { return mapFont_; }
		ImFont* GetFont(const std::string& name) { return mapFont_[name]; }
	};

	class ImGuiExt {
	private:
		ImGuiExt();		// Disallow instantiation; static only
	public:
		static void BeginGroupPanel(ImVector<ImRect>* stack, const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f));
		static void EndGroupPanel(ImVector<ImRect>* stack);

		template<typename Fn>
		static inline void Disabled(bool disable, Fn fnInnerGui) {
			if (disable)
				ImGui::BeginDisabled();
			fnInnerGui();
			if (disable)
				ImGui::EndDisabled();
		}
	};
}
