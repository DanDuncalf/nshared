#include "tui.hpp"

// Include the platform C library
#include "platform.h"

// Windows-specific for raw key reading
#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace gittui {

// ---------------------------------------------------------------------------
// KeyInjector
// ---------------------------------------------------------------------------

KeyInjector& KeyInjector::instance() {
    static KeyInjector inst;
    return inst;
}

static uint16_t parse_key_token_impl(const std::string& token) {
    if (token == "UP")         return GT_VK_UP;
    if (token == "DOWN")       return GT_VK_DOWN;
    if (token == "LEFT")       return GT_VK_LEFT;
    if (token == "RIGHT")      return GT_VK_RIGHT;
    if (token == "PGUP")       return GT_VK_PRIOR;
    if (token == "PGDN")       return GT_VK_NEXT;
    if (token == "HOME")       return GT_VK_HOME;
    if (token == "END")        return GT_VK_END;
    if (token == "ENTER")      return GT_VK_RETURN;
    if (token == "ESC")        return GT_VK_ESCAPE;
    if (token == "TAB")        return GT_VK_TAB;
    if (token == "BACKSPACE")  return GT_VK_BACK;
    if (token == "SPACE")      return 0x20;
    if (token == "DELETE")     return 0x2E;
    if (token == "PLUS")       return '+';
    if (token == "MINUS")      return '-';
    if (token.size() >= 5 && token.substr(0, 5) == "TEXT:") {
        return 0; // signals text expansion
    }
    if (token.size() == 1) {
        return char_to_vk(token[0]);
    }
    return 0;
}

uint16_t parse_key_token(std::string_view token) {
    return parse_key_token_impl(std::string(token));
}

void KeyInjector::parse_env() {
    reset();

    const char* keys_env = nullptr;
#ifdef _WIN32
    _dupenv_s(&const_cast<char*&>(keys_env), nullptr, "GT_UI_KEYS");
#else
    keys_env = std::getenv("GT_UI_KEYS");
#endif

    if (keys_env && keys_env[0]) {
        inject_keys(std::string_view(keys_env));
    }

#ifdef _WIN32
    free(const_cast<char*>(keys_env));
#endif

    const char* timeout_env = nullptr;
#ifdef _WIN32
    _dupenv_s(&const_cast<char*&>(timeout_env), nullptr, "GT_UI_KEY_TIMEOUT_MS");
#else
    timeout_env = std::getenv("GT_UI_KEY_TIMEOUT_MS");
#endif

    if (timeout_env) {
        timeout_ms_ = static_cast<uint32_t>(std::strtoul(timeout_env, nullptr, 10));
    }

#ifdef _WIN32
    free(const_cast<char*>(timeout_env));
#endif
}

void KeyInjector::reset() {
    queue_.clear();
    index_ = 0;
    timeout_ms_ = 0;
    keys_were_injected_ = false;
}

void KeyInjector::inject_keys(std::string_view keys) {
    keys_were_injected_ = true;
    std::string str(keys);
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, ',')) {
        while (!token.empty() && token.front() == ' ') token.erase(0, 1);
        while (!token.empty() && token.back() == ' ') token.pop_back();
        if (token.empty()) continue;
        if (token.size() >= 5 && token.substr(0, 5) == "TEXT:") {
            std::string text = token.substr(5);
            for (char c : text) {
                queue_.push_back(char_to_vk(c));
            }
        } else {
            uint16_t vk = parse_key_token_impl(token);
            if (vk != 0) {
                queue_.push_back(vk);
            }
        }
    }
}

bool KeyInjector::has_injected_key() const {
    return index_ < queue_.size();
}

uint16_t KeyInjector::next_injected_key() {
    if (index_ < queue_.size()) {
        return queue_[index_++];
    }
    return 0;
}

// ---------------------------------------------------------------------------
// TestBackend — in-memory char grid
// ---------------------------------------------------------------------------

