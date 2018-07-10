#pragma once

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandle;

class JsUtils
{
public:
    ~JsUtils();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:
    std::optional<bool> CheckComponent( const pfc::string8_fast& name, bool is_dll = true );
    std::optional<bool> CheckComponentWithOpt( size_t optArgCount, const pfc::string8_fast& name, bool is_dll );
    std::optional<bool> CheckFont( const std::wstring& name );
    std::optional<uint32_t> ColourPicker( uint32_t hWindow, uint32_t default_colour );
    std::optional<JS::Value> FileTest( const std::wstring& path, const std::wstring& mode );
    std::optional<pfc::string8_fast> FormatDuration( double p );
    std::optional<pfc::string8_fast> FormatFileSize( uint64_t p );
    std::optional<std::uint32_t> GetAlbumArtAsync( uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true, bool only_embed = false, bool no_load = false);
    std::optional<std::uint32_t> GetAlbumArtAsyncWithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub, bool only_embed, bool no_load );
    std::optional<JSObject*> GetAlbumArtEmbedded( const pfc::string8_fast& rawpath, uint32_t art_id = 0);
    std::optional<JSObject*> GetAlbumArtEmbeddedWithOpt( size_t optArgCount, const pfc::string8_fast& rawpath, uint32_t art_id );
    std::optional<JSObject*> GetAlbumArtV2( JsFbMetadbHandle* handle, uint32_t art_id = 0, bool need_stub = true );
    std::optional<JSObject*> GetAlbumArtV2WithOpt( size_t optArgCount, JsFbMetadbHandle* handle, uint32_t art_id, bool need_stub );
    std::optional<uint32_t> GetSysColour( uint32_t index );
    std::optional<uint32_t> GetSystemMetrics( uint32_t index );
    std::optional<JSObject*> Glob( const pfc::string8_fast& pattern, uint32_t exc_mask = FILE_ATTRIBUTE_DIRECTORY, uint32_t inc_mask = 0xFFFFFFFF );
    std::optional<JSObject*> GlobWithOpt( size_t optArgCount, const pfc::string8_fast& pattern, uint32_t exc_mask, uint32_t inc_mask );
    std::optional<bool> IsKeyPressed( uint32_t vkey );
    std::optional<std::wstring> MapString( const std::wstring& str, uint32_t lcid, uint32_t flags );
    std::optional<bool> PathWildcardMatch( const std::wstring& pattern, const std::wstring& str );
    std::optional<std::wstring> ReadINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval = L"" );
    std::optional<std::wstring> ReadINIWithOpt( size_t optArgCount, const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& defaultval);
    std::optional<std::wstring> ReadTextFile( const std::wstring& filename, uint32_t codepage = 0 );
    std::optional<std::wstring> ReadTextFileWithOpt( size_t optArgCount, const std::wstring& filename, uint32_t codepage );
    std::optional<bool> WriteINI( const std::wstring& filename, const std::wstring& section, const std::wstring& key, const std::wstring& val );
    std::optional<bool> WriteTextFile( const pfc::string8_fast& filename, const pfc::string8_fast& content, bool write_bom = true );
    std::optional<bool> WriteTextFileWithOpt( size_t optArgCount, const pfc::string8_fast& filename, const pfc::string8_fast& content, bool write_bom );
    
public:    
    std::optional<uint32_t> get_Version();

private:
    JsUtils( JSContext* cx );
    JsUtils( const JsUtils& ) = delete;
    JsUtils& operator=( const JsUtils& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
};

}