#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

// Forward declarations from platform.h
struct PlatformHandle_t;
typedef void* PlatformHandle;

#ifdef _WIN32
#include <windows.h>
#endif

namespace gittui {

// Virtual key codes — use fixed type to avoid template deduction ambiguity
enum VkCode : uint16_t {
    GT_VK_UP      = 0x26,
    GT_VK_DOWN    = 0x28,
    GT_VK_LEFT    = 0x25,
    GT_VK_RIGHT   = 0x27,
    GT_VK_PRIOR   = 0x21,
    GT_VK_NEXT    = 0x22,
    GT_VK_HOME    = 0x24,
    GT_VK_END     = 0x23,
    GT_VK_RETURN  = 0x0D,
    GT_VK_ESCAPE  = 0x1B,
    GT_VK_TAB     = 0x09,
    GT_VK_BACK    = 0x08
};

// Grid dimensions for test backend
enum {
    GT_TEST_MAX_ROWS = 60,
    GT_TEST_MAX_COLS = 200
};

// ---------------------------------------------------------------------------
// Color constants — Windows console attribute flags
// ---------------------------------------------------------------------------

namespace Color {
    // Base intensity bit
    constexpr uint16_t kIntensity = 0x0008;

    // Foreground color bits
    constexpr uint16_t kForegroundRed    = 0x0004;
    constexpr uint16_t kForegroundGreen  = 0x0002;
    constexpr uint16_t kForegroundBlue   = 0x0001;

    // Composite colors
    constexpr uint16_t kWhite    = kForegroundRed | kForegroundGreen | kForegroundBlue;
    constexpr uint16_t kRed      = kForegroundRed | kIntensity;
    constexpr uint16_t kGreen    = kForegroundGreen | kIntensity;
    constexpr uint16_t kYellow   = kForegroundRed | kForegroundGreen | kIntensity;
    constexpr uint16_t kCyan     = kForegroundGreen | kForegroundBlue | kIntensity;
    constexpr uint16_t kGray     = kIntensity;
    constexpr uint16_t kBoldWhite = kWhite | kIntensity;
    constexpr uint16_t kDim      = kForegroundRed | kForegroundGreen | kForegroundBlue;
    constexpr uint16_t kDefault  = kForegroundRed | kForegroundGreen | kForegroundBlue; // 0x07 white on black

    // Highlight attributes (foreground + background)
    // Blue bg (0x10) + White fg (0x0F) = 0x1F
    constexpr uint16_t kActiveHighlight = 0x001F; // white on blue
    // Grey bg (0x70) + Black fg (0x00) = 0x70
    constexpr uint16_t kMarkerHighlight = 0x0070; // black on grey
}

// ---------------------------------------------------------------------------
// TestBackend — in-memory char grid for headless TUI testing
//
// When GT_TEST_MODE env var is set, Terminal renders to this grid instead
// of the real console. On destruction, the grid is dumped to stdout.
//
// Tests can also create their own TestBackend for direct inspection.
// ---------------------------------------------------------------------------

struct TestCell {
    char ch = ' ';
    uint16_t attr = Color::kDefault;
};

struct TestBackend {
    TestCell grid[GT_TEST_MAX_ROWS][GT_TEST_MAX_COLS]{};
    int rows = 25;
    int cols = 80;
    bool dump_on_close = true;       // dump to stdout on destruction

    TestBackend();                   // reads GT_TEST_MODE=cols,rows
    ~TestBackend();                  // dumps grid to stdout if dump_on_close

    void clear();
    void write_at(int r, int c, std::string_view text);
    void fill_rect(int r, int c, int h, int w, char ch, uint16_t attr);
    void fill_attr(int r, int c, int count, uint16_t attr);

    // Get the character at a grid position (for test assertions).
    char cell_char(int r, int c) const;

    // Get a full row as a string (trailing spaces trimmed).
    std::string row_text(int r) const;

    // Find a row containing the given substring; returns -1 if not found.
    int find_row(std::string_view substr) const;

    // Dump the grid to stdout (for expected-output comparison).
    void dump_to_stdout() const;

    // Get the entire grid as a single string (rows joined by newlines).
    std::string as_string() const;
};

// Global test backend — used when GT_TEST_MODE is set.
TestBackend* test_backend_global();

// ---------------------------------------------------------------------------
// Terminal — RAII wrapper around platform console functions
// ---------------------------------------------------------------------------

class Terminal {
public:
    Terminal();
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    // Returns false if terminal could not be initialized.
    bool is_valid() const { return valid_; }

