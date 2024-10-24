#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>
#include <ranges>

#include "SDL_console.h"

#define CONSOLE_SDL_LINK_AT_RUNTIME 0
namespace console {

#if defined(CONSOLE_SDL_LINK_AT_RUNTIME) && (CONSOLE_SDL_LINK_AT_RUNTIME) == 1
#define CONSOLE_SYMBOL_ADDR(sym) nullptr
#else
#define CONSOLE_SYMBOL_ADDR(sym) ::sym
#endif

#define CONSOLE_DEFINE_SYMBOL(sym) decltype(sym)* sym = CONSOLE_SYMBOL_ADDR(sym)

CONSOLE_DEFINE_SYMBOL(SDL_ConvertSurfaceFormat);
CONSOLE_DEFINE_SYMBOL(SDL_CreateRenderer);
CONSOLE_DEFINE_SYMBOL(SDL_CreateRGBSurface);
CONSOLE_DEFINE_SYMBOL(SDL_CreateTexture);
CONSOLE_DEFINE_SYMBOL(SDL_CreateTextureFromSurface);
CONSOLE_DEFINE_SYMBOL(SDL_CreateWindow);
CONSOLE_DEFINE_SYMBOL(SDL_DestroyRenderer);
CONSOLE_DEFINE_SYMBOL(SDL_DestroyTexture);
CONSOLE_DEFINE_SYMBOL(SDL_DestroyWindow);
CONSOLE_DEFINE_SYMBOL(SDL_free);
CONSOLE_DEFINE_SYMBOL(SDL_FreeSurface);
CONSOLE_DEFINE_SYMBOL(SDL_GetClipboardText);
CONSOLE_DEFINE_SYMBOL(SDL_GetError);
CONSOLE_DEFINE_SYMBOL(SDL_GetEventFilter);
CONSOLE_DEFINE_SYMBOL(SDL_GetModState);
CONSOLE_DEFINE_SYMBOL(SDL_GetRendererOutputSize);
CONSOLE_DEFINE_SYMBOL(SDL_GetWindowFlags);
CONSOLE_DEFINE_SYMBOL(SDL_GetWindowID);
CONSOLE_DEFINE_SYMBOL(SDL_HideWindow);
CONSOLE_DEFINE_SYMBOL(SDL_iconv_string);
CONSOLE_DEFINE_SYMBOL(SDL_InitSubSystem);
CONSOLE_DEFINE_SYMBOL(SDL_MapRGB);
CONSOLE_DEFINE_SYMBOL(SDL_memset);
CONSOLE_DEFINE_SYMBOL(SDL_RenderClear);
CONSOLE_DEFINE_SYMBOL(SDL_RenderCopy);
CONSOLE_DEFINE_SYMBOL(SDL_RenderDrawRect);
CONSOLE_DEFINE_SYMBOL(SDL_RenderFillRect);
CONSOLE_DEFINE_SYMBOL(SDL_RenderPresent);
CONSOLE_DEFINE_SYMBOL(SDL_RenderSetIntegerScale);
CONSOLE_DEFINE_SYMBOL(SDL_RenderSetViewport);
CONSOLE_DEFINE_SYMBOL(SDL_PointInRect);
CONSOLE_DEFINE_SYMBOL(SDL_SetClipboardText);
CONSOLE_DEFINE_SYMBOL(SDL_SetColorKey);
CONSOLE_DEFINE_SYMBOL(SDL_SetEventFilter);
CONSOLE_DEFINE_SYMBOL(SDL_SetHint);
CONSOLE_DEFINE_SYMBOL(SDL_SetRenderDrawColor);
CONSOLE_DEFINE_SYMBOL(SDL_SetTextureBlendMode);
CONSOLE_DEFINE_SYMBOL(SDL_SetTextureColorMod);
CONSOLE_DEFINE_SYMBOL(SDL_SetWindowMinimumSize);
CONSOLE_DEFINE_SYMBOL(SDL_ShowWindow);
CONSOLE_DEFINE_SYMBOL(SDL_StartTextInput);
CONSOLE_DEFINE_SYMBOL(SDL_StopTextInput);
CONSOLE_DEFINE_SYMBOL(SDL_UpperBlit);
CONSOLE_DEFINE_SYMBOL(SDL_UpdateTexture);
CONSOLE_DEFINE_SYMBOL(SDL_QuitSubSystem);

struct Symbol {
    const char* name;
    void** addr;
};

void resolve_symbols(Console_SymResolverProc resolver)
{

#define CONSOLE_ADD_SYMBOL(sym)     \
    {                               \
        #sym, (void**)&console::sym \
    }

    /* This list must be in parity with CONSOLE_DEFINE_SYMBOL */
    std::vector<Symbol> symbols = {
        CONSOLE_ADD_SYMBOL(SDL_ConvertSurfaceFormat),
        CONSOLE_ADD_SYMBOL(SDL_CreateRenderer),
        CONSOLE_ADD_SYMBOL(SDL_CreateRGBSurface),
        CONSOLE_ADD_SYMBOL(SDL_CreateTexture),
        CONSOLE_ADD_SYMBOL(SDL_CreateTextureFromSurface),
        CONSOLE_ADD_SYMBOL(SDL_CreateWindow),
        CONSOLE_ADD_SYMBOL(SDL_DestroyRenderer),
        CONSOLE_ADD_SYMBOL(SDL_DestroyTexture),
        CONSOLE_ADD_SYMBOL(SDL_DestroyWindow),
        CONSOLE_ADD_SYMBOL(SDL_free),
        CONSOLE_ADD_SYMBOL(SDL_FreeSurface),
        CONSOLE_ADD_SYMBOL(SDL_GetClipboardText),
        CONSOLE_ADD_SYMBOL(SDL_GetError),
        CONSOLE_ADD_SYMBOL(SDL_GetEventFilter),
        CONSOLE_ADD_SYMBOL(SDL_GetModState),
        CONSOLE_ADD_SYMBOL(SDL_GetRendererOutputSize),
        CONSOLE_ADD_SYMBOL(SDL_GetWindowFlags),
        CONSOLE_ADD_SYMBOL(SDL_GetWindowID),
        CONSOLE_ADD_SYMBOL(SDL_HideWindow),
        CONSOLE_ADD_SYMBOL(SDL_iconv_string),
        CONSOLE_ADD_SYMBOL(SDL_InitSubSystem),
        CONSOLE_ADD_SYMBOL(SDL_MapRGB),
        CONSOLE_ADD_SYMBOL(SDL_memset),
        CONSOLE_ADD_SYMBOL(SDL_RenderClear),
        CONSOLE_ADD_SYMBOL(SDL_RenderCopy),
        CONSOLE_ADD_SYMBOL(SDL_RenderDrawRect),
        CONSOLE_ADD_SYMBOL(SDL_RenderFillRect),
        CONSOLE_ADD_SYMBOL(SDL_RenderPresent),
        CONSOLE_ADD_SYMBOL(SDL_RenderSetIntegerScale),
        CONSOLE_ADD_SYMBOL(SDL_RenderSetViewport),
        CONSOLE_ADD_SYMBOL(SDL_PointInRect),
        CONSOLE_ADD_SYMBOL(SDL_SetClipboardText),
        CONSOLE_ADD_SYMBOL(SDL_SetColorKey),
        CONSOLE_ADD_SYMBOL(SDL_SetEventFilter),
        CONSOLE_ADD_SYMBOL(SDL_SetHint),
        CONSOLE_ADD_SYMBOL(SDL_SetRenderDrawColor),
        CONSOLE_ADD_SYMBOL(SDL_SetTextureBlendMode),
        CONSOLE_ADD_SYMBOL(SDL_SetTextureColorMod),
        CONSOLE_ADD_SYMBOL(SDL_SetWindowMinimumSize),
        CONSOLE_ADD_SYMBOL(SDL_ShowWindow),
        CONSOLE_ADD_SYMBOL(SDL_StartTextInput),
        CONSOLE_ADD_SYMBOL(SDL_StopTextInput),
        CONSOLE_ADD_SYMBOL(SDL_UpperBlit),
        CONSOLE_ADD_SYMBOL(SDL_UpdateTexture),
        CONSOLE_ADD_SYMBOL(SDL_QuitSubSystem)
    };
#undef CONSOLE_ADD_SYMBOL

    for (auto& sym : symbols) {
        *sym.addr = resolver(sym.name);
    }
}

static constexpr size_t default_scrollback = 1024;

#if 0
/*
 * std::wstring_convert and std::codecvt_utf8 are deprecated
 * since C++17 and MSVC will warn with -W4.
 * Implementations are known to be lackluster.
 */
static std::u32string from_utf8(const char* str)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> c;
    return c.from_bytes(str);
}

static std::string to_utf8(const std::u32string& str)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> c;
    return c.to_bytes(str);
}
#endif

size_t utf8_strlen(const char* str)
{
    size_t count = 0;
    size_t i = 0;
    while (str[i]) {
        unsigned char byte = str[i];
        if ((byte & 0x80) == 0) {
            ++i;
        } else if ((byte & 0xE0) == 0xC0) {
            i += 2;
        } else if ((byte & 0xF0) == 0xE0) {
            i += 3;
        } else if ((byte & 0xF8) == 0xF0) {
            i += 4;
        } else {
            // Invalid byte
            ++i;
        }
        ++count;
    }
    return count;
}

/* For consistency it might be better to stick with one utf8 library */
static std::string to_utf8(const std::u32string& str)
{
    // SDL casts the type from wide to narrow when necessary for these things. Should be ok.
    const char* strp = reinterpret_cast<const char*>(str.data());
    // Need to make room for utf8 + terminating null. Can't use character count here.
    size_t utf8_len = (str.length() + 1) * sizeof(char32_t);
    char* utf8_str = console::SDL_iconv_string("UTF-8", "UTF-32LE", strp, utf8_len);
    if (!utf8_str)
        return "?u8?";

    std::string result(utf8_str);
    console::SDL_free(utf8_str);
    return result;
}

#if 0
static std::u32string from_utf8(const char* s)
{
    int char_len = SDL_utf8strlen(s);
    if (char_len == 0)
        return U"";
    char32_t* conv_bytes = reinterpret_cast<char32_t*>(SDL_iconv_string("UTF32", "UTF-8", s, char_len + 1));
    if (!conv_bytes)
        return U"";

    std::u32string result = (*conv_bytes == 0xFEFF) ? conv_bytes + 1 : conv_bytes;
    SDL_free(reinterpret_cast<char*>(conv_bytes));
    return result;
}
#endif

static std::u32string from_utf8(const char* str)
{
    int char_len = utf8_strlen(str);
    if (char_len == 0)
        return U"";

    char* char32_bytes = SDL_iconv_string("UTF-32LE", "UTF-8", str, (char_len + 1) * sizeof(char32_t));
    if (!char32_bytes)
        return U"?u8?";

    std::u32string result(reinterpret_cast<char32_t*>(char32_bytes));
    SDL_free(char32_bytes);
    return result;
}

