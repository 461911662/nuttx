# NuttShell (NSH) Configuration

## Board
- **Board:** 20260409-boss0
- **Chip:** ESP32-S3-WROOM-1N16R8

## Configuration Purpose
Basic NSH (NuttX Shell) configuration for 20260409-boss0 board.

## Features
- NSH as primary initialization entry point
- UART0 serial console
- SPI Flash and SPIRAM support (Octal mode)
- ProcFS and TmpFS file systems
- Board reset support (CONFIG_BOARDCTL_RESET)
- XL9555 GPIO Expander support (CONFIG_20260409_BOSS0_GPIO_EXP)
- TAB command auto-completion (CONFIG_READLINE_TABCOMPLETION)
- Command history (CONFIG_READLINE_CMD_HISTORY)
- Terminal TERMIOS support (CONFIG_SERIAL_TERMIOS)

## Hardware
- **I2C:** GPIO1 (SCL), GPIO2 (SDA) - for XL9555 GPIO expander
- **XL9555 Interrupt:** GPIO16
- **LED:** XL9555 P14 (Active High)
- **Button:** XL9555 P00 (Active Low)
- **Reset:** Supported via board_reset()

## Available Commands
```
nsh> ?
help usage:  help [-v] [<cmd>]

    .           cp          exit        ls          rm          uname
    [           cmp         expr        mkdir       rmdir       umount
    ?           dirname     false       mkrd        set         unset
    alias       dd          fdinfo      mount       sleep       uptime
    unalias     df          free        mv          source      usleep
    basename    dmesg       help        pidof       test        xd
    break       echo        hexdump     printf      time        wait
    cat         env         kill        ps          true
    cd          exec        pkill       pwd         truncate

Builtin Apps:
    sh     nsh
```

## Test Results

| Date | Result | Notes |
|------|--------|-------|
| 2026-08-26 | PASS | Basic NSH works correctly |
