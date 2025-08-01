#include "ModeratorActionsWnd.h"

#include "ClientUI.h"
#include "CUIControls.h"
#include "../util/i18n.h"
#include "../util/OptionsDB.h"
#include "../util/AppInterface.h"
#include "../client/human/GGHumanClientApp.h"
#include "../Empire/EmpireManager.h"
#include "../Empire/Empire.h"
#include "../universe/Planet.h"
#include "../universe/System.h"
#include "TextBrowseWnd.h"

#include <GG/Button.h>

#include <iterator>


namespace {
    constexpr GG::X CONTROL_WIDTH{32};
    constexpr GG::Y CONTROL_HEIGHT{32};
    constexpr GG::X DROP_WIDTH{96};
    constexpr int   PAD = 3;
}

////////////////////////////////////////////////
// ModeratorActionsWnd
////////////////////////////////////////////////
ModeratorActionsWnd::ModeratorActionsWnd(std::string_view config_name) :
    CUIWnd(UserString("MODERATOR"),
           GG::ONTOP | GG::INTERACTIVE | GG::DRAGABLE | GG::RESIZABLE | CLOSABLE | PINABLE,
           config_name, false),
    m_actions_enabled(true),
    m_selected_action(ModeratorActionSetting::MAS_NoAction)
{}

