// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "yue/api_gui.h"

#include <string>

#include "nativeui/nativeui.h"
#include "yue/api_signal.h"

namespace lua {

template<>
struct Type<nu::SizeF> {
  static constexpr const char* name = "yue.Size";
  static inline void Push(State* state, const nu::SizeF& size) {
    lua::PushNewTable(state);
    lua::RawSet(state, -1, "width", size.width(), "height", size.height());
  }
  static inline bool To(State* state, int index, nu::SizeF* out) {
    if (GetType(state, index) != LuaType::Table)
      return false;
    float width, height;
    if (!RawGetAndPop(state, index, "width", &width, "height", &height))
      return false;
    *out = nu::SizeF(width, height);
    return true;
  }
};

template<>
struct Type<nu::RectF> {
  static constexpr const char* name = "yue.Rect";
  static inline void Push(State* state, const nu::RectF& rect) {
    lua::PushNewTable(state);
    lua::RawSet(state, -1,
                "x", rect.x(), "y", rect.y(),
                "width", rect.width(), "height", rect.height());
  }
  static inline bool To(State* state, int index, nu::RectF* out) {
    if (GetType(state, index) != LuaType::Table)
      return false;
    float x, y, width, height;
    if (!RawGetAndPop(state, index,
                      "x", &x, "y", &y, "width", &width, "height", &height))
      return false;
    *out = nu::RectF(x, y, width, height);
    return true;
  }
};

template<>
struct Type<nu::Insets> {
  static constexpr const char* name = "yue.Insets";
  static inline void Push(State* state, const nu::Insets& insets) {
    lua::PushNewTable(state);
    lua::RawSet(state, -1, "top", insets.top(), "left", insets.left(),
                           "bottom", insets.bottom(), "right", insets.right());
  }
  static inline bool To(State* state, int index, nu::Insets* out) {
    if (GetType(state, index) != LuaType::Table)
      return false;
    int top, left, bottom, right;;
    if (!RawGetAndPop(state, index, "top", &top, "left", &left,
                                    "bottom", &bottom, "right", &right))
      return false;
    *out = nu::Insets(top, left, bottom, right);
    return true;
  }
};

template<>
struct Type<nu::Color> {
  static constexpr const char* name = "yue.Color";
  static inline bool To(State* state, int index, nu::Color* out) {
    std::string hex;
    if (!lua::To(state, index, &hex))
      return false;
    *out = nu::Color(hex);
    return true;
  }
};

template<>
struct Type<nu::Accelerator> {
  static constexpr const char* name = "yue.Accelerator";
  static inline bool To(State* state, int index, nu::Accelerator* out) {
    std::string description;
    if (!lua::To(state, index, &description))
      return false;
    nu::Accelerator tmp(description);
    if (tmp.empty())
      return false;
    *out = tmp;
    return true;
  }
};

template<>
struct Type<nu::View> {
  static constexpr const char* name = "yue.View";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "setbackgroundcolor", &nu::View::SetBackgroundColor,
           "setstyle", &SetStyle,
           "printstyle", &nu::View::PrintStyle,
           "layout", &nu::View::Layout,
           "setbounds", &nu::View::SetBounds,
           "getbounds", &nu::View::GetBounds,
           "setvisible", &nu::View::SetVisible,
           "isvisible", &nu::View::IsVisible,
           "getparent", &nu::View::parent);
  }
  static void SetStyle(CallContext* context, nu::View* view) {
    if (GetType(context->state, 2) != LuaType::Table) {
      context->has_error = true;
      Push(context->state, "first arg must be table");
      return;
    }
    Push(context->state, nullptr);
    while (lua_next(context->state, 2) != 0) {
      std::string key, value;
      To(context->state, -2, &key, &value);
      view->SetStyle(key, value);
      PopAndIgnore(context->state, 1);
    }
    // Refresh the view.
    view->Layout();
  }
};