// For testing purposes, to be removed
static const std::unordered_map<char32_t, uint8_t> unicode_to_cp437 = {
    // Control characters and symbols
    /* NULL           */ { U'\u263A', 0x01 }, { U'\u263B', 0x02 }, { U'\u2665', 0x03 },
    { U'\u2666', 0x04 }, { U'\u2663', 0x05 }, { U'\u2660', 0x06 }, { U'\u2022', 0x07 },
    { U'\u25D8', 0x08 }, { U'\u25CB', 0x09 }, { U'\u25D9', 0x0A }, { U'\u2642', 0x0B },
    { U'\u2640', 0x0C }, { U'\u266A', 0x0D }, { U'\u266B', 0x0E }, { U'\u263C', 0x0F },

    { U'\u25BA', 0x10 }, { U'\u25C4', 0x11 }, { U'\u2195', 0x12 }, { U'\u203C', 0x13 },
    { U'\u00B6', 0x14 }, { U'\u00A7', 0x15 }, { U'\u25AC', 0x16 }, { U'\u21A8', 0x17 },
    { U'\u2191', 0x18 }, { U'\u2193', 0x19 }, { U'\u2192', 0x1A }, { U'\u2190', 0x1B },
    { U'\u221F', 0x1C }, { U'\u2194', 0x1D }, { U'\u25B2', 0x1E }, { U'\u25BC', 0x1F },

    // ASCII, no mapping needed

    // Extended Latin characters and others
    { U'\u2302', 0x7F },

    { U'\u00C7', 0x80 }, { U'\u00FC', 0x81 }, { U'\u00E9', 0x82 }, { U'\u00E2', 0x83 },
    { U'\u00E4', 0x84 }, { U'\u00E0', 0x85 }, { U'\u00E5', 0x86 }, { U'\u00E7', 0x87 },
    { U'\u00EA', 0x88 }, { U'\u00EB', 0x89 }, { U'\u00E8', 0x8A }, { U'\u00EF', 0x8B },
    { U'\u00EE', 0x8C }, { U'\u00EC', 0x8D }, { U'\u00C4', 0x8E }, { U'\u00C5', 0x8F },

    { U'\u00C9', 0x90 }, { U'\u00E6', 0x91 }, { U'\u00C6', 0x92 }, { U'\u00F4', 0x93 },
    { U'\u00F6', 0x94 }, { U'\u00F2', 0x95 }, { U'\u00FB', 0x96 }, { U'\u00F9', 0x97 },
    { U'\u00FF', 0x98 }, { U'\u00D6', 0x99 }, { U'\u00DC', 0x9A }, { U'\u00A2', 0x9B },
    { U'\u00A3', 0x9C }, { U'\u00A5', 0x9D }, { U'\u20A7', 0x9E }, { U'\u0192', 0x9F },

    { U'\u00E1', 0xA0 }, { U'\u00ED', 0xA1 }, { U'\u00F3', 0xA2 }, { U'\u00FA', 0xA3 },
    { U'\u00F1', 0xA4 }, { U'\u00D1', 0xA5 }, { U'\u00AA', 0xA6 }, { U'\u00BA', 0xA7 },
    { U'\u00BF', 0xA8 }, { U'\u2310', 0xA9 }, { U'\u00AC', 0xAA }, { U'\u00BD', 0xAB },
    { U'\u00BC', 0xAC }, { U'\u00A1', 0xAD }, { U'\u00AB', 0xAE }, { U'\u00BB', 0xAF },

    // Box drawing characters
    { U'\u2591', 0xB0 }, { U'\u2592', 0xB1 }, { U'\u2593', 0xB2 }, { U'\u2502', 0xB3 },
    { U'\u2524', 0xB4 }, { U'\u2561', 0xB5 }, { U'\u2562', 0xB6 }, { U'\u2556', 0xB7 },
    { U'\u2555', 0xB8 }, { U'\u2563', 0xB9 }, { U'\u2551', 0xBA }, { U'\u2557', 0xBB },
    { U'\u255D', 0xBC }, { U'\u255C', 0xBD }, { U'\u255B', 0xBE }, { U'\u2510', 0xBF },

    { U'\u2514', 0xC0 }, { U'\u2534', 0xC1 }, { U'\u252C', 0xC2 }, { U'\u251C', 0xC3 },
    { U'\u2500', 0xC4 }, { U'\u253C', 0xC5 }, { U'\u255E', 0xC6 }, { U'\u255F', 0xC7 },
    { U'\u255A', 0xC8 }, { U'\u2554', 0xC9 }, { U'\u2569', 0xCA }, { U'\u2566', 0xCB },
    { U'\u2560', 0xCC }, { U'\u2550', 0xCD }, { U'\u256C', 0xCE }, { U'\u2567', 0xCF },

    { U'\u2568', 0xD0 }, { U'\u2564', 0xD1 }, { U'\u2565', 0xD2 }, { U'\u2559', 0xD3 },
    { U'\u2558', 0xD4 }, { U'\u2552', 0xD5 }, { U'\u2553', 0xD6 }, { U'\u256B', 0xD7 },
    { U'\u256A', 0xD8 }, { U'\u2518', 0xD9 }, { U'\u250C', 0xDA }, { U'\u2588', 0xDB },
    { U'\u2584', 0xDC }, { U'\u258C', 0xDD }, { U'\u2590', 0xDE }, { U'\u2580', 0xDF },

    // Mathematical symbols and others
    { U'\u03B1', 0xE0 }, { U'\u00DF', 0xE1 }, { U'\u0393', 0xE2 }, { U'\u03C0', 0xE3 },
    { U'\u03A3', 0xE4 }, { U'\u03C3', 0xE5 }, { U'\u00B5', 0xE6 }, { U'\u03C4', 0xE7 },
    { U'\u03A6', 0xE8 }, { U'\u0398', 0xE9 }, { U'\u03A9', 0xEA }, { U'\u03B4', 0xEB },
    { U'\u221E', 0xEC }, { U'\u03C6', 0xED }, { U'\u03B5', 0xEE }, { U'\u2229', 0xEF },

    { U'\u2261', 0xF0 }, { U'\u00B1', 0xF1 }, { U'\u2265', 0xF2 }, { U'\u2264', 0xF3 },
    { U'\u2320', 0xF4 }, { U'\u2321', 0xF5 }, { U'\u00F7', 0xF6 }, { U'\u2248', 0xF7 },
    { U'\u00B0', 0xF8 }, { U'\u2219', 0xF9 }, { U'\u00B7', 0xFA }, { U'\u221A', 0xFB },
    { U'\u207F', 0xFC }, { U'\u00B2', 0xFD }, { U'\u25A0', 0xFE }, { U'\u00A0', 0xFF }
};

void center_rect(SDL_Rect& r)
{
    r.x = r.x - r.w / 2;
    r.y = r.y - r.h / 2;
}

int snap_to_min(int value, int grid_size)
{
    return std::floor(static_cast<float>(value) / grid_size) * grid_size;
}

int snap_to_max(int value, int grid_size)
{
    return std::ceil(static_cast<float>(value) / grid_size) * grid_size;
}

enum class ScrollDirection {
    up,
    down,
    page_up,
    page_down
};

enum class State {
    active,
    shutdown,
    inactive
};

enum class ExternalEventType {
    sdl,
    api,
};
/* Nneed type promotion to uint32
 * to use with SDL_EventType. SDL_EventType is uint32, but
 * only goes up to uint16 for use by its internal arrays. This leaves
 * plenty of room for custom types.
 */
struct InternalEventType {
    enum Type : Uint32 {
        new_input_line = SDL_LASTEVENT + 1,
        clicked,
        font_size_changed,
        range_changed,
        value_changed
    };
};

enum class EntryType {
    input,
    output
};

namespace colors {
    // Default palette. Needs more. Needs configurable.
    const SDL_Color white = { 255, 255, 255, 255 };
    const SDL_Color lightgray = { 211, 211, 211, 255 };
    const SDL_Color mediumgray = { 65, 65, 65, 255 };
    const SDL_Color charcoal = { 54, 69, 79, 255 };
    const SDL_Color darkgray = { 27, 27, 27, 255 };
}

bool in_rect(int x, int y, SDL_Rect& r);
bool in_rect(SDL_Point& p, SDL_Rect& r);

void render_texture(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& dst);

int set_draw_color(SDL_Renderer*, const SDL_Color&);

struct Widget;
SDL_Texture* create_text_texture(Widget&, const std::u32string&, const SDL_Color&);

struct LogEntry;
void split_entry_text(
    Widget& widget,
    LogEntry& entry,
    std::u32string& text);

struct ISlot {
    virtual ~ISlot() = default;
    virtual void invoke(SDL_Event& event) = 0;
    virtual void disconnect() = 0;
    virtual void connect() = 0;
    virtual bool is_connected() = 0;
};

struct ISignal {
    virtual ~ISignal() = default;
    // todo for connect()
    virtual void disconnect(Uint32 event_type, ISlot* slot) = 0;
    virtual void reconnect(Uint32 event_type, ISlot* slot) = 0;
    virtual bool is_connected(Uint32 event_type, ISlot* slot) = 0;
};

template <typename EventType>
class Slot : public ISlot {
public:
    using Func = std::function<void(EventType&)>;

    Slot(ISignal& emitter, Uint32 event_type, Func& func)
        : emitter_(emitter)
        , event_type_(event_type)
        , func_(func)
    {
    }

    void invoke(SDL_Event& event) override
    {
        func_(get_event(event));
    }

    void disconnect() override
    {
        emitter_.disconnect(event_type_, this);
    }

    void connect() override
    {
        emitter_.reconnect(event_type_, this);
    }

    bool is_connected() override
    {
        return emitter_.is_connected(event_type_, this);
    }

    ~Slot()
    {
    }

private:
    ISignal& emitter_;
    Uint32 event_type_;
    Func func_;

    EventType& get_event(SDL_Event& event)
    {
        if constexpr (std::is_same_v<EventType, SDL_KeyboardEvent>) {
            return event.key;
        } else if constexpr (std::is_same_v<EventType, SDL_MouseButtonEvent>) {
            return event.button;
        } else if constexpr (std::is_same_v<EventType, SDL_MouseMotionEvent>) {
            return event.motion;
        } else if constexpr (std::is_same_v<EventType, SDL_UserEvent>) {
            return event.user;
        } else if constexpr (std::is_same_v<EventType, SDL_TextInputEvent>) {
            return event.text;
        } else if constexpr (std::is_same_v<EventType, SDL_MouseWheelEvent>) {
            return event.wheel;
        } else if constexpr (std::is_same_v<EventType, SDL_WindowEvent>) {
            return event.window;
        } else {
            static_assert(std::is_same_v<EventType, void>, "Unsupported event type");
        }
    }
};

class SignalEmitter : public ISignal {
public:
    template <typename EventType>
    ISlot* connect(Uint32 event_type, typename Slot<EventType>::Func func)
    {
        auto slot = std::make_unique<Slot<EventType>>(*this, event_type, func);
        return slots_[event_type].emplace_back(std::move(slot)).get();
    }

    template <typename EventType>
    ISlot* connect_later(Uint32 event_type, typename Slot<EventType>::Func func)
    {
        auto slot = std::make_unique<Slot<EventType>>(*this, event_type, func);
        return disconnected_slots_[event_type].emplace_back(std::move(slot)).get();
    }

