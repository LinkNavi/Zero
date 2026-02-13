#include "Editor.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Editor editor;

    if (!editor.init(1600, 900)) {
        std::cerr << "Failed to initialize editor\n";
        return 1;
    }

    editor.run();
    editor.shutdown();

    return 0;
}
