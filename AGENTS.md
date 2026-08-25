# AGENTS.md - Apache NuttX Development Guidelines

This file provides guidelines and commands for AI agents working on the Apache NuttX RTOS codebase.

## Global Rules

- Use **Chinese (中文)** for all conversations and communications
- Follow the NuttX C Coding Standard strictly (see below)
- Always run code style checks before submitting changes
- Include Apache 2.0 license headers in all new files
- **我负责 coding（编写代码），您负责测试和执行**
- **不要自动编译，由您来启动编译**

## Build Commands

### Configuration

Configure NuttX for a specific board:
```bash
./tools/configure.sh <board>:<config>
```

List available configurations:
```bash
./tools/configure.sh -L
```

### Building (Makefile)

Build the project:
```bash
make
```

Build with specific parallelism:
```bash
make -j$(nproc)
```

Clean build artifacts:
```bash
make clean
```

Distclean (remove all generated files):
```bash
make distclean
```

### Building (CMake)

Configure with CMake:
```bash
cmake -S <nuttx-dir> -B <build-dir> -DBOARD_CONFIG=<board>:<config>
```

Build:
```bash
cmake --build <build-dir>
```

### Building for Simulation

Configure for simulator:
```bash
./tools/configure.sh sim:nsh
```

Build and run:
```bash
make
./nuttx
```

## Code Style and Linting

### C Code Style Check

Run the NuttX style checker:
```bash
./tools/checkpatch.sh -f <file.c>
```

Check Python files:
```bash
black --check <file.py>
flake8 --config .github/linters/setup.cfg <file.py>
isort --check <file.py>
```

Check Rust files:
```bash
rustfmt --edition 2021 --check <file.rs>
```

Check CMake files:
```bash
cmake-format --check <file.cmake>
```

### Pre-commit Hooks

Install pre-commit:
```bash
pip install pre-commit
pre-commit install
```

Run pre-commit manually:
```bash
./tools/pre-commit
```

### Style Configuration Files

- **C formatting**: `tools/uncrustify.cfg`
- **Python linting**: `.github/linters/setup.cfg`
- **Python formatting**: Uses `black` and `isort`

## Testing

### Running Tests

Run pytest tests:
```bash
cd tools/ci/testrun
pytest
```

Run with verbose output:
```bash
pytest -sv
```

Run specific test markers:
```bash
pytest -m sim       # Simulator tests
pytest -m qemu      # QEMU tests
pytest -m common    # Common tests
```

## Code Style Guidelines

### File Organization

**File Extensions**:
- `.h` for C header files
- `.c` for C source files

**File Headers**: Every file must start with:
- Relative path from top-level
- Optional one-line description
- Blank line
- Apache 2.0 license header (SPDX-License-Identifier)

**Header Guard Format**:
```c
#ifndef __PATH_TO_FILE_H
#define __PATH_TO_FILE_H
/* content */
#endif /* __PATH_TO_FILE_H */
```

### Order of Declarations (Source Files)

1. Included Files
2. Pre-processor Definitions
3. Private Types
4. Private Function Prototypes
5. Private Data
6. Public Data
7. Private Functions
8. Public Functions

### Order of Declarations (Header Files)

1. Included Files
2. Pre-processor Definitions
3. Public Types
4. Public Data Declarations
5. Inline Functions
6. Public Function Prototypes

### Line Width and Formatting

- **Max line width**: 78 characters
- **Indentation**: 2 spaces (no tabs)
- **Line endings**: Unix-style (LF only)
- **No trailing whitespace**
- **End file with single newline**

### Block Comments

Use block comments for grouping:
```c
/****************************************************************************
 * Included Files
 ****************************************************************************/
```

### Naming Conventions

- **Structs**: `<name>_s` suffix
- **Functions**: Descriptive lowercase with underscores
- **Variables**: Lowercase with underscores
- **Constants**: Uppercase with underscores
- **Macros**: Uppercase with underscores

### Pointer Declarations

Place asterisks close to type, not variable:
```c
FAR struct mm_heap_s *heap;  /* Correct */
struct mm_heap_s *heap;      /* Avoid */
```

### Control Structures

Always use braces for blocks:
```c
if (condition)
  {
    /* code */
  }
```

### Error Handling

