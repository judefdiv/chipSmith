# chipSmith
Generate a complete GDS chip from LEF/DEF files.

Version: 0.1

## Features
* Import GDS cells
* Convert LEF/DEF to GDS
* Generate PTLs

## Getting Started

### Prerequisites

The following packages is required to successfully compile and execute chipForge.

``` bash
apt install build-essencials    # for compiling
```

### Installation

``` bash
# Current directory: chipForge root
mkdir build && cd build
cmake ..
make
```

### Usage

Examples of how to execute chipForge:

#### LEF/DEF to GDS

``` bash
./chipForge -g lefExample.lef defExample.def -o gdsOutput.gds
```

#### Run from config file

``` bash
./chipForge -c tomlExample.toml
```


## Notes

For IARPA contract SuperTools

LEF: Library Exchange Format

DEF:

GDS:

PTL: Passive Transmission Line