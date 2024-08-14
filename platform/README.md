# Platform specific files

This directory contains the platform specific files.  Only these files should
need to change to port to a new platform.  To port to a new platform, create
a new platform-_something_.c file and implement:

```c
instant_t instant_now(void);
```

This function should return the current time as an `instant_t`.

```c
void panic(const char *msg);
```

This should output the message somewhere useful, then halt or restart the
process/system.

```c
void register_ticker(ticker_t *ticker);
```

This should register a ticker function that will be called at least every "tick".
The ticker function can return hint how long the process should should sleep until
the next tick, when all the ticker functions will all be called again.

```c
void platform_init(int argc, char *argv[]);
```

This function is called at application startup time to initialise the platform.
Arguments may be passed from the system if they exist.

```c
void platform_run(void);
```

This function is called when application startup has completed.  It is not expected to exit.
This function will call ticker functions, and call `serial_recv_byte` when new data is available.

```c
void serial_recv_byte(uint8_t device, uint8_t byte);
```

This function should be called when a byte is ready on a device.
