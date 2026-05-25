#pragma once
#include <string>
#include <vector>
#include <windows.h>

#define MAX_UNDO 100

struct State {
    std::vector<std::string> lines;
    int cursorX = 0;
    int cursorY = 0;
};

struct Editor {
    std::vector<std::string> lines;
    int cursorX = 0;
    int cursorY = 0;
    int scrollY = 0;
    std::string filename;
    bool modified = false;
    std::string clipboard;
    State undoStack[MAX_UNDO];
    State redoStack[MAX_UNDO];
    int undoTop = -1;
    int redoTop = -1;

    bool selecting = false;
    int selStartX = 0;
    int selStartY = 0;

    void run();
    void render();
    void handleInput();
    void saveState();
    void saveFile();
    void adjustScroll();
    void findText();
    void replaceText();
    void copySelection();
    void cutSelection();
    void pasteClipboard();
    std::string getSelectedText();
    void deleteSelected();
    bool hasSelection();
};

void gotoxy(int x, int y);
