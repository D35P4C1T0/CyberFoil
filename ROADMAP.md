# NSP Install Rework Roadmap

## Goal

Bring CyberFoil's NSP and NSZ install paths closer to a single installer model so local, shop, USB, HTTP, and MTP installs do not drift in behavior.

The immediate target is consistency:

- one shared PFS0/NSP header parser
- one clear definition of what counts as installable NSP content
- fewer install-path-specific edge cases around CNMT, tickets, and retries

## Why This Exists

The current codebase already has solid NCZ support in [source/nx/nca_writer.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/nx/nca_writer.cpp), including `NCZBLOCK`, but NSP handling is still fragmented across multiple entry points.

Compared with Sphaira, CyberFoil's main weakness is not raw decompression support. It is that NSP discovery and install orchestration are split across several flows, which makes fixes land unevenly.

## Phase 1: Shared Parsing

Status: in progress

- Extract duplicated PFS0 parsing into a reusable component.
- Reuse that parser from the classic `NSP` abstraction and the MTP NSP stream installer.
- Keep behavior stable while removing header-layout duplication.

Initial implementation in this branch:

- add shared parser: [include/install/pfs0_parser.hpp](/Users/matteo/Documents/prog/cpp/CyberFoil/include/install/pfs0_parser.hpp)
- add shared parser implementation: [source/install/pfs0_parser.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/install/pfs0_parser.cpp)
- refactor classic NSP wrapper: [source/install/nsp.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/install/nsp.cpp)
- refactor MTP NSP header parsing: [source/mtp_install.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/mtp_install.cpp)

## Phase 2: Shared NSP Content Model

Status: in progress

- Introduce a shared description for NSP entries instead of having each path infer `is_nca`, `is_cnmt`, ticket pairing, and offsets ad hoc.
- Reuse the same matching rules for `.nca`, `.ncz`, `.cnmt.nca`, `.cnmt.ncz`, `.tik`, and `.cert`.
- Audit places that still duplicate file classification outside the base NSP wrapper.

Current progress in this branch:

- add shared content filename classifier: [include/install/content_file.hpp](/Users/matteo/Documents/prog/cpp/CyberFoil/include/install/content_file.hpp)
- reuse it in MTP stream install flow: [source/mtp_install.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/mtp_install.cpp)
- reuse it in HTTP streamed collection installs: [source/shopInstall.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/shopInstall.cpp)
- centralize streamed install metadata commits: [include/install/stream_install_helper.hpp](/Users/matteo/Documents/prog/cpp/CyberFoil/include/install/stream_install_helper.hpp)
- centralize classic package CNMT and ticket/cert helpers: [include/install/package_install_helper.hpp](/Users/matteo/Documents/prog/cpp/CyberFoil/include/install/package_install_helper.hpp)
- centralize classic package NCA install flow: [source/install/package_install_helper.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/install/package_install_helper.cpp)

Likely targets:

- [source/install/install_nsp.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/install/install_nsp.cpp)
- [source/mtp_install.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/mtp_install.cpp)
- [source/shopInstall.cpp](/Users/matteo/Documents/prog/cpp/CyberFoil/source/shopInstall.cpp)

## Phase 3: CNMT and Update Robustness

Status: planned

- Re-check all update installs against the issue-75 fixes.
- Verify that every install path commits CNMT metadata in the same order and with the same payload preservation guarantees.
- Decide whether any path still reconstructs metadata too aggressively for update content.

Focus areas:

- update NSPs delivered over MTP
- shop-driven installs that bypass the classic `NSPInstall` path
- mixed `.cnmt.nca` and `.cnmt.ncz` packages

## Phase 4: Installer Unification

Status: planned

- Move toward a shared install driver for NSP-based sources, similar in spirit to Sphaira's unified installer core.
- Keep source-specific transport logic separate, but make content discovery and commit flow common.
- Reduce the number of places that manually open placeholders, register content, and import tickets.

## Validation Checklist

- local `.nsp` install still enumerates CNMT and NCA entries correctly
- local `.nsz` install still routes `.ncz` content through `NcaWriter`
- MTP NSP stream still parses headers incrementally and installs entries in order
- tickets and certs still import correctly at finalize time
- update content does not regress on the previous hang or metadata bugs

## Notes

- There is an unrelated untracked `tests/` directory in the worktree carried over from earlier work. This roadmap branch should ignore it unless we decide to formalize those tests later.
- Sphaira remains the comparison target for architecture, but CyberFoil already has stronger-than-expected NCZ handling in its writer layer. The rework should prioritize consistency before adding new compression features.
