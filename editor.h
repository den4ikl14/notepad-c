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

struct ThemeStyle {
    WORD header = 0;
    WORD text = 0;
    WORD lineNum = 0;
    WORD selection = 0;
    WORD status = 0;
    WORD prompt = 0;
};

struct Editor {
    std::vector<std::string> lines;
    int cursorX = 0;
    int cursorY = 0;
    int scrollY = 0;
    std::string filename;
    bool modified = false;
    bool darkTheme = false;
    bool running = true;
    std::string lastSearch;
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
    void openFile();
    void adjustScroll();
    void findText();
    void findNext();
    void replaceText();
    void copySelection();
    void cutSelection();
    void pasteClipboard();
    void copyToClipboard(const std::string& text);
    std::string getSelectedText();
    void deleteSelected();
    bool hasSelection();
    void toggleTheme();
    void showHelp();
    ThemeStyle currentTheme() const;
    std::string readStatusInput(const std::string& prompt);
    bool tryExit();
    void getViewport(int& screenWidth, int& editLineCount) const;
    void getWindowLayout(int& winTop, int& winBottom, int& width) const;
    void showStatusMessage(const std::string& msg, bool waitKey = true);
};

void gotoxy(int x, int y);
