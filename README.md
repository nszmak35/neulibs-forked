# neulibs

Third-party libraries bundled for [ofwc](https://github.com/nszmak35/ofwc),
so it can be built without installing them separately.

| Directory | What it is | Upstream |
| --- | --- | --- |
| `neuswc/` | Wayland compositor library that ofwc is built on. Fork of swc, modified for ofwc. | [neuswc](https://git.sr.ht/~shrub900/neuswc), [swc](https://github.com/michaelforney/swc) |
| `neuwld/` | Drawing library used by neuswc. | [neuwld](https://git.sr.ht/~shrub900/neuwld) |

## neuswc

This copy of neuswc is a fork made only for ofwc. It adds new API functions
and bug fixes that ofwc needs. `swc.h` only gains new functions, so other
neuswc window managers should keep working. It is not meant as a
general-purpose release; for that use the original
[neuswc](https://git.sr.ht/~shrub900/neuswc).

Rebuild ofwc after rebuilding neuswc.

## neuwld

A copy of [neuwld](https://git.sr.ht/~shrub900/neuwld), bundled because
neuswc depends on it. For other uses, get it from the original repository.

## Licenses

Each directory keeps its own license; see the `LICENSE` / `COPYING` file
inside it.
