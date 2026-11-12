/////////////////////////////////////////////////////////////////////////////
// Program:     wxWidgets Widgets Sample
// Name:        buttonbox.cpp
// Purpose:     Part of the widgets sample showing wxButtonBox
// Author:      Wlodzimierz ABX Skiba, LLM supported
// Created:     12.11.26
// Copyright:   (c) 2026 Wlodzimierz ABX Skiba
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#include "wx/wxprec.h"

#if wxUSE_BUTTONBOX

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/button.h"
    #include "wx/checkbox.h"
    #include "wx/radiobox.h"
    #include "wx/sizer.h"
    #include "wx/stattext.h"
    #include "wx/statbox.h"
    #include "wx/textctrl.h"
#endif

#include "wx/artprov.h"
#include "wx/buttonbox.h"
#include "wx/generic/buttonbox.h" // wxGenericButtonBox is shown on all ports
#include "wx/spinctrl.h"

#include <functional>

#include "widgets.h"
#include "icons.h"


// ----------------------------------------------------------------------------
// ButtonBoxWidgetsPage
// ----------------------------------------------------------------------------

class ButtonBoxWidgetsPage : public WidgetsPage
{
public:
    ButtonBoxWidgetsPage(WidgetsBookCtrl *book, wxImageList *imaglist)
        : WidgetsPage(book, imaglist, WidgetsIcon::ButtonBox) // see icons.h/icons.cpp
    {
        m_radioMode = nullptr;
        m_chkBitmaps = nullptr;
        m_chkLabels = nullptr;
        m_textLabel = nullptr;
        m_spinIndex = nullptr;
        m_spinWidth = nullptr;
        m_sizerBoxes = nullptr;
        m_box = nullptr;
        m_generic = nullptr;
    }

    virtual void CreateContent() override;
    virtual wxWindow *GetWidget() const override { return m_box; }
    virtual wxWindow *GetWidget2() const override { return m_generic; }
    virtual void RecreatePage() override;

private:
    using BoxFunc = std::function<void(wxButtonBoxBase&)>;

    // Everything the page does is applied to both controls, so that the
    // native and the generic version can be compared directly.
    void ForEach(const BoxFunc& func)
    {
        func(*m_box);
        func(*m_generic);
    }

    void Reset();
    long GetModeStyle() const;
    void RecreateBoxes();
    void FillBox(wxButtonBoxBase& box);
    void AddSegment(wxButtonBoxBase& box, size_t pos, const wxString& label);
    wxBitmapBundle GetNextBitmap() const;
    int GetIndex() const { return m_spinIndex->GetValue(); }

    void OnReset(wxCommandEvent& event);
    void OnButtonBox(wxCommandEvent& event);

    wxRadioBox *m_radioMode;
    wxCheckBox *m_chkBitmaps,
               *m_chkLabels;
    wxTextCtrl *m_textLabel;
    wxSpinCtrl *m_spinIndex,
               *m_spinWidth;

    wxSizer *m_sizerBoxes;
    wxButtonBox *m_box;
    wxGenericButtonBox *m_generic;
};

IMPLEMENT_WIDGETS_PAGE(ButtonBoxWidgetsPage, "ButtonBox", ALL_CTRLS);

// ============================================================================
// implementation
// ============================================================================

namespace
{

enum
{
    ButtonBoxPage_Reset = wxID_HIGHEST + 1,
    ButtonBoxPage_Append,
    ButtonBoxPage_Insert,
    ButtonBoxPage_Delete,
    ButtonBoxPage_Clear,
    ButtonBoxPage_Select,
    ButtonBoxPage_Deselect,
    ButtonBoxPage_ToggleEnabled,
    ButtonBoxPage_SetLabel,
    ButtonBoxPage_SetWidth
};

const wxString g_modes[] =
{
    "wxBB_SELECT_ONE",
    "wxBB_SELECT_ANY",
    "wxBB_MOMENTARY"
};

} // anonymous namespace

