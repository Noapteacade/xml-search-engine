# LocalSearchEngine

A lightweight local search engine based on **TF‑IDF**, usable for any **local XML / XHTML file collection**. The test set currently only includes OpenGL documentation.

- Pure C++17 implementation  
- Depends on vcpkg + CMake  
- Supports **index generation** + **TF‑IDF ranked search**  
- CLI‑friendly, with `--top N` to control result count

---

## Preview

```bash
# Generate index
.\LocalSearchEngine.exe index ./docs.gl

# Search (default top 10)
.\LocalSearchEngine.exe search "buffer" ./index-files/index_gl4.json --top 10
```

Example output (searching `"buffer"`):

```text
Best match: glBindBuffer.xhtml (score: 0.056)
Other matches:
  glVertexArrayElementBuffer.xhtml (score: 0.054)
  glIsBuffer.xhtml (score: 0.052)
  ...
```

---

## Quick Start

### Requirements

- Windows 10 / 11  
- Visual Studio 2022 (with **Desktop development with C++** workload)  
- [vcpkg](https://github.com/microsoft/vcpkg) (**recommended to be integrated automatically with CMake**)  
- CMake 3.10+

### Install Dependencies & Build

```bash
git clone https://github.com/Noapteacade/xml-search-engine.git
cd xml-search-engine

# 1. Install third-party libraries
vcpkg install pugixml:x64-windows nlohmann-json:x64-windows

# 2. Configure & build
cmake -B build -DCMAKE_TOOLCHAIN_FILE="<vcpkg>/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

> `<vcpkg>` is the actual path to your local vcpkg installation, e.g. `C:\vcpkg`.

---

## Usage

### 1️⃣  Index subcommand

```bash
LocalSearchEngine.exe index <path>
```

- Recursively walks `<path>`
- Targets `.xml` / `.xhtml` files
- Generates `index-files/index_<dirname>.json`

### 2️⃣ Search subcommand

```bash
LocalSearchEngine.exe search "<keyword>" <index-file> [--top N]
```

Arguments:

| Argument     | Description                                      |
|--------------|--------------------------------------------------|
| `keyword`    | Search term (multiple space‑separated words allowed) |
| `index-file` | Path to the JSON index file                      |
| `--top N`    | Show only top N results (default: all)          |

Example:

```bash
LocalSearchEngine.exe search "glActiveShaderProgram" ./index-files/index_gl4.json --top 5
```

---

## Project Structure

```text
LocalSearchEngine/
├── build/                 # Temporary build directory (not in Git)
├── index-files/           # Generated index files (not in Git)
├── commandline.hpp        # Custom command line argument parser
├── main.cpp               # Main program (Lexer + TF‑IDF + full pipeline)
├── CMakeLists.txt         # CMake build configuration
├── .github/workflows/ci.yml   # GitHub Actions CI
└── README.md
```

---

## Roadmap

- [ ] **Auto‑detect index files**: search across multiple indexes without user specifying path.
- [ ] Global term‑to‑index metadata to speed up multi‑index search.

---

## Known Limitations

- Index stores only TF vectors; IDF is computed at search time (suitable for small to medium document sets)  
- Currently tuned for OpenGL documentation, but can be extended to arbitrary XML document collections  

---

## License

MIT License

This project is for learning / personal use only.  
Dependencies `pugixml` and `nlohmann/json` are both MIT licensed.  
Data source: [docs.gl](https://github.com/BSVino/docs.gl)

**Made with ❤️ and a lot of C++17**