    void disconnect(Uint32 event_type, ISlot* slot) override
    {
        auto& disconnected_slots = disconnected_slots_[event_type];
        auto it = std::ranges::find_if(disconnected_slots, [slot](const std::unique_ptr<ISlot>& s) {
            return s.get() == slot;
        });

        if (it != disconnected_slots.end()) {
            disconnected_slots.emplace_back(std::move(*it));
            slots_[event_type].erase(it);
        }
    }

    void reconnect(Uint32 event_type, ISlot* slot) override
    {
        auto& disconnected_slots = disconnected_slots_[event_type];
        auto it = std::ranges::find_if(disconnected_slots, [slot](const std::unique_ptr<ISlot>& s) {
            return s.get() == slot;
        });

        if (it != disconnected_slots.end()) {
            slots_[event_type].emplace_back(std::move(*it));
            disconnected_slots.erase(it);
        }
    }

    bool is_connected(Uint32 event_type, ISlot* slot) override
    {
        return std::ranges::any_of(slots_[event_type], [slot](const std::unique_ptr<ISlot>& s) {
            return s.get() == slot;
        });
    }

    void emit(SDL_Event& event)
    {
        auto it = slots_.find(event.type);
        if (it != slots_.end()) {
            for (auto& slot : it->second) {
                slot->invoke(event);
            }
        }
    }

    void emit(InternalEventType::Type type)
    {
        SDL_Event e = make_sdl_user_event(type, nullptr);
        emit(e);
    }

    void emit(InternalEventType::Type type, void* data1)
    {
        SDL_Event e = make_sdl_user_event(type, data1);
        emit(e);
    }

    void clear()
    {
        slots_.clear();
        disconnected_slots_.clear();
    }

    static SDL_Event make_sdl_user_event(InternalEventType::Type type, void* data1)
    {
        SDL_Event event;
        console::SDL_zero(event);
        event.type = type;
        event.user.data1 = data1;
        return event;
    }

private:
    using Container = std::vector<std::unique_ptr<ISlot>>;
    std::map<Uint32, Container> slots_;
    std::map<Uint32, Container> disconnected_slots_;
};

// For internal communication.
class EventEmitter {
public:
    using Slot = std::function<void(SDL_Event&)>;

    Slot* connect(Uint32 event_type, const Slot& func)
    {
        return &slots[event_type].emplace_back(func);
    }

    void disconnect(Uint32 event_type, const Slot* slot)
    {
        auto it = slots.find(event_type);
        if (it != slots.end()) {
            auto& cont = it->second;

            cont.erase(std::remove_if(cont.begin(), cont.end(),
                           [slot](const Slot& s) {
                               return &s == slot;
                           }),
                cont.end());
        }
    }

    void emit(SDL_Event& event)
    {
        auto it = slots.find(event.type);
        if (it != slots.end()) {
            for (auto& slot : it->second) {
                slot(event);
            }
        }
    }

    void emit(InternalEventType::Type type)
    {
        SDL_Event e = make_sdl_user_event(type, nullptr);
        emit(e);
    }

    void emit(InternalEventType::Type type, void* data1)
    {
        SDL_Event e = make_sdl_user_event(type, data1);
        emit(e);
    }

    void clear()
    {
        slots.clear();
    }

    static SDL_Event make_sdl_user_event(InternalEventType::Type type, void* data1)
    {
        SDL_Event event;
        console::SDL_zero(event);
        event.type = type;
        event.user.data1 = data1;
        return event;
    }

    EventEmitter() = default;

    ~EventEmitter()
    {
    }

    EventEmitter(EventEmitter&& other) noexcept
        : slots(std::move(other.slots))
    {
    }

    EventEmitter& operator=(EventEmitter&& other) noexcept
    {
        if (this != &other) {
            slots = std::move(other.slots);
        }
        return *this;
    }

    EventEmitter(const EventEmitter&) = delete;
    EventEmitter& operator=(const EventEmitter&) = delete;

private:
    std::map<Uint32, std::deque<Slot>> slots;
    // std::unordered_map<Uint32, std::deque<EventHandler<SDL_Event>>> slots;
};

struct Property {
    using Value = std::variant<std::string, int>;

    void set_property(const std::string& name, const Value& value)
    {
        properties[name] = value;
    }

    Value get_property(const std::string& name, const Value& default_value = {}) const
    {
        auto it = properties.find(name);
        if (it != properties.end()) {
            return it->second;
        }
        return default_value;
    }

    void print_property(const std::string& name) const
    {
        auto it = properties.find(name);
        if (it != properties.end()) {
            std::visit([](auto&& value) {
                ;
            },
                it->second);
        } else {
            std::cout << "Property not found!" << std::endl;
        }
    }

private:
    std::unordered_map<std::string, Value> properties;
    std::recursive_mutex m;
};

struct WrappedText {
    std::u32string_view text; // text of line segment
    size_t index; // line index into entries
    size_t start_index; // index into entry text, still needed?
    size_t end_index; // index into entry text, still needed?
    SDL_Point coord {};

    WrappedText(std::u32string_view text, size_t line_index, size_t start_index, size_t end_index)
        : text(text)
        , index(line_index)
        , start_index(start_index)
        , end_index(end_index) {};

    ~WrappedText()
    {
    }

    WrappedText(const WrappedText&) = delete;
    WrappedText& operator=(const WrappedText&) = delete;
};

using LogEntryLines = std::deque<WrappedText>;
struct LogEntry {
    EntryType type;
    // Original text.
    std::u32string text;
    SDL_Rect rect {};
    size_t size { 0 }; // total # of lines

    LogEntry() {};

    ~LogEntry()
    {
    }

    LogEntry(EntryType type, const std::u32string& text)
        : type(type)
        , text(text) {};

    auto& add_line(std::u32string_view segment, size_t start_index, size_t end_index)
    {
        return lines_.emplace_back(segment, size++, start_index, end_index);
    }

    void clear()
    {
        size = 0;
        lines_.clear();
    }

    LogEntryLines& lines()
    {
        return lines_;
    }

    LogEntry(const LogEntry&) = delete;
    LogEntry& operator=(const LogEntry&) = delete;

private:
    LogEntryLines lines_;
};

struct Glyph {
    SDL_Rect rect;
};
struct FontLoader;
// XXX, TODO: cleanup.
struct Font : public SignalEmitter {
    FontLoader& loader;
    SDL_Texture* texture;
    std::vector<Glyph> glyphs;
    int char_width;
    int line_height;
    int vertical_spacing;
    float scale_factor { 1 };
    int orig_char_width;
    int orig_line_height;
    int size_delta { 2 };

    Font(FontLoader& loader, SDL_Texture* texture, std::vector<Glyph>& glyphs, int char_width, int line_height)
        : loader(loader)
        , texture(texture)
        , glyphs(glyphs)
        , char_width(char_width)
        , line_height(line_height)
    {
        this->char_width = char_width;
        this->line_height = line_height;
        this->vertical_spacing = line_height * 0.5;
        orig_char_width = this->char_width;
        orig_line_height = this->line_height;
    }

    ~Font()
    {
    }

    void render(SDL_Renderer* renderer, const std::u32string_view& text, int x, int y)
    {
        for (auto& ch : text) {
            char32_t index;
            if (ch <= 127)
                index = ch;
            else {
                index = unicode_glyph_index(ch);
            }
            Glyph& g = glyphs[index];
            SDL_Rect dst = { x, y + (vertical_spacing / 2), (int)(g.rect.w * scale_factor), (int)((g.rect.h * scale_factor)) };
            x += g.rect.w * scale_factor;
            console::SDL_RenderCopy(renderer, texture, &g.rect, &dst);
        }
    }

    // Get the surface size of a text.
    // Mono-spaced faces have the equal widths and heights.
    void size_text(const std::u32string& s, int& w, int& h)
    {
        w = s.length() * char_width;
        h = line_height + vertical_spacing;
    }

    int line_height_with_spacing() {
        return line_height + vertical_spacing;
    }

    void incr_size()
    {
        change_size(size_delta);
        emit(InternalEventType::font_size_changed);
    }

    void decr_size()
    {
        change_size(-size_delta);
        emit(InternalEventType::font_size_changed);
    }

    char32_t unicode_glyph_index(const char32_t ch)
    {
        auto it = unicode_to_cp437.find(ch);
        if (it != unicode_to_cp437.end()) {
            return it->second;
        }
        return '?';
    }

    Font(Font&& other) noexcept
        : loader(other.loader)
        , texture(other.texture)
        , glyphs(other.glyphs)
        , char_width(other.char_width)
        , line_height(other.line_height)
        , vertical_spacing(other.vertical_spacing)
        , scale_factor(other.scale_factor)
        , orig_char_width(other.orig_char_width)
        , orig_line_height(other.orig_line_height)
    {
    }

    Font& operator=(Font&& other) noexcept
    {
        if (this != &other) {
            texture = other.texture;
            glyphs = other.glyphs;
            char_width = other.char_width;
            line_height = other.line_height;
            vertical_spacing = other.vertical_spacing;
            scale_factor = other.scale_factor;
            orig_char_width = other.char_width;
            orig_line_height = other.line_height;
        }
        return *this;
    }

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

private:
    void change_size(int delta)
    {
        scale_factor = (float)(char_width + delta) / orig_char_width;
        char_width = orig_char_width * scale_factor;
        line_height = (orig_line_height) * scale_factor;
    }
};

using FontMap = std::map<std::pair<std::string, int>, Font>;
struct FontLoader {
    FontLoader(SDL_Renderer* renderer)
        : renderer(renderer)
    {
#if 0
        if (TTF_Init()) {
            throw std::runtime_error(TTF_GetError());
        }
#endif
    }

    ~FontLoader()
    {
#if 0
        for (auto& pair : fmap) {
            if (pair.second.handle) {
                TTF_CloseFont(pair.second.handle);
            }
        }

        TTF_Quit();
#endif
    }

    Font* open(const std::string& path, int size)
    {
#if 0
        auto key = std::make_pair(path, size);
        auto it = fmap.find(key);

        if (it != fmap.end()) {
            return &it->second;
        }

        auto* handle = TTF_OpenFont(path.c_str(), size);
        if (!handle) {
            return nullptr;
        }

        int char_width;
        int line_height;
        if (TTF_SizeUTF8(handle, "a", &char_width, &line_height)) {
            TTF_CloseFont(handle);
            return nullptr;
        }

        auto result = fmap.emplace(key, Font(*this, handle, char_width, line_height));
        return &result.first->second;
#endif
        return nullptr;
    }

    Font* default_font()
    {
        return &fmap.begin()->second;
    }

    FontLoader(const FontLoader&) = delete;
    FontLoader& operator=(const FontLoader&) = delete;

    FontLoader(FontLoader&& other) noexcept
        : fmap(std::move(other.fmap))
        , renderer(other.renderer)
        , textures(std::move(other.textures))
    {
    }

    FontLoader& operator=(FontLoader&& other) noexcept
    {
        if (this != &other) {
            fmap = std::move(other.fmap);
            renderer = other.renderer;
            textures = std::move(other.textures);
        }
        return *this;
    }

protected:
    FontMap fmap;
    SDL_Renderer* renderer;
    std::vector<SDL_Texture*> textures;
};

