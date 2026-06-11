use std::env;

fn main() {
    println!("cargo:rerun-if-env-changed=QE6502_RUST_NATIVE_LIBRARY_DIR");
    println!("cargo:rerun-if-env-changed=QE6502_RUST_NATIVE_LIBRARY_LINK_NAME");

    let library_dir = env::var("QE6502_RUST_NATIVE_LIBRARY_DIR")
        .expect("QE6502_RUST_NATIVE_LIBRARY_DIR must be set by CMake");
    let library_link_name = env::var("QE6502_RUST_NATIVE_LIBRARY_LINK_NAME")
        .expect("QE6502_RUST_NATIVE_LIBRARY_LINK_NAME must be set by CMake");

    println!("cargo:rustc-link-search=native={library_dir}");
    println!("cargo:rustc-link-lib=static={library_link_name}");
}
