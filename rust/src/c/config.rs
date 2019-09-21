//! This module contains the C interface for [`stracciatella::config`].
//!
//! [`stracciatella::config`]: ../../config/index.html

use std::ptr;

use crate::c::common::*;
use crate::config::{
    find_stracciatella_home, Cli, EngineOptions, Ja2Json, Resolution, ScalingQuality,
    VanillaVersion,
};

/// Creates `EngineOptions` with the provided command line arguments.
/// Loads values from `(stracciatella_home)/ja2.json`, creating it if it does not exist.
/// The caller is responsible for the returned memory.
#[no_mangle]
pub extern "C" fn create_engine_options(
    array: *const *const c_char,
    length: size_t,
) -> *mut EngineOptions {
    let args: Vec<String> = unsafe_slice(array, length)
        .iter()
        .map(|&x| str_from_c_str_or_panic(unsafe_c_str(x)).to_owned())
        .collect();

    match find_stracciatella_home().and_then(|x| EngineOptions::from_home_and_args(&x, &args)) {
        Ok(engine_options) => {
            if engine_options.show_help {
                print!("{}", Cli::usage());
            }
            into_ptr(engine_options)
        }
        Err(msg) => {
            println!("{}", msg);
            ptr::null_mut()
        }
    }
}

/// Writes `EngineOptions` to `(stracciatella_home)/ja2.json`.
/// Returns true on success, false otherwise.
#[no_mangle]
pub extern "C" fn write_engine_options(ptr: *mut EngineOptions) -> bool {
    let engine_options = unsafe_mut(ptr);
    let ja2_json = Ja2Json::from_stracciatella_home(&engine_options.stracciatella_home);
    ja2_json.write(&engine_options).is_ok()
}

/// Deletes `EngineOptions`.
#[no_mangle]
pub extern "C" fn free_engine_options(ptr: *mut EngineOptions) {
    let _drop_me = from_ptr(ptr);
}

/// Gets the `EngineOptions.stracciatella_home` path.
/// The caller is responsible for the returned memory.
#[no_mangle]
pub extern "C" fn get_stracciatella_home(ptr: *const EngineOptions) -> *mut c_char {
    let engine_options = unsafe_ref(ptr);
    let stracciatella_home = c_string_from_path_or_panic(&engine_options.stracciatella_home);
    stracciatella_home.into_raw()
}

/// Gets the `EngineOptions.vanilla_game_dir` path.
/// The caller is responsible for the returned memory.
#[no_mangle]
pub extern "C" fn get_vanilla_game_dir(ptr: *const EngineOptions) -> *mut c_char {
    let engine_options = unsafe_ref(ptr);
    let vanilla_game_dir = c_string_from_path_or_panic(&engine_options.vanilla_game_dir);
    vanilla_game_dir.into_raw()
}

/// Sets the `EngineOptions.vanilla_game_dir` path.
#[no_mangle]
pub extern "C" fn set_vanilla_game_dir(ptr: *mut EngineOptions, game_dir_ptr: *const c_char) {
    let engine_options = unsafe_mut(ptr);
    let vanilla_game_dir = path_from_c_str_or_panic(unsafe_c_str(game_dir_ptr));
    engine_options.vanilla_game_dir = vanilla_game_dir.to_owned();
}

/// Gets the length of `EngineOptions.mods`.
#[no_mangle]
pub extern "C" fn get_number_of_mods(ptr: *const EngineOptions) -> u32 {
    let engine_options = unsafe_ref(ptr);
    engine_options.mods.len() as u32
}

/// Gets the target index of `EngineOptions.mods`.
/// The caller is responsible for the returned memory.
#[no_mangle]
pub extern "C" fn get_mod(ptr: *const EngineOptions, index: u32) -> *mut c_char {
    let engine_options = unsafe_ref(ptr);
    match engine_options.mods.get(index as usize) {
        Some(str_mod) => {
            let c_str_mod = c_string_from_str(&str_mod);
            c_str_mod.into_raw()
        }
        None => {
            let len = engine_options.mods.len();
            panic!("Invalid mod index {}, len = {}", index, len);
        }
    }
}

/// Clears `EngineOptions.mods`.
#[no_mangle]
pub extern "C" fn clear_mods(ptr: *mut EngineOptions) {
    let engine_options = unsafe_mut(ptr);
    engine_options.mods.clear();
}

