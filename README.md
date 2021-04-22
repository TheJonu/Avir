# Avir

A simple antivirus for Linux systems. 
Malware detection is based on comparison of MD5 file hashes.

### Usage

```
Usage: avir [action] [options]
 Scan actions
   -f <path>     scan a single file
   -l <path>     scan a directory linearly
   -r <path>     scan a directory recursively
 Other actions
   --show        show last scan report
   --stop        stop all ongoing scans
 Scan options
   -b <path>     specify an additional hash list file
   -o <path>     specify an additional output/report file
   --online      check hashes online instead of locally
   --unreadable  list unreadable files in report
```

### Installation

Run `./setup.sh`

### More info

- Avir's base directory is `~/Avir` (`home/{user}/Avir`).
- The default file containing malware hashes is located at `~/Avir/hashbase.txt`.
- You can also specify hash base files at other locations.
- Those files should contain one MD5 hash per line.
- All scan reports are saved to `~/Avir/results` with the signature `avir_{timestamp}.txt`.
- You can also specify additional locations at which to save the result file.
- The `--online` option makes use of
  [Team Cymru's Malware Hash Registry](https://team-cymru.com/community-services/mhr/).
  It works asynchronously, requires *whois* installed and is suggested only for small amounts of files.