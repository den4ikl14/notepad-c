#include "editor.h"
#include <iostream>
#include <fstream>
#include <conio.h>
#include <cstring>
#include <commdlg.h>

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clearScreen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written;
    COORD origin = { 0, 0 };
    FillConsoleOutputCharacterA(hConsole, ' ', size, origin, &written);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, size, origin, &written);
    SetConsoleCursorPosition(hConsole, origin);
}

static int countWords(const std::vector<std::string>& lines) {
    int count = 0;
    for (const auto& line : lines) {
        bool inWord = false;
        for (unsigned char c : line) {
            bool space = (c == ' ' || c == '\t');
            if (!space && !inWord) { inWord = true; count++; }
            else if (space) inWord = false;
        }
    }
    return count;
}

static int countChars(const std::vector<std::string>& lines) {
    int count = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        count += (int)lines[i].size();
        if (i + 1 < lines.size()) count++;
    }
    return count;
}

static std::string cp866ToAnsi(const std::string& s) {
    if (s.empty()) return s;
    int wlen = MultiByteToWideChar(CP_OEMCP, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (wlen <= 0) return s;
    std::wstring ws(wlen, 0);
    MultiByteToWideChar(CP_OEMCP, 0, s.c_str(), (int)s.size(), &ws[0], wlen);
    int alen = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), wlen, NULL, 0, NULL, NULL);
    if (alen <= 0) return s;
    std::string result(alen, 0);
    WideCharToMultiByte(CP_ACP, 0, ws.c_str(), wlen, &result[0], alen, NULL, NULL);
    return result;
}

static std::string ansiToCp866(const std::string& s) {
    if (s.empty()) return s;
    int wlen = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (wlen <= 0) return s;
    std::wstring ws(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), &ws[0], wlen);
    int olen = WideCharToMultiByte(CP_OEMCP, 0, ws.c_str(), wlen, NULL, 0, NULL, NULL);
    if (olen <= 0) return s;
    std::string result(olen, 0);
    WideCharToMultiByte(CP_OEMCP, 0, ws.c_str(), wlen, &result[0], olen, NULL, NULL);
    return result;
}