/// Adds a mod to `EngineOptions.mods`.
#[no_mangle]
pub extern "C" fn push_mod(ptr: *mut EngineOptions, name: *const c_char) {
    let engine_options = unsafe_mut(ptr);
    let name = str_from_c_str_or_panic(unsafe_c_str(name)).to_owned();
    engine_options.mods.push(name);
}

/// Gets the width of `EngineOptions.resolution`.
#[no_mangle]
pub extern "C" fn get_resolution_x(ptr: *const EngineOptions) -> u16 {
    let engine_options = unsafe_ref(ptr);
    engine_options.resolution.0
}

/// Gets the height of `EngineOptions.resolution`.
#[no_mangle]
pub extern "C" fn get_resolution_y(ptr: *const EngineOptions) -> u16 {
    let engine_options = unsafe_ref(ptr);
    engine_options.resolution.1
}

/// Sets `EngineOptions.resolution`.
#[no_mangle]
pub extern "C" fn set_resolution(ptr: *mut EngineOptions, x: u16, y: u16) {
    let engine_options = unsafe_mut(ptr);
    engine_options.resolution = Resolution(x, y);
}

/// Gets `EngineOptions.brightness`.
#[no_mangle]
pub extern "C" fn get_brightness(ptr: *const EngineOptions) -> f32 {
    let engine_options = unsafe_ref(ptr);
    engine_options.brightness
}

/// Sets `EngineOptions.brightness`.
#[no_mangle]
pub extern "C" fn set_brightness(ptr: *mut EngineOptions, brightness: f32) {
    let engine_options = unsafe_mut(ptr);
    engine_options.brightness = brightness
}

/// Gets `EngineOptions.resource_version`.
#[no_mangle]
pub extern "C" fn get_resource_version(ptr: *const EngineOptions) -> VanillaVersion {
    let engine_options = unsafe_ref(ptr);
    engine_options.resource_version
}

/// Sets `EngineOptions.resource_version`.
#[no_mangle]
pub extern "C" fn set_resource_version(ptr: *mut EngineOptions, res: VanillaVersion) {
    let engine_options = unsafe_mut(ptr);
    engine_options.resource_version = res;
}

/// Gets `EngineOptions.run_unittests`.
#[no_mangle]
pub extern "C" fn should_run_unittests(ptr: *const EngineOptions) -> bool {
    let engine_options = unsafe_ref(ptr);
    engine_options.run_unittests
}

/// Gets `EngineOptions.show_help`.
#[no_mangle]
pub extern "C" fn should_show_help(ptr: *const EngineOptions) -> bool {
    let engine_options = unsafe_ref(ptr);
    engine_options.show_help
}

/// Gets `EngineOptions.run_editor`.
#[no_mangle]
pub extern "C" fn should_run_editor(ptr: *const EngineOptions) -> bool {
    let engine_options = unsafe_ref(ptr);
    engine_options.run_editor
}

/// Gets `EngineOptions.start_in_fullscreen`.
#[no_mangle]
pub extern "C" fn should_start_in_fullscreen(ptr: *const EngineOptions) -> bool {
    let engine_options = unsafe_ref(ptr);
    engine_options.start_in_fullscreen
}

/// Sets `EngineOptions.start_in_fullscreen`.
#[no_mangle]
pub extern "C" fn set_start_in_fullscreen(ptr: *mut EngineOptions, val: bool) {
    let engine_options = unsafe_mut(ptr);
    engine_options.start_in_fullscreen = val
}

/// Gets `EngineOptions.scaling_quality`.
#[no_mangle]
pub extern "C" fn get_scaling_quality(ptr: *const EngineOptions) -> ScalingQuality {
    let engine_options = unsafe_ref(ptr);
    engine_options.scaling_quality
}

/// Sets `EngineOptions.scaling_quality`.
#[no_mangle]
pub extern "C" fn set_scaling_quality(ptr: *mut EngineOptions, scaling_quality: ScalingQuality) {
    let engine_options = unsafe_mut(ptr);
    engine_options.scaling_quality = scaling_quality
}

/// Gets `EngineOptions.start_in_window`.
#[no_mangle]
pub extern "C" fn should_start_in_window(ptr: *const EngineOptions) -> bool {
    let engine_options = unsafe_ref(ptr);
    engine_options.start_in_window
}

