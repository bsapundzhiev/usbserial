#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>

#include "usbserial.h"

static void linux_serial_port_close(struct serial_opt *serial);
static int linux_serial_port_open(struct serial_opt *serial);
static int linux_serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read);
static int linux_serial_port_write(int fd, char *write_buffer);

usbserial_ops linux_opts = {
	
	.serial_port_close = linux_serial_port_close,
	.serial_port_open = linux_serial_port_open,
	.serial_port_read = linux_serial_port_read,
	.serial_port_write = linux_serial_port_write,
};

usbserial_ops * serial_initialize(struct serial_opt * options)
{
    return &linux_opts;
}

	
void linux_serial_port_close(struct serial_opt *serial)
{
	tcsetattr(serial->handler,TCSANOW,&(serial)->options);
    	close(serial->handler);
}
	
int linux_serial_port_open(struct serial_opt *serial)
{
    struct termios options;

    serial->handler = open(serial->name,  O_RDWR | O_NOCTTY | O_NONBLOCK );

    if (serial->handler != -1) {
        tcgetattr(serial->handler, &(serial)->options);
        tcgetattr(serial->handler, &options);
        cfsetispeed(&options, serial->baud);
        cfsetospeed(&options, serial->baud);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_lflag |= ICANON;
        tcsetattr(serial->handler, TCSANOW, &options);
    }

    return (serial->handler);
}
	
int linux_serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read)
{
    int chars_read = read(fd, read_buffer, max_chars_to_read);

    return chars_read;
}
	
int linux_serial_port_write(int fd, char *write_buffer)
{
    size_t len = strlen(write_buffer);
    size_t bytes_written = write(fd, write_buffer, len);

    return (bytes_written == len);
}
