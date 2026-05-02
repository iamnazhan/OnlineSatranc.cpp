#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windowsx.h>
#include <commctrl.h>
#include <objidl.h>
#include <propidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "common/Chess.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")

using chess::Board;
using chess::Color;
using chess::Move;
using chess::PieceType;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
//  Pencere / tahta sabitleri
// ─────────────────────────────────────────────────────────────────────────────
constexpr int BOARD_X    = 22;
constexpr int BOARD_Y    = 40;
constexpr int SQ         = 78;
constexpr int BOARD_SIZE = SQ * 8;
constexpr int PANEL_X    = 670;
constexpr int WINDOW_W   = 1010;
constexpr int WINDOW_H   = 720;

// Login / Mod seçim ekranı için geniş merkez pencere
constexpr int LOGIN_W    = 520;
constexpr int LOGIN_H    = 600;
constexpr int MENU_W     = 680;
constexpr int MENU_H     = 520;

constexpr UINT WM_NETLINE = WM_APP + 42;

// ─────────────────────────────────────────────────────────────────────────────
//  Kontrol ID'leri
// ─────────────────────────────────────────────────────────────────────────────
constexpr int ID_BOT        = 1001;
constexpr int ID_LOCAL      = 1002;
constexpr int ID_RESET      = 1003;
constexpr int ID_ONLINE_BTN = 1004;
constexpr int ID_HOST       = 1005;
constexpr int ID_PORT       = 1006;
constexpr int ID_NAME       = 1007;
constexpr int ID_LOGIN_USER = 1008;
constexpr int ID_LOGIN_PASS = 1009;
constexpr int ID_LOGIN      = 1010;
constexpr int ID_REGISTER   = 1011;
constexpr int ID_MENU_BOT   = 1012;
constexpr int ID_MENU_LOCAL = 1013;
constexpr int ID_MENU_ONLINE= 1014;
constexpr int ID_BACK       = 1015;

// ─────────────────────────────────────────────────────────────────────────────
//  Ekran durumu
// ─────────────────────────────────────────────────────────────────────────────
enum class Screen { Login, ModeSelect, Game };
enum class Mode   { Menu, Local, Bot, Online };

Screen g_screen = Screen::Login;

// ─────────────────────────────────────────────────────────────────────────────
//  Global değişkenler
// ─────────────────────────────────────────────────────────────────────────────
HWND g_hwnd           = nullptr;
HWND g_hostEdit       = nullptr;
HWND g_portEdit       = nullptr;
HWND g_nameEdit       = nullptr;
HWND g_loginUserEdit  = nullptr;
HWND g_loginPassEdit  = nullptr;

// Login ekranı kontrolleri
std::vector<HWND> g_loginControls;
// Mod seçim ekranı kontrolleri
std::vector<HWND> g_menuControls;
// Oyun ekranı kontrolleri
std::vector<HWND> g_gameControls;

HFONT g_pieceFont  = nullptr;
HFONT g_textFont   = nullptr;
HFONT g_titleFont  = nullptr;
HFONT g_labelFont  = nullptr;
HFONT g_boldFont   = nullptr;
HFONT g_hugeFont   = nullptr;   // büyük başlık

ULONG_PTR g_gdiplusToken = 0;
std::array<std::unique_ptr<Gdiplus::Image>, 12> g_pieceImages;
HBRUSH g_editBrush  = nullptr;
HBRUSH g_panelBrush = nullptr;

constexpr COLORREF COLOR_BG_APP        = RGB(10, 12, 20);
constexpr COLORREF COLOR_CARD_BG       = RGB(16, 20, 32);
constexpr COLORREF COLOR_CARD_ALT      = RGB(20, 25, 42);
constexpr COLORREF COLOR_CARD_BORDER   = RGB(38, 44, 64);
constexpr COLORREF COLOR_GOLD          = RGB(214, 176, 82);
constexpr COLORREF COLOR_GOLD_DARK     = RGB(170, 132, 48);
constexpr COLORREF COLOR_TEXT_MAIN     = RGB(230, 234, 244);
constexpr COLORREF COLOR_TEXT_MUTED    = RGB(110, 122, 154);
constexpr COLORREF COLOR_TEXT_LABEL    = RGB(146, 154, 182);
constexpr COLORREF COLOR_EDIT_BG       = RGB(21, 28, 40);
constexpr COLORREF COLOR_EDIT_BORDER   = RGB(54, 62, 90);
constexpr COLORREF COLOR_EDIT_TEXT     = RGB(238, 241, 248);

Board g_board;
Mode  g_mode      = Mode::Menu;
Color g_myColor   = Color::White;
bool  g_flipBoard = false;
int   g_selected  = -1;
std::vector<Move> g_selectedMoves;
std::wstring g_extraStatus = L"";
std::wstring g_loginError  = L"";
bool         g_loggedIn    = false;
std::wstring g_loggedUser;

SOCKET g_socket = INVALID_SOCKET;
std::atomic<bool> g_netRunning{false};
std::atomic<bool> g_connecting{false};
std::thread g_netThread;

// ─────────────────────────────────────────────────────────────────────────────
//  Yardımcı: string dönüşüm
// ─────────────────────────────────────────────────────────────────────────────
std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) return L"";
    int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (count <= 0) return L"";
    std::wstring out(static_cast<size_t>(count - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), count);
    return out;
}
std::string wideToUtf8(const std::wstring& text) {
    if (text.empty()) return "";
    int count = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0) return "";
    std::string out(static_cast<size_t>(count - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), count, nullptr, nullptr);
    return out;
}
std::wstring getWindowText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    std::wstring text(static_cast<size_t>(len + 1), L'\0');
    GetWindowTextW(hwnd, text.data(), len + 1);
    text.resize(static_cast<size_t>(len));
    return text;
}

std::wstring exeDirPathWide() {
    wchar_t exe[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring path(exe);
    size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos) path.resize(slash + 1); else path.clear();
    return path;
}

int pieceAssetIndex(int piece) {
    switch (piece) {
    case -4: return 0; case -2: return 1; case -3: return 2; case -5: return 3; case -6: return 4; case -1: return 5;
    case  4: return 6; case  2: return 7; case  3: return 8; case  5: return 9; case  6: return 10; case  1: return 11;
    default: return -1;
    }
}

