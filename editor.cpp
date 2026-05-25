#include "editor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <conio.h>
#include <cstring>

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
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

int countWords(const std::vector<std::string>& lines) {
    int count = 0;
    for (const auto& line : lines) {
        bool inWord = false;
        for (char c : line) {
            if (c != ' ' && !inWord) { inWord = true; count++; }
            else if (c == ' ') inWord = false;
        }
    }
    return count;
}

int countChars(const std::vector<std::string>& lines) {
    int count = 0;
    for (const auto& line : lines) count += (int)line.size();
    return count;
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
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;
    int screenWidth = csbi.srWindow.Right - csbi.srWindow.Left;

    int selMinY = selStartY, selMinX = selStartX;
    int selMaxY = cursorY, selMaxX = cursorX;
    if (selecting && (selMinY > selMaxY || (selMinY == selMaxY && selMinX > selMaxX))) {
        std::swap(selMinY, selMaxY);
        std::swap(selMinX, selMaxX);
    }

    clearScreen();

    WORD headerAttr = BACKGROUND_BLUE | BACKGROUND_INTENSITY |
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    COORD headerPos = { 0, 0 };
    DWORD written;
    FillConsoleOutputAttribute(hConsole, headerAttr, screenWidth, headerPos, &written);
    gotoxy(0, 0);
    SetConsoleTextAttribute(hConsole, headerAttr);

    std::string title = "  ** TEXT EDITOR **";
    std::string fileInfo = "  File: ";
    fileInfo += filename.empty() ? "New file" : filename;
    if (modified) fileInfo += " *";
    if (hasSelection()) fileInfo += "  [SELECTED]";

    int spaces = screenWidth - (int)title.size() - (int)fileInfo.size();
    std::string header = title;
    for (int i = 0; i < spaces; i++) header += " ";
    header += fileInfo;
    if ((int)header.size() > screenWidth) header = header.substr(0, screenWidth);
    std::cout << header;

    SetConsoleTextAttribute(hConsole, csbi.wAttributes);

    int lineNumWidth = 3;
    WORD selAttr = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    for (int i = 0; i < screenHeight - 1 && (i + scrollY) < (int)lines.size(); i++) {
        int lineIdx = i + scrollY;
        gotoxy(0, i + 1);

        std::string numStr = std::to_string(lineIdx + 1);
        while ((int)numStr.size() < lineNumWidth) numStr = " " + numStr;

        WORD lineNumAttr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        SetConsoleTextAttribute(hConsole, lineNumAttr);
        std::cout << numStr;
        SetConsoleTextAttribute(hConsole, csbi.wAttributes);
        std::cout << " ";

        std::string& line = lines[lineIdx];
        int maxChars = screenWidth - lineNumWidth - 1;

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

            std::cout << line.substr(0, from);
            SetConsoleTextAttribute(hConsole, selAttr);
            std::cout << line.substr(from, to - from);
            SetConsoleTextAttribute(hConsole, csbi.wAttributes);
            if (to < (int)line.size() && to < maxChars)
                std::cout << line.substr(to, maxChars - to);
        }
    }

    WORD statusAttr = BACKGROUND_GREEN | 0;
    COORD statusPos = { 0, (SHORT)(screenHeight + 1) };
    FillConsoleOutputAttribute(hConsole, statusAttr, screenWidth, statusPos, &written);
    gotoxy(0, screenHeight + 1);
    SetConsoleTextAttribute(hConsole, statusAttr);

    std::string status = " Line: " + std::to_string(cursorY + 1);
    status += "  Col: " + std::to_string(cursorX + 1);
    status += "  |  Words: " + std::to_string(countWords(lines));
    status += "  Chars: " + std::to_string(countChars(lines));
    while ((int)status.size() < screenWidth) status += " ";
    std::cout << status.substr(0, screenWidth);

    SetConsoleTextAttribute(hConsole, csbi.wAttributes);
    gotoxy(cursorX + lineNumWidth + 1, cursorY - scrollY + 1);
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
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;
    WORD attr = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    if (filename.empty()) {
        OPENFILENAMEA ofn = {};
        char szFile[260] = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
        ofn.lpstrDefExt = "txt";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameA(&ofn)) {
            filename = szFile;
        }
        else {
            return;
        }
    }

    std::ofstream file(filename);
    if (file.is_open()) {
        for (int i = 0; i < (int)lines.size(); i++) {
            file << lines[i];
            if (i < (int)lines.size() - 1) file << "\n";
        }
        file.close();
        modified = false;

        char fullPath[MAX_PATH];
        GetFullPathNameA(filename.c_str(), MAX_PATH, fullPath, NULL);

        gotoxy(0, screenHeight + 1);
        SetConsoleTextAttribute(h, attr);
        std::cout << std::string(100, ' ');
        gotoxy(0, screenHeight + 1);
        std::cout << " Saved: " << fullPath << "  (press any key)";
        SetConsoleTextAttribute(h, csbi.wAttributes);
        int dummy = _getch();
        (void)dummy;
    }
}

