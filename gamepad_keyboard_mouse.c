#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <errno.h>

/*

gcc gamepad_keyboard_mouse.c -o gamepad_keyboard_mouse -I/usr/include/libevdev-1.0 -levdev -lX11 -lXtst

*/

#define JOYSTICK_AXIS_X 0
#define JOYSTICK_AXIS_Y 1

void simulateMouseMove(Display *display, int deltaX, int deltaY) {
    XTestFakeRelativeMotionEvent(display, deltaX, deltaY, CurrentTime);
    XFlush(display);
}

int main() {

    // Input device vendor product ID
    int vendor_id = 0x045e;
    int product_id = 0x028e;

    // Find the device with the matching vendor/product IDs
    struct libevdev* dev = NULL;
    int fd = -1;
    int rc = -1;

    for (int i = 0; i < 300; i++) {
        char path[128];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        fd = open(path, O_RDONLY|O_NONBLOCK);
        if (fd < 0) {continue;}

        rc = libevdev_new_from_fd(fd, &dev);
        if (rc < 0) {close(fd); continue;}

        if (libevdev_get_id_vendor(dev) == vendor_id &&
            libevdev_get_id_product(dev) == product_id) 
			{break;}
    }

    if (fd < 0 || rc < 0) {
        fprintf(stderr, "Failed to find device with vendor/product ID %04x:%04x\n", vendor_id, product_id);
        return 1;
    }

    printf("Device found: %s\n", libevdev_get_name(dev));


    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Unable to open display.\n");
        libevdev_free(dev);
        close(fd);
        return 1;
    }

    while (1) {
        struct input_event ev;
        if (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
            if (ev.type == EV_ABS) {
                if (ev.code == JOYSTICK_AXIS_X) {
                    // Map joystick X-axis to mouse horizontal movement
                    int deltaX = ev.value / 2048;  // Adjust sensitivity as needed
                    simulateMouseMove(display, deltaX, 0);
                } else if (ev.code == JOYSTICK_AXIS_Y) {
                    // Map joystick Y-axis to mouse vertical movement
                    int deltaY = ev.value / 2048;  // Adjust sensitivity as needed
                    simulateMouseMove(display, 0, deltaY);
                }
            }
        } else if (errno != EAGAIN) {
            // Error reading event, exit loop
            break;
        }
    }

    // Clean up resources
    XCloseDisplay(display);
    libevdev_free(dev);
    close(fd);

    return 0;
}
