#include "app.h"
#include <cstdio>

int main(int /*argc*/, char* /*argv*/[]) {
    App app;

    if (!app.init()) {
        fprintf(stderr, "Failed to initialize spider-pet\n");
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}
