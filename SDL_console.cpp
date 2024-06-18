#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <codecvt>
#include <cstring>
#include <functional>
#include <iostream>
#include <locale>
#include <map>
#include <mutex>
#include <queue>
#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "SDL_console.h"

static constexpr size_t default_scrollback = 1024;

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

namespace console {
enum class ScrollDirection { up,
    down,
    page_up,
    page_down };

enum class State { active,
    shutdown,
    inactive };

enum class ExternalEventType { sdl,
    api,
};
/* Nneed type promotion to uint32
 * to use with SDL_EventType. SDL_EventType is uint32, but
 * only goes up to uint16 for use by its internal arrays. This leaves
 * plenty of room for custom types.
 */
struct InternalEventType {
    enum Type : Uint32 { new_input_line = SDL_LASTEVENT + 1,
        clicked };
};

enum class EntryType { input,
    output };

namespace colors {
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
void make_logentry_lines(
    Widget& widget,
    LogEntry& entry,
    std::u32string& text);

struct History {
    std::u32string text;
};

struct LogEntryLine {
    SDL_Texture* texture; // texture of line
    size_t index; // line index into entries
    size_t start_index; // index into entry text
    size_t end_index; // index into entry text
    SDL_Rect rect {};

    LogEntryLine(SDL_Texture* texture, size_t line_index, size_t start_index, size_t end_index)
        : texture(texture)
        , index(line_index)
        , start_index(start_index)
        , end_index(end_index)
    {
        SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
    };

    ~LogEntryLine()
    {
        SDL_DestroyTexture(texture);
    }

    LogEntryLine(const LogEntryLine&) = delete;
    LogEntryLine& operator=(const LogEntryLine&) = delete;
};

using LogEntryLines = std::deque<LogEntryLine>;
struct LogEntry {
    EntryType type;
    std::u32string text;
    SDL_Rect rect;
    size_t size { 0 }; // total # of lines

    LogEntry() {};

    LogEntry(EntryType type, const std::u32string& text)
        : type(type)
        , text(text) {};