- Use `ASSERT()` and `DEBUGASSERT()` for critical checks
- Check return values of functions
- Handle error conditions explicitly
- Use `goto` pattern for cleanup in complex functions

### Documentation

- Use block comments for function documentation
- Follow naming in comments
- Use standard English grammar
- No Doxygen-style comments

### Columnar Alignment

Align similar items on same column when possible:
```c
dog      = cat;
monkey   = oxen;
aardvark = macaque;
```

### Function Wrapping

When wrapping long function calls:
```c
ret = some_function_with_many_parameters(long_parameter_1,
                                          long_parameter_2,
                                          long_parameter_3,
                                          long_parameter_4);
```

## Architecture-Specific Guidelines

### Kernel vs Userspace

- Kernel code: Use `__KERNEL__` checks
- Protected build: Separate kernel and user code
- Flat build: Single address space

### Memory Management

- Use `FAR` qualifier for pointers in kernel space
- Check for NULL returns from allocation functions
- Use KASAN when available for memory debugging

### Interrupt Handling

- Keep interrupt handlers as short as possible
- Use `up_irq_save()`/`up_irq_restore()` for critical sections
- Never call blocking functions from interrupt context

---

## Memory Debugging

### Stack Overflow Detection

NuttX provides multiple mechanisms to detect stack overflow issues:

| 方案 | 配置 | 说明 |
|------|------|------|
| **Stack Coloring** | `CONFIG_STACK_COLORATION=y` | 栈填充检测，定期检查 |
| **Stack Usage** | `CONFIG_STACK_USAGE=y` | 统计任务栈使用率 |
| **Stack Canaries** | `CONFIG_STACK_CANARIES=y` | 函数返回时检查金丝雀值 |
| **MPU 硬件检测** | `CONFIG_ARMV7M_STACKCHECK_HARDWARE=y` | 硬件边界保护，立即触发 |
| **KASAN** | `CONFIG_MM_KASAN=y` | Shadow Memory 检测所有越界 |
| **FORTIFY_SOURCE** | `CONFIG_FORTIFY_SOURCE=level` | 编译插入 memcpy/strcpy 边界检查 |

### Recommended Combinations

| 场景 | 方案组合 |
|------|----------|
| **日常开发** | Stack Usage + Stack Coloring |
| **ESP32-S3 生产** | MPU 硬件检测 |
| **复杂问题调试** | KASAN |
| **代码安全加固** | FORTIFY_SOURCE |

### Common Debug Commands

```bash
# 查看任务栈使用
nsh> ps -s
nsh> ps

# 解码 backtrace
./tools/btdecode.sh esp32s3 backtrace.txt

# 查看符号地址
xtensa-esp32s3-elf-addr2line -e nuttx 0xaddress
```

### Exception Codes

| 异常码 | Xtensa 名称 | 说明 |
|--------|--------------|------|
| 0x1c | StoreProhibited | 向受保护/无效内存地址写入 |
| 0x1b | LoadProhibited | 从受保护/无效内存地址读取 |

### Toolchain Version Check

```bash
# GCC 版本（13.2.0 支持 __builtin_dynamic_object_size）
xtensa-esp32s3-elf-gcc --version
```

---

## ESP32-S3 中断系统

### 核心概念

ESP32-S3 中断系统涉及以下关键组件：

| 组件 | 说明 |
|------|------|
| **外设中断 (Peripheral IRQ)** | 外设触发的中断请求 |
| **CPU 中断 (CPUINT)** | CPU 核心接收的中断信号 |
| **中断矩阵 (Interrupt Matrix)** | 将外设中断路由到 CPU 中断 |

### 关键数据结构

```c
// g_irqmap[IRQ] - IRQ → (CPU, cpuint) 映射
// 格式: CIIIIIII (C=CPU号, I=CPUINT号)
#define IRQ_MKMAP(c, i) (((c) << 0x07) | (i))
#define IRQ_GETCPU(m)    (((m) & 0x80) >> 0x07)
#define IRQ_GETCPUINT(m) ((m) & 0x7f)

// g_cpu0_intmap[cpuint] - cpuint → IRQ 映射
// 格式: EPPPPPPP (E=使能位, P=外设ID)
#define CPUINT_ASSIGN(c) (((c) & 0x7f) | 0x80)
```

