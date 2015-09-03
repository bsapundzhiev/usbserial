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
#include <errno.h>

#ifndef _WIN32
#include "usbserial_linux.h"
#else 
#include "usbserial_win32.h"
#endif

#include "usbserial.h"

#define MAX_BUF_LENGTH  256

struct serial_buf {
   int len;
   char buf[MAX_BUF_LENGTH];
};

static int serial_port_readline(int fd, int timeo, char *buf, int len);
static int serial_get_input(char *buf, int len);
static void sigint_handler(int sig);
static tcflag_t parse_baudrate(int requested);
static int serial_wait_fd(int fd, short stimeout);
static void serial_output(void *p);
static int serial_term(struct serial_opt *serial, const char* outbuf);
static int serial_write_buf(struct serial_opt *serial, const char * buf);
static struct serial_opt *pserial;

usbserial_ops *pusbserial_ops;

int main(int argc, char **argv)
{
    int opt;
    char *pbuf = NULL;
    struct serial_opt serial = 
#ifdef _WIN32
    { DEFAULT_USB_DEV, -1, B9600, -1, 0, 1};
#else
    { .name = DEFAULT_USB_DEV,
      .handler = -1,
      .baud = B9600,
      .timeout = -1,    
      .max_msgs = 0,
      .endl = 1,
    };
#endif

    pserial = &serial;

    while ((opt = getopt(argc, argv, "nwb:t:c:r")) != -1) {
        switch (opt) {
        case 'n':
            serial.name = argv[optind];
            break;
        case 'b':{
        int baud = atoi(optarg);
        serial.baud = parse_baudrate(baud);
        } break;
        case 'w':
            if (strlen(argv[optind]) < MAX_BUF_LENGTH) {
                pbuf =argv[optind];
            } else  {
                fprintf(stderr,"command must be less than %d chars.", MAX_BUF_LENGTH);
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            serial.timeout = atoi(optarg);
            break;
        case 'c':
            serial.max_msgs = atoi(optarg);
            break;
        case 'r':
            serial.endl = 0;
            break;
        default: /* '?' */
            fprintf(stderr, "USB2Serial terminal %s, %s\n\n", VERSION, __DATE__);
            fprintf(stderr, "Usage: %s [-n name] device [-b baud] rate [-t sec] timeout [-w string] write command [-c num] count messages [-r] don't add <CR>\n\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    serial_term(&serial, pbuf);
    return 0; 
}

static int serial_term(struct serial_opt *serial, const char* outbuf) 
{
    struct serial_buf sb = { 0, {0} };
    pusbserial_ops = serial_initialize(serial);

    if (pusbserial_ops->serial_port_open(serial) == -1) {
        printf("Unable to open %s : %s\n", serial->name , strerror(errno));
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "**************************************************\n");
    fprintf(stderr, "* Serial open: %20s              *\n", serial->name);
    fprintf(stderr, "**************************************************\n");

    if (outbuf) {
        if (serial_write_buf(serial, outbuf)) {
            serial->timeout = (serial->timeout == -1) ? 2 : serial->timeout;
            serial_output((void*) serial);
        } else {
            printf("Write to %s failed\n", serial->name);
            pusbserial_ops->serial_port_close(serial);
            return -1;
        }
    }

    if (!serial->max_msgs) {
        fprintf(stderr, "Type quit, bye or ^C to exit.\n");
        signal (SIGINT, (void*)sigint_handler);
    }

    SPAWN_THREAD(serial_output, (void*) serial);

    while(1) {

        serial_get_input(sb.buf, sizeof(sb.buf));

        if (!strcmp(sb.buf, "bye") || !strcmp(sb.buf, "quit")) {
            break;
        } else {
            serial_write_buf(serial, sb.buf);
        }
    }
    printf("Bye!\n");
    pusbserial_ops->serial_port_close(serial);
    return 0;
}

static int serial_write_buf(struct serial_opt *serial, const char * buf) 
{
    int res = pusbserial_ops->serial_port_write(serial->handler, buf);
    fprintf(stderr,">> %s\n", buf);
    if (serial->endl) {
        res = pusbserial_ops->serial_port_write(serial->handler, "\r\n");
    }

    return res;
}

static void serial_output(void *p)
{
     int messages_read=0;
     struct serial_opt *serial = (struct serial_opt *)p;
     struct serial_buf sb = { 0, {0}};   
     
     while (sb.len != -1) {
        sb.len = serial_port_readline(serial->handler, serial->timeout, sb.buf, sizeof(sb.buf));

        if (sb.len > 0) {
            printf("<< %s\n", sb.buf);
            if (++messages_read == serial->max_msgs) {
                break;    
            }
        }
     }

    printf("serial_output end!\n");
    pusbserial_ops->serial_port_close(serial);
    exit(EXIT_SUCCESS);
}

static int serial_get_input(char *buf, int len) 
{
    //fprintf(stderr,">> ");
    return serial_port_readline(_fileno(stdin), -1, buf, len );
}

void  sigint_handler(int sig)
{
    pusbserial_ops->serial_port_close(pserial);
    exit (sig);
}

static int serial_port_readline(int fd, int timeo, char *buf, int len)
{
    int retval;
    char *ptr = buf;
    char *ptr_end = ptr + len - 1;

    while (ptr < ptr_end) {

    retval = serial_wait_fd(fd, timeo);
    if (retval == -1 && errno != 0) {
        perror("select()");
        return -1;
    } else if (retval == 0) {
        fprintf(stderr, "%s No data within %d seconds.\n", __func__, timeo);
        return -1;
    }
        
        switch (pusbserial_ops->serial_port_read(fd, ptr, 1)) {
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
            if(errno != EAGAIN || errno != EWOULDBLOCK) {
                printf("%s() failed: %s\n", __func__, strerror(errno));
                return -1;
            }
        }
    }

    return len;
}

int serial_wait_fd(int fd, short stimeout)
{
    fd_set rfds;
    struct timeval tv = {stimeout, 0};

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    /* > 0 if descriptor is readable, 0 timeo, -1 err*/
    return  select(fd + 1, &rfds, NULL, NULL, stimeout == -1 ? NULL : &tv);
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