TestBackend::TestBackend() {
    const char* mode = nullptr;
#ifdef _WIN32
    _dupenv_s(&const_cast<char*&>(mode), nullptr, "GT_TEST_MODE");
#else
    mode = std::getenv("GT_TEST_MODE");
#endif

    if (mode && mode[0]) {
        int c = 0, r = 0;
        if (std::sscanf(mode, "%d,%d", &c, &r) == 2 && c > 0 && r > 0) {
            cols = (c < GT_TEST_MAX_COLS) ? c : GT_TEST_MAX_COLS;
            rows = (r < GT_TEST_MAX_ROWS) ? r : GT_TEST_MAX_ROWS;
        }
    } else {
        cols = 80;
        rows = 25;
    }

#ifdef _WIN32
    free(const_cast<char*>(mode));
#endif

    clear();
}

TestBackend::~TestBackend() {
    if (dump_on_close) {
        dump_to_stdout();
    }
}

void TestBackend::clear() {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c].ch = ' ';
            grid[r][c].attr = Color::kDefault;
        }
    }
}

void TestBackend::write_at(int r, int c, std::string_view text) {
    if (r < 0 || r >= rows || c < 0) return;
    for (size_t i = 0; i < text.size(); ++i) {
        int cc = c + static_cast<int>(i);
        if (cc >= 0 && cc < cols) {
            grid[r][cc].ch = text[i];
        }
    }
}

void TestBackend::fill_rect(int r, int c, int h, int w, char ch, uint16_t attr) {
    for (int dr = 0; dr < h; ++dr) {
        for (int dc = 0; dc < w; ++dc) {
            int rr = r + dr;
            int cc = c + dc;
            if (rr >= 0 && rr < rows && cc >= 0 && cc < cols) {
                grid[rr][cc].ch = ch;
                grid[rr][cc].attr = attr;
            }
        }
    }
}

void TestBackend::fill_attr(int r, int c, int count, uint16_t attr) {
    (void)attr;
    for (int i = 0; i < count; ++i) {
        int cc = c + i;
        if (cc >= 0 && cc < cols) {
            grid[r][cc].attr = attr;
        }
    }
}

char TestBackend::cell_char(int r, int c) const {
    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        return grid[r][c].ch;
    }
    return ' ';
}

std::string TestBackend::row_text(int r) const {
    if (r < 0 || r >= rows) return {};
    std::string result;
    for (int c = 0; c < cols; ++c) {
        result += grid[r][c].ch;
    }
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

int TestBackend::find_row(std::string_view substr) const {
    std::string needle(substr);
    for (int r = 0; r < rows; ++r) {
        std::string text = row_text(r);
        if (text.find(needle) != std::string::npos) {
            return r;
        }
    }
    return -1;
}

void TestBackend::dump_to_stdout() const {
    for (int r = 0; r < rows; ++r) {
        std::cout << row_text(r) << "\n";
    }
}

std::string TestBackend::as_string() const {
    std::string result;
    for (int r = 0; r < rows; ++r) {
        result += row_text(r);
        result += '\n';
    }
    return result;
}

TestBackend* test_backend_global() {
    static TestBackend global_backend;
    return &global_backend;
}

// ---------------------------------------------------------------------------
// Terminal — Windows implementation
// ---------------------------------------------------------------------------

#ifdef _WIN32

static const char* getenv_safe(const char* name, char** buf) {
    return _dupenv_s(buf, nullptr, name) == 0 ? *buf : nullptr;
}

// Convert UTF-8 to wchar_t for WriteConsoleW (box-drawing chars)
static std::wstring utf8_to_wide(std::string_view text) {
    if (text.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0);
    if (len <= 0) return {};
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), &result[0], len);
    return result;
}

// Set console attributes (foreground + background)
static void set_console_attr(PlatformHandle out, int row, int col, int count, uint16_t attr) {
    if (!out || count <= 0) return;
    COORD coord;
    coord.X = (SHORT)col;
    coord.Y = (SHORT)row;
    DWORD written;
    FillConsoleOutputAttribute((HANDLE)out, (WORD)attr, (DWORD)count, coord, &written);
}

