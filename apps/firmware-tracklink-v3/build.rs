fn main() {
    #[cfg(feature = "embedded")]
    println!("cargo:rustc-link-arg=-Tlinkall.x");
}
