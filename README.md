# Avir

A very simple antivirus for Linux systems.

It works by comparing MD5 hashes of your files with malware hashes provided by [Team Cymru](team-cymru.com/mhr).

#### Usage

```
Usage: avir [type] [path]
    -f <filepath>: scan a single file
    -dl <dirpath>: scan a directory linearly
    -dr <dirpath>: scan a directory recursively
```

#### Installation

1. Navigate to Avir/build
2. `sudo make install`