struct BMPFontLoader : public FontLoader {
    BMPFontLoader(SDL_Renderer* renderer)
        : FontLoader(renderer)
    {
    }

    ~BMPFontLoader()
    {
        for (auto tex : textures) {
            SDL_DestroyTexture(tex);
        }
    }

    Font* open(const std::string& path, int size)
    {
        auto key = std::make_pair(path, size);
        auto it = fmap.find(key);

        if (it != fmap.end()) {
            return &it->second;
        }

        // SDL_Surface* glyph_surface = SDL_LoadBMP(path.c_str());
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (surface == nullptr) {
            return nullptr;
        }

        console::SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        //  XXX: hardcoded magenta
        Uint32 bg_color = console::SDL_MapRGB(surface->format, 255, 0, 255);
        console::SDL_SetColorKey(surface, SDL_TRUE, bg_color);

        std::vector<Glyph> glyphs;
        glyphs = build_glyph_rects(surface->pitch, surface->h, 16, 16);

        // Alpha mask needs setting. FIXME: shouldn't to create a new surface?
        SDL_Surface* conv_surface = console::SDL_CreateRGBSurface(0, surface->pitch, surface->h, 32,
            0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        if (!conv_surface)
            return nullptr;

        console::SDL_BlitSurface(surface, NULL, conv_surface, NULL);
        console::SDL_FreeSurface(surface);

        SDL_Texture* texture = console::SDL_CreateTextureFromSurface(renderer, conv_surface);
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface Error: " << console::SDL_GetError() << std::endl;
        }
        console::SDL_FreeSurface(conv_surface);
        console::SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        textures.emplace_back(texture);

        // FIXME: hardcoded
        auto result = fmap.emplace(key, Font(*this, texture, glyphs, 8, 12));
        return &result.first->second;
    }

    std::vector<Glyph> build_glyph_rects(int sheet_w, int sheet_h, int columns, int rows)
    {
        int tile_w = sheet_w / columns;
        int tile_h = sheet_h / rows;
        int total_glyphs = rows * columns;

        std::vector<Glyph> glyphs;
        glyphs.reserve(rows * columns);
        for (int i = 0; i < total_glyphs; ++i) {
            int r = i / rows;
            int c = i % columns;
            Glyph glyph;
            glyph.rect = { tile_w * c, tile_h * r, tile_w, tile_h };
            glyphs.push_back(glyph);
        }
        return glyphs;
    }

    Font* get_font()
    {
        return &fmap.begin()->second;
    }

    BMPFontLoader(const BMPFontLoader&) = delete;
    BMPFontLoader& operator=(const BMPFontLoader&) = delete;

    BMPFontLoader& operator=(BMPFontLoader&& other) noexcept
    {
        if (this != &other) {
            fmap = std::move(other.fmap);
            renderer = std::move(other.renderer);
            textures = std::move(other.textures);
        }
        return *this;
    }
};

struct MainWindow;
struct WidgetContext {
    WidgetContext(SDL_Renderer* r, SignalEmitter* em, SDL_Point& mouse)
        : renderer(r)
        , global_emitter(em)
        , mouse_coord(mouse)
    {
    }
    SDL_Renderer* renderer;
    SignalEmitter* global_emitter;
    SDL_Point& mouse_coord;
};

// TODO: needs work
struct Widget : public SignalEmitter {
public:
    Widget* parent;
    Font* font;
    SDL_Rect viewport {};

    Widget(Widget* parent)
        : parent(parent)
        , font(parent->font)
        , viewport(parent->viewport)
        , context(parent->context)
    {
    }

    Widget(Widget* parent, SDL_Rect viewport)
        : parent(parent)
        , font(parent->font)
        , viewport(viewport)
        , context(parent->context)
    {
    }

    // Constructor for Window
    Widget(Font* font, WidgetContext& context, SDL_Rect viewport)
        : parent(nullptr)
        , font(font)
        , viewport(viewport)
        , context(context)

    {
    }

    SDL_Renderer* renderer()
    {
        return context.renderer;
    }

    SDL_Point mouse_coord()
    {
        return context.mouse_coord;
    }

    void set_font(const std::string& file, const int size)
    {
        // XXX: check for error
        font = font->loader.open(file, size);
    }

    /*
    const EventEmitter::Slot* connect(Uint32 event_type, const EventEmitter::Slot& func)
    {
        return emitter.connect(event_type, func);
    }*/

    template <typename EventType, typename... Args>
    ISlot* connect_global(Args&&... args)
    {
        return context.global_emitter->connect<EventType>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void disconnect_global(Args&&... args)
    {
        context.global_emitter->disconnect(std::forward<Args>(args)...);
    }

    /*
    template <typename... Args>
    void emit(Args&&... args)
    {
        emitter.emit(std::forward<Args>(args)...);
    }*/

    template <typename... Args>
    void emit_global(Args&&... args)
    {
        context.global_emitter->emit(std::forward<Args>(args)...);
    }

    virtual void render() {};
    virtual void set_viewport(SDL_Rect new_viewport) {
        // viewport = new_viewport;
    };
    virtual void on_resize(SDL_Rect new_viewport) {};

    virtual ~Widget() { }

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    WidgetContext& context;

private:
    EventEmitter emitter;
};

struct Prompt : public Widget {
    Prompt(Widget* parent)
        : Widget(parent)
    {
        input = &history.emplace_back(U"");
        prompt_text = U"> ";
        // Create 1x1 texture for the cursor, it will be stretched to fit the font's line height and character width
        cursor_texture = console::SDL_CreateTexture(renderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
        if (cursor_texture == nullptr)
            throw(std::runtime_error(console::SDL_GetError()));

        // FFFFFF = rgb white, 7F = 50% transparant
        Uint32 pixel = 0xFFFFFF7F;
        console::SDL_UpdateTexture(cursor_texture, NULL, &pixel, sizeof(Uint32));
        // For transparancy
        console::SDL_SetTextureBlendMode(cursor_texture, SDL_BLENDMODE_BLEND);

        connect_global<SDL_KeyboardEvent>(SDL_KEYDOWN, [this](SDL_KeyboardEvent& e) {
            on_key_down(e);
        });

        connect_global<SDL_TextInputEvent>(SDL_TEXTINPUT, [this](SDL_TextInputEvent& e) { add_input(from_utf8(e.text)); });
    }

    ~Prompt()
    {
        console::SDL_DestroyTexture(cursor_texture);
    }

    void on_key_down(const SDL_KeyboardEvent& e)
    {
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_BACKSPACE:
            erase_input();
            break;

        case SDLK_UP:
            set_input_from_history(ScrollDirection::up);
            break;

        case SDLK_DOWN:
            set_input_from_history(ScrollDirection::down);
            break;

        case SDLK_LEFT:
            move_cursor_left();
            break;

        case SDLK_RIGHT:
            move_cursor_right();
            break;
        case SDLK_HOME:
            cursor = 0;
            break;
        case SDLK_END:
            cursor = input->length();
            break;
        }
    }

    void set_prompt(const std::u32string& str)
    {
        prompt_text = str;
        update_entry();
    }

    /*
     * Set the current line. We can go UP (next) or DOWN (previous) through the
     * lines. This function essentially acts as a history viewer. This function
     * will skip lines with zero length. The cursor is always set to the length of
     * the line's input.
     */
    void set_input_from_history(const ScrollDirection dir)
    {
        size_t idx = history_idx;
        if (dir == ScrollDirection::up) {
            if (idx > 0)
                idx--;
            else
                return;
        } else {
            if (idx < history.size() - 1)
                idx++;
            else
                return;
        }

        history_idx = idx;
        input = &history[idx];
        cursor = input->length();
        rebuild = true;
    }

    void add_input(const std::u32string& str)
    {
        /* if cursor is at end of line, it's a simple concatenation */
        if (cursor == input->length()) {
            *input += str;
        } else {
            /* else insert text into line at cursor's index */
            input->insert(cursor, str);
        }
        cursor += str.length();
        rebuild = true;
    }

    void erase_input()
    {
        if (cursor == 0 || input->length() == 0)
            return;

        if (input->length() == cursor) {
            input->pop_back();
        } else {
            /* else shift the text from cursor left by one character */
            input->erase(cursor, 1);
        }
        cursor -= 1;
        rebuild = true;
    }

    void move_cursor_left()
    {
        if (cursor > 0) {
            cursor--;
        }
    }

    void move_cursor_right()
    {
        if (cursor < input->length()) {
            cursor++;
        }
    }

    void on_resize(SDL_Rect new_viewport) override
    {
        viewport = parent->viewport;
        update_entry();
    }

    void maybe_rebuild()
    {
        if (rebuild) {
            update_entry();
            rebuild = false;
        }
    }

    void update_entry()
    {
        std::u32string str = prompt_text + *input;
        split_entry_text(*this, entry, str);
    }

    void render_cursor(int scroll_value)
    {
        if (entry.lines().empty())
            return;

        /* cursor's position */
        int offset = prompt_text.length();
        auto cursor_len = cursor + offset;

        auto* line = &entry.lines().back();
        // WrappedText* line = nullptr;
        //  XXX: fix doing needless work on render
        for (auto& l : entry.lines()) {
            // std::cerr << "render_cursor(loop): cursor_len: " << cursor_len << " start_idx: " << l.start_index << " end_idx: " << l.end_index << std::endl;
            if (cursor_len >= l.start_index && cursor_len <= l.end_index) {
                line = &l;
                break;
            }
        }

        if (!line) {
            return;
        }

        // one based. reverse the row so that last = 0
        // scroll_offset starts at 0.
        int r = (entry.size - 1) - line->index;
        if (scroll_value > r) {
            //   std::cerr << "render_cursor: scroll_value: " << scroll_value << " row: " << r << std::endl;
            return;
        }

        const auto lh = font->line_height_with_spacing();
        const auto cw = font->char_width;
        /*  full range of line + cursor */
        int cx = (cursor_len - line->start_index) * cw;
        int cy = line->coord.y;

        // std::cerr << "render_cursor: cursor_len: " << cursor_len << " start_idx: " << line->start_index << " cx:" << cx << " y:" << cy << std::endl;

        SDL_Rect rect = { cx, cy, cw, lh };
        /* Draw the cursor */
        // No, not for this, but maybe used to pen text
        // SDL_SetTextureBlendMode(cursor_texture, SDL_BLENDMODE_BLEND);
        // SDL_SetTextureAlphaMod(cursor_texture, 0.5 * 255);
        // SDL_SetTextureColorMod(cursor_texture, 255, 255, 255);
        render_texture(renderer(), cursor_texture, rect);
    }

    Prompt(const Prompt&) = delete;
    Prompt& operator=(const Prompt&) = delete;