ThemeStyle Editor::currentTheme() const {
    ThemeStyle t;
    const WORD whiteBg = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
    const WORD blackFg = 0;

    if (darkTheme) {
        t.header = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        t.text = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        t.lineNum = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        t.selection = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        t.status = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        t.prompt = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    else {
        t.header = whiteBg | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        t.text = whiteBg | blackFg;
        t.lineNum = whiteBg | FOREGROUND_GREEN;
        t.selection = BACKGROUND_BLUE | FOREGROUND_INTENSITY;
        t.status = whiteBg | FOREGROUND_BLUE;
        t.prompt = whiteBg | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
    return t;
}

void Editor::toggleTheme() {
    darkTheme = !darkTheme;
}

void Editor::getWindowLayout(int& winTop, int& winBottom, int& width) const {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    winTop = csbi.srWindow.Top;
    winBottom = csbi.srWindow.Bottom;
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

void Editor::getViewport(int& screenWidth, int& editLineCount) const {
    int winTop, winBottom;
    getWindowLayout(winTop, winBottom, screenWidth);
    editLineCount = winBottom - winTop - 1;
    if (editLineCount < 1) editLineCount = 1;
}

void Editor::showStatusMessage(const std::string& msg, bool waitKey) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    int sw, winTop, winBottom;
    getWindowLayout(winTop, winBottom, sw);
    ThemeStyle th = currentTheme();

    gotoxy(0, winBottom);
    SetConsoleTextAttribute(h, th.prompt);
    std::string line = msg;
    while ((int)line.size() < sw) line += ' ';
    if ((int)line.size() > sw) line = line.substr(0, sw);
    std::cout << line;
    SetConsoleTextAttribute(h, csbi.wAttributes);

    if (waitKey) {
        int dummy = _getch();
        (void)dummy;
    }
}

std::string Editor::readStatusInput(const std::string& prompt) {
    std::string result;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    int sw, winTop, winBottom;
    getWindowLayout(winTop, winBottom, sw);
    ThemeStyle th = currentTheme();

    while (true) {
        gotoxy(0, winBottom);
        SetConsoleTextAttribute(h, th.prompt);
        std::string line = prompt + result;
        while ((int)line.size() < sw) line += ' ';
        if ((int)line.size() > sw) line = line.substr(0, sw);
        std::cout << line;
        SetConsoleTextAttribute(h, th.text);

        int key = _getch();
        if (key == 13) break;
        if (key == 27) { result.clear(); break; }
        if (key == 8) {
            if (!result.empty()) result.pop_back();
            continue;
        }
        if (key == 0 || key == 224) {
            _getch();
            continue;
        }
        if ((key >= 32 && key <= 126) || (key >= 128 && key <= 255))
            result += (char)key;
    }
    return result;
}

void Editor::showHelp() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    ThemeStyle th = currentTheme();
    int winTop, winBottom, width;
    getWindowLayout(winTop, winBottom, width);
    DWORD written;
    for (int y = winTop; y <= winBottom; y++) {
        COORD pos = { 0, (SHORT)y };
        FillConsoleOutputCharacterA(h, ' ', width, pos, &written);
        FillConsoleOutputAttribute(h, th.text, width, pos, &written);
    }

    gotoxy(0, winTop);
    SetConsoleTextAttribute(h, th.header);
    std::cout << "   Hotkeys\n\n";
    SetConsoleTextAttribute(h, th.text);
    std::cout << "  Ctrl+S      Save file\n";
    std::cout << "  Ctrl+O      Open file\n";
    std::cout << "  Ctrl+F      Find text\n";
    std::cout << "  Ctrl+H      Replace words\n";
    std::cout << "  Ctrl+C/X/V  Copy / Cut / Paste\n";
    std::cout << "  Ctrl+Z/Y    Undo / Redo\n";
    std::cout << "  Ctrl+Q      Exit\n";
    std::cout << "  F2          Light / Dark theme\n";
    SetConsoleTextAttribute(h, csbi.wAttributes);
    int dummy = _getch();
    (void)dummy;
}

bool Editor::tryExit() {
    if (!modified) {
        running = false;
        return true;
    }
    showStatusMessage(" Save changes before exit? (Y/N) ", false);
    int key = _getch();
    if (key == 'y' || key == 'Y') {
        saveFile();
        running = false;
        return true;
    }
    if (key == 'n' || key == 'N') {
        running = false;
        return true;
    }
    return false;
}

bool Editor::hasSelection() {
    return selecting && (selStartX != cursorX || selStartY != cursorY);
}

std::string Editor::getSelectedText() {
    if (!hasSelection()) return "";

    int startY = selStartY, startX = selStartX;
    int endY = cursorY, endX = cursorX;

    if (startY > endY || (startY == endY && startX > endX)) {
        std::swap(startY, endY);
        std::swap(startX, endX);
    }

    std::string result;
    for (int y = startY; y <= endY; y++) {
        int from = (y == startY) ? startX : 0;
        int to = (y == endY) ? endX : (int)lines[y].size();
        result += lines[y].substr(from, to - from);
        if (y < endY) result += "\n";
    }
    return result;
}

void Editor::deleteSelected() {
    if (!hasSelection()) return;

    int startY = selStartY, startX = selStartX;
    int endY = cursorY, endX = cursorX;

    if (startY > endY || (startY == endY && startX > endX)) {
        std::swap(startY, endY);
        std::swap(startX, endX);
    }

    std::string before = lines[startY].substr(0, startX);
    std::string after = lines[endY].substr(endX);

    lines[startY] = before + after;
    lines.erase(lines.begin() + startY + 1, lines.begin() + endY + 1);

    cursorY = startY;
    cursorX = startX;
    selecting = false;
    modified = true;
}

void Editor::render() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    int winTop, winBottom, screenWidth;
    getWindowLayout(winTop, winBottom, screenWidth);
    int editTop = winTop + 1;
    int editBottom = winBottom - 1;
    int editLineCount = editBottom - editTop + 1;
    if (editLineCount < 1) editLineCount = 1;

    ThemeStyle th = currentTheme();

    int selMinY = selStartY, selMinX = selStartX;
    int selMaxY = cursorY, selMaxX = cursorX;
    if (selecting && (selMinY > selMaxY || (selMinY == selMaxY && selMinX > selMaxX))) {
        std::swap(selMinY, selMaxY);
        std::swap(selMinX, selMaxX);
    }

    DWORD written;
    for (int y = winTop; y <= winBottom; y++) {
        WORD rowAttr = th.text;
        if (y == winTop) rowAttr = th.header;
        else if (y == winBottom) rowAttr = th.status;
        COORD rowPos = { 0, (SHORT)y };
        FillConsoleOutputCharacterA(hConsole, ' ', screenWidth, rowPos, &written);
        FillConsoleOutputAttribute(hConsole, rowAttr, screenWidth, rowPos, &written);
    }

    gotoxy(0, winTop);
    SetConsoleTextAttribute(hConsole, th.header);

    std::string title = "  ** TEXT EDITOR **";
    std::string fileInfo = "  File: ";
    fileInfo += filename.empty() ? "New file" : filename;
    if (modified) fileInfo += " *";
    if (hasSelection()) fileInfo += "  [SEL]";
    fileInfo += darkTheme ? "  | Theme: Dark" : "  | Theme: Light";

    int spaces = screenWidth - (int)title.size() - (int)fileInfo.size();
    if (spaces < 1) spaces = 1;
    std::string header = title;
    for (int i = 0; i < spaces; i++) header += " ";
    header += fileInfo;
    if ((int)header.size() > screenWidth) header = header.substr(0, screenWidth);
    std::cout << header;

    int lineNumWidth = 4;

    for (int i = 0; i < editLineCount; i++) {
        int rowY = editTop + i;
        gotoxy(0, rowY);

        COORD rowPos = { 0, (SHORT)rowY };
        FillConsoleOutputAttribute(hConsole, th.text, screenWidth, rowPos, &written);

        if ((i + scrollY) >= (int)lines.size()) continue;

        int lineIdx = i + scrollY;

        std::string numStr = std::to_string(lineIdx + 1);
        while ((int)numStr.size() < lineNumWidth) numStr = " " + numStr;

        SetConsoleTextAttribute(hConsole, th.lineNum);
        std::cout << numStr;
        SetConsoleTextAttribute(hConsole, th.text);
        std::cout << " ";

        std::string& line = lines[lineIdx];
        int maxChars = screenWidth - lineNumWidth - 1;
        if (maxChars < 0) maxChars = 0;

        if (!selecting || !hasSelection() || lineIdx < selMinY || lineIdx > selMaxY) {
            if ((int)line.size() > maxChars)
                std::cout << line.substr(0, maxChars);
            else
                std::cout << line;
        }
        else {
            int from = (lineIdx == selMinY) ? selMinX : 0;
            int to = (lineIdx == selMaxY) ? selMaxX : (int)line.size();
            if (from > maxChars) from = maxChars;
            if (to > maxChars) to = maxChars;
            if (from < 0) from = 0;
            if (to < from) to = from;

            std::cout << line.substr(0, from);
            SetConsoleTextAttribute(hConsole, th.selection);
            std::cout << line.substr(from, to - from);
            SetConsoleTextAttribute(hConsole, th.text);
            if (to < (int)line.size() && to < maxChars)
                std::cout << line.substr(to, maxChars - to);
        }
    }

    gotoxy(0, winBottom);
    SetConsoleTextAttribute(hConsole, th.status);

    std::string status = " Ln:" + std::to_string(cursorY + 1);
    status += " Col:" + std::to_string(cursorX + 1);
    status += " | Words:" + std::to_string(countWords(lines));
    status += " Chars:" + std::to_string(countChars(lines));
    status += " | F1 Help F2 Theme";
    while ((int)status.size() < screenWidth) status += " ";
    std::cout << status.substr(0, screenWidth);

    int cursorRow = editTop + (cursorY - scrollY);
    if (cursorRow < editTop) cursorRow = editTop;
    if (cursorRow > editBottom) cursorRow = editBottom;
    SetConsoleTextAttribute(hConsole, th.text);
    gotoxy(cursorX + lineNumWidth + 1, cursorRow);
}

