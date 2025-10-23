"""Custom PlatformIO target to execute the native simulator binary."""

from pathlib import Path
from SCons.Script import AlwaysBuild

Import("env")

PROGRAM_PATH = Path(env.subst("$BUILD_DIR")) / f"{env.subst('$PROGNAME')}{env.subst('$PROGSUFFIX')}"


def _run_program(target, source, env):
    return env.Execute(str(PROGRAM_PATH))


run_alias = env.Alias("run", env.File(str(PROGRAM_PATH)), _run_program)
AlwaysBuild(run_alias)