    // Holds wrapped lines from input
    LogEntry entry;
    // The text of the prompt itself.
    std::u32string prompt_text;
    // The input portion of the prompt.
    std::u32string* input;
    // Prompt text was changed flag
    bool rebuild { true };
    size_t cursor { 0 }; // position of cursor within an entry
    // 1x1 texture stretched to font's single character dimensions
    SDL_Texture* cursor_texture;
    /*
     * For input history.
     * use deque to hold a stable reference.
     */
    std::deque<std::u32string> history;
    int history_idx;
};

struct Scrollbar : public Widget {
private:
    int page_size;
    int max_range_value { 0 };
    int range_value { 0 };
    bool depressed { false };
    SDL_Rect thumb_rect {};
    ISlot* slot_SDL_MouseMotionEvent { nullptr };

public:
    Scrollbar(Widget* parent, int page_size)
        : Widget(parent)
        , page_size(page_size)
    {
        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](SDL_MouseButtonEvent& e) {
            this->on_SDL_MouseButtonDown(e);
        });

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](SDL_MouseButtonEvent& e) {
            this->on_SDL_MouseButtonUp(e);
        });

        slot_SDL_MouseMotionEvent = context.global_emitter->connect_later<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](SDL_MouseMotionEvent& e) {
            if (!depressed)
                return;

            int y = e.y;
            range_value = range_value_from_track_position(y);
            set_thumb_position(y);
            emit(InternalEventType::value_changed, &range_value);
        });

        thumb_rect = viewport;
        set_thumb_height();
    }

    void on_SDL_MouseButtonDown(SDL_MouseButtonEvent& e)
    {
        if (!in_rect(e.x, e.y, viewport)) {
            return;
        }

        if (!slot_SDL_MouseMotionEvent->is_connected()) {
            slot_SDL_MouseMotionEvent->connect();
        }

        depressed = true;
        set_thumb_position(e.y);
        range_value = range_value_from_track_position(e.y);
        emit(InternalEventType::value_changed, &range_value);
        // std::cerr << "y=" << e.y << "," << "range_value=" << range_value << std::endl;
    }

    void on_SDL_MouseButtonUp(SDL_MouseButtonEvent& e)
    {
        if (depressed) {
            depressed = false;
            slot_SDL_MouseMotionEvent->disconnect();
        }
    }

    void set_page_size(int size)
    {
        page_size = size;
    }

    void set_range(int value)
    {
        max_range_value = value;
        set_thumb_height();
        // std::cerr << "max_range_value: " << value << " thumb_h: " << thumb_rect.h << " page_size: " << page_size << std::endl;
    }

    void set_value(int value)
    {
        bool ok_to_zero = true;
        // value is increasing, so don't snap back to zero
        if (value > range_value)
            ok_to_zero = false;
        range_value = value;
        set_thumb_position(track_position_from_range_value(), ok_to_zero);
        // std::cerr << "value: " << value << " trackpos: " << track_position_from_range_value() << std::endl;
    }

    void set_viewport(SDL_Rect new_viewport) override
    {
        viewport = new_viewport;
        thumb_rect = viewport;
        set_thumb_height();
        set_thumb_position(track_position_from_range_value(), true);
    }

    void render() override
    {
        set_draw_color(renderer(), colors::white);

        SDL_RenderDrawRect(renderer(), &viewport);
        SDL_RenderFillRect(renderer(), &thumb_rect);

        set_draw_color(renderer(), colors::darkgray);
    }

    ~Scrollbar()
    {
    }

    Scrollbar(const Scrollbar&) = delete;
    Scrollbar& operator=(const Scrollbar&) = delete;

private:
    void set_thumb_position(int y, bool ok_to_zero = true)
    {
        int track_start = viewport.y;
        int track_end = viewport.y + viewport.h;

        // thumb position, centering around the click
        thumb_rect.y = std::max(track_start, y - thumb_rect.h / 2);

        // Prevent thumb from going beyond the top of the track
        if (thumb_rect.y < track_start) {
            thumb_rect.y = track_start;
        }

        // Prevent thumb from going beyond the bottom of the track
        if (thumb_rect.y + thumb_rect.h > track_end) {
            thumb_rect.y = track_end - thumb_rect.h;
            if (ok_to_zero) {
                range_value = 0;
                emit(InternalEventType::value_changed, &range_value);
            }
        }
    }

    void set_thumb_height()
    {
        float thumb_ratio = static_cast<float>(page_size) / (max_range_value - range_value);
        int thumb_size = static_cast<int>(std::round(thumb_ratio * viewport.h));
        thumb_rect.h = std::min(viewport.h, std::max(thumb_size, 10)); // Thumb is at least a minimum size
    }

    int range_value_from_track_position(int y)
    {
        int track_h = viewport.h;
        float y_ratio = static_cast<float>(y) / track_h;
        int val = static_cast<int>((1.0f - y_ratio) * max_range_value);

        // Ensure the scroll offset does not go beyond the valid range
        val = std::max(0, std::min(val, static_cast<int>(max_range_value)));

        return val;
    }

    int track_position_from_range_value()
    {
        int track_h = viewport.h;
        // Calculate the scroll ratio based on the scroll_offset
        float value_ratio = static_cast<float>(range_value) / max_range_value;

        int y = static_cast<int>((1.0f - value_ratio) * track_h);

        return y + viewport.y;
    }
};

struct Button : public Widget {
public:
    Button(Widget* parent, std::u32string& label, SDL_Color color)
        : Widget(parent)
        , label(label)
    {
        size_text();
        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](SDL_MouseButtonEvent& e) {
            this->on_mouse_button_down(e);
        });

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](SDL_MouseButtonEvent& e) {
            this->on_mouse_button_up(e);
        });

        font->connect<SDL_UserEvent>(InternalEventType::font_size_changed, [this](SDL_UserEvent& e) {
            size_text();
            std::cerr << "font_size_changed" << std::endl;
        });
    }

    void size_text()
    {
        font->size_text(this->label, label_rect.w, label_rect.h);
    }

    ~Button()
    {
    }

    void on_mouse_button_down(SDL_MouseButtonEvent& e)
    {
        if (!in_rect(e.x, e.y, viewport)) {
            return;
        }
        depressed = true;
    }

    void on_mouse_button_up(SDL_MouseButtonEvent& e)
    {
        if (!in_rect(e.x, e.y, viewport)) {
            if (depressed)
                depressed = false;
            return;
        }

        if (depressed) {
            emit(InternalEventType::clicked);
            depressed = false;
        }
    }

    void render() override
    {
        // Align label to center of outer rect vertically and horizontally
        label_rect.x = viewport.x + (viewport.w / 2) - (label_rect.w / 2);
        label_rect.y = (viewport.h / 2) - (label_rect.h / 2);

        SDL_Point coord = mouse_coord();
        if (depressed) {
            set_draw_color(renderer(), colors::lightgray);
            console::SDL_RenderFillRect(renderer(), &viewport);
            // SDL_RenderDrawRect(ui.renderer, &w.rect);
            set_draw_color(renderer(), colors::darkgray);
        } else if (in_rect(coord, viewport)) {
            set_draw_color(renderer(), colors::lightgray);
            console::SDL_RenderDrawRect(renderer(), &viewport);
            set_draw_color(renderer(), colors::darkgray);
        }

        font->render(renderer(), label, label_rect.x, label_rect.y);
    }

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    std::u32string label;
    SDL_Rect label_rect {};
    bool depressed { false };
};

struct Toolbar : public Widget {
    Toolbar(Widget* parent, SDL_Rect viewport);
    ~Toolbar() {};
    virtual void render() override;
    virtual void on_resize(SDL_Rect new_viewport) override;
    virtual void set_viewport(SDL_Rect new_viewport) override;
    Button* add_button(std::u32string text);
    void size_buttons();
    int compute_widgets_startx();
    void on_mouse_button_down(SDL_MouseButtonEvent& e);
    void on_mouse_button_up(SDL_MouseButtonEvent& e);
    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;
    // Should be changed to children and probably moved to base class
    std::deque<std::unique_ptr<Widget>> widgets;
};

struct InputLineWaiter {
    SignalEmitter& emitter;

    InputLineWaiter(SignalEmitter& emitter)
        : emitter(emitter)
    {
        emitter.connect<SDL_UserEvent>(InternalEventType::new_input_line, [this](SDL_UserEvent& e) {
            auto* str = static_cast<std::u32string*>(e.data1);
            (str == nullptr) ? push(U"") : push(*str);
        });
    }

    void push(std::u32string s)
    {
        std::scoped_lock lock(m);
        input_q.push(s);
        completed = true;
        completed.notify_one();
    }

    ~InputLineWaiter()
    {
    }

    void shutdown()
    {
        // std::cerr << "shutdown inputreader" << std::endl;
        {
            std::scoped_lock l(m);
            completed = true;
        }
        completed.notify_one();
        // std::cerr << "shutdown inputreader exit" << std::endl;
    }

    /* This function may be called recursively */
    int
    wait_get(std::string& buf)
    {
        std::unique_lock<std::recursive_mutex> lock(m);
        if (input_q.empty()) {
            lock.unlock();
            completed.wait(false);
            lock.lock();
            completed = false;
            // Likely being shutdown
            if (input_q.empty()) {
                // std::cerr << "input_q=empty()" << std::endl;
                return 0;
            }
        }
        buf = to_utf8(input_q.front());
        input_q.pop();
        return buf.length();
    }

private:
    std::recursive_mutex m;
    std::queue<std::u32string> input_q;
    std::atomic<bool> completed;
};

struct LogScreen : public Widget {
    // Use deque to hold a stable reference.
    std::deque<LogEntry> entries;
    Prompt prompt;
    Scrollbar scrollbar;
    // Scrollbar could be made optional.
    int scroll_value { 0 };
    SDL_Point viewport_offset;
    int max_lines { default_scrollback }; /* max numbers of lines allowed */
    int num_lines { 0 };
    bool depressed { false };
    SDL_Point mouse_motion_start { -1, -1 };
    SDL_Point mouse_motion_end { -1, -1 };

    LogScreen(Widget* parent, SDL_Rect& viewport)
        : Widget(parent, viewport)
        , prompt(this)
        , scrollbar(this, rows())
    {
        // Adjust viewport
        set_viewport(viewport);
        scrollbar.set_page_size(rows());
        scrollbar.set_range(rows());

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONDOWN, [this](SDL_MouseButtonEvent& e) {
            on_mouse_button_down(e);
        });

