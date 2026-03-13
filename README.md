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
gcc -nostdlib -static seashell.c -o SeaShell
```

> This builds a **standalone binary ~9.4 KB**. No `linenoise` or `readline` required.

---

## Install Instructions
1. **Download '*nolibc*'**
```bash
git clone https://git.kernel.org/pub/scm/linux/libs/nolibc/nolibc.git
git clone https://git.kernel.org/pub/scm/linux/libs/nolibc/nolibc.git
git clone https://github.com/torvalds/linux.git --depth=1
cd linux
git sparse-checkout init --cone
git sparse-checkout set tools/include/nolibc
```
2. **Compile and install the micro variant:**

```bash
curl -O https://raw.githubusercontent.com/LewisLint2/SeaShell/main/seashell.c
# there are no dependancies for the 9KB version, so you can just download the file dirrectly
# In this case, Git isn’t needed; curl works fine
gcc -nostdlib -static -I/path/to/linux/tools/include/nolibc -Os seashell.c -o SeaShell
# -I for path to the tools. you will find a linux folder in your current directory if you havent CD'd anywhere after downloading nolibc
# -Os strips the file down
# -nostdlib because we are using nolibc
# -static for a static binary
# -o for output file  (you can change the output file to a name of your choosing on your system)
```
3. **Compress *SeaShell* with *upx* using LZMA algorithm**

```bash
upx --best --lzma SeaShell
# --best for best effort for maximum compression
# --lzma for LZMA compression; very efficient compression
```

4. **Move to a system-wide path:**

```bash
sudo cp SeaShell /bin/SeaShell
# this puts 'SeaShell' in /bin, which is slightly more powerful than /user/bin because you
# can set the default shell to the entire system in /bin.
```

5. **Run it:**

```bash
SeaShell
```

6. **Optional: set *SeaShell* as the default shell for your user**

```bash
echo "/bin/SeaShell" | sudo tee -a /etc/shells
# this will put '/bin/SeaShell' in your '/etc/shells' file, so it can be used as a shell
sudo chsh -s /bin/SeaShell username 
# replace 'username' with the user you want to use SeaShell
# to set SeaShell to the default shell systemwide, look at the following optional step
```
7. **Optional: Set SeaShell as the default shell SystemWide**

```bash
# to set the default shell to SeaShell, you will have to do all the previous steps
sudo nano /etc/default/useradd # this is the default user config. any new users will use these settings
# find the 'SHELL' line, and change it to 'SHELL=/bin/SeaShell'
# then click Ctrl+O, Enter, Ctrl+x to save
```

> Now you have the tiny shell available system-wide.

---

## Usage

```bash
SeaShell
```

### Examples

```bash
ls -la                 # List all files
cat file.txt | grep x   # Use a pipeline
cd /path/to/dir        # Change directory
command &               # Run in background (I am still trying to figure this out, but SeaShell is officially stable to use.)
# like every shell really
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
## **To-Do:**
1. Add history logic similar to Bash, but only for one session (and use for *!!*)
2. Add variable statements
3. Add alias logic
4. Make a *MAKEPKG* file to turn this into an Arch Linux package (*Pacman*)
5. Add *SeaShell* to the AUR (*Arch User Repository*)
6. Make a *.deb* package for Debian/Ubuntu wretches
7. Fix any reported problems by any users (**UNLESS**: *it will bloat the shell un unnecessary proportions*)
8. Make SeaShell more secure
---

## **Things I might do; not guaranteed**
1. Add full scripting logic
2. Fix some errors with *if*, *for*, and *while* statements in *inline commands*
3. Any **suggestions** to my repo (*different than* bugs)

---

## **What I intend ***not*** to do**

1. Add persistant history
2. Bloat SeaShell by too much
3. Stray from the origional design
## License

Open source. Modify, distribute, or use freely.
