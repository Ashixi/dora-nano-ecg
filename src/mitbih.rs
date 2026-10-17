use std::fs::File;
use std::io::Read;
use std::path::Path;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum AamiClass {
    /// N - Normal, Left/Right bundle branch blocks
    Normal = 0,
    /// S - Supraventricular ectopic (Atrial premature, etc.)
    Supraventricular = 1,
    /// V - Ventricular ectopic (PVC, Ventricular Escape) - CRITICAL LIFE-THREATENING
    Ventricular = 2,
    /// F - Fusion of ventricular and normal beat
    Fusion = 3,
    /// Q - Unknown / Paced / Unclassifiable
    Unknown = 4,
}

impl AamiClass {
    pub fn name(&self) -> &'static str {
        match self {
            AamiClass::Normal => "N (Normal)",
            AamiClass::Supraventricular => "S (Supraventricular)",
            AamiClass::Ventricular => "V (Ventricular - Critical)",
            AamiClass::Fusion => "F (Fusion)",
            AamiClass::Unknown => "Q (Unknown)",
        }
    }
}

pub const DS1_RECORDS: [&str; 22] = [
    "101", "106", "108", "109", "112", "114", "115", "116", "118", "119",
    "122", "124", "201", "203", "205", "207", "208", "209", "215", "220",
    "223", "230"
];

pub const DS2_RECORDS: [&str; 22] = [
    "100", "103", "105", "111", "113", "117", "121", "123", "200", "202",
    "210", "212", "213", "214", "219", "221", "222", "228", "231", "232",
    "233", "234"
];

#[derive(Clone, Debug)]
pub struct EcgBeat {
    /// 180 samples centered on R-peak (Lead 0, MLII)
    pub waveform: [f32; 180],
    /// 180 samples centered on R-peak (Lead 1, V5/V1/V2)
    pub waveform_ch1: [f32; 180],
    /// Timing dynamics: [pre_rr, post_rr, local_rr, rr_ratio]
    pub timing: [f32; 4],
    /// AAMI standard class
    pub class: AamiClass,
    /// Patient record number
    pub patient_id: u32,
    /// Sample index of R-peak in the 30-min recording
    pub r_peak: u32,
}

/// Map MIT annotation code to AAMI super-class
fn mit_code_to_aami(code: u8) -> Option<AamiClass> {
    match code {
        // N class: Normal, LBBB, RBBB, nodal escape, atrial escape
        1 | 2 | 3 | 11 | 34 => Some(AamiClass::Normal),
        // S class: Atrial premature, aberrant APB, nodal premature, supraventricular premature
        4 | 7 | 8 | 9 => Some(AamiClass::Supraventricular),
        // V class: Premature ventricular contraction (PVC), ventricular escape
        5 | 10 => Some(AamiClass::Ventricular),
        // F class: Fusion of ventricular and normal
        6 => Some(AamiClass::Fusion),
        // Q class: Paced, unclassifiable
        12 | 13 => Some(AamiClass::Unknown),
        // All others (non-beat markers, rhythm annotations, noise) are ignored
        _ => None,
    }
}

/// Parse MIT-BIH Format 212 binary signal
/// In Format 212, every 3 bytes contain two 12-bit samples.
pub fn parse_format_212(raw_bytes: &[u8]) -> (Vec<f32>, Vec<f32>) {
    let num_triplets = raw_bytes.len() / 3;
    let mut ch0 = Vec::with_capacity(num_triplets);
    let mut ch1 = Vec::with_capacity(num_triplets);

    for i in 0..num_triplets {
        let b0 = raw_bytes[i * 3] as u16;
        let b1 = raw_bytes[i * 3 + 1] as u16;
        let b2 = raw_bytes[i * 3 + 2] as u16;

        let s0_u = b0 | ((b1 & 0x0F) << 8);
        let s1_u = b2 | ((b1 >> 4) << 8);

        let s0 = if s0_u >= 2048 { s0_u as i32 - 4096 } else { s0_u as i32 };
        let s1 = if s1_u >= 2048 { s1_u as i32 - 4096 } else { s1_u as i32 };

        // Convert ADC units to physical millivolts: standard MIT-BIH gain = 200 ADC units/mV
        ch0.push(s0 as f32 / 200.0);
        ch1.push(s1 as f32 / 200.0);
    }

    (ch0, ch1)
}

