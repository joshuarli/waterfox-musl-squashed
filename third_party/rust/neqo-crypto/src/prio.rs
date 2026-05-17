// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#![allow(
    dead_code,
    non_upper_case_globals,
    non_snake_case,
    clippy::cognitive_complexity,
    clippy::too_many_lines,
    clippy::used_underscore_binding,
    reason = "For included bindgen code."
)]

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct PRFileDesc {
    pub methods: *const PRIOMethods,
    pub secret: *mut PRFilePrivate,
    pub lower: *mut PRFileDesc,
    pub higher: *mut PRFileDesc,
    pub dtor: Option<unsafe extern "C" fn(fd: *mut PRFileDesc)>,
    pub identity: PRDescIdentity,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct PRNetAddrRaw {
    pub family: PRUint16,
    pub data: [::std::os::raw::c_char; 14],
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct PRNetAddrInet {
    pub family: PRUint16,
    pub port: PRUint16,
    pub ip: PRUint32,
    pub pad: [::std::os::raw::c_char; 8],
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct PRNetAddrIpv6 {
    pub family: PRUint16,
    pub port: PRUint16,
    pub flowinfo: PRUint32,
    pub ip: PRIPv6Addr,
    pub scope_id: PRUint32,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct PRNetAddrLocal {
    pub family: PRUint16,
    pub path: [::std::os::raw::c_char; 104],
}

#[repr(C)]
#[derive(Copy, Clone)]
pub union PRNetAddr {
    pub raw: PRNetAddrRaw,
    pub inet: PRNetAddrInet,
    pub ipv6: PRNetAddrIpv6,
    pub local: PRNetAddrLocal,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct PRIOMethods {
    pub file_type: PRDescType::Type,
    pub close: PRCloseFN,
    pub read: PRReadFN,
    pub write: PRWriteFN,
    pub available: PRAvailableFN,
    pub available64: PRAvailable64FN,
    pub fsync: PRFsyncFN,
    pub seek: PRSeekFN,
    pub seek64: PRSeek64FN,
    pub fileInfo: PRFileInfoFN,
    pub fileInfo64: PRFileInfo64FN,
    pub writev: PRWritevFN,
    pub connect: PRConnectFN,
    pub accept: PRAcceptFN,
    pub bind: PRBindFN,
    pub listen: PRListenFN,
    pub shutdown: PRShutdownFN,
    pub recv: PRRecvFN,
    pub send: PRSendFN,
    pub recvfrom: PRRecvfromFN,
    pub sendto: PRSendtoFN,
    pub poll: PRPollFN,
    pub acceptread: PRAcceptreadFN,
    pub transmitfile: PRTransmitfileFN,
    pub getsockname: PRGetsocknameFN,
    pub getpeername: PRGetpeernameFN,
    pub reserved_fn_6: PRReservedFN,
    pub reserved_fn_5: PRReservedFN,
    pub getsocketoption: PRGetsocketoptionFN,
    pub setsocketoption: PRSetsocketoptionFN,
    pub sendfile: PRSendfileFN,
    pub connectcontinue: PRConnectcontinueFN,
    pub reserved_fn_3: PRReservedFN,
    pub reserved_fn_2: PRReservedFN,
    pub reserved_fn_1: PRReservedFN,
    pub reserved_fn_0: PRReservedFN,
}

include!(concat!(env!("OUT_DIR"), "/nspr_io.rs"));

pub enum PRFileInfo {}
pub enum PRFileInfo64 {}
pub enum PRFilePrivate {}
pub enum PRIOVec {}
pub enum PRSendFileData {}
