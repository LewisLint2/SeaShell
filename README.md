# SeaShell

**SeaShell** is a **minimal, ultra-lightweight Unix shell** written entirely in C. Designed for **speed, portability, and learning**, it provides only the **essential shell functionality** without any bloat or luxury features.

The current “micro” version is ~9.4 KB, making it **smaller than a children’s book** while still being **fully functional** for basic command execution and pipelines.

---

## Purpose

SeaShell was created with two main goals:

1. **Learning:** Understand how shells work at the system level by building a shell from scratch.
2. **Minimalism & Speed:** Provide a shell that is **tiny, portable, and fast**, removing non-essential features.

> **Note:** The micro variant **does not include** history storage, advanced job control, or tab completion.

---

## Target Audience

* Developers and hobbyists who want a **fast, tiny shell**.
* Users needing **portable Unix tools** for minimal installations.
* Arch Linux users building **tiny environments or ISO setups**.

---

## Design Philosophy

* **Extreme Minimalism:** Only core shell features are included.
* **No “luxury” features:** Advanced history, scripting, or tab completion are stripped for size.
* **Portability:** Written in pure C with minimal system calls.
* **Speed & Efficiency:** Quick startup and execution, perfect for constrained environments.
* **Readable Code:** Despite being tiny, the codebase remains understandable for educational purposes.

---

## Features (Micro Variant)

* **Command Execution:** Run programs and built-ins.
* **Pipelines:** Chain commands using `|`.
* **Input/Output Redirection:** Supports `<`, `>`, `>>`.
* **Background Execution:** Commands can run in the background with `&`.
* **Prompt:** Displays `user@host:cwd$` (or `#` for root).

> **Excluded in micro version:**
>
> * Command history (`!!`)
> * Tab completion
> * Environment variable expansion (`$VAR`)
> * Wildcard expansion (`*`, `?`)
> * Multi-line editing

---

## Requirements

* Unix-like system (Linux, BSD; minimal POSIX environment)
* GCC or another C compiler

> **Note:** No external libraries are needed for the micro variant.

---

## Compilation

For the **micro variant**:

```bash
gcc -nostdlib -static seashell.c -o seashell
```

> This builds a **standalone binary ~9.4 KB**. No `linenoise` or `readline` required.

---

## Install Instructions

1. **Compile the micro variant:**

```bash
gcc -nostdlib -static seashell.c -o seashell
```

2. **Move to a system-wide path (optional):**

```bash
sudo mv seashell /usr/local/bin/
```

3. **Run it:**

```bash
seashell
```

> Now you have the tiny shell available system-wide.

---

## Usage

```bash
./seashell
```

### Examples

```bash
ls -la                 # List all files
cat file.txt | grep x   # Use a pipeline
cd /path/to/dir        # Change directory
command &               # Run in background
```

> Features like history, tab completion, and variable expansion are **not included** in this micro variant.

---

## Notes

* SeaShell is **ultra-minimal** and suitable for **tiny environments or rescue systems**.
* The micro binary is **easily portable** across Linux systems.
* Ideal for **learning** how shells execute commands and handle pipelines.

---

## Comparison (Optional)

| Feature            | Full Version | Micro Variant |
| ------------------ | ------------ | ------------- |
| Command History    | Yes          | No            |
| Tab Completion     | Yes          | No            |
| Variable Expansion | Yes          | No            |
| Multi-line Editing | Yes          | No            |
| Binary Size        | ~50 KB       | ~9.4 KB       |

---

## License

Open source. Modify, distribute, or use freely.