template<>
struct Type<nu::App> {
  static constexpr const char* name = "yue.App";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
#if defined(OS_MACOSX)
           "setapplicationmenu", &nu::App::SetApplicationMenu,
#endif
           "run", &nu::App::Run,
           "quit", &nu::App::Quit,
           "post", &nu::App::PostTask,
           "postdelayed", &nu::App::PostDelayedTask);
  }
  static int NewIndex(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::MemberNewIndex(state, name, "onready", &nu::App::on_ready);
  }
};

template<>
struct Type<nu::Button::Type> {
  static constexpr const char* name = "yue.Button.Type";
  static bool To(State* state, int index, nu::Button::Type* out) {
    std::string type;
    if (!lua::To(state, index, &type))
      return false;
    if (type.empty() || type == "normal") {
      *out = nu::Button::Normal;
      return true;
    } else if (type == "checkbox") {
      *out = nu::Button::CheckBox;
      return true;
    } else if (type == "radio") {
      *out = nu::Button::Radio;
      return true;
    } else {
      return false;
    }
  }
};

template<>
struct Type<nu::Button> {
  using base = nu::View;
  static constexpr const char* name = "yue.Button";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index, "new", &New,
                         "settitle", &nu::Button::SetTitle,
                         "gettitle", &nu::Button::GetTitle,
                         "setchecked", &nu::Button::SetChecked,
                         "ischecked", &nu::Button::IsChecked);
  }
  static nu::Button* New(CallContext* context) {
    std::string title;
    if (To(context->state, 1, &title)) {
      return new nu::Button(title);
    } else if (GetType(context->state, 1) == LuaType::Table) {
      RawGetAndPop(context->state, 1, "title", &title);
      nu::Button::Type type = nu::Button::Normal;
      RawGetAndPop(context->state, 1, "type", &type);
      return new nu::Button(title, type);
    } else {
      context->has_error = true;
      Push(context->state, "Button must be created with string or table");
      return nullptr;
    }
  }
  static int Index(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::SignalIndex(state, name, "onclick", &nu::Button::on_click);
  }
  static int NewIndex(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::MemberNewIndex(state, name, "onclick", &nu::Button::on_click);
  }
};

template<>
struct Type<nu::Entry> {
  using base = nu::View;
  static constexpr const char* name = "yue.Entry";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index, "new", &MetaTable<nu::Entry>::NewInstance<>,
                         "settext", &nu::Entry::SetText,
                         "gettext", &nu::Entry::GetText);
  }
  static int Index(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::SignalIndex(state, name,
                            "onactivate", &nu::Entry::on_activate,
                            "ontextchange", &nu::Entry::on_text_change);
  }
  static int NewIndex(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::MemberNewIndex(state, name,
                               "onactivate", &nu::Entry::on_activate,
                               "ontextchange", &nu::Entry::on_text_change);
  }
};

template<>
struct Type<nu::Label> {
  using base = nu::View;
  static constexpr const char* name = "yue.Label";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &MetaTable<nu::Label>::NewInstance<const std::string&>,
           "settext", &nu::Label::SetText,
           "gettext", &nu::Label::GetText);
  }
};

template<>
struct Type<nu::Progress> {
  using base = nu::View;
  static constexpr const char* name = "yue.Progress";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index, "new", &MetaTable<nu::Progress>::NewInstance<>,
                         "setvalue", &nu::Progress::SetValue,
                         "getvalue", &nu::Progress::GetValue,
                         "setindeterminate", &nu::Progress::SetIndeterminate,
                         "isindeterminate", &nu::Progress::IsIndeterminate);
  }
};

template<>
struct Type<nu::Group> {
  using base = nu::View;
  static constexpr const char* name = "yue.Group";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &MetaTable<nu::Group>::NewInstance<const std::string&>,
           "setcontentview", &nu::Group::SetContentView,
           "getcontentview", &nu::Group::GetContentView,
           "settitle", &nu::Group::SetTitle,
           "gettitle", &nu::Group::GetTitle);
  }
};

