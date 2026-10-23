// ==WindhawkMod==
// @id              glowing-traffic-light-caption-buttons
// @name            Glowing Traffic Light Caption Buttons (macOS Style)
// @description     Removes native Windows titlebar buttons and replaces them with pure transparent macOS Red, Yellow, and Green circles.
// @version         1.5.0
// @author          Mostafa Ahmed Elshorbagy
// @github          https://github.com/elahorbagy
// @include         *
// @exclude         dwm.exe
// @exclude         csrss.exe
// @exclude         smss.exe
// @exclude         services.exe
// @exclude         lsass.exe
// @exclude         svchost.exe
// @exclude         wininit.exe
// @exclude         winlogon.exe
// @exclude         fontdrvhost.exe
// @exclude         spoolsv.exe
// @exclude         conhost.exe
// @exclude         RuntimeBroker.exe
// @exclude         backgroundTaskHost.exe
// @exclude         SearchIndexer.exe
// @exclude         StartMenuExperienceHost.exe
// @exclude         ShellExperienceHost.exe
// @exclude         SearchHost.exe
// @exclude         TextInputHost.exe
// @exclude         LockApp.exe
// @exclude         sihost.exe
// @exclude         ctfmon.exe
// @exclude         taskhostw.exe
// @exclude         dllhost.exe
// @exclude         audiodg.exe
// @exclude         MsMpEng.exe
// @exclude         SecurityHealthService.exe
// @exclude         SearchProtocolHost.exe
// @exclude         WmiPrvSE.exe
// @architecture    x86-64
// @compilerOptions -luser32 -lgdi32 -ldwmapi -luxtheme
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Glowing Traffic Light Caption Buttons (v1.5)

Completely removes the native Windows 11/10 titlebar buttons (`—`, `□`, `✕`) and replaces them with **pure macOS-style Red / Yellow / Green circles** floating directly on a 100% transparent background.

## What's New in v1.5
- **Native Buttons Completely Removed**: Strips `WS_SYSMENU` & non-client glyphs from eligible windows so DWM never draws the native Minimize/Maximize/Close buttons in the first place.
- **100% Transparent Background (No Rectangle Box)**: Because the native glyphs are gone, no backing box is needed! The 3 circles float cleanly over any Mica, Acrylic, Dark, or Light titlebar.
- **Zero Flashing on Close**: Since native buttons are removed from the window style, nothing ever flashes underneath when closing a window.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- buttonStyle: flat_circle
  $name: Button Shape & Style
  $options:
  - flat_circle: Clean Flat macOS Circle (No outer glow)
  - glow_circle: Glowing macOS Circle
  - glow_squircle: Glowing Rounded Square
  - flat_squircle: Clean Flat Rounded Square (No outer glow)
- buttonOrder: ryg_visual
  $name: Button Color & Action Order
  $options:
  - ryg_visual: Red (Close) · Yellow (Minimize) · Green (Maximize)
  - ryg_win_actions: Red · Yellow · Green colors, but Rightmost = Close (Windows muscle memory)
  - ygr_actions: Yellow (Minimize) · Green (Maximize) · Red (Close)
- buttonPosition: right
  $name: Titlebar Position
  $options:
  - right: Top-Right (Replaces Windows caption buttons in place)
  - left: Top-Left (macOS position)
- hoverSymbols: hover_only
  $name: Inner Button Symbols (x, -, +)
  $options:
  - hover_only: macOS behavior - all three show symbols while hovering the group
  - always: Always show symbols
  - never: No symbols
- backingMode: transparent
  $name: Background Mode
  $options:
  - transparent: 100% Transparent (Removes native buttons, no box behind circles)
  - auto_sample: Auto-Match Window Titlebar Color (For custom-drawn app titlebars)
  - frosted_capsule: Dark Frosted Capsule
  - custom_color: Custom Solid Hex Color (configured below)
- customBackingColor: "#202020"
  $name: Custom Backing Bar Color (Hex)
- redColor: "#FF5F57"
  $name: Red Button Color (Hex)
- yellowColor: "#FEBC2E"
  $name: Yellow Button Color (Hex)
- greenColor: "#28C840"
  $name: Green Button Color (Hex)
- buttonCoreSize: 12
  $name: Button Size (8 to 20 px at 100% DPI)
- glowStrength: 70
  $name: Outer Glow Strength (0 to 100%, glow styles only)
- glowRadius: 9
  $name: Outer Glow Spread (2 to 20 px at 100% DPI, glow styles only)
- buttonSpacing: 22
  $name: Center-to-Center Button Spacing (16 to 48 px at 100% DPI)
- barWidthPx: 138
  $name: Caption Region Width (100 to 160 px at 100% DPI)
- barHeightPx: 31
  $name: Caption Region Height (24 to 42 px at 100% DPI)
- topOffsetPx: 1
  $name: Vertical Offset from Top Edge (0 to 12 px)
- sideOffsetPx: 1
  $name: Horizontal Offset from Window Edge (0 to 20 px)
- dimWhenInactive: true
  $name: Gray Out Buttons When Window Is Inactive (macOS style)
*/
// ==/WindhawkModSettings==

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <uxtheme.h>

#ifndef DWMWA_EXTENDED_FRAME_BOUNDS
#define DWMWA_EXTENDED_FRAME_BOUNDS 9
#endif
#ifndef DWMWA_CLOAKED
#define DWMWA_CLOAKED 14
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_CAPTION_BUTTON_BOUNDS
#define DWMWA_CAPTION_BUTTON_BOUNDS 5
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef WM_POPUPSYSTEMMENU
#define WM_POPUPSYSTEMMENU 0x0313
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr wchar_t kOverlayClassName[] = L"WindhawkGlowingCaptionOverlay";
constexpr UINT_PTR kTimerHoverAnim = 1;
constexpr UINT kMsgReloadSettings = WM_APP + 0x47;
constexpr UINT kPollIntervalMs = 45;
constexpr int kScanEveryNTicks = 30;

