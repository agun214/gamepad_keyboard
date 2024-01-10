#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <errno.h>
#include <unistd.h>  // Add this header for usleep

/*

sudo apt-get install libsdl2-dev
sudo apt-get install libgl1-mesa-dev
sudo apt-get install libevdev-dev
sudo apt-get install libusb-1.0-0-dev
sudo apt-get install libevdev-dev
sudo apt-get install libxtst-dev

gcc gamepad_keyboard_mouse.c -o gamepad_keyboard_mouse -I/usr/include/libevdev-1.0 -levdev -lX11 -lXtst

*/

#define NUM_BUTTONS 14

// Structure to store the state of each button
struct ButtonState {
    int ps3Code;  // PS3 button code
    int state;    // 0 for released, 1 for pressed
    KeySym key;   // X11 KeySym associated with the button
};

// Structure to store the state of each joystick
struct JoystickState {
    int joystick;  // 
    int state;  // 
};

void simulateMouseMove(Display *display, int deltaX, int deltaY) {
    XTestFakeRelativeMotionEvent(display, deltaX, deltaY, CurrentTime);
    XFlush(display);
}

void simulateKeyPress(Display *display, KeySym key) {
    KeyCode code = XKeysymToKeycode(display, key);
    XTestFakeKeyEvent(display, code, True, 0);
    XFlush(display);
}

void simulateKeyRelease(Display *display, KeySym key) {
    KeyCode code = XKeysymToKeycode(display, key);
    XTestFakeKeyEvent(display, code, False, 0);
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

	// Array to store the state of each button
	struct ButtonState buttonStates[NUM_BUTTONS] = {
		{304, 0, XK_A},      // A
		{305, 0, XK_B},      // B
		{307, 0, XK_X},      // X
		{308, 0, XK_Y},      // Y
		{310, 0, XK_BackSpace},  // L1
		{311, 0, XK_Control_R},  // R1
		{314, 0, XK_Escape},     // SELECT
		{315, 0, XK_Tab},        // START
		{316, 0, XK_A},          // HOME		
		{317, 0, XK_BackSpace},  // L3
		{318, 0, XK_Delete},     // R3
		{2, 0, XK_Shift_R},      // L2
		{5, 0, XK_Alt_R},        // R2
		{0, 0, XK_Alt_L},        // L2 (placeholder, you can update it)
		// Add more buttons as needed
	};

	// Array to store the state of each button
	struct JoystickState JoystickStates[2] = {
		{0, 0},      // A
		{1, 0},      // B
	};

	int deltaX;
	int deltaY;

    while (1) {
        struct input_event ev;
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0) {
		    // Find the corresponding button state
		    int buttonIndex = -1;

		    for (int i = 0; i < NUM_BUTTONS; i++) {
		        // Print the values for debugging
		        printf("ev.code: %d, ev.value: %d, buttonStates[%d].ps3Code: %d\n", ev.code, ev.value, i, buttonStates[i].ps3Code);

		        if (ev.code == buttonStates[i].ps3Code) {
		            buttonIndex = i;
		            break;
		        }
		    }

			if (ev.type == EV_ABS) {
                if (ev.code == JoystickStates[0].joystick) {
                    // Map joystick X-axis to mouse horizontal movement
                    deltaX = ev.value / 2048;  // Adjust sensitivity as needed
					JoystickStates[0].state = deltaX;

                } else if (ev.code == JoystickStates[1].joystick) {
                    // Map joystick Y-axis to mouse vertical movement
                    deltaY = (ev.value + 1) / 2048;  // Adjust sensitivity as needed
					JoystickStates[1].state = deltaY;
                }
				//simulateMouseMove(display, deltaX, deltaY);								
            }

		    if (buttonIndex != -1) {
				switch (ev.value) {
					case 1 ... 255:  // Button pressed
						if (buttonStates[buttonIndex].state == 0) {
							simulateKeyPress(display, buttonStates[buttonIndex].key);
							buttonStates[buttonIndex].state = 1;
						}
						break;

					case 0:  // Button released
						if (buttonStates[buttonIndex].state == 1) {
							simulateKeyRelease(display, buttonStates[buttonIndex].key);
							buttonStates[buttonIndex].state = 0;
						}
						break;
				}
		    } 
        } else if (errno != EAGAIN) {
            // Error reading event, exit loop
            break;
        }
		simulateMouseMove(display, deltaX, deltaY);
		usleep(5000);  // Sleep for 5000 microseconds (5 milliseconds)
    }

    // Clean up resources
    XCloseDisplay(display);
    libevdev_free(dev);
    close(fd);

    return 0;
}