void ButtonBoxWidgetsPage::CreateContent()
{
    wxSizer *sizerTop = new wxBoxSizer(wxHORIZONTAL);

    // left pane: configuration
    wxSizer *sizerLeft = new wxBoxSizer(wxVERTICAL);

    m_radioMode = new wxRadioBox(this, wxID_ANY, "Selection &mode",
                                 wxDefaultPosition, wxDefaultSize,
                                 WXSIZEOF(g_modes), g_modes, 1);
    sizerLeft->Add(m_radioMode, wxSizerFlags().Expand().Border(wxBOTTOM));

    m_chkLabels = CreateCheckBoxAndAddToSizer(sizerLeft, "Use &labels");
    m_chkLabels->SetValue(true);
    m_chkBitmaps = CreateCheckBoxAndAddToSizer(sizerLeft, "Use &bitmaps");

    sizerLeft->AddSpacer(10);
    sizerLeft->Add(new wxButton(this, ButtonBoxPage_Reset, "&Reset"),
                   wxSizerFlags().CenterHorizontal());

    // middle pane: operations on segments
    wxSizer *sizerOps = new wxStaticBoxSizer(wxVERTICAL, this, "&Segments");
    wxWindow* const box = sizerOps->GetStaticBox();

    wxFlexGridSizer *sizerParams = new wxFlexGridSizer(2, wxSize(5, 5));
    sizerParams->Add(new wxStaticText(box, wxID_ANY, "Index:"),
                     wxSizerFlags().CentreVertical());
    m_spinIndex = new wxSpinCtrl(box, wxID_ANY, "1", wxDefaultPosition,
                                 wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 1);
    sizerParams->Add(m_spinIndex);

    sizerParams->Add(new wxStaticText(box, wxID_ANY, "Label:"),
                     wxSizerFlags().CentreVertical());
    m_textLabel = new wxTextCtrl(box, wxID_ANY, "New");
    sizerParams->Add(m_textLabel);

    sizerParams->Add(new wxStaticText(box, wxID_ANY, "Width:"),
                     wxSizerFlags().CentreVertical());
    m_spinWidth = new wxSpinCtrl(box, wxID_ANY, "0", wxDefaultPosition,
                                 wxDefaultSize, wxSP_ARROW_KEYS, 0, 500, 0);
    m_spinWidth->SetToolTip("0 means automatic width");
    sizerParams->Add(m_spinWidth);
    sizerOps->Add(sizerParams, wxSizerFlags().Border());

    static const struct
    {
        int id;
        const char *label;
    } ops[] =
    {
        { ButtonBoxPage_Append,        "&Append" },
        { ButtonBoxPage_Insert,        "&Insert at index" },
        { ButtonBoxPage_Delete,        "&Delete index" },
        { ButtonBoxPage_Clear,         "&Clear all" },
        { ButtonBoxPage_Select,        "&Select index" },
        { ButtonBoxPage_Deselect,      "D&eselect index" },
        { ButtonBoxPage_ToggleEnabled, "Toggle e&nabled" },
        { ButtonBoxPage_SetLabel,      "Set l&abel" },
        { ButtonBoxPage_SetWidth,      "Set &width" },
    };

    for ( size_t n = 0; n < WXSIZEOF(ops); ++n )
    {
        sizerOps->Add(new wxButton(box, ops[n].id, ops[n].label),
                      wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));
    }

    // right pane: the controls themselves
    m_sizerBoxes = new wxBoxSizer(wxVERTICAL);

    m_sizerBoxes->Add(new wxStaticText(this, wxID_ANY,
                                       "wxButtonBox (native on macOS):"),
                      wxSizerFlags().Border(wxBOTTOM));
    m_box = new wxButtonBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            GetModeStyle());
    m_sizerBoxes->Add(m_box, wxSizerFlags().Border(wxBOTTOM, 20));

    m_sizerBoxes->Add(new wxStaticText(this, wxID_ANY,
                                       "wxGenericButtonBox (toggle buttons):"),
                      wxSizerFlags().Border(wxBOTTOM));
    m_generic = new wxGenericButtonBox(this, wxID_ANY, wxDefaultPosition,
                                       wxDefaultSize, GetModeStyle());
    m_sizerBoxes->Add(m_generic, wxSizerFlags().Border(wxBOTTOM, 20));

    ForEach([this](wxButtonBoxBase& b) { FillBox(b); });

    sizerTop->Add(sizerLeft, wxSizerFlags().Expand().Border(wxALL, 10));
    sizerTop->Add(sizerOps, wxSizerFlags().Expand().Border(wxALL, 10));
    sizerTop->Add(m_sizerBoxes, wxSizerFlags(1).Expand().Border(wxALL, 10));

    SetSizer(sizerTop);

    // events
    Bind(wxEVT_BUTTON, &ButtonBoxWidgetsPage::OnReset, this, ButtonBoxPage_Reset);
    Bind(wxEVT_BUTTONBOX, &ButtonBoxWidgetsPage::OnButtonBox, this);
    Bind(wxEVT_RADIOBOX, [this](wxCommandEvent&) { RecreateBoxes(); },
         m_radioMode->GetId());
    Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { RecreateBoxes(); },
         m_chkLabels->GetId());
    Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { RecreateBoxes(); },
         m_chkBitmaps->GetId());

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const wxString label = m_textLabel->GetValue();
        ForEach([&](wxButtonBoxBase& b) { AddSegment(b, b.GetCount(), label); });
    }, ButtonBoxPage_Append);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const wxString label = m_textLabel->GetValue();
        const size_t pos = GetIndex();
        ForEach([&](wxButtonBoxBase& b)
        {
            if ( pos <= b.GetCount() )
                AddSegment(b, pos, label);
        });
    }, ButtonBoxPage_Insert);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const size_t n = GetIndex();
        ForEach([&](wxButtonBoxBase& b) { if ( n < b.GetCount() ) b.Delete(n); });
    }, ButtonBoxPage_Delete);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        ForEach([](wxButtonBoxBase& b) { b.Clear(); });
    }, ButtonBoxPage_Clear);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const size_t n = GetIndex();
        ForEach([&](wxButtonBoxBase& b) { if ( n < b.GetCount() ) b.SetSelected(n, true); });
    }, ButtonBoxPage_Select);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const size_t n = GetIndex();
        ForEach([&](wxButtonBoxBase& b) { if ( n < b.GetCount() ) b.SetSelected(n, false); });
    }, ButtonBoxPage_Deselect);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const size_t n = GetIndex();
        ForEach([&](wxButtonBoxBase& b)
        {
            if ( n < b.GetCount() )
                b.EnableItem(n, !b.IsItemEnabled(n));
        });
    }, ButtonBoxPage_ToggleEnabled);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const size_t n = GetIndex();
        const wxString label = m_textLabel->GetValue();
        ForEach([&](wxButtonBoxBase& b) { if ( n < b.GetCount() ) b.SetItemLabel(n, label); });
    }, ButtonBoxPage_SetLabel);

    Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        const size_t n = GetIndex();
        const int w = m_spinWidth->GetValue();
        ForEach([&](wxButtonBoxBase& b)
        {
            if ( n < b.GetCount() )
                b.SetItemWidth(n, w > 0 ? w : wxDefaultCoord);
        });
    }, ButtonBoxPage_SetWidth);

    Reset();
}