enum class ButtonStyle : int { GlowSquircle = 0, GlowCircle = 1, FlatCircle = 2, FlatSquircle = 3 };
enum class ButtonOrder : int { RYG_Visual = 0, RYG_WinActions = 1, YGR_Actions = 2 };
enum class HoverSymbols : int { Never = 0, HoverOnly = 1, Always = 2 };
enum class BackingMode : int { Transparent = 0, AutoSample = 1, FrostedCapsule = 2, CustomColor = 3 };
enum class BtnAction : int { Close = 0, Minimize = 1, Maximize = 2 };

struct ModConfig {
    ButtonStyle style = ButtonStyle::FlatCircle;
    ButtonOrder order = ButtonOrder::RYG_Visual;
    bool leftSide = false;
    HoverSymbols symbols = HoverSymbols::HoverOnly;
    BackingMode backing = BackingMode::Transparent;
    COLORREF customBackingColor = RGB(32, 32, 32);
    COLORREF redColor = RGB(255, 95, 87);
    COLORREF yellowColor = RGB(254, 188, 46);
    COLORREF greenColor = RGB(40, 200, 64);
    int coreSizePx = 12;
    int glowStrength = 70;
    int glowRadiusPx = 9;
    int spacingPx = 22;
    int barWidthPx = 138;
    int barHeightPx = 31;
    int topOffsetPx = 1;
    int sideOffsetPx = 1;
    bool dimWhenInactive = true;
};

struct OverlayInstance {
    HWND targetHwnd = nullptr;
    HWND overlayHwnd = nullptr;
    UINT dpi = 96;
    int width = 0;
    int height = 0;
    int hoverIdx = -1;
    int pressedIdx = -1;
    float hoverAnim[3] = {0.0f, 0.0f, 0.0f};
    bool trackingMouse = false;
    bool isTargetActive = true;
    bool strippedSysMenu = false;
    std::vector<COLORREF> rowColors;
    ULONGLONG lastSampleTick = 0;
    int cornerPx = 0;
    bool hasSampledBg = false;
};

ModConfig g_cfg;
std::unordered_map<HWND, std::unique_ptr<OverlayInstance>> g_byTarget;
std::unordered_map<HWND, OverlayInstance*> g_byOverlay;
HWINEVENTHOOK g_hooks[3] = {nullptr, nullptr, nullptr};
UINT_PTR g_pollTimer = 0;
int g_tick = 0;

std::atomic<bool> g_running{false};
HANDLE g_thread = nullptr;
DWORD g_threadId = 0;

////////////////////////////////////////////////////////////////////////////////
// Utility & Color Helpers

int ScaleForDpi(int val, UINT dpi) {
    return MulDiv(val, static_cast<int>(dpi ? dpi : 96), 96);
}

std::wstring TrimStr(const std::wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n\"");
    if (a == std::wstring::npos) return L"";
    size_t b = s.find_last_not_of(L" \t\r\n\"");
    return s.substr(a, b - a + 1);
}

std::wstring LowerStr(std::wstring s) {
    for (wchar_t& ch : s) ch = static_cast<wchar_t>(std::towlower(ch));
    return s;
}

