## RadShim

A **shim** is a small executable that acts as a proxy for another program. It contains a reference to a target executable and ensures that when the shim is invoked, the target program is executed transparently.

Shims allow commands (e.g. `git`) to be run without requiring the actual executable to be present in the system `PATH`.

---

## Alternatives

### Symbolic Link
A symbolic link can be used to point to the target executable. However, it assumes the executable operates correctly from the link’s location. This can lead to issues when the program depends on relative paths or external resources (e.g. DLLs), which may fail to resolve.

### Shortcut (`.lnk`)
Shortcuts require the `.lnk` file extension, meaning commands must be invoked with the extension (e.g. `git.lnk`). While this can be mitigated using the `PATHEXT` environment variable, it adds complexity and may require multiple entries (e.g. `git.lnk` and `git.exe.lnk`).

### Batch File
Batch files (`.bat`) can forward execution to another executable and behave similarly to shortcuts. However, they share the same extension-related limitations and are primarily intended for use with `cmd.exe`, which may lead to inconsistent behavior in other shells.
