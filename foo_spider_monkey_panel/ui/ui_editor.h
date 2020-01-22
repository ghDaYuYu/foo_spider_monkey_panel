#pragma once

#include <ui/scintilla/ui_sci_editor.h>

#include <resource.h>
#include <user_message.h>

#include <functional>

// Forward declarations
namespace smp::panel
{
class js_panel_window;
}

namespace smp::ui
{

class CEditor
    : public CDialogImpl<CEditor>
    , public CDialogResize<CEditor>
{
public:
    using SaveCallback = std::function<void()>;

    CEditor( std::u8string& text, SaveCallback callback = nullptr );

    BEGIN_DLGRESIZE_MAP( CEditor )
        DLGRESIZE_CONTROL( IDC_EDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y )
        DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y )
        DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
    END_DLGRESIZE_MAP()

    // TODO: make `apply` and `save` buttons dynamic via SCN_SAVEPOINTREACHED and SCN_SAVEPOINTLEFT

    BEGIN_MSG_MAP( CEditor )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_NOTIFY( OnNotify )
        MESSAGE_HANDLER( static_cast<UINT>( smp::MiscMessage::key_down ), OnUwmKeyDown )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
        COMMAND_ID_HANDLER_EX( IDAPPLY, OnCloseCmd )
        // Menu
        COMMAND_ID_HANDLER_EX( ID_FILE_SAVE, OnFileSave )
        COMMAND_ID_HANDLER_EX( ID_FILE_IMPORT, OnFileImport )
        COMMAND_ID_HANDLER_EX( ID_FILE_EXPORT, OnFileExport )
        COMMAND_ID_HANDLER_EX( ID_HELP, OnHelp )
        COMMAND_ID_HANDLER_EX( ID_APP_ABOUT, OnAbout )
        CHAIN_MSG_MAP( CDialogResize<CEditor> )
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_EDITOR
    };

    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnNotify( int idCtrl, LPNMHDR pnmh );
    LRESULT OnUwmKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    // Menu
    LRESULT OnFileSave( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFileImport( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFileExport( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnHelp( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnAbout( WORD wNotifyCode, WORD wID, HWND hWndCtl );

    bool ProcessKey( uint32_t vk );
    void Apply();

private:
    smp::ui::sci::CScriptEditorCtrl sciEditor_;
    CMenu menu;

    SaveCallback callback_;

    std::u8string& text_;
    std::u8string m_caption;
};

} // namespace smp::ui