Terminal::Terminal()
    : out_(nullptr)
    , in_(nullptr)
    , orig_input_mode_(0)
    , width_(0)
    , height_(0)
    , valid_(false)
    , input_mode_saved_(false)
    , test_mode_(false)
    , test_backend_(nullptr)
{
    KeyInjector::instance().parse_env();

    char* test_mode_env = nullptr;
    if (getenv_safe("GT_TEST_MODE", &test_mode_env)) {
        if (test_mode_env[0]) {
            test_mode_ = true;
            test_backend_ = new TestBackend();
            width_ = test_backend_->cols;
            height_ = test_backend_->rows;
            valid_ = true;
        }
    }
    free(test_mode_env);
    if (test_mode_) return;

    out_ = GetStdHandle(STD_OUTPUT_HANDLE);
    in_ = platform_console_open_in();
    if (!out_ || out_ == INVALID_HANDLE_VALUE || !in_) {
        out_ = nullptr;
        return;
    }

    // Enable ANSI on the standard output handle
    DWORD mode = 0;
    if (GetConsoleMode(out_, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(out_, mode);
    }

    input_mode_saved_ = platform_console_get_mode(in_, &orig_input_mode_);
    refresh_size();
    valid_ = true;
}

Terminal::~Terminal() {
    if (test_mode_) {
        if (test_backend_) {
            delete test_backend_;
            test_backend_ = nullptr;
        }
        return;
    }
    if (valid_) {
        set_cursor_visible(true);
        if (input_mode_saved_) {
            platform_console_set_mode(in_, orig_input_mode_);
        }
    }
    if (out_) platform_handle_close(out_);
    if (in_) platform_handle_close(in_);
}

void Terminal::refresh_size() {
    if (!out_) return;
    PlatformConsoleInfo info;
    if (platform_console_get_info(out_, &info)) {
        width_ = info.window_right - info.window_left + 1;
        height_ = info.window_bottom - info.window_top + 1;
    }
}

void Terminal::set_cursor(int row, int col) {
    if (test_mode_) return;
    if (!out_) return;
    platform_console_set_cursor(out_, row, col);
}

void Terminal::write_at(int row, int col, std::string_view text, uint16_t attr) {
    if (test_mode_ && test_backend_) {
        test_backend_->write_at(row, col, text);
        return;
    }
    if (!out_ || text.empty()) return;

    std::wstring wide = utf8_to_wide(text);
    SHORT char_count = (SHORT)wide.size();

    COORD bufSize = { char_count, 1 };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion;
    writeRegion.Left = (SHORT)col;
    writeRegion.Top = (SHORT)row;
    writeRegion.Right = (SHORT)(col + char_count - 1);
    writeRegion.Bottom = (SHORT)row;

    std::vector<CHAR_INFO> buffer(char_count);
    for (SHORT i = 0; i < char_count; ++i) {
        buffer[i].Char.UnicodeChar = wide[i];
        buffer[i].Attributes = attr;
    }

    WriteConsoleOutputW((HANDLE)out_, buffer.data(), bufSize, bufCoord, &writeRegion);
}

// Write text chars with default attribute (white on black).
void Terminal::write_chars(int row, int col, std::string_view text) {
    write_at(row, col, text, Color::kDefault);
}

// Fill attributes for a row AFTER text has been written.
// This is used for highlighted rows: write chars first, then fill bg+fg attrs on top.
// Uses ANSI escape sequences since FillConsoleOutputAttribute doesn't work in
// modern Windows Terminal.
void Terminal::fill_attr_after(int row, int col, int count, uint16_t attr) {
    if (test_mode_ && test_backend_) {
        test_backend_->fill_attr(row, col, count, attr);
        return;
    }
    if (!out_ || count <= 0) return;
    COORD pos = { (SHORT)col, (SHORT)row };
    DWORD written;
    FillConsoleOutputAttribute((HANDLE)out_, (WORD)attr, (DWORD)count, pos, &written);
}

void Terminal::clear_screen() {
    if (test_mode_ && test_backend_) {
        test_backend_->clear();
        return;
    }
    if (!out_) return;
    // Use WriteConsoleOutput for fast bulk clear
    COORD bufSize = { (SHORT)width_, (SHORT)height_ };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(width_ - 1), (SHORT)(height_ - 1) };
    std::vector<CHAR_INFO> buffer(width_ * height_);
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i].Char.UnicodeChar = L' ';
        buffer[i].Attributes = gittui::Color::kDefault;
    }
    WriteConsoleOutputW((HANDLE)out_, buffer.data(), bufSize, bufCoord, &writeRegion);
    set_cursor(0, 0);
}