        connect_global<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP, [this](SDL_MouseButtonEvent& e) {
            on_mouse_button_up(e);
        });

        connect_global<SDL_MouseWheelEvent>(SDL_MOUSEWHEEL, [this](SDL_MouseWheelEvent& e) {
            scroll(e.y);
        });

        connect_global<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](SDL_MouseMotionEvent& e) {
            if (!in_rect(e.x, e.y, this->viewport))
                return;
            if (depressed) {
                set_mouse_motion_end({ e.x, e.y });
            }
        });

        connect_global<SDL_KeyboardEvent>(SDL_KEYDOWN, [this](SDL_KeyboardEvent& e) {
            on_key_down(e);
        });

        connect_global<SDL_TextInputEvent>(SDL_TEXTINPUT, [this](SDL_TextInputEvent& e) {
            scroll_value = 0;
            emit(InternalEventType::value_changed, &scroll_value); // ? XXX: check why
        });

        scrollbar.connect<SDL_UserEvent>(InternalEventType::value_changed, [this](SDL_UserEvent& e) {
            scroll_value = *static_cast<int*>(e.data1);
        });
    }

    // Ctrl-k to delete to end of line, Ctrl-b to go back a word, Ctrl-f to go forward a word, etc

    int on_key_down(const SDL_KeyboardEvent& e)
    {
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_TAB:
            new_input_line(from_utf8("(tab)"));
            break;
        /* copy */
        case SDLK_c:
            if (console::SDL_GetModState() & KMOD_CTRL) {
                copy_to_clipboard();
            }
            break;

        /* paste */
        case SDLK_v:
            if (console::SDL_GetModState() & KMOD_CTRL) {
                add_prompt_input_from_clipboard();
            }
            break;

        case SDLK_PAGEUP:
            scroll(ScrollDirection::page_up);
            break;

        case SDLK_PAGEDOWN:
            scroll(ScrollDirection::page_down);
            break;

        case SDLK_RETURN:
            new_input_line(*prompt.input);
        case SDLK_BACKSPACE:
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            set_scroll_value(0);
            break;
        }
        return 0;
    }

    void add_prompt_input_from_clipboard()
    {
        auto* str = console::SDL_GetClipboardText();
        if (*str != '\0')
            prompt.add_input(from_utf8(str));
        console::SDL_free(str);
    }

    void on_mouse_button_down(SDL_MouseButtonEvent& e)
    {
        if (!in_rect(e.x, e.y, viewport))
            return;

        if (e.button != SDL_BUTTON_LEFT) {
            return;
        }

        mouse_motion_end = { -1, -1 };
        depressed = true;
        set_mouse_motion_begin({ e.x, e.y });
    }

    void on_mouse_button_up(SDL_MouseButtonEvent& e)
    {
        if (!in_rect(e.x, e.y, viewport))
            return;

        depressed = false;
    }

    void clear()
    {
        entries.clear();
        num_lines = 0;
        set_scroll_value(0);
        scrollbar.set_range(rows());
    }

    void set_scroll_value(int v)
    {
        scroll_value = v;
        scrollbar.set_value(v);
    }

    void set_mouse_motion_begin(SDL_Point p)
    {
        translate_coord(p);
        mouse_motion_start = p;
    }

    void set_mouse_motion_end(SDL_Point p)
    {
        translate_coord(p);
        mouse_motion_end = p;
    }

    void translate_coord(SDL_Point& window_p)
    {
        window_p.x -= viewport.x;
        window_p.y -= viewport.y;
    }

    void scroll(int y)
    {
        if (y > 0) {
            scroll(ScrollDirection::up);
        } else if (y < 0) {
            scroll(ScrollDirection::down);
        }
    }

    void scroll(ScrollDirection dir)
    {
        switch (dir) {
        case ScrollDirection::up:
            scroll_value += 1;
            break;
        case ScrollDirection::down:
            scroll_value -= 1;
            break;
        case ScrollDirection::page_up:
            scroll_value += rows() / 2;
            break;
        case ScrollDirection::page_down:
            scroll_value -= rows() / 2;
            break;
        }

        scroll_value = std::min(std::max(0, scroll_value), num_lines - 1);
        set_scroll_value(scroll_value);
    }

    void on_resize(SDL_Rect new_viewport) override
    {
        viewport = new_viewport;
        scrollbar.set_viewport({ viewport.w - font->char_width * 2, viewport.y, font->char_width * 2, viewport.h });
        adjust_viewport();
        num_lines = 0;

        /*
         * TODO: we probably don't need to rebuild everything outside
         * visible view.
         */
        for (auto& e : entries) {
            update_entry(e);
        }
    }

    void set_viewport(SDL_Rect new_viewport) override
    {
        viewport_offset = { new_viewport.x, new_viewport.y };
        viewport = new_viewport;
        scrollbar.set_viewport({ viewport.w - font->char_width * 2, viewport.y, font->char_width * 2, viewport.h });
        adjust_viewport();
    }

    /*
     * Set viewport dimensions based on margin and font constraints.
     * For alignment, viewport must have equally sized rows large enough to
     * fit font height. Similarly, equally sized columns to fit font width.
     * We don't want partial rows and columns throwing off alignment.
     */
    void adjust_viewport()
    {
        // Make room for scrollbar. TODO: needs layout framework
        viewport.w -= font->char_width * 3;
        int margin = 4;
        // max width
        int w = viewport.w - (margin * 2);
        // max width respect to font and margin
        int wfit = (w / font->char_width) * font->char_width;
        // max height

        // int h = viewport.h - viewport_offset.y - margin;
        int h = viewport.h - margin;
        // max height with respect to font and margin
        int hfit = (h / font->line_height_with_spacing()) * font->line_height_with_spacing();

        viewport.x = viewport_offset.x + margin;
        viewport.y = viewport_offset.y + margin;
        viewport.w = wfit;
        viewport.h = hfit;
        // Prompt viewport is shared with this
        prompt.on_resize(viewport);
    }

    void new_output_line(const std::u32string& text)
    {
        LogEntry& entry = create_entry(EntryType::output, text);
        update_entry(entry);
    }

    // TODO: cleanup, most of this belongs in Prompt
    void new_input_line(const std::u32string& text)
    {
        auto both = prompt.prompt_text + text;
        auto& entry = create_entry(EntryType::input, both);
        prompt.history.emplace_back(text);

        update_entry(entry);

        emit_global(InternalEventType::new_input_line, prompt.input);

        prompt.input = &prompt.history.emplace_back(U"");
        prompt.history_idx = prompt.history.size() - 1;

        prompt.cursor = 0;
        prompt.rebuild = true;
    }

    void update_entry(
        LogEntry& entry)
    {
        split_entry_text(*this, entry, entry.text);
        num_lines += entry.size;
        // XXX: during resize, update_entry is called for every line
        scrollbar.set_range(num_lines + 1);
    }

    /*
     * Create a new line and set it to be the head. This function will
     * automatically cycle-out lines if the number of lines has reached the max.
     */
    LogEntry&
    create_entry(const EntryType line_type,
        const std::u32string& text)
    {
        entries.emplace_front(line_type, text);

        /* When the list is too long, start chopping */
        if (num_lines >= max_lines) {
            num_lines -= entries.back().size;
            entries.pop_back();
        }

        return entries.front();
    }

    // XXX: cleanup
    void copy_to_clipboard()
    {
        std::u32string ret;
        char32_t sep = U'\n';

        auto rects = get_highlighted_rects();
        std::reverse(rects.begin(), rects.end());
        for (auto entry_rit = entries.rbegin(); entry_rit != entries.rend(); ++entry_rit) {
            auto& entry = *entry_rit;

            for (auto& line : entry.lines()) {
                for (auto& rect : rects) {
                    // std::cerr << "hirect: x=" << rect.x << ", y=" << rect.y << std::endl;
                    // std::cerr << "linerect:" << to_utf8(line.text) << ", x=" << line.rect.x << ", y=" << line.rect.y << std::endl;
                    auto col = get_column(rect.x);
                    if (rect.y == line.coord.y && col < line.text.size()) {
                        if (!ret.empty())
                            ret += sep;
                        auto extent = column_extent(rect.w) + col;
                        ret += line.text.substr(col, std::min(extent - col, line.text.size() - col));
                    }
                }
            }
        }

        console::SDL_SetClipboardText(to_utf8(ret).c_str());
    }

    size_t
    get_column(const int x)
    {
        return x / font->char_width;
    }

    size_t column_extent(const int width)
    {
        return width / font->char_width;
    }

    int columns()
    {
        return (float)viewport.w / font->char_width;
    }

    int rows()
    {
        return (float)viewport.h / font->line_height_with_spacing();
    }

    void render() override
    {
        // SDL_RenderSetScale(renderer(), 1.2, 1.2);
        console::SDL_RenderSetViewport(renderer(), &viewport);
        prompt.maybe_rebuild();
        // TODO: make sure renderer supports blending else highlighting
        // will make the text invisible
        render_highlighted_lines();
        // SDL_SetTextureColorMod(font->texture, 0, 128, 0);
        render_lines();
        // SDL_SetTextureColorMod(font->texture, 255, 255, 255);
        //  Prompt input rendering is done in render_lines()
        prompt.render_cursor(scroll_value);
        console::SDL_RenderSetViewport(renderer(), &parent->viewport);
        scrollbar.render();
        // SDL_RenderSetScale(renderer(), 1.0, 1.0);
    }

    void render_lines()
    {
        const int max_row = rows() + scroll_value;
        int ypos = viewport.h;
        int row_counter = 0;

        render_entry(prompt.entry, ypos, row_counter, max_row);

        if (entries.empty())
            return;

        for (auto& entry : entries) {
            render_entry(entry, ypos, row_counter, max_row);
        }
    }

    void render_entry(LogEntry& entry, int& ypos, int& row_counter, const int max_row)
    {
        // TODO: get rid of the reverse iterator
        for (auto it = entry.lines().rbegin(); it != entry.lines().rend(); ++it) {
            row_counter++;
            if (row_counter <= scroll_value) {
                continue;
            } else if (row_counter > max_row) {
                return;
            }

            auto& line = *it;
            // ypos -= font->line_height * scale_factor
            ypos -= font->line_height_with_spacing();
            // record y position of this line
            // line.coord.y = ypos / scale_factor
            line.coord.y = ypos;
            font->render(renderer(), line.text, line.coord.x, line.coord.y);
        }
    }

    void render_highlighted_lines()
    {
        if (mouse_motion_end.y == -1)
            return;

        auto rects = get_highlighted_rects();
        if (rects.empty())
            return;

        set_draw_color(renderer(), colors::mediumgray);
        for (auto& rect : rects) {

            console::SDL_RenderFillRect(renderer(), &rect);
        }
        set_draw_color(renderer(), colors::darkgray);
    }

    // XXX: cleanup
    std::vector<SDL_Rect> get_highlighted_rects()
    {
        const int char_width = font->char_width;
        const int line_height = font->line_height_with_spacing();
        const SDL_Point& selection_start = mouse_motion_start;
        const SDL_Point& selection_end = mouse_motion_end;

        // Calculate the start and end positions, snapping to line and character boundaries
        auto [top_point, bottom_point] = std::minmax({selection_start, selection_end},
                                            [](const SDL_Point& a, const SDL_Point& b) {
                                                return a.y < b.y;
                                            });

        auto top = snap_to_min(top_point.y, line_height);
        auto bottom = snap_to_max(bottom_point.y, line_height);
        bool is_single_row = (bottom_point.y - top_point.y) <= line_height;

        int left;
        int right;
        if (is_single_row) {
            left = snap_to_min(std::min(selection_start.x, selection_end.x), char_width);
            right = snap_to_max(std::max(selection_start.x, selection_end.x), char_width);
        } else {
            left = snap_to_min(top_point.x, char_width);
            right = snap_to_max(bottom_point.x, char_width);
        }

        SDL_Rect current_rect = { left, top, (right - left), line_height };
        if (is_single_row)
            return { current_rect };

        int rows = std::ceil((float)(bottom - top) / line_height);
        std::vector<SDL_Rect> selected_rects;
        current_rect.w = viewport.w;
        selected_rects.push_back(current_rect);
        // Handle intermediate rows
        for (int i = 1; i < rows; ++i) {
            current_rect.x = 0;
            current_rect.y = top + i * line_height;
            current_rect.w = viewport.w;
            selected_rects.push_back(current_rect);
        }
        // Fill last row to end of selected text
        selected_rects.back().w = right;

        return selected_rects;
    }

    LogScreen(const LogScreen&) = delete;
    LogScreen& operator=(const LogScreen&) = delete;
};