void ModeratorActionsWnd::CompleteConstruction() {
    auto& app = GetApp();
    auto& ui = app.GetUI();
    GG::Flags<GG::GraphicStyle> style = GG::GRAPHIC_CENTER | GG::GRAPHIC_VCENTER | GG::GRAPHIC_FITGRAPHIC | GG::GRAPHIC_PROPSCALE;

    boost::filesystem::path button_texture_dir = ClientUI::ArtDir() / "icons" / "buttons";

    // button for no action
    m_no_action_button = Wnd::Create<CUIButton>(
        GG::SubTexture(ui.GetTexture(button_texture_dir / "nomoderatoraction.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "nomoderatoraction_clicked.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "nomoderatoraction_mouseover.png")));

    m_no_action_button->SetBrowseModeTime(GetOptionsDB().Get<int>("ui.tooltip.delay"));
    m_no_action_button->SetBrowseInfoWnd(GG::Wnd::Create<TextBrowseWnd>(
        UserString("MOD_NONE"), UserString("MOD_NONE")));
    AttachChild(m_no_action_button);
    m_no_action_button->LeftClickedSignal.connect(boost::bind(&ModeratorActionsWnd::NoAction, this));

    // button for create system and droplist to select system type to create
    m_create_system_button = Wnd::Create<CUIButton>(
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addstar.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addstar_clicked.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addstar_mouseover.png")));

    m_create_system_button->SetBrowseModeTime(GetOptionsDB().Get<int>("ui.tooltip.delay"));
    m_create_system_button->SetBrowseInfoWnd(GG::Wnd::Create<TextBrowseWnd>(
        UserString("MOD_CREATE_SYSTEM"), UserString("MOD_CREATE_SYSTEM")));
    AttachChild(m_create_system_button);

    m_create_system_button->LeftClickedSignal.connect(boost::bind(&ModeratorActionsWnd::CreateSystem, this));
    m_star_type_drop = GG::Wnd::Create<CUIDropDownList>(6);
    m_star_type_drop->Resize(GG::Pt(DROP_WIDTH, CONTROL_HEIGHT));
    for (StarType star_type = StarType::STAR_BLUE; star_type != StarType::NUM_STAR_TYPES;
         star_type = StarType(int(star_type) + 1))
    {
        auto disc_texture = ui.GetModuloTexture(
            ClientUI::ArtDir() / "stars", ClientUI::StarTypeFilePrefix(star_type), 0);
        auto row = GG::Wnd::Create<GG::DropDownList::Row>();
        auto icon = GG::Wnd::Create<GG::StaticGraphic>(std::move(disc_texture), style);
        icon->Resize(GG::Pt(CONTROL_WIDTH, CONTROL_HEIGHT));
        row->push_back(std::move(icon));
        m_star_type_drop->Insert(std::move(row));
    }
    m_star_type_drop->Select(m_star_type_drop->begin());        // default select first type
    m_star_type_drop->SelChangedSignal.connect(boost::bind(&ModeratorActionsWnd::CreateSystem, this));

    // button for create planet and droplists to select planet type and size
    m_create_planet_button = Wnd::Create<CUIButton>(
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addplanet.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addplanet_clicked.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addplanet_mouseover.png")));

    m_create_planet_button->SetBrowseModeTime(GetOptionsDB().Get<int>("ui.tooltip.delay"));
    m_create_planet_button->SetBrowseInfoWnd(GG::Wnd::Create<TextBrowseWnd>(
        UserString("MOD_CREATE_PLANET"), UserString("MOD_CREATE_PLANET")));
    AttachChild(m_create_planet_button);
    m_create_planet_button->LeftClickedSignal.connect(boost::bind(&ModeratorActionsWnd::CreatePlanet, this));

    m_planet_type_drop = GG::Wnd::Create<CUIDropDownList>(6);
    m_planet_type_drop->Resize(GG::Pt(DROP_WIDTH, CONTROL_HEIGHT));
    for (PlanetType planet_type = PlanetType::PT_SWAMP; planet_type != PlanetType::NUM_PLANET_TYPES;
         planet_type = PlanetType(int(planet_type) + 1))
    {
        auto texture = ui.PlanetIcon(planet_type);
        auto row = GG::Wnd::Create<GG::DropDownList::Row>();
        auto icon = GG::Wnd::Create<GG::StaticGraphic>(std::move(texture), style);
        icon->Resize(GG::Pt(CONTROL_WIDTH, CONTROL_HEIGHT));
        row->push_back(std::move(icon));
        m_planet_type_drop->Insert(std::move(row));
    }
    m_planet_type_drop->Select(m_planet_type_drop->begin());    // default select first type
    m_planet_type_drop->SelChangedSignal.connect(boost::bind(&ModeratorActionsWnd::CreatePlanet, this));

    m_planet_size_drop = GG::Wnd::Create<CUIDropDownList>(6);
    m_planet_size_drop->Resize(GG::Pt(DROP_WIDTH, CONTROL_HEIGHT));
    for (PlanetSize planet_size = PlanetSize::SZ_TINY; planet_size != PlanetSize::NUM_PLANET_SIZES;
         planet_size = PlanetSize(int(planet_size) + 1))
    {
        auto texture = ui.PlanetSizeIcon(planet_size);
        auto row = GG::Wnd::Create<GG::DropDownList::Row>();
        auto icon = GG::Wnd::Create<GG::StaticGraphic>(std::move(texture), style);
        icon->Resize(GG::Pt(CONTROL_WIDTH, CONTROL_HEIGHT));
        row->push_back(std::move(icon));
        m_planet_size_drop->Insert(std::move(row));
    }
    auto it = m_planet_size_drop->begin();
    std::advance(it, 2);
    m_planet_size_drop->Select(it); // default select 3rd size (should be medium?)
    m_planet_size_drop->SelChangedSignal.connect(boost::bind(&ModeratorActionsWnd::CreatePlanet, this));

    // button for destroying object
    m_delete_object_button = Wnd::Create<CUIButton>(
        GG::SubTexture(ui.GetTexture(button_texture_dir / "delete.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "delete_clicked.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "delete_mouseover.png")));

    m_delete_object_button->SetBrowseModeTime(GetOptionsDB().Get<int>("ui.tooltip.delay"));
    m_delete_object_button->SetBrowseInfoWnd(GG::Wnd::Create<TextBrowseWnd>(
        UserString("MOD_DESTROY"), UserString("MOD_DESTROY")));
    AttachChild(m_delete_object_button);
    m_delete_object_button->LeftClickedSignal.connect(boost::bind(&ModeratorActionsWnd::DeleteObject, this));

    // button for setting owner
    m_set_owner_button = Wnd::Create<CUIButton>(
        GG::SubTexture(ui.GetTexture(button_texture_dir / "setowner.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "setowner_clicked.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "setowner_mouseover.png")));

    m_set_owner_button->SetBrowseModeTime(GetOptionsDB().Get<int>("ui.tooltip.delay"));
    m_set_owner_button->SetBrowseInfoWnd(GG::Wnd::Create<TextBrowseWnd>(
        UserString("MOD_SET_OWNER"), UserString("MOD_SET_OWNER")));
    AttachChild(m_set_owner_button);

    m_set_owner_button->LeftClickedSignal.connect(boost::bind(&ModeratorActionsWnd::SetOwner, this));
    m_empire_drop = GG::Wnd::Create<CUIDropDownList>(6);
    m_empire_drop->SetStyle(GG::LIST_NOSORT);
    // empires added later when gamestate info available
    m_empire_drop->SelChangedSignal.connect(boost::bind(&ModeratorActionsWnd::SetOwner, this));

    // button for creating starlane
    m_add_starlane_button = Wnd::Create<CUIButton>(
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addstarlane.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addstarlane_clicked.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "addstarlane_mouseover.png")));

    m_add_starlane_button->SetBrowseModeTime(GetOptionsDB().Get<int>("ui.tooltip.delay"));
    m_add_starlane_button->SetBrowseInfoWnd(GG::Wnd::Create<TextBrowseWnd>(
        UserString("MOD_ADD_STARLANE"), UserString("MOD_ADD_STARLANE")));
    AttachChild(m_add_starlane_button);
    m_add_starlane_button->LeftClickedSignal.connect(boost::bind(&ModeratorActionsWnd::AddStarlane, this));

    // button for removing starlane
    m_remove_starlane_button = Wnd::Create<CUIButton>(
        GG::SubTexture(ui.GetTexture(button_texture_dir / "removestarlane.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "removestarlane_clicked.png")),
        GG::SubTexture(ui.GetTexture(button_texture_dir / "removestarlane_mouseover.png")));

    m_remove_starlane_button->SetBrowseModeTime(GetOptionsDB().Get<int>("ui.tooltip.delay"));
    m_remove_starlane_button->SetBrowseInfoWnd(GG::Wnd::Create<TextBrowseWnd>(
        UserString("MOD_REMOVE_STARLANE"), UserString("MOD_REMOVE_STARLANE")));
    AttachChild(m_remove_starlane_button);
    m_remove_starlane_button->LeftClickedSignal.connect(boost::bind(&ModeratorActionsWnd::RemoveStarlane, this));

    CUIWnd::CompleteConstruction();

    DoLayout();
    SaveDefaultedOptions();
}

void ModeratorActionsWnd::NoAction() {
    m_selected_action = ModeratorActionSetting::MAS_NoAction;
    NoActionSelectedSignal();
    DetachChild(m_star_type_drop);
    DetachChild(m_planet_type_drop);
    DetachChild(m_planet_size_drop);
    DetachChild(m_empire_drop);
}

void ModeratorActionsWnd::CreateSystem() {
    m_selected_action = ModeratorActionSetting::MAS_CreateSystem;
    CreateSystemActionSelectedSignal(SelectedStarType());
    AttachChild(m_star_type_drop);
    DetachChild(m_planet_type_drop);
    DetachChild(m_planet_size_drop);
    DetachChild(m_empire_drop);
}

void ModeratorActionsWnd::CreatePlanet() {
    m_selected_action = ModeratorActionSetting::MAS_CreatePlanet;
    CreatePlanetActionSelectedSignal(SelectedPlanetType());
    DetachChild(m_star_type_drop);
    AttachChild(m_planet_type_drop);
    AttachChild(m_planet_size_drop);
    DetachChild(m_empire_drop);
}

void ModeratorActionsWnd::DeleteObject() {
    m_selected_action = ModeratorActionSetting::MAS_Destroy;
    DeleteObjectActionSelectedSignal();
    DetachChild(m_star_type_drop);
    DetachChild(m_planet_type_drop);
    DetachChild(m_planet_size_drop);
    DetachChild(m_empire_drop);
}

void ModeratorActionsWnd::SetOwner() {
    m_selected_action = ModeratorActionSetting::MAS_SetOwner;
    SetOwnerActionSelectedSignal(SelectedEmpire());
    DetachChild(m_star_type_drop);
    DetachChild(m_planet_type_drop);
    DetachChild(m_planet_size_drop);
    AttachChild(m_empire_drop);
}

void ModeratorActionsWnd::AddStarlane() {
    m_selected_action = ModeratorActionSetting::MAS_AddStarlane;
    AddStarlaneActionSelectedSignal();
    DetachChild(m_star_type_drop);
    DetachChild(m_planet_type_drop);
    DetachChild(m_planet_size_drop);
    DetachChild(m_empire_drop);
}

void ModeratorActionsWnd::RemoveStarlane() {
    m_selected_action = ModeratorActionSetting::MAS_RemoveStarlane;
    AddStarlaneActionSelectedSignal();
    DetachChild(m_star_type_drop);
    DetachChild(m_planet_type_drop);
    DetachChild(m_planet_size_drop);
    DetachChild(m_empire_drop);
}

ModeratorActionSetting ModeratorActionsWnd::SelectedAction() const
{ return m_selected_action; }

PlanetType ModeratorActionsWnd::SelectedPlanetType() const
{ return PlanetTypeFromIndex(m_planet_type_drop->CurrentItemIndex()); }

PlanetSize ModeratorActionsWnd::SelectedPlanetSize() const
{ return PlanetSizeFromIndex(m_planet_size_drop->CurrentItemIndex()); }

StarType ModeratorActionsWnd::SelectedStarType() const
{ return StarTypeFromIndex(m_star_type_drop->CurrentItemIndex()); }

int ModeratorActionsWnd::SelectedEmpire() const
{ return EmpireIDFromIndex(m_empire_drop->CurrentItemIndex()); }

void ModeratorActionsWnd::DoLayout() {
    GG::X left = GG::X0 + PAD;
    GG::Y top = GG::Y0 + PAD;

    m_no_action_button->SizeMove(GG::Pt(left, top), GG::Pt(left + CONTROL_WIDTH, top + CONTROL_HEIGHT));
    left += CONTROL_WIDTH + PAD;

    m_create_system_button->SizeMove(GG::Pt(left, top), GG::Pt(left + CONTROL_WIDTH, top + CONTROL_HEIGHT));
    left += CONTROL_WIDTH + PAD;

    m_create_planet_button->SizeMove(GG::Pt(left, top), GG::Pt(left + CONTROL_WIDTH, top + CONTROL_HEIGHT));
    left += CONTROL_WIDTH + PAD;

    m_delete_object_button->SizeMove(GG::Pt(left, top), GG::Pt(left + CONTROL_WIDTH, top + CONTROL_HEIGHT));
    left += CONTROL_WIDTH + PAD;

    m_set_owner_button->SizeMove(GG::Pt(left, top), GG::Pt(left + CONTROL_WIDTH, top + CONTROL_HEIGHT));
    left += CONTROL_WIDTH + PAD;

    m_add_starlane_button->SizeMove(GG::Pt(left, top), GG::Pt(left + CONTROL_WIDTH, top + CONTROL_HEIGHT));
    left += CONTROL_WIDTH + PAD;

    m_remove_starlane_button->SizeMove(GG::Pt(left, top), GG::Pt(left + CONTROL_WIDTH, top + CONTROL_HEIGHT));
    left += CONTROL_WIDTH + PAD;

    left = GG::X0 + PAD;
    top += CONTROL_HEIGHT + PAD;

    // all at same location; only one shown at a time
    m_star_type_drop->SizeMove(GG::Pt(left, top),   GG::Pt(left + DROP_WIDTH, top + CONTROL_HEIGHT));
    m_empire_drop->SizeMove(GG::Pt(left, top),      GG::Pt(left + DROP_WIDTH, top + CONTROL_HEIGHT));

    m_planet_type_drop->SizeMove(GG::Pt(left, top), GG::Pt(left + DROP_WIDTH, top + CONTROL_HEIGHT));
    left += DROP_WIDTH + PAD;
    m_planet_size_drop->SizeMove(GG::Pt(left, top), GG::Pt(left + DROP_WIDTH, top + CONTROL_HEIGHT));
}

void ModeratorActionsWnd::SizeMove(GG::Pt ul, GG::Pt lr) {
    GG::Pt old_size = GG::Wnd::Size();

    CUIWnd::SizeMove(ul, lr);

    if (old_size != GG::Wnd::Size())
        DoLayout();
}

void ModeratorActionsWnd::Refresh() {
    // todo: get currently selected empire, if any, reselect after refresh

    m_empire_drop->Clear();
    for (const auto& entry : Empires()) {
        const auto& empire = entry.second;
        auto row = GG::Wnd::Create<GG::DropDownList::Row>();
        auto label = GG::Wnd::Create<CUILabel>(empire->Name(), GG::FORMAT_NOWRAP);
        label->SetTextColor(empire->Color());
        row->push_back(std::move(label));
        m_empire_drop->Insert(std::move(row));
    }

    // no empire / monsters
    auto row = GG::Wnd::Create<GG::DropDownList::Row>();
    auto label = GG::Wnd::Create<CUILabel>(UserString("UNOWNED"), GG::FORMAT_NOWRAP);
    label->SetTextColor(GG::CLR_RED);
    row->push_back(std::move(label));
    m_empire_drop->Insert(std::move(row));

    if (!m_empire_drop->Empty())
        m_empire_drop->Select(m_empire_drop->begin());
}

void ModeratorActionsWnd::EnableActions(bool enable)
{ m_actions_enabled = enable; }

void ModeratorActionsWnd::CloseClicked()
{ ClosingSignal(); }

StarType ModeratorActionsWnd::StarTypeFromIndex(std::size_t i) const {
    if (i == static_cast<std::size_t>(-1) || i >= int(StarType::NUM_STAR_TYPES))
        return StarType::STAR_BLUE;
    return StarType(i); // assumes first enum and first index are value 0, and that items in list are in same order as enum values
}

PlanetType ModeratorActionsWnd::PlanetTypeFromIndex(std::size_t i) const {
    if (i == std::size_t(-1) || i >= std::size_t(PlanetType::NUM_PLANET_TYPES))
        return PlanetType::PT_SWAMP;
    return PlanetType(i);   // assumes first enum and first index are value 0, and that items in list are in same order as enum values
}

PlanetSize ModeratorActionsWnd::PlanetSizeFromIndex(std::size_t i) const {
    if (i == std::size_t(-1) || i + 1 >= std::size_t(PlanetSize::NUM_PLANET_SIZES))
        return PlanetSize::SZ_MEDIUM;
    return PlanetSize(i + 1);// enum index 0 is NO_WORLD, but don't put that into the list, so have to add 1 to all the list indices
}

int ModeratorActionsWnd::EmpireIDFromIndex(std::size_t i) const {
    if (i == static_cast<std::size_t>(-1) ||
        i >= static_cast<std::size_t>(Empires().NumEmpires()))
    { return ALL_EMPIRES; }
    return std::next(Empires().begin(), i)->first;
}