template<>
struct Type<nu::Container> {
  using base = nu::View;
  static constexpr const char* name = "yue.Container";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &MetaTable<nu::Container>::NewInstance<>,
           "getpreferredsize", &nu::Container::GetPreferredSize,
           "getpreferredwidthforheight",
           &nu::Container::GetPreferredWidthForHeight,
           "getpreferredheightforwidth",
           &nu::Container::GetPreferredHeightForWidth,
           "addchildview", &nu::Container::AddChildView,
           "addchildviewat", &AddChildViewAt,
           "removechildview", &nu::Container::RemoveChildView,
           "childcount", &nu::Container::child_count,
           "childat", &ChildAt);
  }
  static int Index(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::SignalIndex(state, name, "ondraw", &nu::Container::on_draw);
  }
  static int NewIndex(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::MemberNewIndex(state, name, "ondraw", &nu::Container::on_draw);
  }
  // Transalte 1-based index to 0-based.
  static inline void AddChildViewAt(nu::Container* c, nu::View* view, int i) {
    c->AddChildViewAt(view, i - 1);
  }
  static inline nu::View* ChildAt(nu::Container* container, int i) {
    return container->child_at(i - 1);
  }
};

template<>
struct Type<nu::Scroll::Policy> {
  static constexpr const char* name = "yue.Scroll.Policy";
  static inline bool To(State* state, int index, nu::Scroll::Policy* out) {
    std::string policy;
    if (!lua::To(state, index, &policy))
      return false;
    if (policy == "automatic") {
      *out = nu::Scroll::Policy::Automatic;
      return true;
    } else if (policy == "always") {
      *out = nu::Scroll::Policy::Always;
      return true;
    } else if (policy == "never") {
      *out = nu::Scroll::Policy::Never;
      return true;
    } else {
      return false;
    }
  }
  static inline void Push(State* state, nu::Scroll::Policy policy) {
    if (policy == nu::Scroll::Policy::Always)
      lua::Push(state, "always");
    else if (policy == nu::Scroll::Policy::Never)
      lua::Push(state, "never");
    else
      lua::Push(state, "automatic");
  }
};

template<>
struct Type<nu::Scroll> {
  using base = nu::View;
  static constexpr const char* name = "yue.Scroll";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &MetaTable<nu::Scroll>::NewInstance<>,
           "setscrollbarpolicy", &nu::Scroll::SetScrollBarPolicy,
           "getscrollbarpolicy", &nu::Scroll::GetScrollBarPolicy,
           "setcontentsize", &nu::Scroll::SetContentSize,
           "getcontentsize", &nu::Scroll::GetContentSize,
           "setcontentview", &nu::Scroll::SetContentView,
           "getcontentview", &nu::Scroll::GetContentView);
  }
};

template<>
struct Type<nu::Window::Options> {
  static constexpr const char* name = "yue.Window.Options";
  static inline bool To(State* state, int index, nu::Window::Options* out) {
    if (GetType(state, index) == LuaType::Table)
      RawGetAndPop(state, index, "bounds", &out->bounds);
    return true;
  }
};

template<>
struct Type<nu::Window> {
  static constexpr const char* name = "yue.Window";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &MetaTable<nu::Window>::NewInstance<nu::Window::Options>,
           "close", &nu::Window::Close,
           "setcontentbounds", &nu::Window::SetContentBounds,
           "getcontentbounds", &nu::Window::GetContentBounds,
           "setbounds", &nu::Window::SetBounds,
           "getbounds", &nu::Window::GetBounds,
           "setcontentview", &nu::Window::SetContentView,
           "getcontentview", &nu::Window::GetContentView,
           "setvisible", &nu::Window::SetVisible,
           "isvisible", &nu::Window::IsVisible,
#if defined(OS_WIN) || defined(OS_LINUX)
           "setmenubar", &nu::Window::SetMenuBar,
           "getmenubar", &nu::Window::GetMenuBar,
#endif
           "setbackgroundcolor", &nu::Window::SetBackgroundColor);
  }
  static int Index(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::SignalIndex(state, name, "onclose", &nu::Window::on_close);
  }
  static int NewIndex(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::MemberNewIndex(state, name,
                               "onclose", &nu::Window::on_close,
                               "shouldclose", &nu::Window::should_close);
  }
};

