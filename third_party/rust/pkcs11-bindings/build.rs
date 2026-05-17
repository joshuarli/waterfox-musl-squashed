/* -*- Mode: rust; rust-indent-offset: 4 -*- */

extern crate bindgen;

use bindgen::callbacks::*;
use bindgen::*;

use std::env;
use std::path::PathBuf;

#[derive(Debug)]
struct PKCS11TypesParseCallbacks;

impl ParseCallbacks for PKCS11TypesParseCallbacks {
    fn int_macro(&self, name: &str, _value: i64) -> Option<IntKind> {
        if name == "CK_TRUE" || name == "CK_FALSE" {
            Some(IntKind::U8)
        } else {
            Some(IntKind::ULong)
        }
    }
}

const PKCS11_COMPAT_TYPES: &str = r#"
pub type CK_BYTE = ::std::os::raw::c_uchar;
pub type CK_CHAR = CK_BYTE;
pub type CK_UTF8CHAR = CK_BYTE;
pub type CK_BBOOL = CK_BYTE;
pub type CK_ULONG = ::std::os::raw::c_ulong;
pub type CK_LONG = ::std::os::raw::c_long;
pub type CK_FLAGS = CK_ULONG;
pub type CK_BYTE_PTR = *mut CK_BYTE;
pub type CK_CHAR_PTR = *mut CK_CHAR;
pub type CK_UTF8CHAR_PTR = *mut CK_UTF8CHAR;
pub type CK_ULONG_PTR = *mut CK_ULONG;
pub type CK_VOID_PTR = *mut ::std::os::raw::c_void;
pub type CK_VOID_PTR_PTR = *mut CK_VOID_PTR;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_VERSION {
    pub major: CK_BYTE,
    pub minor: CK_BYTE,
}
pub type CK_VERSION_PTR = *mut CK_VERSION;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_INFO {
    pub cryptokiVersion: CK_VERSION,
    pub manufacturerID: [CK_UTF8CHAR; 32usize],
    pub flags: CK_FLAGS,
    pub libraryDescription: [CK_UTF8CHAR; 32usize],
    pub libraryVersion: CK_VERSION,
}
pub type CK_INFO_PTR = *mut CK_INFO;

pub type CK_NOTIFICATION = CK_ULONG;
pub type CK_SLOT_ID = CK_ULONG;
pub type CK_SLOT_ID_PTR = *mut CK_SLOT_ID;

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct CK_SLOT_INFO {
    pub slotDescription: [CK_UTF8CHAR; 64usize],
    pub manufacturerID: [CK_UTF8CHAR; 32usize],
    pub flags: CK_FLAGS,
    pub hardwareVersion: CK_VERSION,
    pub firmwareVersion: CK_VERSION,
}
impl Default for CK_SLOT_INFO {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type CK_SLOT_INFO_PTR = *mut CK_SLOT_INFO;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_TOKEN_INFO {
    pub label: [CK_UTF8CHAR; 32usize],
    pub manufacturerID: [CK_UTF8CHAR; 32usize],
    pub model: [CK_UTF8CHAR; 16usize],
    pub serialNumber: [CK_CHAR; 16usize],
    pub flags: CK_FLAGS,
    pub ulMaxSessionCount: CK_ULONG,
    pub ulSessionCount: CK_ULONG,
    pub ulMaxRwSessionCount: CK_ULONG,
    pub ulRwSessionCount: CK_ULONG,
    pub ulMaxPinLen: CK_ULONG,
    pub ulMinPinLen: CK_ULONG,
    pub ulTotalPublicMemory: CK_ULONG,
    pub ulFreePublicMemory: CK_ULONG,
    pub ulTotalPrivateMemory: CK_ULONG,
    pub ulFreePrivateMemory: CK_ULONG,
    pub hardwareVersion: CK_VERSION,
    pub firmwareVersion: CK_VERSION,
    pub utcTime: [CK_CHAR; 16usize],
}
pub type CK_TOKEN_INFO_PTR = *mut CK_TOKEN_INFO;

pub type CK_SESSION_HANDLE = CK_ULONG;
pub type CK_SESSION_HANDLE_PTR = *mut CK_SESSION_HANDLE;
pub type CK_USER_TYPE = CK_ULONG;
pub type CK_STATE = CK_ULONG;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_SESSION_INFO {
    pub slotID: CK_SLOT_ID,
    pub state: CK_STATE,
    pub flags: CK_FLAGS,
    pub ulDeviceError: CK_ULONG,
}
pub type CK_SESSION_INFO_PTR = *mut CK_SESSION_INFO;

pub type CK_OBJECT_HANDLE = CK_ULONG;
pub type CK_OBJECT_HANDLE_PTR = *mut CK_OBJECT_HANDLE;
pub type CK_OBJECT_CLASS = CK_ULONG;
pub type CK_OBJECT_CLASS_PTR = *mut CK_OBJECT_CLASS;
pub type CK_HW_FEATURE_TYPE = CK_ULONG;
pub type CK_KEY_TYPE = CK_ULONG;
pub type CK_CERTIFICATE_TYPE = CK_ULONG;
pub type CK_ATTRIBUTE_TYPE = CK_ULONG;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_ATTRIBUTE {
    pub type_: CK_ATTRIBUTE_TYPE,
    pub pValue: CK_VOID_PTR,
    pub ulValueLen: CK_ULONG,
}
pub type CK_ATTRIBUTE_PTR = *mut CK_ATTRIBUTE;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_DATE {
    pub year: [CK_CHAR; 4usize],
    pub month: [CK_CHAR; 2usize],
    pub day: [CK_CHAR; 2usize],
}

pub type CK_MECHANISM_TYPE = CK_ULONG;
pub type CK_MECHANISM_TYPE_PTR = *mut CK_MECHANISM_TYPE;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_MECHANISM {
    pub mechanism: CK_MECHANISM_TYPE,
    pub pParameter: CK_VOID_PTR,
    pub ulParameterLen: CK_ULONG,
}
pub type CK_MECHANISM_PTR = *mut CK_MECHANISM;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_MECHANISM_INFO {
    pub ulMinKeySize: CK_ULONG,
    pub ulMaxKeySize: CK_ULONG,
    pub flags: CK_FLAGS,
}
pub type CK_MECHANISM_INFO_PTR = *mut CK_MECHANISM_INFO;

pub type CK_RV = CK_ULONG;
pub type CK_NOTIFY = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_NOTIFICATION, CK_VOID_PTR) -> CK_RV>;