### 关键文件

| 文件 | 说明 |
|------|------|
| `arch/xtensa/src/boss1-esp32s3/esp32s3_irq.c` | ESP32-S3 中断初始化和管理 |
| `arch/xtensa/src/boss1-esp32s3/esp32s3_gpio.c` | GPIO 中断处理 |
| `sched/irq/irq_dispatch.c` | 中断分发核心逻辑 |

### ESP32-S3 中断号定义

```c
// arch/xtensa/include/boss1-esp32s3/irq.h

#define BOSS1_ESP32S3_IRQ_GPIO_INT_CPU  (XTENSA_IRQ_FIRSTPERIPH + PERIPH_GPIO_INT_CPU)
#define BOSS1_ESP32S3_FIRST_GPIOIRQ    (XTENSA_NIRQ_INTERNAL + BOSS1_ESP32S3_NIRQ_PERIPH)
#define BOSS1_ESP32S3_PIN2IRQ(p)        ((p) + BOSS1_ESP32S3_FIRST_GPIOIRQ)
```

### esp32s3_gpioirqinitialize() 工作流程

```c
void esp32s3_gpioirqinitialize(void)
{
  int cpu = this_cpu();

  // 1. 分配 CPU 中断给 GPIO 外设
  g_gpio_cpuint = esp32s3_setup_irq(cpu,
                                     BOSS1_ESP32S3_PERIPH_GPIO_INT_CPU,
                                     1,  // 优先级
                                     BOSS1_ESP32S3_CPUINT_LEVEL);

  // 2. 注册 GPIO 中断处理函数
  irq_attach(BOSS1_ESP32S3_IRQ_GPIO_INT_CPU, gpio_interrupt, NULL);

  // 3. 启用 GPIO CPU 中断
  up_enable_irq(BOSS1_ESP32S3_IRQ_GPIO_INT_CPU);
}
```

### 中断完整工作流程图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        系统启动阶段                                      │
└─────────────────────────────────────────────────────────────────────────┘

系统启动 (up_irqinitialize)
        │
        ▼
esp32s3_gpioirqinitialize()
        │
        ├─[1] esp32s3_setup_irq()
        │     │
        │     ├─ 分配 CPU 中断号 (如 cpuint=16)
        │     │
        │     ├─ g_irqmap[IRQ52] = IRQ_MKMAP(0, 16)
        │     │        ↑
        │     │   IRQ52 = XTENSA_IRQ_FIRSTPERIPH + PERIPH_GPIO_INT_CPU
        │     │
        │     └─ 返回分配的 cpuint
        │
        ├─[2] irq_attach()
        │     │
        │     └─ g_irqvector[IRQ52].handler = gpio_interrupt
        │
        └─[3] up_enable_irq()
              │
              └─ 启用 CPU 中断 cpuint

┌─────────────────────────────────────────────────────────────────────────┐
│                        中断触发阶段                                      │
└─────────────────────────────────────────────────────────────────────────┘

外部 GPIO 中断触发 (如 GPIO10 下降沿)
        │
        ▼
    ESP32-S3 GPIO 硬件检测边沿
        │
        ▼
    设置 GPIO_STATUS_REG[10] = 1
        │
        ▼
    触发 CPU 中断 IRQ52 (GPIO_INT_CPU)
        │
        ▼
    xtensa_int_decode()
        │
        ├─ 读取挂起的 CPU 中断
        │     cpuint = g_gpio_cpuint (如 16)
        │
        └─ 从 g_cpu0_intmap[16] 获取 IRQ 号
                  │
                  ▼
            IRQ = 52 (GPIO_INT_CPU)
                  │
                  ▼
            xtensa_irq_dispatch(52, regs)
                  │
                  ▼
            gpio_interrupt(52, context, NULL)
                  │
                  ├─ 读取 GPIO_STATUS_REG
                  │     status = getreg32(GPIO_STATUS_REG)
                  │
                  ├─ 清除中断标志
                  │     putreg32(status, GPIO_STATUS_W1TC_REG)
                  │
                  └─ gpio_dispatch(BOSS1_ESP32S3_FIRST_GPIOIRQ=52, status)
                             │
                             └─ 遍历每个触发位
                                   │
                                   ├─ GPIO10 触发 → status & (1<<10)
                                   │        │
                                   │        ▼
                                   │   irq_dispatch(62, context)  ← 虚拟 IRQ 62
                                   │        │
                                   │        └─ g_irqvector[62].handler  ← xl9555_irq_handler
                                   │
                                   └─ 其他触发的 GPIO...