void ButtonBoxWidgetsPage::Reset()
{
    m_radioMode->SetSelection(0);
    m_chkLabels->SetValue(true);
    m_chkBitmaps->SetValue(false);
    m_textLabel->SetValue("New");
    m_spinIndex->SetValue(1);
    m_spinWidth->SetValue(0);
}

long ButtonBoxWidgetsPage::GetModeStyle() const
{
    switch ( m_radioMode ? m_radioMode->GetSelection() : 0 )
    {
        case 1:  return wxBB_SELECT_ANY;
        case 2:  return wxBB_MOMENTARY;
        default: return wxBB_SELECT_ONE;
    }
}

wxBitmapBundle ButtonBoxWidgetsPage::GetNextBitmap() const
{
    static const wxArtID ids[] =
        { wxART_GO_BACK, wxART_GO_UP, wxART_GO_FORWARD, wxART_GO_DOWN };

    static size_t s_next = 0;
    return wxArtProvider::GetBitmapBundle(ids[s_next++ % WXSIZEOF(ids)],
                                          wxART_BUTTON);
}

void ButtonBoxWidgetsPage::AddSegment(wxButtonBoxBase& box, size_t pos,
                                      const wxString& label)
{
    box.Insert(pos,
               m_chkLabels->GetValue() ? label : wxString(),
               m_chkBitmaps->GetValue() ? GetNextBitmap() : wxBitmapBundle());
    box.SetItemToolTip(pos, wxString::Format("Segment \"%s\"", label));
}

void ButtonBoxWidgetsPage::FillBox(wxButtonBoxBase& box)
{
    static const char* const labels[] = { "Left", "Center", "Right" };

    for ( size_t n = 0; n < WXSIZEOF(labels); ++n )
        AddSegment(box, n, labels[n]);
}

void ButtonBoxWidgetsPage::RecreateBoxes()
{
    // The selection mode is a creation-time style, so the controls have to be
    // replaced when it changes (the same is done for other style flags in the
    // other pages of this sample).
    wxButtonBox *box = new wxButtonBox(this, wxID_ANY, wxDefaultPosition,
                                       wxDefaultSize, GetModeStyle());
    m_sizerBoxes->Replace(m_box, box);
    delete m_box;
    m_box = box;

    wxGenericButtonBox *generic =
        new wxGenericButtonBox(this, wxID_ANY, wxDefaultPosition,
                               wxDefaultSize, GetModeStyle());
    m_sizerBoxes->Replace(m_generic, generic);
    delete m_generic;
    m_generic = generic;

    ForEach([this](wxButtonBoxBase& b) { FillBox(b); });

    m_sizerBoxes->Layout();
}

void ButtonBoxWidgetsPage::RecreatePage()
{
    RecreateBoxes();
}

void ButtonBoxWidgetsPage::OnReset(wxCommandEvent& WXUNUSED(event))
{
    Reset();
    RecreateBoxes();
}

void ButtonBoxWidgetsPage::OnButtonBox(wxCommandEvent& event)
{
    wxLogMessage("%s: segment %d (\"%s\") %s",
                 event.GetEventObject() == m_box ? "wxButtonBox"
                                                 : "wxGenericButtonBox",
                 event.GetInt(),
                 event.GetString(),
                 event.GetExtraLong() ? "selected/clicked" : "deselected");
}

#endif // wxUSE_BUTTONBOX