    auto& add_line(SDL_Texture* texture, size_t start_index, size_t end_index)
    {
        return lines_.emplace_back(texture, size++, start_index, end_index);
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

struct FontLoader;
struct Font {
    FontLoader& loader;
    TTF_Font* handle;
    int char_width;
    int line_height;

    Font(FontLoader& loader, TTF_Font* handle, int char_width, int line_height)
        : loader(loader)
        , handle(handle)
        , char_width(char_width)
        , line_height(line_height)
    {
    }

    ~Font()
    {
    }

    Font(Font&& other) noexcept
        : loader(other.loader)
        , handle(other.handle)
        , char_width(other.char_width)
        , line_height(other.line_height)
    {
        other.handle = nullptr;
    }

    Font& operator=(Font&& other) noexcept
    {
        if (this != &other) {
            handle = other.handle;
            char_width = other.char_width;
            line_height = other.line_height;
            other.handle = nullptr;
        }
        return *this;
    }

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
};

using FontMap = std::map<std::pair<std::string, int>, Font>;
struct FontLoader {
    FontLoader()
    {
        if (TTF_Init()) {
            throw std::runtime_error(TTF_GetError());
        }
    }

    ~FontLoader()
    {
        for (auto& pair : fmap) {
            if (pair.second.handle) {
                TTF_CloseFont(pair.second.handle);
            }
        }

        TTF_Quit();
    }

    Font* open(const std::string& path, int size)
    {
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
    }

    Font* get_font()
    {
        return &fmap.begin()->second;
    }

    FontLoader(const FontLoader&) = delete;
    FontLoader& operator=(const FontLoader&) = delete;

    FontLoader(FontLoader&& other) noexcept
        : fmap(std::move(other.fmap))
    {
    }

    FontLoader& operator=(FontLoader&& other) noexcept
    {
        if (this != &other) {
            fmap = std::move(other.fmap);
        }
        return *this;
    }

private:
    FontMap fmap;
};

class EventEmitter {
public:
    using Handler = std::function<void(SDL_Event&)>;

    void subscribe(Uint32 event_type, Handler handler)
    {
        handlers[event_type].push_back(handler);
    }

    void emit(SDL_Event& event)
    {
        auto it = handlers.find(event.type);
        if (it != handlers.end()) {
            for (auto& handler : it->second) {
                handler(event);
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
        handlers.clear();
    }

    static SDL_Event make_sdl_user_event(InternalEventType::Type type, void* data1)
    {
        SDL_Event event;
        SDL_zero(event);
        event.type = type;
        event.user.data1 = data1;
        return event;
    }

    EventEmitter() = default;

    ~EventEmitter()
    {
    }

    EventEmitter(EventEmitter&& other) noexcept
        : handlers(std::move(other.handlers))
    {
    }

    EventEmitter& operator=(EventEmitter&& other) noexcept
    {
        if (this != &other) {
            handlers = std::move(other.handlers);
        }
        return *this;
    }

    EventEmitter(const EventEmitter&) = delete;
    EventEmitter& operator=(const EventEmitter&) = delete;

private:
    std::map<Uint32, std::vector<Handler>> handlers;
};

struct Window;
struct WidgetContext {
    WidgetContext(SDL_Renderer* r, EventEmitter* em, SDL_Point& mouse)
        : renderer(r)
        , global_emitter(em)
        , mouse_coord(mouse)
    {
    }
    SDL_Renderer* renderer;
    EventEmitter* global_emitter;
    SDL_Point& mouse_coord;
};

class Widget {
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

    void set_font(std::string file, int size)
    {
        // XXX: check for error
        font = font->loader.open(file, size);
    }

    void subscribe(Uint32 event_type, EventEmitter::Handler handler)
    {
        emitter.subscribe(event_type, handler);
    }

    void subscribe_global(Uint32 event_type, EventEmitter::Handler handler)
    {
        context.global_emitter->subscribe(event_type, handler);
    }

    template <typename... Args>
    void emit(Args&&... args)
    {
        emitter.emit(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void emit_global(Args&&... args)
    {
        context.global_emitter->emit(std::forward<Args>(args)...);
    }

    virtual void render() {};
    virtual void set_viewport(SDL_Rect new_viewport) {};
    virtual void on_resize() {};

    virtual ~Widget() { }

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

private:
    EventEmitter emitter;
    WidgetContext& context;
};

struct Prompt : public Widget {
    Prompt(Widget* parent)
        : Widget(parent)
    {
        // Create 1x1 texture for the cursor, it will be stretched to fit the font's line height and character width
        cursor_texture = SDL_CreateTexture(renderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
        if (cursor_texture == nullptr)
            throw(std::runtime_error(SDL_GetError()));

        // FFFFFF = rgb white, 7F = 50% transparant
        Uint32 pixel = 0xFFFFFF7F;
        SDL_UpdateTexture(cursor_texture, NULL, &pixel, sizeof(Uint32));
        // For transparancy
        SDL_SetTextureBlendMode(cursor_texture, SDL_BLENDMODE_BLEND);

        subscribe_global(SDL_KEYDOWN, [this](SDL_Event& e) {
            on_key_down(e.key);
        });

        subscribe_global(SDL_TEXTINPUT, [this](SDL_Event& e) {
            add_input(from_utf8(e.text.text));
        });
    }

    ~Prompt()
    {
        SDL_DestroyTexture(cursor_texture);
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
        }
    }

    void set_prompt(const std::u32string& str)
    {
        prompt_text = str;
        entry.text = str + input;
        rebuild = true;
    }

    /*
     * Set the current line. We can go UP (next) or DOWN (previous) through the
     * lines. This function essentially acts as a history viewer. This function
     * will skip lines with zero length. The cursor is always set to the length of
     * the line's input.
     */
    void set_input_from_history(ScrollDirection dir)
    {
        if (history.empty())
            return;

        size_t idx = history_idx;
        if (dir == ScrollDirection::up) {
            if (idx > 0)
                idx--;
            else
                return;
        } else {
            if (idx < history.size() - 1)
                idx++;
        }
        return;

        history_idx = idx;
        input = history[idx].text;
        cursor = history[idx].text.length();
        rebuild = true;
    }

    void add_input(const std::u32string& str)
    {
        /* if cursor is at end of line, it's a simple concatenation */
        if (cursor == input.length()) {
            input += str;
        } else {
            /* else insert text into line at cursor's index */
            input.insert(cursor, str);
        }
        cursor += str.length();
        rebuild = true;
    }

    void erase_input()
    {
        if (cursor == 0 || input.length() == 0)
            return;

        if (input.length() == cursor) {
            input.pop_back();
        } else {
            /* else shift the text from cursor left by one character */
            input.erase(cursor, 1);
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
        if (cursor < input.length()) {
            cursor++;
        }
    }

    void on_resize() override
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
        std::u32string str = prompt_text + input;
        make_logentry_lines(*this, entry, str);
    }

    void render_cursor(int scroll_offset)
    {
        if (entry.lines().empty())
            return;

        /* cursor's position */
        int offset = prompt_text.length();
        auto cursor_len = cursor + offset;

        auto* line = &entry.lines().back();
        // XXX: fix doing needless work on render
        for (auto& l : entry.lines()) {
            if (cursor_len >= l.start_index - 1 && cursor_len <= l.end_index - 1) {
                line = &l;
                break;
            }
        }

        // one based. reverse the row so that last = 0
        // scroll_offset starts at 0.
        int r = (entry.size - 1) - line->index;
        if (scroll_offset > r)
            return;

        const auto lh = font->line_height;
        const auto cw = font->char_width;
        /*  full range of line + cursor */
        int cx = (cursor_len - line->start_index) * cw;
        int cy = line->rect.y;

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

    LogEntry entry; // prompt may be line wrapped
    std::u32string prompt_text;
    std::u32string input;
    bool rebuild { true };
    size_t cursor { 0 }; /* position of cursor within a line */
    /* 1x1 texture */
    SDL_Texture* cursor_texture;
    /* For input history */
    std::vector<History> history;
    int history_idx;
};

class Button : public Widget {
public:
    Button(Widget* parent, std::u32string label, SDL_Color color)
        : Widget(parent)
        , label(label)
    {
        texture = create_text_texture(*this, label, color);
        SDL_QueryTexture(texture, NULL, NULL, &label_rect.w, &label_rect.h);

        subscribe_global(SDL_MOUSEBUTTONDOWN, [this](SDL_Event& e) {
            this->on_mouse_button_down(e.button);
        });

        subscribe_global(SDL_MOUSEBUTTONUP, [this](SDL_Event& e) {
            this->on_mouse_button_up(e.button);
        });
    }

    ~Button()
    {
        SDL_DestroyTexture(texture);
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
            emit_clicked();
            depressed = false;
        }
    }

    void emit_clicked()
    {
        emit(InternalEventType::clicked);
    }

    void render() override
    {
        // Align label to center of outer rect vertically and horizontally
        label_rect.x = viewport.x + (viewport.w / 2) - (label_rect.w / 2);
        label_rect.y = (viewport.h / 2) - (label_rect.h / 2);

        SDL_Point coord = mouse_coord();
        if (depressed) {
            set_draw_color(renderer(), colors::lightgray);
            SDL_RenderFillRect(renderer(), &viewport);
            // SDL_RenderDrawRect(ui.renderer, &w.rect);
            set_draw_color(renderer(), colors::darkgray);
        } else if (in_rect(coord, viewport)) {
            set_draw_color(renderer(), colors::lightgray);
            SDL_RenderDrawRect(renderer(), &viewport);
            set_draw_color(renderer(), colors::darkgray);
        }

        render_texture(renderer(), texture, label_rect);
    }

    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    std::u32string label;
    SDL_Texture* texture { nullptr };
    SDL_Rect label_rect {};
    bool depressed { false };
};

struct Toolbar : public Widget {
    Toolbar(Widget* parent);
    ~Toolbar() {};
    virtual void render() override;
    virtual void on_resize() override;
    virtual void set_viewport(SDL_Rect new_viewport) override;
    Button* add_button(std::u32string text);
    int compute_widgets_startx();
    void on_mouse_button_down(SDL_MouseButtonEvent& e);
    void on_mouse_button_up(SDL_MouseButtonEvent& e);
    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    std::deque<std::unique_ptr<Widget>> widgets;
};

struct InputLineWaiter {
    EventEmitter& emitter;

    InputLineWaiter(EventEmitter& emitter)
        : emitter(emitter)
    {
        emitter.subscribe(InternalEventType::new_input_line, [this](SDL_Event& e) {
            auto* str = static_cast<std::u32string*>(e.user.data1);
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
        {
            std::scoped_lock l(m);
            completed = true;
            completed.notify_one();
        }
        std::scoped_lock bl(busy_mutex);
    }

    /* This function may be called recursively */
    int
    wait_get(std::string& buf)
    {
        std::scoped_lock busy_lock(busy_mutex);
        std::unique_lock<std::recursive_mutex> lock(m);
        if (input_q.empty()) {
            lock.unlock();
            completed.wait(false);
            lock.lock();
            completed = false;
            if (input_q.empty()) {
                return 0;
            }
        }
        buf = to_utf8(input_q.front());
        input_q.pop();
        return buf.length();
    }

    std::recursive_mutex busy_mutex;

private:
    std::recursive_mutex m;
    std::queue<std::u32string> input_q;
    std::atomic<bool> completed;
};

struct LogScreen : public Widget {
    std::deque<LogEntry> entries;
    Prompt prompt;

    int scroll_offset { 0 };
    int max_lines { default_scrollback }; /* max numbers of lines allowed */
    int num_lines { 0 };
    bool mouse_depressed { false };

    LogScreen(Widget* parent)
        : Widget(parent)
        , prompt(this)
    {
        subscribe_global(SDL_MOUSEBUTTONDOWN, [this](SDL_Event& e) {
            on_mouse_button_down(e.button);
        });

        subscribe_global(SDL_MOUSEBUTTONUP, [this](SDL_Event& e) {
            on_mouse_button_up(e.button);
        });

        subscribe_global(SDL_MOUSEWHEEL, [this](SDL_Event& e) {
            on_scroll(e.wheel.y);
        });

        subscribe_global(SDL_MOUSEMOTION, [this](SDL_Event& e) {
            if (!in_rect(e.button.x, e.button.y, viewport))
                return;
            if (mouse_depressed) {
                set_mouse_motion_end({ e.motion.x, e.motion.y });
            }
        });

        subscribe_global(SDL_KEYDOWN, [this](SDL_Event& e) {
            on_key_down(e.key);
        });

        subscribe_global(SDL_TEXTINPUT, [this](SDL_Event& e) {
            scroll_offset = 0;
        });
    }

    int on_key_down(const SDL_KeyboardEvent& e)
    {
        auto sym = e.keysym.sym;
        switch (sym) {
        case SDLK_TAB:
            on_new_input_line(from_utf8("(tab)"));
            break;
        /* copy */
        case SDLK_c:
            if (SDL_GetModState() & KMOD_CTRL) {
                on_set_clipboard_text();
            }
            break;

        /* paste */
        case SDLK_v:
            if (SDL_GetModState() & KMOD_CTRL) {
                on_get_clipboard_text();
            }
            break;

        case SDLK_PAGEUP:
            on_scroll(ScrollDirection::page_up);
            break;

        case SDLK_PAGEDOWN:
            on_scroll(ScrollDirection::page_down);
            break;

        case SDLK_RETURN:
            on_new_input_line(prompt.input);
        case SDLK_BACKSPACE:
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            scroll_offset = 0;
            break;
        }
        return 0;
    }

    void on_get_clipboard_text()
    {
        auto* str = SDL_GetClipboardText();
        if (*str != '\0')
            prompt.add_input(from_utf8(str));
        SDL_free(str);
    }

    void on_mouse_button_down(SDL_MouseButtonEvent& e)
    {
        if (!in_rect(e.x, e.y, viewport))
            return;

        if (e.button != SDL_BUTTON_LEFT) {
            return;
        }

        mouse_motion_end = { -1, -1 };
        mouse_depressed = true;
        set_mouse_motion_begin({ e.x, e.y });
    }

    void on_mouse_button_up(SDL_MouseButtonEvent& e)
    {
        if (!in_rect(e.x, e.y, viewport))
            return;

        mouse_depressed = false;
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

    void translate_coord(SDL_Point& p)
    {
        p.x -= viewport.x;
        p.y -= viewport.y;
    }

    void on_scroll(int y)
    {
        if (y > 0) {
            on_scroll(ScrollDirection::up);
        } else if (y < 0) {
            on_scroll(ScrollDirection::down);
        }
    }

    void on_scroll(const ScrollDirection dir)
    {
        switch (dir) {
        case ScrollDirection::up:
            scroll_offset += 1;
            break;
        case ScrollDirection::down:
            scroll_offset -= 1;
            break;
        case ScrollDirection::page_up:
            scroll_offset += rows() / 2;
            break;
        case ScrollDirection::page_down:
            scroll_offset -= rows() / 2;
            break;
        }

        scroll_offset = std::min(std::max(0, scroll_offset), num_lines - 1);
    }

    void on_resize() override
    {
        set_viewport({ viewport.x, viewport.y, parent->viewport.w, parent->viewport.h });
        num_lines = 0;
        prompt.on_resize();
        /* XXX: we probably don't need to update outside visible view */
        for (auto& e : entries) {
            update_entry(e);
        }
    }

    void set_viewport(SDL_Rect new_viewport) override
    {
        int margin = 4;
        // max width
        int w = new_viewport.w - margin;
        // max width respect to font and margin
        int wfit = (w / font->char_width) * font->char_width;
        // max height
        int h = new_viewport.h - new_viewport.y - margin;
        // max height with respect to font and margin
        int hfit = (h / font->line_height) * font->line_height;
        // Adjust height to fit equally sized rows in the screen.
        // Offset y with the delta, which may leave a bit of space below toolbar.
        // Otherwise, there might be extra space at the bottom of the prompt.
        int dh = h - hfit;

        viewport.x = margin;
        viewport.y = new_viewport.y + dh;
        viewport.w = wfit;
        viewport.h = hfit;
    }

    void on_new_output_line(std::u32string text)
    {
        LogEntry& l = create_entry(EntryType::output, std::move(text));
        update_entry(l);
    }

    void on_new_input_line(const std::u32string text)
    {
        auto both = prompt.prompt_text + text;
        auto& l = create_entry(EntryType::input, both);
        // prompt.history.emplace_back(text);
        // prompt.history_idx = prompt.history.size();

        update_entry(l);

        emit_global(InternalEventType::new_input_line, &prompt.input);

        prompt.input.clear();
        prompt.cursor = 0;
        prompt.rebuild = true;
    }

    void update_entry(
        LogEntry& entry)
    {
        make_logentry_lines(*this, entry, entry.text);
        num_lines += entry.size;
    }

    /*
     * Create a new line and set it to be the head. This function will
     * automatically cycle-out lines if the number of lines has reached the max.
     */
    LogEntry&
    create_entry(const EntryType line_type,
        const std::u32string text)
    {
        entries.emplace_front(line_type, text);

        /* When the list is too long, start chopping the tail off each new line */
        if (num_lines >= max_lines) {
            num_lines -= entries.back().size;
            entries.pop_back();
        }

        return entries.front();
    }

    // XXX: cleanup
    void on_set_clipboard_text()
    {
        std::u32string ret;
        const std::u32string sep(U"\n");

        auto rects = get_highlighted_line_rects();
        std::reverse(rects.begin(), rects.end());
        for (auto entry_rit = entries.rbegin(); entry_rit != entries.rend(); ++entry_rit) {
            auto& entry = *entry_rit;
            if (!ret.empty())
                ret += sep;

            for (auto& line : entry.lines()) {
                for (auto& rect : rects) {
                    SDL_Point p = { rect.x, rect.y };
                    if (SDL_PointInRect(&p, &line.rect)) {
                        size_t start_x = line.start_index + (rect.x / font->char_width);
                        size_t extent = (rect.w / font->char_width) + start_x;
                        ret += entry.text.substr(start_x, std::min(extent - start_x, line.end_index - start_x));
                    }
                }
            }
        }

        SDL_SetClipboardText(to_utf8(ret).c_str());
    }

    int columns()
    {
        return (float)viewport.w / (float)font->char_width;
    }

    int rows()
    {
        return (float)viewport.h / (float)font->line_height;
    }

    void render() override
    {
        SDL_RenderSetViewport(renderer(), &viewport);
        prompt.maybe_rebuild();
        render_highlighted_lines();
        render_lines();
        // Prompt input rendering is done in render_lines()
        prompt.render_cursor(scroll_offset);
        SDL_RenderSetViewport(renderer(), &parent->viewport);
    }

    // XXX: cleanup
    void render_lines()
    {
        const int max_row = rows() + scroll_offset;
        int ypos = viewport.h;

        int cur_row = 0;
        LogEntry& entry = prompt.entry;
        for (auto it = entry.lines().rbegin(); it != entry.lines().rend(); ++it) {
            cur_row++;
            if (cur_row <= scroll_offset) {
                continue;
            }

            auto& line = *it;
            ypos -= line.rect.h;
            // record y position of this line
            line.rect.y = ypos;
            render_texture(renderer(), line.texture, line.rect);
        }

        if (entries.empty())
            return;

        for (auto& entry : entries) {
            for (auto rit = entry.lines().rbegin(); rit != entry.lines().rend(); ++rit) {
                cur_row++;
                if (cur_row <= scroll_offset) {
                    continue;
                }

                auto& line = *rit;
                // record y position of this line
                line.rect.y = ypos - line.rect.h;
                render_texture(renderer(), line.texture, line.rect);
                ypos -= line.rect.h;

                if (cur_row > max_row)
                    goto leave;
            }
        }
    leave:
        return;
    }

    void render_highlighted_lines()
    {
        if (mouse_motion_end.y == -1)
            return;

        auto rects = get_highlighted_line_rects();
        if (rects.empty())
            return;

        set_draw_color(renderer(), colors::mediumgray);
        for (auto& rect : rects) {

            SDL_RenderFillRect(renderer(), &rect);
        }
        set_draw_color(renderer(), colors::darkgray);
    }

    // XXX: fix shimmering when going from bottom to top or right to left
    std::vector<SDL_Rect> get_highlighted_line_rects()
    {
        int char_width = font->char_width;
        int line_height = font->line_height;
        const SDL_Point& mouse_start = mouse_motion_start;
        const SDL_Point& mouse_end = mouse_motion_end;

        // Calculate the start and end positions, snapping to line and character boundaries
        SDL_Rect srect;
        srect.x = std::min(mouse_start.x, mouse_end.x);
        srect.x = std::floor(srect.x / char_width) * char_width;

        srect.w = std::abs(mouse_end.x - mouse_start.x);
        srect.w = std::ceil(static_cast<float>(srect.w) / static_cast<float>(char_width)) * char_width;

        srect.y = std::min(mouse_start.y, mouse_end.y);
        srect.y = std::floor(static_cast<float>(srect.y) / static_cast<float>(line_height)) * line_height;

        srect.h = std::abs(mouse_end.y - mouse_start.y);
        srect.h = std::ceil(static_cast<float>(srect.h) / static_cast<float>(line_height)) * line_height;

        int start_x = (mouse_start.y <= mouse_end.y) ? mouse_start.x : mouse_end.x;
        start_x = std::floor(start_x / char_width) * char_width;

        SDL_Rect cur_rect = { start_x, srect.y, srect.w, line_height };
        int rows = srect.h / line_height;

        std::vector<SDL_Rect> rects;
        rects.push_back(cur_rect);
        if (rows == 1)
            return rects;

        // Handle intermediate rows
        cur_rect.y += line_height;
        for (int i = 1; i < rows; i++) {
            rects.back().w = viewport.w;
            cur_rect.x = 0;

            cur_rect.w = std::ceil(static_cast<float>(cur_rect.w) / static_cast<float>(char_width)) * char_width;

            rects.push_back(cur_rect);
            cur_rect.y += line_height;
        }

        // Adjust width for last row with respect to direction
        if (mouse_start.y > mouse_end.y) {
            rects.back().w = mouse_start.x;
        } else {
            rects.back().w = mouse_end.x;
        }

        return rects;
    }

    LogScreen(const LogScreen&) = delete;
    LogScreen& operator=(const LogScreen&) = delete;

    SDL_Point mouse_motion_start { -1, -1 };
    SDL_Point mouse_motion_end { -1, -1 };
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

struct Window : public Widget {
    WidgetContext widget_context;
    SDL_Window* handle { nullptr };
    SDL_Point mouse_coord {}; // stores mouse position relative to window
    std::unique_ptr<Toolbar> toolbar; // optional toolbar. XXX: implementation requires it
    LogScreen log_screen;
    void render() {};

    Window(WindowContext winctx, Font* font, EventEmitter& emitter)
        : Widget(font, widget_context, winctx.rect)
        , widget_context(winctx.renderer, &emitter, mouse_coord)
        , handle(winctx.handle)
        , log_screen(this)
    {
        subscribe_global(SDL_WINDOWEVENT, [this](SDL_Event& e) {
            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                on_resize();
            }
        });

        subscribe_global(SDL_MOUSEMOTION, [this](SDL_Event& e) {
            mouse_coord.x = e.button.x;
            mouse_coord.y = e.button.y;
        });

        SDL_SetWindowMinimumSize(handle, 64, 48);
        SDL_RenderSetIntegerScale(renderer(), SDL_TRUE);

        toolbar = std::make_unique<Toolbar>(this);

        toolbar->set_viewport({ 0, 0, viewport.w, font->line_height * 2 });
        log_screen.set_viewport({ 0, toolbar->viewport.h, viewport.w, viewport.h });
    }

    ~Window()
    {
        if (renderer()) {
            SDL_DestroyRenderer(renderer());
        }
        if (handle) {
            SDL_DestroyWindow(handle);
        }
    }

    // XXX: cleanup
    void on_resize() override
    {
        SDL_GetRendererOutputSize(renderer(), &viewport.w, &viewport.h);
        SDL_RenderSetViewport(renderer(), &viewport);
        toolbar->on_resize();
        log_screen.on_resize();
    }

    static WindowContext create(const char* title, int x, int y, int w, int h, Uint32 flags)
    {
        SDL_Window* handle = SDL_CreateWindow(title, x, y, w, h, flags);
        if (!handle) {
            throw std::runtime_error("Failed to create SDL window");
        }

        SDL_Renderer* renderer = SDL_CreateRenderer(handle, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            SDL_DestroyWindow(handle);
            throw std::runtime_error("Failed to create SDL renderer");
        }

        SDL_Rect rect = {};
        SDL_GetRendererOutputSize(renderer, &rect.w, &rect.h);
        return WindowContext(handle, renderer, rect);
    }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

Toolbar::Toolbar(Widget* parent)
    : Widget(parent) {};

void Toolbar::render()
{
    set_draw_color(renderer(), colors::charcoal);

    // Render bg
    // SDL_RenderFillRect(renderer(), &viewport);
    // Draw a border
    SDL_RenderDrawRect(renderer(), &viewport);

    int margin_right = font->char_width;
    int x = (parent->viewport.w - margin_right) - compute_widgets_startx();

    // Lay out horizontally
    for (auto& w : widgets) {
        w->viewport.x = x;
        x += w->viewport.w;

        w->render();
    }

    set_draw_color(renderer(), colors::darkgray);
}

void Toolbar::on_resize()
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

int Toolbar::compute_widgets_startx()
{
    int x = 0;
    for (auto& w : widgets) {
        x += w->viewport.w;
    }
    return x + (widgets.size() - 1);
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
                notifier = true; /* Mutex may not be necessary, but extra paranoid! */
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
        : sdl(notifier, status)
        , api(notifier, status)
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

    void drain()
    {
        SDL_Event e;
        while (sdl.pop(e))
            ;
        Task t;
        while (api.pop(t))
            ;
    }

    void reset()
    {
        drain();
        std::scoped_lock lock(sdl.mutex, api.mutex);
        status = State::active;
    }

    void shutdown()
    {
        {
            std::scoped_lock lock(sdl.mutex, api.mutex);
            status = State::shutdown;
        }
        drain();
        std::scoped_lock l(busy_mutex);
    }

    ExternalEventWaiter(const ExternalEventWaiter&) = delete;
    ExternalEventWaiter& operator=(const ExternalEventWaiter&) = delete;

    std::mutex busy_mutex;
    EventQueue<SDL_Event> sdl;
    using Task = std::function<void()>;
    EventQueue<Task> api;

private:
    Notifier notifier { false };
    State status = { State::active };
};

bool in_rect(int x, int y, SDL_Rect& r)
{
    return ((x >= r.x) && (x < (r.x + r.w)) && (y >= r.y) && (y < (r.y + r.h)));
}

bool in_rect(SDL_Point& p, SDL_Rect& r)
{
    return SDL_PointInRect(&p, &r);
}

void render_texture(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    const SDL_Rect& dst)
{
    SDL_RenderCopy(renderer, texture, NULL, &dst);
}

int set_draw_color(SDL_Renderer* renderer, const SDL_Color& color)
{
    return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

// Used by Prompt and LogScreen
// XXX: cleanup.
void make_logentry_lines(
    Widget& widget,
    LogEntry& entry,
    std::u32string& text)
{
    entry.clear();
    // Break up the text into line segments, if needed
    int advance = widget.font->char_width;
    int delim_idx = 0; // last whitespace character for wrapping on word boundaries
    int start_idx = 0;
    int end_idx = 0;
    std::vector<std::pair<int, int>> segments;
    for (auto& ch : text) {
        if (ch == U'\n' || ch == U'\r') {
            // Not including the new line character
            // Don't attempt to add an empty segment
            if (end_idx - start_idx > 0)
                segments.emplace_back(start_idx, end_idx);
            start_idx = end_idx + 1;
            delim_idx = 0;
        } else if (ch == U' ' || ch == U'\t') {
            delim_idx = end_idx;
        } else {
            // check if width exceeded
            if (((end_idx - start_idx + 1) * advance) >= widget.viewport.w) {
                if (delim_idx) {
                    // wrap at last whitespace
                    segments.emplace_back(start_idx, delim_idx + 1);
                    start_idx = delim_idx + 1;
                } else {
                    // wrap at last character
                    segments.emplace_back(start_idx, end_idx + 1);
                    start_idx = end_idx + 1;
                }
                delim_idx = 0;
            }
        }

        end_idx++;
    }
    //  Handle any remaining text
    if (end_idx > start_idx) {
        // second idx = -1
        segments.emplace_back(start_idx, std::u32string::npos);
    }

    for (auto idx_pair : segments) {
        if (idx_pair.second - idx_pair.first == 0) {
            // XXX: Sanity check.
            // std:cerr << "attempt to use empty segment" << endl;
            continue;
        }
        // printf("Adding Line: '%s'\n", to_utf8(text.substr(idx_pair.first, idx_pair.second - idx_pair.first)).c_str());
        SDL_Texture* texture = create_text_texture(widget, text.substr(idx_pair.first, idx_pair.second - idx_pair.first), colors::white);
        entry.add_line(texture, idx_pair.first, idx_pair.second);
    }
}
// TODO: handle errors properly
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

int on_sdl_event(void* data, SDL_Event* e);
}

using namespace console;

/* Note: when an an SDL_EventFilter is set, SDL2 deletes all pending events */
struct SDLEventFilterSetter {
    SDLEventFilterSetter(SDL_EventFilter filter, void* user_data)
    {
        SDL_GetEventFilter(&saved_filter, &saved_user_data);
        SDL_SetEventFilter(filter, user_data);
    }

    ~SDLEventFilterSetter()
    {
        // reset_saved() gets called on shutdown but in the event
        // something very bad happened, do it here.
        if (!did_reset_saved)
            SDL_SetEventFilter(saved_filter, saved_user_data);
    }

    void reset_saved()
    {
        did_reset_saved = true;
        SDL_SetEventFilter(saved_filter, saved_user_data);
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
        EventEmitter internal_emitter;
        Window window;
        std::unique_ptr<FontLoader> font_loader;
        SDL_Color bg_color; // not currently used
        SDL_Color font_color; // not currently used
        InputLineWaiter input_line_waiter;
        ExternalEventWaiter& external_event_waiter;
        SDLEventFilterSetter event_filter_setter;
        std::thread::id render_thread_id;

        Impl(WindowContext wctx, std::unique_ptr<FontLoader> fl, ExternalEventWaiter& external_event_waiter)
            : window(wctx, fl->get_font(), internal_emitter)
            , font_loader(std::move(fl))
            , input_line_waiter(internal_emitter)
            , external_event_waiter(external_event_waiter)
            , event_filter_setter(on_sdl_event, this)
            , render_thread_id(std::this_thread::get_id())
        {
            external_event_waiter.reset();
            SDL_StartTextInput();
        };

        ~Impl()
        {
            SDL_StopTextInput();
            /* SDLEventFilter destructor resets event filter to saved event filter.
             * SDL_DestroyWindow handled by Window destructor.
             */
        }
    };

    Console_con() = default;
    ~Console_con() = default;

    void init(WindowContext wctx, std::unique_ptr<FontLoader> fl)
    {
        impl = std::make_unique<Impl>(wctx, std::move(fl), external_event_waiter);
    }

    bool is_active()
    {
        return status == State::active;
    }

    bool is_shutdown()
    {
        return status == State::shutdown;
    }

    LogScreen& lscreen()
    {
        return impl->window.log_screen;
    }

    /* Stores SDL events and API tasks to be run on the render thread.
     * SDL events should be drained from it on shutdown.
     * API tasks should be drained as well, but just in case
     * we'll leave the storage in place after destroy(),
     * and instruct the queues not to accept more items.
     */
    ExternalEventWaiter external_event_waiter;
    std::atomic<State> status { State::active };
    std::unique_ptr<Impl> impl;
    std::mutex mutex;
};
static Console_con num_con[1];

namespace console {

// XXX: move rendering to Window
int render_frame(Console_con::Impl* impl)
{
    assert(impl);

    // Should not fail unless memory starvation.
    SDL_RenderClear(impl->window.renderer());

    //  set background color
    // Should not fail unless renderer is invalid
    set_draw_color(impl->window.renderer(), colors::darkgray);

    impl->window.toolbar->render();

    /* render text area */

    impl->window.log_screen.render();

    SDL_RenderPresent(impl->window.renderer());

    return 0;
}

int handle_sdl_event(Console_con::Impl* impl, SDL_Event& e)
{
    impl->internal_emitter.emit(e);
    return 0;
}

int on_sdl_event(void* data, SDL_Event* e)
{
    auto impl = static_cast<Console_con::Impl*>(data);
    std::scoped_lock l(impl->external_event_waiter.busy_mutex);

    if (e->type == SDL_QUIT)
        return impl->event_filter_setter.maybe_call_saved(data, e);

    // maybe also check SDL_GetMouseFocus
    const Uint32 flags = SDL_GetWindowFlags(impl->window.handle);
    if (!(flags & SDL_WINDOW_INPUT_FOCUS)) {
        return impl->event_filter_setter.maybe_call_saved(data, e);
    }

    SDL_Event ec;
    std::memcpy(&ec, e, sizeof(SDL_Event));
    impl->external_event_waiter.sdl.push(ec);
    return 0;
}
}

// XXX: cleanup
Console_con*
Console_Create(const char* title,
    const char* prompt,
    const int font_size)

{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL failed to init: " << SDL_GetError();
        return nullptr;
    }

    try {
        WindowContext wctx = Window::create(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            640, 480,
            SDL_WINDOW_RESIZABLE);

        auto font_loader = std::make_unique<FontLoader>();
        auto font = font_loader->open("source_code_pro.ttf", font_size);
        if (font == nullptr) {
            std::cerr << "error initializing TTF: " << TTF_GetError();
            return nullptr;
        }

        Console_con* con = &num_con[0];
        con->init(wctx, std::move(font_loader));

        Widget* copy = con->impl->window.toolbar->add_button(U"Copy");
        copy->subscribe(InternalEventType::clicked, [con](SDL_Event& e) {
            con->lscreen().on_set_clipboard_text();
        });

        Widget* paste = con->impl->window.toolbar->add_button(U"Paste");
        paste->subscribe(InternalEventType::clicked, [con](SDL_Event& e) {
            con->lscreen().on_get_clipboard_text();
        });

        con->lscreen().prompt.set_prompt(from_utf8(prompt));
        con->status = State::active;

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
        std::cerr << e.what() << std::endl;
        return nullptr;
    }
}

void Console_SetPrompt(Console_con* con,
    const char* prompt)
{
    auto str = from_utf8(prompt);
    con->impl->external_event_waiter.api.push([con, str = std::move(str)] {
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
                handle_sdl_event(impl, event);
            }
            ExternalEventWaiter::Task f;
            while (impl->external_event_waiter.api.pop(f)) {
                f();
            }
        }

        if (con->is_shutdown()) {
            impl->event_filter_setter.reset_saved();
            impl->input_line_waiter.shutdown();
            impl->external_event_waiter.shutdown();
            break;
        }
    }
    return 0;
}

void Console_AddLine(Console_con* con, const char* s)
{
    auto str = from_utf8(s);
    con->impl->external_event_waiter.api.push([con, str = std::move(str)] {
        con->lscreen().on_new_output_line(str);
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
    con->impl->external_event_waiter.api.push([con] {
        con->lscreen().entries.clear();
    });
}

void Console_Shutdown(Console_con* con)
{
    assert(con);
    con->status = State::shutdown;
    // Must push an event to wake up the main render thread
    con->impl->external_event_waiter.api.push([con] {});
}

// XXX: cleanup properly
bool Console_Destroy(Console_con* con)
{
    assert(con);
    if (con->status == State::inactive)
        return true;

    if (std::this_thread::get_id() != con->impl->render_thread_id)
        return false;

    con->status = State::inactive;
    con->impl.reset();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return true;
}

int Console_GetLine(Console_con* con, std::string& buf)
{
    if (!con->is_active())
        return -1;

    return con->impl->input_line_waiter.wait_get(buf);
}

void Console_SetScrollback(Console_con* con, const int lines)
{
    con->impl->external_event_waiter.api.push([con, lines = lines] {
        con->lscreen().max_lines = lines;
    });
}

const char*
Console_GetError(void)
{
    // TODO:
    return "";
}

// kate: replace-tabs on; indent-width 4;