```

### 中断触发流程总结

| 步骤 | 描述 | 关键函数/寄存器 |
|------|------|-----------------|
| 1 | 外设触发中断 | GPIO 硬件 |
| 2 | 设置中断状态位 | `GPIO_STATUS_REG` |
| 3 | 触发 CPU 中断 | 中断矩阵 |
| 4 | CPU 响应中断 | `xtensa_int_decode()` |
| 5 | 分发到外设处理 | `gpio_interrupt()` |
| 6 | 读取状态并清除 | `GPIO_STATUS_REG`, `GPIO_STATUS_W1TC_REG` |
| 7 | 分发到具体引脚 | `gpio_dispatch()` |
| 8 | 调用用户 ISR | `irq_dispatch()` → xl9555_irq_handler |

### GPIO 中断相关 API

| 函数 | 说明 |
|------|------|
| `esp32s3_gpioirqinitialize()` | 初始化 GPIO 中断子系统 |
| `esp32s3_configgpio(pin, attr)` | 配置 GPIO 引脚 |
| `esp32s3_gpioirqenable(irq, intrtype)` | 使能指定 GPIO 中断 |
| `esp32s3_gpioirqdisable(irq)` | 禁用指定 GPIO 中断 |

### GPIO 中断触发类型

```c
#define DISABLED    0x00  // 禁用
#define RISING      0x01  // 上升沿
#define FALLING     0x02  // 下降沿  ← XL9555 使用
#define CHANGE      0x03  // 电平变化
#define ONLOW       0x04  // 低电平
#define ONHIGH      0x05  // 高电平
```

### XL9555 中断配置示例

```c
// 1. 配置中断 GPIO
esp32s3_configgpio(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_PIN, INPUT_PULLUP);

// 2. 获取 IRQ 号
int irq = BOSS1_ESP32S3_PIN2IRQ(CONFIG_BOSS1_ESP32S3_XL9555_IRQ_PIN);

// 3. 设置触发类型
esp32s3_gpioirqenable(irq, FALLING);

// 4. 注册中断处理函数
irq_attach(irq, xl9555_irq_handler, xl);
```

### 常见问题

1. **中断不触发**
   - 检查 `esp32s3_gpioirqinitialize()` 是否调用
   - 检查 `CONFIG_BOSS1_ESP32S3_GPIO_IRQ` 是否使能
   - 检查中断触发类型是否正确配置

2. **中断重复触发**
   - 确保正确清除中断标志位
   - 检查 `GPIO_STATUS_W1TC_REG` 是否正确写入

3. **中断延迟过高**
   - 考虑使用 HPWORK 而非 LPWORK
   - 减少中断处理函数中的耗时操作

---

## Project Specific Directories

### BOSS1 Project Directory Structure

**Board Directory**:
```
boards/xtensa/boss1-esp32s3/
├── boss1/                  # BOSS1 board configuration
│   ├── Kconfig             # Board configuration options
│   ├── configs/            # Configuration files
│   ├── include/            # Header files
│   ├── scripts/            # Script files
│   └── src/                # Board driver sources
└── common/                 # Common board code
```

**Platform Directory**:
```
arch/xtensa/src/boss1-esp32s3/  # BOSS1 ESP32-S3 platform code
```

### BOSS1 Project Configuration Prefix

- **Board Configuration Prefix**: `BOSS1_ESP32S3_`
- **Board Name**: `boss1-esp32s3`
- **Current Configuration**: `ESP32S3_BOSS1_V1_0_0`
- **Configure Command**: `./tools/configure.sh boss1-esp32s3:<config>`

### Common Configuration Options

| Configuration | Description |
|---------------|--------------|
| `BOSS1_ESP32S3_I2C` | Enable I2C support |
| `BOSS1_ESP32S3_LEDC` | Enable LEDC/PWM support |
| `BOSS1_ESP32S3_WIFI` | Enable WiFi support |
| `BOSS1_ESP32S3_BLE` | Enable Bluetooth support |

### Adding New Board Drivers

1. Create driver file in `boards/xtensa/boss1-esp32s3/boss1/src/`
2. Add configuration options in `boards/xtensa/boss1-esp32s3/boss1/Kconfig`
3. Add initialization call in `boards/xtensa/boss1-esp32s3/boss1/src/esp32s3_bringup.c`
4. Add source file in `boards/xtensa/boss1-esp32s3/boss1/src/Make.defs`

## Commit Message Format

Use prefix in first line:
```
<subsystem>: <description>

