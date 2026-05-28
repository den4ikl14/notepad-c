#include "editor.h"
#include <conio.h>

void Editor::adjustScroll() {
    int sw, editLines;
    getViewport(sw, editLines);

    if (cursorY < scrollY) scrollY = cursorY;
    if (cursorY >= scrollY + editLines) scrollY = cursorY - editLines + 1;
    if (scrollY < 0) scrollY = 0;
}

void Editor::handleInput() {
    int key = _getch();

    if (key == 0 || key == 224) {
        int code = _getch();

        if (code == 59) { showHelp(); return; }
        if (code == 60) { toggleTheme(); return; }

        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shift && !selecting) {
            selecting = true;
            selStartX = cursorX;
            selStartY = cursorY;
        }
        if (!shift) selecting = false;

        if (code == 72) {
            if (cursorY > 0) {
                cursorY--;
                int len = (int)lines[cursorY].size();
                if (cursorX > len) cursorX = len;
            }
        }
        else if (code == 80) {
            if (cursorY < (int)lines.size() - 1) {
                cursorY++;
                int len = (int)lines[cursorY].size();
                if (cursorX > len) cursorX = len;
            }
        }
        else if (code == 75) {
            if (cursorX > 0) cursorX--;
            else if (cursorY > 0) {
                cursorY--;
                cursorX = (int)lines[cursorY].size();
            }
        }
        else if (code == 77) {
            if (cursorX < (int)lines[cursorY].size()) cursorX++;
            else if (cursorY < (int)lines.size() - 1) {
                cursorY++;
                cursorX = 0;
            }
        }
        else if (code == 71) { cursorX = 0; }
        else if (code == 79) { cursorX = (int)lines[cursorY].size(); }
        else if (code == 73) {
            int sw, editLines;
            getViewport(sw, editLines);
            cursorY -= editLines;
            if (cursorY < 0) cursorY = 0;
            int len = (int)lines[cursorY].size();
            if (cursorX > len) cursorX = len;
        }
        else if (code == 81) {
            int sw, editLines;
            getViewport(sw, editLines);
            cursorY += editLines;
            if (cursorY >= (int)lines.size()) cursorY = (int)lines.size() - 1;
            int len = (int)lines[cursorY].size();
            if (cursorX > len) cursorX = len;
        }

        adjustScroll();
        return;
    }

    if (key == 17) { tryExit(); }
    else if (key == 19) { saveFile(); }
    else if (key == 15) { openFile(); }
    else if (key == 6) { findText(); }
    else if (key == 14) { findNext(); }
    else if (key == 18) { replaceText(); }  
    else if (key == 3) { copySelection(); }
    else if (key == 24) { cutSelection(); }
    else if (key == 22) { pasteClipboard(); }
    else if (key == 1) {
        selecting = true;
        selStartX = 0; selStartY = 0;
        cursorY = (int)lines.size() - 1;
        cursorX = (int)lines[cursorY].size();
        adjustScroll();
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
            selecting = false;
            adjustScroll();
        }
    }
    else if (key == 25) {
        if (redoTop >= 0) {
            saveState();
            lines = redoStack[redoTop].lines;
            cursorX = redoStack[redoTop].cursorX;
            cursorY = redoStack[redoTop].cursorY;
            redoTop--;
            modified = true;
            selecting = false;
            adjustScroll();
        }
    }
    else if (key == 13) {
        selecting = false;
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
        
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) {
            replaceText();
            return;
        }
        if (hasSelection()) {
            saveState();
            deleteSelected();
        }
        else {
            selecting = false;
            saveState();
            if (cursorX > 0) {
                lines[cursorY].erase(cursorX - 1, 1);
                cursorX--;
            }
            else if (cursorY > 0) {
                int prevLen = (int)lines[cursorY - 1].size();
                lines[cursorY - 1] += lines[cursorY];
                lines.erase(lines.begin() + cursorY);
                cursorY--;
                cursorX = prevLen;
            }
        }
        modified = true;
        adjustScroll();
    }
    else if ((key >= 32 && key <= 126) || (key >= 128 && key <= 255)) {
        if (hasSelection()) {
            saveState();
            deleteSelected();
        }
        else {
            saveState();
        }
        selecting = false;
        lines[cursorY].insert(cursorX, 1, (char)key);
        cursorX++;
        modified = true;
        adjustScroll();
    }
}
