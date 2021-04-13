# Avir

A very simple antivirus for Linux systems.

### Usage

```
Usage: avir [action] [options]
 Scan actions
   -sf <path>    scan a single file
   -sl <path>    scan a directory linearly
   -sr <path>    scan a directory recursively
 Other actions
   --show        show last scan result
   --stop        stop all ongoing scans
 Options
   -b <path>     specify additional hash base file
   -o <path>     specify additional output file
   --online      check file hashes online
```

### Installation

`./run.sh`