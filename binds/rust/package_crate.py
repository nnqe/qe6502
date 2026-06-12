#!/usr/bin/env python3
"""Create a self-contained qe6502 Rust crate staging directory.

The repository keeps the canonical C core under cpu/. This script creates a
publish/package-ready Rust crate by copying the Rust binding plus the required
canonical C sources into a generated output directory. The generated native/
tree is an artifact only and must not be committed as a second source of truth.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

PACKAGE_INCLUDE_BLOCK = """readme = \"README.md\"
include = [
    \"Cargo.toml\",
    \"Cargo.lock\",
    \"build.rs\",
    \"src/**/*.rs\",
    \"README.md\",
    \"LICENSE\",
    \"native/include/qe6502/qe6502.h\",
    \"native/include/qe6502/qe6502_version.h\",
    \"native/include/qe6502/qe6502_abi.h\",
    \"native/src/qe6502.c\",
    \"native/src/control_store/**\",
]
"""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Stage a self-contained qe6502 Rust crate for cargo package/publish checks."
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="Repository root containing binds/rust and cpu (default: inferred from script path).",
    )
    parser.add_argument(
        "--out",
        type=Path,
        required=True,
        help="Output directory for the generated crate, for example build/rust-package/qe6502.",
    )
    return parser.parse_args()


def copy_file(src: Path, dst: Path) -> None:
    if not src.is_file():
        raise FileNotFoundError(src)
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def copy_dir(src: Path, dst: Path) -> None:
    if not src.is_dir():
        raise FileNotFoundError(src)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def transform_cargo_toml(src: Path, dst: Path) -> None:
    text = src.read_text(encoding="utf-8")
    lines = text.splitlines()

    output: list[str] = []
    inserted_package_fields = False

    for line in lines:
        stripped = line.strip()

        if stripped == "publish = false":
            continue

        if stripped == "[build-dependencies]" and not inserted_package_fields:
            if not any(existing.strip().startswith("readme =") for existing in output):
                output.extend(PACKAGE_INCLUDE_BLOCK.rstrip("\n").splitlines())
            inserted_package_fields = True
            output.append("")

        output.append(line)

    if not inserted_package_fields:
        if output and output[-1] != "":
            output.append("")
        output.extend(PACKAGE_INCLUDE_BLOCK.rstrip("\n").splitlines())

    dst.write_text("\n".join(output) + "\n", encoding="utf-8")


def stage_crate(repo_root: Path, out_dir: Path) -> None:
    repo_root = repo_root.resolve()
    rust_dir = repo_root / "binds" / "rust"
    cpu_dir = repo_root / "cpu"

    required_inputs = [
        rust_dir / "Cargo.toml",
        rust_dir / "Cargo.lock",
        rust_dir / "build.rs",
        rust_dir / "src",
        cpu_dir / "include" / "qe6502" / "qe6502.h",
        cpu_dir / "include" / "qe6502" / "qe6502_version.h",
        cpu_dir / "include" / "qe6502" / "qe6502_abi.h",
        cpu_dir / "src" / "qe6502.c",
        cpu_dir / "src" / "control_store",
        repo_root / "README.md",
        repo_root / "LICENSE",
    ]

    missing = [str(path) for path in required_inputs if not path.exists()]
    if missing:
        raise RuntimeError("required packaging inputs are missing:\n" + "\n".join(missing))

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    transform_cargo_toml(rust_dir / "Cargo.toml", out_dir / "Cargo.toml")
    copy_file(rust_dir / "Cargo.lock", out_dir / "Cargo.lock")
    copy_file(rust_dir / "build.rs", out_dir / "build.rs")
    copy_dir(rust_dir / "src", out_dir / "src")
    copy_file(repo_root / "README.md", out_dir / "README.md")
    copy_file(repo_root / "LICENSE", out_dir / "LICENSE")

    copy_file(
        cpu_dir / "include" / "qe6502" / "qe6502.h",
        out_dir / "native" / "include" / "qe6502" / "qe6502.h",
    )
    copy_file(
        cpu_dir / "include" / "qe6502" / "qe6502_version.h",
        out_dir / "native" / "include" / "qe6502" / "qe6502_version.h",
    )
    copy_file(
        cpu_dir / "include" / "qe6502" / "qe6502_abi.h",
        out_dir / "native" / "include" / "qe6502" / "qe6502_abi.h",
    )
    copy_file(
        cpu_dir / "src" / "qe6502.c",
        out_dir / "native" / "src" / "qe6502.c",
    )
    copy_dir(
        cpu_dir / "src" / "control_store",
        out_dir / "native" / "src" / "control_store",
    )


def main() -> int:
    args = parse_args()
    try:
        stage_crate(args.repo_root, args.out)
    except Exception as exc:  # noqa: BLE001 - script should print user-facing build error.
        print(f"error: failed to stage qe6502 Rust crate: {exc}", file=sys.stderr)
        return 1

    print(f"staged qe6502 Rust crate at {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