void Editor::saveState() {
    if (undoTop < MAX_UNDO - 1) {
        undoTop++;
        undoStack[undoTop].lines = lines;
        undoStack[undoTop].cursorX = cursorX;
        undoStack[undoTop].cursorY = cursorY;
        redoTop = -1;
    }
}

void Editor::saveFile() {
    if (filename.empty()) {
        OPENFILENAMEA ofn = {};
        char szFile[260] = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
        ofn.lpstrDefExt = "txt";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (!GetSaveFileNameA(&ofn))
            return;
        filename = szFile;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        showStatusMessage(" Error: cannot save file. Press any key...");
        return;
    }

    for (int i = 0; i < (int)lines.size(); i++) {
        file << cp866ToAnsi(lines[i]);
        if (i < (int)lines.size() - 1) file << "\r\n";
    }
    file.close();
    modified = false;

    char fullPath[MAX_PATH];
    GetFullPathNameA(filename.c_str(), MAX_PATH, fullPath, NULL);
    showStatusMessage(std::string(" Saved: ") + fullPath + "  (any key)");
}

void Editor::openFile() {
    OPENFILENAMEA ofn = {};
    char szFile[260] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameA(&ofn))
        return;

    std::ifstream file(szFile, std::ios::binary);
    if (!file.is_open()) {
        showStatusMessage(" Error: cannot open file. Press any key...");
        return;
    }

    lines.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(ansiToCp866(line));
    }
    file.close();
    if (lines.empty()) lines.push_back("");
    filename = szFile;
    cursorX = 0; cursorY = 0; scrollY = 0;
    modified = false; selecting = false;
}

