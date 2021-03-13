# Avir

A very simple antivirus for Linux systems.

It works by comparing MD5 hashes of your files with malware hashes provided by [Team Cymru](https://team-cymru.com/mhr).

#### Usage

```
avir -f <filepath>: scan a single file
avir -dl <dirpath>: scan a directory linearly
avir -dr <dirpath>: scan a directory recursively
```

#### Installation

`sudo path_to_Avir/installer.sh`