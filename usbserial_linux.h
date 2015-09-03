#ifndef _USBSERIAL_LINUX
#define _USBSERIAL_LINUX

#include <termios.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_USB_DEV "/dev/ttyUSB0"
#define _fileno fileno

#define END_TREAD() pthread_exit(NULL)
#define SPAWN_THREAD(threadfn, params)\
    {\
    pthread_t   thread;\
    pthread_create(&thread, NULL, (void*)serial_output, params);\
    pthread_detach(thread);\
    }

#endif