/// Parse MIT-BIH .atr binary annotation file matching official WFDB specification
pub fn parse_atr(raw_bytes: &[u8]) -> Vec<(u32, AamiClass)> {
    let mut annotations = Vec::new();
    let mut sample_total: u64 = 0;
    let mut i = 0;

    while i + 1 < raw_bytes.len() {
        let b0 = raw_bytes[i];
        let b1 = raw_bytes[i + 1];
        let mut label_store = b1 >> 2;

        if label_store == 0 && b0 == 0 {
            // EOF
            break;
        }

        // Handle SKIP (code 59)
        let mut sample_diff: u64 = 0;
        while label_store == 59 {
            if i + 5 < raw_bytes.len() {
                let s0 = raw_bytes[i + 2] as u64;
                let s1 = raw_bytes[i + 3] as u64;
                let s2 = raw_bytes[i + 4] as u64;
                let s3 = raw_bytes[i + 5] as u64;
                let skip_diff = (s0 << 16) | (s1 << 24) | s2 | (s3 << 8);
                sample_diff += skip_diff;
                i += 6;
                if i + 1 < raw_bytes.len() {
                    label_store = raw_bytes[i + 1] >> 2;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        if i + 1 >= raw_bytes.len() {
            break;
        }

        let curr_b0 = raw_bytes[i];
        let curr_b1 = raw_bytes[i + 1];
        let curr_code = curr_b1 >> 2;

        if curr_code == 0 && curr_b0 == 0 {
            break;
        }

        if curr_code == 63 {
            // AUX: aux_len = curr_b0
            let aux_len = curr_b0 as usize;
            let pairs_to_skip = (aux_len + 1) / 2;
            i += 2 + pairs_to_skip * 2;
            continue;
        } else if curr_code == 60 || curr_code == 61 || curr_code == 62 {
            // NUM, SUB, CHAN
            i += 2;
            continue;
        } else if curr_code < 59 {
            let delta = (curr_b0 as u64) | (((curr_b1 & 3) as u64) << 8);
            sample_total += sample_diff + delta;
            i += 2;

            if let Some(aami_class) = mit_code_to_aami(curr_code) {
                annotations.push((sample_total as u32, aami_class));
            }
        } else {
            i += 2;
        }
    }

    annotations
}

/// Digital 2nd-order Biquad Bandpass filter (0.5 Hz - 40 Hz) at 360 Hz sampling rate
pub struct BiquadFilter {
    b0: f32, b1: f32, b2: f32,
    a1: f32, a2: f32,
    x1: f32, x2: f32,
    y1: f32, y2: f32,
}

impl BiquadFilter {
    /// Cascade of Highpass (0.5 Hz) and Lowpass (40 Hz)
    pub fn new_ecg_bandpass(fs: f32) -> (Self, Self) {
        // Highpass at 0.5 Hz: removes baseline drift / respiration
        let hp = Self::create_highpass(0.5, fs, 0.707);
        // Lowpass at 40.0 Hz: removes 50/60 Hz powerline hum and high-frequency muscle noise
        let lp = Self::create_lowpass(40.0, fs, 0.707);
        (hp, lp)
    }

    pub fn create_highpass(fc: f32, fs: f32, q: f32) -> Self {
        let w0 = 2.0 * std::f32::consts::PI * fc / fs;
        let alpha = w0.sin() / (2.0 * q);
        let cos_w0 = w0.cos();

        let a0 = 1.0 + alpha;
        let b0 = ((1.0 + cos_w0) / 2.0) / a0;
        let b1 = (-(1.0 + cos_w0)) / a0;
        let b2 = ((1.0 + cos_w0) / 2.0) / a0;
        let a1 = (-2.0 * cos_w0) / a0;
        let a2 = (1.0 - alpha) / a0;

        Self { b0, b1, b2, a1, a2, x1: 0.0, x2: 0.0, y1: 0.0, y2: 0.0 }
    }

    pub fn create_lowpass(fc: f32, fs: f32, q: f32) -> Self {
        let w0 = 2.0 * std::f32::consts::PI * fc / fs;
        let alpha = w0.sin() / (2.0 * q);
        let cos_w0 = w0.cos();

        let a0 = 1.0 + alpha;
        let b0 = ((1.0 - cos_w0) / 2.0) / a0;
        let b1 = (1.0 - cos_w0) / a0;
        let b2 = ((1.0 - cos_w0) / 2.0) / a0;
        let a1 = (-2.0 * cos_w0) / a0;
        let a2 = (1.0 - alpha) / a0;

        Self { b0, b1, b2, a1, a2, x1: 0.0, x2: 0.0, y1: 0.0, y2: 0.0 }
    }

    #[inline(always)]
    pub fn process(&mut self, x: f32) -> f32 {
        let y = self.b0 * x + self.b1 * self.x1 + self.b2 * self.x2 - self.a1 * self.y1 - self.a2 * self.y2;
        self.x2 = self.x1;
        self.x1 = x;
        self.y2 = self.y1;
        self.y1 = y;
        y
    }
}

pub fn filter_signal(raw: &[f32], fs: f32) -> Vec<f32> {
    let (mut hp, mut lp) = BiquadFilter::new_ecg_bandpass(fs);
    let mut out = Vec::with_capacity(raw.len());
    for &sample in raw {
        let h = hp.process(sample);
        let l = lp.process(h);
        out.push(l);
    }
    out
}

/// Load and slice a single patient record into EcgBeat structures
pub fn load_patient_record(data_dir: &str, rec_name: &str) -> Result<Vec<EcgBeat>, Box<dyn std::error::Error>> {
    let dat_path = Path::new(data_dir).join(format!("{}.dat", rec_name));
    let atr_path = Path::new(data_dir).join(format!("{}.atr", rec_name));

    let mut dat_file = File::open(&dat_path)?;
    let mut dat_bytes = Vec::new();
    dat_file.read_to_end(&mut dat_bytes)?;

    let mut atr_file = File::open(&atr_path)?;
    let mut atr_bytes = Vec::new();
    atr_file.read_to_end(&mut atr_bytes)?;

    // Parse both signals (Lead 0 and Lead 1)
    let (ch0_raw, ch1_raw) = parse_format_212(&dat_bytes);
    let clean_ecg0 = filter_signal(&ch0_raw, 360.0);
    let clean_ecg1 = filter_signal(&ch1_raw, 360.0);

    let annotations = parse_atr(&atr_bytes);
    let patient_id = rec_name.parse::<u32>().unwrap_or(0);

    let mut beats = Vec::new();
    let window_half: i32 = 90; // 90 samples before & 90 samples after R-peak

    for i in 1..annotations.len().saturating_sub(1) {
        let (r_peak, class) = annotations[i];
        let r_i32 = r_peak as i32;
        // Dynamic R-peak centering: find true local peak in [-12, +12] window around annotation
        let search_start = (r_i32 - 12).max(0) as usize;
        let search_end = (r_i32 + 13).min(clean_ecg0.len() as i32) as usize;
        let mut max_abs = 0.0f32;
        let mut best_r = r_i32 as usize;
        for s in search_start..search_end {
            let abs_v = clean_ecg0[s].abs();
            if abs_v > max_abs {
                max_abs = abs_v;
                best_r = s;
            }
        }

        let start = best_r as i32 - window_half;
        let end = best_r as i32 + window_half;

        if start < 0 || end as usize > clean_ecg0.len() || end as usize > clean_ecg1.len() {
            continue;
        }

        let slice0 = &clean_ecg0[start as usize..end as usize];
        let slice1 = &clean_ecg1[start as usize..end as usize];
        let mut waveform = [0.0f32; 180];
        let mut waveform_ch1 = [0.0f32; 180];
        
        // Normalize waveform 0
        let mut sum0 = 0.0f32;
        for &v in slice0 { sum0 += v; }
        let mean0 = sum0 / 180.0;
        let mut var_sum0 = 0.0f32;
        for &v in slice0 { var_sum0 += (v - mean0) * (v - mean0); }
        let std0 = (var_sum0 / 180.0).sqrt().max(1e-6);
        for j in 0..180 { waveform[j] = (slice0[j] - mean0) / std0; }

        // Normalize waveform 1
        let mut sum1 = 0.0f32;
        for &v in slice1 { sum1 += v; }
        let mean1 = sum1 / 180.0;
        let mut var_sum1 = 0.0f32;
        for &v in slice1 { var_sum1 += (v - mean1) * (v - mean1); }
        let std1 = (var_sum1 / 180.0).sqrt().max(1e-6);
        for j in 0..180 { waveform_ch1[j] = (slice1[j] - mean1) / std1; }

        // Timing features
        let prev_r = annotations[i - 1].0 as f32;
        let next_r = annotations[i + 1].0 as f32;
        let curr_r = r_peak as f32;

        let pre_rr = (curr_r - prev_r) / 360.0;
        let post_rr = (next_r - curr_r) / 360.0;

        let history_start = if i >= 5 { i - 5 } else { 0 };
        let mut history_sum = 0.0;
        let mut history_count = 0.0;
        for h in history_start..i {
            history_sum += (annotations[h + 1].0 - annotations[h].0) as f32 / 360.0;
            history_count += 1.0;
        }
        let local_rr = if history_count > 0.0 { history_sum / history_count } else { pre_rr };
        let rr_ratio = pre_rr / (local_rr + 1e-6);

        beats.push(EcgBeat {
            waveform,
            waveform_ch1,
            timing: [pre_rr, post_rr, local_rr, rr_ratio],
            class,
            patient_id,
            r_peak,
        });
    }

    Ok(beats)
}

/// Load entire DS1 (Train) split
pub fn load_ds1(data_dir: &str) -> Vec<EcgBeat> {
    let mut all_beats = Vec::new();
    for rec in DS1_RECORDS {
        if let Ok(beats) = load_patient_record(data_dir, rec) {
            all_beats.extend(beats);
        }
    }
    all_beats
}

/// Load entire DS2 (Test) split
pub fn load_ds2(data_dir: &str) -> Vec<EcgBeat> {
    let mut all_beats = Vec::new();
    for rec in DS2_RECORDS {
        if let Ok(beats) = load_patient_record(data_dir, rec) {
            all_beats.extend(beats);
        }
    }
    all_beats
}
