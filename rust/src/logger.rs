//! This module implements logging facilities.
//!
//! # FFI
//!
//! [`stracciatella::c::logger`] contains a C interface for this module.
//!
//! [`stracciatella::c::logger`]: ../c/logger/index.html

use std::fs::File;
use std::path::Path;
use std::sync::atomic::{AtomicUsize, Ordering};

use log::{logger, set_boxed_logger, set_max_level, Level, LevelFilter, Log, Metadata, Record};
use simplelog::{CombinedLogger, Config, TermLogger, TerminalMode, WriteLogger};

static GLOBAL_LOG_LEVEL: AtomicUsize = AtomicUsize::new(LogLevel::Info as usize);

#[derive(Debug, PartialEq, Copy, Clone)]
#[repr(C)]
/// Enum to represent log levels in the application
pub enum LogLevel {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
    Trace = 4,
}

impl From<LogLevel> for Level {
    fn from(other: LogLevel) -> Level {
        match other {
            LogLevel::Debug => Level::Debug,
            LogLevel::Error => Level::Error,
            LogLevel::Info => Level::Info,
            LogLevel::Trace => Level::Trace,
            LogLevel::Warn => Level::Warn,
        }
    }
}

impl From<LogLevel> for usize {
    fn from(other: LogLevel) -> usize {
        other as usize
    }
}

impl From<usize> for LogLevel {
    fn from(other: usize) -> LogLevel {
        match other {
            0 => LogLevel::Error,
            1 => LogLevel::Warn,
            2 => LogLevel::Info,
            3 => LogLevel::Debug,
            4 => LogLevel::Trace,
            _ => panic!("Unexpected log level: {}", other),
        }
    }
}

/// Runtime level filter to filter messages based on a global variable
///
/// Other log levels should be set to max level in order for the filter
/// to work properly
struct RuntimeLevelFilter {
    logger: Box<CombinedLogger>,
}

impl RuntimeLevelFilter {
    fn init(logger: Box<CombinedLogger>) {
        let filter = RuntimeLevelFilter { logger };

        set_max_level(LevelFilter::max());
        set_boxed_logger(Box::new(filter)).unwrap();
    }

    fn get_global_log_level() -> Level {
        let current_level = GLOBAL_LOG_LEVEL.load(Ordering::Relaxed);
        LogLevel::from(current_level).into()
    }
}

impl Log for RuntimeLevelFilter {
    fn enabled(&self, metadata: &Metadata) -> bool {
        let current_level = Self::get_global_log_level();
        metadata.level() <= current_level
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            self.logger.log(record);
        }
    }

    fn flush(&self) {
        self.logger.flush()
    }
}

/// Convenience struct to group logging functionality
pub struct Logger;

impl Logger {
    /// Initializes the logging system
    ///
    /// Needs to be called once at start of the game engine. Any log messages send
    /// before will be discarded.
    pub fn init(log_file: &Path) {
        let mut config = Config::default();
        config.target = Some(Level::Error);
        config.thread = None;
        config.time_format = Some("%FT%T");
        let logger = CombinedLogger::new(vec![
            TermLogger::new(LevelFilter::max(), config, TerminalMode::Mixed).unwrap(),
            WriteLogger::new(LevelFilter::max(), config, File::create(log_file).unwrap()),
        ]);
        RuntimeLevelFilter::init(logger);
    }

    /// Sets the global log level to a specific value
    pub fn set_level(level: LogLevel) {
        GLOBAL_LOG_LEVEL.store(level.into(), Ordering::Relaxed);
    }

    /// Logs message with specific metadata
    ///
    /// Can be used e.g. in C++ or scripting
    pub fn log_with_custom_metadata(level: LogLevel, message: &str, target: &str) {
        let level = level.into();

        logger().log(
            &Record::builder()
                .level(level)
                .target(target)
                .args(format_args!("{}", message))
                .build(),
        );
    }
}
