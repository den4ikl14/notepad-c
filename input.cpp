#include "editor.h"
#include <conio.h>
#include <iostream>
#include <fstream>

void Editor::adjustScroll() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;

    if (cursorY < scrollY) scrollY = cursorY;
    if (cursorY >= scrollY + screenHeight) scrollY = cursorY - screenHeight + 1;
}

void Editor::handleInput() {
    int key = _getch();

    if (key == 0 || key == 224) {
        int arrow = _getch();
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shift && !selecting) {
            selecting = true;
            selStartX = cursorX;
            selStartY = cursorY;
        }
        if (!shift) selecting = false;

        if (arrow == 72) {
            if (cursorY > 0) { cursorY--; int len = (int)lines[cursorY].size(); if (cursorX > len) cursorX = len; }
        }
        else if (arrow == 80) {
            if (cursorY < (int)lines.size() - 1) { cursorY++; int len = (int)lines[cursorY].size(); if (cursorX > len) cursorX = len; }
        }
        else if (arrow == 75) {
            if (cursorX > 0) cursorX--;
            else if (cursorY > 0) { cursorY--; cursorX = (int)lines[cursorY].size(); }
        }
        else if (arrow == 77) {
            if (cursorX < (int)lines[cursorY].size()) cursorX++;
            else if (cursorY < (int)lines.size() - 1) { cursorY++; cursorX = 0; }
        }
        else if (arrow == 71) { cursorX = 0; }
        else if (arrow == 79) { cursorX = (int)lines[cursorY].size(); }
        else if (arrow == 73) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            int sh = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;
            cursorY -= sh; if (cursorY < 0) cursorY = 0;
            int len = (int)lines[cursorY].size(); if (cursorX > len) cursorX = len;
        }
        else if (arrow == 81) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            int sh = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;
            cursorY += sh; if (cursorY >= (int)lines.size()) cursorY = (int)lines.size() - 1;
            int len = (int)lines[cursorY].size(); if (cursorX > len) cursorX = len;
        }

        adjustScroll();
        return;
    }

    if (key == 17) { exit(0); }
    else if (key == 19) { saveFile(); }
    else if (key == 15) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 3;
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD attr = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        gotoxy(0, screenHeight + 1);
        SetConsoleTextAttribute(h, attr);
        std::cout << std::string(80, ' ');
        gotoxy(0, screenHeight + 1);
        std::cout << " Open file: ";
        SetConsoleTextAttribute(h, csbi.wAttributes);
        std::string fname;
        std::cin >> fname;
        std::ifstream file(fname);
        if (file.is_open()) {
            lines.clear();
            std::string line;
            while (std::getline(file, line)) lines.push_back(line);
            file.close();
            if (lines.empty()) lines.push_back("");
            filename = fname;
            cursorX = 0; cursorY = 0; scrollY = 0;
            modified = false; selecting = false;
        }
    }
    else if (key == 6) { findText(); }
    else if (key == 18) { replaceText(); }
    else if (key == 3) { copySelection(); }
    else if (key == 24) { cutSelection(); }
    else if (key == 22) { pasteClipboard(); }
    else if (key == 1) { selecting = true; selStartX = 0; selStartY = 0; cursorY = (int)lines.size() - 1; cursorX = (int)lines[cursorY].size(); adjustScroll(); }
    else if (key == 26) {
        if (undoTop >= 0) {
            if (redoTop < MAX_UNDO - 1) {
                redoTop++;
                redoStack[redoTop].lines = lines;
                redoStack[redoTop].cursorX = cursorX;
                redoStack[redoTop].cursorY = cursorY;
            }
            lines = undoStack[undoTop].lines;
            cursorX = undoStack[undoTop].cursorX;
            cursorY = undoStack[undoTop].cursorY;
            undoTop--; modified = true; selecting = false;
            adjustScroll();
        }
    }
    else if (key == 25) {
        if (redoTop >= 0) {
            saveState();
            lines = redoStack[redoTop].lines;
            cursorX = redoStack[redoTop].cursorX;
            cursorY = redoStack[redoTop].cursorY;
            redoTop--; modified = true; selecting = false;
            adjustScroll();
        }
    }
    else if (key == 13) {
        selecting = false;
        saveState();
        std::string newLine = lines[cursorY].substr(cursorX);
        lines[cursorY] = lines[cursorY].substr(0, cursorX);
        lines.insert(lines.begin() + cursorY + 1, newLine);
        cursorY++; cursorX = 0; modified = true;
        adjustScroll();
    }
    else if (key == 8) {
        selecting = false;
        saveState();
        if (cursorX > 0) { lines[cursorY].erase(cursorX - 1, 1); cursorX--; }
        else if (cursorY > 0) {
            int prevLen = (int)lines[cursorY - 1].size();
            lines[cursorY - 1] += lines[cursorY];
            lines.erase(lines.begin() + cursorY);
            cursorY--; cursorX = prevLen;
        }
        modified = true; adjustScroll();
    }
    else if ((key >= 32 && key <= 126) || (key >= 128 && key <= 255)) {
        if (hasSelection()) { saveState(); deleteSelected(); }
        selecting = false;
        saveState();
        lines[cursorY].insert(cursorX, 1, (char)key);
        cursorX++; modified = true;
        adjustScroll();
    }
}
