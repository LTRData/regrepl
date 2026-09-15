# Registry Replace Tool (regrepl)

A Windows GUI utility for recursively replacing text in registry value data under a selected root key or subkey, locally or on a remote computer. It provides a “replace all” operation for registry contents.

This is a historical native C++ application. The original documentation dates from 1998–2007 and describes Windows 3.1x with Win32s, Windows 95 and Windows NT; those statements are preserved in [regrepl.txt](regrepl.txt). They are not a current Windows compatibility matrix. The repository contains the source, dialog resources and an NMAKE build file.

## Usage

Start `regrepl.exe` without arguments. It does not accept command-line parameters for performing replacements; supplying an argument such as `/?` displays an information dialog.

| Field | Meaning |
| --- | --- |
| **Text to replace** | Nonempty text to find. Matching in string values is literal and case-sensitive. |
| **Replace with text** | Replacement text. An empty replacement deletes the matching text. |
| **Remote computer name** | Leave blank for the local registry, or enter a computer name without leading backslashes. Remote access uses Windows registry APIs and the caller's existing access rights. |
| **Root key** | Select `HKEY_CLASSES_ROOT`, `HKEY_CURRENT_USER`, `HKEY_LOCAL_MACHINE` or `HKEY_USERS`. The normal default is `HKEY_LOCAL_MACHINE`. |
| **Subkey** | Path relative to the selected root, such as `Software\RegreplExample`. Leave blank to process the entire selected root. |

Click **Start** to process values in the selected key and recursively in its subkeys. The progress dialog shows replacement and error counts. The replacement count is the number of successfully rewritten values, not the number of matching occurrences.

**Changes are written immediately, with no preview, automatic backup or undo.** Export or otherwise back up the intended scope first. **Cancel** stops further processing but does not reverse changes already made. Access failures can leave only part of the selected tree updated.

For an initial trial, create a disposable `HKEY_CURRENT_USER\Software\RegreplExample` key containing a string value with `old-text`. Select **HKEY_CURRENT_USER**, enter `Software\RegreplExample` as the subkey, and replace `old-text` with `new-text`. Inspect that value afterward before using the tool on real settings.

## Replacement behavior and limits

- Processes data in `REG_SZ`, `REG_EXPAND_SZ`, `REG_MULTI_SZ` and `REG_BINARY` values, retaining each value's type. It does not rename keys or value names, or replace numeric registry values.
- Binary values are searched for embedded text too. The implementation first searches for the narrow-character form; if none is found, it tries a Windows wide-character form. There is no value-type filter, so a selected subtree's binary data is included.
- The UI and registry string operations use legacy ANSI APIs. This is not a general Unicode text-replacement tool.
- Search and replacement fields have 260-byte buffers, and value data uses fixed buffers of approximately 64 KiB. Large values and replacement results are subject to these limits.
- There is no explicit 32-bit/64-bit registry-view selector.
- Registry permissions still apply. The tool does not elevate itself or offer alternate credentials for remote access.

## Building

The [Makefile](Makefile) uses Microsoft **NMAKE**, the Visual C++ compiler/linker (`cl` and `link`) and the Windows resource compiler (`rc`). There is no Visual Studio solution or project file.

The build depends on shared LTR Data files outside this repository:

- Headers including `winstrct.h`, `winstrct.hpp` and `wstring.h` from [LTRData/include](https://github.com/LTRData/include), available through the compiler's include path. The makefile also expects `..\include\winstrct.h`.
- Shared `winstrct.lib` and `winstrcp.lib` libraries referenced by those headers.
- `..\lib\minwcrt.lib`, listed as a makefile prerequisite and selected by the source for the x86 DLL-runtime build.

The makefile selects `CPU` from `_BUILDARCH`, falling back to `i386`, and writes objects and the executable into that directory. Prepare the matching compiler environment, include/library paths and output directory before invoking `nmake`. This is a legacy build recipe, not a self-contained modern Windows SDK build; its compiler options and custom runtime dependencies may need adaptation.

The `install` target contains a maintainer-specific `P:\utils` destination.

## Original documentation and terms

By Olof Lagerkvist. See [regrepl.txt](regrepl.txt) for the original background and distribution notice, which permits free copying/distribution, prohibits modification and requires the executable and text file to be distributed together. That historical notice is retained unchanged.
