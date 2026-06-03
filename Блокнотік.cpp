#define _RICHEDIT_VER 0x0500

#include <windows.h>
#include <commdlg.h>
#include <richedit.h>
#include <cwchar>
#include <string>
#include <vector>

#pragma comment(lib, "Comdlg32.lib")

const int ID_BUTTON_NEW = 101;
const int ID_BUTTON_OPEN = 102;
const int ID_BUTTON_SAVE = 103;
const int ID_BUTTON_COLOR = 104;
const int ID_BUTTON_SIZE = 105;
const int ID_BUTTON_SELECT_ALL = 107;
const int ID_BUTTON_LIST_PAREN = 108;
const int ID_BUTTON_LIST_DOT = 109;
const int ID_BUTTON_LIST_SUB = 110;
const int ID_BUTTON_THEME = 111;
const int ID_FONT_SIZE_FIRST = 200;

COLORREF g_appBackgroundColor = RGB(232, 238, 235);
COLORREF g_textColor = RGB(33, 47, 51);
COLORREF g_editorColor = RGB(255, 252, 241);
COLORREF g_panelColor = RGB(247, 248, 253);
COLORREF g_buttonColor = RGB(244, 245, 251);
COLORREF g_borderColor = RGB(207, 210, 224);
COLORREF g_accentColor = RGB(137, 88, 235);
COLORREF g_mutedTextColor = RGB(114, 118, 133);

enum class NumberStyle
{
    Parenthesis,
    Dot,
    SubPoint
};

HWND g_mainWindow = nullptr;
HWND g_themeButton = nullptr;
HWND g_titleLabel = nullptr;
HWND g_editBox = nullptr;
HWND g_newButton = nullptr;
HWND g_openButton = nullptr;
HWND g_saveButton = nullptr;
HWND g_colorButton = nullptr;
HWND g_sizeButton = nullptr;
HWND g_selectAllButton = nullptr;
HWND g_listParenButton = nullptr;
HWND g_listDotButton = nullptr;
HWND g_listSubButton = nullptr;

HFONT g_titleFont = nullptr;
HFONT g_buttonFont = nullptr;
HBRUSH g_backgroundBrush = nullptr;
HBRUSH g_windowClassBrush = nullptr;
HMODULE g_richEditLibrary = nullptr;
bool g_darkThemeEnabled = false;
bool g_autoNumberingEnabled = false;
NumberStyle g_autoNumberingStyle = NumberStyle::Parenthesis;
int g_nextNumber = 1;
std::vector<std::wstring> g_noteTexts(3);
int g_activeNoteIndex = 0;
const int g_maxNotes = 6;

void UpdateNumberingButtons();