/// Gets `EngineOptions.start_in_debug_mode`.
#[no_mangle]
pub extern "C" fn should_start_in_debug_mode(ptr: *const EngineOptions) -> bool {
    let engine_options = unsafe_ref(ptr);
    engine_options.start_in_debug_mode
}

/// Gets `EngineOptions.start_without_sound`.
#[no_mangle]
pub extern "C" fn should_start_without_sound(ptr: *const EngineOptions) -> bool {
    let engine_options = unsafe_ref(ptr);
    engine_options.start_without_sound
}

/// Sets `EngineOptions.start_without_sound`.
#[no_mangle]
pub extern "C" fn set_start_without_sound(ptr: *mut EngineOptions, val: bool) {
    let engine_options = unsafe_mut(ptr);
    engine_options.start_without_sound = val
}

/// Gets the string representation of the `ScalingQuality` value.
/// The caller is responsible for the returned memory.
#[no_mangle]
pub extern "C" fn get_scaling_quality_string(quality: ScalingQuality) -> *mut c_char {
    let c_string = c_string_from_str(&quality.to_string());
    c_string.into_raw()
}

/// Gets the string represntation of the `VanillaVersion` value.
/// The caller is responsible for the returned memory.
#[no_mangle]
pub extern "C" fn get_resource_version_string(version: VanillaVersion) -> *mut c_char {
    let c_string = c_string_from_str(&version.to_string());
    c_string.into_raw()
}

#[cfg(test)]
mod tests {
    use std::fs;

    use crate::c::common::*;
    use crate::c::config::*;
    use crate::c::misc::free_rust_string;
    use crate::config::{EngineOptions, Resolution};
    use crate::parse_json_config;
    use crate::tests::write_temp_folder_with_ja2_json;

    #[test]
    fn write_engine_options_should_write_a_json_file_that_can_be_serialized_again() {
        let mut engine_options = EngineOptions::default();
        let temp_dir = write_temp_folder_with_ja2_json(b"Invalid JSON");
        let stracciatella_home = temp_dir.path().join(".ja2");

        engine_options.stracciatella_home = stracciatella_home.clone();
        engine_options.resolution = Resolution(100, 100);

        assert_eq!(write_engine_options(&mut engine_options), true);

        let got_engine_options = parse_json_config(&stracciatella_home).unwrap();

        assert_eq!(got_engine_options.resolution, engine_options.resolution);
    }

    #[test]
    fn write_engine_options_should_write_a_pretty_json_file() {
        let mut engine_options = EngineOptions::default();
        let temp_dir = write_temp_folder_with_ja2_json(b"Invalid JSON");
        let stracciatella_home = temp_dir.path().join(".ja2");
        let stracciatella_json = temp_dir.path().join(".ja2/ja2.json");

        engine_options.stracciatella_home = stracciatella_home.clone();
        engine_options.resolution = Resolution(100, 100);

        write_engine_options(&mut engine_options);

        let config_file_contents = fs::read_to_string(stracciatella_json).unwrap();

        assert_eq!(
            config_file_contents,
            r##"{
  "game_dir": "",
  "mods": [],
  "res": "100x100",
  "brightness": 1.0,
  "resversion": "ENGLISH",
  "fullscreen": false,
  "scaling": "PERFECT",
  "debug": false,
  "nosound": false
}"##
        );
    }

    #[test]
    fn get_resource_version_string_should_return_the_correct_resource_version_string() {
        macro_rules! t {
            ($version:expr, $expected:expr) => {
                let got = get_resource_version_string($version);
                assert_eq!(str_from_c_str_or_panic(unsafe_c_str(got)), $expected);
                free_rust_string(got);
            };
        }
        t!(VanillaVersion::DUTCH, "Dutch");
        t!(VanillaVersion::ENGLISH, "English");
        t!(VanillaVersion::FRENCH, "French");
        t!(VanillaVersion::GERMAN, "German");
        t!(VanillaVersion::ITALIAN, "Italian");
        t!(VanillaVersion::POLISH, "Polish");
        t!(VanillaVersion::RUSSIAN, "Russian");
        t!(VanillaVersion::RUSSIAN_GOLD, "Russian (Gold)");
    }
}
