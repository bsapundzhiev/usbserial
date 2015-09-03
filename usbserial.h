#ifndef USBSERIAL_H
#define USBSERIAL_H

#include <sys/types.h>

#define  VERSION "0.1.0-beta"

struct serial_opt {
    char *name;
    int handler;
    tcflag_t baud;
#ifndef _WIN32
    struct termios options;
#endif
    int timeout;
    int max_msgs;
    int endl;
};

typedef struct s_usbserial_ops {

    void (*serial_port_close)(struct serial_opt *serial);
    int (*serial_port_open)(struct serial_opt *serial);
    int (*serial_port_read)(int fd, char *read_buffer, size_t max_chars_to_read);
    int (*serial_port_write)(int fd, const char *write_buffer);

} usbserial_ops;

usbserial_ops * serial_initialize(struct serial_opt * options);

#endif