    // Console dimensions (0-indexed).
    int width() const { return width_; }
    int height() const { return height_; }

    // Cursor positioning.
    void set_cursor(int row, int col);

    // Write text at a specific position (0-indexed row, col).
    void write_at(int row, int col, std::string_view text, uint16_t attr = Color::kDefault);

    // Write text chars only — does NOT set attributes.
    // Use when background attr is set separately (e.g. highlighted rows).
    void write_chars(int row, int col, std::string_view text);

    // Fill attributes for a range AFTER text has been written.
    // Use for highlighted rows: write chars first, then fill highlight attrs on top.
    void fill_attr_after(int row, int col, int count, uint16_t attr);

    // Clear the entire screen (fills with spaces, default attributes).
    void clear_screen();

    // Clear a single line.
    void clear_line(int row);

    // Fill a rectangular region with a character and attribute.
    void fill_rect(int row, int col, int h, int w, char ch, uint16_t attr);

    // Fill a horizontal region with default attributes (used for highlighting).
    void fill_attr_default(int row, int col, int count);

    // Show/hide cursor.
    void set_cursor_visible(bool visible, int size_percent = 25);

    // Enable raw console input mode (disables line buffering, echo).
    void enable_raw_input();

    // Restore the original console input mode.
    void restore_input_mode();

    // Read a single virtual key code.
    // timeout_ms == 0 means blocking; > 0 means wait up to that many ms.
    // Checks GT_UI_KEYS injection queue first.
    uint16_t read_key(uint32_t timeout_ms = 0);

    // Refresh cached dimensions (call after resize).
    void refresh_size();

    // Returns true if running in test mode (stdio backend).
    bool is_test_mode() const { return test_mode_; }

    // Get the test backend (only valid in test mode).
    TestBackend* test_backend() const { return test_backend_; }

    // Save the current visible console viewport (chars + attributes + position).
    // Call before clear_screen() to preserve what was on screen.
    void save_screen();

    // Restore the previously saved console viewport (chars + attributes + position).
    // Call before destructor to leave the console as we found it.
    void restore_screen();

    // Returns true if a screen snapshot was saved.
    bool has_saved_screen() const { return screen_saved_; }

private:
    PlatformHandle out_;
    PlatformHandle in_;
    unsigned long orig_input_mode_;
    int width_;
    int height_;
    bool valid_;
    bool input_mode_saved_;
    bool test_mode_;
    TestBackend* test_backend_;

    // Saved screen state (Windows only)
    bool screen_saved_ = false;
#ifdef _WIN32
    SMALL_RECT saved_viewport_{};
    std::vector<CHAR_INFO> saved_buffer_;
    COORD saved_buffer_size_{};
#endif
};

// ---------------------------------------------------------------------------
// KeyInjector — environment-variable based keystroke injection for testing
// ---------------------------------------------------------------------------

class KeyInjector {
public:
    static KeyInjector& instance();

    // Parse GT_UI_KEYS and GT_UI_KEY_TIMEOUT_MS from environment.
    void parse_env();

    // Returns true if there are injected keys remaining.
    bool has_injected_key() const;

    // Get the next injected key, or 0 if queue is empty.
    uint16_t next_injected_key();

    // Get timeout for injected keys in ms (0 = blocking).
    uint32_t timeout_ms() const { return timeout_ms_; }

    // Reset the queue (for tests that need to inject multiple sequences).
    void reset();

    // Directly inject a comma-separated key string (for tests).
    void inject_keys(std::string_view keys);

    // Return whether any keys were ever injected (for assertion).
    bool keys_were_injected() const { return keys_were_injected_; }

private:
    KeyInjector() = default;
    std::vector<uint16_t> queue_;
    size_t index_ = 0;
    uint32_t timeout_ms_ = 0;
    bool keys_were_injected_ = false;
};

// ---------------------------------------------------------------------------
// Utility: map ASCII char to VK code
// ---------------------------------------------------------------------------

inline uint16_t char_to_vk(char c) {
    return static_cast<uint16_t>(static_cast<unsigned char>(c));
}

// ---------------------------------------------------------------------------
// Utility: parse a key token string to VK code
// ---------------------------------------------------------------------------

uint16_t parse_key_token(std::string_view token);

} // namespace gittui
