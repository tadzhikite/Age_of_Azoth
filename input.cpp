#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <libevdev-1.0/libevdev/libevdev.h>

int main(void) {
    int fd = open("/dev/input/event7", O_RDONLY | O_NONBLOCK);  // Replace "X" with your device number
    struct libevdev *dev = NULL;
    int rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        _exit(1);
    }
    printf("Input device name: \"%s\"\n", libevdev_get_name(dev));

     while (1) {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0) {
            printf("Event: %s %s %d\n",
                    libevdev_event_type_get_name(ev.type),
                    libevdev_event_code_get_name(ev.type, ev.code),
                    ev.value);
        }
    }

    libevdev_free(dev);
    close(fd);
    return 0;
}

