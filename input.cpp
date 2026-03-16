#include "editor.h"
#include <conio.h>

void Editor::handleInput() {
    int key = _getch();
    if (key == 17) {
        exit(0);
    }
}
