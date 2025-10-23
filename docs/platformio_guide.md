# PlatformIO Usage Guide

This project now includes a [PlatformIO](https://platformio.org/) configuration so you can
build and run the host-based Simon game simulator with the same workflow that you use for
embedded targets.

## Prerequisites

1. Install PlatformIO Core (CLI) by following the official instructions:
   ```bash
   pip install -U platformio
   ```
   Alternatively, install the PlatformIO IDE extension in VS Code. The extension bundles the
   CLI, so you do not need to install it separately.
2. Verify the installation:
   ```bash
   platformio --version
   ```

## Project Layout

PlatformIO uses the existing source tree directly:

- Application sources live in `src/` (for example `src/main.c`).
- Public headers are provided via the `include/` directory.
- The PlatformIO project configuration is stored in `platformio.ini` at the repository root.

## Common Tasks

### Configure the Environment

This project defines a single environment named `native`, which builds a host executable using
your system toolchain. To make it the default, `default_envs = native` is set in `platformio.ini`.

### Build the Simulator

Run the following command from the repository root to compile the project:

```bash
platformio run
```

The compiled binary is placed under `.pio/build/native/program`. PlatformIO will automatically
rebuild the project when sources change.

### Execute the Simulator

Use PlatformIO's `run` target to execute the freshly built binary:

```bash
platformio run -t run
```

The program behaves identically to invoking the executable manually. You can also run it directly
from the build directory if you prefer:

```bash
.pio/build/native/program
```

### Clean the Build Artifacts

To remove all cached build outputs, run:

```bash
platformio run -t clean
```

### Select the Environment Explicitly

If you add more environments later (for example, to target the ATtiny1626 hardware), you can
specify which one to use with the `-e` flag:

```bash
platformio run -e native
```

## Troubleshooting

- If PlatformIO cannot locate your system compiler, ensure that the necessary build-essential
  packages are installed (for example, `sudo apt install build-essential` on Debian/Ubuntu).
- When switching between the legacy Makefile build and PlatformIO, clean the workspace in between
  to avoid stale artifacts: `make clean` or `platformio run -t clean`.

For further details on PlatformIO commands, consult the
[official documentation](https://docs.platformio.org/en/latest/core/userguide/).
