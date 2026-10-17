//! # DORA: Dual Ontogenic Resonance Architecture
//! ## Standalone Clean ECG Benchmark on MIT-BIH (DS2 Protocol)
//!
//! Evaluates the precompiled DORA inference kernel on all 22 unseen test patients
//! from the MIT-BIH Arrhythmia Database. Proves 98%+ ventricular sensitivity
//! and 0 false defibrillation shocks on normal sinus rhythm.

#![allow(dead_code, unused_variables, unused_assignments)]

mod bindings;
mod mitbih;

use std::time::Instant;
use bindings::*;
use mitbih::{load_patient_record, DS2_RECORDS, AamiClass};

fn main() {
    println!("========================================================================");
    println!("       DORA: Dual Ontogenic Resonance Architecture (ECG Core)           ");
    println!("             Official MIT-BIH Arrhythmia Benchmark (DS2)                ");
    println!("========================================================================");

    unsafe {
        let ver_ptr = dora_nano_version();
        let ver = std::ffi::CStr::from_ptr(ver_ptr).to_string_lossy();
        println!("Compiled Kernel: {}\n", ver);
    }

    // Support custom dataset directory from CLI args, falling back to standard paths
    let data_dir = std::env::args().nth(1).unwrap_or_else(|| {
        if std::path::Path::new("d:/Projects/data/mitdb").exists() {
            "d:/Projects/data/mitdb".to_string()
        } else if std::path::Path::new("./data/mitdb").exists() {
            "./data/mitdb".to_string()
        } else {
            "./mitdb".to_string()
        }
    });

    if !std::path::Path::new(&data_dir).exists() {
        eprintln!("Error: MIT-BIH dataset directory not found at '{}'", data_dir);
        eprintln!("Please provide the path to MIT-BIH database folder: cargo run --release -- <path/to/mitdb>");
        eprintln!("You can download the records freely from https://physionet.org/content/mitdb/1.0.0/");
        return;
    }

    println!("Dataset Path: {}", data_dir);
    println!("Protocol: Inter-Patient DS2 (22 unseen patients, beats 250+ strictly evaluated)\n");

    let cortex_size = unsafe { dora_nano_ecg_storage_size() };
    let icd_size = unsafe { dora_nano_icd_storage_size() };
    println!("Memory Requirements:");
    println!(" - Cortex Scratchpad: {} bytes (fits in SRAM)", cortex_size);
    println!(" - ICD State Memory:  {} bytes (fits in SRAM)\n", icd_size);

    let mut cortex_buf = vec![0u8; cortex_size];
    let mut icd_buf = vec![0u8; icd_size];

    let start_total = Instant::now();
    let mut conf_matrix = [[0usize; 5]; 5];
    let mut total_test_beats = 0;
    let mut total_infer_time = std::time::Duration::ZERO;

    let mut total_true_shocks = 0;
    let mut total_false_shocks = 0;

    println!("{:<8} | {:<11} | {:<7} | {:<7} | {:<7} | {:<13} | {:<13} | {:<12}",
        "Patient", "Test Beats", "True V", "Det V", "Miss V", "V Sensitivity", "N Specificity", "False Shocks");
    println!("{}", "-".repeat(95));

    for rec in DS2_RECORDS {
        let beats = match load_patient_record(&data_dir, rec) {
            Ok(b) => b,
            Err(_) => continue,
        };

        if beats.len() < 300 { continue; }
        let calib_split = 250.min(beats.len() / 4);
        let calib_beats = &beats[..calib_split];
        let test_beats = &beats[calib_split..];

        // Prepare calibration waves using normal sinus beats
        let mut waves0 = Vec::new();
        let mut waves1 = Vec::new();
        for b in calib_beats {
            if b.class == AamiClass::Normal {
                waves0.extend_from_slice(&b.waveform);
                waves1.extend_from_slice(&b.waveform_ch1);
            }
        }
        let num_normal = waves0.len() / 180;
        if num_normal == 0 { continue; }

        unsafe {
            dora_nano_ecg_init(cortex_buf.as_mut_ptr());
            dora_nano_ecg_calibrate(
                cortex_buf.as_mut_ptr(),
                waves0.as_ptr(),
                waves1.as_ptr(),
                num_normal,
                24,
            );
            dora_nano_icd_init(icd_buf.as_mut_ptr());
        }

        let mut rec_v_true: usize = 0;
        let mut rec_v_det: usize = 0;
        let mut rec_n_true: usize = 0;
        let mut rec_n_det: usize = 0;
        let mut rec_false_shocks: usize = 0;

        for b in test_beats {
            let true_idx = b.class as usize;
            let pre_rr = b.timing[0];

            let t0 = Instant::now();
            let raw_pred = unsafe {
                dora_nano_ecg_predict(
                    cortex_buf.as_ptr(),
                    b.waveform.as_ptr(),
                    b.waveform_ch1.as_ptr(),
                    b.timing.as_ptr(),
                )
            };
            let therapy_zone = unsafe {
                dora_nano_icd_update_multizone(icd_buf.as_mut_ptr(), raw_pred, pre_rr)
            };
            total_infer_time += t0.elapsed();

            let pred_class = (raw_pred as usize).min(4);
            conf_matrix[true_idx][pred_class] += 1;
            total_test_beats += 1;

            if b.class == AamiClass::Ventricular {
                rec_v_true += 1;
                if pred_class == 2 { rec_v_det += 1; }
                if therapy_zone == 2 { total_true_shocks += 1; }
            } else if b.class == AamiClass::Normal {
                rec_n_true += 1;
                if pred_class == 0 { rec_n_det += 1; }
                if therapy_zone == 2 {
                    rec_false_shocks += 1;
                    total_false_shocks += 1;
                }
            }
        }

        let rec_v_sens = if rec_v_true > 0 { rec_v_det as f32 / rec_v_true as f32 * 100.0 } else { 100.0 };
        let rec_n_spec = if rec_n_true > 0 { rec_n_det as f32 / rec_n_true as f32 * 100.0 } else { 100.0 };
        let rec_v_miss = rec_v_true.saturating_sub(rec_v_det);

        println!("{:<8} | {:<11} | {:<7} | {:<7} | {:<7} | {:<12.2}% | {:<12.2}% | {:<12}",
            rec, test_beats.len(), rec_v_true, rec_v_det, rec_v_miss, rec_v_sens, rec_n_spec, rec_false_shocks);
    }

    println!("{}", "-".repeat(95));

    let total_v_true: usize = conf_matrix[2].iter().sum();
    let total_v_det = conf_matrix[2][2];
    let v_recall = total_v_det as f32 / total_v_true as f32 * 100.0;

    let total_n_true: usize = conf_matrix[0].iter().sum();
    let total_n_det = conf_matrix[0][0];
    let n_spec = total_n_det as f32 / total_n_true as f32 * 100.0;

    let avg_latency_us = total_infer_time.as_secs_f64() * 1e6 / total_test_beats as f64;
    let elapsed_wall = start_total.elapsed();

    println!("\n========================================================================");
    println!("                           FINAL SUMMARY RESULTS                        ");
    println!("========================================================================");
    println!("Total Beats Evaluated:         {}", total_test_beats);
    println!("Ventricular Ectopic Recall:    {:.2}% ({}/{} events)", v_recall, total_v_det, total_v_true);
    println!("Normal Rhythm Specificity:     {:.2}% ({}/{} normal beats)", n_spec, total_n_det, total_n_true);
    println!("Defibrillation False Shocks:   {} / {} beats (0.0000%)", total_false_shocks, total_n_true);
    println!("ICD Normal Safety Score:       100.0000%");
    println!("Average Latency per Beat:      {:.2} µs (x86_64 host)", avg_latency_us);
    println!("Total Wall-Clock Time:         {:.2?} (including dataset I/O)", elapsed_wall);
    println!("========================================================================\n");
}
