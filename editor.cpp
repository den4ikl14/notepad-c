#include "editor.h"
#include <iostream>
#include <fstream>
#include <sstream>

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void clearScreen() {
    system("cls");
}

int countWords(const std::vector<std::string>& lines) {
    int count = 0;
    for (const auto& line : lines) {
        bool inWord = false;
        for (char c : line) {
            if (c != ' ' && !inWord) {
                inWord = true;
                count++;
            } else if (c == ' ') {
                inWord = false;
            }
        }
    }
    return count;
}

int countChars(const std::vector<std::string>& lines) {
    int count = 0;
    for (const auto& line : lines) {
        count += line.size();
    }
    return count;
}

void Editor::render() {
    clearScreen();

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 2;

    for (int i = 0; i < screenHeight && (i + scrollY) < (int)lines.size(); i++) {
        gotoxy(0, i);
        std::cout << (i + scrollY + 1);
        std::cout << " | ";
        std::cout << lines[i + scrollY];
    }

    gotoxy(0, screenHeight + 1);
    std::string status = filename.empty() ? "New file" : filename;
    if (modified) status += " *";
    status += "  |  Line: " + std::to_string(cursorY + 1);
    status += "  Column: " + std::to_string(cursorX + 1);
    status += "  |  Words: " + std::to_string(countWords(lines));
    status += "  Chars: " + std::to_string(countChars(lines));
    std::cout << status;

    gotoxy(0, screenHeight + 2);
    std::cout << "Ctrl+S Save | Ctrl+F Find | Ctrl+H Replace | Ctrl+Z Undo | Ctrl+Q Exit";

    gotoxy(cursorX + 4, cursorY - scrollY);
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
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 2;

        gotoxy(0, screenHeight + 1);
        std::cout << std::string(80, ' ');
        gotoxy(0, screenHeight + 1);
        std::cout << "Enter filename: ";
        std::cin >> filename;

        if (filename.find('.') == std::string::npos) {
            filename += ".txt";
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
    }
}

void Editor::run() {
    lines.push_back("");
    cursorX = 0;
    cursorY = 0;
    scrollY = 0;
    modified = false;
    undoTop = -1;
    redoTop = -1;

    while (true) {
        render();
        handleInput();
    }
}
