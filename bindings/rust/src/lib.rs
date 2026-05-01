use std::ffi::CStr;
use std::os::raw::{c_char, c_int};

pub const HESIA_STATUS_OK: i32 = 0;
pub const HESIA_STATUS_UNSUPPORTED: i32 = 3;

pub const HESIA_FEATURE_OS_RANDOM: i32 = 1;
pub const HESIA_FEATURE_OPENSSL_AEAD: i32 = 2;
pub const HESIA_FEATURE_LIBOQS_PQC: i32 = 3;
pub const HESIA_FEATURE_WATCHDOG: i32 = 4;
pub const HESIA_FEATURE_INFERENCE_GUARD: i32 = 5;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct HesiaError {
    pub status: c_int,
    pub subsystem: [c_char; 32],
    pub message: [c_char; 192],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct HesiaWatchdog {
    pub timeout_ms: u64,
    pub last_heartbeat_ms: u64,
    pub max_missed: u32,
    pub missed: u32,
    pub armed: c_int,
}

#[repr(C)]
pub struct HesiaModelContract {
    pub model_id: *const c_char,
    pub model_version: u32,
    pub max_input_bytes: usize,
    pub max_output_bytes: usize,
    pub max_latency_ms: u32,
    pub require_signed_model: c_int,
    pub model_digest: *const u8,
    pub model_digest_len: usize,
}

#[repr(C)]
pub struct HesiaInferenceRequest {
    pub input: *const u8,
    pub input_len: usize,
    pub output: *mut u8,
    pub output_capacity: usize,
    pub deadline_ms: u32,
}

extern "C" {
    fn hesia_version() -> *const c_char;
    fn hesia_status_string(status: c_int) -> *const c_char;
    fn hesia_feature_available(feature: c_int) -> c_int;
    fn hesia_error_init(error: *mut HesiaError);
    fn hesia_error_message(error: *const HesiaError) -> *const c_char;
    fn hesia_random(out: *mut u8, len: usize, error: *mut HesiaError) -> c_int;
    fn hesia_watchdog_init(
        watchdog: *mut HesiaWatchdog,
        timeout_ms: u64,
        max_missed: u32,
        error: *mut HesiaError,
    ) -> c_int;
    fn hesia_watchdog_arm(watchdog: *mut HesiaWatchdog, error: *mut HesiaError) -> c_int;
    fn hesia_watchdog_heartbeat(watchdog: *mut HesiaWatchdog, error: *mut HesiaError) -> c_int;
    fn hesia_watchdog_check(watchdog: *mut HesiaWatchdog, error: *mut HesiaError) -> c_int;
    fn hesia_watchdog_disarm(watchdog: *mut HesiaWatchdog);
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
    pub status: i32,
    pub message: String,
}

fn cstr(ptr: *const c_char) -> String {
    if ptr.is_null() {
        return String::new();
    }
    unsafe { CStr::from_ptr(ptr).to_string_lossy().into_owned() }
}

fn check(status: i32, error: &HesiaError) -> Result<(), Error> {
    if status == HESIA_STATUS_OK {
        return Ok(());
    }
    let message = unsafe { cstr(hesia_error_message(error as *const HesiaError)) };
    Err(Error { status, message })
}

pub fn version() -> String {
    unsafe { cstr(hesia_version()) }
}

pub fn status_string(status: i32) -> String {
    unsafe { cstr(hesia_status_string(status)) }
}

pub fn feature_available(feature: i32) -> bool {
    unsafe { hesia_feature_available(feature) != 0 }
}

pub fn random_bytes(len: usize) -> Result<Vec<u8>, Error> {
    let mut out = vec![0u8; len];
    let mut error = HesiaError {
        status: 0,
        subsystem: [0; 32],
        message: [0; 192],
    };
    unsafe {
        hesia_error_init(&mut error);
        let status = hesia_random(out.as_mut_ptr(), out.len(), &mut error);
        check(status, &error)?;
    }
    Ok(out)
}

pub struct Watchdog {
    raw: HesiaWatchdog,
}

impl Watchdog {
    pub fn new(timeout_ms: u64, max_missed: u32) -> Result<Self, Error> {
        let mut raw = HesiaWatchdog {
            timeout_ms: 0,
            last_heartbeat_ms: 0,
            max_missed: 0,
            missed: 0,
            armed: 0,
        };
        let mut error = HesiaError {
            status: 0,
            subsystem: [0; 32],
            message: [0; 192],
        };
        unsafe {
            hesia_error_init(&mut error);
            let status = hesia_watchdog_init(&mut raw, timeout_ms, max_missed, &mut error);
            check(status, &error)?;
        }
        Ok(Self { raw })
    }

    pub fn arm(&mut self) -> Result<(), Error> {
        self.call(unsafe { hesia_watchdog_arm })
    }

    pub fn heartbeat(&mut self) -> Result<(), Error> {
        self.call(unsafe { hesia_watchdog_heartbeat })
    }

    pub fn check(&mut self) -> Result<(), Error> {
        self.call(unsafe { hesia_watchdog_check })
    }

    pub fn disarm(&mut self) {
        unsafe { hesia_watchdog_disarm(&mut self.raw) }
    }

    fn call(&mut self, function: unsafe extern "C" fn(*mut HesiaWatchdog, *mut HesiaError) -> c_int) -> Result<(), Error> {
        let mut error = HesiaError {
            status: 0,
            subsystem: [0; 32],
            message: [0; 192],
        };
        unsafe {
            hesia_error_init(&mut error);
            let status = function(&mut self.raw, &mut error);
            check(status, &error)
        }
    }
}
