//! C-ABI Bindings for DoraNano precompiled kernel

use std::os::raw::c_char;

extern "C" {
    pub fn dora_nano_version() -> *const c_char;
    pub fn dora_nano_ecg_storage_size() -> usize;
    pub fn dora_nano_ecg_init(storage: *mut u8) -> i32;
    pub fn dora_nano_ecg_calibrate(
        handle: *mut u8,
        waves0: *const f32,
        waves1: *const f32,
        num_beats: usize,
        k_prototypes: usize,
    ) -> i32;
    pub fn dora_nano_ecg_predict(
        handle: *const u8,
        wave0: *const f32,
        wave1: *const f32,
        timing: *const f32,
    ) -> i32;
    pub fn dora_nano_icd_storage_size() -> usize;
    pub fn dora_nano_icd_init(storage: *mut u8) -> i32;
    pub fn dora_nano_icd_update(
        handle: *mut u8,
        raw_pred: i32,
        rr_sec: f32,
        out_shock_armed: *mut bool,
    ) -> i32;
    pub fn dora_nano_icd_update_multizone(
        handle: *mut u8,
        raw_pred: i32,
        rr_sec: f32,
    ) -> i32;
}
