# Avir

A simple antivirus for Linux systems. 
Malware detection is based on comparison of MD5 file hashes.

### Usage

```
Usage: sudo avir [action] [options]
 Scan actions
   -sf <path>    scan a single file
   -sl <path>    scan a directory linearly
   -sr <path>    scan a directory recursively
 Other actions
   --show        show last scan report
   --stop        stop all ongoing scans
 Scan options
   -h <path>     specify an additional hash list file
   -r <path>     specify an additional output/report file
   -o, --online  check hashes online instead of locally
   -u, --unread  list unreadable files in report
```

### Installation

Run `./setup.sh`

### More info

- Avir's base directory is `/avir`.
- The default file containing malware hashes is located at `/avir/hashlist.txt`.
- You can also specify hash list files at other locations.
- Those files should contain one MD5 hash per line.
- All scan reports are saved to `/avir/reports` with the signature `avir_{timestamp}.txt`.
- You can also specify additional locations at which to save the report file.
- The `--online` option makes use of
  [Team Cymru's Malware Hash Registry](https://team-cymru.com/community-services/mhr/)
  to determine if hashes are safe.
  It works asynchronously, requires `whois` and is suggested only for small numbers of files.
- Unsafe files are put into quarantine at `/avir/quarantine`.