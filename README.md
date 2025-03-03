# OSC Lab1 - Sean20405
### Project Structure
```
Lab1/
├── build/            # Compiled object files
├── include
│   ├── gpio.h        # GPIO definitions
│   ├── mailbox.h     # Mailbox communication protocol
│   ├── string.h      # String handling functions
│   └── uart.h        # UART communication
├── src/
│   ├── boot.S        # System startup code
│   ├── linker.ld     # Linker script
│   ├── mailbox.c     # Mailbox communication implementation
│   ├── mm.S          # Memory operation functions
│   ├── shell.c       # Command-line interface
│   ├── string.c      # String handling implementation
│   └── uart.c        # UART communication implementation
├── Makefile          # Build script
└── README
```

### Compile and Run
1. Compile
  ```
  make
  ```
2. Run with QEMU
  ```
  make run
  ```
3. Run with QEMU and debug with GDB:
  ```
  make run-gdb
  ```
