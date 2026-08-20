// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025-2026 rtldg <rtldg@protonmail.com>

#![allow(non_snake_case)]
// TODO: Bleh, static muts...
#![allow(static_mut_refs)]

use std::ffi::c_void;
use std::fs::File;
use std::io::Read;
use std::io::Seek;
use std::io::SeekFrom;
use std::io::Write;
use std::ptr::NonNull;
use std::sync::mpsc::Receiver;
use std::sync::mpsc::Sender;
use std::sync::mpsc::channel;
use std::thread::JoinHandle;

use extshared::ICellArray::ICellArray;
use extshared::ICellArray::ICellArray_at;
use extshared::cpp_add_frame_action;
use extshared::cpp_extension_log_error;
use extshared::cpp_forward_execute;
use extshared::cpp_forward_push_cell;
use extshared::cpp_forward_release;

extshared::smext_conf_boilerplate_extension_info!(description, version, author, datestring, url, logtag, license, load);
#[unsafe(no_mangle)]
pub extern "C" fn rust_conf_name() -> *const u8 {
	"srcwr💾\0".as_ptr()
}
extshared::smext_conf_boilerplate_load_funcs!();

static mut SENDER: Option<Sender<Msg>> = None;
static mut THREAD: Option<JoinHandle<()>> = None;
static mut LOAD_SENDER: Option<Sender<LoadMsg>> = None;
static mut LOAD_THREAD: Option<JoinHandle<()>> = None;

#[derive(Debug)]
struct Msg {
	forward:          NonNull<c_void>,
	value:            i32,
	friendly_paths:   Vec<String>,
	header:           Vec<u8>,
	playerrecording:  *const ICellArray,
	totalframes:      usize,
	// Optional footer (extra per-frame module data) appended after the frames.
	// `footer` empty or `extrarecording` null => no footer is written.
	footer:           Vec<u8>,
	extrarecording:   *const ICellArray,
	totalextraframes: usize,
}
unsafe impl Send for Msg {} // so we can store the pointers...

#[derive(Debug)]
struct LoadMsg {
	forward:         NonNull<c_void>,
	value:           i32,
	path:            String,
	// Pre-sized ArrayList backing stores to fill with raw bytes off-thread.
	frames:          *const ICellArray,
	frame_offset:    u64,
	frame_cellcount: usize,
	frame_count:     usize,
	// Optional footer extra frames. `extra` null or `extra_count` 0 => frames only.
	extra:           *const ICellArray,
	extra_offset:    u64,
	extra_cellcount: usize,
	extra_count:     usize,
}
unsafe impl Send for LoadMsg {} // so we can store the pointers...

struct Callbacker {
	forward: NonNull<c_void>,
	saved:   bool,
	value:   i32,
}
unsafe impl Send for Callbacker {} // so we can store the pointers...