<detailed description if needed>
```

Examples:
```
mm: Add memory corruption detection
sched: Fix priority inversion in worker thread
drivers: Add UART DMA support for STM32
```

## Additional Resources

- Coding Standard: `Documentation/contributing/coding_style.rst`
- Contributing Guide: `CONTRIBUTING.md`
- Build System: See `CMakeLists.txt` and `Makefile`
- Tools: `tools/` directory

---

## Watch-UI Application Guidelines

**Application Path**: `vendor/boss/app/boss1-app/watch-ui/`

### Directory Structure

```
watch-ui/
├── apps/                    # Application code
│   ├── main/               # Main application
│   │   ├── watchui.cpp    # WatchUI main class
│   │   ├── work/          # Worker implementations
│   │   └── include/       # Public headers
│   ├── login/             # Login app
│   │   ├── login_app.cpp  # Login ability
│   │   ├── page/          # UI pages
│   │   ├── work/          # Background workers
│   │   └── mode_map/      # Mode mappings
│   ├── face/              # Face recognition app
│   └── base/              # Base framework (Mooncake)
│       ├── src/           # Framework implementation
│       │   ├── ability/   # Ability base classes
│       │   └── ability_manager/
│       ├── example/       # Example applications
│       └── test/          # Unit tests
├── entry/                  # Application entry point
├── apps/base/.clang-format # C++ code style
└── apps/base/CMakeLists.txt
```

### C++ Code Style

**File Headers** (Doxygen-style):
```cpp
/**
 * @file filename.cpp
 * @brief Brief description
 * @author AuthorName
 * @version 1.0
 * @date 2024-01-01
 * @copyright Copyright (c) 2024
 */
```

**Block Comments** (NuttX style):
```c
/****************************************************************************
 * Included Files
 ****************************************************************************/
```

**Code Formatting**:
- **Column limit**: 120 characters
- **Indentation**: 4 spaces
- **Pointer alignment**: Left
- **Brace style**: Custom (function braces on new line)
- **Standard**: C++11

**C++ Class Structure**:
```cpp
class ClassName {
public:
    ClassName();
    ~ClassName();

    void publicMethod();
    int publicProperty;

private:
    void privateMethod();
    int _privateMember;
};
```

**Framework Patterns**:
- **Ability Pattern**: Inherit from `AppAbility`, `UiAbility`, or `WorkerAbility`
- **Lifecycle Methods**: `onCreate()`, `onOpen()`, `onRunning()`, `onClose()`
- **Singleton Pattern**: Use `GetInstance()` and `DestroyInstance()`
- **Mooncake Framework**: Application lifecycle management system

**Logging**:
```cpp
appinfo("message");      // Info level
appwarn("message");      // Warning level
apperr("message");       // Error level
LV_LOG_USER("message");  // LVGL logging
```

### Build System

**Makefile**:
```bash
# In nuttx directory
make -j$(nproc)
```

**C++ Linting**:
```bash
# Check clang-format
clang-format -style=file -i <file.cpp>

# Or use the local config
./node_modules/.bin/prettier --write <file.cpp>
```

### Key Framework Components

1. **Mooncake**: Application ability management framework
   - Install/uninstall apps: `mooncake.installApp()`, `mooncake.uninstallApp()`
   - Open/close apps: `mooncake.openApp()`, `mooncake.closeApp()`
   - Extension management: `mooncake.createExtension()`

2. **Ability Types**:
   - `AbilityType_App`: Main application
   - `AbilityType_UI`: UI-based ability
   - `AbilityType_Worker`: Background worker

3. **Event Loop**:
   - libuv-based: `uv_loop_t`, `uv_async_t`, `uv_timer_t`
   - Simple polling: `while(1) { update(); sleep(1); }`
