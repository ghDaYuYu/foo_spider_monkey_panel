#include <stdafx.h>
#include "ui_html.h"

#include <js_utils/scope_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_property_helper.h>
#include <com_objects/scriptable_web_browser.h>
#include <com_objects/dispatch_ptr.h>
#include <convert/js_to_native.h>
#include <convert/com.h>

#include <helpers.h>

#pragma warning( push )
#pragma warning( disable : 4192 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4278 )
#import <mshtml.tlb>
#pragma warning( pop )

namespace smp::ui
{
using namespace mozjs;

CDialogHtml::CDialogHtml( JSContext* cx, const std::wstring& htmlCodeOrPath, JS::HandleValue options )
    : pJsCtx_( cx )
    , htmlCodeOrPath_( htmlCodeOrPath )
{
    isInitSuccess = ParseOptions( options );
}

CDialogHtml::~CDialogHtml()
{
    if ( hIcon_ )
    {
        DestroyIcon( hIcon_ );
    }
}

LRESULT CDialogHtml::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    scope::final_action autoExit( [&] {
        EndDialog( -1 );
    } );

    if ( !isInitSuccess )
    { // report in ctor
        return -1;
    }

    SetOptions();

    CAxWindow wndIE = GetDlgItem( IDC_IE );
    IObjectWithSitePtr pOWS = nullptr;
    HRESULT hr = wndIE.QueryHost( IID_IObjectWithSite, (void**)&pOWS );
    IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, QueryHost );

    hr = pOWS->SetSite( static_cast<IServiceProvider*>( this ) );
    IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, SetSite );

    try
    {
        IWebBrowserPtr pBrowser;
        hr = wndIE.QueryControl( &pBrowser );
        IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, QueryControl );

        _variant_t v;
        hr = pBrowser->Navigate( _bstr_t( L"about:blank" ), &v, &v, &v, &v ); ///< Document object is only available after Navigate
        IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, Navigate );

        IDispatchPtr pDocDispatch;
        hr = pBrowser->get_Document( &pDocDispatch );
        IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, get_Document );

        MSHTML::IHTMLDocument2Ptr pDocument = pDocDispatch;

        {
            // Request default handler from MSHTML client site
            IOleObjectPtr pOleObject( pDocument );
            IOleClientSitePtr pClientSite;
            hr = pOleObject->GetClientSite( &pClientSite );
            IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, GetClientSite );

            pDefaultUiHandler_ = pClientSite;

            // Set the new custom IDocHostUIHandler
            ICustomDocPtr pCustomDoc( pDocument );
            hr = pCustomDoc->SetUIHandler( this );
            IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, SetUIHandler );
        }

        if ( const std::wstring filePrefix = L"file://";
             htmlCodeOrPath_.length() > filePrefix.length()
             && !wmemcmp( htmlCodeOrPath_.c_str(), filePrefix.c_str(), filePrefix.length() ) )
        {
            hr = pBrowser->Navigate( _bstr_t( htmlCodeOrPath_.c_str() ), &v, &v, &v, &v );
            IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, Navigate );
        }
        else
        {
            SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
            scope::final_action autoPsa( [pSaStrings]() {
                SafeArrayDestroy( pSaStrings );
            } );

            VARIANT* pSaVar = nullptr;
            hr = SafeArrayAccessData( pSaStrings, (LPVOID*)&pSaVar );
            IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, SafeArrayAccessData );

            _bstr_t bstr( htmlCodeOrPath_.c_str() );
            pSaVar->vt = VT_BSTR;
            pSaVar->bstrVal = bstr.Detach();
            hr = SafeArrayUnaccessData( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, SafeArrayUnaccessData );

            hr = pDocument->write( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, write );

            hr = pDocument->close();
            IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, -1, close );
        }
    }
    catch ( const _com_error& e )
    {
        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( e.Source().length() ? (const wchar_t*)e.Source() : L"" );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( e.Description().length() ? (const wchar_t*)e.Description() : L"" );
        JS_ReportErrorUTF8( pJsCtx_, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
        return -1;
    }

    autoExit.cancel();
    return TRUE; // set focus to default control
}

LRESULT CDialogHtml::OnSize( UINT nType, CSize size )
{
    switch ( nType )
    {
    case SIZE_MAXIMIZED:
    case SIZE_RESTORED:
    {
        CAxWindow wndIE = GetDlgItem( IDC_IE );
        wndIE.ResizeClient( size.cx, size.cy );
        break;
    }
    default:
        break;
    }

    return 0;
}

LRESULT CDialogHtml::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    EndDialog( wID );
    return 0;
}

