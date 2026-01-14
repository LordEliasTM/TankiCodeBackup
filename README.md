# TankiCodeBackup

A multithreaded cross-platform C++ tool to back up the latest `main.js` files and source maps from Tanki Online main and test servers.
It automates the process of checking each available server for updates and downloads relevant files accordingly.

## Features

- Scans all available Tanki Online servers (prod and test).
- Parses HTML to extract `main.js` url.
- Downloads JavaScript files and their `.map` source maps (if available).
- Limits parallel downloads (default: 2).
- Uses barriers to synchronize download phases across threads.
- Skips files that already exist on disk

## Requirements

- C++20 compatible compiler
- CMake
- llvm (only macOS)
- libidn2 (only macOS)
- Optional: UPX

## Output Structure

Each server's files are stored in their own directory:
```
./0/main.XXXXXXXX.js
./1/main.YYYYYYYY.js
...
```

- `0` refers to prod server (`tankionline.com/play`)
- `1-10` are the test servers (`public-deployX.test-eu.tankionline.com`)