struct WindowContext {
    SDL_Window* handle;
    SDL_Renderer* renderer;
    SDL_Rect rect;

    WindowContext(SDL_Window* h, SDL_Renderer* r, SDL_Rect re)
        : handle(h)
        , renderer(r)
        , rect(re)
    {
    }
};

struct MainWindow : public Widget {
    WidgetContext widget_context;
    SDL_Window* handle { nullptr };
    SDL_Point mouse_coord {}; // stores mouse position relative to window
    std::unique_ptr<Toolbar> toolbar; // optional toolbar. XXX: implementation requires it
    std::unique_ptr<LogScreen> log_screen;
    Uint32 window_id; // Window id from SDL
    void render() {};

    MainWindow(WindowContext winctx, Font* font, SignalEmitter& emitter)
        : Widget(font, widget_context, winctx.rect)
        , widget_context(winctx.renderer, &emitter, mouse_coord)
        , handle(winctx.handle)
    {
        window_id = console::SDL_GetWindowID(handle);
        if (window_id == 0)
            throw(std::runtime_error(SDL_GetError()));

        connect_global<SDL_WindowEvent>(SDL_WINDOWEVENT, [this](SDL_WindowEvent& e) {
            if (e.event == SDL_WINDOWEVENT_RESIZED) {
                console::SDL_GetRendererOutputSize(renderer(), &viewport.w, &viewport.h);
                on_resize(viewport);
            }
        });

        connect_global<SDL_MouseMotionEvent>(SDL_MOUSEMOTION, [this](SDL_MouseMotionEvent& e) {
            mouse_coord.x = e.x;
            mouse_coord.y = e.y;
        });

        console::SDL_SetWindowMinimumSize(handle, 64, 48);
        console::SDL_RenderSetIntegerScale(renderer(), SDL_TRUE);

        SDL_Rect tv = { 0, 0, viewport.w, font->line_height * 2 };
        toolbar = std::make_unique<Toolbar>(this, tv);

        SDL_Rect lv = { 0, toolbar->viewport.h, viewport.w, viewport.h - toolbar->viewport.h };
        log_screen = std::make_unique<LogScreen>(this, lv);
    }

    ~MainWindow()
    {
        if (renderer()) {
            console::SDL_DestroyRenderer(renderer());
        }
        if (handle) {
            console::SDL_DestroyWindow(handle);
        }
    }

    void on_resize(SDL_Rect new_viewport) override
    {
        toolbar->on_resize({ 0, 0, viewport.w, font->line_height_with_spacing() * 2 });
        log_screen->on_resize({ 0, toolbar->viewport.h, viewport.w, viewport.h - toolbar->viewport.h });
    }

    static WindowContext create(const char* title, int x, int y, int w, int h, Uint32 flags)
    {
        SDL_Window* handle = console::SDL_CreateWindow(title, x, y, w, h, flags);
        if (!handle) {
            throw std::runtime_error("Failed to create SDL window");
        }

        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
        // Flags 0 instructs SDL to choose the default backend for the
        // host system. TODO: add config to force software rendering
        SDL_RendererFlags rflags = (SDL_RendererFlags)0;
        SDL_Renderer* renderer = console::SDL_CreateRenderer(handle, -1, rflags);
        if (!renderer) {
            console::SDL_DestroyWindow(handle);
            throw std::runtime_error("Failed to create SDL renderer");
        }

        SDL_Rect rect = {};
        console::SDL_GetRendererOutputSize(renderer, &rect.w, &rect.h);
        return WindowContext(handle, renderer, rect);
    }

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
};

Toolbar::Toolbar(Widget* parent, SDL_Rect viewport)
    : Widget(parent, viewport)
{

    font->connect<SDL_UserEvent>(InternalEventType::font_size_changed, [this](SDL_UserEvent& e) {
        size_buttons();
        std::cerr << "font_size_changed" << std::endl;
    });
};

void Toolbar::render()
{
    set_draw_color(renderer(), colors::charcoal);

    // Render bg
    // SDL_RenderFillRect(renderer(), &viewport);
    // Draw a border
    console::SDL_RenderDrawRect(renderer(), &viewport);

    int margin_right = 4;
    int x = (parent->viewport.w - margin_right) - compute_widgets_startx();

    // Lay out horizontally
    // TODO: don't calculate child widget offsets here
    // TODO: rename to children, should be in base class
    for (auto& w : widgets) {
        w->viewport.x = x;
        x += w->viewport.w;

        w->render();
    }

    set_draw_color(renderer(), colors::darkgray);
}

void Toolbar::on_resize(SDL_Rect new_viewport)
{
    viewport.w = parent->viewport.w;
}

void Toolbar::set_viewport(SDL_Rect new_viewport)
{
    viewport = new_viewport;
}

Button* Toolbar::add_button(std::u32string text)
{
    auto button = std::make_unique<Button>(this, text, colors::white);
    Button* button_p = button.get();
    button_p->viewport.h = viewport.h;
    button_p->viewport.y = 0;
    button_p->viewport.w = button_p->label_rect.w + (font->char_width * 2);
    widgets.emplace_back(std::move(button));
    return button_p;
}

void Toolbar::size_buttons()
{
    for (auto& w : widgets) {
        auto b = static_cast<Button*>(w.get());
        b->size_text();
        b->viewport.w = b->label_rect.w + (font->char_width * 2);
    }
}

int Toolbar::compute_widgets_startx()
{
    int x = 0;
    for (auto& w : widgets) {
        x += w->viewport.w;
    }
    return x;
}

class ExternalEventWaiter {
    using Notifier = std::atomic<bool>;
    template <typename T>
    class EventQueue {
        friend class ExternalEventWaiter;

    public:
        EventQueue(Notifier& notifier, State& status)
            : notifier(notifier)
            , status(status)
        {
        }

        void push(T event)
        {
            {
                std::scoped_lock lock(mutex);
                if (status != State::active)
                    return;
                queue.push(event);
                notifier = true;
            }
            notifier.notify_one();
        }

        bool pop(T& event)
        {
            std::scoped_lock lock(mutex);
            if (!queue.empty()) {
                event = queue.front();
                queue.pop();
                return true;
            }
            return false;
        }

    protected:
        std::mutex mutex;

    private:
        std::queue<T> queue;
        Notifier& notifier;
        State& status;
    };

public:
    ExternalEventWaiter()
        : sdl(notifier, state)
        , api(notifier, state)
    {
    }

    void wait_for_events()
    {
        notifier.wait(false);
        {
            /* synchronize to ensure we don't miss any events */
            std::scoped_lock lock(sdl.mutex, api.mutex);
            notifier = false;
        }
    }

    void reset()
    {
        drain();
        std::scoped_lock lock(sdl.mutex, api.mutex);
        state = State::active;
    }

    void shutdown()
    {
        {
            std::scoped_lock lock(sdl.mutex, api.mutex);
            state = State::shutdown;
        }
        drain();
    }

    ExternalEventWaiter(const ExternalEventWaiter&) = delete;
    ExternalEventWaiter& operator=(const ExternalEventWaiter&) = delete;

    EventQueue<SDL_Event> sdl;
    using Task = std::function<void()>;
    EventQueue<Task> api;

private:
    void drain()
    {
        SDL_Event e;
        while (sdl.pop(e))
            ;
        Task t;
        while (api.pop(t))
            ;
    }

    Notifier notifier { false };
    State state = { State::active };
};

bool in_rect(int x, int y, SDL_Rect& r)
{
    return ((x >= r.x) && (x < (r.x + r.w)) && (y >= r.y) && (y < (r.y + r.h)));
}

bool in_rect(SDL_Point& p, SDL_Rect& r)
{
    return console::SDL_PointInRect(&p, &r);
}

void render_texture(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& dst)
{
    console::SDL_RenderCopy(renderer, texture, NULL, &dst);
}