void loadPieceAssets() {
    const std::wstring base = exeDirPathWide() + L"assets\\pieces\\";
    const wchar_t* files[] = {
        L"b_rook.png", L"b_knight.png", L"b_bishop.png", L"b_queen.png", L"b_king.png", L"b_pawn.png",
        L"w_rook.png", L"w_knight.png", L"w_bishop.png", L"w_queen.png", L"w_king.png", L"w_pawn.png"
    };
    for (size_t i = 0; i < 12; ++i) {
        std::wstring path = base + files[i];
        auto img = std::make_unique<Gdiplus::Image>(path.c_str());
        if (img && img->GetLastStatus() == Gdiplus::Ok) g_pieceImages[i] = std::move(img);
        else g_pieceImages[i].reset();
    }
}

void releasePieceAssets() {
    for (auto& img : g_pieceImages) img.reset();
}

bool drawPieceAsset(HDC hdc, int piece, const RECT& rc) {
    int idx = pieceAssetIndex(piece);
    if (idx < 0 || idx >= 12 || !g_pieceImages[idx]) return false;
    Gdiplus::Graphics graphics(hdc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.DrawImage(g_pieceImages[idx].get(), rc.left + 4, rc.top + 4, (rc.right - rc.left) - 8, (rc.bottom - rc.top) - 8);
    return true;
}

BOOL isPrimaryButtonId(int id) {
    return id == ID_LOGIN || id == ID_MENU_ONLINE || id == ID_ONLINE_BTN;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Yardımcı: dosya / hesap
// ─────────────────────────────────────────────────────────────────────────────
std::string sanitizeLogin(const std::wstring& value) {
    std::string v = wideToUtf8(value);
    std::string out;
    for (char ch : v)
        if (ch != ':' && ch != '\n' && ch != '\r' && out.size() < 40) out.push_back(ch);
    return out;
}
std::string accountsFilePath() {
    char exe[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string path(exe);
    size_t slash = path.find_last_of("\\/");
    if (slash != std::string::npos) path.resize(slash + 1); else path.clear();
    return path + "accounts.txt";
}
bool userExists(const std::string& user, std::string* storedPass = nullptr) {
    std::ifstream in(accountsFilePath());
    std::string line;
    while (std::getline(in, line)) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) continue;
        if (line.substr(0, pos) == user) {
            if (storedPass) *storedPass = line.substr(pos + 1);
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Yardımcı: çizim
// ─────────────────────────────────────────────────────────────────────────────
void fillRoundRect(HDC hdc, RECT r, int radius, COLORREF fill, COLORREF border) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN   pen   = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(hdc, brush);
    HGDIOBJ op = SelectObject(hdc, pen);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(brush);   DeleteObject(pen);
}
void drawHLine(HDC hdc, int x1, int x2, int y, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ op = SelectObject(hdc, pen);
    MoveToEx(hdc, x1, y, nullptr); LineTo(hdc, x2, y);
    SelectObject(hdc, op); DeleteObject(pen);
}
void drawText(HDC hdc, const std::wstring& text, RECT rect, HFONT font,
              COLORREF color, UINT format = DT_LEFT | DT_WORDBREAK) {
    HGDIOBJ of = SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text.c_str(), -1, &rect, format);
    SelectObject(hdc, of);
}

void drawStyledButton(const DRAWITEMSTRUCT* dis) {
    if (!dis || dis->CtlType != ODT_BUTTON) return;
    const int id = static_cast<int>(dis->CtlID);
    const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    const bool focused = (dis->itemState & ODS_FOCUS) != 0;
    RECT rc = dis->rcItem;

    COLORREF fill = RGB(28, 35, 52);
    COLORREF border = RGB(55, 66, 94);
    COLORREF textColor = COLOR_TEXT_MAIN;

    if (id == ID_LOGIN) {
        fill = COLOR_GOLD; border = COLOR_GOLD_DARK; textColor = RGB(28, 24, 16);
    } else if (id == ID_REGISTER) {
        fill = RGB(31, 39, 58); border = RGB(74, 86, 118);
    } else if (id == ID_MENU_BOT) {
        fill = RGB(76, 64, 138); border = RGB(112, 95, 178);
    } else if (id == ID_MENU_LOCAL) {
        fill = RGB(48, 108, 88); border = RGB(72, 144, 118);
    } else if (id == ID_MENU_ONLINE || id == ID_ONLINE_BTN) {
        fill = RGB(53, 102, 198); border = RGB(76, 129, 220);
    } else if (id == ID_RESET) {
        fill = RGB(84, 70, 28); border = RGB(126, 108, 42);
    } else if (id == ID_BACK) {
        fill = RGB(33, 40, 58); border = RGB(71, 82, 112);
    }

    if (pressed) {
        fill = RGB(std::max(0, GetRValue(fill) - 18), std::max(0, GetGValue(fill) - 18), std::max(0, GetBValue(fill) - 18));
        border = RGB(std::max(0, GetRValue(border) - 12), std::max(0, GetGValue(border) - 12), std::max(0, GetBValue(border) - 12));
    }

    HBRUSH back = CreateSolidBrush(COLOR_BG_APP);
    FillRect(dis->hDC, &rc, back);
    DeleteObject(back);
    InflateRect(&rc, -1, -1);
    fillRoundRect(dis->hDC, rc, 16, fill, border);

    if (focused) {
        RECT focusRc = rc;
        InflateRect(&focusRc, -4, -4);
        HPEN focusPen = CreatePen(PS_DOT, 1, RGB(240, 240, 248));
        HGDIOBJ oldPen = SelectObject(dis->hDC, focusPen);
        HGDIOBJ oldBrush = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
        RoundRect(dis->hDC, focusRc.left, focusRc.top, focusRc.right, focusRc.bottom, 12, 12);
        SelectObject(dis->hDC, oldBrush);
        SelectObject(dis->hDC, oldPen);
        DeleteObject(focusPen);
    }

    wchar_t textBuf[128]{};
    GetWindowTextW(dis->hwndItem, textBuf, 127);
    RECT tr = rc;
    drawText(dis->hDC, textBuf, tr, g_textFont, textColor, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ekran boyutu değiştirme yardımcısı
// ─────────────────────────────────────────────────────────────────────────────
void resizeWindow(int w, int h) {
    RECT rc{0, 0, w, h};
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int wx = rc.right - rc.left;
    int wy = rc.bottom - rc.top;
    SetWindowPos(g_hwnd, nullptr,
        (sw - wx) / 2, (sh - wy) / 2,
        wx, wy, SWP_NOZORDER);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Kontrol gizle / göster
// ─────────────────────────────────────────────────────────────────────────────
void showControls(std::vector<HWND>& list, bool visible) {
    for (HWND h : list) ShowWindow(h, visible ? SW_SHOW : SW_HIDE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  setStatus
// ─────────────────────────────────────────────────────────────────────────────
void setStatus(const std::wstring& text) {
    g_extraStatus = text;
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ekran geçişleri
// ─────────────────────────────────────────────────────────────────────────────
void switchScreen(Screen s);   // forward decl

// ─────────────────────────────────────────────────────────────────────────────
//  Login işlemi
// ─────────────────────────────────────────────────────────────────────────────
void handleLogin(bool createAccount) {
    std::string user = sanitizeLogin(getWindowText(g_loginUserEdit));
    std::string pass = sanitizeLogin(getWindowText(g_loginPassEdit));
    if (user.size() < 3 || pass.size() < 3) {
        g_loginError = L"Kullanıcı adı ve şifre en az 3 karakter olmalı.";
        InvalidateRect(g_hwnd, nullptr, FALSE);
        return;
    }
    std::string stored;
    if (createAccount) {
        if (userExists(user)) {
            g_loginError = L"Bu kullanıcı adı zaten var. Giriş yapmayı dene.";
            InvalidateRect(g_hwnd, nullptr, FALSE);
            return;
        }
        std::ofstream out(accountsFilePath(), std::ios::app);
        out << user << ':' << pass << '\n';
        g_loggedIn = true;
    } else {
        if (!userExists(user, &stored) || stored != pass) {
            g_loginError = L"Kullanıcı adı veya şifre yanlış.";
            InvalidateRect(g_hwnd, nullptr, FALSE);
            return;
        }
        g_loggedIn = true;
    }
    g_loggedUser = utf8ToWide(user);
    SetWindowTextW(g_nameEdit, g_loggedUser.c_str());
    g_loginError = L"";
    switchScreen(Screen::ModeSelect);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mod metni
// ─────────────────────────────────────────────────────────────────────────────
std::wstring modeText() {
    switch (g_mode) {
    case Mode::Bot:    return L"Mod: Bota karşı (sen Beyaz)";
    case Mode::Local:  return L"Mod: 2 oyuncu / aynı PC";
    case Mode::Online: return g_myColor == Color::White
                              ? L"Mod: Online (sen Beyaz)"
                              : L"Mod: Online (sen Siyah)";
    default: return L"";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tahta: taş metni
// ─────────────────────────────────────────────────────────────────────────────
std::wstring pieceText(int piece) {
    switch (piece) {
    case  1: return L"\u2659"; case  2: return L"\u2658"; case  3: return L"\u2657";
    case  4: return L"\u2656"; case  5: return L"\u2655"; case  6: return L"\u2654";
    case -1: return L"\u265f"; case -2: return L"\u265e"; case -3: return L"\u265d";
    case -4: return L"\u265c"; case -5: return L"\u265b"; case -6: return L"\u265a";
    default: return L"";
    }
}

int displayToSquare(int dr, int dc) {
    int r = g_flipBoard ? 7 - dr : dr;
    int c = g_flipBoard ? 7 - dc : dc;
    return r * 8 + c;
}
void squareToDisplay(int sq, int& dr, int& dc) {
    int r = sq / 8, c = sq % 8;
    dr = g_flipBoard ? 7 - r : r;
    dc = g_flipBoard ? 7 - c : c;
}
int pointToSquare(int x, int y) {
    if (x < BOARD_X || y < BOARD_Y || x >= BOARD_X+BOARD_SIZE || y >= BOARD_Y+BOARD_SIZE) return -1;
    return displayToSquare((y - BOARD_Y) / SQ, (x - BOARD_X) / SQ);
}
bool canControlCurrentSide() {
    if (g_board.isGameOver()) return false;
    if (g_mode == Mode::Local)  return true;
    if (g_mode == Mode::Bot)    return g_board.sideToMove() == Color::White;
    if (g_mode == Mode::Online) return g_board.sideToMove() == g_myColor;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ağ
// ─────────────────────────────────────────────────────────────────────────────
bool sendLine(const std::string& line) {
    if (g_socket == INVALID_SOCKET) return false;
    std::string data = line;
    if (data.empty() || data.back() != '\n') data.push_back('\n');
    const char* ptr = data.c_str();
    int left = static_cast<int>(data.size());
    while (left > 0) { int s = send(g_socket, ptr, left, 0); if (s <= 0) return false; ptr += s; left -= s; }
    return true;
}
void closeNetwork() {
    g_netRunning = false;
    if (g_socket != INVALID_SOCKET) { shutdown(g_socket, SD_BOTH); closesocket(g_socket); g_socket = INVALID_SOCKET; }
}
void postNetLine(const std::string& line) {
    auto* h = new std::string(line);
    PostMessageW(g_hwnd, WM_NETLINE, 0, reinterpret_cast<LPARAM>(h));
}
void networkLoop() {
    std::string pending;
    char buf[512];
    while (g_netRunning && g_socket != INVALID_SOCKET) {
        int n = recv(g_socket, buf, sizeof(buf), 0);
        if (n <= 0) break;
        pending.append(buf, buf + n);
        size_t pos = 0;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            pending.erase(0, pos + 1);
            postNetLine(line);
        }
    }
    postNetLine("DISCONNECTED");
}

void connectOnline() {
    if (!g_loggedIn)    { setStatus(L"Önce giriş yapmalısın."); return; }
    if (g_connecting)   { setStatus(L"Zaten bağlantı deneniyor..."); return; }
    closeNetwork();
    std::string host       = wideToUtf8(getWindowText(g_hostEdit));
    std::string portText   = wideToUtf8(getWindowText(g_portEdit));
    std::string playerName = wideToUtf8(getWindowText(g_nameEdit));
    if (host.empty())       host       = "127.0.0.1";
    if (portText.empty())   portText   = "5555";
    if (playerName.empty()) playerName = "Player";
    g_connecting = true;
    setStatus(L"Sunucuya bağlanılıyor...");
    std::thread([host, portText, playerName]() {
        WSADATA wsa{}; WSAStartup(MAKEWORD(2,2), &wsa);
        addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
        addrinfo* result = nullptr;
        if (getaddrinfo(host.c_str(), portText.c_str(), &hints, &result) != 0) { g_connecting=false; postNetLine("CONNECT_FAIL Adres çözülemedi."); return; }
        SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (s == INVALID_SOCKET) { freeaddrinfo(result); g_connecting=false; postNetLine("CONNECT_FAIL Socket oluşturulamadı."); return; }
        u_long nb=1; ioctlsocket(s, FIONBIO, &nb);
        int cr = connect(s, result->ai_addr, (int)result->ai_addrlen);
        freeaddrinfo(result);
        bool ok = false;
        if (cr == 0) ok = true;
        else if (WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set ws; FD_ZERO(&ws); FD_SET(s, &ws);
            timeval tv{}; tv.tv_sec = 6;
            int sel = select(0, nullptr, &ws, nullptr, &tv);
            if (sel > 0 && FD_ISSET(s, &ws)) { int err=0,len=sizeof(err); getsockopt(s,SOL_SOCKET,SO_ERROR,(char*)&err,&len); ok=(err==0); }
        }
        nb=0; ioctlsocket(s, FIONBIO, &nb);
        if (!ok) { closesocket(s); g_connecting=false; postNetLine("CONNECT_FAIL Sunucuya bağlanamadı."); return; }
        g_socket=s; g_connecting=false; postNetLine("CONNECTED "+playerName);
    }).detach();
}

void clearSelection() { g_selected=-1; g_selectedMoves.clear(); }

Move preferQueenPromotion(const std::vector<Move>& moves, int to) {
    Move fallback; bool has=false;
    for (const auto& m : moves) {
        if (m.to != to) continue;
        if (!has) { fallback=m; has=true; }
        if (m.promotion == PieceType::Queen) return m;
    }
    return fallback;
}

void maybeBotMove() {
    if (g_mode != Mode::Bot || g_board.isGameOver() || g_board.sideToMove() != Color::Black) return;
    setStatus(L"Bot düşünüyor...");
    auto best = chess::findBestBotMove(g_board, 3);
    if (best) { g_board.makeMove(*best); setStatus(L"Bot hamlesi: " + utf8ToWide(Board::moveToUci(*best))); }
}

void chooseMode(Mode mode) {
    closeNetwork();
    g_mode = mode;
    g_board.reset();
    g_flipBoard = false;
    g_myColor = Color::White;
    clearSelection();
    switchScreen(Screen::Game);
    if (mode == Mode::Bot)   setStatus(L"Bot modu başladı. Sen Beyaz taşlarla oynuyorsun.");
    if (mode == Mode::Local) setStatus(L"Aynı bilgisayarda 2 oyuncu modu başladı.");
    if (mode == Mode::Online) setStatus(L"Sunucu bilgilerini gir ve Online Bağlan'a bas.");
}

void clickSquare(int square) {
    if (square < 0 || g_mode == Mode::Menu) return;
    if (!canControlCurrentSide()) return;
    const int piece = g_board.at(square);
    const bool own = piece != 0 && Board::pieceColor(piece) == g_board.sideToMove();
    if (g_selected < 0) {
        if (own) { g_selected = square; g_selectedMoves = g_board.legalMovesFrom(square); }
        InvalidateRect(g_hwnd, nullptr, FALSE); return;
    }
    if (own && square != g_selected) {
        g_selected = square; g_selectedMoves = g_board.legalMovesFrom(square);
        InvalidateRect(g_hwnd, nullptr, FALSE); return;
    }
    bool canMove = false;
    for (const auto& m : g_selectedMoves) if (m.to == square) { canMove=true; break; }
    if (canMove) {
        Move mv = preferQueenPromotion(g_selectedMoves, square);
        std::string uci = Board::moveToUci(mv);
        if (g_board.makeMove(mv)) {
            clearSelection();
            if (g_mode == Mode::Online) { sendLine("MOVE "+uci); setStatus(L"Hamle gönderildi: "+utf8ToWide(uci)); }
            else { setStatus(utf8ToWide(g_board.statusText())); maybeBotMove(); }
        }
    } else clearSelection();
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ÇİZİM: Login Ekranı
// ─────────────────────────────────────────────────────────────────────────────
void drawLoginScreen(HDC hdc) {
    // Arka plan
    RECT full{0, 0, LOGIN_W, LOGIN_H};
    HBRUSH bg = CreateSolidBrush(RGB(10, 12, 20));
    FillRect(hdc, &full, bg); DeleteObject(bg);

    // Köşeli süsleme çizgileri (sağ üst, sol alt)
    HPEN deco = CreatePen(PS_SOLID, 1, RGB(201, 169, 78));
    HGDIOBJ op = SelectObject(hdc, deco);
    // Sol üst köşe
    MoveToEx(hdc, 20, 40, nullptr); LineTo(hdc, 60, 40);
    MoveToEx(hdc, 20, 40, nullptr); LineTo(hdc, 20, 80);
    // Sağ alt köşe
    MoveToEx(hdc, LOGIN_W-20, LOGIN_H-40, nullptr); LineTo(hdc, LOGIN_W-60, LOGIN_H-40);
    MoveToEx(hdc, LOGIN_W-20, LOGIN_H-40, nullptr); LineTo(hdc, LOGIN_W-20, LOGIN_H-80);
    SelectObject(hdc, op); DeleteObject(deco);

    // Kart arka planı (orta)
    int cx = LOGIN_W / 2;
    RECT card{cx - 210, 60, cx + 210, LOGIN_H - 40};
    fillRoundRect(hdc, card, 20, RGB(16, 20, 32), RGB(38, 44, 64));

    // Header şeridi
    RECT hdr{cx-210, 60, cx+210, 148};
    fillRoundRect(hdc, hdr, 20, RGB(20, 25, 42), RGB(38, 44, 64));
    drawHLine(hdc, cx-210, cx+210, 148, RGB(44, 50, 72));

    // Taç + başlık
    RECT crownRc{cx-190, 68, cx-148, 108};
    drawText(hdc, L"\u265a", crownRc, g_hugeFont, RGB(210, 175, 72), DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    RECT titleRc{cx-142, 68, cx+200, 108};
    drawText(hdc, L"Royal Chess Arena", titleRc, g_titleFont, RGB(228, 196, 108), DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    RECT subRc{cx-142, 110, cx+200, 134};
    drawText(hdc, L"Giriş yap, profilini hazırla ve oyuna zarif bir başlangıç yap.", subRc, g_labelFont, RGB(102, 112, 144), DT_LEFT|DT_SINGLELINE);

    // Input etiketleri
    int iy = 168;
    RECT uLbl{cx-190, iy, cx+190, iy+18};
    drawText(hdc, L"KULLANICI ADI", uLbl, g_labelFont, RGB(120, 130, 160), DT_LEFT|DT_SINGLELINE);
    iy += 52;
    RECT pLbl{cx-190, iy, cx+190, iy+18};
    drawText(hdc, L"\u015c\u0130FRE", pLbl, g_labelFont, RGB(120, 130, 160), DT_LEFT|DT_SINGLELINE);

    // Hata mesajı
    if (!g_loginError.empty()) {
        RECT errBg{cx-190, 316, cx+190, 348};
        fillRoundRect(hdc, errBg, 8, RGB(50, 16, 16), RGB(140, 40, 40));
        RECT errRc{cx-182, 318, cx+182, 346};
        drawText(hdc, L"\u26a0  " + g_loginError, errRc, g_textFont, RGB(240, 120, 120), DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    // Alt ayırıcı
    drawHLine(hdc, cx-190, cx+190, 362, RGB(38, 44, 64));

    // Alt açıklama
    RECT tip{cx-190, 372, cx+190, 400};
    drawText(hdc, L"İlk kez geliyorsan \"Hesap Oluştur\" ile saniyeler içinde kayıt olabilirsin.", tip, g_labelFont, RGB(84, 94, 124), DT_CENTER|DT_SINGLELINE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ÇİZİM: Mod Seçim Ekranı
// ─────────────────────────────────────────────────────────────────────────────
void drawMenuScreen(HDC hdc) {
    RECT full{0, 0, MENU_W, MENU_H};
    HBRUSH bg = CreateSolidBrush(RGB(10, 12, 20));
    FillRect(hdc, &full, bg); DeleteObject(bg);

    int cx = MENU_W / 2;

    // Ana kart
    RECT card{30, 20, MENU_W-30, MENU_H-20};
    fillRoundRect(hdc, card, 20, RGB(16, 20, 32), RGB(38, 44, 64));

    // Header
    RECT hdr{30, 20, MENU_W-30, 110};
    fillRoundRect(hdc, hdr, 20, RGB(20, 25, 42), RGB(38, 44, 64));
    drawHLine(hdc, 30, MENU_W-30, 110, RGB(44, 50, 72));

    RECT crownRc{54, 28, 94, 70};
    drawText(hdc, L"\u265a", crownRc, g_hugeFont, RGB(210, 175, 72), DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    RECT titleRc{98, 28, MENU_W-50, 70};
    drawText(hdc, L"Merhaba, " + g_loggedUser + L"!", titleRc, g_titleFont, RGB(228, 196, 108), DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    RECT subRc{98, 72, MENU_W-50, 96};
    drawText(hdc, L"Aşağıdan oyun deneyimini seç ve karşılaşmaya başla.", subRc, g_labelFont, RGB(102, 112, 144), DT_LEFT|DT_SINGLELINE);

    // "OYUN MODU SEÇ" label
    RECT secLbl{54, 122, MENU_W-54, 142};
    drawText(hdc, L"OYUN MODLARI", secLbl, g_labelFont, RGB(180, 148, 60), DT_LEFT|DT_SINGLELINE);
    drawHLine(hdc, 54+128, MENU_W-54, 132, RGB(50, 56, 78));

    // Mod kart açıklamaları (butonların üstüne)
    // Bota Karşı: x=54, y=148 → +180wide +90tall
    RECT b1bg{54, 148, 234, 248};
    fillRoundRect(hdc, b1bg, 14, RGB(18, 22, 36), RGB(42, 48, 70));
    RECT b1ic{60, 152, 228, 200};
    drawText(hdc, L"\U0001F916", b1ic, g_boldFont, RGB(200, 200, 220), DT_CENTER|DT_SINGLELINE|DT_VCENTER);
    RECT b1lbl{54, 200, 234, 218};
    drawText(hdc, L"YAPAY ZEKA", b1lbl, g_labelFont, RGB(140, 148, 180), DT_CENTER|DT_SINGLELINE);

    // 2 Oyuncu: x=254, y=148
    RECT b2bg{254, 148, 434, 248};
    fillRoundRect(hdc, b2bg, 14, RGB(18, 22, 36), RGB(42, 48, 70));
    RECT b2ic{260, 152, 428, 200};
    drawText(hdc, L"\U0001F465", b2ic, g_boldFont, RGB(200, 200, 220), DT_CENTER|DT_SINGLELINE|DT_VCENTER);
    RECT b2lbl{254, 200, 434, 218};
    drawText(hdc, L"YEREL DÜELLO", b2lbl, g_labelFont, RGB(140, 148, 180), DT_CENTER|DT_SINGLELINE);

    // Online: x=454, y=148
    RECT b3bg{454, 148, 634, 248};
    fillRoundRect(hdc, b3bg, 14, RGB(18, 22, 36), RGB(42, 48, 70));
    RECT b3ic{460, 152, 628, 200};
    drawText(hdc, L"\U0001F310", b3ic, g_boldFont, RGB(200, 200, 220), DT_CENTER|DT_SINGLELINE|DT_VCENTER);
    RECT b3lbl{454, 200, 634, 218};
    drawText(hdc, L"ÇEVRİM İÇİ", b3lbl, g_labelFont, RGB(140, 148, 180), DT_CENTER|DT_SINGLELINE);

    // Ayırıcı
    drawHLine(hdc, 54, MENU_W-54, 360, RGB(38, 44, 64));
    RECT footRc{54, 368, MENU_W-54, 390};
    drawText(hdc, L"Geri dön ile hesabından güvenle çıkış yapabilirsin.", footRc, g_labelFont, RGB(76, 86, 116), DT_CENTER|DT_SINGLELINE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ÇİZİM: Oyun Ekranı (tahta + sağ panel)
// ─────────────────────────────────────────────────────────────────────────────
void drawBoardAndPanel(HDC hdc) {
    // Arka plan
    RECT full{0, 0, WINDOW_W, WINDOW_H};
    HBRUSH bg = CreateSolidBrush(RGB(10, 12, 20));
    FillRect(hdc, &full, bg); DeleteObject(bg);

    // ── TAHTA ──────────────────────────────────────────────────────
    HPEN outerPen = CreatePen(PS_SOLID, 3, RGB(22, 25, 38));
    HGDIOBJ op = SelectObject(hdc, outerPen);
    HGDIOBJ ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, BOARD_X-2, BOARD_Y-2, BOARD_X+BOARD_SIZE+2, BOARD_Y+BOARD_SIZE+2);
    SelectObject(hdc, ob); SelectObject(hdc, op); DeleteObject(outerPen);

    for (int dr=0; dr<8; ++dr) for (int dc=0; dc<8; ++dc) {
        int sq = displayToSquare(dr, dc);
        bool light = ((dr+dc)%2)==0;
        COLORREF c = light ? RGB(236, 232, 208) : RGB(115, 148, 82);
        if (sq == g_selected) c = RGB(244, 236, 80);
        HBRUSH br = CreateSolidBrush(c);
        RECT rc{BOARD_X+dc*SQ, BOARD_Y+dr*SQ, BOARD_X+(dc+1)*SQ, BOARD_Y+(dr+1)*SQ};
        FillRect(hdc, &rc, br); DeleteObject(br);
    }
    for (const auto& m : g_selectedMoves) {
        int dr=0, dc=0; squareToDisplay(m.to, dr, dc);
        int cx2 = BOARD_X+dc*SQ+SQ/2, cy2 = BOARD_Y+dr*SQ+SQ/2;
        HBRUSH dot = CreateSolidBrush(g_board.at(m.to)==0 ? RGB(50,75,50) : RGB(140,40,40));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(30,30,30));
        op = SelectObject(hdc, dot); HGDIOBJ op2 = SelectObject(hdc, pen);
        int r = g_board.at(m.to)==0 ? 9 : 17;
        Ellipse(hdc, cx2-r, cy2-r, cx2+r, cy2+r);
        SelectObject(hdc, op); SelectObject(hdc, op2); DeleteObject(dot); DeleteObject(pen);
    }
    HGDIOBJ of = SelectObject(hdc, g_pieceFont);
    SetBkMode(hdc, TRANSPARENT);
    for (int sq=0; sq<64; ++sq) {
        int piece = g_board.at(sq); if (!piece) continue;
        int dr=0, dc=0; squareToDisplay(sq, dr, dc);
        RECT rc{BOARD_X+dc*SQ, BOARD_Y+dr*SQ, BOARD_X+(dc+1)*SQ, BOARD_Y+(dr+1)*SQ};
        if (!drawPieceAsset(hdc, piece, rc)) {
            RECT textRc{rc.left, rc.top-3, rc.right, rc.bottom+3};
            std::wstring t = pieceText(piece);
            RECT sh = textRc; OffsetRect(&sh, 2, 2);
            SetTextColor(hdc, RGB(40,40,40));
            DrawTextW(hdc, t.c_str(), -1, &sh, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            SetTextColor(hdc, piece>0 ? RGB(248,244,232) : RGB(14,14,14));
            DrawTextW(hdc, t.c_str(), -1, &textRc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }
    }
    SelectObject(hdc, of);
    // Koordinatlar
    SelectObject(hdc, g_labelFont);
    SetTextColor(hdc, RGB(170, 176, 200)); SetBkMode(hdc, TRANSPARENT);
    for (int i=0; i<8; ++i) {
        wchar_t file = (wchar_t)((g_flipBoard ? L'h' : L'a') + (g_flipBoard ? -i : i));
        wchar_t rank = (wchar_t)((g_flipBoard ? L'1' : L'8') + (g_flipBoard ? i : -i));
        RECT fr{BOARD_X+i*SQ, BOARD_Y+BOARD_SIZE+4, BOARD_X+(i+1)*SQ, BOARD_Y+BOARD_SIZE+22};
        DrawTextW(hdc, std::wstring(1,file).c_str(), -1, &fr, DT_CENTER|DT_SINGLELINE);
        RECT rr{BOARD_X-20, BOARD_Y+i*SQ+22, BOARD_X-4, BOARD_Y+(i+1)*SQ};
        DrawTextW(hdc, std::wstring(1,rank).c_str(), -1, &rr, DT_CENTER|DT_SINGLELINE);
    }

    // ── SAĞ PANEL ──────────────────────────────────────────────────
    const int PX  = PANEL_X - 6;
    const int PX2 = WINDOW_W - 8;
    const int CX  = PANEL_X + 8;
    const int CX2 = WINDOW_W - 16;
    const int CW  = CX2 - CX;

    RECT panelRect{PX, 10, PX2, WINDOW_H-10};
    fillRoundRect(hdc, panelRect, 16, RGB(16, 19, 28), RGB(36, 42, 60));

    // Header
    RECT hdrBg{PX, 10, PX2, 82};
    fillRoundRect(hdc, hdrBg, 16, RGB(20, 24, 38), RGB(36, 42, 60));
    drawHLine(hdc, PX, PX2, 82, RGB(44, 50, 70));
    RECT crownRc2{CX, 16, CX+34, 54};
    drawText(hdc, L"\u265a", crownRc2, g_titleFont, RGB(210, 175, 72), DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    RECT titleRc2{CX+36, 14, CX2, 50};
    drawText(hdc, g_loggedUser, titleRc2, g_titleFont, RGB(228, 196, 108), DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    RECT subRc2{CX+36, 52, CX2, 74};
    drawText(hdc, modeText(), subRc2, g_labelFont, RGB(100, 108, 140), DT_LEFT|DT_SINGLELINE);

    // Durum kutusu
    auto drawSec = [&](const wchar_t* lbl, int y) {
        RECT lr{CX, y, CX+160, y+15};
        drawText(hdc, lbl, lr, g_labelFont, RGB(170, 142, 55), DT_LEFT|DT_SINGLELINE);
        SIZE sz{}; HGDIOBJ of2 = SelectObject(hdc, g_labelFont);
        GetTextExtentPoint32W(hdc, lbl, (int)wcslen(lbl), &sz);
        SelectObject(hdc, of2);
        drawHLine(hdc, CX+sz.cx+6, CX2, y+7, RGB(44, 50, 70));
    };

    drawSec(L"DURUM", 92);
    RECT stBg{CX, 110, CX2, 196};
    fillRoundRect(hdc, stBg, 8, RGB(18, 22, 34), RGB(44, 50, 70));
    HBRUSH abar = CreateSolidBrush(RGB(175, 143, 55));
    RECT abrc{CX, 110, CX+4, 196}; FillRect(hdc, &abrc, abar); DeleteObject(abar);
    RECT stRc{CX+10, 116, CX2-4, 192};
    std::wstring stText = utf8ToWide(g_board.statusText()) + L"\n" + g_extraStatus;
    drawText(hdc, stText, stRc, g_textFont, RGB(212, 218, 235));

    // Online bağlantı etiketleri
    drawSec(L"ONL\u0130NE BA\u011eLANTI", 282);
    const int ipW2 = CW - 86 - 8;
    RECT ipLbl{CX, 300, CX+ipW2, 316};
    drawText(hdc, L"SUNUCU IP / ADRES", ipLbl, g_labelFont, RGB(100, 108, 140), DT_LEFT|DT_SINGLELINE);
    RECT portLbl2{CX+ipW2+8, 300, CX2, 316};
    drawText(hdc, L"PORT", portLbl2, g_labelFont, RGB(100, 108, 140), DT_LEFT|DT_SINGLELINE);
    RECT nameLbl2{CX, 354, CX2, 370};
    drawText(hdc, L"PROFİL ADI", nameLbl2, g_labelFont, RGB(100, 108, 140), DT_LEFT|DT_SINGLELINE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Kontrol oluşturma
// ─────────────────────────────────────────────────────────────────────────────
void createAllControls(HWND hwnd) {
    // ── Fontlar ───────────────────────────────────────────────────
    g_hugeFont  = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    g_titleFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    g_boldFont  = CreateFontW(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    g_textFont  = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    g_labelFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
    g_pieceFont = CreateFontW(54, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH|FF_SWISS, L"Segoe UI Symbol");

    int lcx = LOGIN_W / 2;

    // ── LOGIN kontrolleri ─────────────────────────────────────────
    g_loginUserEdit = CreateWindowW(L"EDIT", L"",
        WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
        lcx-190, 188, 380, 34, hwnd,
        reinterpret_cast<HMENU>(ID_LOGIN_USER), nullptr, nullptr);
    g_loginPassEdit = CreateWindowW(L"EDIT", L"",
        WS_CHILD|WS_BORDER|ES_AUTOHSCROLL|ES_PASSWORD,
        lcx-190, 242, 380, 34, hwnd,
        reinterpret_cast<HMENU>(ID_LOGIN_PASS), nullptr, nullptr);
    HWND loginBtn = CreateWindowW(L"BUTTON", L"Giriş Yap",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        lcx-190, 290, 183, 40, hwnd,
        reinterpret_cast<HMENU>(ID_LOGIN), nullptr, nullptr);
    HWND regBtn = CreateWindowW(L"BUTTON", L"Hesap Oluştur",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        lcx+7, 290, 183, 40, hwnd,
        reinterpret_cast<HMENU>(ID_REGISTER), nullptr, nullptr);
    g_loginControls = {g_loginUserEdit, g_loginPassEdit, loginBtn, regBtn};

    // ── MENU kontrolleri ──────────────────────────────────────────
    HWND mBot = CreateWindowW(L"BUTTON", L"Bot'a Karşı",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        54, 258, 180, 44, hwnd,
        reinterpret_cast<HMENU>(ID_MENU_BOT), nullptr, nullptr);
    HWND mLocal = CreateWindowW(L"BUTTON", L"Yerel 2 Oyuncu",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        254, 258, 180, 44, hwnd,
        reinterpret_cast<HMENU>(ID_MENU_LOCAL), nullptr, nullptr);
    HWND mOnline = CreateWindowW(L"BUTTON", L"Online Oyna",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        454, 258, 180, 44, hwnd,
        reinterpret_cast<HMENU>(ID_MENU_ONLINE), nullptr, nullptr);
    HWND backBtn = CreateWindowW(L"BUTTON", L"←  Geri Dön",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        54, 400, 200, 36, hwnd,
        reinterpret_cast<HMENU>(ID_BACK), nullptr, nullptr);
    g_menuControls = {mBot, mLocal, mOnline, backBtn};

    // ── GAME kontrolleri ──────────────────────────────────────────
    const int GCX = PANEL_X + 8;
    const int GCX2 = WINDOW_W - 16;
    const int GCW = GCX2 - GCX;
    const int ipW3 = GCW - 86 - 8;

    g_hostEdit = CreateWindowW(L"EDIT", L"127.0.0.1",
        WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
        GCX, 318, ipW3, 28, hwnd,
        reinterpret_cast<HMENU>(ID_HOST), nullptr, nullptr);
    g_portEdit = CreateWindowW(L"EDIT", L"5555",
        WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
        GCX+ipW3+8, 318, 78, 28, hwnd,
        reinterpret_cast<HMENU>(ID_PORT), nullptr, nullptr);
    g_nameEdit = CreateWindowW(L"EDIT", L"Player",
        WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
        GCX, 372, GCW, 28, hwnd,
        reinterpret_cast<HMENU>(ID_NAME), nullptr, nullptr);
    HWND onlineBtn2 = CreateWindowW(L"BUTTON", L"⚡  Sunucuya Bağlan",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        GCX, 408, GCW, 36, hwnd,
        reinterpret_cast<HMENU>(ID_ONLINE_BTN), nullptr, nullptr);
    HWND resetBtn = CreateWindowW(L"BUTTON", L"↺  Oyunu Sıfırla",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        GCX, 206, GCW, 36, hwnd,
        reinterpret_cast<HMENU>(ID_RESET), nullptr, nullptr);
    HWND menuBtn2 = CreateWindowW(L"BUTTON", L"←  Ana Menü",
        WS_CHILD|BS_OWNERDRAW|WS_TABSTOP,
        GCX, 460, GCW, 32, hwnd,
        reinterpret_cast<HMENU>(ID_BACK), nullptr, nullptr);
    g_gameControls = {g_hostEdit, g_portEdit, g_nameEdit, onlineBtn2, resetBtn, menuBtn2};

    // Fontları uygula
    for (HWND h : {g_loginUserEdit, g_loginPassEdit, g_hostEdit, g_portEdit, g_nameEdit,
                   loginBtn, regBtn, mBot, mLocal, mOnline, backBtn, onlineBtn2, resetBtn, menuBtn2})
        SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(g_textFont), TRUE);

    SendMessageW(g_loginUserEdit, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"ör. ali_nazhan"));
    SendMessageW(g_loginPassEdit, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"şifrenizi girin"));
    SendMessageW(g_hostEdit, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"ör. 127.0.0.1 veya sunucu IP"));
    SendMessageW(g_portEdit, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"5555"));
    SendMessageW(g_nameEdit, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"oyuncu takma adı"));

    if (!g_editBrush)  g_editBrush  = CreateSolidBrush(COLOR_EDIT_BG);
    if (!g_panelBrush) g_panelBrush = CreateSolidBrush(COLOR_BG_APP);

    // Hepsini gizle — switchScreen gösterecek
    showControls(g_loginControls, false);
    showControls(g_menuControls, false);
    showControls(g_gameControls, false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ekran geçişi
// ─────────────────────────────────────────────────────────────────────────────
void switchScreen(Screen s) {
    g_screen = s;
    showControls(g_loginControls, false);
    showControls(g_menuControls,  false);
    showControls(g_gameControls,  false);

    if (s == Screen::Login) {
        resizeWindow(LOGIN_W, LOGIN_H);
        showControls(g_loginControls, true);
        SetFocus(g_loginUserEdit);
    } else if (s == Screen::ModeSelect) {
        resizeWindow(MENU_W, MENU_H);
        showControls(g_menuControls, true);
    } else { // Game
        resizeWindow(WINDOW_W, WINDOW_H);
        showControls(g_gameControls, true);
        SetFocus(g_hostEdit);
    }
    InvalidateRect(g_hwnd, nullptr, TRUE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ağ olayları
// ─────────────────────────────────────────────────────────────────────────────
void handleNetworkLine(const std::string& line) {
    if (line.rfind("CONNECT_FAIL ", 0) == 0) { setStatus(utf8ToWide(line.substr(13))); return; }
    if (line.rfind("CONNECTED ", 0) == 0) {
        std::string playerName = line.substr(10);
        g_mode = Mode::Online; g_flipBoard=false; g_myColor=Color::White;
        clearSelection(); g_board.reset();
        sendLine("HELLO "+playerName);
        setStatus(L"Bağlandı. Eşleşme bekleniyor...");
        g_netRunning = true;
        g_netThread = std::thread(networkLoop); g_netThread.detach();
        return;
    }
    if (line == "DISCONNECTED") { if (g_mode==Mode::Online) setStatus(L"Bağlantı kesildi."); return; }
    if (line.rfind("COLOR ",0)==0) {
        char c = line.size()>=7 ? line[6] : 'W';
        g_myColor = (c=='B') ? Color::Black : Color::White;
        g_flipBoard = g_myColor == Color::Black;
        setStatus(g_myColor==Color::White ? L"Renginiz: Beyaz" : L"Renginiz: Siyah");
        return;
    }
    if (line=="START") { g_board.reset(); clearSelection(); setStatus(L"Online maç başladı."); return; }
    if (line.rfind("MOVE ",0)==0) {
        std::string uci = line.substr(5);
        if (g_board.makeMoveUci(uci)) { clearSelection(); setStatus(L"Rakip hamlesi: "+utf8ToWide(uci)); }
        else setStatus(L"Geçersiz hamle: "+utf8ToWide(uci));
        return;
    }
    if (line.rfind("OK",0)==0)  { setStatus(L"Hamle onaylandı."); return; }
    if (line.rfind("ERR ",0)==0){ setStatus(L"Sunucu hatası: "+utf8ToWide(line.substr(4))); return; }
    if (line.rfind("MSG ",0)==0){ setStatus(utf8ToWide(line.substr(4))); return; }
    setStatus(L"Sunucu: "+utf8ToWide(line));
}

// ─────────────────────────────────────────────────────────────────────────────
//  WndProc
// ─────────────────────────────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        createAllControls(hwnd);
        loadPieceAssets();
        switchScreen(Screen::Login);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_LOGIN:      handleLogin(false); return 0;
        case ID_REGISTER:   handleLogin(true);  return 0;
        case ID_MENU_BOT:   chooseMode(Mode::Bot);   return 0;
        case ID_MENU_LOCAL: chooseMode(Mode::Local);  return 0;
        case ID_MENU_ONLINE:chooseMode(Mode::Online); return 0;
        case ID_BACK:
            if (g_screen == Screen::Game) {
                closeNetwork();
                g_mode = Mode::Menu;
                switchScreen(Screen::ModeSelect);
            } else if (g_screen == Screen::ModeSelect) {
                g_loggedIn = false;
                g_loggedUser = L"";
                switchScreen(Screen::Login);
            }
            return 0;
        case ID_RESET:
            if (g_mode==Mode::Online) setStatus(L"Online oyunda yeniden başlatmak için tekrar bağlan.");
            else if (g_mode==Mode::Bot||g_mode==Mode::Local) chooseMode(g_mode);
            return 0;
        case ID_ONLINE_BTN: connectOnline(); return 0;
        default: break;
        }
        break;

    case WM_LBUTTONDOWN: {
        if (g_screen != Screen::Game) break;
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        clickSquare(pointToSquare(x, y));
        return 0;
    }
    case WM_NETLINE: {
        auto* line = reinterpret_cast<std::string*>(lParam);
        if (line) { handleNetworkLine(*line); delete line; }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_DRAWITEM:
        drawStyledButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    case WM_CTLCOLOREDIT: {
        HDC editDc = reinterpret_cast<HDC>(wParam);
        SetTextColor(editDc, COLOR_EDIT_TEXT);
        SetBkColor(editDc, COLOR_EDIT_BG);
        return reinterpret_cast<INT_PTR>(g_editBrush ? g_editBrush : GetStockObject(BLACK_BRUSH));
    }

    case WM_CTLCOLORSTATIC: {
        HDC staticDc = reinterpret_cast<HDC>(wParam);
        SetTextColor(staticDc, COLOR_TEXT_MAIN);
        SetBkMode(staticDc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(g_panelBrush ? g_panelBrush : GetStockObject(NULL_BRUSH));
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        int W = (g_screen==Screen::Login) ? LOGIN_W
              : (g_screen==Screen::ModeSelect) ? MENU_W : WINDOW_W;
        int H = (g_screen==Screen::Login) ? LOGIN_H
              : (g_screen==Screen::ModeSelect) ? MENU_H : WINDOW_H;
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, W, H);
        HGDIOBJ old = SelectObject(mem, bmp);
        if      (g_screen==Screen::Login)      drawLoginScreen(mem);
        else if (g_screen==Screen::ModeSelect) drawMenuScreen(mem);
        else                                   drawBoardAndPanel(mem);
        BitBlt(hdc, 0, 0, W, H, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        closeNetwork();
        releasePieceAssets();
        if (g_pieceFont)  DeleteObject(g_pieceFont);
        if (g_textFont)   DeleteObject(g_textFont);
        if (g_titleFont)  DeleteObject(g_titleFont);
        if (g_boldFont)   DeleteObject(g_boldFont);
        if (g_labelFont)  DeleteObject(g_labelFont);
        if (g_hugeFont)   DeleteObject(g_hugeFont);
        if (g_editBrush)  DeleteObject(g_editBrush);
        if (g_panelBrush) DeleteObject(g_panelBrush);
        if (g_gdiplusToken) Gdiplus::GdiplusShutdown(g_gdiplusToken);
        WSACleanup();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
//  wWinMain
// ─────────────────────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    SetProcessDPIAware();
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
    const wchar_t CLASS[] = L"OnlineChessCppWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);

    // İlk pencere login boyutunda açılır
    RECT rc{0, 0, LOGIN_W, LOGIN_H};
    AdjustWindowRect(&rc, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, FALSE);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    HWND hwnd = CreateWindowExW(0, CLASS, L"Online Satranç",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        (sw-(rc.right-rc.left))/2, (sh-(rc.bottom-rc.top))/2,
        rc.right-rc.left, rc.bottom-rc.top,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
