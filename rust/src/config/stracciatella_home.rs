use std::path::PathBuf;

use dirs;

/// Find ja2 stracciatella configuration directory inside the user's home directory
pub fn find_stracciatella_home() -> Result<PathBuf, String> {
    #[cfg(not(windows))]
    let base = dirs::home_dir();
    #[cfg(windows)]
    let base = dirs::document_dir();
    #[cfg(not(windows))]
    let dir = ".ja2";
    #[cfg(windows)]
    let dir = "JA2";

    match base {
        Some(mut path) => {
            path.push(dir);
            Ok(path)
        }
        None => Err(String::from("Could not find home directory")),
    }
}