pub type CK_CREATEMUTEX = ::std::option::Option<unsafe extern "C" fn(CK_VOID_PTR_PTR) -> CK_RV>;
pub type CK_DESTROYMUTEX = ::std::option::Option<unsafe extern "C" fn(CK_VOID_PTR) -> CK_RV>;
pub type CK_LOCKMUTEX = ::std::option::Option<unsafe extern "C" fn(CK_VOID_PTR) -> CK_RV>;
pub type CK_UNLOCKMUTEX = ::std::option::Option<unsafe extern "C" fn(CK_VOID_PTR) -> CK_RV>;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_C_INITIALIZE_ARGS {
    pub CreateMutex: CK_CREATEMUTEX,
    pub DestroyMutex: CK_DESTROYMUTEX,
    pub LockMutex: CK_LOCKMUTEX,
    pub UnlockMutex: CK_UNLOCKMUTEX,
    pub flags: CK_FLAGS,
    pub pReserved: CK_VOID_PTR,
}
pub type CK_C_INITIALIZE_ARGS_PTR = *mut CK_C_INITIALIZE_ARGS;

pub type CK_RSA_PKCS_MGF_TYPE = CK_ULONG;
pub type CK_RSA_PKCS_MGF_TYPE_PTR = *mut CK_RSA_PKCS_MGF_TYPE;
pub type CK_RSA_PKCS_OAEP_SOURCE_TYPE = CK_ULONG;
pub type CK_RSA_PKCS_OAEP_SOURCE_TYPE_PTR = *mut CK_RSA_PKCS_OAEP_SOURCE_TYPE;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_RSA_PKCS_PSS_PARAMS {
    pub hashAlg: CK_MECHANISM_TYPE,
    pub mgf: CK_RSA_PKCS_MGF_TYPE,
    pub sLen: CK_ULONG,
}
pub type CK_RSA_PKCS_PSS_PARAMS_PTR = *mut CK_RSA_PKCS_PSS_PARAMS;

pub type CK_FUNCTION_LIST_PTR = *mut CK_FUNCTION_LIST;
pub type CK_FUNCTION_LIST_PTR_PTR = *mut CK_FUNCTION_LIST_PTR;

