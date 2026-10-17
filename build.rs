use std::path::Path;

fn main() {
    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let lib_dir = Path::new(&manifest_dir).join("lib");
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    
    // Link against DLL import library
    println!("cargo:rustc-link-lib=dora_nano_import");

    // Copy DLL to root / output so executable can find it at runtime
    let dll_src = lib_dir.join("dora_nano.dll");
    let dll_dst = Path::new(&manifest_dir).join("dora_nano.dll");
    let _ = std::fs::copy(&dll_src, &dll_dst);

    if let Ok(out_dir) = std::env::var("OUT_DIR") {
        // Also copy next to target executable
        let target_dir = Path::new(&out_dir).ancestors().nth(3);
        if let Some(td) = target_dir {
            let _ = std::fs::copy(&dll_src, td.join("dora_nano.dll"));
        }
    }
}
