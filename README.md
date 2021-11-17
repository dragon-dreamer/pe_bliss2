# PE Bliss 2

### The most advanced and thorough library to read and write Portable executable files

**This is a work-in-progress active development project! Everything may change, and the library may contain critical bugs, do not use in production!**

The [Portable executable](https://en.wikipedia.org/wiki/Portable_Executable) is a popular executable file format, which is extensively used in Windows family operating systems. It is as well utilized by the X-Box, EFI and some other environments. The **PE Bliss 2** library is a successor of the old and unsupported [PE Bliss](https://code.google.com/archive/p/portable-executable-library/) library.

The library is aimed at being able to provide as much information about the PE file as possible, while being robust and keeping track of possible parsing or format errors and irregularities. Currently, the library is compiled using the MSVC compiler, but the code itself is cross-platform. The current features of the library are:
- Reading PE/PE+ binaries.
  - Reading/modifying binaries either from a file, or from a memory buffer.
  - Reading/modifying binaries, which are already loaded into memory by the Windows loader.
  - Supports copying the full binary sections memory or referencing the existing memory buffer (read-only mode).
- Rebuilding PE/PE+ binaries to a file or a memory buffer.
- DOS, File, Optional header parsing.
- Data directories parsing.
- DOS Stub, Rich header parsing.
  - Rich COMPID database.
- Section table parsing.
- Directory support:
  - Bound import (read/modify)
  - Exception (read-only)
    - x64
    - ARM
    - ARM64
  - Exports (read/modify)
  - Imports, delay imports (read/modify)
  - Load configuration (including recent versions up to Windows 10 21H1, read-only)
  - Relocations (read/modify)
    - Image rebase
  - TLS (read/modify)
- Address conversions (RVA/VA/file offsets).
- Overlay at the end of file parsing.
- Simple console dumper of PE/PE+ files (no extended configuration yet).

The following features are planned:
- Cross-platform compilation with CMake.
- Unit-tests/integration tests.
- Documentation.
- Examples.
- Porting of missing old functionality from the PE Bliss library.
- Directory support:
  - Resources
    - Dialogs
    - Icons, Cursors
    - Manifests
    - Version info 
    - Bitmaps
    - Message lists
    - String tables
  - Debug
  - Digital signature (Security)
- Image signature verification/calculation.
- Import redirects (apisetschema.dll).
- Trustlets.
- Entropy calculation.
- .NET support.

Current library requirements are:
- C++20 or newer.
- [Boost](https://www.boost.org/) libraries (header-only): **endian**, **pfr**.
