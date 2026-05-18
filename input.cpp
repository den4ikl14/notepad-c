#include "editor.h"
#include <conio.h>
#include <algorithm>

void Editor::adjustScroll() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 2;

    if (cursorY < scrollY)
        scrollY = cursorY;
    if (cursorY >= scrollY + screenHeight)
        scrollY = cursorY - screenHeight + 1;
}

void Editor::handleInput() {
    int key = _getch();

    if (key == 17) {
        exit(0);
    }
    else if (key == 19) {
        saveFile();
    }
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
            undoTop--;
            modified = true;
            adjustScroll();
        }
    }
    else if (key == 25) {
        if (redoTop >= 0) {
            if (undoTop < MAX_UNDO - 1) {
                undoTop++;
                undoStack[undoTop].lines = lines;
                undoStack[undoTop].cursorX = cursorX;
                undoStack[undoTop].cursorY = cursorY;
            }
            lines = redoStack[redoTop].lines;
            cursorX = redoStack[redoTop].cursorX;
            cursorY = redoStack[redoTop].cursorY;
            redoTop--;
            modified = true;
            adjustScroll();
        }
    }
    else if (key == 224) {
        int arrow = _getch();

        if (arrow == 72) {
            if (cursorY > 0) {
                cursorY--;
                cursorX = std::min(cursorX, (int)lines[cursorY].size());
            }
        }
        else if (arrow == 80) {
            if (cursorY < (int)lines.size() - 1) {
                cursorY++;
                cursorX = std::min(cursorX, (int)lines[cursorY].size());
            }
        }
        else if (arrow == 75) {
            if (cursorX > 0) {
                cursorX--;
            } else if (cursorY > 0) {
                cursorY--;
                cursorX = lines[cursorY].size();
            }
        }
        else if (arrow == 77) {
            if (cursorX < (int)lines[cursorY].size()) {
                cursorX++;
            } else if (cursorY < (int)lines.size() - 1) {
                cursorY++;
                cursorX = 0;
            }
        }
        else if (arrow == 71) {
            cursorX = 0;
        }
        else if (arrow == 79) {
            cursorX = lines[cursorY].size();
        }
        else if (arrow == 73) {
            cursorY = std::max(0, cursorY - 10);
            cursorX = std::min(cursorX, (int)lines[cursorY].size());
        }
        else if (arrow == 81) {
            cursorY = std::min((int)lines.size() - 1, cursorY + 10);
            cursorX = std::min(cursorX, (int)lines[cursorY].size());
        }

        adjustScroll();
    }
    else if (key == 13) {
        saveState();
        std::string newLine = lines[cursorY].substr(cursorX);
        lines[cursorY] = lines[cursorY].substr(0, cursorX);
        lines.insert(lines.begin() + cursorY + 1, newLine);
        cursorY++;
        cursorX = 0;
        modified = true;
        adjustScroll();
    }
    else if (key == 8) {
        saveState();
        if (cursorX > 0) {
            lines[cursorY].erase(cursorX - 1, 1);
            cursorX--;
        } else if (cursorY > 0) {
            int prevLen = lines[cursorY - 1].size();
            lines[cursorY - 1] += lines[cursorY];
            lines.erase(lines.begin() + cursorY);
            cursorY--;
            cursorX = prevLen;
        }
        modified = true;
        adjustScroll();
    }
    else if (key >= 32 && key <= 126) {
        saveState();
        lines[cursorY].insert(cursorX, 1, (char)key);
        cursorX++;
        modified = true;
        adjustScroll();
    }
}
