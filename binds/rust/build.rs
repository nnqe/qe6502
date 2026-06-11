use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(
        env::var_os("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR is set by Cargo"),
    );
    let native_dir = manifest_dir.join("native");
    let source_file = native_dir.join("src").join("qe6502.c");
    let include_dir = native_dir.join("include");
    let source_dir = native_dir.join("src");

    println!("cargo:rerun-if-changed={}", source_file.display());
    println!(
        "cargo:rerun-if-changed={}",
        include_dir.join("qe6502").join("qe6502.h").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        include_dir.join("qe6502").join("qe6502_abi.h").display()
    );
    println!("cargo:rerun-if-changed={}", source_dir.join("control_store").display());

    cc::Build::new()
        .file(source_file)
        .include(include_dir)
        .include(source_dir)
        .compile("qe6502");
}
