#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <map>
#include <functional>

class js_panel_window;

// TODO: raise wraning level to 4 or ALL with error on warning

namespace mozjs
{

class JsContainer;

class JsEngine final
{
public:
    ~JsEngine();

    static JsEngine& GetInstance();

public:
    JSContext * GetJsContext() const;

    bool RegisterPanel( js_panel_window& parentPanel, JsContainer& jsContainer );
    void UnregisterPanel( js_panel_window& parentPanel );

private:
    JsEngine();
    JsEngine( const JsEngine& ) = delete;

private:
    bool Initialize();
    void Finalize();

private:
    JSContext * pJsCtx_ = nullptr;

    bool isInitialized_ = false;

    std::map<HWND, std::reference_wrapper<JsContainer>> registeredPanels_;
};

}
