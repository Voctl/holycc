// HolyC compiler – entry point
#include "holyc/driver.h"

int main(int argc, char **argv) {
    return driver_main(argc, argv); // all work is delegated to the driver
}