pub type CK_C_Initialize = ::std::option::Option<unsafe extern "C" fn(CK_VOID_PTR) -> CK_RV>;
pub type CK_C_Finalize = ::std::option::Option<unsafe extern "C" fn(CK_VOID_PTR) -> CK_RV>;
pub type CK_C_GetInfo = ::std::option::Option<unsafe extern "C" fn(CK_INFO_PTR) -> CK_RV>;
pub type CK_C_GetFunctionList = ::std::option::Option<unsafe extern "C" fn(CK_FUNCTION_LIST_PTR_PTR) -> CK_RV>;
pub type CK_C_GetSlotList = ::std::option::Option<unsafe extern "C" fn(CK_BBOOL, CK_SLOT_ID_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_GetSlotInfo = ::std::option::Option<unsafe extern "C" fn(CK_SLOT_ID, CK_SLOT_INFO_PTR) -> CK_RV>;
pub type CK_C_GetTokenInfo = ::std::option::Option<unsafe extern "C" fn(CK_SLOT_ID, CK_TOKEN_INFO_PTR) -> CK_RV>;
pub type CK_C_GetMechanismList = ::std::option::Option<unsafe extern "C" fn(CK_SLOT_ID, CK_MECHANISM_TYPE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_GetMechanismInfo = ::std::option::Option<unsafe extern "C" fn(CK_SLOT_ID, CK_MECHANISM_TYPE, CK_MECHANISM_INFO_PTR) -> CK_RV>;
pub type CK_C_InitToken = ::std::option::Option<unsafe extern "C" fn(CK_SLOT_ID, CK_UTF8CHAR_PTR, CK_ULONG, CK_UTF8CHAR_PTR) -> CK_RV>;
pub type CK_C_InitPIN = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_UTF8CHAR_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_SetPIN = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_UTF8CHAR_PTR, CK_ULONG, CK_UTF8CHAR_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_OpenSession = ::std::option::Option<unsafe extern "C" fn(CK_SLOT_ID, CK_FLAGS, CK_VOID_PTR, CK_NOTIFY, CK_SESSION_HANDLE_PTR) -> CK_RV>;
pub type CK_C_CloseSession = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE) -> CK_RV>;
pub type CK_C_CloseAllSessions = ::std::option::Option<unsafe extern "C" fn(CK_SLOT_ID) -> CK_RV>;
pub type CK_C_GetSessionInfo = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_SESSION_INFO_PTR) -> CK_RV>;
pub type CK_C_GetOperationState = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_SetOperationState = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_OBJECT_HANDLE, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_Login = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_USER_TYPE, CK_UTF8CHAR_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_Logout = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE) -> CK_RV>;
pub type CK_C_CreateObject = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR) -> CK_RV>;
pub type CK_C_CopyObject = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR) -> CK_RV>;
pub type CK_C_DestroyObject = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_GetObjectSize = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_GetAttributeValue = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_SetAttributeValue = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_FindObjectsInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_FindObjects = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_OBJECT_HANDLE_PTR, CK_ULONG, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_FindObjectsFinal = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE) -> CK_RV>;
pub type CK_C_EncryptInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_Encrypt = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_EncryptUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_EncryptFinal = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DecryptInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_Decrypt = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DecryptUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DecryptFinal = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DigestInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR) -> CK_RV>;
pub type CK_C_Digest = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DigestUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_DigestKey = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_DigestFinal = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_SignInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_Sign = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_SignUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_SignFinal = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_SignRecoverInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_SignRecover = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_VerifyInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_Verify = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_VerifyUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_VerifyFinal = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_VerifyRecoverInit = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE) -> CK_RV>;
pub type CK_C_VerifyRecover = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DigestEncryptUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DecryptDigestUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_SignEncryptUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_DecryptVerifyUpdate = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_GenerateKey = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR) -> CK_RV>;
pub type CK_C_GenerateKeyPair = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_ATTRIBUTE_PTR, CK_ULONG, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR, CK_OBJECT_HANDLE_PTR) -> CK_RV>;
pub type CK_C_WrapKey = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE, CK_OBJECT_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR) -> CK_RV>;
pub type CK_C_UnwrapKey = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR) -> CK_RV>;
pub type CK_C_DeriveKey = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR) -> CK_RV>;
pub type CK_C_SeedRandom = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_GenerateRandom = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG) -> CK_RV>;
pub type CK_C_GetFunctionStatus = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE) -> CK_RV>;
pub type CK_C_CancelFunction = ::std::option::Option<unsafe extern "C" fn(CK_SESSION_HANDLE) -> CK_RV>;
pub type CK_C_WaitForSlotEvent = ::std::option::Option<unsafe extern "C" fn(CK_FLAGS, CK_SLOT_ID_PTR, CK_VOID_PTR) -> CK_RV>;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct CK_FUNCTION_LIST {
    pub version: CK_VERSION,
    pub C_Initialize: CK_C_Initialize,
    pub C_Finalize: CK_C_Finalize,
    pub C_GetInfo: CK_C_GetInfo,
    pub C_GetFunctionList: CK_C_GetFunctionList,
    pub C_GetSlotList: CK_C_GetSlotList,
    pub C_GetSlotInfo: CK_C_GetSlotInfo,
    pub C_GetTokenInfo: CK_C_GetTokenInfo,
    pub C_GetMechanismList: CK_C_GetMechanismList,
    pub C_GetMechanismInfo: CK_C_GetMechanismInfo,
    pub C_InitToken: CK_C_InitToken,
    pub C_InitPIN: CK_C_InitPIN,
    pub C_SetPIN: CK_C_SetPIN,
    pub C_OpenSession: CK_C_OpenSession,
    pub C_CloseSession: CK_C_CloseSession,
    pub C_CloseAllSessions: CK_C_CloseAllSessions,
    pub C_GetSessionInfo: CK_C_GetSessionInfo,
    pub C_GetOperationState: CK_C_GetOperationState,
    pub C_SetOperationState: CK_C_SetOperationState,
    pub C_Login: CK_C_Login,
    pub C_Logout: CK_C_Logout,
    pub C_CreateObject: CK_C_CreateObject,
    pub C_CopyObject: CK_C_CopyObject,
    pub C_DestroyObject: CK_C_DestroyObject,
    pub C_GetObjectSize: CK_C_GetObjectSize,
    pub C_GetAttributeValue: CK_C_GetAttributeValue,
    pub C_SetAttributeValue: CK_C_SetAttributeValue,
    pub C_FindObjectsInit: CK_C_FindObjectsInit,
    pub C_FindObjects: CK_C_FindObjects,
    pub C_FindObjectsFinal: CK_C_FindObjectsFinal,
    pub C_EncryptInit: CK_C_EncryptInit,
    pub C_Encrypt: CK_C_Encrypt,
    pub C_EncryptUpdate: CK_C_EncryptUpdate,
    pub C_EncryptFinal: CK_C_EncryptFinal,
    pub C_DecryptInit: CK_C_DecryptInit,
    pub C_Decrypt: CK_C_Decrypt,
    pub C_DecryptUpdate: CK_C_DecryptUpdate,
    pub C_DecryptFinal: CK_C_DecryptFinal,
    pub C_DigestInit: CK_C_DigestInit,
    pub C_Digest: CK_C_Digest,
    pub C_DigestUpdate: CK_C_DigestUpdate,
    pub C_DigestKey: CK_C_DigestKey,
    pub C_DigestFinal: CK_C_DigestFinal,
    pub C_SignInit: CK_C_SignInit,
    pub C_Sign: CK_C_Sign,
    pub C_SignUpdate: CK_C_SignUpdate,
    pub C_SignFinal: CK_C_SignFinal,
    pub C_SignRecoverInit: CK_C_SignRecoverInit,
    pub C_SignRecover: CK_C_SignRecover,
    pub C_VerifyInit: CK_C_VerifyInit,
    pub C_Verify: CK_C_Verify,
    pub C_VerifyUpdate: CK_C_VerifyUpdate,
    pub C_VerifyFinal: CK_C_VerifyFinal,
    pub C_VerifyRecoverInit: CK_C_VerifyRecoverInit,
    pub C_VerifyRecover: CK_C_VerifyRecover,
    pub C_DigestEncryptUpdate: CK_C_DigestEncryptUpdate,
    pub C_DecryptDigestUpdate: CK_C_DecryptDigestUpdate,
    pub C_SignEncryptUpdate: CK_C_SignEncryptUpdate,
    pub C_DecryptVerifyUpdate: CK_C_DecryptVerifyUpdate,
    pub C_GenerateKey: CK_C_GenerateKey,
    pub C_GenerateKeyPair: CK_C_GenerateKeyPair,
    pub C_WrapKey: CK_C_WrapKey,
    pub C_UnwrapKey: CK_C_UnwrapKey,
    pub C_DeriveKey: CK_C_DeriveKey,
    pub C_SeedRandom: CK_C_SeedRandom,
    pub C_GenerateRandom: CK_C_GenerateRandom,
    pub C_GetFunctionStatus: CK_C_GetFunctionStatus,
    pub C_CancelFunction: CK_C_CancelFunction,
    pub C_WaitForSlotEvent: CK_C_WaitForSlotEvent,
}

extern "C" {
    pub fn C_GetFunctionList(ppFunctionList: CK_FUNCTION_LIST_PTR_PTR) -> CK_RV;
}
"#;

fn main() {
    println!("cargo:rerun-if-changed=wrapper.h");

    let bindings = Builder::default()
        .header("wrapper.h")
        .allowlist_var("CK.*")
        .blocklist_type("CK.*")
        .raw_line(PKCS11_COMPAT_TYPES)
        .derive_default(true)
        .parse_callbacks(Box::new(PKCS11TypesParseCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR should be set in env"));
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