int set_draw_color(SDL_Renderer* renderer, const SDL_Color& color)
{
    return console::SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

// Used by Prompt and LogScreen
// XXX: cleanup.
void split_entry_text(
    Widget& widget,
    LogEntry& entry,
    std::u32string& text)
{
    struct Segment {
        size_t start;
        size_t end;
    };

    entry.clear();
    entry.text = text;

    const int char_width = widget.font->char_width;
    const int viewport_width = widget.viewport.w;

    // Break up the text into line segments, if needed
    int delim_idx = 0; // last whitespace character for wrapping on word boundaries
    int start_idx = 0;
    int curr_idx = 0;
    std::vector<Segment> segments;
    for (auto& ch : text) {
        if (ch == U'\n' || ch == U'\r') {
            // Not including the new line character?
            // Don't attempt to add an empty segment
            if (curr_idx > start_idx)
                segments.emplace_back(start_idx, curr_idx);
            start_idx = curr_idx + 1;
            delim_idx = 0;
            // TODO: check for spaces properly?
        } else if (ch == U' ' || ch == U'\t') {
            delim_idx = curr_idx;
            // check if width exceeded
        } else if (((curr_idx - start_idx + 1) * char_width) >= viewport_width) {
            if (delim_idx) {
                // wrap at last whitespace
                segments.emplace_back(start_idx, delim_idx);
                start_idx = delim_idx + 1;
                delim_idx = 0;
            } else {
                // wrap at last character
                segments.emplace_back(start_idx, curr_idx);
                start_idx = curr_idx + 1;
            }
        }

        curr_idx++;
    }

    //  Handle any remaining text
    if (start_idx < text.size()) {
        segments.emplace_back(start_idx, text.size() - 1);
    }

    for (auto& seg : segments) {
        if (seg.end >= seg.start) {
            auto view = std::u32string_view(entry.text).substr(seg.start, seg.end - seg.start + 1);
            entry.add_line(view, seg.start, seg.end);
        }
    }
}
// TODO: handle errors properly.  TODO: TTF not currently used, needs reworked to support font atlas.
#if 0
SDL_Texture* create_text_texture(Widget& widget, const std::u32string& text, const SDL_Color& color)
{
    SDL_Surface* surface = TTF_RenderUTF8_Blended(widget.font->handle,
        to_utf8(text).c_str(), colors::white);
    if (!surface) {
        std::cerr << "Error initializing surface: " << TTF_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(widget.renderer(), surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "Error creating texture: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    return texture;
}
#endif

int sdl_event_callback(void* data, SDL_Event* e);
}

using namespace console;

struct SDLEventFilterSetter {
    SDLEventFilterSetter(SDL_EventFilter filter, void* user_data)
    {
        // Save the old filter so we can call it when
        // we aren't handling an event.
        console::SDL_GetEventFilter(&saved_filter, &saved_user_data);
        console::SDL_SetEventFilter(filter, user_data);
    }

    ~SDLEventFilterSetter()
    {
        // reset_saved() gets called on shutdown but in the event
        // something very bad happened, do it here.
        if (!did_reset_saved)
            console::SDL_SetEventFilter(saved_filter, saved_user_data);
    }

    void reset_saved()
    {
        did_reset_saved = true;
        console::SDL_SetEventFilter(saved_filter, saved_user_data);
    }

    int maybe_call_saved(void* user_data, SDL_Event* e)
    {
        return !saved_filter ? 1 : saved_filter(user_data, e);
    }

    SDLEventFilterSetter(const SDLEventFilterSetter&) = delete;
    SDLEventFilterSetter& operator=(const SDLEventFilterSetter&) = delete;

private:
    bool did_reset_saved { false };
    SDL_EventFilter saved_filter { nullptr };
    void* saved_user_data { nullptr };
};

struct Console_con {
    struct Impl {
        // For internal communication, mainly by widgets.
        SignalEmitter internal_emitter;
        MainWindow window;
        // Opens and caches Font objects.
        // A new Font object may be used when
        // changing font size.
        std::unique_ptr<FontLoader> font_loader;
        SDL_Color bg_color; // not currently used
        SDL_Color font_color; // not currently used
        // Used by GetLine() to wait for a new input line event.
        InputLineWaiter input_line_waiter;
        ExternalEventWaiter& external_event_waiter;
        // Event Filter is how we currently receive events from SDL.
        SDLEventFilterSetter event_filter_setter;
        // Stores the thread id of the thread used to create the console, which is also
        // the thread responsible for rendering.
        std::thread::id render_thread_id;

        Impl(Console_con* con, WindowContext wctx, std::unique_ptr<FontLoader> fl, ExternalEventWaiter& external_event_waiter)
            : window(wctx, fl->default_font(), internal_emitter)
            , font_loader(std::move(fl))
            , input_line_waiter(internal_emitter)
            , external_event_waiter(external_event_waiter)
            , event_filter_setter(sdl_event_callback, con)
            , render_thread_id(std::this_thread::get_id())
        {
            external_event_waiter.reset();
            console::SDL_StartTextInput();
        };

        ~Impl()
        {
            console::SDL_StopTextInput();
        }
    };

    Console_con() = default;
    ~Console_con() = default;

    void init(WindowContext wctx, std::unique_ptr<FontLoader> fl)
    {
        impl = std::make_unique<Impl>(this, wctx, std::move(fl), external_event_waiter);
    }

    bool is_active()
    {
        return state == State::active;
    }

    bool is_shuttingdown()
    {
        return state == State::shutdown;
    }

    LogScreen& lscreen()
    {
        return *impl->window.log_screen.get();
    }

    /* Queues SDL events and API tasks to later run on the render thread.
     * SDL events should be drained from it on shutdown.
     * API tasks should be drained as well, but just in case
     * we'll leave the storage in place after destroy(),
     * and instruct the queues not to accept more items.
     */
    ExternalEventWaiter external_event_waiter;
    std::atomic<State> state { State::active };
    std::unique_ptr<Impl> impl;
    // Protects access to data such as rows() and column()
    // information fetched from API functions.
    std::mutex mutex;
    // These mutexes are only relevant during shutdown.
    std::mutex getline_inproc_mutex;
    std::mutex on_sdl_event_inproc_mutex;
};
static Console_con num_con[1];

namespace console {

// XXX: move rendering to Window
int render_frame(Console_con::Impl* impl)
{
    assert(impl);

    // Should not fail unless memory starvation.
    console::SDL_RenderClear(impl->window.renderer());

    //  set background color
    // Should not fail unless renderer is invalid
    set_draw_color(impl->window.renderer(), colors::darkgray);

    impl->window.toolbar->render();

    /* render text area */

    impl->window.log_screen->render();

    console::SDL_RenderPresent(impl->window.renderer());

    return 0;
}

int emit_sdl_event(Console_con::Impl* impl, SDL_Event& e)
{
    impl->internal_emitter.emit(e);
    return 0;
}

int sdl_event_callback(void* data, SDL_Event* e)
{
    auto con = static_cast<Console_con*>(data);
    std::scoped_lock l(con->on_sdl_event_inproc_mutex);

    const Uint32 flags = console::SDL_GetWindowFlags(con->impl->window.handle);
    if ((e->type == SDL_WINDOWEVENT && e->window.windowID != con->impl->window.window_id)
        || !(flags & SDL_WINDOW_INPUT_FOCUS)
        || e->type == SDL_USEREVENT) {

        return con->impl->event_filter_setter.maybe_call_saved(data, e);
    }

    SDL_Event ec;
    std::memcpy(&ec, e, sizeof(SDL_Event));
    con->external_event_waiter.sdl.push(ec);
    return 0;
}
}

void Console_Init(void* (*resolver)(const char*))
{
    try {
        resolve_symbols(resolver);
    } catch (...) {
    }
}

// XXX: cleanup
Console_con*
Console_Create(const char* title,
    const char* prompt,
    const int font_size)

{
    if (console::SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL failed to init: " << console::SDL_GetError();
        return nullptr;
    }

    try {
        WindowContext wctx = MainWindow::create(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            640, 480,
            SDL_WINDOW_RESIZABLE);

        console::SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
        // SDL_RenderSetLogicalSize(wctx.renderer, 384, 216);

        auto font_loader = std::make_unique<BMPFontLoader>(wctx.renderer);
        auto bmpfont = font_loader->open("test.png", 14);
        if (!bmpfont)
            std::cerr << "Failed to open font: " << console::SDL_GetError() << std::endl;

        // SDL_RenderSetScale(wctx.renderer, 1.5, 1.5);

        /*
        auto font_loader = std::make_unique<FontLoader>();
        auto font = font_loader->open("source_code_pro.ttf", font_size);
        if (font == nullptr) {
            std::cerr << "error initializing TTF: " << TTF_GetError();
            return nullptr;
        }*/

        Console_con* con = &num_con[0];
        con->init(wctx, std::move(font_loader));

        Widget* copy = con->impl->window.toolbar->add_button(U"Copy");
        copy->connect<SDL_UserEvent>(InternalEventType::clicked, [con](SDL_UserEvent& e) {
            con->lscreen().copy_to_clipboard();
        });

        Widget* paste = con->impl->window.toolbar->add_button(U"Paste");
        paste->connect<SDL_UserEvent>(InternalEventType::clicked, [con](SDL_UserEvent& e) {
            con->lscreen().add_prompt_input_from_clipboard();
        });

        //* Best to change font size in a menu, I think.
        Widget* font_inc = con->impl->window.toolbar->add_button(U"A+");
        font_inc->connect<SDL_UserEvent>(InternalEventType::clicked, [con](SDL_UserEvent& e) {
            con->lscreen().font->incr_size();
        });

        Widget* font_dec = con->impl->window.toolbar->add_button(U"A-");
        font_dec->connect<SDL_UserEvent>(InternalEventType::clicked, [con](SDL_UserEvent& e) {
            con->lscreen().font->decr_size();
        });

        con->lscreen().prompt.set_prompt(from_utf8(prompt));
        con->state = State::active;

        /*
        SDL_RendererInfo info;
        SDL_GetRendererInfo(con->impl->window.renderer(), &info);
        printf("Renderer name: %s\n", info.name);
        printf("Texture formats:\n");
        for (Uint32 i = 0; i < info.num_texture_formats; i++) {
            printf("%s\n", SDL_GetPixelFormatName(info.texture_formats[i]));
        }*/

        // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Console Error", "Error", tty->window.handle);

        return con;
    } catch (std::runtime_error& e) {
        console::SDL_QuitSubSystem(SDL_INIT_VIDEO);
        std::cerr << e.what() << std::endl;
    }
    return nullptr;
}

void Console_SetPrompt(Console_con* con,
    const char* prompt)
{
    con->external_event_waiter.api.push([con, str = from_utf8(prompt)] {
        con->lscreen().prompt.set_prompt(str);
    });
}

/* XXX: cleanup */
int Console_MainLoop(Console_con* con)
{
    while (1) {
        auto impl = con->impl.get();
        // No mutex should be needed yet.
        // Data writes happen only on the render thread.
        if (render_frame(impl))
            return -1;

        impl->external_event_waiter.wait_for_events();
        {
            std::scoped_lock lock(con->mutex);
            SDL_Event event;
            while (impl->external_event_waiter.sdl.pop(event)) {
                emit_sdl_event(impl, event);
            }
            ExternalEventWaiter::Task f;
            while (impl->external_event_waiter.api.pop(f)) {
                f();
            }
        }

        if (con->is_shuttingdown()) {
            impl->event_filter_setter.reset_saved();
            impl->input_line_waiter.shutdown();
            {
                std::scoped_lock l(con->getline_inproc_mutex);
            }
            impl->external_event_waiter.shutdown();
            {
                std::scoped_lock l(con->on_sdl_event_inproc_mutex);
            }
            break;
        }

        SDL_Delay(50);
    }
    return 0;
}

void Console_AddLine(Console_con* con, const char* s)
{
    auto str = from_utf8(s);
    con->external_event_waiter.api.push([con, str = std::move(str)] {
        con->lscreen().new_output_line(str);
    });
}

void Console_SetBackgroundColor(Console_con* con, const SDL_Color c)
{
    std::scoped_lock lock(con->mutex);
    con->impl->bg_color = c;
}

void Console_SetFontColor(Console_con* con, const SDL_Color c)
{
    std::scoped_lock lock(con->mutex);
    con->impl->font_color = c;
}

int Console_GetColumns(Console_con* con)
{
    std::scoped_lock lock(con->mutex);
    return con->lscreen().columns();
}

int Console_GetRows(Console_con* con)
{
    std::scoped_lock lock(con->mutex);
    return con->lscreen().rows();
}

void Console_Clear(Console_con* con)
{
    con->external_event_waiter.api.push([con] {
        con->lscreen().clear();
    });
}

void Console_Shutdown(Console_con* con)
{
    assert(con);
    con->state = State::shutdown;
    // Must push an event to wake up the main render thread
    con->external_event_waiter.api.push([] {});
}

// XXX: cleanup properly
bool Console_Destroy(Console_con* con)
{
    assert(con);
    if (con->state == State::inactive)
        return true;

    if (std::this_thread::get_id() != con->impl->render_thread_id)
        return false;

    con->state = State::inactive;
    con->impl.reset();
    console::SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return true;
}

int Console_GetLine(Console_con* con, std::string& buf)
{
    std::scoped_lock l(con->getline_inproc_mutex);

    if (!con->is_active())
        return -1;

    return con->impl->input_line_waiter.wait_get(buf);
}

void Console_SetScrollback(Console_con* con, const int lines)
{
    con->external_event_waiter.api.push([con, lines = lines] {
        con->lscreen().max_lines = lines;
    });
}

void Console_ShowWindow(Console_con* con)
{
    con->external_event_waiter.api.push([con] {
        console::SDL_ShowWindow(con->impl->window.handle);
    });
}

void Console_HideWindow(Console_con* con)
{
    con->external_event_waiter.api.push([con] {
        console::SDL_HideWindow(con->impl->window.handle);
    });
}

const char*
Console_GetError(void)
{
    // TODO:
    return "";
}

// kate: replace-tabs on; indent-width 4;
