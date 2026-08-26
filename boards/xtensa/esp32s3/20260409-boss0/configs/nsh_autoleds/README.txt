# NSH with ARCH LEDs and USERLEDs Test Configuration

## Board
- **Board:** 20260409-boss0
- **Chip:** ESP32-S3-WROOM-1N16R8

## Configuration Purpose
This configuration is used to test both ARCH LED (automatic) and USERLED (manual) functionality on the 20260409-boss0 board.

## Enabled Features
- `CONFIG_USERLED=y` - User LED upper half driver
- `CONFIG_USERLED_LOWER=y` - Generic lower half LED driver
- `CONFIG_TESTING_LEDTEST=y` - LED test application
- `CONFIG_20260409_BOSS0_GPIO_EXP=y` - XL9555 GPIO Expander
- `CONFIG_ARCH_LEDS=y` - Architecture LED automatic control
- `CONFIG_BOARDCTL_RESET=y` - Board reset support

## Hardware
- **SYSLED:** XL9555 P14 (Active High)
- **I2C:** GPIO1 (SCL), GPIO2 (SDA)

## ARCH LED Behavior
| System Event | LED State |
|--------------|-----------|
| LED_STARTED | OFF |
| LED_HEAPALLOCATE | OFF |
| LED_IRQSENABLED | OFF |
| LED_STACKCREATED | **ON** |
| LED_INIRQ | N/C |
| LED_SIGNAL | N/C |
| LED_ASSERTION | N/C |
| LED_PANIC | ON |

## Test Commands
```bash
nsh> ledtest status      # Show LED status
nsh> ledtest on 0       # Turn LED0 on
nsh> ledtest off 0      # Turn LED0 off
nsh> ledtest blink 0     # Blink LED0 5 times
```

## Test Results

| Date | Result | Notes |
|------|--------|-------|
| 2026-08-26 | PASS | LED toggle and auto-LED works correctly |