static bool findInDocument(Editor& ed, const std::string& query, int startY, int startX, bool wrap) {
    if (query.empty()) return false;

    for (int y = startY; y < (int)ed.lines.size(); y++) {
        int sx = (y == startY) ? startX : 0;
        size_t pos = ed.lines[y].find(query, sx);
        if (pos != std::string::npos) {
            ed.selStartY = y; ed.selStartX = (int)pos;
            ed.cursorY = y; ed.cursorX = (int)pos + (int)query.size();
            ed.selecting = true;
            ed.adjustScroll();
            return true;
        }
    }

    if (!wrap) return false;

    for (int y = 0; y <= startY; y++) {
        size_t pos = ed.lines[y].find(query);
        if (pos != std::string::npos) {
            ed.selStartY = y; ed.selStartX = (int)pos;
            ed.cursorY = y; ed.cursorX = (int)pos + (int)query.size();
            ed.selecting = true;
            ed.adjustScroll();
            return true;
        }
    }
    return false;
}

void Editor::findText() {
    std::string query = readStatusInput(" Find: ");
    if (query.empty()) return;

    lastSearch = query;
    int startX = cursorX + 1;
    int startY = cursorY;

    if (!findInDocument(*this, query, startY, startX, true))
        showStatusMessage(" Not found. Press any key...");
}

void Editor::findNext() {
    if (lastSearch.empty()) {
        findText();
        return;
    }
    if (!findInDocument(*this, lastSearch, cursorY, cursorX, true))
        showStatusMessage(" Not found. Press any key...");
}

void Editor::replaceText() {
    selecting = false;

    std::string findStr = readStatusInput(" Find: ");
    if (findStr.empty()) return;

    std::string replaceStr = readStatusInput(" Replace with: ");

    saveState();
    int count = 0;
    for (auto& line : lines) {
        size_t pos = 0;
        while ((pos = line.find(findStr, pos)) != std::string::npos) {
            line.replace(pos, findStr.size(), replaceStr);
            pos += replaceStr.size();
            count++;
        }
    }
    if (count > 0) modified = true;

    if (count > 0)
        showStatusMessage(" Replaced: " + std::to_string(count) + " time(s). Any key...");
    else
        showStatusMessage(" No matches found. Any key...");
}