template<>
struct Type<nu::Font> {
  static constexpr const char* name = "yue.Font";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &New,
           "default", &GetDefault,
           "getname", &nu::Font::GetName,
           "getsize", &nu::Font::GetSize);
  }
  static nu::Font* New(const std::string& font_name, int font_size ){
    return nu::Font::CreateFromNameAndSize(font_name, font_size);
  }
  static nu::Font* GetDefault() {
    return nu::State::current()->GetDefaultFont();
  }
};

template<>
struct Type<nu::Image> {
  static constexpr const char* name = "yue.Image";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &MetaTable<nu::Image>::NewInstance<const nu::String&>);
  }
};

template<>
struct Type<nu::Painter> {
  static constexpr const char* name = "yue.Painter";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "save", &nu::Painter::Save,
           "restore", &nu::Painter::Restore,
           "fillrect", &nu::Painter::FillRect);
  }
};

template<>
struct Type<nu::MenuBase> {
  static constexpr const char* name = "yue.MenuBase";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index, "append", &nu::MenuBase::Append,
                         "insert", &Insert,
                         "itemcount", &nu::MenuBase::item_count,
                         "itemat", &ItemAt);
  }
  static inline void Insert(nu::MenuBase* menu, nu::MenuItem* item, int i) {
    menu->Insert(item, i - 1);
  }
  static inline nu::MenuItem* ItemAt(nu::MenuBase* menu, int i) {
    return menu->item_at(i - 1);
  }
  // Used by subclasses.
  static void ReadMembers(State* state, nu::MenuBase* menu) {
    if (GetType(state, 1) != LuaType::Table)
      return;
    Push(state, nullptr);
    while (lua_next(state, 1) != 0) {
      nu::MenuItem* item;
      if (Pop(state, &item))
        menu->Append(item);
      else
        PopAndIgnore(state, 1);
    }
  }
};

template<>
struct Type<nu::MenuBar> {
  using base = nu::MenuBase;
  static constexpr const char* name = "yue.MenuBar";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index, "new", &New);
  }
  static nu::MenuBar* New(CallContext* context) {
    nu::MenuBar* menu = new nu::MenuBar;
    Type<nu::MenuBase>::ReadMembers(context->state, menu);
    return menu;
  }
};

template<>
struct Type<nu::Menu> {
  using base = nu::MenuBase;
  static constexpr const char* name = "yue.Menu";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index, "new", &New, "popup", &nu::Menu::Popup);
  }
  static nu::Menu* New(CallContext* context) {
    nu::Menu* menu = new nu::Menu;
    Type<nu::MenuBase>::ReadMembers(context->state, menu);
    return menu;
  }
};

template<>
struct Type<nu::MenuItem::Type> {
  static constexpr const char* name = "yue.MenuItem.Type";
  static bool To(State* state, int index, nu::MenuItem::Type* out) {
    std::string type;
    if (!lua::To(state, index, &type))
      return false;
    if (type == "label") {
      *out = nu::MenuItem::Label;
      return true;
    } else if (type == "checkbox") {
      *out = nu::MenuItem::CheckBox;
      return true;
    } else if (type == "radio") {
      *out = nu::MenuItem::Radio;
      return true;
    } else if (type == "separator") {
      *out = nu::MenuItem::Separator;
      return true;
    } else if (type == "submenu") {
      *out = nu::MenuItem::Submenu;
      return true;
    } else {
      return false;
    }
  }
};