void CDialogHtml::OnBeforeNavigate2( IDispatch* pDisp, VARIANT* URL, VARIANT* Flags,
                                     VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers,
                                     VARIANT_BOOL* Cancel )
{
    if ( !Cancel || !URL )
    {
        return;
    }

    *Cancel = VARIANT_FALSE;

    try
    {
        _bstr_t url_b( *URL );
        for ( const auto& urlPrefix : std::initializer_list<std::wstring>{ L"http://", L"https://" } )
        {
            if ( url_b.length() > urlPrefix.length()
                 && !wmemcmp( url_b.GetBSTR(), urlPrefix.c_str(), urlPrefix.length() ) )
            {
                if ( Cancel )
                {
                    *Cancel = VARIANT_TRUE;
                    return;
                }
            }
        }
    }
    catch ( const _com_error& )
    {
    }
}

void CDialogHtml::OnTitleChange( BSTR title )
{
    try
    {
        SetWindowText( static_cast<wchar_t*>( static_cast<_bstr_t>( title ) ) );
    }
    catch ( const _com_error& )
    {
    }
}

void __stdcall CDialogHtml::OnWindowClosing( VARIANT_BOOL bIsChildWindow, VARIANT_BOOL* Cancel )
{
    EndDialog( IDOK );
    if ( Cancel )
    {
        *Cancel = VARIANT_TRUE;
        return;
    }
}

STDMETHODIMP CDialogHtml::moveTo( LONG x, LONG y )
{
    if ( RECT rect; GetWindowRect( &rect ) )
    {
        MoveWindow( x, y, ( rect.right - rect.left ) + x, ( rect.bottom - rect.top ) + y );
    }
    return S_OK;
}

STDMETHODIMP CDialogHtml::moveBy( LONG x, LONG y )
{
    if ( RECT rect; GetWindowRect( &rect ) )
    {
        MoveWindow( rect.left + x, rect.top + y, rect.right + x, rect.bottom + y );
    }
    return S_OK;
}

STDMETHODIMP CDialogHtml::resizeTo( LONG x, LONG y )
{
    if ( RECT windowRect, clientRect; GetWindowRect( &windowRect ) && GetClientRect( &clientRect ) )
    {
        const LONG clientW = x - ( ( windowRect.right - windowRect.left ) - clientRect.right );
        const LONG clientH = y - ( ( windowRect.bottom - windowRect.top ) - clientRect.bottom );
        ResizeClient( clientW, clientH );
    }
    return S_OK;
}

STDMETHODIMP CDialogHtml::resizeBy( LONG x, LONG y )
{
    if ( RECT clientRect; GetClientRect( &clientRect ) )
    {
        const LONG clientW = x + clientRect.right;
        const LONG clientH = y + clientRect.bottom;
        ResizeClient( clientW, clientH );
    }

    return S_OK;
}

STDMETHODIMP CDialogHtml::ShowContextMenu( DWORD dwID, POINT* ppt, IUnknown* pcmdtReserved, IDispatch* pdispReserved )
{
    if ( !pDefaultUiHandler_ || !isContextMenuEnabled_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->ShowContextMenu( dwID, ppt, pcmdtReserved, pdispReserved );
}

STDMETHODIMP CDialogHtml::GetHostInfo( DOCHOSTUIINFO* pInfo )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    HRESULT hr = pDefaultUiHandler_->GetHostInfo( pInfo );
    if ( SUCCEEDED( hr ) && pInfo )
    {
        if ( !isFormSelectionEnabled_ )
        {
            pInfo->dwFlags |= DOCHOSTUIFLAG_DIALOG;
        }
        if ( !isScrollEnabled_ )
        {
            pInfo->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;
        }
    }

    return hr;
}

STDMETHODIMP CDialogHtml::ShowUI( DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->ShowUI( dwID, pActiveObject, pCommandTarget, pFrame, pDoc );
}

STDMETHODIMP CDialogHtml::HideUI( void )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->HideUI();
}

STDMETHODIMP CDialogHtml::UpdateUI( void )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->UpdateUI();
}

STDMETHODIMP CDialogHtml::EnableModeless( BOOL fEnable )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->EnableModeless( fEnable );
}

STDMETHODIMP CDialogHtml::OnDocWindowActivate( BOOL fActivate )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->OnDocWindowActivate( fActivate );
}

STDMETHODIMP CDialogHtml::OnFrameWindowActivate( BOOL fActivate )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->OnFrameWindowActivate( fActivate );
}

STDMETHODIMP CDialogHtml::ResizeBorder( LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fRameWindow )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->ResizeBorder( prcBorder, pUIWindow, fRameWindow );
}

STDMETHODIMP CDialogHtml::TranslateAccelerator( LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->TranslateAccelerator( lpMsg, pguidCmdGroup, nCmdID );
}

STDMETHODIMP CDialogHtml::GetOptionKeyPath( LPOLESTR* pchKey, DWORD dw )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->GetOptionKeyPath( pchKey, dw );
}

STDMETHODIMP CDialogHtml::GetDropTarget( IDropTarget* pDropTarget, IDropTarget** ppDropTarget )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->GetDropTarget( pDropTarget, ppDropTarget );
}

