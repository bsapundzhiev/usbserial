#ifndef USBSERIAL_H
#define USBSERIAL_H

#include <sys/types.h>

struct serial_opt {
    char *name;
    int handler;
    tcflag_t baud;
    struct termios options;
    int timeout;
};

typedef struct s_usbserial_ops {

	void (*serial_port_close)(struct serial_opt *serial);
	int (*serial_port_open)(struct serial_opt *serial);
	int (*serial_port_read)(int fd, char *read_buffer, size_t max_chars_to_read);
	int (*serial_port_write)(int fd, char *write_buffer);

} usbserial_ops;


#endif