void CreateAppFonts()
{
    g_titleFont = CreateFont(
        28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    g_buttonFont = CreateFont(
        16, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

void ApplyWindowLayout(HWND hwnd)
{
    RECT client = {};
    GetClientRect(hwnd, &client);

    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    const int padding = 18;
    const int railWidth = 126;
    const int topArea = 118;
    const int buttonWidth = 96;
    const int smallButtonWidth = 58;
    const int buttonHeight = 34;
    const int buttonGap = 10;

    MoveWindow(g_themeButton, padding + 14, 88, 92, buttonHeight, TRUE);
    MoveWindow(g_titleLabel, railWidth + padding + 24, 28, 320, 38, TRUE);

    const int firstRowX = width - padding - buttonWidth * 3 - buttonGap * 2;
    MoveWindow(g_newButton, firstRowX, 28, buttonWidth, buttonHeight, TRUE);
    MoveWindow(g_openButton, firstRowX + buttonWidth + buttonGap, 28, buttonWidth, buttonHeight, TRUE);
    MoveWindow(g_saveButton, firstRowX + (buttonWidth + buttonGap) * 2, 28, buttonWidth, buttonHeight, TRUE);

    int x = railWidth + padding + 24;
    const int secondRowY = 78;
    MoveWindow(g_colorButton, x, secondRowY, buttonWidth, buttonHeight, TRUE);
    x += buttonWidth + buttonGap;
    MoveWindow(g_sizeButton, x, secondRowY, buttonWidth, buttonHeight, TRUE);
    x += buttonWidth + buttonGap;
    MoveWindow(g_selectAllButton, x, secondRowY, buttonWidth, buttonHeight, TRUE);
    x += buttonWidth + buttonGap;
    MoveWindow(g_listParenButton, x, secondRowY, smallButtonWidth, buttonHeight, TRUE);
    x += smallButtonWidth + buttonGap;
    MoveWindow(g_listDotButton, x, secondRowY, smallButtonWidth, buttonHeight, TRUE);
    x += smallButtonWidth + buttonGap;
    MoveWindow(g_listSubButton, x, secondRowY, 72, buttonHeight, TRUE);

    MoveWindow(
        g_editBox,
        railWidth + padding + 28,
        topArea + 26,
        width - railWidth - padding * 2 - 58,
        height - topArea - padding - 34,
        TRUE);
}

HWND CreateButton(HWND parent, const wchar_t* text, int id)
{
    HWND button = CreateWindowEx(
        0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandle(nullptr), nullptr);

    SendMessage(button, WM_SETFONT, reinterpret_cast<WPARAM>(g_buttonFont), TRUE);
    return button;
}

void FillRoundRectangle(HDC dc, const RECT& rect, int radius, COLORREF fill, COLORREF border)
{
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);

    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawCornerLines(HDC dc, const RECT& rect, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    const int length = 24;
    const int inset = 7;

    MoveToEx(dc, rect.left + inset, rect.top + length, nullptr);
    LineTo(dc, rect.left + inset, rect.top + inset);
    LineTo(dc, rect.left + length, rect.top + inset);

    MoveToEx(dc, rect.right - length, rect.top + inset, nullptr);
    LineTo(dc, rect.right - inset, rect.top + inset);
    LineTo(dc, rect.right - inset, rect.top + length);

    MoveToEx(dc, rect.left + inset, rect.bottom - length, nullptr);
    LineTo(dc, rect.left + inset, rect.bottom - inset);
    LineTo(dc, rect.left + length, rect.bottom - inset);

    MoveToEx(dc, rect.right - length, rect.bottom - inset, nullptr);
    LineTo(dc, rect.right - inset, rect.bottom - inset);
    LineTo(dc, rect.right - inset, rect.bottom - length);

    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void DrawOrbitBadge(HDC dc, int centerX, int centerY)
{
    HPEN softPen = CreatePen(PS_SOLID, 1, g_borderColor);
    HPEN accentPen = CreatePen(PS_SOLID, 2, g_accentColor);
    HBRUSH accentBrush = CreateSolidBrush(g_accentColor);
    HBRUSH panelBrush = CreateSolidBrush(g_panelColor);
    HGDIOBJ oldPen = SelectObject(dc, softPen);
    HGDIOBJ oldBrush = SelectObject(dc, panelBrush);

    Ellipse(dc, centerX - 34, centerY - 34, centerX + 34, centerY + 34);
    Ellipse(dc, centerX - 23, centerY - 23, centerX + 23, centerY + 23);

    SelectObject(dc, accentPen);
    MoveToEx(dc, centerX, centerY - 42, nullptr);
    LineTo(dc, centerX + 8, centerY - 8);
    LineTo(dc, centerX + 42, centerY);
    LineTo(dc, centerX + 8, centerY + 8);
    LineTo(dc, centerX, centerY + 42);
    LineTo(dc, centerX - 8, centerY + 8);
    LineTo(dc, centerX - 42, centerY);
    LineTo(dc, centerX - 8, centerY - 8);
    LineTo(dc, centerX, centerY - 42);

    SelectObject(dc, accentBrush);
    Ellipse(dc, centerX - 8, centerY - 8, centerX + 8, centerY + 8);

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(panelBrush);
    DeleteObject(accentBrush);
    DeleteObject(accentPen);
    DeleteObject(softPen);
}

RECT GetNoteTabRect(const RECT& rail, int tabIndex)
{
    const int top = 126 + tabIndex * 44;
    return {rail.left + 14, top, rail.right - 14, top + 34};
}

std::wstring GetNoteTabLabel(int noteIndex)
{
    return L"\u041d\u043e\u0442\u0430\u0442\u043a\u0438 " + std::to_wstring(noteIndex + 1);
}

void DrawAccentStrips(HDC dc, const RECT& rail, const RECT& topPanel, const RECT& editorPanel)
{
    HPEN accentPen = CreatePen(PS_SOLID, 2, g_accentColor);
    HPEN softPen = CreatePen(PS_SOLID, 1, g_accentColor);
    HGDIOBJ oldPen = SelectObject(dc, accentPen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    HGDIOBJ oldFont = SelectObject(dc, g_buttonFont);

    for (int i = 0; i < static_cast<int>(g_noteTexts.size()); ++i)
    {
        RECT tab = GetNoteTabRect(rail, i);
        const bool active = i == g_activeNoteIndex;
        FillRoundRectangle(dc, tab, 12, active ? g_buttonColor : g_panelColor, active ? g_accentColor : g_borderColor);

        HBRUSH markerBrush = CreateSolidBrush(g_accentColor);
        RECT marker = {tab.left + 8, tab.top + 9, tab.left + 12, tab.bottom - 9};
        FillRect(dc, &marker, markerBrush);
        DeleteObject(markerBrush);

        RECT labelRect = {tab.left + 18, tab.top, tab.right - 8, tab.bottom};
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, active ? g_textColor : g_mutedTextColor);

        std::wstring label = GetNoteTabLabel(i);
        DrawText(dc, label.c_str(), -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (static_cast<int>(g_noteTexts.size()) < g_maxNotes)
    {
        RECT plusTab = GetNoteTabRect(rail, static_cast<int>(g_noteTexts.size()));
        FillRoundRectangle(dc, plusTab, 12, g_panelColor, g_accentColor);

        RECT plusRect = plusTab;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, g_accentColor);
        DrawText(dc, L"+", -1, &plusRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    MoveToEx(dc, topPanel.left + 260, topPanel.top + 16, nullptr);
    LineTo(dc, topPanel.right - 34, topPanel.top + 16);

    MoveToEx(dc, editorPanel.left + 26, editorPanel.top + 18, nullptr);
    LineTo(dc, editorPanel.left + 150, editorPanel.top + 18);

    MoveToEx(dc, editorPanel.right - 18, editorPanel.top + 86, nullptr);
    LineTo(dc, editorPanel.right - 18, editorPanel.bottom - 42);

    SelectObject(dc, softPen);
    for (int i = 0; i < 4; ++i)
    {
        const int y = editorPanel.bottom - 54 + i * 8;
        MoveToEx(dc, editorPanel.left + 36, y, nullptr);
        LineTo(dc, editorPanel.left + 220 - i * 28, y);
    }

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldFont);
    DeleteObject(softPen);
    DeleteObject(accentPen);
}

void DrawInterface(HDC dc, const RECT& client)
{
    FillRect(dc, &client, g_backgroundBrush);

    const int padding = 18;
    const int railWidth = 126;
    RECT rail = {padding, padding, padding + railWidth - 18, client.bottom - padding};
    RECT topPanel = {railWidth + padding + 12, padding, client.right - padding, 104};
    RECT editorPanel = {railWidth + padding + 12, 112, client.right - padding, client.bottom - padding};

    FillRoundRectangle(dc, rail, 20, g_panelColor, g_borderColor);
    FillRoundRectangle(dc, topPanel, 18, g_panelColor, g_borderColor);
    FillRoundRectangle(dc, editorPanel, 20, g_panelColor, g_borderColor);

    DrawCornerLines(dc, rail, g_accentColor);
    DrawCornerLines(dc, topPanel, g_borderColor);
    DrawCornerLines(dc, editorPanel, g_borderColor);
    DrawAccentStrips(dc, rail, topPanel, editorPanel);
    DrawOrbitBadge(dc, rail.left + (rail.right - rail.left) / 2, 62);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, g_textColor);
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(dc, g_buttonFont));

    RECT brand = {rail.left + 16, rail.top + 14, rail.right - 16, rail.top + 38};
    DrawText(dc, L"N-TEX", -1, &brand, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    SetTextColor(dc, g_mutedTextColor);
    RECT version = {rail.left + 16, rail.top + 38, rail.right - 16, rail.top + 58};
    DrawText(dc, L"v1.0", -1, &version, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    SelectObject(dc, oldFont);
}

void DrawOwnerButton(const DRAWITEMSTRUCT* item)
{
    RECT rect = item->rcItem;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    const bool focused = (item->itemState & ODS_FOCUS) != 0;

    COLORREF fill = pressed ? g_accentColor : g_buttonColor;
    COLORREF border = focused ? g_accentColor : g_borderColor;
    COLORREF text = pressed ? RGB(255, 255, 255) : g_textColor;

    FillRoundRectangle(item->hDC, rect, 12, fill, border);

    wchar_t label[64] = {};
    GetWindowText(item->hwndItem, label, 64);

    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, text);
    HGDIOBJ oldFont = SelectObject(item->hDC, g_buttonFont);
    DrawText(item->hDC, label, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(item->hDC, oldFont);
}

void ApplyThemeColors()
{
    if (g_backgroundBrush != nullptr)
    {
        DeleteObject(g_backgroundBrush);
    }
    if (g_windowClassBrush != nullptr)
    {
        DeleteObject(g_windowClassBrush);
    }

    if (g_darkThemeEnabled)
    {
        g_appBackgroundColor = RGB(13, 14, 19);
        g_textColor = RGB(235, 232, 246);
        g_editorColor = RGB(18, 19, 25);
        g_panelColor = RGB(22, 23, 31);
        g_buttonColor = RGB(39, 33, 55);
        g_borderColor = RGB(67, 61, 86);
        g_accentColor = RGB(149, 82, 255);
        g_mutedTextColor = RGB(154, 149, 170);
        if (g_themeButton != nullptr)
        {
            SetWindowText(g_themeButton, L"\u0421\u0432\u0456\u0442\u043b\u0430");
        }
    }
    else
    {
        g_appBackgroundColor = RGB(239, 241, 248);
        g_textColor = RGB(42, 43, 55);
        g_editorColor = RGB(252, 252, 255);
        g_panelColor = RGB(247, 248, 253);
        g_buttonColor = RGB(239, 234, 253);
        g_borderColor = RGB(204, 207, 222);
        g_accentColor = RGB(137, 88, 235);
        g_mutedTextColor = RGB(114, 118, 133);
        if (g_themeButton != nullptr)
        {
            SetWindowText(g_themeButton, L"\u0422\u0435\u043c\u043d\u0430");
        }
    }

    g_backgroundBrush = CreateSolidBrush(g_appBackgroundColor);
    g_windowClassBrush = CreateSolidBrush(g_appBackgroundColor);

    if (g_mainWindow != nullptr)
    {
        SetClassLongPtr(g_mainWindow, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(g_windowClassBrush));
    }
    if (g_editBox != nullptr)
    {
        SendMessage(g_editBox, EM_SETBKGNDCOLOR, 0, g_editorColor);

        CHARRANGE selection = {};
        SendMessage(g_editBox, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));

        CHARFORMAT2 textFormat = {};
        textFormat.cbSize = sizeof(textFormat);
        textFormat.dwMask = CFM_COLOR;
        textFormat.crTextColor = g_textColor;

        SendMessage(g_editBox, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&textFormat));
        SendMessage(g_editBox, EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&textFormat));
        SendMessage(g_editBox, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&selection));
    }

    if (g_mainWindow != nullptr)
    {
        InvalidateRect(g_mainWindow, nullptr, TRUE);
        RedrawWindow(g_mainWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
}

void ToggleTheme()
{
    g_darkThemeEnabled = !g_darkThemeEnabled;
    ApplyThemeColors();
    SetFocus(g_editBox);
}

std::wstring GetEditText()
{
    const int textLength = GetWindowTextLength(g_editBox);
    std::wstring text(textLength + 1, L'\0');
    GetWindowText(g_editBox, text.data(), static_cast<int>(text.size()));
    text.resize(textLength);
    return text;
}

void SaveActiveNoteText()
{
    if (g_editBox != nullptr && g_activeNoteIndex >= 0 && g_activeNoteIndex < static_cast<int>(g_noteTexts.size()))
    {
        g_noteTexts[g_activeNoteIndex] = GetEditText();
    }
}

void LoadActiveNoteText()
{
    if (g_editBox == nullptr || g_activeNoteIndex < 0 || g_activeNoteIndex >= static_cast<int>(g_noteTexts.size()))
    {
        return;
    }

    SetWindowText(g_editBox, g_noteTexts[g_activeNoteIndex].c_str());
    SendMessage(g_editBox, EM_EMPTYUNDOBUFFER, 0, 0);
    SetFocus(g_editBox);
}

void SwitchToNote(int noteIndex)
{
    if (noteIndex < 0 || noteIndex >= static_cast<int>(g_noteTexts.size()) || noteIndex == g_activeNoteIndex)
    {
        return;
    }

    SaveActiveNoteText();
    g_activeNoteIndex = noteIndex;
    g_autoNumberingEnabled = false;
    g_nextNumber = 1;
    UpdateNumberingButtons();
    LoadActiveNoteText();
    InvalidateRect(g_mainWindow, nullptr, TRUE);
}

void AddNote()
{
    if (static_cast<int>(g_noteTexts.size()) >= g_maxNotes)
    {
        return;
    }

    SaveActiveNoteText();
    g_noteTexts.push_back(L"");
    g_activeNoteIndex = static_cast<int>(g_noteTexts.size()) - 1;
    g_autoNumberingEnabled = false;
    g_nextNumber = 1;
    UpdateNumberingButtons();
    LoadActiveNoteText();
    InvalidateRect(g_mainWindow, nullptr, TRUE);
}

int HitTestNoteTabs(HWND hwnd, LPARAM lParam)
{
    RECT client = {};
    GetClientRect(hwnd, &client);

    const int padding = 18;
    const int railWidth = 126;
    RECT rail = {padding, padding, padding + railWidth - 18, client.bottom - padding};
    POINT point = {static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};

    for (int i = 0; i < static_cast<int>(g_noteTexts.size()); ++i)
    {
        RECT tab = GetNoteTabRect(rail, i);
        if (PtInRect(&tab, point))
        {
            return i;
        }
    }

    if (static_cast<int>(g_noteTexts.size()) < g_maxNotes)
    {
        RECT plusTab = GetNoteTabRect(rail, static_cast<int>(g_noteTexts.size()));
        if (PtInRect(&plusTab, point))
        {
            return -2;
        }
    }

    return -1;
}

bool SaveTextToFile(const wchar_t* filePath)
{
    SaveActiveNoteText();
    std::wstring text = GetEditText();

    const int byteCount = WideCharToMultiByte(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
        nullptr, 0, nullptr, nullptr);

    std::vector<char> bytes(byteCount);
    WideCharToMultiByte(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
        bytes.data(), byteCount, nullptr, nullptr);

    HANDLE file = CreateFile(
        filePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    DWORD written = 0;
    WriteFile(file, bom, sizeof(bom), &written, nullptr);

    if (!bytes.empty())
    {
        WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    }

    CloseHandle(file);
    return true;
}

bool LoadTextFromFile(const wchar_t* filePath)
{
    HANDLE file = CreateFile(
        filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    const DWORD fileSize = GetFileSize(file, nullptr);
    std::vector<char> bytes(fileSize);

    DWORD read = 0;
    if (fileSize > 0)
    {
        if (!ReadFile(file, bytes.data(), fileSize, &read, nullptr))
        {
            CloseHandle(file);
            return false;
        }
    }

    CloseHandle(file);

    const char* data = bytes.data();
    int size = static_cast<int>(read);

    if (size >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF)
    {
        data += 3;
        size -= 3;
    }

    int charCount = MultiByteToWideChar(CP_UTF8, 0, data, size, nullptr, 0);
    if (charCount == 0 && size > 0)
    {
        charCount = MultiByteToWideChar(CP_ACP, 0, data, size, nullptr, 0);
        std::wstring text(charCount, L'\0');
        MultiByteToWideChar(CP_ACP, 0, data, size, text.data(), charCount);
        SetWindowText(g_editBox, text.c_str());
        g_noteTexts[g_activeNoteIndex] = text;
        return true;
    }

    std::wstring text(charCount, L'\0');
    if (charCount > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, data, size, text.data(), charCount);
    }

    SetWindowText(g_editBox, text.c_str());
    g_noteTexts[g_activeNoteIndex] = text;
    return true;
}

void ShowSaveDialog()
{
    wchar_t filePath[MAX_PATH] = L"";

    OPENFILENAME dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_mainWindow;
    dialog.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"txt";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetSaveFileName(&dialog))
    {
        SaveTextToFile(filePath);
    }
}

void ShowOpenDialog()
{
    wchar_t filePath[MAX_PATH] = L"";

    OPENFILENAME dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_mainWindow;
    dialog.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&dialog))
    {
        LoadTextFromFile(filePath);
    }
}

void ClearNotepad()
{
    SetWindowText(g_editBox, L"");
    g_noteTexts[g_activeNoteIndex] = L"";
    g_autoNumberingEnabled = false;
    g_nextNumber = 1;
    SetFocus(g_editBox);
}

void SelectAllText()
{
    SendMessage(g_editBox, EM_SETSEL, 0, -1);
    SetFocus(g_editBox);
}

void ChangeSelectedTextColor()
{
    static COLORREF customColors[16] = {};
    CHOOSECOLOR dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_mainWindow;
    dialog.rgbResult = g_textColor;
    dialog.lpCustColors = customColors;
    dialog.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (!ChooseColor(&dialog))
    {
        return;
    }

    CHARFORMAT2 format = {};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR;
    format.crTextColor = dialog.rgbResult;

    SendMessage(g_editBox, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
    SetFocus(g_editBox);
}

void SetSelectedTextSize(int points)
{
    if (points < 8)
    {
        points = 8;
    }
    else if (points > 48)
    {
        points = 48;
    }

    CHARFORMAT2 format = {};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_SIZE;
    format.yHeight = points * 20;

    SendMessage(g_editBox, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
    SetFocus(g_editBox);
}

void ShowFontSizeMenu()
{
    const int fontSizes[] = {8, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48};
    const int fontSizeCount = sizeof(fontSizes) / sizeof(fontSizes[0]);
    HMENU menu = CreatePopupMenu();

    for (int i = 0; i < fontSizeCount; ++i)
    {
        std::wstring label = std::to_wstring(fontSizes[i]);
        AppendMenu(menu, MF_STRING, ID_FONT_SIZE_FIRST + i, label.c_str());
    }

    RECT buttonRect = {};
    GetWindowRect(g_sizeButton, &buttonRect);

    const int selectedCommand = TrackPopupMenu(
        menu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        buttonRect.left,
        buttonRect.bottom,
        0,
        g_mainWindow,
        nullptr);

    DestroyMenu(menu);

    const int selectedIndex = selectedCommand - ID_FONT_SIZE_FIRST;
    if (selectedIndex >= 0 && selectedIndex < fontSizeCount)
    {
        SetSelectedTextSize(fontSizes[selectedIndex]);
    }
}

std::wstring MakeNumberPrefix(NumberStyle style, int number)
{
    switch (style)
    {
    case NumberStyle::Parenthesis:
        return std::to_wstring(number) + L") ";

    case NumberStyle::Dot:
        return std::to_wstring(number) + L". ";

    case NumberStyle::SubPoint:
        return L"1." + std::to_wstring(number) + L" ";
    }

    return L"";
}

void UpdateNumberingButtons()
{
    SetWindowText(g_listParenButton, g_autoNumberingEnabled && g_autoNumberingStyle == NumberStyle::Parenthesis ? L"1) ON" : L"1)");
    SetWindowText(g_listDotButton, g_autoNumberingEnabled && g_autoNumberingStyle == NumberStyle::Dot ? L"1. ON" : L"1.");
    SetWindowText(g_listSubButton, g_autoNumberingEnabled && g_autoNumberingStyle == NumberStyle::SubPoint ? L"1.1 ON" : L"1.1");
}

int NumberSelectedLines(NumberStyle style)
{
    std::wstring text = GetEditText();

    CHARRANGE selection = {};
    SendMessage(g_editBox, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));

    int start = static_cast<int>(selection.cpMin);
    int end = static_cast<int>(selection.cpMax);

    if (start < 0)
    {
        start = 0;
    }
    if (end < start)
    {
        end = start;
    }
    if (start > static_cast<int>(text.size()))
    {
        start = static_cast<int>(text.size());
    }
    if (end > static_cast<int>(text.size()))
    {
        end = static_cast<int>(text.size());
    }

    int lineStart = start;
    while (lineStart > 0 && text[lineStart - 1] != L'\n')
    {
        --lineStart;
    }

    int lineEnd = end;
    if (lineEnd == lineStart)
    {
        while (lineEnd < static_cast<int>(text.size()) && text[lineEnd] != L'\n')
        {
            ++lineEnd;
        }
    }
    else
    {
        while (lineEnd < static_cast<int>(text.size()) && text[lineEnd] != L'\n')
        {
            ++lineEnd;
        }
    }

    std::wstring block = text.substr(lineStart, lineEnd - lineStart);
    std::wstring numbered;
    int number = 1;
    int lineCount = 0;

    size_t position = 0;
    while (position <= block.size())
    {
        size_t next = block.find(L'\n', position);
        std::wstring line = block.substr(
            position,
            next == std::wstring::npos ? std::wstring::npos : next - position);

        if (!line.empty() && line.back() == L'\r')
        {
            line.pop_back();
        }

        numbered += MakeNumberPrefix(style, number) + line;
        ++lineCount;

        if (next == std::wstring::npos)
        {
            break;
        }

        numbered += L"\r\n";
        position = next + 1;
        ++number;
    }

    CHARRANGE replaceRange = {};
    replaceRange.cpMin = lineStart;
    replaceRange.cpMax = lineEnd;
    SendMessage(g_editBox, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&replaceRange));
    SendMessage(g_editBox, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(numbered.c_str()));

    replaceRange.cpMin = lineStart + static_cast<LONG>(numbered.size());
    replaceRange.cpMax = replaceRange.cpMin;
    SendMessage(g_editBox, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&replaceRange));
    SetFocus(g_editBox);
    return lineCount;
}

void ToggleAutoNumbering(NumberStyle style)
{
    if (g_autoNumberingEnabled && g_autoNumberingStyle == style)
    {
        g_autoNumberingEnabled = false;
        g_nextNumber = 1;
        UpdateNumberingButtons();
        SetFocus(g_editBox);
        return;
    }

    const int numberedLines = NumberSelectedLines(style);
    g_autoNumberingEnabled = true;
    g_autoNumberingStyle = style;
    g_nextNumber = numberedLines + 1;
    UpdateNumberingButtons();
}

void InsertNextAutomaticNumber()
{
    std::wstring textToInsert = L"\r\n" + MakeNumberPrefix(g_autoNumberingStyle, g_nextNumber);
    ++g_nextNumber;

    SendMessage(g_editBox, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(textToInsert.c_str()));
    SetFocus(g_editBox);
}

bool HandleAutomaticNumberingKey(const MSG& message)
{
    if (message.message != WM_KEYDOWN || g_editBox == nullptr || GetFocus() != g_editBox)
    {
        return false;
    }

    if (message.wParam == VK_ESCAPE && g_autoNumberingEnabled)
    {
        g_autoNumberingEnabled = false;
        g_nextNumber = 1;
        UpdateNumberingButtons();
        return true;
    }

    if (!g_autoNumberingEnabled)
    {
        return false;
    }

    if (message.wParam == VK_RETURN || message.wParam == VK_TAB)
    {
        InsertNextAutomaticNumber();
        return true;
    }

    return false;
}

bool HandleHotKey(const MSG& message)
{
    if (message.message != WM_KEYDOWN || g_editBox == nullptr)
    {
        return false;
    }

    const bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (!ctrlPressed)
    {
        return false;
    }

    switch (message.wParam)
    {
    case 'A':
        SelectAllText();
        return true;

    case 'C':
        SendMessage(g_editBox, WM_COPY, 0, 0);
        return true;

    case 'N':
        ClearNotepad();
        return true;

    case 'O':
        ShowOpenDialog();
        return true;

    case 'S':
        ShowSaveDialog();
        return true;

    case 'V':
        SendMessage(g_editBox, WM_PASTE, 0, 0);
        return true;

    case 'X':
        SendMessage(g_editBox, WM_CUT, 0, 0);
        return true;

    case 'Z':
        SendMessage(g_editBox, EM_UNDO, 0, 0);
        return true;
    }

    return false;
}

bool ShouldSeparateUndoStep(const MSG& message)
{
    if (message.hwnd != g_editBox)
    {
        return false;
    }

    if (message.message == WM_CHAR)
    {
        return message.wParam >= 32 || message.wParam == VK_RETURN || message.wParam == VK_TAB;
    }

    if (message.message == WM_KEYDOWN)
    {
        const bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;

        if (ctrlPressed || altPressed)
        {
            return false;
        }

        return message.wParam == VK_BACK || message.wParam == VK_DELETE;
    }

    return false;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        CreateAppFonts();

        g_themeButton = CreateButton(hwnd, L"\u0422\u0435\u043c\u043d\u0430", ID_BUTTON_THEME);
        ApplyThemeColors();

        g_titleLabel = CreateWindowEx(
            0, L"STATIC",
            L"\u041c\u0456\u0439 \u0411\u043b\u043e\u043a\u043d\u043e\u0442\u0456\u043a",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

        g_newButton = CreateButton(hwnd, L"\u041d\u043e\u0432\u0438\u0439", ID_BUTTON_NEW);
        g_openButton = CreateButton(hwnd, L"\u0412\u0456\u0434\u043a\u0440\u0438\u0442\u0438", ID_BUTTON_OPEN);
        g_saveButton = CreateButton(hwnd, L"\u0417\u0431\u0435\u0440\u0435\u0433\u0442\u0438", ID_BUTTON_SAVE);
        g_colorButton = CreateButton(hwnd, L"\u041a\u043e\u043b\u0456\u0440", ID_BUTTON_COLOR);
        g_sizeButton = CreateButton(hwnd, L"\u0420\u043e\u0437\u043c\u0456\u0440", ID_BUTTON_SIZE);
        g_selectAllButton = CreateButton(hwnd, L"\u0412\u0441\u0435", ID_BUTTON_SELECT_ALL);
        g_listParenButton = CreateButton(hwnd, L"1)", ID_BUTTON_LIST_PAREN);
        g_listDotButton = CreateButton(hwnd, L"1.", ID_BUTTON_LIST_DOT);
        g_listSubButton = CreateButton(hwnd, L"1.1", ID_BUTTON_LIST_SUB);
        UpdateNumberingButtons();

        g_editBox = CreateWindowEx(
            0,
            L"RICHEDIT50W",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE |
                ES_AUTOVSCROLL | ES_WANTRETURN,
            0,
            0,
            0,
            0,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);

        CHARFORMAT2 defaultFormat = {};
        defaultFormat.cbSize = sizeof(defaultFormat);
        defaultFormat.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR;
        defaultFormat.yHeight = 11 * 20;
        defaultFormat.crTextColor = g_textColor;
        wcscpy_s(defaultFormat.szFaceName, L"Consolas");
        SendMessage(g_editBox, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&defaultFormat));
        SendMessage(g_editBox, EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&defaultFormat));
        SendMessage(g_editBox, EM_SETBKGNDCOLOR, 0, g_editorColor);
        SendMessage(g_editBox, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(16, 16));

        SendMessage(g_titleLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_titleFont), TRUE);

        ApplyWindowLayout(hwnd);
        SetFocus(g_editBox);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BUTTON_THEME:
            ToggleTheme();
            return 0;

        case ID_BUTTON_NEW:
            ClearNotepad();
            return 0;

        case ID_BUTTON_OPEN:
            g_autoNumberingEnabled = false;
            UpdateNumberingButtons();
            ShowOpenDialog();
            return 0;

        case ID_BUTTON_SAVE:
            ShowSaveDialog();
            return 0;

        case ID_BUTTON_COLOR:
            ChangeSelectedTextColor();
            return 0;

        case ID_BUTTON_SIZE:
            ShowFontSizeMenu();
            return 0;

        case ID_BUTTON_SELECT_ALL:
            SelectAllText();
            return 0;

        case ID_BUTTON_LIST_PAREN:
            ToggleAutoNumbering(NumberStyle::Parenthesis);
            return 0;

        case ID_BUTTON_LIST_DOT:
            ToggleAutoNumbering(NumberStyle::Dot);
            return 0;

        case ID_BUTTON_LIST_SUB:
            ToggleAutoNumbering(NumberStyle::SubPoint);
            return 0;
        }
        break;

    case WM_LBUTTONDOWN:
    {
        const int tabHit = HitTestNoteTabs(hwnd, lParam);
        if (tabHit >= 0)
        {
            SwitchToNote(tabHit);
            return 0;
        }
        if (tabHit == -2)
        {
            AddNote();
            return 0;
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
        SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
        SetTextColor(reinterpret_cast<HDC>(wParam), g_textColor);
        return reinterpret_cast<LRESULT>(g_backgroundBrush);

    case WM_DRAWITEM:
        DrawOwnerButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    case WM_PAINT:
    {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(hwnd, &paint);
        RECT client = {};
        GetClientRect(hwnd, &client);
        DrawInterface(dc, client);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        if (g_editBox != nullptr)
        {
            ApplyWindowLayout(hwnd);
        }
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 860;
        info->ptMinTrackSize.y = 520;
        return 0;
    }

    case WM_SETFOCUS:
        SetFocus(g_editBox);
        return 0;

    case WM_DESTROY:
        SaveActiveNoteText();
        DeleteObject(g_titleFont);
        DeleteObject(g_buttonFont);
        DeleteObject(g_backgroundBrush);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

int main()
{
    g_richEditLibrary = LoadLibrary(L"Msftedit.dll");
    if (g_richEditLibrary == nullptr)
    {
        MessageBox(
            nullptr,
            L"RichEdit \u043d\u0435 \u0437\u0430\u0432\u0430\u043d\u0442\u0430\u0436\u0438\u0432\u0441\u044f.",
            L"\u041f\u043e\u043c\u0438\u043b\u043a\u0430",
            MB_OK | MB_ICONERROR);
        return 0;
    }

    HINSTANCE instance = GetModuleHandle(nullptr);
    const wchar_t className[] = L"MyNotepadWindow";

    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_IBEAM);
    windowClass.hbrBackground = nullptr;

    RegisterClass(&windowClass);

    HWND window = CreateWindowEx(
        0,
        className,
        L"\u041c\u0456\u0439 \u0411\u043b\u043e\u043a\u043d\u043e\u0442\u0456\u043a - \u041d\u041e\u0412\u0418\u0419 \u0414\u0418\u0417\u0410\u0419\u041d",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        980,
        700,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (window == nullptr)
    {
        FreeLibrary(g_richEditLibrary);
        return 0;
    }

    g_mainWindow = window;

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG message = {};
    while (GetMessage(&message, nullptr, 0, 0))
    {
        const bool separateUndoStep = ShouldSeparateUndoStep(message);

        if (HandleAutomaticNumberingKey(message))
        {
            continue;
        }

        if (HandleHotKey(message))
        {
            continue;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);

        if (separateUndoStep)
        {
            SendMessage(g_editBox, EM_STOPGROUPTYPING, 0, 0);
        }
    }

    DeleteObject(g_windowClassBrush);
    FreeLibrary(g_richEditLibrary);

    return static_cast<int>(message.wParam);
}
