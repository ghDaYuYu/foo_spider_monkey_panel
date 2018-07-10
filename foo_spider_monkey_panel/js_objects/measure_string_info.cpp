#include <stdafx.h>
#include "measure_string_info.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>


namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFinalizeOp<JsMeasureStringInfo>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "MeasureStringInfo",
    DefaultClassFlags(),
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsMeasureStringInfo, get_Chars )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMeasureStringInfo, get_Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMeasureStringInfo, get_Lines )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMeasureStringInfo, get_Width )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMeasureStringInfo, get_X )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMeasureStringInfo, get_Y )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Chars", get_Chars, DefaultPropsFlags() ),
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Lines", get_Lines, DefaultPropsFlags() ),
    JS_PSG( "Width", get_Width, DefaultPropsFlags() ),
    JS_PSG( "X", get_X, DefaultPropsFlags() ),
    JS_PSG( "Y", get_Y, DefaultPropsFlags() ),
    JS_PS_END
};


}

namespace mozjs
{


JsMeasureStringInfo::JsMeasureStringInfo( JSContext* cx, float x, float y, float w, float h, uint32_t lines, uint32_t characters )
    : pJsCtx_( cx )
    , x_(x)
    , y_( y )
    , w_( w )
    , h_( h )
    , lines_( lines )
    , characters_( characters )
{
}


JsMeasureStringInfo::~JsMeasureStringInfo()
{
}

JSObject* JsMeasureStringInfo::Create( JSContext* cx, float x, float y, float w, float h, uint32_t lines, uint32_t characters )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsMeasureStringInfo( cx, x, y, w, h, lines, characters ) );

    return jsObj;
}

const JSClass& JsMeasureStringInfo::GetClass()
{
    return jsClass;
}

std::optional<uint32_t> 
JsMeasureStringInfo::get_Chars()
{
    return characters_;
}

std::optional<float> 
JsMeasureStringInfo::get_Height()
{
    return h_;
}

std::optional<uint32_t> 
JsMeasureStringInfo::get_Lines()
{
    return lines_;
}

std::optional<float> 
JsMeasureStringInfo::get_Width()
{
    return w_;
}

std::optional<float> 
JsMeasureStringInfo::get_X()
{
    return x_;
}

std::optional<float> 
JsMeasureStringInfo::get_Y()
{
    return y_;
}

}