void Terminal::clear_line(int row) {
    if (test_mode_ && test_backend_) {
        if (row >= 0 && row < height_ && test_backend_) {
            for (int c = 0; c < width_; ++c) {
                test_backend_->grid[row][c].ch = ' ';
                test_backend_->grid[row][c].attr = Color::kDefault;
            }
        }
        return;
    }
    if (!out_) return;
    COORD bufSize = { (SHORT)width_, 1 };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, (SHORT)row, (SHORT)(width_ - 1), (SHORT)row };
    std::vector<CHAR_INFO> buffer(width_);
    for (int i = 0; i < width_; ++i) {
        buffer[i].Char.UnicodeChar = L' ';
        buffer[i].Attributes = gittui::Color::kDefault;
    }
    WriteConsoleOutputW((HANDLE)out_, buffer.data(), bufSize, bufCoord, &writeRegion);
}

void Terminal::fill_rect(int row, int col, int h, int w, char ch, uint16_t attr) {
    if (test_mode_ && test_backend_) {
        test_backend_->fill_rect(row, col, h, w, ch, attr);
        return;
    }
    if (!out_) return;
    for (int i = 0; i < h; ++i) {
        if (row + i >= 0 && row + i < height_) {
            platform_console_fill_char(out_, row + i, col, w, ch);
            set_console_attr(out_, row + i, col, w, attr);
        }
    }
}

void Terminal::fill_attr_default(int row, int col, int count) {
    if (test_mode_ && test_backend_) {
        test_backend_->fill_attr(row, col, count, Color::kDefault);
        return;
    }
    if (!out_ || count <= 0) return;
    set_console_attr(out_, row, col, count, gittui::Color::kDefault);
}

void Terminal::set_cursor_visible(bool visible, int size_percent) {
    if (test_mode_) return;
    if (!out_) return;
    platform_console_set_cursor_visible(out_, visible, size_percent);
}

void Terminal::enable_raw_input() {
    if (test_mode_) return;
    if (!in_) return;
    unsigned long mode;
    if (platform_console_get_mode(in_, &mode)) {
        mode &= ~0x0002;
        mode &= ~0x0004;
        platform_console_set_mode(in_, mode);
    }
}

void Terminal::restore_input_mode() {
    if (test_mode_) return;
    if (!in_ || !input_mode_saved_) return;
    platform_console_set_mode(in_, orig_input_mode_);
}

uint16_t Terminal::read_key(uint32_t timeout_ms) {
    (void)timeout_ms;
    if (KeyInjector::instance().has_injected_key()) {
        uint32_t t = KeyInjector::instance().timeout_ms();
        if (t > 0) platform_sleep_ms(t);
        uint16_t vk = KeyInjector::instance().next_injected_key();
        if (vk != 0) return vk;
    }
    if (test_mode_) {
        return GT_VK_ESCAPE;
    }
    uint16_t vk = 0;
    platform_console_read_key_vk(in_, &vk);
    return vk;
}

// ---------------------------------------------------------------------------
// Terminal — POSIX/Linux implementation
// ---------------------------------------------------------------------------

#else

