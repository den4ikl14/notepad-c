#pragma once
#include <string>
#include <vector>
#include <windows.h>

#define MAX_UNDO 100

struct State {
    std::vector<std::string> lines;
    int cursorX, cursorY;
};

struct Editor {
    std::vector<std::string> lines;
    int cursorX, cursorY;
    int scrollY;
    std::string filename;
    bool modified;
    std::string clipboard;
    State undoStack[MAX_UNDO];
    State redoStack[MAX_UNDO];
    int undoTop, redoTop;

    void run();
    void render();
    void handleInput();
    void saveState();

    void adjustScroll(); 
};
