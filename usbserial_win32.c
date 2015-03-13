#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include "usbserial_win32.h"
#include "usbserial.h"

static void win32_serial_port_close(struct serial_opt *serial);
static int win32_serial_port_open(struct serial_opt *serial);
static int win32_serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read);
static int win32_serial_port_write(int fd, char *write_buffer);

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
		printf("INVALID_HANDLE_VALUE\n");
	}
	 
	if (GetCommState(hComm, &dcb))
	{
		dcb.BaudRate = serial->baud;
		dcb.ByteSize = 8; 
		dcb.Parity = NOPARITY; 
		dcb.StopBits = ONESTOPBIT; 
		printf("set UP DCB\n");
	}
	else
		printf("ERROR getting \n", GetLastError());

	serial->handler = _open_osfhandle((intptr_t)hComm, O_TEXT );

	CloseHandle(hComm);

	return serial->handler;
}

static int win32_serial_port_read(int fd, char *read_buffer, size_t max_chars_to_read)
{
    int chars_read = _read(fd, read_buffer, max_chars_to_read);

    return chars_read;
}
	
static int win32_serial_port_write(int fd, char *write_buffer)
{
    size_t len = strlen(write_buffer);
    size_t bytes_written = _write(fd, write_buffer, len);

    return (bytes_written == len);
}