// HolyC compiler – entry point
// "If main() doesn't work, nothing works. Let's cook."
#include "holyc/driver.h"

int main(int argc, char **argv) {
    return driver_main(argc, argv); // all work is delegated to the driver
}
