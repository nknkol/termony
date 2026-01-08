# Pitfalls (踩坑记录)

## 1) New-thread SIGSEGV after clone/clone3 fallback

Symptoms
- Crash right after `Mocking clone3 -> -ENOSYS` and a new LWP appears.
- GDB shows `$pc` inside a `rw-p` anonymous mapping; disassembly near PC is `udf`/garbage.

Root cause
- Old AArch64 stub used a hashed `tpidr_el0` slot array to save `x16/x17/x30`.
- New threads may share low bits or have `tpidr_el0` uninitialized, causing slot collisions and a corrupted return address.

Fix
- Switch stub to stack-based save/restore for `x16/x17/x30`.
- Rebuild to make sure the new stub is embedded in `payload.bin` and `elfloader`.

Quick check
- `info proc mappings` shows PC in `rw-p` anon region.
- Crash happens immediately after thread creation.

## 2) `tar: Child died with signal 11` during install

Symptoms
- `tar: Child died with signal 11` followed by install failure (e.g. `.tar.xz` unpack).
- Lots of `[Payload]` debug lines in the log right before the error.

Root cause
- Heavy debug output fills stderr/pty pipes; `tar`/`xz` can abort or get killed when output cannot drain.

Mitigation
- Lower logging (`DEBUG 0` on loader side; reduce payload hook logs).
- Limit scanning during install with `HOOK_RANGE`/`HOOK_RANGE_INTERP`.
- Redirect stderr to a file to avoid pipe pressure, e.g. `elfloader ... 2>payload.log`.
