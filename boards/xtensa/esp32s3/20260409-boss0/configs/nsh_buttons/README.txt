# NSH with Buttons Test Configuration

## Board
- **Board:** 20260409-boss0
- **Chip:** ESP32-S3-WROOM-1N16R8

## Configuration Purpose
This configuration is used to test button functionality on the 20260409-boss0 board.

## Enabled Features
- `CONFIG_ARCH_BUTTONS=y` - Architecture button support
- `CONFIG_ARCH_IRQBUTTONS=y` - Button interrupt support
- `CONFIG_INPUT_BUTTONS=y` - Input button subsystem
- `CONFIG_INPUT_BUTTONS_LOWER=y` - Button lower half driver
- `CONFIG_EXAMPLES_BUTTONS=y` - Button test example
- `CONFIG_EXAMPLES_BUTTONS_POLL=y` - Poll mode button test
- `CONFIG_20260409_BOSS0_GPIO_EXP=y` - XL9555 GPIO Expander
- `CONFIG_SCHED_HPWORK=y` - High priority work queue (required for GPIO interrupt)
- `CONFIG_BOARDCTL_RESET=y` - Board reset support

## Hardware
- **Button:** XL9555 P00 (Active Low - press to ground)
- **I2C:** GPIO1 (SCL), GPIO2 (SDA)

## Button Behavior
| Action | Hardware State | NuttX Return |
|--------|----------------|--------------|
| Button pressed | Low (0) | 0 |
| Button released | High (1) | 1 |

## Test Commands
```bash
nsh> buttons
nsh> btndemo
```

## Test Results

| Date | Result | Notes |
|------|--------|-------|
| 2026-08-26 | PASS | Button press/release working |
