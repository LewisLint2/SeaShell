# SeaShell

**SeaShell** is a **minimal, ultra-lightweight Unix shell** written entirely in C. Designed for **speed, portability, and learning**, it provides only the **essential shell functionality** without any bloat or luxury features.

It is currently ~50KB, making it **tiny enough to be portable** on almost any Unix system, while still being **fully functional** for developers and Unix users, particularly those on **Arch Linux**.

---

## Purpose

SeaShell was created with two main goals:

1. **Learning:** Understand how shells work at the system level by building a shell from scratch.
2. **Speed & Minimalism:** Provide a tiny, indestructible shell that only does what’s necessary — no extra features, no bloat.

---

## Target Audience

* Developers who want a **fast, minimal shell**.
* Unix users who need **portable tools** across different systems.
* Arch Linux users looking for a **tiny shell** to use during installations or minimal setups.

---

## Design Philosophy

* **Minimalism:** Only essential features are included.
* **No luxury:** No aliases, scripting, or auto-suggestions beyond basic tab completion.
* **Portability:** Written in 100% C with minimal dependencies.
* **Speed & Efficiency:** Quick startup and execution, suitable for tiny environments or ISO builds.
* **Indestructible:** Robust handling of errors and processes with a simple, readable codebase.

---

## Features

* **Command Execution:** Run programs, scripts, and built-ins.
* **Pipelines:** Chain commands with `|`.
* **Input/Output Redirection:** `<`, `>`, and `>>`.
* **Background Execution:** Run commands in the background with `&`.
* **Tab Completion:** Auto-complete files and commands in `$PATH`.
* **Environment Variable Expansion:** Supports `$HOME` and other variables.
* **Wildcard Expansion:** `*` and `?` for files and directories.
* **Multi-line Editing:** Support for long commands spanning multiple lines.
* **Command History:** Stored in `.seashellhist` for recall and repeat (`!!`).
* **Prompt Customization:** Shows `user@host:cwd$` or `#` for root.

---

## Requirements

* Unix-like system (Linux, macOS, BSD)
* GCC or another C compiler
* [`linenoise`](https://github.com/antirez/linenoise) library for line editing and history

---

## Compilation

```bash
gcc -o SeaShell seashell.c -lreadline
```

Make sure `linenoise.h` and `linenoise.c` are included in your project directory.

---

## Usage

```bash
./SeaShell
```

### Examples

```bash
ls -la                 # List all files
cat file.txt | grep foo # Pipeline
echo $HOME             # Environment variable expansion
cd /path/to/dir        # Change directory
command &               # Run in background
!!                      # Repeat last command
```

---

## Notes

* SeaShell is **purposefully minimal** — it excludes scripting, advanced job control, or aliases.
* Designed for use during **Arch Linux installations** or **tiny Unix environments**.
* Focused on being **portable, fast, and tiny**, while leaving the source code open for anyone to use.

---

## License

Open source. Modify, distribute, or use freely.