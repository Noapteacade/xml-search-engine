# LocalSearchEngine

一个基于 **TF‑IDF** 的轻量级本地搜索引擎，专为 **OpenGL 文档（docs.gl）** 设计，但也可用于任何**本地 XML / XHTML 文件集合**。

- ✅ 纯 C++17 实现  
- ✅ 依赖 vcpkg + CMake  
- ✅ 支持 **生成索引** + **按 TF‑IDF 排序搜索**  
- ✅ 命令行友好，提供 `--top N` 控制结果数量  
- ✅ 自带 GitHub Actions CI，可自动编译验证

---

## 效果预览

```bash
# 生成索引
.\LocalSearchEngine.exe index ./docs.gl

# 搜索（默认显示前 10）
.\LocalSearchEngine.exe search "buffer" ./index-files/index_gl4.json --top 10
```

输出示例（搜 `"buffer"`）：

```text
Best match: glBindBuffer.xhtml (score: 0.056)
Other matches:
  glVertexArrayElementBuffer.xhtml (score: 0.054)
  glIsBuffer.xhtml (score: 0.052)
  ...
```

---

## 快速开始

### 环境要求

- Windows 10 / 11  
- Visual Studio 2022（含 **C++ 桌面开发** 工作负载）  
- [vcpkg](https://github.com/microsoft/vcpkg)（**推荐由 CMake 自动集成**）  
- CMake 3.10+

### 安装依赖 & 编译

```bash
git clone https://github.com/Noapteacade/xml-search-engine.git
cd xml-search-engine

# 1. 安装第三方库
vcpkg install pugixml:x64-windows nlohmann-json:x64-windows

# 2. 配置 & 编译
cmake -B build -DCMAKE_TOOLCHAIN_FILE="<vcpkg>/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

> `<vcpkg>` 是你本机 vcpkg 的实际路径，例如 `C:\vcpkg`。

---

## 使用说明

### 1️⃣ 索引子命令

```bash
LocalSearchEngine.exe index <path>
```

- 递归遍历 `<path>`
- 以 `.xml` / `.xhtml` 为目标文件
- 生成 `index-files/index_<目录名>.json`

### 2️⃣ 搜索子命令

```bash
LocalSearchEngine.exe search "<keyword>" <index-file> [--top N]
```

参数说明：

| 参数        | 说明                         |
|------------|------------------------------|
| `keyword`  | 搜索词（支持多个空格分隔的词） |
| `index-file` | JSON 索引文件路径             |
| `--top N`  | 只显示前 N 条结果（默认全部）   |

示例：

```bash
LocalSearchEngine.exe search "glActiveShaderProgram" ./index-files/index_gl4.json --top 5
```

---

## 项目结构

```text
LocalSearchEngine/
├── build/                 # 临时编译目录（不进入 Git）
├── index-files/           # 生成的索引文件（不进入 Git）
├── commandline.hpp        # 自己的命令行参数解析器
├── main.cpp               # 主程序（Lexer + TF‑IDF + 全流程）
├── CMakeLists.txt         # CMake 构建配置
├── .github/workflows/ci.yml   # GitHub Actions CI
└── README.md
```

---

## 已知限制

- 仅支持 **ASCII 英文**，非 ASCII 字符会被 Lexer 直接跳过  
- 索引仅存储 TF 向量，IDF 在搜索阶段实时计算（适合中小规模文档集）  
- 目前仅针对 OpenGL 文档调优，但可扩展任意 XML 文档集合  

---

## 许可证

该项目仅供学习 / 个人使用。  
依赖库 `pugixml` 与 `nlohmann/json` 均为 MIT 许可证。  
数据来源：[docs.gl](https://github.com/BSVino/docs.gl)

---

## 下一步可扩展方向

- [ ] 支持 `AND` / `OR` 查询语法  
- [ ] 支持更多文档格式（JSON、Markdown、纯文本）  
- [ ] 合并同文档的多条分数记录  
- [ ] 缓存 `doc_freq` 以提高 IDF 速度  

---

**Made with ❤️ and a lot of C++17**
```