void Editor::copyToClipboard(const std::string& text) {
    if (text.empty()) return;

    std::string ansi = cp866ToAnsi(text);
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), (int)ansi.size(), NULL, 0);
    if (wlen <= 0) return;

    std::wstring wide(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), (int)ansi.size(), &wide[0], wlen);

    HWND owner = GetConsoleWindow();
    if (!OpenClipboard(owner))
        return;

    EmptyClipboard();

    size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem) {
        void* ptr = GlobalLock(hMem);
        if (ptr) {
            memcpy(ptr, wide.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        else {
            GlobalFree(hMem);
        }
    }
    CloseClipboard();
}

void Editor::copySelection() {
    std::string text = hasSelection() ? getSelectedText() : lines[cursorY];
    copyToClipboard(text);
    selecting = false;
}

void Editor::cutSelection() {
    if (!hasSelection()) {
        if (cursorX < (int)lines[cursorY].size()) {
            saveState();
            std::string text = lines[cursorY].substr(cursorX);
            copyToClipboard(text);
            lines[cursorY].erase(cursorX);
            modified = true;
        }
        return;
    }

    saveState();
    std::string text = getSelectedText();
    copyToClipboard(text);
    deleteSelected();
}

static std::string clipboardToCp866() {
    std::string ansi;

    HANDLE hUni = GetClipboardData(CF_UNICODETEXT);
    if (hUni) {
        wchar_t* wtext = (wchar_t*)GlobalLock(hUni);
        if (wtext) {
            int alen = WideCharToMultiByte(CP_ACP, 0, wtext, -1, NULL, 0, NULL, NULL);
            if (alen > 0) {
                ansi.resize(alen - 1);
                WideCharToMultiByte(CP_ACP, 0, wtext, -1, &ansi[0], alen, NULL, NULL);
            }
            GlobalUnlock(hUni);
        }
    }

    if (ansi.empty()) {
        HANDLE hAnsi = GetClipboardData(CF_TEXT);
        if (hAnsi) {
            char* text = (char*)GlobalLock(hAnsi);
            if (text) {
                ansi = text;
                GlobalUnlock(hAnsi);
            }
        }
    }

    return ansi.empty() ? std::string() : ansiToCp866(ansi);
}

void Editor::pasteClipboard() {
    if (hasSelection()) { saveState(); deleteSelected(); }

    HWND owner = GetConsoleWindow();
    if (!OpenClipboard(owner)) return;

    std::string str = clipboardToCp866();
    CloseClipboard();

    if (str.empty()) return;

    saveState();

    std::string before = lines[cursorY].substr(0, cursorX);
    std::string after = lines[cursorY].substr(cursorX);

    std::vector<std::string> parts;
    std::string cur;
    for (char c : str) {
        if (c == '\n') { parts.push_back(cur); cur = ""; }
        else if (c != '\r') cur += c;
    }
    parts.push_back(cur);

    lines[cursorY] = before + parts[0];
    for (int i = 1; i < (int)parts.size(); i++) {
        lines.insert(lines.begin() + cursorY + i,
            i == (int)parts.size() - 1 ? parts[i] + after : parts[i]);
    }
    cursorY += (int)parts.size() - 1;
    if ((int)parts.size() == 1)
        cursorX = (int)(before + parts[0]).size();
    else
        cursorX = (int)parts.back().size();
    modified = true;
    selecting = false;
    adjustScroll();
}

void Editor::run() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    {
        COORD bufSize;
        bufSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        bufSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        SetConsoleScreenBufferSize(h, bufSize);
        SMALL_RECT rect = csbi.srWindow;
        SetConsoleWindowInfo(h, TRUE, &rect);
    }

    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(h, &ci);
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(h, &ci);

    SetConsoleTitleA("Text Editor - Coursework");

    lines.push_back("");
    cursorX = 0; cursorY = 0; scrollY = 0;
    modified = false; undoTop = -1; redoTop = -1;
    selecting = false; selStartX = 0; selStartY = 0;
    lastSearch = "";
    darkTheme = false;
    running = true;

    while (running) {
        render();
        handleInput();
    }
}
