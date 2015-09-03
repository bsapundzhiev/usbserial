/*  usbserial_win32.c - Arduino communications on a USB to serial converter.
 *
 *  Copyright (C) 2015  Borislav Sapundzhiev <bsapundjiev_AT_gmail.com>
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
#include <fcntl.h>
#include <io.h>
#include "usbserial_win32.h"
#include "usbserial.h"

static void win32_serial_port_close(struct serial_opt *serial);
static int win32_serial_port_open(struct serial_opt *serial);
static int win32_serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read);
static int win32_serial_port_write(int fd, const char *write_buffer);

usbserial_ops win32_opts = {
    
     win32_serial_port_close,
     win32_serial_port_open,
     win32_serial_port_read,
     win32_serial_port_write,
};

usbserial_ops * serial_initialize(struct serial_opt * options)
{
    return &win32_opts;
}

static void win32_serial_port_close(struct serial_opt *serial)
{
    _close( serial->handler );
}

static int win32_serial_port_open(struct serial_opt *serial)
{
    static DCB dcb = {0};
    HANDLE hComm = CreateFile(
        serial->name,
        GENERIC_WRITE | GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,   
        NULL
    );

    if (hComm == INVALID_HANDLE_VALUE) {
        printf("CreateFile() failed with error %d\n",  GetLastError());
    }
     
    if (GetCommState(hComm, &dcb))
    {
        dcb.BaudRate = serial->baud;
        dcb.ByteSize = 8; 
        dcb.Parity = NOPARITY; 
        dcb.StopBits = ONESTOPBIT; 
    } 
    else {
        printf("Failed to get the comm state - Error: %d\n", GetLastError());
        CloseHandle(hComm);
    }

    if ( !SetCommState( hComm, &dcb ) ) {
        printf("Failed to set the comm state - Error: %d", GetLastError()); 
        CloseHandle(hComm);
    }

    serial->handler = _open_osfhandle((intptr_t)hComm, O_TEXT);
    if(serial->handler == -1) {
        printf("Error in _open_osfhandle\n");
        CloseHandle(hComm);
    }

    return serial->handler;
}

static int win32_serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read)
{
    int chars_read = _read(fd, read_buffer, max_chars_to_read);

    return chars_read;
}
    
static int win32_serial_port_write(int fd, const char *write_buffer)
{
    size_t len = strlen(write_buffer);
    size_t bytes_written = _write(fd, write_buffer, len);

    return (bytes_written == len);
}