template<>
struct Type<nu::MenuItem> {
  static constexpr const char* name = "yue.MenuItem";
  static void BuildMetaTable(State* state, int index) {
    RawSet(state, index,
           "new", &New,
           "setlabel", &nu::MenuItem::SetLabel,
           "getlabel", &nu::MenuItem::GetLabel,
           "setchecked", &nu::MenuItem::SetChecked,
           "ischecked", &nu::MenuItem::IsChecked,
           "setenabled", &nu::MenuItem::SetEnabled,
           "isenabled", &nu::MenuItem::IsEnabled,
           "setvisible", &nu::MenuItem::SetVisible,
           "isvisible", &nu::MenuItem::IsVisible,
           "setsubmenu", &nu::MenuItem::SetSubmenu,
           "getsubmenu", &nu::MenuItem::GetSubmenu,
           "setaccelerator", &nu::MenuItem::SetAccelerator);
  }
  static int Index(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::SignalIndex(state, name, "onclick", &nu::MenuItem::on_click);
  }
  static int NewIndex(State* state) {
    std::string name;
    if (!To(state, 2, &name))
      return 0;
    return yue::MemberNewIndex(state, name, "onclick", &nu::MenuItem::on_click);
  }
  static nu::MenuItem* New(CallContext* context) {
    nu::MenuItem::Type type = nu::MenuItem::Label;
    if (lua::To(context->state, 1, &type) ||  // 'type'
        GetType(context->state, 1) != LuaType::Table)  // {type='type'}
      return new nu::MenuItem(type);
    // Use label unless "type" is specified.
    nu::MenuItem* item = nullptr;
    if (RawGetAndPop(context->state, 1, "type", &type))
      item = new nu::MenuItem(type);
    // Read table fields and set attributes.
    bool b = false;
    if (RawGetAndPop(context->state, 1, "checked", &b)) {
      if (!item) item = new nu::MenuItem(nu::MenuItem::CheckBox);
      item->SetChecked(b);
    }
    nu::Menu* submenu = nullptr;
    if (RawGetAndPop(context->state, 1, "submenu", &submenu)) {
      if (!item) item = new nu::MenuItem(nu::MenuItem::Submenu);
      item->SetSubmenu(submenu);
    }
    if (!item)  // can not deduce type from property, assuming Label item.
      item = new nu::MenuItem(nu::MenuItem::Label);
    if (RawGetAndPop(context->state, 1, "visible", &b))
      item->SetVisible(b);
    if (RawGetAndPop(context->state, 1, "enabled", &b))
      item->SetEnabled(b);
    std::string label;
    if (RawGetAndPop(context->state, 1, "label", &label))
      item->SetLabel(label);
    nu::Accelerator accelerator;
    if (RawGetAndPop(context->state, 1, "accelerator", &accelerator))
      item->SetAccelerator(accelerator);
    return item;
  }
};

}  // namespace lua

extern "C" int luaopen_yue_gui(lua::State* state) {
  // Manage the gui state in lua.
  void* memory = lua_newuserdata(state, sizeof(nu::State));
  new(memory) nu::State;
  lua::PushNewTable(state);
  lua::RawSet(state, -1, "__gc", lua::CFunction(lua::OnGC<nu::State>));
  lua::SetMetaTable(state, -2);

  // Put the gui state into registry, so it is alive through whole lua state.
  luaL_ref(state, LUA_REGISTRYINDEX);

  // Populate the table with GUI elements.
  lua::PushNewTable(state);
  lua::Push(state, "App");
  lua::MetaTable<nu::App>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Window");
  lua::MetaTable<nu::Window>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Container");
  lua::MetaTable<nu::Container>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Button");
  lua::MetaTable<nu::Button>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Entry");
  lua::MetaTable<nu::Entry>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Label");
  lua::MetaTable<nu::Label>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Progress");
  lua::MetaTable<nu::Progress>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Group");
  lua::MetaTable<nu::Group>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Scroll");
  lua::MetaTable<nu::Scroll>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Font");
  lua::MetaTable<nu::Font>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Image");
  lua::MetaTable<nu::Image>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Painter");
  lua::MetaTable<nu::Painter>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "MenuBar");
  lua::MetaTable<nu::MenuBar>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "Menu");
  lua::MetaTable<nu::Menu>::Push(state);
  lua_rawset(state, -3);
  lua::Push(state, "MenuItem");
  lua::MetaTable<nu::MenuItem>::Push(state);
  lua_rawset(state, -3);

  // Create APIs that only available as instances.
  lua::Push(state, "app");
  lua::MetaTable<nu::App>::PushNewWrapper(state, nu::App::current());
  lua_rawset(state, -3);
  return 1;
}
