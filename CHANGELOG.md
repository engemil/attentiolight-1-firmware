# Changelog

All notable changes to the EngEmil STM32 Bootloader project will be documented in this file.

**Version Format:** MAJOR.MINOR.PATCH
- **MAJOR:** Incompatible API/protocol changes
- **MINOR:** New features (backward compatible)
- **PATCH:** Bug fixes (backward compatible)

[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Development] (2026-02-11)

Added

- **License note** added and adjusted on most source files.
- **Button Driver** for reading the button presses and "decode" different type of presses, as well as callback functionality.

---

## [1.0.0] - (2026-02-11)

Changed
- Updated **.devcontainer files** with better container structure.
- Removed **ChibiOS** command from .vscode files. 

Added
- **Board Files** for ST_NUCLEO64_C071RB added (from ChibiOS) in boards-folder.
- **ChibiOS** as submodule
- **Integrated bootloader requirements** to be able to upload application firmware over USB.
- **Bootloader** added as submodule.
- Application Firmware first iteration as a prototype. With test code, LED light driver, USB driver, and VCP.
- Application built on ChibiOS.

---


