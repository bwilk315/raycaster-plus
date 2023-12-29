
"""
Usage:
    python3 build.by [--out <file>] [--debug] [--release] [--run]
Switches description:
    out     - set the output file path
    debug   - build the project with debugging tools included
    release - build the project only, without any additional tools
    run     - run the output executable just after building is finished
"""

from os  import system
from sys import argv

# Build settings, customizable through command switches
stg_output = "./a.out"
stg_run = False
stg_debug = False

# Interpret switches information
argc = len(argv)
index = 1
while index < argc:
    switch = argv[index]
    if switch.startswith("--") and len(switch) > 2:
        cmd = switch[2:]
        # No-value commands
        if cmd == "run":
            stg_run = True
        elif cmd == "debug":
            stg_debug = True
        elif cmd == "release":
            stg_debug = False
        elif(index < argc):
            # Commands that require a value being next argument
            index += 1
            if cmd == "out":
                stg_output = argv[index]
            else:
                print(f"Unknown switch '{switch}'")
                exit(1)
        else:
            print(f"Switch '{cmd}' requires a value")
            exit(1)
        index += 1
    else:
        print(f"Invalid switch format '{switch}' (maybe prepend it with two dashes?)")
        exit(1)

# Build the project according to the settings
build_cmd  = "g++ main.cpp source/* "
build_cmd += "-DDEBUG " if stg_debug else ""
build_cmd += "-lpng -lSDL2 -o "
build_cmd += stg_output
system(build_cmd)
if stg_run:
    system(f"./{stg_output}")
