fn main() {
    if let Ok(dir) = std::env::var("HESIA_CORE_LIB_DIR") {
        println!("cargo:rustc-link-search=native={dir}");
        println!("cargo:rustc-link-lib=dylib=hesia_core");
    }
}