COLORREF ParseHexColor(std::wstring text, COLORREF fallback) {
    text = TrimStr(text);
    if (!text.empty() && text[0] == L'#') text.erase(0, 1);
    if (text.size() != 6) return fallback;
    for (wchar_t ch : text) {
        if (!std::iswxdigit(ch)) return fallback;
    }
    unsigned long rgb = wcstoul(text.c_str(), nullptr, 16);
    return RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

std::wstring GetSettingStr(PCWSTR name, PCWSTR fallback) {
    PCWSTR raw = Wh_GetStringSetting(name);
    if (!raw) return fallback ? fallback : L"";
    std::wstring res(raw);
    Wh_FreeStringSetting(raw);
    return res;
}

void LoadSettings() {
    ModConfig c;
    std::wstring st = LowerStr(TrimStr(GetSettingStr(L"buttonStyle", L"flat_circle")));
    if (st == L"glow_circle") c.style = ButtonStyle::GlowCircle;
    else if (st == L"glow_squircle") c.style = ButtonStyle::GlowSquircle;
    else if (st == L"flat_squircle") c.style = ButtonStyle::FlatSquircle;
    else c.style = ButtonStyle::FlatCircle;

    std::wstring ord = LowerStr(TrimStr(GetSettingStr(L"buttonOrder", L"ryg_visual")));
    if (ord == L"ryg_win_actions") c.order = ButtonOrder::RYG_WinActions;
    else if (ord == L"ygr_actions") c.order = ButtonOrder::YGR_Actions;
    else c.order = ButtonOrder::RYG_Visual;

    c.leftSide = (LowerStr(TrimStr(GetSettingStr(L"buttonPosition", L"right"))) == L"left");

    std::wstring sym = LowerStr(TrimStr(GetSettingStr(L"hoverSymbols", L"hover_only")));
    if (sym == L"hover_only") c.symbols = HoverSymbols::HoverOnly;
    else if (sym == L"always") c.symbols = HoverSymbols::Always;
    else if (sym == L"never") c.symbols = HoverSymbols::Never;
    else c.symbols = HoverSymbols::HoverOnly;

    std::wstring bMode = LowerStr(TrimStr(GetSettingStr(L"backingMode", L"transparent")));
    if (bMode == L"auto_sample") c.backing = BackingMode::AutoSample;
    else if (bMode == L"frosted_capsule") c.backing = BackingMode::FrostedCapsule;
    else if (bMode == L"custom_color") c.backing = BackingMode::CustomColor;
    else c.backing = BackingMode::Transparent;

    c.customBackingColor = ParseHexColor(GetSettingStr(L"customBackingColor", L"#202020"), RGB(32, 32, 32));
    c.redColor = ParseHexColor(GetSettingStr(L"redColor", L"#FF5F57"), RGB(255, 95, 87));
    c.yellowColor = ParseHexColor(GetSettingStr(L"yellowColor", L"#FEBC2E"), RGB(254, 188, 46));
    c.greenColor = ParseHexColor(GetSettingStr(L"greenColor", L"#28C840"), RGB(40, 200, 64));

    int core = Wh_GetIntSetting(L"buttonCoreSize");
    c.coreSizePx = (core <= 0) ? 12 : std::clamp(core, 8, 20);
    int gStr = Wh_GetIntSetting(L"glowStrength");
    c.glowStrength = (gStr < 0) ? 70 : std::clamp(gStr, 0, 100);
    int gRad = Wh_GetIntSetting(L"glowRadius");
    c.glowRadiusPx = (gRad <= 0) ? 9 : std::clamp(gRad, 2, 20);
    int sp = Wh_GetIntSetting(L"buttonSpacing");
    c.spacingPx = (sp <= 0) ? 22 : std::clamp(sp, 16, 48);
    int bw = Wh_GetIntSetting(L"barWidthPx");
    c.barWidthPx = (bw <= 0) ? 138 : std::clamp(bw, 100, 160);
    int bh = Wh_GetIntSetting(L"barHeightPx");
    c.barHeightPx = (bh <= 0) ? 31 : std::clamp(bh, 24, 42);
    c.topOffsetPx = std::clamp(Wh_GetIntSetting(L"topOffsetPx"), 0, 12);
    c.sideOffsetPx = std::clamp(Wh_GetIntSetting(L"sideOffsetPx"), 0, 20);
    c.dimWhenInactive = Wh_GetIntSetting(L"dimWhenInactive") != 0;

    g_cfg = c;
}

void GetSlotColorAndAction(int slotIndex, COLORREF& outColor, BtnAction& outAction) {
    slotIndex = std::clamp(slotIndex, 0, 2);
    switch (g_cfg.order) {
        case ButtonOrder::RYG_WinActions:
            if (slotIndex == 0) { outColor = g_cfg.redColor;    outAction = BtnAction::Minimize; }
            if (slotIndex == 1) { outColor = g_cfg.yellowColor; outAction = BtnAction::Maximize; }
            if (slotIndex == 2) { outColor = g_cfg.greenColor;  outAction = BtnAction::Close;    }
            break;
        case ButtonOrder::YGR_Actions:
            if (slotIndex == 0) { outColor = g_cfg.yellowColor; outAction = BtnAction::Minimize; }
            if (slotIndex == 1) { outColor = g_cfg.greenColor;  outAction = BtnAction::Maximize; }
            if (slotIndex == 2) { outColor = g_cfg.redColor;    outAction = BtnAction::Close;    }
            break;
        case ButtonOrder::RYG_Visual:
        default:
            if (slotIndex == 0) { outColor = g_cfg.redColor;    outAction = BtnAction::Close;    }
            if (slotIndex == 1) { outColor = g_cfg.yellowColor; outAction = BtnAction::Minimize; }
            if (slotIndex == 2) { outColor = g_cfg.greenColor;  outAction = BtnAction::Maximize; }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Native Caption Button Removal Helper

void RemoveNativeCaptionButtons(HWND hwnd, bool remove) {
    if (!hwnd || !IsWindow(hwnd)) return;

    // 1. Suppress non-client icon & system menu drawing via uxtheme
    WTA_OPTIONS opts{};
    opts.dwMask = WTNCA_NODRAWICON | WTNCA_NOSYSMENU;
    opts.dwFlags = remove ? (WTNCA_NODRAWICON | WTNCA_NOSYSMENU) : 0;
    SetWindowThemeAttribute(hwnd, WTA_NONCLIENT, &opts, sizeof(opts));

    // 2. Strip WS_SYSMENU so Windows DWM completely stops rendering Minimize/Maximize/Close glyphs
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (remove) {
        if (style & WS_SYSMENU) {
            SetWindowLongPtrW(hwnd, GWL_STYLE, style & ~WS_SYSMENU);
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
    } else {
        if (!(style & WS_SYSMENU)) {
            SetWindowLongPtrW(hwnd, GWL_STYLE, style | WS_SYSMENU);
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// 32-bit premultiplied ARGB renderer

float SdfRoundedBox(float px, float py, float hx, float hy, float r) {
    float qx = std::fabs(px) - hx + r;
    float qy = std::fabs(py) - hy + r;
    float outDist = std::hypot(std::max(qx, 0.0f), std::max(qy, 0.0f));
    float inDist = std::min(std::max(qx, qy), 0.0f);
    return outDist + inDist - r;
}

void BlendPixelPremul(uint32_t& dst, int r, int g, int b, int a) {
    a = std::clamp(a, 0, 255);
    if (a <= 0) return;
    uint32_t dstA = (dst >> 24) & 0xFF;
    uint32_t dstR = (dst >> 16) & 0xFF;
    uint32_t dstG = (dst >> 8) & 0xFF;
    uint32_t dstB = dst & 0xFF;

    uint32_t srcR = (r * a + 127) / 255;
    uint32_t srcG = (g * a + 127) / 255;
    uint32_t srcB = (b * a + 127) / 255;

    uint32_t invA = 255 - a;
    uint32_t outA = a + (dstA * invA + 127) / 255;
    uint32_t outR = std::min(outA, srcR + (dstR * invA + 127) / 255);
    uint32_t outG = std::min(outA, srcG + (dstG * invA + 127) / 255);
    uint32_t outB = std::min(outA, srcB + (dstB * invA + 127) / 255);

    dst = (outA << 24) | (outR << 16) | (outG << 8) | outB;
}

void DrawAntiAliasedSegment(std::vector<uint32_t>& buf, int w, int h,
                            float x0, float y0, float x1, float y1,
                            float thickness, COLORREF color, int alpha) {
    int minX = std::max(0, static_cast<int>(std::floor(std::min(x0, x1) - thickness - 2.0f)));
    int maxX = std::min(w - 1, static_cast<int>(std::ceil(std::max(x0, x1) + thickness + 2.0f)));
    int minY = std::max(0, static_cast<int>(std::floor(std::min(y0, y1) - thickness - 2.0f)));
    int maxY = std::min(h - 1, static_cast<int>(std::ceil(std::max(y0, y1) + thickness + 2.0f)));

    float dx = x1 - x0;
    float dy = y1 - y0;
    float lenSq = dx * dx + dy * dy;
    float halfT = thickness * 0.5f;
    int r = GetRValue(color), g = GetGValue(color), b = GetBValue(color);

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {
            float cx = px + 0.5f;
            float cy = py + 0.5f;
            float t = (lenSq > 0.0001f) ? std::clamp(((cx - x0) * dx + (cy - y0) * dy) / lenSq, 0.0f, 1.0f) : 0.0f;
            float dist = std::hypot(cx - (x0 + t * dx), cy - (y0 + t * dy));
            float cov = std::clamp(halfT + 0.5f - dist, 0.0f, 1.0f);
            if (cov > 0.0f) {
                BlendPixelPremul(buf[static_cast<size_t>(py) * w + px], r, g, b, static_cast<int>(alpha * cov));
            }
        }
    }
}

void RenderCaptionButtonsBuffer(std::vector<uint32_t>& outPixels, int width, int height,
                                UINT dpi, const std::vector<COLORREF>& rowColors, int cornerPx,
                                bool isWindowActive,
                                int hoverIdx, int pressedIdx, const float hoverAnim[3]) {
    if (width <= 0 || height <= 0) return;
    outPixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);

    // 1. Optional Backing Plate (Skipped completely in Transparent mode!)
    if (g_cfg.backing != BackingMode::Transparent) {
        bool autoMode = (g_cfg.backing == BackingMode::AutoSample);
        COLORREF plateCol = RGB(32, 32, 32);
        int plateAlpha = 255;
        float plateCornerRad = static_cast<float>(ScaleForDpi(4, dpi));
        if (g_cfg.backing == BackingMode::FrostedCapsule) {
            plateCol = RGB(26, 30, 38);
            plateAlpha = 240;
            plateCornerRad = static_cast<float>(ScaleForDpi(6, dpi));
        } else if (g_cfg.backing == BackingMode::CustomColor) {
            plateCol = g_cfg.customBackingColor;
        }

        float halfW = width * 0.5f;
        float halfH = height * 0.5f;
        float featherW = static_cast<float>(ScaleForDpi(14, dpi));

        for (int y = 0; y < height; ++y) {
            COLORREF rowCol = plateCol;
            if (autoMode) {
                rowCol = (static_cast<size_t>(y) < rowColors.size()) ? rowColors[y]
                       : (rowColors.empty() ? RGB(32, 32, 32) : rowColors.back());
            }
            int pr = GetRValue(rowCol), pg = GetGValue(rowCol), pb = GetBValue(rowCol);

            for (int x = 0; x < width; ++x) {
                uint32_t& dst = outPixels[static_cast<size_t>(y) * width + x];
                if (autoMode) {
                    float cov = 1.0f;
                    float fx = x + 0.5f, fy = y + 0.5f;
                    float innerDist = g_cfg.leftSide ? (width - fx) : fx;
                    if (innerDist < featherW) {
                        cov *= std::clamp(innerDist / featherW, 0.0f, 1.0f);
                    }
                    float rad = static_cast<float>(cornerPx);
                    if (rad > 0.5f) {
                        bool inCorner = g_cfg.leftSide ? (fx < rad && fy < rad)
                                                       : (fx > width - rad && fy < rad);
                        if (inCorner) {
                            float ccx = g_cfg.leftSide ? rad : (width - rad);
                            float cornerCov = std::clamp(0.5f - (std::hypot(fx - ccx, fy - rad) - rad), 0.0f, 1.0f);
                            cov = std::min(cov, cornerCov);
                        }
                    }
                    if (cov > 0.0f) BlendPixelPremul(dst, pr, pg, pb, static_cast<int>(255 * cov));
                } else {
                    float px = (x + 0.5f) - halfW;
                    float py = (y + 0.5f) - halfH;
                    float d = SdfRoundedBox(px, py, halfW - 0.5f, halfH - 0.5f, plateCornerRad);
                    float cov = std::clamp(0.5f - d, 0.0f, 1.0f);
                    if (cov > 0.0f) {
                        BlendPixelPremul(dst, pr, pg, pb, static_cast<int>(plateAlpha * cov));
                    }
                }
            }
        }
    }

    // 2. Render the 3 Traffic Light Circles directly onto the transparent buffer
    float spacing = static_cast<float>(ScaleForDpi(g_cfg.spacingPx, dpi));
    float centerY = height * 0.5f;
    float midX = width * 0.5f;
    float centersX[3] = {midX - spacing, midX, midX + spacing};

    bool useCircle = (g_cfg.style == ButtonStyle::GlowCircle || g_cfg.style == ButtonStyle::FlatCircle);
    bool isFlat = (g_cfg.style == ButtonStyle::FlatCircle || g_cfg.style == ButtonStyle::FlatSquircle);
    bool useGlow = !isFlat && (g_cfg.glowStrength > 0);

    float baseCoreHalf = ScaleForDpi(g_cfg.coreSizePx, dpi) * 0.5f;
    float baseGlowRad = static_cast<float>(ScaleForDpi(g_cfg.glowRadiusPx, dpi));
    float squircleRad = std::max(2.0f, baseCoreHalf * 0.42f);
    bool grayed = !isWindowActive && g_cfg.dimWhenInactive;
    float activeMult = grayed ? 0.52f : 1.0f;

    float groupAnim = 0.0f;
    for (int k = 0; k < 3; ++k) {
        groupAnim = std::max(groupAnim, hoverAnim ? hoverAnim[k] : (hoverIdx >= 0 ? 1.0f : 0.0f));
    }
    if (hoverIdx >= 0) groupAnim = std::max(groupAnim, 0.01f);

    int inactiveGray = 110;

    for (int i = 0; i < 3; ++i) {
        COLORREF btnColor;
        BtnAction action;
        GetSlotColorAndAction(i, btnColor, action);

        float hAnim = std::clamp(groupAnim, 0.0f, 1.0f);
        bool isPressed = (pressedIdx == i);
        bool isHot = (hoverIdx == i);

        float coreHalf = baseCoreHalf * (1.0f - (isPressed ? 0.05f : 0.0f));
        float glowRad = baseGlowRad * (1.0f + 0.22f * hAnim);
        float glowPeakAlpha = (g_cfg.glowStrength / 100.0f) * (215.0f + 40.0f * hAnim) * activeMult;

        int cr = GetRValue(btnColor), cg = GetGValue(btnColor), cb = GetBValue(btnColor);
        if (grayed && hAnim < 0.5f) { cr = cg = cb = inactiveGray; }
        if (isHot && !isPressed) { cr = std::min(255, cr + 16); cg = std::min(255, cg + 16); cb = std::min(255, cb + 16); }
        if (isPressed) { cr = cr * 4 / 5; cg = cg * 4 / 5; cb = cb * 4 / 5; }
        COLORREF symColor = RGB(GetRValue(btnColor) * 28 / 100, GetGValue(btnColor) * 28 / 100, GetBValue(btnColor) * 28 / 100);

        int coreCenterR = std::min(255, cr + 55);
        int coreCenterG = std::min(255, cg + 55);
        int coreCenterB = std::min(255, cb + 55);

        float cx = centersX[i];
        float cy = centerY;
        float maxExtent = coreHalf + (useGlow ? glowRad * 1.6f : 2.0f);

        int x0 = std::max(0, static_cast<int>(std::floor(cx - maxExtent)));
        int x1 = std::min(width - 1, static_cast<int>(std::ceil(cx + maxExtent)));
        int y0 = std::max(0, static_cast<int>(std::floor(cy - maxExtent)));
        int y1 = std::min(height - 1, static_cast<int>(std::ceil(cy + maxExtent)));

        for (int py = y0; py <= y1; ++py) {
            for (int px = x0; px <= x1; ++px) {
                float dx = (px + 0.5f) - cx;
                float dy = (py + 0.5f) - cy;
                float dist = useCircle ? (std::hypot(dx, dy) - coreHalf)
                                       : SdfRoundedBox(dx, dy, coreHalf, coreHalf, squircleRad);

                if (useGlow && dist > -1.5f) {
                    float normD = std::max(0.0f, dist) / std::max(1.0f, glowRad);
                    if (normD < 1.55f) {
                        int ga = static_cast<int>(glowPeakAlpha * std::exp(-2.35f * normD * normD));
                        if (ga > 1) BlendPixelPremul(outPixels[static_cast<size_t>(py) * width + px], cr, cg, cb, ga);
                    }
                }

                float coreCov = std::clamp(0.5f - dist, 0.0f, 1.0f);
                if (coreCov > 0.0f) {
                    int rMix = cr, gMix = cg, bMix = cb;
                    if (!isFlat) {
                        float innerFactor = std::clamp(-dist / std::max(1.0f, coreHalf), 0.0f, 1.0f);
                        rMix = static_cast<int>(cr + (coreCenterR - cr) * innerFactor * 0.65f);
                        gMix = static_cast<int>(cg + (coreCenterG - cg) * innerFactor * 0.65f);
                        bMix = static_cast<int>(cb + (coreCenterB - cb) * innerFactor * 0.65f);
                    }
                    int coreAlpha = static_cast<int>(255.0f * coreCov);
                    BlendPixelPremul(outPixels[static_cast<size_t>(py) * width + px], rMix, gMix, bMix, coreAlpha);

                    if (!isFlat && dist > -1.4f && dist <= 0.0f) {
                        int rimA = static_cast<int>(95.0f * coreCov * activeMult);
                        BlendPixelPremul(outPixels[static_cast<size_t>(py) * width + px],
                                         std::min(255, cr + 85), std::min(255, cg + 85), std::min(255, cb + 85), rimA);
                    }
                }
            }
        }

        bool drawSym = (g_cfg.symbols == HoverSymbols::Always) ||
                       (g_cfg.symbols == HoverSymbols::HoverOnly && hAnim > 0.05f);
        if (drawSym) {
            float symAlphaF = (g_cfg.symbols == HoverSymbols::Always) ? 0.9f : hAnim;
            int symAlpha = static_cast<int>(235.0f * symAlphaF * (grayed && hAnim < 0.5f ? 0.6f : 1.0f));
            float rSym = coreHalf * 0.44f;
            float stroke = std::max(1.25f, ScaleForDpi(1, dpi) * 1.25f);
            COLORREF symCol = symColor;

            if (action == BtnAction::Close) {
                DrawAntiAliasedSegment(outPixels, width, height, cx - rSym, cy - rSym, cx + rSym, cy + rSym, stroke, symCol, symAlpha);
                DrawAntiAliasedSegment(outPixels, width, height, cx + rSym, cy - rSym, cx - rSym, cy + rSym, stroke, symCol, symAlpha);
            } else if (action == BtnAction::Minimize) {
                DrawAntiAliasedSegment(outPixels, width, height, cx - rSym, cy, cx + rSym, cy, stroke, symCol, symAlpha);
            } else {
                DrawAntiAliasedSegment(outPixels, width, height, cx - rSym, cy, cx + rSym, cy, stroke, symCol, symAlpha);
                DrawAntiAliasedSegment(outPixels, width, height, cx, cy - rSym, cx, cy + rSym, stroke, symCol, symAlpha);
            }
        }
    }
}

int HitTestButtonSlot(int x, int y, int width, int height, UINT dpi) {
    if (x < 0 || x >= width || y < 0 || y >= height) return -1;
    float spacing = static_cast<float>(ScaleForDpi(g_cfg.spacingPx, dpi));
    float midX = width * 0.5f;
    float centersX[3] = {midX - spacing, midX, midX + spacing};
    float hitHalfW = spacing * 0.5f;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(x - centersX[i]) <= hitHalfW) return i;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Target window filtering & positioning

UINT GetTargetDpi(HWND hwnd) {
    UINT dpi = GetDpiForWindow(hwnd);
    if (!dpi) dpi = 96;
    DPI_AWARENESS_CONTEXT ctx = GetWindowDpiAwarenessContext(hwnd);
    if (ctx && GetAwarenessFromDpiAwarenessContext(ctx) == DPI_AWARENESS_UNAWARE) {
        UINT sys = GetDpiForSystem();
        if (sys) dpi = sys;
    }
    return dpi;
}

bool IsEligibleTopLevelWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd)) return false;
    if (GetAncestor(hwnd, GA_ROOT) != hwnd) return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) return false;

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    if (style & WS_CHILD) return false;
    if (exStyle & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)) return false;
    if ((style & WS_CAPTION) != WS_CAPTION) return false;
    if ((style & (WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) != (WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) return false;

    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0) {
        return false;
    }

    wchar_t cls[128]{};
    if (GetClassNameW(hwnd, cls, 128)) {
        if (wcscmp(cls, kOverlayClassName) == 0 ||
            wcscmp(cls, L"Shell_TrayWnd") == 0 ||
            wcscmp(cls, L"Shell_SecondaryTrayWnd") == 0 ||
            wcscmp(cls, L"Progman") == 0 ||
            wcscmp(cls, L"WorkerW") == 0 ||
            wcscmp(cls, L"#32768") == 0) {
            return false;
        }
    }

    RECT wr{};
    if (!GetWindowRect(hwnd, &wr)) return false;
    if (wr.right - wr.left < 180 || wr.bottom - wr.top < 80) return false;
    return true;
}

bool GetTargetFrameRect(HWND targetHwnd, RECT& outFrame) {
    if (SUCCEEDED(DwmGetWindowAttribute(targetHwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &outFrame, sizeof(outFrame)))) {
        if (outFrame.right > outFrame.left && outFrame.bottom > outFrame.top) return true;
    }
    return GetWindowRect(targetHwnd, &outFrame) != FALSE;
}

bool IsWindows11OrLater() {
    static int cached = -1;
    if (cached < 0) {
        typedef LONG(WINAPI * RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
        RtlGetVersionFn fn = reinterpret_cast<RtlGetVersionFn>(
            GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
        RTL_OSVERSIONINFOW vi{};
        vi.dwOSVersionInfoSize = sizeof(vi);
        cached = (fn && fn(&vi) == 0 && vi.dwBuildNumber >= 22000) ? 1 : 0;
    }
    return cached == 1;
}

COLORREF GetFallbackTitlebarColor(HWND targetHwnd) {
    COLORREF dwmCapColor = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(targetHwnd, DWMWA_CAPTION_COLOR, &dwmCapColor, sizeof(dwmCapColor)))) {
        if (dwmCapColor != 0xFFFFFFFF && dwmCapColor != 0xFFFFFFFE) {
            return dwmCapColor & 0x00FFFFFF;
        }
    }
    BOOL dark = TRUE;
    DwmGetWindowAttribute(targetHwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    return dark ? RGB(32, 32, 32) : RGB(243, 243, 243);
}

void PaintOverlayWindow(OverlayInstance& inst) {
    if (!inst.overlayHwnd || !IsWindow(inst.overlayHwnd)) return;
    if (inst.width <= 0 || inst.height <= 0) return;

    std::vector<uint32_t> pixels;
    RenderCaptionButtonsBuffer(pixels, inst.width, inst.height, inst.dpi, inst.rowColors,
                               inst.cornerPx, inst.isTargetActive, inst.hoverIdx, inst.pressedIdx,
                               inst.hoverAnim);
    if (pixels.empty()) return;

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = inst.width;
    bi.bmiHeader.biHeight = -inst.height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screenDc = GetDC(nullptr);
    HDC memDc = CreateCompatibleDC(screenDc);
    HBITMAP hbmp = CreateDIBSection(memDc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (hbmp && bits) {
        memcpy(bits, pixels.data(), pixels.size() * sizeof(uint32_t));
        HGDIOBJ oldBmp = SelectObject(memDc, hbmp);

        POINT ptSrc{0, 0};
        SIZE sz{inst.width, inst.height};
        BLENDFUNCTION bf{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UpdateLayeredWindow(inst.overlayHwnd, screenDc, nullptr, &sz, memDc, &ptSrc, 0, &bf, ULW_ALPHA);

        SelectObject(memDc, oldBmp);
    }
    if (hbmp) DeleteObject(hbmp);
    if (memDc) DeleteDC(memDc);
    if (screenDc) ReleaseDC(nullptr, screenDc);
}

void SyncOverlay(OverlayInstance& inst, bool force) {
    HWND target = inst.targetHwnd;
    HWND ov = inst.overlayHwnd;
    if (!target || !ov || !IsWindow(target) || !IsWindow(ov)) return;

    LONG_PTR style = GetWindowLongPtrW(target, GWL_STYLE);
    bool hide = !IsWindowVisible(target) || IsIconic(target) || ((style & WS_CAPTION) != WS_CAPTION);
    if (!hide) {
        DWORD cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(target, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0) {
            hide = true;
        }
    }
    if (hide) {
        if (IsWindowVisible(ov)) {
            if (GetCapture() == ov) ReleaseCapture();
            inst.hoverIdx = -1;
            inst.pressedIdx = -1;
            inst.trackingMouse = false;
            inst.hoverAnim[0] = inst.hoverAnim[1] = inst.hoverAnim[2] = 0.0f;
            KillTimer(ov, kTimerHoverAnim);
            ShowWindow(ov, SW_HIDE);
            inst.hasSampledBg = false;
        }
        return;
    }

    // Keep native buttons stripped if the app re-added WS_SYSMENU
    if (style & WS_SYSMENU) {
        RemoveNativeCaptionButtons(target, true);
        inst.strippedSysMenu = true;
    }

    UINT dpi = GetTargetDpi(target);
    RECT frame{};
    if (!GetTargetFrameRect(target, frame)) return;

    int w = ScaleForDpi(g_cfg.barWidthPx, dpi);
    int h = ScaleForDpi(g_cfg.barHeightPx, dpi);

    int sideOff = ScaleForDpi(g_cfg.sideOffsetPx, dpi);
    int topOff = ScaleForDpi(g_cfg.topOffsetPx, dpi);
    if (IsZoomed(target)) topOff = std::max(0, topOff - ScaleForDpi(1, dpi));

    int x = g_cfg.leftSide ? (frame.left + sideOff) : (frame.right - w - sideOff);
    int y = frame.top + topOff;

    HWND fg = GetForegroundWindow();
    bool activeNow = fg && (fg == target || GetAncestor(fg, GA_ROOTOWNER) == target);
    bool sizeOrDpiChanged = (inst.width != w || inst.height != h || inst.dpi != dpi);
    bool activeChanged = (inst.isTargetActive != activeNow);

    inst.dpi = dpi;
    inst.width = w;
    inst.height = h;
    inst.isTargetActive = activeNow;

    int newCorner = (IsWindows11OrLater() && !IsZoomed(target)) ? ScaleForDpi(7, dpi) : 0;
    bool cornerChanged = (inst.cornerPx != newCorner);
    inst.cornerPx = newCorner;

    bool wasHidden = !IsWindowVisible(ov);
    if (!inst.hasSampledBg || activeChanged || force || sizeOrDpiChanged) {
        inst.rowColors.assign(h, GetFallbackTitlebarColor(target));
        inst.hasSampledBg = true;
    }

    bool targetTopmost = (GetWindowLongPtrW(target, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    HWND prev = GetWindow(target, GW_HWNDPREV);
    UINT flags = SWP_NOACTIVATE | SWP_SHOWWINDOW;
    HWND insertAfter = nullptr;
    if (prev == ov) {
        flags |= SWP_NOZORDER;
    } else {
        insertAfter = prev ? prev : (targetTopmost ? HWND_TOPMOST : HWND_TOP);
    }
    SetWindowPos(ov, insertAfter, x, y, w, h, flags);

    if (sizeOrDpiChanged || activeChanged || force || cornerChanged || wasHidden) {
        PaintOverlayWindow(inst);
    }
}

void ExecuteButtonAction(OverlayInstance& inst, BtnAction action) {
    HWND targetHwnd = inst.targetHwnd;
    if (!targetHwnd || !IsWindow(targetHwnd)) return;
    switch (action) {
        case BtnAction::Close:
            PostMessageW(targetHwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            break;
        case BtnAction::Minimize:
            if (inst.overlayHwnd) ShowWindow(inst.overlayHwnd, SW_HIDE);
            ShowWindowAsync(targetHwnd, SW_MINIMIZE);
            break;
        case BtnAction::Maximize:
            ShowWindowAsync(targetHwnd, IsZoomed(targetHwnd) ? SW_RESTORE : SW_MAXIMIZE);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Overlay window procedure

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    OverlayInstance* inst = nullptr;
    auto it = g_byOverlay.find(hwnd);
    if (it != g_byOverlay.end()) inst = it->second;
    if (!inst) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;

        case WM_NCHITTEST: {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);
            int slot = HitTestButtonSlot(pt.x, pt.y, inst->width, inst->height, inst->dpi);
            return (slot >= 0) ? HTCLIENT : HTTRANSPARENT;
        }

        case WM_MOUSEMOVE: {
            if (!inst->trackingMouse) {
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
                inst->trackingMouse = TrackMouseEvent(&tme) != FALSE;
            }
            int slot = HitTestButtonSlot(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         inst->width, inst->height, inst->dpi);
            if (slot != inst->hoverIdx) {
                inst->hoverIdx = slot;
                SetTimer(hwnd, kTimerHoverAnim, 16, nullptr);
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            inst->trackingMouse = false;
            inst->hoverIdx = -1;
            inst->pressedIdx = -1;
            SetTimer(hwnd, kTimerHoverAnim, 16, nullptr);
            return 0;

        case WM_TIMER:
            if (wParam == kTimerHoverAnim) {
                bool anyMoving = false;
                for (int i = 0; i < 3; ++i) {
                    float target = (inst->hoverIdx >= 0) ? 1.0f : 0.0f;
                    float diff = target - inst->hoverAnim[i];
                    if (std::fabs(diff) > 0.04f) {
                        inst->hoverAnim[i] += diff * 0.35f;
                        anyMoving = true;
                    } else {
                        inst->hoverAnim[i] = target;
                    }
                }
                PaintOverlayWindow(*inst);
                if (!anyMoving) KillTimer(hwnd, kTimerHoverAnim);
            }
            return 0;

        case WM_LBUTTONDOWN: {
            int slot = HitTestButtonSlot(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         inst->width, inst->height, inst->dpi);
            if (slot >= 0) {
                inst->pressedIdx = slot;
                SetCapture(hwnd);
                PaintOverlayWindow(*inst);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (GetCapture() == hwnd) ReleaseCapture();
            int slot = HitTestButtonSlot(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                         inst->width, inst->height, inst->dpi);
            int wasPressed = inst->pressedIdx;
            inst->pressedIdx = -1;
            BtnAction act = BtnAction::Close;
            if (wasPressed >= 0 && slot == wasPressed) {
                COLORREF dummy;
                GetSlotColorAndAction(slot, dummy, act);
            }
            PaintOverlayWindow(*inst);
            if (wasPressed >= 0 && slot == wasPressed) ExecuteButtonAction(*inst, act);
            return 0;
        }

        case WM_RBUTTONUP: {
            if (inst->targetHwnd && IsWindow(inst->targetHwnd)) {
                POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ClientToScreen(hwnd, &pt);
                PostMessageW(inst->targetHwnd, WM_POPUPSYSTEMMENU, 0, MAKELPARAM(pt.x, pt.y));
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// Overlay lifetime

void DestroyOverlayForTarget(HWND targetHwnd, bool restoreNative = false) {
    auto it = g_byTarget.find(targetHwnd);
    if (it == g_byTarget.end()) return;
    HWND ov = it->second->overlayHwnd;
    bool wasStripped = it->second->strippedSysMenu;
    g_byOverlay.erase(ov);
    g_byTarget.erase(it);
    if (ov && IsWindow(ov)) DestroyWindow(ov);
    if (restoreNative && wasStripped && IsWindow(targetHwnd)) {
        RemoveNativeCaptionButtons(targetHwnd, false);
    }
}

void AttachOverlayToWindow(HWND targetHwnd) {
    if (!g_running.load() || g_byTarget.find(targetHwnd) != g_byTarget.end()) return;
    if (!IsEligibleTopLevelWindow(targetHwnd)) return;

    RemoveNativeCaptionButtons(targetHwnd, true);

    HWND overlay = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                   kOverlayClassName, L"", WS_POPUP,
                                   0, 0, 1, 1,
                                   nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!overlay) return;

    auto inst = std::make_unique<OverlayInstance>();
    inst->targetHwnd = targetHwnd;
    inst->overlayHwnd = overlay;
    inst->dpi = GetTargetDpi(targetHwnd);
    inst->strippedSysMenu = true;

    OverlayInstance* raw = inst.get();
    g_byOverlay[overlay] = raw;
    g_byTarget[targetHwnd] = std::move(inst);
    SyncOverlay(*raw, true);
}

std::vector<HWND> SnapshotTargets() {
    std::vector<HWND> list;
    list.reserve(g_byTarget.size());
    for (auto& kv : g_byTarget) list.push_back(kv.first);
    return list;
}

void SyncAllOverlays(bool force) {
    for (HWND t : SnapshotTargets()) {
        auto it = g_byTarget.find(t);
        if (it == g_byTarget.end()) continue;
        if (!IsWindow(t)) {
            DestroyOverlayForTarget(t, false);
            continue;
        }
        if (force) it->second->hasSampledBg = false;
        SyncOverlay(*it->second, force);
    }
}

BOOL CALLBACK ScanEnumProc(HWND hwnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == static_cast<DWORD>(lParam) && g_byTarget.find(hwnd) == g_byTarget.end() &&
        IsEligibleTopLevelWindow(hwnd)) {
        AttachOverlayToWindow(hwnd);
    }
    return TRUE;
}

void ScanProcessWindows() {
    EnumWindows(ScanEnumProc, static_cast<LPARAM>(GetCurrentProcessId()));
}

void CALLBACK PollTimerProc(HWND, UINT, UINT_PTR, DWORD) {
    if (!g_running.load()) return;
    if (++g_tick >= kScanEveryNTicks) {
        g_tick = 0;
        ScanProcessWindows();
    }
    if (!g_byTarget.empty()) SyncAllOverlays(false);
}

void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                           DWORD, DWORD) {
    if (!g_running.load() || !hwnd || idObject != OBJID_WINDOW || idChild != CHILDID_SELF) return;
    if (g_byOverlay.find(hwnd) != g_byOverlay.end()) return;

    auto it = g_byTarget.find(hwnd);
    switch (event) {
        case EVENT_OBJECT_DESTROY:
            if (it != g_byTarget.end()) DestroyOverlayForTarget(hwnd, false);
            break;
        case EVENT_OBJECT_HIDE:
        case EVENT_OBJECT_LOCATIONCHANGE:
            if (it != g_byTarget.end()) SyncOverlay(*it->second, false);
            break;
        case EVENT_OBJECT_SHOW:
            if (it != g_byTarget.end()) SyncOverlay(*it->second, false);
            else AttachOverlayToWindow(hwnd);
            break;
        case EVENT_SYSTEM_FOREGROUND:
            if (it == g_byTarget.end()) AttachOverlayToWindow(hwnd);
            SyncAllOverlays(false);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Worker thread

DWORD WINAPI WorkerThreadProc(LPVOID param) {
    HANDLE readyEvent = static_cast<HANDLE>(param);

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kOverlayClassName;
    RegisterClassExW(&wc);

    DWORD pid = GetCurrentProcessId();
    g_hooks[0] = SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_HIDE, nullptr, WinEventProc,
                                 pid, 0, WINEVENT_OUTOFCONTEXT);
    g_hooks[1] = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, nullptr,
                                 WinEventProc, pid, 0, WINEVENT_OUTOFCONTEXT);
    g_hooks[2] = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
                                 WinEventProc, pid, 0, WINEVENT_OUTOFCONTEXT);
    g_pollTimer = SetTimer(nullptr, 0, kPollIntervalMs, PollTimerProc);

    if (readyEvent) SetEvent(readyEvent);

    ScanProcessWindows();

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!msg.hwnd && msg.message == kMsgReloadSettings) {
            LoadSettings();
            ScanProcessWindows();
            SyncAllOverlays(true);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_running = false;
    for (HWINEVENTHOOK& h : g_hooks) {
        if (h) { UnhookWinEvent(h); h = nullptr; }
    }
    if (g_pollTimer) { KillTimer(nullptr, g_pollTimer); g_pollTimer = 0; }

    for (HWND t : SnapshotTargets()) {
        DestroyOverlayForTarget(t, true);
    }
    g_byOverlay.clear();
    g_byTarget.clear();

    UnregisterClassW(kOverlayClassName, hInst);
    return 0;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Windhawk entry points

BOOL Wh_ModInit() {
    LoadSettings();
    g_running = true;

    HANDLE readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_thread = CreateThread(nullptr, 0, WorkerThreadProc, readyEvent, 0, &g_threadId);
    if (!g_thread) {
        g_running = false;
        if (readyEvent) CloseHandle(readyEvent);
        return FALSE;
    }
    if (readyEvent) {
        WaitForSingleObject(readyEvent, 3000);
        CloseHandle(readyEvent);
    }
    Wh_Log(L"Traffic Light Caption Buttons v1.5 initialized in PID %lu.", GetCurrentProcessId());
    return TRUE;
}

void Wh_ModUninit() {
    g_running = false;
    if (g_thread) {
        PostThreadMessageW(g_threadId, WM_QUIT, 0, 0);
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = nullptr;
        g_threadId = 0;
    }
    Wh_Log(L"Traffic Light Caption Buttons v1.5 unloaded.");
}

void Wh_ModSettingsChanged() {
    if (g_running.load() && g_threadId) {
        PostThreadMessageW(g_threadId, kMsgReloadSettings, 0, 0);
    }
}