Terminal::Terminal()
    : out_(nullptr)
    , in_(nullptr)
    , orig_input_mode_(0)
    , width_(0)
    , height_(0)
    , valid_(false)
    , input_mode_saved_(false)
    , test_mode_(false)
    , test_backend_(nullptr)
{
    KeyInjector::instance().parse_env();

    const char* test_mode_env = std::getenv("GT_TEST_MODE");
    if (test_mode_env && test_mode_env[0]) {
        test_mode_ = true;
        test_backend_ = new TestBackend();
        width_ = test_backend_->cols;
        height_ = test_backend_->rows;
        valid_ = true;
        return;
    }

    out_ = GetStdHandle(STD_OUTPUT_HANDLE);
    in_ = platform_console_open_in();
    if (!out_ || out_ == INVALID_HANDLE_VALUE || !in_) {
        out_ = nullptr;
        return;
    }

    // Enable ANSI on the standard output handle
    DWORD mode = 0;
    if (GetConsoleMode(out_, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(out_, mode);
    }

    input_mode_saved_ = platform_console_get_mode(in_, &orig_input_mode_);
    refresh_size();
    valid_ = true;
}

Terminal::~Terminal() {
    if (test_mode_) {
        if (test_backend_) {
            delete test_backend_;
            test_backend_ = nullptr;
        }
        return;
    }
    if (valid_) {
        set_cursor_visible(true);
        if (input_mode_saved_) {
            platform_console_set_mode(in_, orig_input_mode_);
        }
    }
    if (out_) platform_handle_close(out_);
    if (in_) platform_handle_close(in_);
}

void Terminal::refresh_size() {
    if (!out_) return;
    PlatformConsoleInfo info;
    if (platform_console_get_info(out_, &info)) {
        width_ = info.window_right - info.window_left + 1;
        height_ = info.window_bottom - info.window_top + 1;
    }
}

void Terminal::set_cursor(int row, int col) {
    if (test_mode_) return;
    if (!out_) return;
    platform_console_set_cursor(out_, row, col);
}

void Terminal::write_at(int row, int col, std::string_view text, uint16_t attr) {
    if (test_mode_ && test_backend_) {
        test_backend_->write_at(row, col, text);
        return;
    }
    if (!out_ || text.empty()) return;
    platform_console_write_at(out_, row, col, std::string(text).c_str());
}

void Terminal::clear_screen() {
    if (test_mode_ && test_backend_) {
        test_backend_->clear();
        return;
    }
    if (!out_) return;
    for (int r = 0; r < height_; ++r) {
        platform_console_fill_char(out_, r, 0, width_, ' ');
        platform_console_fill_attr_default(out_, r, 0, width_);
    }
    set_cursor(0, 0);
}

void Terminal::clear_line(int row) {
    if (test_mode_ && test_backend_) {
        if (row >= 0 && row < height_ && test_backend_) {
            for (int c = 0; c < width_; ++c) {
                test_backend_->grid[row][c].ch = ' ';
                test_backend_->grid[row][c].attr = Color::kDefault;
            }
        }
        return;
    }
    if (!out_) return;
    platform_console_fill_char(out_, row, 0, width_, ' ');
    platform_console_fill_attr_default(out_, row, 0, width_);
}

void Terminal::fill_rect(int row, int col, int h, int w, char ch, uint16_t attr) {
    if (test_mode_ && test_backend_) {
        test_backend_->fill_rect(row, col, h, w, ch, attr);
        return;
    }
    if (!out_) return;
    for (int i = 0; i < h; ++i) {
        if (row + i >= 0 && row + i < height_) {
            platform_console_fill_char(out_, row + i, col, w, ch);
        }
    }
}

void Terminal::fill_attr_default(int row, int col, int count) {
    if (test_mode_ && test_backend_) {
        test_backend_->fill_attr(row, col, count, Color::kDefault);
        return;
    }
    if (!out_ || count <= 0) return;
    platform_console_fill_attr_default(out_, row, col, count);
}

void Terminal::set_cursor_visible(bool visible, int size_percent) {
    if (test_mode_) return;
    if (!out_) return;
    platform_console_set_cursor_visible(out_, visible, size_percent);
}

void Terminal::enable_raw_input() {
    if (test_mode_) return;
    if (!in_) return;
    unsigned long mode;
    if (platform_console_get_mode(in_, &mode)) {
        mode &= ~0x0002;
        mode &= ~0x0004;
        platform_console_set_mode(in_, mode);
    }
}

void Terminal::restore_input_mode() {
    if (test_mode_) return;
    if (!in_ || !input_mode_saved_) return;
    platform_console_set_mode(in_, orig_input_mode_);
}

uint16_t Terminal::read_key(uint32_t timeout_ms) {
    (void)timeout_ms;
    if (KeyInjector::instance().has_injected_key()) {
        uint32_t t = KeyInjector::instance().timeout_ms();
        if (t > 0) platform_sleep_ms(t);
        uint16_t vk = KeyInjector::instance().next_injected_key();
        if (vk != 0) return vk;
    }
    if (test_mode_) {
        return GT_VK_ESCAPE;
    }
    uint16_t vk = 0;
    platform_console_read_key_vk(in_, &vk);
    return vk;
}

#endif // _WIN32

} // namespace gittui