void Editor::findText() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;
    WORD attr = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    gotoxy(0, screenHeight + 1);
    SetConsoleTextAttribute(h, attr);
    std::cout << std::string(80, ' ');
    gotoxy(0, screenHeight + 1);
    std::cout << " Find: ";
    SetConsoleTextAttribute(h, csbi.wAttributes);

    std::string query;
    std::cin >> query;
    if (query.empty()) return;

    for (int y = cursorY; y < (int)lines.size(); y++) {
        int startX = (y == cursorY) ? cursorX : 0;
        size_t pos = lines[y].find(query, startX);
        if (pos != std::string::npos) {
            selStartY = y;
            selStartX = (int)pos;
            cursorY = y;
            cursorX = (int)pos + (int)query.size();
            selecting = true;
            adjustScroll();
            return;
        }
    }

    for (int y = 0; y < cursorY; y++) {
        size_t pos = lines[y].find(query);
        if (pos != std::string::npos) {
            selStartY = y;
            selStartX = (int)pos;
            cursorY = y;
            cursorX = (int)pos + (int)query.size();
            selecting = true;
            adjustScroll();
            return;
        }
    }

    gotoxy(0, screenHeight + 1);
    SetConsoleTextAttribute(h, attr);
    std::cout << " Not found. Press any key...";
    SetConsoleTextAttribute(h, csbi.wAttributes);
    int dummy = _getch();
    (void)dummy;
}

void Editor::replaceText() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;
    WORD attr = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    gotoxy(0, screenHeight + 1);
    SetConsoleTextAttribute(h, attr);
    std::cout << std::string(80, ' ');
    gotoxy(0, screenHeight + 1);
    std::cout << " Find: ";
    SetConsoleTextAttribute(h, csbi.wAttributes);

    std::string findStr;
    std::cin >> findStr;
    if (findStr.empty()) return;

    gotoxy(0, screenHeight + 1);
    SetConsoleTextAttribute(h, attr);
    std::cout << std::string(80, ' ');
    gotoxy(0, screenHeight + 1);
    std::cout << " Replace with: ";
    SetConsoleTextAttribute(h, csbi.wAttributes);

    std::string replaceStr;
    std::getline(std::cin, replaceStr);
    std::getline(std::cin, replaceStr);

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

    gotoxy(0, screenHeight + 1);
    SetConsoleTextAttribute(h, attr);
    std::cout << " Replaced: " << count << " occurrence(s). Press any key...";
    SetConsoleTextAttribute(h, csbi.wAttributes);
    int dummy = _getch();
    (void)dummy;
}

void Editor::copySelection() {
    std::string text = hasSelection() ? getSelectedText() : lines[cursorY];
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hMem) {
            memcpy(GlobalLock(hMem), text.c_str(), text.size() + 1);
            GlobalUnlock(hMem);
            SetClipboardData(CF_TEXT, hMem);
        }
        CloseClipboard();
    }
    selecting = false;
}

void Editor::cutSelection() {
    saveState();
    copySelection();
    if (hasSelection()) {
        deleteSelected();
    }
    else {
        if ((int)lines.size() > 1) {
            lines.erase(lines.begin() + cursorY);
            if (cursorY >= (int)lines.size()) cursorY = (int)lines.size() - 1;
        }
        else {
            lines[cursorY] = "";
        }
        cursorX = 0;
    }
    modified = true;
}

void Editor::pasteClipboard() {
    if (hasSelection()) { saveState(); deleteSelected(); }

    if (OpenClipboard(NULL)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData) {
            char* text = (char*)GlobalLock(hData);
            if (text) {
                saveState();
                std::string str(text);
                GlobalUnlock(hData);
                CloseClipboard();

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
                adjustScroll();
                return;
            }
            GlobalUnlock(hData);
        }
        CloseClipboard();
    }
}

void Editor::run() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(h, &ci);
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(h, &ci);

    SetConsoleTitleA("Text Editor");

    lines.push_back("");
    cursorX = 0; cursorY = 0; scrollY = 0;
    modified = false; undoTop = -1; redoTop = -1;
    selecting = false; selStartX = 0; selStartY = 0;

    while (true) {
        render();
        handleInput();
    }
}
