#pragma once

#define WINAPI_RETURN_WITH_REPORT(cx, retValue, funcName) \
    do \
    {\
        DWORD errorCode = GetLastError();\
        pfc::string8_fast errorStr = mozjs::MessageFromErrorCode( errorCode );\
        JS_ReportErrorUTF8( cx, "WinAPI error: '%s' failed with error (0x%X): %s", #funcName, errorCode, errorStr.c_str() );\
        return retValue;\
    } while(false)

#define IF_WINAPI_FAILED_RETURN_WITH_REPORT(cx, successPredicate, retValue, funcName) \
    do \
    {\
        if ( !(successPredicate) )\
        {\
            WINAPI_RETURN_WITH_REPORT(cx, retValue, funcName);\
        }\
    } while(false)

#define IF_HR_FAILED_RETURN_WITH_REPORT(cx, hr, retValue, funcName) \
    do \
    {\
        int iRet_internal = mozjs::Win32FromHResult(hr);\
        if ( ERROR_SUCCESS != iRet_internal )\
        {\
            SetLastError(iRet_internal);\
            WINAPI_RETURN_WITH_REPORT(cx, retValue, funcName);\
        }\
    } while(false)

namespace mozjs
{

DWORD Win32FromHResult( HRESULT hr );
pfc::string8_fast MessageFromErrorCode( DWORD errorCode );

}