STDMETHODIMP CDialogHtml::GetExternal( IDispatch** ppDispatch )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    if ( ppDispatch && pExternal_ )
    {
        pExternal_.AddRef();
        *ppDispatch = pExternal_;
        return S_OK;
    }

    return pDefaultUiHandler_->GetExternal( ppDispatch );
}

STDMETHODIMP CDialogHtml::TranslateUrl( DWORD dwTranslate, LPWSTR pchURLIn, LPWSTR* ppchURLOut )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->TranslateUrl( dwTranslate, pchURLIn, ppchURLOut );
}

STDMETHODIMP CDialogHtml::FilterDataObject( IDataObject* pDO, IDataObject** ppDORet )
{
    if ( !pDefaultUiHandler_ )
    {
        return S_OK;
    }

    return pDefaultUiHandler_->FilterDataObject( pDO, ppDORet );
}

ULONG STDMETHODCALLTYPE CDialogHtml::AddRef( void )
{
    return 0;
}

ULONG STDMETHODCALLTYPE CDialogHtml::Release( void )
{
    return 0;
}

bool CDialogHtml::ParseOptions( JS::HandleValue options )
{
    assert( pJsCtx_ );

    if ( options.isNullOrUndefined() )
    {
        return true;
    }

    if ( !options.isObject() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "options argument is not an object" );
        return false;
    }

    JS::RootedObject jsObject( pJsCtx_, &options.toObject() );
    bool hasFailed = false; // TODO: replace with exception

    width_ = GetOptionalProperty<uint32_t>( pJsCtx_, jsObject, "width", hasFailed );
    if ( hasFailed )
    { // reports
        return false;
    }

    height_ = GetOptionalProperty<uint32_t>( pJsCtx_, jsObject, "height", hasFailed );
    if ( hasFailed )
    { // reports
        return false;
    }

    x_ = GetOptionalProperty<int32_t>( pJsCtx_, jsObject, "x", hasFailed );
    if ( hasFailed )
    { // reports
        return false;
    }

    y_ = GetOptionalProperty<int32_t>( pJsCtx_, jsObject, "y", hasFailed );
    if ( hasFailed )
    { // reports
        return false;
    }

    isCentered_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "center", hasFailed ).value_or( true );
    if ( hasFailed )
    { // reports
        return false;
    }

    isContextMenuEnabled_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "context_menu", hasFailed ).value_or( false );
    if ( hasFailed )
    { // reports
        return false;
    }

    isFormSelectionEnabled_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "selection", hasFailed ).value_or( false );
    if ( hasFailed )
    { // reports
        return false;
    }

    isResizable_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "resizable", hasFailed ).value_or( false );
    if ( hasFailed )
    { // reports
        return false;
    }

    isScrollEnabled_ = GetOptionalProperty<bool>( pJsCtx_, jsObject, "scroll", hasFailed ).value_or( false );
    if ( hasFailed )
    { // reports
        return false;
    }

    bool hasProperty;
    if ( !JS_HasProperty( pJsCtx_, jsObject, "data", &hasProperty ) )
    { // reports
        return false;
    }

    if ( hasProperty )
    {
        _variant_t data;

        JS::RootedValue jsValue( pJsCtx_ );
        if ( !JS_GetProperty( pJsCtx_, jsObject, "data", &jsValue ) )
        { // reports
            return false;
        }

        if ( !convert::com::JsToVariant( pJsCtx_, jsValue, *data.GetAddress() ) )
        {
            JS_ReportErrorUTF8( pJsCtx_, "`data` is of unsupported type" );
            return false;
        }

        pExternal_.Attach( new com_object_impl_t<smp::com::HostExternal>( data ) );
    }

    return true;
}

void CDialogHtml::SetOptions()
{
    if ( !isResizable_ )
    {
        ModifyStyle( WS_THICKFRAME, WS_BORDER, SWP_FRAMECHANGED ); ///< ignore return value, since we don't really care
    }

    if ( width_ || height_ )
    {
        resizeTo( width_.value_or( 250 ), height_.value_or( 100 ) ); ///< ignore return value
    }

    // Center only after we know the size
    if ( isCentered_ )
    {
        CenterWindow(); ///< ignore return value
    }

    if ( x_ || y_ )
    {
        moveTo( x_.value_or( 0 ), y_.value_or( 0 ) ); ///< ignore return value
    }

    {
        const std::wstring w_fb2k_path = pfc::stringcvt::string_wide_from_utf8( helpers::get_fb2k_path() + "foobar2000.exe" );
        SHFILEINFO shfi;
        SHGetFileInfo( w_fb2k_path.c_str(), 0, &shfi, sizeof( SHFILEINFO ), SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_SMALLICON );
        if ( shfi.hIcon )
        {
            hIcon_ = shfi.hIcon;
            SetIcon( hIcon_, FALSE );
        }
    }
}

} // namespace smp::ui