#[unsafe(no_mangle)]
pub extern "C" fn rust_setup_replay_thread() {
	let (send, recv) = channel();
	let (load_send, load_recv) = channel();
	unsafe {
		SENDER = Some(send);
		THREAD = Some(
			std::thread::Builder::new()
				.name("srcwrfloppy replay thread".to_string())
				.spawn(move || replay_thread(recv))
				.unwrap(),
		);
		LOAD_SENDER = Some(load_send);
		LOAD_THREAD = Some(
			std::thread::Builder::new()
				.name("srcwrfloppy load thread".to_string())
				.spawn(move || load_thread(load_recv))
				.unwrap(),
		);
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_KILL_replay_thread() {
	unsafe {
		SENDER = None; // closes channel
		THREAD.take().unwrap().join().unwrap();
		LOAD_SENDER = None; // closes channel
		LOAD_THREAD.take().unwrap().join().unwrap();
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_post_to_replay_thread(
	forward: NonNull<c_void>,
	value: i32,
	pathsarray: &ICellArray,
	header: *const u8,
	headersize: usize,
	playerrecording: *const ICellArray,
	totalframes: usize,
) {
	// No footer: forward to the _ex variant with empty footer / null extra frames.
	rust_post_to_replay_thread_ex(
		forward,
		value,
		pathsarray,
		header,
		headersize,
		playerrecording,
		totalframes,
		std::ptr::null(),
		0,
		std::ptr::null(),
		0,
	);
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_post_to_replay_thread_ex(
	forward: NonNull<c_void>,
	value: i32,
	pathsarray: &ICellArray,
	header: *const u8,
	headersize: usize,
	playerrecording: *const ICellArray,
	totalframes: usize,
	footer: *const u8,
	footersize: usize,
	extrarecording: *const ICellArray,
	totalextraframes: usize,
) {
	let mut pathsvec = vec![];

	unsafe {
		let len = pathsarray.size;
		for i in 0..len {
			let path = extshared::build_path(ICellArray_at(pathsarray, i) as *const u8, extshared::PathType::Path_Game);
			if !pathsvec.contains(&path) {
				pathsvec.push(path);
			}
		}
	}

	let header = unsafe { std::slice::from_raw_parts(header, headersize).to_vec() };

	let footer = if footer.is_null() || footersize == 0 {
		vec![]
	} else {
		unsafe { std::slice::from_raw_parts(footer, footersize).to_vec() }
	};

	//println!("hello from poster!");
	unsafe {
		if let Some(sender) = &SENDER {
			sender
				.send(Msg {
					forward,
					value,
					friendly_paths: pathsvec,
					header,
					playerrecording,
					totalframes,
					footer,
					extrarecording,
					totalextraframes,
				})
				.unwrap();
			//println!("posted!");
		}
	}
}

fn replay_thread(recv: Receiver<Msg>) {
	while let Ok(msg) = recv.recv() {
		//println!("received {msg:?}");

		let mut writers = vec![];

		for path in &msg.friendly_paths {
			let tmp = path.clone() + ".tmp";
			if let Ok(f) = std::fs::File::create(&tmp).map(std::io::BufWriter::new) {
				writers.push((path, tmp, f));
			} else {
				log_error(format!("Failed to open '{tmp}' replay file for writing."));
			}
		}

		let saved = !writers.is_empty();

		if saved {
			let cellarray = unsafe { &*msg.playerrecording };
			let frames = unsafe {
				std::slice::from_raw_parts(
					cellarray.data as *const u8,
					cellarray.blocksize * size_of::<i32>() * msg.totalframes,
				)
			};

			// Optional footer: the opaque footer bytes (built SP-side) followed by
			// the raw extra-frame blob, streamed the same way as `frames`.
			let has_footer =
				!msg.footer.is_empty() && !msg.extrarecording.is_null() && msg.totalextraframes > 0;
			let extraframes = has_footer.then(|| unsafe {
				let extra = &*msg.extrarecording;
				std::slice::from_raw_parts(
					extra.data as *const u8,
					extra.blocksize * size_of::<i32>() * msg.totalextraframes,
				)
			});

			for (_, _, f) in writers.iter_mut() {
				let _ = f.write_all(&msg.header);
				let _ = f.write_all(frames);
				if let Some(extraframes) = extraframes {
					let _ = f.write_all(&msg.footer);
					let _ = f.write_all(extraframes);
				}
			}

			for (p, t, f) in writers {
				// BufWriter::into_inner() will flush the buffer.
				if let Ok(f) = f.into_inner() {
					// ignoring errors like a boss...
					let _ = f.sync_all();
					drop(f);
					let _ = std::fs::rename(t, p);
				}
			}
		}

		unsafe {
			cpp_add_frame_action(
				do_callback,
				Box::leak(Box::new(Callbacker {
					forward: msg.forward,
					saved,
					value: msg.value,
				})) as *mut _ as *mut c_void,
			);
		}
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_post_load_to_replay_thread(
	forward: NonNull<c_void>,
	value: i32,
	path: *const u8,
	frames: *const ICellArray,
	frame_offset: i32,
	frame_cellcount: i32,
	frame_count: i32,
	extra: *const ICellArray,
	extra_offset: i32,
	extra_cellcount: i32,
	extra_count: i32,
) {
	// Resolve relative to the game dir, exactly like the save path does for its paths.
	let path = extshared::build_path(path, extshared::PathType::Path_Game);

	unsafe {
		if let Some(sender) = &LOAD_SENDER {
			sender
				.send(LoadMsg {
					forward,
					value,
					path,
					frames,
					frame_offset: frame_offset as u64,
					frame_cellcount: frame_cellcount as usize,
					frame_count: frame_count as usize,
					extra,
					extra_offset: extra_offset as u64,
					extra_cellcount: extra_cellcount as usize,
					extra_count: extra_count as usize,
				})
				.unwrap();
		}
	}
}

// Reads `count` frames of `cellcount` cells each from `offset` straight into the
// ArrayList's pre-allocated backing store. Capped at the array's actual size so a
// bogus count can never write past the allocation (load writes memory, unlike save).
fn read_into(f: &mut File, arr: *const ICellArray, offset: u64, cellcount: usize, count: usize) -> std::io::Result<()> {
	let a = unsafe { &*arr };
	let n = count.min(a.size);
	let len = cellcount.min(a.blocksize) * size_of::<i32>() * n;
	f.seek(SeekFrom::Start(offset))?;
	let buf = unsafe { std::slice::from_raw_parts_mut(a.data as *mut u8, len) };
	f.read_exact(buf)
}

fn load_thread(recv: Receiver<LoadMsg>) {
	while let Ok(msg) = recv.recv() {
		let mut loaded = false;

		match File::open(&msg.path) {
			Ok(mut f) => {
				if read_into(&mut f, msg.frames, msg.frame_offset, msg.frame_cellcount, msg.frame_count).is_ok() {
					loaded = if msg.extra.is_null() || msg.extra_count == 0 {
						true // frames-only load (no footer / vanilla replay)
					} else {
						read_into(&mut f, msg.extra, msg.extra_offset, msg.extra_cellcount, msg.extra_count).is_ok()
					};
				}
			}
			Err(_) => log_error(format!("Failed to open '{}' replay file for loading.", msg.path)),
		}

		unsafe {
			cpp_add_frame_action(
				do_callback,
				Box::leak(Box::new(Callbacker {
					forward: msg.forward,
					saved: loaded, // reuses the (saved, value) callback shape as (loaded, value)
					value: msg.value,
				})) as *mut _ as *mut c_void,
			);
		}
	}
}

unsafe extern "C" fn do_callback(data: *mut c_void) {
	unsafe {
		let data = Box::from_raw(data as *mut Callbacker);
		cpp_forward_push_cell(data.forward, data.saved as i32);
		//println!("data.value: {:x}", data.value);
		cpp_forward_push_cell(data.forward, data.value);
		cpp_forward_execute(data.forward, &mut 0);
		cpp_forward_release(data.forward);
	}
}

unsafe extern "C" fn log_error_frame_action(error: *mut c_void) {
	unsafe {
		let mut error = Box::from_raw(error as *mut String);
		error.push('\0');
		cpp_extension_log_error(error.as_ptr());
	}
}

fn log_error(error: String) {
	unsafe {
		cpp_add_frame_action(log_error_frame_action, Box::leak(Box::new(error)) as *mut _ as *mut c_void);
	}
}
