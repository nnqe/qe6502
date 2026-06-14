#!/usr/bin/env python3
"""Assemble workflow artifacts into stable GitHub Release asset files."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
import zipfile
from pathlib import Path

NATIVE_TARGETS = (
    "linux-x64",
    "linux-arm64",
    "macos-x64",
    "macos-arm64",
    "windows-x64",
    "windows-arm64",
)


def all_files(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob("*") if path.is_file())


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def find_exact_file(root: Path, name: str) -> Path:
    matches = [path for path in all_files(root) if path.name == name]
    if len(matches) != 1:
        raise RuntimeError(f"Expected exactly one {name}, found {len(matches)}: {matches}")
    return matches[0]


def artifact_dir(root: Path, name: str) -> Path:
    direct = root / name
    if direct.is_dir():
        return direct

    matches = [path for path in root.rglob(name) if path.is_dir()]
    if len(matches) != 1:
        raise RuntimeError(f"Expected exactly one artifact directory {name}, found {len(matches)}: {matches}")
    return matches[0]


def copy_asset(src: Path, out_dir: Path, name: str | None = None) -> Path:
    dst = out_dir / (name or src.name)
    shutil.copy2(src, dst)
    return dst


def write_zip_from_files(zip_path: Path, files: list[Path], base_dir: Path | None = None) -> Path:
    require(files, f"No files supplied for {zip_path.name}")
    if zip_path.exists():
        zip_path.unlink()

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for src in sorted(files):
            if base_dir is None:
                arcname = src.name
            else:
                arcname = src.relative_to(base_dir).as_posix()
            zf.write(src, arcname)

    return zip_path


def write_zip_from_dir(zip_path: Path, src_dir: Path) -> Path:
    files = all_files(src_dir)
    return write_zip_from_files(zip_path, files, src_dir)


def find_artifact_file(artifacts_root: Path, artifact_name: str, file_name: str) -> Path:
    directory = artifact_dir(artifacts_root, artifact_name)
    matches = [path for path in all_files(directory) if path.name == file_name]
    if len(matches) != 1:
        raise RuntimeError(
            f"Expected exactly one {file_name} under artifact {artifact_name}, found {len(matches)}: {matches}"
        )
    return matches[0]


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def assemble(args: argparse.Namespace) -> list[Path]:
    artifacts_root = args.artifacts_dir.resolve()
    out_dir = args.out_dir.resolve()
    version = args.version

    require(artifacts_root.is_dir(), f"Artifacts directory does not exist: {artifacts_root}")
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    produced: list[Path] = []

    for target in NATIVE_TARGETS:
        name = f"qe6502-native-{version}-{target}.zip"
        produced.append(copy_asset(find_exact_file(artifacts_root, name), out_dir))

    python_wheel_artifacts = [artifact_dir(artifacts_root, f"qe6502-release-python-wheel-{os_name}-{version}") for os_name in (
        "ubuntu-latest",
        "ubuntu-24.04-arm",
        "macos-15-intel",
        "macos-15",
        "windows-latest",
        "windows-11-arm",
    )]
    python_files: list[Path] = []
    for directory in python_wheel_artifacts:
        wheels = [path for path in all_files(directory) if path.suffix == ".whl"]
        require(len(wheels) == 1, f"Expected one wheel under {directory}, found {len(wheels)}: {wheels}")
        python_files.extend(wheels)
    python_sdist = find_artifact_file(
        artifacts_root,
        f"qe6502-release-python-sdist-{version}",
        f"qe6502-{version}.tar.gz",
    )
    python_files.append(python_sdist)
    produced.append(write_zip_from_files(out_dir / f"qe6502-python-{version}-wheels.zip", python_files))

    csharp_nupkg = find_artifact_file(
        artifacts_root,
        f"qe6502-release-csharp-nuget-{version}",
        f"Qe6502.{version}.nupkg",
    )
    produced.append(write_zip_from_files(out_dir / f"qe6502-csharp-{version}-nupkg.zip", [csharp_nupkg]))

    js_dir = artifact_dir(artifacts_root, f"qe6502-release-js-npm-{version}")
    js_files = all_files(js_dir)
    require(any(path.name == f"qe6502-{version}.tgz" for path in js_files), f"Missing qe6502-{version}.tgz in {js_dir}")
    produced.append(write_zip_from_dir(out_dir / f"qe6502-wasm-{version}.zip", js_dir))

    java_dir = artifact_dir(artifacts_root, f"qe6502-release-java-maven-layout-{version}")
    java_files = all_files(java_dir)
    for suffix in (".jar", "-sources.jar", "-javadoc.jar", ".pom"):
        expected = f"qe6502-{version}{suffix}"
        require(any(path.name == expected for path in java_files), f"Missing Java Maven layout file {expected}")
    produced.append(write_zip_from_dir(out_dir / f"qe6502-java-{version}-maven-bundle.zip", java_dir))

    central_bundle_name = f"qe6502-java-maven-central-{version}.zip"
    central_bundle_matches = [path for path in all_files(artifacts_root) if path.name == central_bundle_name]
    if central_bundle_matches:
        require(len(central_bundle_matches) == 1, f"Expected at most one {central_bundle_name}, found {len(central_bundle_matches)}")
        produced.append(copy_asset(central_bundle_matches[0], out_dir, f"qe6502-java-{version}-maven-central-bundle.zip"))

    rust_dir = artifact_dir(artifacts_root, f"qe6502-release-rust-crate-{version}")
    rust_files = all_files(rust_dir)
    require(any(path.suffix == ".crate" for path in rust_files), f"Missing Rust .crate under {rust_dir}")
    produced.append(write_zip_from_dir(out_dir / f"qe6502-rust-{version}-crate.zip", rust_dir))

    vcpkg_dir = artifact_dir(artifacts_root, f"qe6502-release-vcpkg-port-{version}")
    require((vcpkg_dir / "ports" / "qe6502" / "vcpkg.json").is_file(), f"Missing vcpkg.json under {vcpkg_dir}")
    require((vcpkg_dir / "ports" / "qe6502" / "portfile.cmake").is_file(), f"Missing portfile.cmake under {vcpkg_dir}")
    produced.append(write_zip_from_dir(out_dir / f"qe6502-release-vcpkg-port-{version}.zip", vcpkg_dir))

    source_tar = find_artifact_file(
        artifacts_root,
        f"qe6502-release-source-{version}",
        f"qe6502-{version}.tar.gz",
    )
    source_sha = find_artifact_file(
        artifacts_root,
        f"qe6502-release-source-{version}",
        f"qe6502-{version}.tar.gz.sha256",
    )
    produced.append(copy_asset(source_tar, out_dir))
    produced.append(copy_asset(source_sha, out_dir))

    manifest = {
        "version": version,
        "assets": [
            {"name": path.name, "sha256": sha256_file(path), "size": path.stat().st_size}
            for path in sorted(produced, key=lambda p: p.name)
        ],
    }
    manifest_path = out_dir / f"qe6502-release-assets-{version}.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    produced.append(manifest_path)

    sums_path = out_dir / "SHA256SUMS"
    with sums_path.open("w", encoding="utf-8") as f:
        for path in sorted(produced, key=lambda p: p.name):
            f.write(f"{sha256_file(path)}  {path.name}\n")
    produced.append(sums_path)

    return sorted(produced, key=lambda p: p.name)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts-dir", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--version", required=True)
    args = parser.parse_args()

    try:
        produced = assemble(args)
    except Exception as exc:  # noqa: BLE001 - release assembly should fail with a clear message.
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print("Assembled release assets:")
    for path in produced:
        print(f"  {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
