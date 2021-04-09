# Avir

A very simple antivirus for Linux systems.

It works by comparing MD5 hashes of your files with malware hashes provided by [Team Cymru](https://team-cymru.com/mhr).

### Usage

```
Usage: avir [type] [option]
 Types:
  -f  <path>:   scan a single file
  -dl <path>:   scan a directory linearly
  -dr <path>:   scan a directory recursively
 Options:
  -o  <file>:    save output to file
```

### Installation

Run `path_to_Avir/builder.sh`.