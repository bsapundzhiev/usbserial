/*  usbserial.c - Arduino communications on a USB to serial converter.
 *
 *  Copyright (C) 2014  Borislav Sapundzhiev <bsapundjiev_AT_gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 256

struct serial_opt {
    char *name;
    int handler;
    tcflag_t baud;
    struct termios options;
    int timeout;
};

static void serial_port_close(struct serial_opt *serial);
static int serial_port_open(struct serial_opt *serial);
static int serial_port_readline(struct serial_opt *serial, char *buf, int len);
static int serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read);
static int serial_port_write(int fd, char *write_buffer);
static void sigint_handler(int sig);
static tcflag_t parse_baudrate(int requested);
static int serial_wait_fd(int fd, short stimeout);

static struct serial_opt *pserial;

int main(int argc, char **argv)
{
    int chars_read, messages_read=0;
    char buffer[MAX_COMMAND_LENGTH] = {'\0'};

    int opt, baud, write=0, count=0;

    struct serial_opt serial = {
        .name = "/dev/ttyUSB0",
        .handler = -1,
        .baud = B9600,
        .timeout = -1,
    };

    pserial = &serial;

    while ((opt = getopt(argc, argv, "nwb:t:c:")) != -1) {
        switch (opt) {
        case 'n':
            serial.name = argv[optind];
            break;
        case 'b':
            baud = atoi(optarg);
            serial.baud = parse_baudrate(baud);
            break;
        case 'w':
            write =1;
            if (strlen(argv[optind]) < MAX_COMMAND_LENGTH ) {
                strcpy(buffer, argv[optind]);
            } else  {
                fprintf(stderr,"command must be less than %d chars.", MAX_COMMAND_LENGTH );
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            serial.timeout = atoi(optarg);
            break;
        case 'c':
            count = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Simple USB to serial converter monitor\n\n");
            fprintf(stderr, "Usage: %s [-n name] device [-b baud] rate [-t sec] timeout [-w string] write command [-c num] count messages\n\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (serial_port_open(&serial) == -1) {
        printf("Unable to open %s\n", serial.name);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Serial open: %s\n", serial.name);

    if (write) {
        printf("Write %s...", buffer);
        if (serial_port_write(serial.handler, buffer)) {
            printf("OK\n");
        } else {
            printf("Failed\n");
        }

        serial_port_close(&serial);
        return 0;
    }

    if (!count) {
        fprintf(stderr, "Press Ctrl+C to exit the program.\n");
        signal (SIGINT, (void*)sigint_handler);
    }

    while (chars_read != -1) {
        chars_read = serial_port_readline(&serial, buffer, sizeof(buffer));

        if (chars_read > 0) {

            printf("%s\n", buffer);
            if (++messages_read == count) {
                serial_port_close(&serial);
                break;
            }
        }
    }

    return 0;
}

void serial_port_close(struct serial_opt *serial)
{
    tcsetattr(serial->handler,TCSANOW,&(serial)->options);
    close(serial->handler);
}

int serial_port_open(struct serial_opt *serial)
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

int serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read)
{
    int chars_read = read(fd, read_buffer, max_chars_to_read);

    return chars_read;
}

int serial_port_write(int fd, char *write_buffer)
{
    int bytes_written;
    size_t len = 0;

    len = strlen(write_buffer);
    bytes_written = write(fd, write_buffer, len);

    return (bytes_written == len);
}

void  sigint_handler(int sig)
{
    serial_port_close(pserial);
    exit (sig);
}

int serial_port_readline(struct serial_opt *serial, char *buf, int len)
{
    int retval;
    char *ptr = buf;
    char *ptr_end = ptr + len - 1;

    while (ptr < ptr_end) {

        retval = serial_wait_fd(serial->handler, serial->timeout) ;
        if (retval == -1) {
            perror("select()");
            return -1;
        } else if (retval == 0) {
            fprintf(stderr, "%s No data within %d seconds.\n", __func__, serial->timeout);
            return -1;
        }

        switch ( read(serial->handler, ptr, 1)) {
        case 1:
            if (*ptr == '\r')
                continue;
            else if (*ptr == '\n') {
                *ptr = '\0';
                return ptr - buf;
            } else {
                ptr++;
                continue;
            }
        case 0:
            *ptr = '\0';
            return ptr - buf;
        default:
            printf("%s() failed: %s\n", __func__, strerror(errno));
            return -1;
        }
    }

    return len;
}

int serial_wait_fd(int fd, short stimeout)
{
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = stimeout;
    tv.tv_usec = 0;
    /* > 0 if descriptor is readable, 0 timeo, -1 err*/
    return  select(fd + 1, &rfds, NULL, NULL, stimeout == -1 ? NULL :  &tv);
}

static tcflag_t parse_baudrate(int requested)
{
    int baudrate;

    switch (requested) {
    case 50:
        baudrate = B50;
        break;
    case 75:
        baudrate = B75;
        break;
    case 110:
        baudrate = B110;
        break;
    case 134:
        baudrate = B134;
        break;
    case 150:
        baudrate = B150;
        break;
    case 200:
        baudrate = B200;
        break;
    case 300:
        baudrate = B300;
        break;
    case 600:
        baudrate = B600;
        break;
    case 1200:
        baudrate = B1200;
        break;
    case 1800:
        baudrate = B1800;
        break;
    case 2400:
        baudrate = B2400;
        break;
    case 4800:
        baudrate = B4800;
        break;
    case 9600:
        baudrate = B9600;
        break;
    case 19200:
        baudrate = B19200;
        break;
    case 38400:
        baudrate = B38400;
        break;
    case 57600:
        baudrate = B57600;
        break;
    case 115200:
        baudrate = B115200;
        break;
    case 230400:
        baudrate = B230400;
        break;
    case 460800:
        baudrate = B460800;
        break;
    case 500000:
        baudrate = B500000;
        break;
    case 576000:
        baudrate = B576000;
        break;
    case 921600:
        baudrate = B921600;
        break;
    case 1000000:
        baudrate = B1000000;
        break;
    case 1152000:
        baudrate = B1152000;
        break;
    case 1500000:
        baudrate = B1500000;
        break;
    case 2000000:
        baudrate = B2000000;
        break;
    case 2500000:
        baudrate = B2500000;
        break;
    case 3000000:
        baudrate = B3000000;
        break;
    case 3500000:
        baudrate = B3500000;
        break;
    case 4000000:
        baudrate = B4000000;
        break;
    default:
        baudrate = 0;
    }

    return baudrate;
}

