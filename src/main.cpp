#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pf16 {

constexpr int kLogicalW = 640;
constexpr int kLogicalH = 480;
constexpr int kDefaultWindowW = 1280;
constexpr int kDefaultWindowH = 960;
constexpr float kPi = 3.14159265358979323846f;
constexpr uint32_t kInvalidId = 0;

static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }
static int clampi(int v, int lo, int hi) { return std::max(lo, std::min(v, hi)); }
static float radians(float degrees) { return degrees * kPi / 180.0f; }
static bool nearlyZero(float v) { return std::fabs(v) < 0.00001f; }

static std::string formatFloat(float v) {
    if (std::fabs(v) < 0.0005f) v = 0.0f;
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%.2f", v);
    std::string out = buf;
    while (out.size() > 1 && out.back() == '0') out.pop_back();
    if (!out.empty() && out.back() == '.') out.pop_back();
    if (out == "-0") out = "0";
    return out;
}

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
static Vec3 &operator+=(Vec3 &a, Vec3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

static float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static float length(Vec3 v) { return std::sqrt(dot(v, v)); }
static Vec3 normalize(Vec3 v) {
    float len = length(v);
    if (len <= 0.00001f) {
        return {0, 1, 0};
    }
    return v / len;
}

static Vec3 rotateX(Vec3 p, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
}

static Vec3 rotateY(Vec3 p, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
}

static Vec3 rotateZ(Vec3 p, float a) {
    float c = std::cos(a), s = std::sin(a);
    return {p.x * c - p.y * s, p.x * s + p.y * c, p.z};
}

struct Transform {
    Vec3 position {0, 0, 0};
    Vec3 rotation {0, 0, 0};
    Vec3 scale {1, 1, 1};
};

static Vec3 transformPoint(Vec3 p, const Transform &t) {
    p = {p.x * t.scale.x, p.y * t.scale.y, p.z * t.scale.z};
    p = rotateX(p, t.rotation.x);
    p = rotateY(p, t.rotation.y);
    p = rotateZ(p, t.rotation.z);
    return p + t.position;
}

static Vec3 inverseTransformVector(Vec3 v, const Transform &t) {
    v = rotateZ(v, -t.rotation.z);
    v = rotateY(v, -t.rotation.y);
    v = rotateX(v, -t.rotation.x);
    if (!nearlyZero(t.scale.x)) v.x /= t.scale.x;
    if (!nearlyZero(t.scale.y)) v.y /= t.scale.y;
    if (!nearlyZero(t.scale.z)) v.z /= t.scale.z;
    return v;
}

struct Rgb {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

static const std::array<Rgb, 16> kPalette {{
    {0x00, 0x00, 0x00}, // black
    {0x00, 0x00, 0xaa}, // blue
    {0x00, 0xaa, 0x00}, // green
    {0x00, 0xaa, 0xaa}, // cyan
    {0xaa, 0x00, 0x00}, // red
    {0xaa, 0x00, 0xaa}, // magenta
    {0xaa, 0x55, 0x00}, // brown
    {0xaa, 0xaa, 0xaa}, // light gray
    {0x55, 0x55, 0x55}, // dark gray
    {0x55, 0x55, 0xff}, // light blue
    {0x55, 0xff, 0x55}, // light green
    {0x55, 0xff, 0xff}, // light cyan
    {0xff, 0x55, 0x55}, // light red
    {0xff, 0x55, 0xff}, // light magenta
    {0xff, 0xff, 0x55}, // yellow
    {0xff, 0xff, 0xff}, // white
}};

static const std::array<const char *, 16> kPaletteNames {{
    "BLACK", "BLUE", "GREEN", "CYAN", "RED", "MAGENTA", "BROWN", "LIGHT GRAY",
    "DARK GRAY", "LIGHT BLUE", "LIGHT GREEN", "LIGHT CYAN", "LIGHT RED", "LIGHT MAGENTA",
    "YELLOW", "WHITE"
}};

enum ColorIndex : uint8_t {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    BROWN = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    LIGHT_MAGENTA = 13,
    YELLOW = 14,
    WHITE = 15
};

struct UITheme {
    uint8_t background = BLACK;
    uint8_t viewportBackground = BLACK;
    uint8_t panel = BLUE;
    uint8_t panelActive = LIGHT_BLUE;
    uint8_t border = BLUE;
    uint8_t borderActive = LIGHT_CYAN;
    uint8_t text = LIGHT_GRAY;
    uint8_t textBright = WHITE;
    uint8_t hover = LIGHT_CYAN;
    uint8_t selection = YELLOW;
    uint8_t positive = LIGHT_GREEN;
    uint8_t warning = LIGHT_RED;
    uint8_t destructive = RED;
    uint8_t disabled = DARK_GRAY;
    uint8_t wire = LIGHT_GRAY;
    uint8_t gridMinor = DARK_GRAY;
    uint8_t gridMajor = BLUE;
    uint8_t origin = LIGHT_CYAN;
};

static const UITheme kTheme {};

static uint32_t packColor(uint8_t index) {
    const Rgb c = kPalette[index & 15];
    return (uint32_t(c.r) << 16) | (uint32_t(c.g) << 8) | uint32_t(c.b);
}

static uint8_t nearestPalette(float r, float g, float b) {
    float best = 1.0e30f;
    uint8_t bestIndex = 0;
    for (uint8_t i = 0; i < 16; ++i) {
        float dr = r - kPalette[i].r;
        float dg = g - kPalette[i].g;
        float db = b - kPalette[i].b;
        float d = dr * dr + dg * dg + db * db;
        if (d < best) {
            best = d;
            bestIndex = i;
        }
    }
    return bestIndex;
}

static uint8_t shadePalette(uint8_t base, float light) {
    light = clampf(light, 0.20f, 1.20f);
    const Rgb c = kPalette[base & 15];
    return nearestPalette(c.r * light, c.g * light, c.b * light);
}

struct IRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

struct FrameBuffer {
    int w = kLogicalW;
    int h = kLogicalH;
    std::vector<uint32_t> pixels;
    std::vector<float> zbuffer;
    BITMAPINFO bmi {};

    FrameBuffer() {
        pixels.resize(size_t(w) * size_t(h), packColor(0));
        zbuffer.resize(size_t(w) * size_t(h), 1.0e30f);
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
    }

    void clear(uint8_t color) {
        std::fill(pixels.begin(), pixels.end(), packColor(color));
        std::fill(zbuffer.begin(), zbuffer.end(), 1.0e30f);
    }

    void clearZ(const IRect &clip) {
        int x0 = clampi(clip.x, 0, w);
        int y0 = clampi(clip.y, 0, h);
        int x1 = clampi(clip.x + clip.w, 0, w);
        int y1 = clampi(clip.y + clip.h, 0, h);
        for (int y = y0; y < y1; ++y) {
            std::fill(zbuffer.begin() + size_t(y) * w + x0, zbuffer.begin() + size_t(y) * w + x1, 1.0e30f);
        }
    }

    void pixel(int x, int y, uint8_t color) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        pixels[size_t(y) * w + x] = packColor(color);
    }

    void pixelClip(int x, int y, uint8_t color, const IRect &clip) {
        if (!clip.contains(x, y)) return;
        pixel(x, y, color);
    }

    void pixelZ(int x, int y, float z, uint8_t color, const IRect &clip) {
        if (!clip.contains(x, y) || x < 0 || y < 0 || x >= w || y >= h) return;
        size_t idx = size_t(y) * w + x;
        if (z < zbuffer[idx]) {
            zbuffer[idx] = z;
            pixels[idx] = packColor(color);
        }
    }

    void fillRect(IRect r, uint8_t color) {
        int x0 = clampi(r.x, 0, w);
        int y0 = clampi(r.y, 0, h);
        int x1 = clampi(r.x + r.w, 0, w);
        int y1 = clampi(r.y + r.h, 0, h);
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                pixel(x, y, color);
            }
        }
    }

    void rect(IRect r, uint8_t color) {
        for (int x = r.x; x < r.x + r.w; ++x) {
            pixel(x, r.y, color);
            pixel(x, r.y + r.h - 1, color);
        }
        for (int y = r.y; y < r.y + r.h; ++y) {
            pixel(r.x, y, color);
            pixel(r.x + r.w - 1, y, color);
        }
    }

    void line(int x0, int y0, int x1, int y1, uint8_t color, const IRect *clip = nullptr) {
        int dx = std::abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        while (true) {
            if (!clip || clip->contains(x0, y0)) pixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    void lineZ(int x0, int y0, float z0, int x1, int y1, float z1, uint8_t color, const IRect &clip) {
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int steps = std::max(dx, dy);
        if (steps == 0) {
            pixelZ(x0, y0, z0, color, clip);
            return;
        }
        for (int i = 0; i <= steps; ++i) {
            float t = float(i) / float(steps);
            int x = int(std::round(x0 + (x1 - x0) * t));
            int y = int(std::round(y0 + (y1 - y0) * t));
            float z = z0 + (z1 - z0) * t;
            pixelZ(x, y, z, color, clip);
        }
    }
};

enum class Locale { English, Korean, Japanese };

static const char *localeId(Locale locale) {
    switch (locale) {
    case Locale::English: return "en";
    case Locale::Korean: return "ko";
    case Locale::Japanese: return "ja";
    }
    return "en";
}

static Locale localeFromId(const std::string &id) {
    if (id == "ko" || id == "KR" || id == "korean") return Locale::Korean;
    if (id == "ja" || id == "JP" || id == "japanese") return Locale::Japanese;
    return Locale::English;
}

static Locale systemLocale() {
    wchar_t name[LOCALE_NAME_MAX_LENGTH] {};
    if (GetUserDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH) > 0) {
        if ((name[0] == L'k' || name[0] == L'K') && (name[1] == L'o' || name[1] == L'O')) return Locale::Korean;
        if ((name[0] == L'j' || name[0] == L'J') && (name[1] == L'a' || name[1] == L'A')) return Locale::Japanese;
    }
    return Locale::English;
}

struct TranslationEntry {
    const char *key;
    const char *en;
    const char *ko;
    const char *ja;
};

static const TranslationEntry kTranslations[] = {
    {"menu.file", "FILE", "파일", "ファイル"},
    {"menu.edit", "EDIT", "편집", "編集"},
    {"menu.add", "ADD", "추가", "追加"},
    {"menu.view", "VIEW", "보기", "表示"},
    {"menu.lang", "LANG", "언어", "言語"},
    {"menu.help", "HELP", "도움말", "ヘルプ"},
    {"view.top", "TOP", "상단", "上面"},
    {"view.perspective", "PERSPECTIVE", "원근", "透視"},
    {"view.front", "FRONT", "전면", "前面"},
    {"view.side", "SIDE", "측면", "側面"},
    {"mode.wireframe", "WIREFRAME", "와이어", "ワイヤ"},
    {"mode.flat", "FLAT", "플랫", "フラット"},
    {"mode.flat_wire", "FLAT+WIRE", "플랫+선", "面+線"},
    {"label.snap", "SNAP", "스냅", "スナップ"},
    {"label.grid", "GRID", "격자", "グリッド"},
    {"label.color", "COLOR", "색", "色"},
    {"label.vertex_short", "V", "점", "点"},
    {"label.face_short", "F", "면", "面"},
    {"label.palette", "16 COLOR", "16색", "16色"},
    {"hint.main", "LMB SELECT/DRAG  SPACE VIEW  Z MOVE Z  E EXTRUDE  I INSET  M MODE", "좌클릭 선택/드래그  SPACE 보기  Z 깊이  E 돌출  I 삽입  M 모드", "左クリック 選択/ドラッグ  SPACE 表示  Z 奥行  E 押出  I インセット"},
    {"hint.split", "SPACE: SPLIT", "SPACE: 분할", "SPACE: 分割"},
    {"cmd.new", "NEW", "새 파일", "新規"},
    {"cmd.open", "OPEN", "열기", "開く"},
    {"cmd.save", "SAVE", "저장", "保存"},
    {"cmd.save_as", "SAVE AS", "다른 저장", "別名保存"},
    {"cmd.import_obj", "IMPORT OBJ", "OBJ 가져오기", "OBJ読込"},
    {"cmd.export_obj", "EXPORT OBJ", "OBJ 내보내기", "OBJ書出"},
    {"cmd.export_image", "EXPORT IMAGE", "이미지 저장", "画像保存"},
    {"cmd.exit", "EXIT", "종료", "終了"},
    {"cmd.undo", "UNDO", "실행 취소", "元に戻す"},
    {"cmd.redo", "REDO", "다시 실행", "やり直す"},
    {"cmd.copy", "COPY", "복사", "コピー"},
    {"cmd.paste", "PASTE", "붙여넣기", "貼付"},
    {"cmd.duplicate", "DUPLICATE", "복제", "複製"},
    {"cmd.delete", "DELETE", "삭제", "削除"},
    {"cmd.vertex", "VERTEX", "정점", "頂点"},
    {"cmd.plane", "PLANE", "평면", "平面"},
    {"cmd.cube", "CUBE", "큐브", "立方体"},
    {"cmd.pyramid", "PYRAMID", "피라미드", "角錐"},
    {"cmd.prism", "PRISM", "프리즘", "柱体"},
    {"cmd.cylinder", "CYLINDER", "원기둥", "円柱"},
    {"cmd.sphere", "SPHERE", "구", "球"},
    {"cmd.grid", "GRID", "격자", "グリッド"},
    {"cmd.snap", "SNAP", "스냅", "スナップ"},
    {"cmd.wireframe", "WIREFRAME", "와이어", "ワイヤ"},
    {"cmd.flat", "FLAT", "플랫", "フラット"},
    {"cmd.flat_wire", "FLAT+WIRE", "플랫+선", "面+線"},
    {"cmd.light", "LIGHT", "조명", "ライト"},
    {"cmd.frame_all", "FRAME ALL", "전체 보기", "全体表示"},
    {"cmd.reset_views", "RESET VIEWS", "보기 초기화", "表示リセット"},
    {"cmd.screenshot", "F12 SCREENSHOT", "F12 스크린샷", "F12 スクショ"},
    {"cmd.readme", "README", "도움말 파일", "README"},
    {"cmd.move", "MOVE", "이동", "移動"},
    {"cmd.move_object", "MOVE OBJECT", "오브젝트 이동", "オブジェクト移動"},
    {"cmd.pan", "PAN VIEW", "화면 이동", "表示移動"},
    {"cmd.frame_selected", "FRAME SELECTION", "선택 보기", "選択表示"},
    {"cmd.reset_view", "RESET VIEW", "화면 초기화", "表示リセット"},
    {"cmd.reset_all_views", "RESET ALL VIEWS", "모든 화면 초기화", "全表示リセット"},
    {"cmd.move_z", "MOVE Z", "Z 이동", "Z移動"},
    {"cmd.merge", "MERGE", "병합", "結合"},
    {"cmd.snap_to_grid", "SNAP TO GRID", "격자 맞춤", "グリッドへ"},
    {"cmd.extrude", "EXTRUDE", "돌출", "押し出し"},
    {"cmd.inset", "INSET", "삽입", "インセット"},
    {"cmd.flip_face", "FLIP FACE", "면 뒤집기", "面反転"},
    {"cmd.color", "COLOR", "색", "色"},
    {"cmd.lang_en", "ENGLISH", "영어", "英語"},
    {"cmd.lang_ko", "KOREAN", "한국어", "韓国語"},
    {"cmd.lang_ja", "JAPANESE", "일본어", "日本語"},
    {"status.ready", "READY", "준비", "準備"},
    {"status.nothing_undo", "NOTHING TO UNDO", "취소할 작업 없음", "戻す操作なし"},
    {"status.nothing_redo", "NOTHING TO REDO", "다시 실행 없음", "やり直しなし"},
    {"status.undo", "UNDO", "실행 취소", "元に戻す"},
    {"status.redo", "REDO", "다시 실행", "やり直す"},
    {"status.new_scene", "NEW SCENE", "새 장면", "新規シーン"},
    {"status.add_cube", "ADD CUBE", "큐브 추가", "立方体追加"},
    {"status.add_plane", "ADD PLANE", "평면 추가", "平面追加"},
    {"status.add_pyramid", "ADD PYRAMID", "피라미드 추가", "角錐追加"},
    {"status.add_prism", "ADD PRISM", "프리즘 추가", "柱体追加"},
    {"status.add_cylinder", "ADD CYLINDER", "원기둥 추가", "円柱追加"},
    {"status.add_sphere", "ADD SPHERE", "구 추가", "球追加"},
    {"status.add_vertex", "ADD VERTEX", "정점 추가", "頂点追加"},
    {"status.move_vertex", "MOVE VERTEX", "정점 이동", "頂点移動"},
    {"status.move_object", "MOVE OBJECT", "오브젝트 이동", "オブジェクト移動"},
    {"status.move_z", "MOVE Z | DRAG OR TYPE VALUE | ENTER APPLY | ESC CANCEL", "Z 이동 | 드래그 또는 숫자 | ENTER 적용 | ESC 취소", "Z移動 | ドラッグまたは数値 | ENTER適用 | ESC取消"},
    {"status.drag_move", "DRAG TO MOVE", "드래그 이동", "ドラッグ移動"},
    {"status.move_done", "MOVE DONE", "이동 완료", "移動完了"},
    {"status.move_cancel", "MOVE CANCEL", "이동 취소", "移動取消"},
    {"status.box_select", "BOX SELECT", "영역 선택", "範囲選択"},
    {"status.box_select_done", "BOX SELECT DONE", "영역 선택 완료", "範囲選択完了"},
    {"status.select_empty", "SELECT EMPTY", "빈 곳 선택", "空を選択"},
    {"status.select_vertices", "SELECT VERTICES", "정점을 선택", "頂点を選択"},
    {"status.select_or_hover_face", "SELECT OR HOVER FACE", "면 선택 또는 가리키기", "面を選択/ホバー"},
    {"status.select_2_vertices", "SELECT 2+ VERTICES", "정점 2개 이상 선택", "頂点2個以上"},
    {"status.select_3_vertices", "SELECT 3+ VERTICES", "정점 3개 이상 선택", "頂点3個以上"},
    {"status.face_one_object", "FACE NEEDS ONE OBJECT", "한 오브젝트 정점만 가능", "同一オブジェクトのみ"},
    {"status.nothing_selected", "NOTHING SELECTED", "선택 없음", "選択なし"},
    {"status.nothing_duplicate", "NOTHING TO DUPLICATE", "복제할 대상 없음", "複製対象なし"},
    {"status.clipboard_empty", "CLIPBOARD EMPTY", "클립보드 비어 있음", "クリップ空"},
    {"status.copy", "COPY", "복사", "コピー"},
    {"status.paste", "PASTE", "붙여넣기", "貼付"},
    {"status.delete", "DELETE", "삭제", "削除"},
    {"status.merge_center", "MERGE CENTER", "중앙 병합", "中央結合"},
    {"status.snap_to_grid", "SNAP TO GRID", "격자 맞춤", "グリッドへ"},
    {"status.flip_face", "FLIP FACE", "면 뒤집기", "面反転"},
    {"status.extrude", "EXTRUDE 1 UNIT", "1 단위 돌출", "1単位押出"},
    {"status.inset", "INSET", "삽입", "インセット"},
    {"status.focus", "FOCUS", "초점", "フォーカス"},
    {"status.frame_selection", "FRAME SELECTION", "선택 보기", "選択表示"},
    {"status.frame_all", "FRAME ALL", "전체 보기", "全体表示"},
    {"status.reset_view", "RESET VIEW", "화면 초기화", "表示リセット"},
    {"status.reset_all_views", "RESET ALL VIEWS", "모든 화면 초기화", "全表示リセット"},
    {"status.reset_views", "RESET VIEWS", "보기 초기화", "表示リセット"},
    {"status.zoom", "ZOOM", "확대/축소", "ズーム"},
    {"status.pan", "PAN", "이동 보기", "パン"},
    {"status.pan_view", "PAN VIEW", "화면 이동", "表示移動"},
    {"status.orbit", "ORBIT", "회전 보기", "オービット"},
    {"status.orbit_view", "ORBIT VIEW", "화면 회전", "表示回転"},
    {"status.saved", "SAVED", "저장됨", "保存済み"},
    {"status.loaded", "LOADED", "불러옴", "読込済み"},
    {"status.save_cancel", "SAVE CANCEL", "저장 취소", "保存取消"},
    {"status.open_cancel", "OPEN CANCEL", "열기 취소", "読込取消"},
    {"status.export_cancel", "EXPORT CANCEL", "내보내기 취소", "書出取消"},
    {"status.import_cancel", "IMPORT CANCEL", "가져오기 취소", "読込取消"},
    {"status.save_failed", "SAVE FAILED", "저장 실패", "保存失敗"},
    {"status.open_failed", "OPEN FAILED", "열기 실패", "読込失敗"},
    {"status.obj_exported", "OBJ EXPORTED", "OBJ 내보냄", "OBJ書出済み"},
    {"status.obj_export_failed", "OBJ EXPORT FAILED", "OBJ 내보내기 실패", "OBJ書出失敗"},
    {"status.obj_imported", "OBJ IMPORTED", "OBJ 가져옴", "OBJ読込済み"},
    {"status.obj_import_failed", "OBJ IMPORT FAILED", "OBJ 가져오기 실패", "OBJ読込失敗"},
    {"status.grid_on", "GRID ON", "격자 켜짐", "グリッドON"},
    {"status.grid_off", "GRID OFF", "격자 꺼짐", "グリッドOFF"},
    {"status.snap_on", "SNAP ON", "스냅 켜짐", "スナップON"},
    {"status.snap_off", "SNAP OFF", "스냅 꺼짐", "スナップOFF"},
    {"status.language", "LANGUAGE", "언어", "言語"}
};

static std::string tr(Locale locale, const char *key) {
    for (const auto &entry : kTranslations) {
        if (std::string(entry.key) == key) {
            switch (locale) {
            case Locale::Korean: return entry.ko;
            case Locale::Japanese: return entry.ja;
            case Locale::English: return entry.en;
            }
        }
    }
    return key;
}

struct WideGlyphBitmap {
    int w = 12;
    int h = 12;
    std::vector<uint8_t> pixels;
};

static WideGlyphBitmap rasterizeWideGlyph(wchar_t wc) {
    constexpr int gw = 12;
    constexpr int gh = 12;
    WideGlyphBitmap glyph;
    glyph.w = gw;
    glyph.h = gh;
    glyph.pixels.assign(size_t(gw) * gh, 0);

    BITMAPINFO bmi {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = gw;
    bmi.bmiHeader.biHeight = -gh;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;
    HDC dc = CreateCompatibleDC(nullptr);
    HBITMAP bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dc || !bitmap || !bits) {
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
        return glyph;
    }

    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    RECT rc {0, 0, gw, gh};
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &rc, brush);
    DeleteObject(brush);

    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                             FIXED_PITCH | FF_MODERN, L"Malgun Gothic");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutW(dc, 0, -1, &wc, 1);

    const uint32_t *src = static_cast<const uint32_t *>(bits);
    for (int y = 0; y < gh; ++y) {
        for (int x = 0; x < gw; ++x) {
            uint32_t px = src[size_t(y) * gw + x];
            uint8_t r = uint8_t(px & 0xff);
            uint8_t g = uint8_t((px >> 8) & 0xff);
            uint8_t b = uint8_t((px >> 16) & 0xff);
            glyph.pixels[size_t(y) * gw + x] = (int(r) + int(g) + int(b)) > 96 ? 1 : 0;
        }
    }

    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return glyph;
}

static const WideGlyphBitmap &wideGlyphFor(wchar_t wc) {
    static std::map<wchar_t, WideGlyphBitmap> cache;
    auto it = cache.find(wc);
    if (it != cache.end()) return it->second;
    auto inserted = cache.emplace(wc, rasterizeWideGlyph(wc));
    return inserted.first->second;
}

static uint32_t readUtf8Codepoint(const std::string &text, size_t &i) {
    unsigned char c = static_cast<unsigned char>(text[i++]);
    if (c < 0x80) return c;
    if ((c >> 5) == 0x06 && i < text.size()) {
        uint32_t cp = uint32_t(c & 0x1f) << 6;
        cp |= uint32_t(static_cast<unsigned char>(text[i++]) & 0x3f);
        return cp;
    }
    if ((c >> 4) == 0x0e && i + 1 < text.size()) {
        uint32_t cp = uint32_t(c & 0x0f) << 12;
        cp |= uint32_t(static_cast<unsigned char>(text[i++]) & 0x3f) << 6;
        cp |= uint32_t(static_cast<unsigned char>(text[i++]) & 0x3f);
        return cp;
    }
    if ((c >> 3) == 0x1e && i + 2 < text.size()) {
        uint32_t cp = uint32_t(c & 0x07) << 18;
        cp |= uint32_t(static_cast<unsigned char>(text[i++]) & 0x3f) << 12;
        cp |= uint32_t(static_cast<unsigned char>(text[i++]) & 0x3f) << 6;
        cp |= uint32_t(static_cast<unsigned char>(text[i++]) & 0x3f);
        return cp;
    }
    return '?';
}

static std::array<uint8_t, 7> glyphFor(char input) {
    char c = char(std::toupper(static_cast<unsigned char>(input)));
    switch (c) {
    case 'A': return {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11};
    case 'B': return {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e};
    case 'C': return {0x0f,0x10,0x10,0x10,0x10,0x10,0x0f};
    case 'D': return {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e};
    case 'E': return {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f};
    case 'F': return {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10};
    case 'G': return {0x0f,0x10,0x10,0x13,0x11,0x11,0x0f};
    case 'H': return {0x11,0x11,0x11,0x1f,0x11,0x11,0x11};
    case 'I': return {0x1f,0x04,0x04,0x04,0x04,0x04,0x1f};
    case 'J': return {0x1f,0x02,0x02,0x02,0x12,0x12,0x0c};
    case 'K': return {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
    case 'L': return {0x10,0x10,0x10,0x10,0x10,0x10,0x1f};
    case 'M': return {0x11,0x1b,0x15,0x15,0x11,0x11,0x11};
    case 'N': return {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    case 'O': return {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e};
    case 'P': return {0x1e,0x11,0x11,0x1e,0x10,0x10,0x10};
    case 'Q': return {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d};
    case 'R': return {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11};
    case 'S': return {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e};
    case 'T': return {0x1f,0x04,0x04,0x04,0x04,0x04,0x04};
    case 'U': return {0x11,0x11,0x11,0x11,0x11,0x11,0x0e};
    case 'V': return {0x11,0x11,0x11,0x11,0x11,0x0a,0x04};
    case 'W': return {0x11,0x11,0x11,0x15,0x15,0x1b,0x11};
    case 'X': return {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11};
    case 'Y': return {0x11,0x11,0x0a,0x04,0x04,0x04,0x04};
    case 'Z': return {0x1f,0x01,0x02,0x04,0x08,0x10,0x1f};
    case '0': return {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e};
    case '1': return {0x04,0x0c,0x04,0x04,0x04,0x04,0x0e};
    case '2': return {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f};
    case '3': return {0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e};
    case '4': return {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02};
    case '5': return {0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e};
    case '6': return {0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e};
    case '7': return {0x1f,0x01,0x02,0x04,0x08,0x08,0x08};
    case '8': return {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e};
    case '9': return {0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e};
    case ':': return {0x00,0x04,0x04,0x00,0x04,0x04,0x00};
    case '.': return {0x00,0x00,0x00,0x00,0x00,0x0c,0x0c};
    case ',': return {0x00,0x00,0x00,0x00,0x04,0x04,0x08};
    case '-': return {0x00,0x00,0x00,0x1f,0x00,0x00,0x00};
    case '+': return {0x00,0x04,0x04,0x1f,0x04,0x04,0x00};
    case '/': return {0x01,0x02,0x02,0x04,0x08,0x08,0x10};
    case '\\': return {0x10,0x08,0x08,0x04,0x02,0x02,0x01};
    case '|': return {0x04,0x04,0x04,0x04,0x04,0x04,0x04};
    case '[': return {0x0e,0x08,0x08,0x08,0x08,0x08,0x0e};
    case ']': return {0x0e,0x02,0x02,0x02,0x02,0x02,0x0e};
    case '>': return {0x10,0x08,0x04,0x02,0x04,0x08,0x10};
    case '<': return {0x01,0x02,0x04,0x08,0x04,0x02,0x01};
    case '=': return {0x00,0x00,0x1f,0x00,0x1f,0x00,0x00};
    case '_': return {0x00,0x00,0x00,0x00,0x00,0x00,0x1f};
    case '?': return {0x0e,0x11,0x01,0x02,0x04,0x00,0x04};
    case '!': return {0x04,0x04,0x04,0x04,0x04,0x00,0x04};
    case ' ': return {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    default: return {0x1f,0x11,0x05,0x02,0x05,0x11,0x1f};
    }
}

static void drawGlyph(FrameBuffer &fb, int x, int y, char c, uint8_t color, int scale = 1) {
    auto g = glyphFor(c);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if ((g[row] >> (4 - col)) & 1) {
                for (int sy = 0; sy < scale; ++sy) {
                    for (int sx = 0; sx < scale; ++sx) {
                        fb.pixel(x + col * scale + sx, y + row * scale + sy, color);
                    }
                }
            }
        }
    }
}

static void drawWideGlyph(FrameBuffer &fb, int x, int y, wchar_t wc, uint8_t color, int scale = 1) {
    const WideGlyphBitmap &g = wideGlyphFor(wc);
    for (int row = 0; row < g.h; ++row) {
        for (int col = 0; col < g.w; ++col) {
            if (!g.pixels[size_t(row) * g.w + col]) continue;
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    fb.pixel(x + col * scale + sx, y + row * scale + sy, color);
                }
            }
        }
    }
}

static void drawText(FrameBuffer &fb, int x, int y, const std::string &text, uint8_t color, int scale = 1) {
    int cx = x;
    for (size_t i = 0; i < text.size();) {
        uint32_t cp = readUtf8Codepoint(text, i);
        if (cp == '\n') {
            y += 8 * scale;
            cx = x;
            continue;
        }
        if (cp < 0x80) {
            drawGlyph(fb, cx, y, char(cp), color, scale);
            cx += 6 * scale;
        } else if (cp <= 0xffff) {
            drawWideGlyph(fb, cx, y - 2 * scale, wchar_t(cp), color, scale);
            cx += 12 * scale;
        } else {
            drawGlyph(fb, cx, y, '?', color, scale);
            cx += 6 * scale;
        }
    }
}

static int textWidth(const std::string &text, int scale = 1) {
    int width = 0;
    int line = 0;
    for (size_t i = 0; i < text.size();) {
        uint32_t cp = readUtf8Codepoint(text, i);
        if (cp == '\n') {
            width = std::max(width, line);
            line = 0;
            continue;
        }
        line += (cp < 0x80 ? 6 : 12) * scale;
    }
    return std::max(width, line);
}

enum class ViewKind { Top, Perspective, Front, Side };
enum class RenderMode { Wireframe, Flat, FlatWire };
enum class ElementType { None, Vertex, Edge, Face, Object };
enum class DragMode { None, VertexDrag, VertexZMove, ObjectDrag, BoxSelect, Orbit, Pan };

static const char *viewKey(ViewKind k) {
    switch (k) {
    case ViewKind::Top: return "view.top";
    case ViewKind::Perspective: return "view.perspective";
    case ViewKind::Front: return "view.front";
    case ViewKind::Side: return "view.side";
    }
    return "";
}

static const char *renderModeKey(RenderMode mode) {
    switch (mode) {
    case RenderMode::Wireframe: return "mode.wireframe";
    case RenderMode::Flat: return "mode.flat";
    case RenderMode::FlatWire: return "mode.flat_wire";
    }
    return "";
}

static const char *renderModeName(RenderMode mode) {
    switch (mode) {
    case RenderMode::Wireframe: return "WIREFRAME";
    case RenderMode::Flat: return "FLAT";
    case RenderMode::FlatWire: return "FLAT+WIRE";
    }
    return "";
}

struct Vertex {
    uint32_t id = 0;
    Vec3 position;
};

struct Face {
    uint32_t id = 0;
    std::vector<uint32_t> vertices;
    uint8_t color = 9;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};

struct Object3D {
    uint32_t id = 0;
    std::string name;
    Transform transform;
    Mesh mesh;
};

struct Scene {
    uint32_t nextId = 1;
    std::vector<Object3D> objects;
    RenderMode renderMode = RenderMode::FlatWire;
    uint8_t currentColor = 9;
    bool grid = true;
    bool snap = true;
    bool hiddenLine = false;
    bool showBackfaces = true;
    float gridSize = 1.0f;
};

struct ElementRef {
    ElementType type = ElementType::None;
    uint32_t objectId = 0;
    uint32_t id = 0;
};

struct HoverInfo {
    ElementType type = ElementType::None;
    uint32_t objectId = 0;
    uint32_t id = 0;
    uint32_t edgeA = 0;
    uint32_t edgeB = 0;
    float depth = 1.0e30f;
    int sx = 0;
    int sy = 0;
    ViewKind view = ViewKind::Perspective;
};

struct Snapshot {
    Scene scene;
    std::vector<ElementRef> selection;
    uint32_t activeObjectId = 0;
    std::filesystem::path currentFile;
};

struct ViewState {
    ViewKind kind = ViewKind::Perspective;
    Vec3 center {0, 0, 0};
    float zoom = 18.0f;
    float yaw = radians(35.0f);
    float pitch = radians(25.0f);
    float distance = 10.0f;
};

struct ViewportDraw {
    ViewState *state = nullptr;
    IRect rect;
};

struct Projected {
    bool ok = false;
    int x = 0;
    int y = 0;
    float depth = 1.0e30f;
};

struct VertexDragItem {
    uint32_t objectId = 0;
    uint32_t vertexId = 0;
    Vec3 original;
};

struct ObjectDragItem {
    uint32_t objectId = 0;
    Vec3 original;
};

struct DragState {
    DragMode mode = DragMode::None;
    ViewKind view = ViewKind::Perspective;
    int startX = 0;
    int startY = 0;
    int lastX = 0;
    int lastY = 0;
    bool changed = false;
    std::string typedValue;
    Snapshot before;
    std::vector<VertexDragItem> vertices;
    std::vector<ObjectDragItem> objects;
};

struct MenuState {
    bool open = false;
    std::string name;
    IRect rect;
    std::vector<std::string> items;
};

struct ClipboardScene {
    bool hasData = false;
    std::vector<Object3D> objects;
};

static bool sameRef(const ElementRef &a, const ElementRef &b) {
    return a.type == b.type && a.objectId == b.objectId && a.id == b.id;
}

static Vertex *findVertex(Object3D &obj, uint32_t id) {
    for (auto &v : obj.mesh.vertices) {
        if (v.id == id) return &v;
    }
    return nullptr;
}

static const Vertex *findVertex(const Object3D &obj, uint32_t id) {
    for (const auto &v : obj.mesh.vertices) {
        if (v.id == id) return &v;
    }
    return nullptr;
}

static Face *findFace(Object3D &obj, uint32_t id) {
    for (auto &f : obj.mesh.faces) {
        if (f.id == id) return &f;
    }
    return nullptr;
}

static const Face *findFace(const Object3D &obj, uint32_t id) {
    for (const auto &f : obj.mesh.faces) {
        if (f.id == id) return &f;
    }
    return nullptr;
}

static Object3D *findObject(Scene &scene, uint32_t id) {
    for (auto &obj : scene.objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

static const Object3D *findObject(const Scene &scene, uint32_t id) {
    for (const auto &obj : scene.objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

static Vec3 faceCenterLocal(const Object3D &obj, const Face &face) {
    Vec3 c {0, 0, 0};
    int n = 0;
    for (uint32_t id : face.vertices) {
        if (const Vertex *v = findVertex(obj, id)) {
            c += v->position;
            ++n;
        }
    }
    if (n == 0) return c;
    return c / float(n);
}

static Vec3 faceNormalLocal(const Object3D &obj, const Face &face) {
    if (face.vertices.size() < 3) return {0, 1, 0};
    const Vertex *a = findVertex(obj, face.vertices[0]);
    const Vertex *b = findVertex(obj, face.vertices[1]);
    const Vertex *c = findVertex(obj, face.vertices[2]);
    if (!a || !b || !c) return {0, 1, 0};
    return normalize(cross(b->position - a->position, c->position - a->position));
}

static Vec3 faceNormalWorld(const Object3D &obj, const Face &face) {
    if (face.vertices.size() < 3) return {0, 1, 0};
    const Vertex *a = findVertex(obj, face.vertices[0]);
    const Vertex *b = findVertex(obj, face.vertices[1]);
    const Vertex *c = findVertex(obj, face.vertices[2]);
    if (!a || !b || !c) return {0, 1, 0};
    Vec3 aw = transformPoint(a->position, obj.transform);
    Vec3 bw = transformPoint(b->position, obj.transform);
    Vec3 cw = transformPoint(c->position, obj.transform);
    return normalize(cross(bw - aw, cw - aw));
}

static std::filesystem::path executableDir() {
    wchar_t path[MAX_PATH] {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

static std::filesystem::path ensureDir(const std::filesystem::path &dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

class App {
public:
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;
    FrameBuffer fb;
    Scene scene;
    std::vector<ElementRef> selection;
    uint32_t activeObjectId = 0;
    HoverInfo hover;
    std::array<ViewState, 4> views;
    std::optional<ViewKind> maximizedView;
    ViewKind activeView = ViewKind::Perspective;
    DragState drag;
    MenuState menu;
    ClipboardScene clipboard;
    std::vector<Snapshot> undoStack;
    std::vector<Snapshot> redoStack;
    std::filesystem::path currentFile;
    Locale language = Locale::English;
    std::string status = "READY";
    int clientW = kDefaultWindowW;
    int clientH = kDefaultWindowH;
    int scale = 2;
    int offsetX = 0;
    int offsetY = 0;
    int mouseX = 0;
    int mouseY = 0;
    bool mouseCaptured = false;

    App() {
        loadSettings();
        views[0].kind = ViewKind::Top;
        views[1].kind = ViewKind::Perspective;
        views[2].kind = ViewKind::Front;
        views[3].kind = ViewKind::Side;
        views[0].center = {0, 0, 0};
        views[2].center = {0, 0, 0};
        views[3].center = {0, 0, 0};
        views[1].center = {0, 0, 0};
        newScene(false);
    }

    std::filesystem::path settingsPath() const {
        return executableDir() / "settings.ini";
    }

    void loadSettings() {
        language = systemLocale();
        std::ifstream in(settingsPath());
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            const std::string prefix = "language=";
            if (line.rfind(prefix, 0) == 0) {
                language = localeFromId(line.substr(prefix.size()));
            }
        }
    }

    void saveSettings() const {
        std::ofstream out(settingsPath(), std::ios::binary);
        if (!out) return;
        out << "language=" << localeId(language) << "\n";
    }

    std::string text(const char *key) const {
        return tr(language, key);
    }

    const char *menuKey(const std::string &id) const {
        if (id == "FILE") return "menu.file";
        if (id == "EDIT") return "menu.edit";
        if (id == "ADD") return "menu.add";
        if (id == "VIEW") return "menu.view";
        if (id == "LANG") return "menu.lang";
        if (id == "HELP") return "menu.help";
        return "";
    }

    const char *commandKey(const std::string &id) const {
        if (id == "NEW") return "cmd.new";
        if (id == "OPEN") return "cmd.open";
        if (id == "SAVE") return "cmd.save";
        if (id == "SAVE AS") return "cmd.save_as";
        if (id == "IMPORT OBJ") return "cmd.import_obj";
        if (id == "EXPORT OBJ") return "cmd.export_obj";
        if (id == "EXPORT IMAGE") return "cmd.export_image";
        if (id == "EXIT") return "cmd.exit";
        if (id == "UNDO") return "cmd.undo";
        if (id == "REDO") return "cmd.redo";
        if (id == "COPY") return "cmd.copy";
        if (id == "PASTE") return "cmd.paste";
        if (id == "DUPLICATE") return "cmd.duplicate";
        if (id == "DELETE") return "cmd.delete";
        if (id == "VERTEX") return "cmd.vertex";
        if (id == "PLANE") return "cmd.plane";
        if (id == "CUBE") return "cmd.cube";
        if (id == "PYRAMID") return "cmd.pyramid";
        if (id == "PRISM") return "cmd.prism";
        if (id == "CYLINDER") return "cmd.cylinder";
        if (id == "SPHERE") return "cmd.sphere";
        if (id == "GRID") return "cmd.grid";
        if (id == "TOGGLE SNAP") return "cmd.snap";
        if (id == "WIREFRAME") return "cmd.wireframe";
        if (id == "FLAT") return "cmd.flat";
        if (id == "FLAT+WIRE") return "cmd.flat_wire";
        if (id == "LIGHT") return "cmd.light";
        if (id == "FRAME ALL") return "cmd.frame_all";
        if (id == "RESET VIEWS") return "cmd.reset_views";
        if (id == "F12 SCREENSHOT") return "cmd.screenshot";
        if (id == "README") return "cmd.readme";
        if (id == "MOVE") return "cmd.move";
        if (id == "PAN") return "cmd.pan";
        if (id == "FRAME SELECTED") return "cmd.frame_selected";
        if (id == "RESET VIEW") return "cmd.reset_view";
        if (id == "RESET ALL VIEWS") return "cmd.reset_all_views";
        if (id == "MOVE Z") return "cmd.move_z";
        if (id == "MOVE OBJECT") return "cmd.move_object";
        if (id == "MERGE") return "cmd.merge";
        if (id == "SNAP TO GRID") return "cmd.snap_to_grid";
        if (id == "EXTRUDE") return "cmd.extrude";
        if (id == "INSET") return "cmd.inset";
        if (id == "FLIP FACE") return "cmd.flip_face";
        if (id == "COLOR") return "cmd.color";
        if (id == "LANG EN") return "cmd.lang_en";
        if (id == "LANG KO") return "cmd.lang_ko";
        if (id == "LANG JA") return "cmd.lang_ja";
        return "";
    }

    std::string menuLabel(const std::string &id) const {
        const char *key = menuKey(id);
        return key[0] ? text(key) : id;
    }

    std::string itemLabel(const std::string &id) const {
        const char *key = commandKey(id);
        std::string label = key[0] ? text(key) : id;
        if ((id == "LANG EN" && language == Locale::English) ||
            (id == "LANG KO" && language == Locale::Korean) ||
            (id == "LANG JA" && language == Locale::Japanese)) {
            label = "[X] " + label;
        }
        return label;
    }

    std::string localizedStatus() const {
        static const std::pair<const char *, const char *> statusKeys[] = {
            { "READY", "status.ready" },
            { "NOTHING TO UNDO", "status.nothing_undo" },
            { "NOTHING TO REDO", "status.nothing_redo" },
            { "UNDO", "status.undo" },
            { "REDO", "status.redo" },
            { "NEW SCENE", "status.new_scene" },
            { "ADD CUBE", "status.add_cube" },
            { "ADD PLANE", "status.add_plane" },
            { "ADD PYRAMID", "status.add_pyramid" },
            { "ADD PRISM", "status.add_prism" },
            { "ADD CYLINDER", "status.add_cylinder" },
            { "ADD SPHERE", "status.add_sphere" },
            { "ADD VERTEX", "status.add_vertex" },
            { "MOVE VERTEX", "status.move_vertex" },
            { "MOVE OBJECT", "status.move_object" },
            { "MOVE Z | DRAG OR TYPE VALUE | ENTER APPLY | ESC CANCEL", "status.move_z" },
            { "DRAG TO MOVE", "status.drag_move" },
            { "MOVE DONE", "status.move_done" },
            { "MOVE CANCEL", "status.move_cancel" },
            { "BOX SELECT", "status.box_select" },
            { "BOX SELECT DONE", "status.box_select_done" },
            { "SELECT EMPTY", "status.select_empty" },
            { "SELECT VERTICES", "status.select_vertices" },
            { "SELECT OR HOVER FACE", "status.select_or_hover_face" },
            { "SELECT 2+ VERTICES", "status.select_2_vertices" },
            { "SELECT 3+ VERTICES", "status.select_3_vertices" },
            { "FACE NEEDS ONE OBJECT", "status.face_one_object" },
            { "NOTHING SELECTED", "status.nothing_selected" },
            { "NOTHING TO DUPLICATE", "status.nothing_duplicate" },
            { "CLIPBOARD EMPTY", "status.clipboard_empty" },
            { "COPY", "status.copy" },
            { "PASTE", "status.paste" },
            { "DELETE", "status.delete" },
            { "MERGE CENTER", "status.merge_center" },
            { "SNAP TO GRID", "status.snap_to_grid" },
            { "FLIP FACE", "status.flip_face" },
            { "EXTRUDE 1 UNIT", "status.extrude" },
            { "INSET", "status.inset" },
            { "FOCUS", "status.focus" },
            { "FRAME SELECTION", "status.frame_selection" },
            { "FRAME ALL", "status.frame_all" },
            { "RESET VIEW", "status.reset_view" },
            { "RESET ALL VIEWS", "status.reset_all_views" },
            { "RESET VIEWS", "status.reset_views" },
            { "ZOOM", "status.zoom" },
            { "PAN", "status.pan" },
            { "PAN VIEW", "status.pan_view" },
            { "ORBIT", "status.orbit" },
            { "ORBIT VIEW", "status.orbit_view" },
            { "SAVED", "status.saved" },
            { "LOADED", "status.loaded" },
            { "SAVE CANCEL", "status.save_cancel" },
            { "OPEN CANCEL", "status.open_cancel" },
            { "EXPORT CANCEL", "status.export_cancel" },
            { "IMPORT CANCEL", "status.import_cancel" },
            { "SAVE FAILED", "status.save_failed" },
            { "OPEN FAILED", "status.open_failed" },
            { "OBJ EXPORTED", "status.obj_exported" },
            { "OBJ EXPORT FAILED", "status.obj_export_failed" },
            { "OBJ IMPORTED", "status.obj_imported" },
            { "OBJ IMPORT FAILED", "status.obj_import_failed" },
            { "GRID ON", "status.grid_on" },
            { "GRID OFF", "status.grid_off" },
            { "SNAP ON", "status.snap_on" },
            { "SNAP OFF", "status.snap_off" },
            { "WIREFRAME", "mode.wireframe" },
            { "FLAT", "mode.flat" },
            { "FLAT+WIRE", "mode.flat_wire" },
            { "4 VIEWPORTS", "menu.view" },
            { "MAXIMIZE VIEW", "menu.view" },
            { "ADD MENU", "menu.add" },
            { "CLICK PALETTE COLOR", "cmd.color" },
            { "DRAG VERTEX DIRECTLY", "cmd.move" },
            { "ESC", "ESC" },
            { "CANCEL", "status.move_cancel" }
        };
        for (const auto &[raw, key] : statusKeys) {
            if (status == raw) return text(key);
        }
        if (status.rfind("LANGUAGE ", 0) == 0) return text("status.language") + " " + status.substr(9);
        if (status.rfind("COLOR ", 0) == 0) return text("label.color") + status.substr(5);
        if (status.rfind("MOVE OBJECT |", 0) == 0) return text("status.move_object") + status.substr(11);
        return status;
    }

    void setLanguage(Locale locale) {
        language = locale;
        saveSettings();
        status = std::string("LANGUAGE ") + localeId(language);
    }

    uint32_t allocId() { return scene.nextId++; }

    Snapshot makeSnapshot() const {
        return {scene, selection, activeObjectId, currentFile};
    }

    void restoreSnapshot(const Snapshot &s) {
        scene = s.scene;
        selection = s.selection;
        activeObjectId = s.activeObjectId;
        currentFile = s.currentFile;
        sanitizeActiveObject();
    }

    void pushUndo(const Snapshot &before) {
        undoStack.push_back(before);
        if (undoStack.size() > 256) {
            undoStack.erase(undoStack.begin());
        }
        redoStack.clear();
    }

    void undo() {
        if (undoStack.empty()) {
            status = "NOTHING TO UNDO";
            return;
        }
        redoStack.push_back(makeSnapshot());
        restoreSnapshot(undoStack.back());
        undoStack.pop_back();
        status = "UNDO";
    }

    void redo() {
        if (redoStack.empty()) {
            status = "NOTHING TO REDO";
            return;
        }
        undoStack.push_back(makeSnapshot());
        restoreSnapshot(redoStack.back());
        redoStack.pop_back();
        status = "REDO";
    }

    void clearSelection() {
        selection.clear();
    }

    bool objectExists(uint32_t objectId) const {
        return findObject(scene, objectId) != nullptr;
    }

    void setActiveObject(uint32_t objectId) {
        activeObjectId = objectExists(objectId) ? objectId : 0;
    }

    void sanitizeActiveObject() {
        if (activeObjectId != 0 && objectExists(activeObjectId)) return;
        activeObjectId = 0;
        for (const ElementRef &r : selection) {
            if (objectExists(r.objectId)) {
                activeObjectId = r.objectId;
                return;
            }
        }
    }

    void captureMouse() {
        if (!hwnd) return;
        SetCapture(hwnd);
        mouseCaptured = true;
    }

    void releaseMouseCapture() {
        if (!mouseCaptured) return;
        mouseCaptured = false;
        if (hwnd && GetCapture() == hwnd) ReleaseCapture();
    }

    bool isSelected(ElementType type, uint32_t objectId, uint32_t id) const {
        ElementRef r {type, objectId, id};
        return std::any_of(selection.begin(), selection.end(), [&](const ElementRef &s) { return sameRef(s, r); });
    }

    void addSelection(ElementRef r) {
        if (r.type == ElementType::None) return;
        setActiveObject(r.objectId);
        if (!isSelected(r.type, r.objectId, r.id)) selection.push_back(r);
    }

    void toggleSelection(ElementRef r) {
        auto it = std::find_if(selection.begin(), selection.end(), [&](const ElementRef &s) { return sameRef(s, r); });
        if (it == selection.end()) {
            setActiveObject(r.objectId);
            selection.push_back(r);
        } else {
            selection.erase(it);
            sanitizeActiveObject();
        }
    }

    std::vector<ElementRef> selectedVertices() const {
        std::vector<ElementRef> out;
        for (const auto &s : selection) {
            if (s.type == ElementType::Vertex) out.push_back(s);
        }
        return out;
    }

    std::vector<ElementRef> selectedFaces() const {
        std::vector<ElementRef> out;
        for (const auto &s : selection) {
            if (s.type == ElementType::Face) out.push_back(s);
        }
        return out;
    }

    std::vector<ElementRef> selectedObjects() const {
        std::vector<ElementRef> out;
        for (const auto &s : selection) {
            if (s.type == ElementType::Object) out.push_back(s);
        }
        return out;
    }

    void newScene(bool withUndo = true) {
        Snapshot before = makeSnapshot();
        scene = Scene {};
        selection.clear();
        activeObjectId = 0;
        currentFile.clear();
        addCube(false);
        resetViews();
        status = "NEW SCENE";
        if (withUndo) pushUndo(before);
    }

    Object3D &addObject(const std::string &name) {
        Object3D obj;
        obj.id = allocId();
        obj.name = name;
        scene.objects.push_back(obj);
        return scene.objects.back();
    }

    uint32_t addVertex(Object3D &obj, Vec3 p) {
        Vertex v;
        v.id = allocId();
        v.position = p;
        obj.mesh.vertices.push_back(v);
        return v.id;
    }

    uint32_t addFace(Object3D &obj, std::vector<uint32_t> verts, uint8_t color) {
        Face f;
        f.id = allocId();
        f.vertices = std::move(verts);
        f.color = color & 15;
        obj.mesh.faces.push_back(f);
        return f.id;
    }

    void addCube(bool withUndo = true) {
        Snapshot before = makeSnapshot();
        Object3D &obj = addObject("Cube");
        const float s = 1.0f;
        uint32_t v0 = addVertex(obj, {-s, -s, -s});
        uint32_t v1 = addVertex(obj, { s, -s, -s});
        uint32_t v2 = addVertex(obj, { s,  s, -s});
        uint32_t v3 = addVertex(obj, {-s,  s, -s});
        uint32_t v4 = addVertex(obj, {-s, -s,  s});
        uint32_t v5 = addVertex(obj, { s, -s,  s});
        uint32_t v6 = addVertex(obj, { s,  s,  s});
        uint32_t v7 = addVertex(obj, {-s,  s,  s});
        addFace(obj, {v0, v3, v2, v1}, 9);
        addFace(obj, {v4, v5, v6, v7}, 10);
        addFace(obj, {v0, v4, v7, v3}, 11);
        addFace(obj, {v1, v2, v6, v5}, 12);
        addFace(obj, {v3, v7, v6, v2}, 14);
        addFace(obj, {v0, v1, v5, v4}, 6);
        clearSelection();
        addSelection({ElementType::Object, obj.id, obj.id});
        status = "ADD CUBE";
        if (withUndo) pushUndo(before);
    }

    void addPlane(bool withUndo = true) {
        Snapshot before = makeSnapshot();
        Object3D &obj = addObject("Plane");
        uint32_t a = addVertex(obj, {-1, 0, -1});
        uint32_t b = addVertex(obj, { 1, 0, -1});
        uint32_t c = addVertex(obj, { 1, 0,  1});
        uint32_t d = addVertex(obj, {-1, 0,  1});
        uint32_t f = addFace(obj, {a, b, c, d}, scene.currentColor);
        clearSelection();
        addSelection({ElementType::Face, obj.id, f});
        status = "ADD PLANE";
        if (withUndo) pushUndo(before);
    }

    void addPyramid(bool withUndo = true) {
        Snapshot before = makeSnapshot();
        Object3D &obj = addObject("Pyramid");
        uint32_t a = addVertex(obj, {-1, 0, -1});
        uint32_t b = addVertex(obj, { 1, 0, -1});
        uint32_t c = addVertex(obj, { 1, 0,  1});
        uint32_t d = addVertex(obj, {-1, 0,  1});
        uint32_t e = addVertex(obj, { 0, 1.8f, 0});
        addFace(obj, {a, b, c, d}, 6);
        addFace(obj, {a, e, b}, 9);
        addFace(obj, {b, e, c}, 10);
        addFace(obj, {c, e, d}, 12);
        addFace(obj, {d, e, a}, 14);
        clearSelection();
        addSelection({ElementType::Object, obj.id, obj.id});
        status = "ADD PYRAMID";
        if (withUndo) pushUndo(before);
    }

    void addPrism(bool withUndo = true) {
        Snapshot before = makeSnapshot();
        Object3D &obj = addObject("Prism");
        const float h = 1.0f;
        uint32_t a = addVertex(obj, {-1, -h, -0.8f});
        uint32_t b = addVertex(obj, { 1, -h, -0.8f});
        uint32_t c = addVertex(obj, { 0, -h,  1.0f});
        uint32_t d = addVertex(obj, {-1,  h, -0.8f});
        uint32_t e = addVertex(obj, { 1,  h, -0.8f});
        uint32_t f = addVertex(obj, { 0,  h,  1.0f});
        addFace(obj, {a, c, b}, 9);
        addFace(obj, {d, e, f}, 10);
        addFace(obj, {a, d, f, c}, 11);
        addFace(obj, {b, c, f, e}, 12);
        addFace(obj, {a, b, e, d}, 14);
        clearSelection();
        addSelection({ElementType::Object, obj.id, obj.id});
        status = "ADD PRISM";
        if (withUndo) pushUndo(before);
    }

    void addCylinder(int sides = 8, bool withUndo = true) {
        Snapshot before = makeSnapshot();
        sides = clampi(sides, 3, 32);
        Object3D &obj = addObject("Cylinder");
        std::vector<uint32_t> bottom;
        std::vector<uint32_t> top;
        for (int i = 0; i < sides; ++i) {
            float a = 2.0f * kPi * float(i) / float(sides);
            float x = std::cos(a);
            float z = std::sin(a);
            bottom.push_back(addVertex(obj, {x, -1, z}));
            top.push_back(addVertex(obj, {x, 1, z}));
        }
        std::vector<uint32_t> bottomFace = bottom;
        std::reverse(bottomFace.begin(), bottomFace.end());
        addFace(obj, bottomFace, 8);
        addFace(obj, top, scene.currentColor);
        for (int i = 0; i < sides; ++i) {
            int j = (i + 1) % sides;
            addFace(obj, {bottom[i], bottom[j], top[j], top[i]}, uint8_t(9 + (i % 6)));
        }
        clearSelection();
        addSelection({ElementType::Object, obj.id, obj.id});
        status = "ADD CYLINDER";
        if (withUndo) pushUndo(before);
    }

    void addLowPolySphere(bool withUndo = true) {
        Snapshot before = makeSnapshot();
        Object3D &obj = addObject("Sphere");
        int segments = 8;
        int rings = 4;
        std::vector<std::vector<uint32_t>> ids(rings + 1);
        for (int r = 0; r <= rings; ++r) {
            float v = float(r) / float(rings);
            float phi = -kPi * 0.5f + v * kPi;
            float y = std::sin(phi);
            float radius = std::cos(phi);
            for (int s = 0; s < segments; ++s) {
                float u = float(s) / float(segments);
                float theta = u * 2.0f * kPi;
                ids[r].push_back(addVertex(obj, {std::cos(theta) * radius, y, std::sin(theta) * radius}));
            }
        }
        for (int r = 0; r < rings; ++r) {
            for (int s = 0; s < segments; ++s) {
                int n = (s + 1) % segments;
                if (r == 0) {
                    addFace(obj, {ids[r][s], ids[r + 1][s], ids[r + 1][n]}, uint8_t(9 + (s % 6)));
                } else if (r == rings - 1) {
                    addFace(obj, {ids[r][s], ids[r + 1][s], ids[r][n]}, uint8_t(9 + (s % 6)));
                } else {
                    addFace(obj, {ids[r][s], ids[r + 1][s], ids[r + 1][n], ids[r][n]}, uint8_t(9 + (s % 6)));
                }
            }
        }
        clearSelection();
        addSelection({ElementType::Object, obj.id, obj.id});
        status = "ADD SPHERE";
        if (withUndo) pushUndo(before);
    }

    void addSingleVertex(bool withUndo = true) {
        Snapshot before = makeSnapshot();
        Object3D &obj = addObject("Vertex");
        uint32_t id = addVertex(obj, {0, 0, 0});
        clearSelection();
        addSelection({ElementType::Vertex, obj.id, id});
        status = "ADD VERTEX";
        if (withUndo) pushUndo(before);
    }

    ViewState *stateFor(ViewKind kind) {
        for (auto &v : views) {
            if (v.kind == kind) return &v;
        }
        return &views[1];
    }

    const ViewState *stateFor(ViewKind kind) const {
        for (const auto &v : views) {
            if (v.kind == kind) return &v;
        }
        return &views[1];
    }

    std::vector<ViewportDraw> layoutViewports() {
        constexpr int menuH = 14;
        constexpr int statusH = 38;
        IRect area {0, menuH, kLogicalW, kLogicalH - menuH - statusH};
        std::vector<ViewportDraw> out;
        if (maximizedView.has_value()) {
            out.push_back({stateFor(*maximizedView), area});
            return out;
        }
        int halfW = area.w / 2;
        int halfH = area.h / 2;
        out.push_back({stateFor(ViewKind::Top), {area.x, area.y, halfW, halfH}});
        out.push_back({stateFor(ViewKind::Perspective), {area.x + halfW, area.y, area.w - halfW, halfH}});
        out.push_back({stateFor(ViewKind::Front), {area.x, area.y + halfH, halfW, area.h - halfH}});
        out.push_back({stateFor(ViewKind::Side), {area.x + halfW, area.y + halfH, area.w - halfW, area.h - halfH}});
        return out;
    }

    std::optional<ViewportDraw> viewportAt(int x, int y) {
        for (auto &v : layoutViewports()) {
            if (v.rect.contains(x, y)) return v;
        }
        return std::nullopt;
    }

    Vec3 cameraPosition(const ViewState &view) const {
        float cp = std::cos(view.pitch);
        Vec3 dir {
            std::sin(view.yaw) * cp,
            std::sin(view.pitch),
            std::cos(view.yaw) * cp
        };
        return view.center - dir * view.distance;
    }

    Projected projectPoint(const ViewportDraw &vd, Vec3 world) const {
        const ViewState &view = *vd.state;
        int cx = vd.rect.x + vd.rect.w / 2;
        int cy = vd.rect.y + vd.rect.h / 2;
        if (view.kind == ViewKind::Top) {
            return {true, int(std::round(cx + (world.x - view.center.x) * view.zoom)),
                    int(std::round(cy - (world.z - view.center.z) * view.zoom)), -world.y};
        }
        if (view.kind == ViewKind::Front) {
            return {true, int(std::round(cx + (world.x - view.center.x) * view.zoom)),
                    int(std::round(cy - (world.y - view.center.y) * view.zoom)), -world.z};
        }
        if (view.kind == ViewKind::Side) {
            return {true, int(std::round(cx + (world.z - view.center.z) * view.zoom)),
                    int(std::round(cy - (world.y - view.center.y) * view.zoom)), -world.x};
        }

        Vec3 cam = cameraPosition(view);
        Vec3 forward = normalize(view.center - cam);
        Vec3 right = normalize(cross(forward, {0, 1, 0}));
        if (length(right) < 0.01f) right = {1, 0, 0};
        Vec3 up = normalize(cross(right, forward));
        Vec3 rel = world - cam;
        float x = dot(rel, right);
        float y = dot(rel, up);
        float z = dot(rel, forward);
        if (z <= 0.05f) return {};
        float f = float(vd.rect.h) * 0.78f;
        return {true, int(std::round(cx + (x / z) * f)), int(std::round(cy - (y / z) * f)), z};
    }

    Vec3 viewDelta(ViewKind kind, int dx, int dy) const {
        const ViewState *v = stateFor(kind);
        float inv = 1.0f / std::max(1.0f, v->zoom);
        if (kind == ViewKind::Top) return {dx * inv, 0, -dy * inv};
        if (kind == ViewKind::Front) return {dx * inv, -dy * inv, 0};
        if (kind == ViewKind::Side) return {0, -dy * inv, dx * inv};
        return {dx * inv, -dy * inv, 0};
    }

    Vec3 objectDragDelta(ViewKind kind, int dx, int dy) const {
        const ViewState *v = stateFor(kind);
        if (kind != ViewKind::Perspective) return viewDelta(kind, dx, dy);

        Vec3 cam = cameraPosition(*v);
        Vec3 forward = normalize(v->center - cam);
        Vec3 right = normalize(cross(forward, {0, 1, 0}));
        if (length(right) < 0.01f) right = {1, 0, 0};
        Vec3 up = normalize(cross(right, forward));
        float units = v->distance / 220.0f;
        return right * (float(dx) * units) - up * (float(dy) * units);
    }

    Vec3 depthDelta(ViewKind kind, float amount) const {
        if (kind == ViewKind::Top) return {0, amount, 0};
        if (kind == ViewKind::Front) return {0, 0, amount};
        if (kind == ViewKind::Side) return {amount, 0, 0};
        return {0, 0, amount};
    }

    Vec3 snapPosition(Vec3 p, Vec3 original, Vec3 changedMask, bool snapOn) const {
        if (!snapOn) return p;
        float g = std::max(0.001f, scene.gridSize);
        auto snapOne = [&](float v) { return std::round(v / g) * g; };
        if (changedMask.x != 0) p.x = snapOne(p.x); else p.x = original.x;
        if (changedMask.y != 0) p.y = snapOne(p.y); else p.y = original.y;
        if (changedMask.z != 0) p.z = snapOne(p.z); else p.z = original.z;
        return p;
    }

    void drawTriangle(const Projected &a, const Projected &b, const Projected &c, uint8_t color, const IRect &clip) {
        float x0 = float(a.x), y0 = float(a.y);
        float x1 = float(b.x), y1 = float(b.y);
        float x2 = float(c.x), y2 = float(c.y);
        float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
        if (std::fabs(area) < 0.001f) return;
        int minX = clampi(int(std::floor(std::min({x0, x1, x2}))), clip.x, clip.x + clip.w - 1);
        int maxX = clampi(int(std::ceil(std::max({x0, x1, x2}))), clip.x, clip.x + clip.w - 1);
        int minY = clampi(int(std::floor(std::min({y0, y1, y2}))), clip.y, clip.y + clip.h - 1);
        int maxY = clampi(int(std::ceil(std::max({y0, y1, y2}))), clip.y, clip.y + clip.h - 1);
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float px = float(x) + 0.5f;
                float py = float(y) + 0.5f;
                float w0 = ((x1 - px) * (y2 - py) - (y1 - py) * (x2 - px)) / area;
                float w1 = ((x2 - px) * (y0 - py) - (y2 - py) * (x0 - px)) / area;
                float w2 = 1.0f - w0 - w1;
                if (w0 >= -0.001f && w1 >= -0.001f && w2 >= -0.001f) {
                    float z = w0 * a.depth + w1 * b.depth + w2 * c.depth;
                    fb.pixelZ(x, y, z, color, clip);
                }
            }
        }
    }

    void drawFaceOutline(const ViewportDraw &vd, const Object3D &obj, const Face &face, uint8_t color) {
        if (face.vertices.size() < 2) return;
        for (size_t i = 0; i < face.vertices.size(); ++i) {
            const Vertex *a = findVertex(obj, face.vertices[i]);
            const Vertex *b = findVertex(obj, face.vertices[(i + 1) % face.vertices.size()]);
            if (!a || !b) continue;
            Projected pa = projectPoint(vd, transformPoint(a->position, obj.transform));
            Projected pb = projectPoint(vd, transformPoint(b->position, obj.transform));
            if (!pa.ok || !pb.ok) continue;
            fb.lineZ(pa.x, pa.y, pa.depth - 0.01f, pb.x, pb.y, pb.depth - 0.01f, color, vd.rect);
        }
    }

    void drawObjectBounds(const ViewportDraw &vd, const Object3D &obj, uint8_t color) {
        if (obj.mesh.vertices.empty()) return;
        Vec3 minP {1.0e9f, 1.0e9f, 1.0e9f};
        Vec3 maxP {-1.0e9f, -1.0e9f, -1.0e9f};
        for (const Vertex &v : obj.mesh.vertices) {
            Vec3 p = transformPoint(v.position, obj.transform);
            minP.x = std::min(minP.x, p.x); minP.y = std::min(minP.y, p.y); minP.z = std::min(minP.z, p.z);
            maxP.x = std::max(maxP.x, p.x); maxP.y = std::max(maxP.y, p.y); maxP.z = std::max(maxP.z, p.z);
        }
        std::array<Vec3, 8> corners {{
            {minP.x, minP.y, minP.z}, {maxP.x, minP.y, minP.z}, {maxP.x, maxP.y, minP.z}, {minP.x, maxP.y, minP.z},
            {minP.x, minP.y, maxP.z}, {maxP.x, minP.y, maxP.z}, {maxP.x, maxP.y, maxP.z}, {minP.x, maxP.y, maxP.z}
        }};
        const std::array<std::pair<int, int>, 12> edges {{
            std::pair<int, int>{0, 1}, std::pair<int, int>{1, 2}, std::pair<int, int>{2, 3}, std::pair<int, int>{3, 0},
            std::pair<int, int>{4, 5}, std::pair<int, int>{5, 6}, std::pair<int, int>{6, 7}, std::pair<int, int>{7, 4},
            std::pair<int, int>{0, 4}, std::pair<int, int>{1, 5}, std::pair<int, int>{2, 6}, std::pair<int, int>{3, 7}
        }};
        for (auto [a, b] : edges) {
            Projected pa = projectPoint(vd, corners[size_t(a)]);
            Projected pb = projectPoint(vd, corners[size_t(b)]);
            if (!pa.ok || !pb.ok) continue;
            fb.lineZ(pa.x, pa.y, pa.depth - 0.02f, pb.x, pb.y, pb.depth - 0.02f, color, vd.rect);
        }
    }

    void drawGeometryHighlights(const ViewportDraw &vd, bool drawNormalVertices) {
        for (const auto &obj : scene.objects) {
            bool objectSelected = isSelected(ElementType::Object, obj.id, obj.id);
            if (objectSelected) drawObjectBounds(vd, obj, kTheme.selection);

            for (const Face &face : obj.mesh.faces) {
                bool selected = isSelected(ElementType::Face, obj.id, face.id);
                bool hovered = hover.type == ElementType::Face && hover.objectId == obj.id && hover.id == face.id;
                if (selected) drawFaceOutline(vd, obj, face, kTheme.selection);
                if (hovered) drawFaceOutline(vd, obj, face, kTheme.hover);
            }

            for (const Vertex &v : obj.mesh.vertices) {
                bool selected = isSelected(ElementType::Vertex, obj.id, v.id);
                bool hovered = hover.type == ElementType::Vertex && hover.objectId == obj.id && hover.id == v.id;
                if (!drawNormalVertices && !selected && !hovered) continue;
                Projected p = projectPoint(vd, transformPoint(v.position, obj.transform));
                if (!p.ok || !vd.rect.contains(p.x, p.y)) continue;
                uint8_t color = selected ? kTheme.selection : kTheme.wire;
                if (hovered) color = kTheme.hover;
                fb.fillRect({p.x - 2, p.y - 2, 5, 5}, color);
                fb.rect({p.x - 2, p.y - 2, 5, 5}, kTheme.viewportBackground);
            }
        }
    }

    void drawGrid(const ViewportDraw &vd) {
        const ViewState &view = *vd.state;
        if (view.kind == ViewKind::Perspective) {
            drawPerspectiveAxes(vd);
            return;
        }
        float grid = std::max(0.001f, scene.gridSize);
        float step = grid * view.zoom;

        Vec3 axisH, axisV;
        uint8_t colorH = LIGHT_RED;
        uint8_t colorV = LIGHT_CYAN;
        if (view.kind == ViewKind::Top) {
            axisH = {1, 0, 0}; axisV = {0, 0, 1};
            colorH = LIGHT_RED; colorV = LIGHT_CYAN;
        } else if (view.kind == ViewKind::Front) {
            axisH = {1, 0, 0}; axisV = {0, 1, 0};
            colorH = LIGHT_RED; colorV = LIGHT_GREEN;
        } else {
            axisH = {0, 0, 1}; axisV = {0, 1, 0};
            colorH = LIGHT_CYAN; colorV = LIGHT_GREEN;
        }

        float spanH = float(vd.rect.w) / view.zoom * 0.5f + grid * 2.0f;
        float spanV = float(vd.rect.h) / view.zoom * 0.5f + grid * 2.0f;
        float centerH = dot(view.center, axisH);
        float centerV = dot(view.center, axisV);
        float hMin = std::floor((centerH - spanH) / grid) * grid;
        float hMax = std::ceil((centerH + spanH) / grid) * grid;
        float vMin = std::floor((centerV - spanV) / grid) * grid;
        float vMax = std::ceil((centerV + spanV) / grid) * grid;

        auto gridColor = [&](float coord) {
            float units = coord / grid;
            int nearest = int(std::round(units));
            if (std::fabs(units - float(nearest)) < 0.001f && nearest % 5 == 0) return kTheme.gridMajor;
            return kTheme.gridMinor;
        };

        auto isOriginCoord = [&](float coord) {
            return std::fabs(coord) < grid * 0.001f;
        };
        auto drawWorldLine = [&](Vec3 p0, Vec3 p1, uint8_t color) {
            Projected s0 = projectPoint(vd, p0);
            Projected s1 = projectPoint(vd, p1);
            if (!s0.ok || !s1.ok) return;
            fb.line(s0.x, s0.y, s1.x, s1.y, color, &vd.rect);
        };
        auto drawWorldAxes = [&]() {
            drawWorldLine(axisV * vMin, axisV * vMax, colorV);
            drawWorldLine(axisH * hMin, axisH * hMax, colorH);
        };

        if (scene.grid && step >= 3.0f) {
            for (float h = hMin; h <= hMax + grid * 0.5f; h += grid) {
                if (isOriginCoord(h)) continue;
                Vec3 p0 = axisH * h + axisV * vMin;
                Vec3 p1 = axisH * h + axisV * vMax;
                drawWorldLine(p0, p1, gridColor(h));
            }
            for (float vCoord = vMin; vCoord <= vMax + grid * 0.5f; vCoord += grid) {
                if (isOriginCoord(vCoord)) continue;
                Vec3 p0 = axisH * hMin + axisV * vCoord;
                Vec3 p1 = axisH * hMax + axisV * vCoord;
                drawWorldLine(p0, p1, gridColor(vCoord));
            }
        }

        drawWorldAxes();
        drawOriginMarker(vd);
    }

    float sceneAxisLength() const {
        float maxAbs = 4.0f;
        for (const auto &obj : scene.objects) {
            for (const auto &v : obj.mesh.vertices) {
                Vec3 p = transformPoint(v.position, obj.transform);
                maxAbs = std::max({maxAbs, std::fabs(p.x), std::fabs(p.y), std::fabs(p.z)});
            }
        }
        float grid = std::max(0.001f, scene.gridSize);
        return std::ceil((maxAbs + grid * 2.0f) / grid) * grid;
    }

    void drawPerspectiveAxes(const ViewportDraw &vd) {
        float len = sceneAxisLength();
        auto drawAxis = [&](Vec3 axis, uint8_t color) {
            Projected p0 = projectPoint(vd, axis * -len);
            Projected p1 = projectPoint(vd, axis * len);
            if (!p0.ok || !p1.ok) return;
            fb.line(p0.x, p0.y, p1.x, p1.y, color, &vd.rect);
        };
        drawAxis({1, 0, 0}, LIGHT_RED);
        drawAxis({0, 1, 0}, LIGHT_GREEN);
        drawAxis({0, 0, 1}, LIGHT_CYAN);
        drawOriginMarker(vd);
    }

    void drawOriginMarker(const ViewportDraw &vd) {
        Projected origin = projectPoint(vd, {0, 0, 0});
        if (!origin.ok || !vd.rect.contains(origin.x, origin.y)) return;
        fb.pixelClip(origin.x, origin.y, WHITE, vd.rect);
        fb.pixelClip(origin.x - 1, origin.y, WHITE, vd.rect);
        fb.pixelClip(origin.x + 1, origin.y, WHITE, vd.rect);
        fb.pixelClip(origin.x, origin.y - 1, WHITE, vd.rect);
        fb.pixelClip(origin.x, origin.y + 1, WHITE, vd.rect);
    }

    void renderMesh(const ViewportDraw &vd) {
        Vec3 lightDir = normalize(Vec3 {-0.4f, 0.8f, -0.5f});
        bool drawFlat = scene.renderMode == RenderMode::Flat || scene.renderMode == RenderMode::FlatWire;
        bool drawWire = scene.renderMode == RenderMode::Wireframe || scene.renderMode == RenderMode::FlatWire;

        if (drawFlat) {
            fb.clearZ(vd.rect);
            for (const auto &obj : scene.objects) {
                for (const Face &face : obj.mesh.faces) {
                    if (face.vertices.size() < 3) continue;
                    std::vector<Projected> pts;
                    pts.reserve(face.vertices.size());
                    bool ok = true;
                    for (uint32_t vid : face.vertices) {
                        const Vertex *v = findVertex(obj, vid);
                        if (!v) {
                            ok = false;
                            break;
                        }
                        Projected p = projectPoint(vd, transformPoint(v->position, obj.transform));
                        if (!p.ok) {
                            ok = false;
                            break;
                        }
                        pts.push_back(p);
                    }
                    if (!ok) continue;

                    uint8_t color = face.color;
                    if (vd.state->kind == ViewKind::Perspective) {
                        Vec3 n = faceNormalWorld(obj, face);
                        float lit = 0.58f + std::max(0.0f, dot(n, lightDir)) * 0.55f;
                        color = shadePalette(face.color, lit);
                    }
                    for (size_t i = 1; i + 1 < pts.size(); ++i) {
                        drawTriangle(pts[0], pts[i], pts[i + 1], color, vd.rect);
                    }
                }
            }
        }

        if (drawWire) {
            for (const auto &obj : scene.objects) {
                bool objectSelected = isSelected(ElementType::Object, obj.id, obj.id);
                std::set<std::pair<uint32_t, uint32_t>> drawn;
                for (const Face &face : obj.mesh.faces) {
                    for (size_t i = 0; i < face.vertices.size(); ++i) {
                        uint32_t aId = face.vertices[i];
                        uint32_t bId = face.vertices[(i + 1) % face.vertices.size()];
                        auto key = std::minmax(aId, bId);
                        if (drawn.count(key)) continue;
                        drawn.insert(key);
                        const Vertex *a = findVertex(obj, aId);
                        const Vertex *b = findVertex(obj, bId);
                        if (!a || !b) continue;
                        Projected pa = projectPoint(vd, transformPoint(a->position, obj.transform));
                        Projected pb = projectPoint(vd, transformPoint(b->position, obj.transform));
                        if (!pa.ok || !pb.ok) continue;
                        uint8_t color = kTheme.wire;
                        if (objectSelected || isSelected(ElementType::Face, obj.id, face.id)) {
                            color = kTheme.selection;
                        }
                        if (hover.type == ElementType::Edge && hover.objectId == obj.id &&
                            ((hover.edgeA == aId && hover.edgeB == bId) || (hover.edgeA == bId && hover.edgeB == aId))) {
                            color = kTheme.hover;
                        }
                        fb.lineZ(pa.x, pa.y, pa.depth, pb.x, pb.y, pb.depth, color, vd.rect);
                    }
                }
            }
        }
        drawGeometryHighlights(vd, drawWire);
    }

    void renderViewport(const ViewportDraw &vd) {
        bool active = vd.state->kind == activeView;
        fb.fillRect(vd.rect, kTheme.viewportBackground);
        fb.rect(vd.rect, active ? kTheme.borderActive : kTheme.border);
        drawGrid(vd);
        renderMesh(vd);
        std::string title = text(viewKey(vd.state->kind));
        IRect header {vd.rect.x + 1, vd.rect.y + 1, std::min(vd.rect.w - 2, textWidth(title) + 10), 13};
        fb.fillRect(header, active ? kTheme.panelActive : kTheme.panel);
        drawText(fb, vd.rect.x + 4, vd.rect.y + 3, title, active ? kTheme.textBright : kTheme.text);
        if (maximizedView.has_value()) {
            std::string split = text("hint.split");
            drawText(fb, vd.rect.x + vd.rect.w - textWidth(split) - 5, vd.rect.y + 3, split, kTheme.hover);
        }
    }

    std::vector<std::pair<std::string, IRect>> menuBarRects() const {
        std::array<std::string, 6> ids {"FILE", "EDIT", "ADD", "VIEW", "LANG", "HELP"};
        std::vector<std::pair<std::string, IRect>> out;
        int x = 130;
        for (const auto &id : ids) {
            std::string label = menuLabel(id);
            int w = textWidth(label) + 6;
            out.push_back({id, {x - 3, 1, w, 12}});
            x += w + 8;
        }
        return out;
    }

    void drawMenuBar() {
        fb.fillRect({0, 0, kLogicalW, 14}, kTheme.panel);
        fb.line(0, 13, kLogicalW - 1, 13, kTheme.borderActive);
        drawText(fb, 4, 3, "PIXELFORGE 16", kTheme.textBright);
        for (const auto &[id, r] : menuBarRects()) {
            std::string label = menuLabel(id);
            bool open = menu.open && menu.name == id;
            if (open) fb.fillRect(r, kTheme.panelActive);
            drawText(fb, r.x + 3, 3, label, open ? kTheme.textBright : kTheme.textBright);
        }
    }

    IRect paletteRect(int index) const {
        int size = 10;
        int gap = 2;
        int x0 = 336;
        int y0 = kLogicalH - 20;
        return {x0 + index * (size + gap), y0, size, size};
    }

    int paletteIndexAt(int x, int y) const {
        for (int i = 0; i < 16; ++i) {
            if (paletteRect(i).contains(x, y)) return i;
        }
        return -1;
    }

    uint8_t statusColor() const {
        std::string upper = status;
        for (char &c : upper) c = char(std::toupper(static_cast<unsigned char>(c)));
        if (upper.find("FAILED") != std::string::npos ||
            upper.find("ERROR") != std::string::npos ||
            upper.find("INVALID") != std::string::npos ||
            upper.rfind("SELECT", 0) == 0 ||
            upper.find("NOTHING") != std::string::npos ||
            upper.find("CANCEL") != std::string::npos) {
            return kTheme.warning;
        }
        if (upper.find("SAVED") != std::string::npos ||
            upper.find("LOADED") != std::string::npos ||
            upper.find("EXPORTED") != std::string::npos ||
            upper.find("IMPORTED") != std::string::npos ||
            upper.find("DONE") != std::string::npos ||
            upper.find(" ON") != std::string::npos) {
            return kTheme.positive;
        }
        return kTheme.text;
    }

    std::string shortcutFor(const std::string &item) const {
        if (item == "NEW") return "CTRL+N";
        if (item == "OPEN") return "CTRL+O";
        if (item == "SAVE") return "CTRL+S";
        if (item == "SAVE AS") return "CTRL+SHIFT+S";
        if (item == "EXTRUDE") return "E";
        if (item == "INSET") return "I";
        if (item == "FLIP FACE") return "F";
        if (item == "MOVE Z") return "Z";
        if (item == "DUPLICATE") return "D";
        if (item == "DELETE") return "X";
        if (item == "GRID") return "ON/OFF";
        if (item == "TOGGLE SNAP") return "ON/OFF";
        if (item == "SNAP TO GRID") return "CTRL";
        if (item == "PAN") return "MMB";
        if (item == "WIREFRAME") return "M";
        if (item == "FLAT") return "M";
        if (item == "FLAT+WIRE") return "M";
        if (item == "FRAME SELECTED") return "F";
        if (item == "FRAME ALL") return "HOME";
        if (item == "RESET VIEWS") return "CTRL+HOME";
        if (item == "RESET ALL VIEWS") return "CTRL+HOME";
        return {};
    }

    int menuItemHeight() const {
        return 12;
    }

    int popupWidthFor(const std::vector<std::string> &items, int minWidth) const {
        int width = minWidth;
        for (const auto &item : items) {
            int row = textWidth(itemLabel(item)) + 10;
            std::string shortcut = shortcutFor(item);
            if (!shortcut.empty()) row += textWidth(shortcut) + 12;
            width = std::max(width, row);
        }
        return clampi(width, minWidth, kLogicalW - 4);
    }

    void placePopup(int x, int y, int minWidth) {
        int w = popupWidthFor(menu.items, minWidth);
        int h = int(menu.items.size()) * menuItemHeight() + 2;
        menu.rect = {clampi(x, 0, kLogicalW - w - 1), clampi(y, 14, kLogicalH - h - 1), w, h};
    }

    void drawStatusBar() {
        IRect r {0, kLogicalH - 38, kLogicalW, 38};
        fb.fillRect(r, kTheme.panel);
        fb.line(0, r.y, kLogicalW - 1, r.y, kTheme.borderActive);

        int verts = 0;
        int faces = 0;
        for (const auto &obj : scene.objects) {
            verts += int(obj.mesh.vertices.size());
            faces += int(obj.mesh.faces.size());
        }
        std::ostringstream ss;
        ss << text(renderModeKey(scene.renderMode))
           << " | " << text("label.snap") << ":" << (scene.snap ? scene.gridSize : 0)
           << " | " << text("label.grid") << ":" << (scene.grid ? scene.gridSize : 0)
           << " | " << text("label.vertex_short") << ":" << verts << " " << text("label.face_short") << ":" << faces
           << " | " << text("label.color") << ":" << int(scene.currentColor)
           << " | " << localizedStatus();
        drawText(fb, 4, r.y + 5, ss.str(), statusColor());

        drawText(fb, 4, r.y + 22, text("hint.main"), kTheme.hover);
        drawText(fb, 238, kLogicalH - 18, text("label.palette"), kTheme.textBright);
        for (int i = 0; i < 16; ++i) {
            IRect pr = paletteRect(i);
            fb.fillRect(pr, uint8_t(i));
            uint8_t border = kTheme.disabled;
            if (paletteIndexAt(mouseX, mouseY) == i) border = kTheme.hover;
            if (i == scene.currentColor) border = kTheme.selection;
            fb.rect(pr, border);
            if (i == scene.currentColor) fb.rect({pr.x - 1, pr.y - 1, pr.w + 2, pr.h + 2}, kTheme.textBright);
        }
    }

    void drawMenuPopup() {
        if (!menu.open) return;
        fb.fillRect(menu.rect, kTheme.panel);
        fb.rect(menu.rect, kTheme.borderActive);
        for (size_t i = 0; i < menu.items.size(); ++i) {
            IRect item {menu.rect.x + 1, menu.rect.y + 1 + int(i) * menuItemHeight(), menu.rect.w - 2, menuItemHeight()};
            bool hot = item.contains(mouseX, mouseY);
            if (hot) fb.fillRect(item, kTheme.panelActive);
            uint8_t textColor = menu.items[i] == "DELETE" ? kTheme.warning : (hot ? kTheme.textBright : kTheme.text);
            std::string label = itemLabel(menu.items[i]);
            drawText(fb, item.x + 3, item.y + 2, label, textColor);
            std::string shortcut = shortcutFor(menu.items[i]);
            if (!shortcut.empty()) {
                drawText(fb, item.x + item.w - textWidth(shortcut) - 3, item.y + 2, shortcut, kTheme.hover);
            }
        }
    }

    void drawBoxSelection() {
        if (drag.mode != DragMode::BoxSelect) return;
        int x0 = std::min(drag.startX, drag.lastX);
        int y0 = std::min(drag.startY, drag.lastY);
        int x1 = std::max(drag.startX, drag.lastX);
        int y1 = std::max(drag.startY, drag.lastY);
        fb.rect({x0, y0, x1 - x0 + 1, y1 - y0 + 1}, kTheme.hover);
    }

    void render() {
        fb.clear(kTheme.background);
        drawMenuBar();
        for (auto &vd : layoutViewports()) {
            renderViewport(vd);
        }
        drawBoxSelection();
        drawStatusBar();
        drawMenuPopup();
    }

    float distToSegment(Vec2 p, Vec2 a, Vec2 b) const {
        Vec2 ab {b.x - a.x, b.y - a.y};
        Vec2 ap {p.x - a.x, p.y - a.y};
        float len2 = ab.x * ab.x + ab.y * ab.y;
        if (len2 <= 0.0001f) {
            float dx = p.x - a.x, dy = p.y - a.y;
            return std::sqrt(dx * dx + dy * dy);
        }
        float t = clampf((ap.x * ab.x + ap.y * ab.y) / len2, 0.0f, 1.0f);
        float x = a.x + ab.x * t;
        float y = a.y + ab.y * t;
        float dx = p.x - x, dy = p.y - y;
        return std::sqrt(dx * dx + dy * dy);
    }

    bool pointInTri(float px, float py, const Projected &a, const Projected &b, const Projected &c, float &depth) const {
        float x0 = float(a.x), y0 = float(a.y);
        float x1 = float(b.x), y1 = float(b.y);
        float x2 = float(c.x), y2 = float(c.y);
        float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
        if (std::fabs(area) < 0.001f) return false;
        float w0 = ((x1 - px) * (y2 - py) - (y1 - py) * (x2 - px)) / area;
        float w1 = ((x2 - px) * (y0 - py) - (y2 - py) * (x0 - px)) / area;
        float w2 = 1.0f - w0 - w1;
        if (w0 >= -0.001f && w1 >= -0.001f && w2 >= -0.001f) {
            depth = w0 * a.depth + w1 * b.depth + w2 * c.depth;
            return true;
        }
        return false;
    }

    HoverInfo pick(int x, int y) {
        HoverInfo result;
        auto opt = viewportAt(x, y);
        if (!opt) return result;
        ViewportDraw vd = *opt;
        result.view = vd.state->kind;

        HoverInfo bestVertex;
        float bestVD = 25.0f;
        for (const auto &obj : scene.objects) {
            for (const auto &v : obj.mesh.vertices) {
                Projected p = projectPoint(vd, transformPoint(v.position, obj.transform));
                if (!p.ok) continue;
                float dx = float(x - p.x), dy = float(y - p.y);
                float d = dx * dx + dy * dy;
                if (d <= bestVD) {
                    bestVD = d;
                    bestVertex = {ElementType::Vertex, obj.id, v.id, 0, 0, p.depth, p.x, p.y, vd.state->kind};
                }
            }
        }
        if (bestVertex.type != ElementType::None) return bestVertex;

        HoverInfo bestEdge;
        float bestED = 4.0f;
        for (const auto &obj : scene.objects) {
            std::set<std::pair<uint32_t, uint32_t>> edges;
            for (const Face &f : obj.mesh.faces) {
                for (size_t i = 0; i < f.vertices.size(); ++i) {
                    auto key = std::minmax(f.vertices[i], f.vertices[(i + 1) % f.vertices.size()]);
                    edges.insert(key);
                }
            }
            for (auto [aId, bId] : edges) {
                const Vertex *a = findVertex(obj, aId);
                const Vertex *b = findVertex(obj, bId);
                if (!a || !b) continue;
                Projected pa = projectPoint(vd, transformPoint(a->position, obj.transform));
                Projected pb = projectPoint(vd, transformPoint(b->position, obj.transform));
                if (!pa.ok || !pb.ok) continue;
                float d = distToSegment({float(x), float(y)}, {float(pa.x), float(pa.y)}, {float(pb.x), float(pb.y)});
                if (d <= bestED) {
                    bestED = d;
                    bestEdge = {ElementType::Edge, obj.id, 0, aId, bId, std::min(pa.depth, pb.depth), int((pa.x + pb.x) / 2), int((pa.y + pb.y) / 2), vd.state->kind};
                }
            }
        }
        if (bestEdge.type != ElementType::None) return bestEdge;

        HoverInfo bestFace;
        for (const auto &obj : scene.objects) {
            for (const Face &f : obj.mesh.faces) {
                std::vector<Projected> pts;
                bool ok = true;
                for (uint32_t vid : f.vertices) {
                    const Vertex *v = findVertex(obj, vid);
                    if (!v) {
                        ok = false;
                        break;
                    }
                    Projected p = projectPoint(vd, transformPoint(v->position, obj.transform));
                    if (!p.ok) {
                        ok = false;
                        break;
                    }
                    pts.push_back(p);
                }
                if (!ok || pts.size() < 3) continue;
                for (size_t i = 1; i + 1 < pts.size(); ++i) {
                    float depth = 1.0e30f;
                    if (pointInTri(float(x) + 0.5f, float(y) + 0.5f, pts[0], pts[i], pts[i + 1], depth)) {
                        if (depth < bestFace.depth) {
                            bestFace = {ElementType::Face, obj.id, f.id, 0, 0, depth, x, y, vd.state->kind};
                        }
                    }
                }
            }
        }
        if (bestFace.type != ElementType::None) return bestFace;

        return result;
    }

    void refreshHoverUnderMouse() {
        hover = pick(mouseX, mouseY);
        if (hover.type != ElementType::None) {
            activeView = hover.view;
            return;
        }
        auto vd = viewportAt(mouseX, mouseY);
        if (vd) activeView = vd->state->kind;
    }

    ElementRef refFromHover(const HoverInfo &h) const {
        if (h.type == ElementType::Vertex) return {ElementType::Vertex, h.objectId, h.id};
        if (h.type == ElementType::Face) return {ElementType::Face, h.objectId, h.id};
        if (h.type == ElementType::Object) return {ElementType::Object, h.objectId, h.id};
        return {};
    }

    void selectFromHover(bool shift, bool ctrl) {
        ElementRef r = refFromHover(hover);
        if (r.type == ElementType::None) {
            if (!shift && !ctrl) {
                clearSelection();
                activeObjectId = 0;
            }
            return;
        }
        if (ctrl) {
            toggleSelection(r);
        } else if (shift) {
            addSelection(r);
        } else {
            clearSelection();
            addSelection(r);
        }
    }

    void beginVertexDrag(int x, int y, ViewKind view, bool shift, bool ctrl) {
        if (hover.type != ElementType::Vertex) return;
        ElementRef hit {ElementType::Vertex, hover.objectId, hover.id};
        if (!isSelected(hit.type, hit.objectId, hit.id)) {
            selectFromHover(shift, ctrl);
        } else if (ctrl) {
            toggleSelection(hit);
        }
        std::vector<ElementRef> verts = selectedVertices();
        if (verts.empty()) return;
        drag = DragState {};
        drag.mode = DragMode::VertexDrag;
        drag.view = view;
        drag.startX = drag.lastX = x;
        drag.startY = drag.lastY = y;
        drag.before = makeSnapshot();
        for (const ElementRef &r : verts) {
            Object3D *obj = findObject(scene, r.objectId);
            if (!obj) continue;
            Vertex *v = findVertex(*obj, r.id);
            if (v) drag.vertices.push_back({r.objectId, r.id, v->position});
        }
        captureMouse();
        status = "MOVE VERTEX";
    }

    void updateVertexDrag(int x, int y, bool ctrlDown) {
        int dx = x - drag.startX;
        int dy = y - drag.startY;
        Vec3 deltaWorld = viewDelta(drag.view, dx, dy);
        bool snapOn = ctrlDown ? !scene.snap : scene.snap;
        for (const auto &item : drag.vertices) {
            Object3D *obj = findObject(scene, item.objectId);
            if (!obj) continue;
            Vertex *v = findVertex(*obj, item.vertexId);
            if (!v) continue;
            Vec3 localDelta = inverseTransformVector(deltaWorld, obj->transform);
            Vec3 changedMask {std::fabs(localDelta.x) > 0.00001f ? 1.0f : 0.0f,
                              std::fabs(localDelta.y) > 0.00001f ? 1.0f : 0.0f,
                              std::fabs(localDelta.z) > 0.00001f ? 1.0f : 0.0f};
            Vec3 p = item.original + localDelta;
            v->position = snapPosition(p, item.original, changedMask, snapOn);
        }
        drag.lastX = x;
        drag.lastY = y;
        drag.changed = true;
        if (std::fabs(deltaWorld.z) > 0.00001f) {
            status = "Z: " + formatFloat(deltaWorld.z);
            if (!drag.vertices.empty()) status += " (" + formatFloat(deltaWorld.z) + ")";
        } else {
            status = "DRAG TO MOVE";
        }
    }

    void beginVertexZMove(int x, int y) {
        std::vector<ElementRef> verts = selectedVertices();
        if (verts.empty() && hover.type == ElementType::Vertex) {
            clearSelection();
            addSelection({ElementType::Vertex, hover.objectId, hover.id});
            verts = selectedVertices();
        }
        if (verts.empty()) {
            status = "SELECT VERTICES";
            return;
        }
        drag = DragState {};
        drag.mode = DragMode::VertexZMove;
        drag.view = activeView;
        drag.startX = drag.lastX = x;
        drag.startY = drag.lastY = y;
        drag.before = makeSnapshot();
        for (const ElementRef &r : verts) {
            Object3D *obj = findObject(scene, r.objectId);
            if (!obj) continue;
            Vertex *v = findVertex(*obj, r.id);
            if (v) drag.vertices.push_back({r.objectId, r.id, v->position});
        }
        if (drag.vertices.empty()) {
            drag = DragState {};
            status = "SELECT VERTICES";
            return;
        }
        captureMouse();
        status = "MOVE Z | DRAG OR TYPE VALUE | ENTER APPLY | ESC CANCEL";
    }

    float zAmountFromMouse(int x, int y) const {
        const ViewState *view = stateFor(drag.view);
        float inv = 1.0f / std::max(1.0f, view->zoom);
        if (drag.view == ViewKind::Side) return float(x - drag.startX) * inv;
        return float(drag.startY - y) * inv;
    }

    void applyZMove(float amount, bool snapOn) {
        Vec3 deltaWorld {0.0f, 0.0f, amount};
        for (const auto &item : drag.vertices) {
            Object3D *obj = findObject(scene, item.objectId);
            if (!obj) continue;
            Vertex *v = findVertex(*obj, item.vertexId);
            if (!v) continue;
            Vec3 localDelta = inverseTransformVector(deltaWorld, obj->transform);
            Vec3 changedMask {std::fabs(localDelta.x) > 0.00001f ? 1.0f : 0.0f,
                              std::fabs(localDelta.y) > 0.00001f ? 1.0f : 0.0f,
                              std::fabs(localDelta.z) > 0.00001f ? 1.0f : 0.0f};
            Vec3 p = item.original + localDelta;
            v->position = snapPosition(p, item.original, changedMask, snapOn);
        }
        drag.changed = drag.changed || std::fabs(amount) > 0.00001f;
    }

    bool parseZValue(float &amount) const {
        if (drag.typedValue.empty() || drag.typedValue == "-" || drag.typedValue == "+" || drag.typedValue == ".") return false;
        char *end = nullptr;
        amount = std::strtof(drag.typedValue.c_str(), &end);
        return end && *end == '\0';
    }

    void updateVertexZMove(int x, int y, bool ctrlDown) {
        float amount = 0.0f;
        if (!parseZValue(amount)) {
            amount = zAmountFromMouse(x, y);
        }
        bool snapOn = ctrlDown ? !scene.snap : scene.snap;
        applyZMove(amount, snapOn);
        drag.lastX = x;
        drag.lastY = y;
        status = "Z: " + formatFloat(amount) + " (" + formatFloat(amount) + ")";
        if (!drag.typedValue.empty()) status += " | " + drag.typedValue;
    }

    void commitVertexZMove() {
        if (drag.mode != DragMode::VertexZMove) return;
        float amount = 0.0f;
        bool exact = parseZValue(amount);
        if (exact) {
            bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool snapOn = ctrl ? !scene.snap : scene.snap;
            applyZMove(amount, snapOn);
        }
        if (drag.changed) {
            pushUndo(drag.before);
            status = "MOVE DONE";
        } else {
            status = "MOVE CANCEL";
        }
        releaseMouseCapture();
        drag = DragState {};
        refreshHoverUnderMouse();
    }

    bool isObjectSelected(uint32_t objectId) const {
        return isSelected(ElementType::Object, objectId, objectId);
    }

    bool hoverOnDraggableObject() const {
        if (hover.objectId == 0) return false;
        return isObjectSelected(hover.objectId) || hover.objectId == activeObjectId;
    }

    std::vector<ElementRef> objectDragTargets() const {
        std::vector<ElementRef> objects = selectedObjects();
        if (!objects.empty()) return objects;
        if (hover.objectId != 0 && hover.objectId == activeObjectId && objectExists(hover.objectId)) {
            return {{ElementType::Object, hover.objectId, hover.objectId}};
        }
        return {};
    }

    void selectObjectFromHover(bool shift, bool ctrl) {
        if (hover.objectId == 0) return;
        ElementRef r {ElementType::Object, hover.objectId, hover.objectId};
        if (ctrl) {
            toggleSelection(r);
        } else if (shift) {
            addSelection(r);
        } else {
            clearSelection();
            addSelection(r);
        }
    }

    void beginObjectDrag(int x, int y, ViewKind view) {
        std::vector<ElementRef> objects = objectDragTargets();
        if (objects.empty()) {
            status = "NOTHING SELECTED";
            return;
        }
        drag = DragState {};
        drag.mode = DragMode::ObjectDrag;
        drag.view = view;
        drag.startX = drag.lastX = x;
        drag.startY = drag.lastY = y;
        drag.before = makeSnapshot();
        for (const ElementRef &r : objects) {
            Object3D *obj = findObject(scene, r.objectId);
            if (obj) drag.objects.push_back({obj->id, obj->transform.position});
        }
        if (drag.objects.empty()) {
            drag = DragState {};
            status = "NOTHING SELECTED";
            return;
        }
        captureMouse();
        status = "MOVE OBJECT";
    }

    void updateObjectDrag(int x, int y, bool ctrlDown) {
        int dx = x - drag.startX;
        int dy = y - drag.startY;
        if (!drag.changed && std::abs(dx) < 3 && std::abs(dy) < 3) {
            drag.lastX = x;
            drag.lastY = y;
            return;
        }

        Vec3 deltaWorld = objectDragDelta(drag.view, dx, dy);
        Vec3 changedMask {std::fabs(deltaWorld.x) > 0.00001f ? 1.0f : 0.0f,
                          std::fabs(deltaWorld.y) > 0.00001f ? 1.0f : 0.0f,
                          std::fabs(deltaWorld.z) > 0.00001f ? 1.0f : 0.0f};
        bool snapOn = ctrlDown ? !scene.snap : scene.snap;
        Vec3 firstPos {};
        bool haveFirst = false;
        for (const auto &item : drag.objects) {
            Object3D *obj = findObject(scene, item.objectId);
            if (!obj) continue;
            obj->transform.position = snapPosition(item.original + deltaWorld, item.original, changedMask, snapOn);
            if (!haveFirst) {
                firstPos = obj->transform.position;
                haveFirst = true;
            }
        }

        drag.lastX = x;
        drag.lastY = y;
        drag.changed = true;
        status = "MOVE OBJECT";
        if (haveFirst) {
            Vec3 shownDelta = firstPos - drag.objects.front().original;
            status += " | X:" + formatFloat(firstPos.x) + " Y:" + formatFloat(firstPos.y) + " Z:" + formatFloat(firstPos.z);
            status += " | dX:" + formatFloat(shownDelta.x) + " dY:" + formatFloat(shownDelta.y) + " dZ:" + formatFloat(shownDelta.z);
        }
    }

    void beginBoxSelect(int x, int y, ViewKind view) {
        drag = DragState {};
        drag.mode = DragMode::BoxSelect;
        drag.view = view;
        drag.startX = drag.lastX = x;
        drag.startY = drag.lastY = y;
        drag.before = makeSnapshot();
        captureMouse();
        status = "BOX SELECT";
    }

    void finishBoxSelect(bool shift, bool ctrl) {
        auto vd = viewportAt(drag.startX, drag.startY);
        if (!vd) return;
        int x0 = std::min(drag.startX, drag.lastX);
        int y0 = std::min(drag.startY, drag.lastY);
        int x1 = std::max(drag.startX, drag.lastX);
        int y1 = std::max(drag.startY, drag.lastY);
        if (!shift && !ctrl) clearSelection();
        for (const auto &obj : scene.objects) {
            for (const Vertex &v : obj.mesh.vertices) {
                Projected p = projectPoint(*vd, transformPoint(v.position, obj.transform));
                if (!p.ok) continue;
                if (p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1) {
                    ElementRef r {ElementType::Vertex, obj.id, v.id};
                    if (ctrl) toggleSelection(r); else addSelection(r);
                }
            }
        }
        status = "BOX SELECT DONE";
    }

    void beginCameraDrag(int x, int y, ViewKind view, bool pan) {
        drag = DragState {};
        drag.mode = pan ? DragMode::Pan : DragMode::Orbit;
        drag.view = view;
        drag.startX = drag.lastX = x;
        drag.startY = drag.lastY = y;
        captureMouse();
        status = pan ? "PAN VIEW" : "ORBIT VIEW";
    }

    void updateCameraDrag(int x, int y) {
        ViewState *view = stateFor(drag.view);
        int dx = x - drag.lastX;
        int dy = y - drag.lastY;
        if (drag.mode == DragMode::Orbit) {
            view->yaw += dx * 0.01f;
            view->pitch = clampf(view->pitch + dy * 0.01f, radians(-85.0f), radians(85.0f));
        } else if (drag.mode == DragMode::Pan) {
            if (view->kind == ViewKind::Perspective) {
                Vec3 cam = cameraPosition(*view);
                Vec3 forward = normalize(view->center - cam);
                Vec3 right = normalize(cross(forward, {0, 1, 0}));
                Vec3 up = normalize(cross(right, forward));
                float units = view->distance / 220.0f;
                view->center = view->center - right * (float(dx) * units) + up * (float(dy) * units);
            } else {
                view->center = view->center - viewDelta(view->kind, dx, dy);
            }
        }
        drag.lastX = x;
        drag.lastY = y;
    }

    void finishDrag() {
        if (drag.mode == DragMode::VertexDrag && drag.changed) {
            pushUndo(drag.before);
            status = "MOVE DONE";
        } else if (drag.mode == DragMode::VertexZMove) {
            commitVertexZMove();
            return;
        } else if (drag.mode == DragMode::ObjectDrag && drag.changed) {
            pushUndo(drag.before);
            status = "MOVE DONE";
        } else if (drag.mode == DragMode::BoxSelect) {
            // Selection-only changes are intentionally not undo history entries.
        }
        drag = DragState {};
        releaseMouseCapture();
        refreshHoverUnderMouse();
    }

    void cancelDrag() {
        if (drag.mode == DragMode::VertexDrag || drag.mode == DragMode::VertexZMove || drag.mode == DragMode::ObjectDrag) {
            restoreSnapshot(drag.before);
            status = "MOVE CANCEL";
        } else {
            status = "CANCEL";
        }
        releaseMouseCapture();
        drag = DragState {};
        refreshHoverUnderMouse();
    }

    void applyColorToSelection(uint8_t color) {
        Snapshot before = makeSnapshot();
        bool changed = false;
        for (const ElementRef &r : selectedFaces()) {
            Object3D *obj = findObject(scene, r.objectId);
            if (!obj) continue;
            Face *f = findFace(*obj, r.id);
            if (f) {
                f->color = color & 15;
                changed = true;
            }
        }
        if (!changed && hover.type == ElementType::Face) {
            Object3D *obj = findObject(scene, hover.objectId);
            if (obj) {
                Face *f = findFace(*obj, hover.id);
                if (f) {
                    f->color = color & 15;
                    changed = true;
                }
            }
        }
        scene.currentColor = color & 15;
        status = std::string("COLOR ") + std::to_string(int(scene.currentColor));
        if (changed) pushUndo(before);
    }

    std::optional<ElementRef> firstTargetFace() const {
        auto faces = selectedFaces();
        if (!faces.empty()) return faces.front();
        if (hover.type == ElementType::Face) return ElementRef {ElementType::Face, hover.objectId, hover.id};
        return std::nullopt;
    }

    void extrude(float distance = 1.0f) {
        auto faceRef = firstTargetFace();
        if (!faceRef) {
            status = "SELECT OR HOVER FACE";
            return;
        }
        Snapshot before = makeSnapshot();
        Object3D *obj = findObject(scene, faceRef->objectId);
        if (!obj) return;
        Face *face = findFace(*obj, faceRef->id);
        if (!face || face->vertices.size() < 3) return;

        Vec3 n = faceNormalLocal(*obj, *face);
        std::vector<uint32_t> oldVerts = face->vertices;
        std::vector<uint32_t> newVerts;
        for (uint32_t vid : oldVerts) {
            Vertex *v = findVertex(*obj, vid);
            if (!v) continue;
            newVerts.push_back(addVertex(*obj, v->position + n * distance));
        }
        if (newVerts.size() != oldVerts.size()) return;
        uint32_t newFaceId = addFace(*obj, newVerts, face->color);
        size_t nverts = oldVerts.size();
        for (size_t i = 0; i < nverts; ++i) {
            size_t j = (i + 1) % nverts;
            addFace(*obj, {oldVerts[i], oldVerts[j], newVerts[j], newVerts[i]}, scene.currentColor);
        }
        clearSelection();
        addSelection({ElementType::Face, obj->id, newFaceId});
        pushUndo(before);
        status = "EXTRUDE 1 UNIT";
    }

    void inset(float amount = 0.25f) {
        auto faceRef = firstTargetFace();
        if (!faceRef) {
            status = "SELECT OR HOVER FACE";
            return;
        }
        Snapshot before = makeSnapshot();
        Object3D *obj = findObject(scene, faceRef->objectId);
        if (!obj) return;
        Face *face = findFace(*obj, faceRef->id);
        if (!face || face->vertices.size() < 3) return;
        Vec3 center = faceCenterLocal(*obj, *face);
        std::vector<uint32_t> oldVerts = face->vertices;
        std::vector<uint32_t> newVerts;
        float insetScale = clampf(1.0f - amount, 0.05f, 0.95f);
        for (uint32_t vid : oldVerts) {
            Vertex *v = findVertex(*obj, vid);
            if (!v) continue;
            newVerts.push_back(addVertex(*obj, center + (v->position - center) * insetScale));
        }
        if (newVerts.size() != oldVerts.size()) return;
        uint32_t newFaceId = addFace(*obj, newVerts, face->color);
        for (size_t i = 0; i < oldVerts.size(); ++i) {
            size_t j = (i + 1) % oldVerts.size();
            addFace(*obj, {oldVerts[i], oldVerts[j], newVerts[j], newVerts[i]}, scene.currentColor);
        }
        clearSelection();
        addSelection({ElementType::Face, obj->id, newFaceId});
        pushUndo(before);
        status = "INSET";
    }

    void createFaceFromSelection() {
        std::vector<ElementRef> verts = selectedVertices();
        if (verts.size() < 3) {
            status = "SELECT 3+ VERTICES";
            return;
        }
        uint32_t objectId = verts.front().objectId;
        if (std::any_of(verts.begin(), verts.end(), [&](const ElementRef &r) { return r.objectId != objectId; })) {
            status = "FACE NEEDS ONE OBJECT";
            return;
        }
        Snapshot before = makeSnapshot();
        Object3D *obj = findObject(scene, objectId);
        if (!obj) return;
        std::vector<uint32_t> ids;
        for (const ElementRef &r : verts) ids.push_back(r.id);
        uint32_t faceId = addFace(*obj, ids, scene.currentColor);
        clearSelection();
        addSelection({ElementType::Face, obj->id, faceId});
        pushUndo(before);
        status = "FACE CREATED";
    }

    void deleteSelection() {
        if (selection.empty()) {
            if (hover.type == ElementType::Face || hover.type == ElementType::Vertex) {
                selection.push_back(refFromHover(hover));
            } else {
                status = "NOTHING SELECTED";
                return;
            }
        }
        Snapshot before = makeSnapshot();
        std::set<uint32_t> objectsToDelete;
        std::map<uint32_t, std::set<uint32_t>> facesToDelete;
        std::map<uint32_t, std::set<uint32_t>> verticesToDelete;
        for (const ElementRef &r : selection) {
            if (r.type == ElementType::Object) objectsToDelete.insert(r.objectId);
            if (r.type == ElementType::Face) facesToDelete[r.objectId].insert(r.id);
            if (r.type == ElementType::Vertex) verticesToDelete[r.objectId].insert(r.id);
        }

        scene.objects.erase(std::remove_if(scene.objects.begin(), scene.objects.end(), [&](const Object3D &obj) {
            return objectsToDelete.count(obj.id) > 0;
        }), scene.objects.end());

        for (auto &obj : scene.objects) {
            auto fit = facesToDelete.find(obj.id);
            auto vit = verticesToDelete.find(obj.id);
            if (fit != facesToDelete.end()) {
                obj.mesh.faces.erase(std::remove_if(obj.mesh.faces.begin(), obj.mesh.faces.end(), [&](const Face &f) {
                    return fit->second.count(f.id) > 0;
                }), obj.mesh.faces.end());
            }
            if (vit != verticesToDelete.end()) {
                obj.mesh.faces.erase(std::remove_if(obj.mesh.faces.begin(), obj.mesh.faces.end(), [&](const Face &f) {
                    for (uint32_t id : f.vertices) {
                        if (vit->second.count(id)) return true;
                    }
                    return false;
                }), obj.mesh.faces.end());
                obj.mesh.vertices.erase(std::remove_if(obj.mesh.vertices.begin(), obj.mesh.vertices.end(), [&](const Vertex &v) {
                    return vit->second.count(v.id) > 0;
                }), obj.mesh.vertices.end());
            }
        }
        clearSelection();
        sanitizeActiveObject();
        refreshHoverUnderMouse();
        pushUndo(before);
        status = "DELETE";
    }

    void duplicateSelection() {
        Snapshot before = makeSnapshot();
        std::set<uint32_t> objectIds;
        for (const ElementRef &r : selection) objectIds.insert(r.objectId);
        if (objectIds.empty() && hover.objectId != 0) objectIds.insert(hover.objectId);
        if (objectIds.empty() && !scene.objects.empty()) objectIds.insert(scene.objects.back().id);
        std::vector<Object3D> copies;
        for (uint32_t id : objectIds) {
            Object3D *obj = findObject(scene, id);
            if (!obj) continue;
            Object3D copy = *obj;
            copy.id = allocId();
            copy.name += "_Copy";
            copy.transform.position += {1.5f, 0.0f, 1.5f};
            std::unordered_map<uint32_t, uint32_t> idMap;
            for (Vertex &v : copy.mesh.vertices) {
                uint32_t old = v.id;
                v.id = allocId();
                idMap[old] = v.id;
            }
            for (Face &f : copy.mesh.faces) {
                f.id = allocId();
                for (uint32_t &vid : f.vertices) vid = idMap[vid];
            }
            copies.push_back(copy);
        }
        if (copies.empty()) {
            status = "NOTHING TO DUPLICATE";
            return;
        }
        clearSelection();
        for (const Object3D &copy : copies) {
            scene.objects.push_back(copy);
            addSelection({ElementType::Object, copy.id, copy.id});
        }
        pushUndo(before);
        status = "DUPLICATE";
    }

    void mergeSelectedVertices() {
        std::vector<ElementRef> verts = selectedVertices();
        if (verts.size() < 2) {
            status = "SELECT 2+ VERTICES";
            return;
        }
        uint32_t objId = verts.front().objectId;
        if (std::any_of(verts.begin(), verts.end(), [&](const ElementRef &r) { return r.objectId != objId; })) {
            status = "MERGE ONE OBJECT";
            return;
        }
        Snapshot before = makeSnapshot();
        Object3D *obj = findObject(scene, objId);
        if (!obj) return;
        Vec3 center {0, 0, 0};
        int count = 0;
        for (const ElementRef &r : verts) {
            Vertex *v = findVertex(*obj, r.id);
            if (v) {
                center += v->position;
                ++count;
            }
        }
        if (count == 0) return;
        center = center / float(count);
        uint32_t keep = verts.front().id;
        if (Vertex *kv = findVertex(*obj, keep)) kv->position = center;
        std::set<uint32_t> remove;
        for (size_t i = 1; i < verts.size(); ++i) remove.insert(verts[i].id);
        for (Face &f : obj->mesh.faces) {
            for (uint32_t &vid : f.vertices) {
                if (remove.count(vid)) vid = keep;
            }
            std::vector<uint32_t> compact;
            for (uint32_t vid : f.vertices) {
                if (compact.empty() || compact.back() != vid) compact.push_back(vid);
            }
            if (compact.size() > 1 && compact.front() == compact.back()) compact.pop_back();
            f.vertices = compact;
        }
        obj->mesh.faces.erase(std::remove_if(obj->mesh.faces.begin(), obj->mesh.faces.end(), [](const Face &f) {
            return f.vertices.size() < 3;
        }), obj->mesh.faces.end());
        obj->mesh.vertices.erase(std::remove_if(obj->mesh.vertices.begin(), obj->mesh.vertices.end(), [&](const Vertex &v) {
            return remove.count(v.id) > 0;
        }), obj->mesh.vertices.end());
        clearSelection();
        addSelection({ElementType::Vertex, obj->id, keep});
        pushUndo(before);
        status = "MERGE CENTER";
    }

    void snapSelectedVertices() {
        std::vector<ElementRef> verts = selectedVertices();
        if (verts.empty() && hover.type == ElementType::Vertex) {
            verts.push_back({ElementType::Vertex, hover.objectId, hover.id});
        }
        if (verts.empty()) {
            status = "SELECT VERTICES";
            return;
        }
        Snapshot before = makeSnapshot();
        float g = std::max(0.001f, scene.gridSize);
        for (const ElementRef &r : verts) {
            Object3D *obj = findObject(scene, r.objectId);
            if (!obj) continue;
            Vertex *v = findVertex(*obj, r.id);
            if (!v) continue;
            v->position.x = std::round(v->position.x / g) * g;
            v->position.y = std::round(v->position.y / g) * g;
            v->position.z = std::round(v->position.z / g) * g;
        }
        pushUndo(before);
        status = "SNAP TO GRID";
    }

    void flipFace() {
        auto faceRef = firstTargetFace();
        if (!faceRef) {
            status = "SELECT OR HOVER FACE";
            return;
        }
        Snapshot before = makeSnapshot();
        Object3D *obj = findObject(scene, faceRef->objectId);
        if (!obj) return;
        Face *face = findFace(*obj, faceRef->id);
        if (!face) return;
        std::reverse(face->vertices.begin(), face->vertices.end());
        pushUndo(before);
        status = "FLIP FACE";
    }

    Vec3 selectionCenterLocal(uint32_t objectId) {
        Object3D *obj = findObject(scene, objectId);
        if (!obj) return {0, 0, 0};
        Vec3 c {0, 0, 0};
        int count = 0;
        for (const ElementRef &r : selectedVertices()) {
            if (r.objectId != objectId) continue;
            Vertex *v = findVertex(*obj, r.id);
            if (v) {
                c += v->position;
                ++count;
            }
        }
        if (count == 0) return {0, 0, 0};
        return c / float(count);
    }

    void rotateSelection(float degrees) {
        Snapshot before = makeSnapshot();
        auto verts = selectedVertices();
        if (!verts.empty()) {
            std::set<uint32_t> objIds;
            for (const auto &r : verts) objIds.insert(r.objectId);
            for (uint32_t objId : objIds) {
                Object3D *obj = findObject(scene, objId);
                if (!obj) continue;
                Vec3 c = selectionCenterLocal(objId);
                for (const auto &r : verts) {
                    if (r.objectId != objId) continue;
                    Vertex *v = findVertex(*obj, r.id);
                    if (!v) continue;
                    Vec3 p = v->position - c;
                    if (activeView == ViewKind::Top) p = rotateY(p, radians(degrees));
                    else if (activeView == ViewKind::Front) p = rotateZ(p, radians(degrees));
                    else p = rotateX(p, radians(degrees));
                    v->position = c + p;
                }
            }
            pushUndo(before);
            status = "ROTATE GEOMETRY";
            return;
        }
        bool changed = false;
        for (const auto &objRef : selectedObjects()) {
            Object3D *obj = findObject(scene, objRef.objectId);
            if (!obj) continue;
            obj->transform.rotation.y += radians(degrees);
            changed = true;
        }
        if (!changed && hover.objectId != 0) {
            Object3D *obj = findObject(scene, hover.objectId);
            if (obj) {
                obj->transform.rotation.y += radians(degrees);
                changed = true;
            }
        }
        if (changed) pushUndo(before);
        status = changed ? "ROTATE OBJECT" : "NOTHING TO ROTATE";
    }

    void scaleSelection(float factor) {
        Snapshot before = makeSnapshot();
        auto verts = selectedVertices();
        if (!verts.empty()) {
            std::set<uint32_t> objIds;
            for (const auto &r : verts) objIds.insert(r.objectId);
            for (uint32_t objId : objIds) {
                Object3D *obj = findObject(scene, objId);
                if (!obj) continue;
                Vec3 c = selectionCenterLocal(objId);
                for (const auto &r : verts) {
                    if (r.objectId != objId) continue;
                    Vertex *v = findVertex(*obj, r.id);
                    if (v) v->position = c + (v->position - c) * factor;
                }
            }
            pushUndo(before);
            status = "SCALE GEOMETRY";
            return;
        }
        bool changed = false;
        for (const auto &objRef : selectedObjects()) {
            Object3D *obj = findObject(scene, objRef.objectId);
            if (!obj) continue;
            obj->transform.scale = obj->transform.scale * factor;
            changed = true;
        }
        if (!changed && hover.objectId != 0) {
            Object3D *obj = findObject(scene, hover.objectId);
            if (obj) {
                obj->transform.scale = obj->transform.scale * factor;
                changed = true;
            }
        }
        if (changed) pushUndo(before);
        status = changed ? "SCALE OBJECT" : "NOTHING TO SCALE";
    }

    void nudgeSelection(Vec3 delta, bool shift, bool alt) {
        auto verts = selectedVertices();
        if (verts.empty()) {
            status = "SELECT VERTICES";
            return;
        }
        float mult = shift ? 10.0f : (alt ? 0.1f : 1.0f);
        delta = delta * (scene.gridSize * mult);
        Snapshot before = makeSnapshot();
        for (const ElementRef &r : verts) {
            Object3D *obj = findObject(scene, r.objectId);
            if (!obj) continue;
            Vertex *v = findVertex(*obj, r.id);
            if (v) v->position += inverseTransformVector(delta, obj->transform);
        }
        pushUndo(before);
        status = "NUDGE";
    }

    void cycleRenderMode() {
        if (scene.renderMode == RenderMode::Wireframe) scene.renderMode = RenderMode::Flat;
        else if (scene.renderMode == RenderMode::Flat) scene.renderMode = RenderMode::FlatWire;
        else scene.renderMode = RenderMode::Wireframe;
        status = renderModeName(scene.renderMode);
    }

    void frameAll() {
        if (scene.objects.empty()) return;
        Vec3 minP {1.0e9f, 1.0e9f, 1.0e9f};
        Vec3 maxP {-1.0e9f, -1.0e9f, -1.0e9f};
        bool any = false;
        for (const auto &obj : scene.objects) {
            for (const auto &v : obj.mesh.vertices) {
                Vec3 p = transformPoint(v.position, obj.transform);
                minP.x = std::min(minP.x, p.x); minP.y = std::min(minP.y, p.y); minP.z = std::min(minP.z, p.z);
                maxP.x = std::max(maxP.x, p.x); maxP.y = std::max(maxP.y, p.y); maxP.z = std::max(maxP.z, p.z);
                any = true;
            }
        }
        if (!any) return;
        Vec3 c = (minP + maxP) * 0.5f;
        Vec3 size = maxP - minP;
        float span = std::max({size.x, size.y, size.z, 1.0f});
        for (auto &v : views) {
            v.center = c;
            v.zoom = clampf(150.0f / span, 6.0f, 80.0f);
            if (v.kind == ViewKind::Perspective) {
                v.distance = span * 2.3f + 4.0f;
            }
        }
        status = "FRAME ALL";
    }

    void focusSelection() {
        if (selection.empty()) {
            frameAll();
            return;
        }
        Vec3 c {0, 0, 0};
        int count = 0;
        for (const auto &r : selection) {
            const Object3D *obj = findObject(scene, r.objectId);
            if (!obj) continue;
            if (r.type == ElementType::Vertex) {
                if (const Vertex *v = findVertex(*obj, r.id)) {
                    c += transformPoint(v->position, obj->transform);
                    ++count;
                }
            } else if (r.type == ElementType::Face) {
                if (const Face *f = findFace(*obj, r.id)) {
                    c += transformPoint(faceCenterLocal(*obj, *f), obj->transform);
                    ++count;
                }
            } else if (r.type == ElementType::Object) {
                c += obj->transform.position;
                ++count;
            }
        }
        if (count == 0) {
            frameAll();
            return;
        }
        c = c / float(count);
        for (auto &v : views) v.center = c;
        status = "FRAME SELECTION";
    }

    void resetViews() {
        for (auto &v : views) {
            v.center = {0, 0, 0};
            v.zoom = 18.0f;
            v.yaw = radians(35.0f);
            v.pitch = radians(25.0f);
            v.distance = 10.0f;
        }
        status = "RESET ALL VIEWS";
    }

    void resetActiveView() {
        ViewState *v = stateFor(activeView);
        v->center = {0, 0, 0};
        v->zoom = 18.0f;
        v->yaw = radians(35.0f);
        v->pitch = radians(25.0f);
        v->distance = 10.0f;
        status = "RESET VIEW";
    }

    void copySelection() {
        clipboard.objects.clear();
        std::set<uint32_t> ids;
        for (const auto &r : selection) ids.insert(r.objectId);
        if (ids.empty() && hover.objectId != 0) ids.insert(hover.objectId);
        for (uint32_t id : ids) {
            if (const Object3D *obj = findObject(scene, id)) clipboard.objects.push_back(*obj);
        }
        clipboard.hasData = !clipboard.objects.empty();
        status = clipboard.hasData ? "COPY" : "NOTHING TO COPY";
    }

    void pasteClipboard() {
        if (!clipboard.hasData) {
            status = "CLIPBOARD EMPTY";
            return;
        }
        Snapshot before = makeSnapshot();
        clearSelection();
        for (Object3D copy : clipboard.objects) {
            std::unordered_map<uint32_t, uint32_t> idMap;
            copy.id = allocId();
            copy.name += "_Paste";
            copy.transform.position += {1.0f, 0.0f, 1.0f};
            for (Vertex &v : copy.mesh.vertices) {
                uint32_t old = v.id;
                v.id = allocId();
                idMap[old] = v.id;
            }
            for (Face &f : copy.mesh.faces) {
                f.id = allocId();
                for (uint32_t &vid : f.vertices) vid = idMap[vid];
            }
            scene.objects.push_back(copy);
            addSelection({ElementType::Object, copy.id, copy.id});
        }
        pushUndo(before);
        status = "PASTE";
    }

    std::filesystem::path fileDialog(bool save, const wchar_t *filter, const wchar_t *defaultExt) {
        wchar_t filename[MAX_PATH] {};
        OPENFILENAMEW ofn {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = filter;
        ofn.lpstrDefExt = defaultExt;
        ofn.Flags = OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
        BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
        if (!ok) return {};
        return std::filesystem::path(filename);
    }

    bool savePf16(const std::filesystem::path &path) {
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out << "PF16 1\n";
        out << "NEXT " << scene.nextId << "\n";
        out << "SETTINGS " << int(scene.renderMode) << " " << int(scene.currentColor) << " "
            << scene.grid << " " << scene.snap << " " << scene.gridSize << "\n";
        out << "OBJECTS " << scene.objects.size() << "\n";
        for (const Object3D &obj : scene.objects) {
            out << "OBJECT " << obj.id << " " << obj.name << "\n";
            out << "TRANS " << obj.transform.position.x << " " << obj.transform.position.y << " " << obj.transform.position.z << " "
                << obj.transform.rotation.x << " " << obj.transform.rotation.y << " " << obj.transform.rotation.z << " "
                << obj.transform.scale.x << " " << obj.transform.scale.y << " " << obj.transform.scale.z << "\n";
            out << "VERTICES " << obj.mesh.vertices.size() << "\n";
            for (const Vertex &v : obj.mesh.vertices) {
                out << "v " << v.id << " " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
            }
            out << "FACES " << obj.mesh.faces.size() << "\n";
            for (const Face &f : obj.mesh.faces) {
                out << "f " << f.id << " " << int(f.color) << " " << f.vertices.size();
                for (uint32_t vid : f.vertices) out << " " << vid;
                out << "\n";
            }
            out << "END_OBJECT\n";
        }
        return true;
    }

    bool loadPf16(const std::filesystem::path &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        Scene loaded;
        std::string tag;
        int version = 0;
        in >> tag >> version;
        if (tag != "PF16" || version != 1) return false;
        size_t objectCount = 0;
        while (in >> tag) {
            if (tag == "NEXT") {
                in >> loaded.nextId;
            } else if (tag == "SETTINGS") {
                int mode, color;
                in >> mode >> color >> loaded.grid >> loaded.snap >> loaded.gridSize;
                loaded.renderMode = RenderMode(clampi(mode, 0, 2));
                loaded.currentColor = uint8_t(color & 15);
            } else if (tag == "OBJECTS") {
                in >> objectCount;
                for (size_t oi = 0; oi < objectCount; ++oi) {
                    Object3D obj;
                    in >> tag >> obj.id >> obj.name;
                    if (tag != "OBJECT") return false;
                    in >> tag;
                    if (tag != "TRANS") return false;
                    in >> obj.transform.position.x >> obj.transform.position.y >> obj.transform.position.z
                       >> obj.transform.rotation.x >> obj.transform.rotation.y >> obj.transform.rotation.z
                       >> obj.transform.scale.x >> obj.transform.scale.y >> obj.transform.scale.z;
                    size_t count = 0;
                    in >> tag >> count;
                    if (tag != "VERTICES") return false;
                    for (size_t i = 0; i < count; ++i) {
                        Vertex v;
                        in >> tag >> v.id >> v.position.x >> v.position.y >> v.position.z;
                        if (tag != "v") return false;
                        obj.mesh.vertices.push_back(v);
                    }
                    in >> tag >> count;
                    if (tag != "FACES") return false;
                    for (size_t i = 0; i < count; ++i) {
                        Face f;
                        size_t n = 0;
                        int color = 0;
                        in >> tag >> f.id >> color >> n;
                        if (tag != "f") return false;
                        f.color = uint8_t(color & 15);
                        f.vertices.resize(n);
                        for (size_t j = 0; j < n; ++j) in >> f.vertices[j];
                        obj.mesh.faces.push_back(f);
                    }
                    in >> tag;
                    if (tag != "END_OBJECT") return false;
                    loaded.objects.push_back(obj);
                }
            }
        }
        Snapshot before = makeSnapshot();
        scene = loaded;
        selection.clear();
        activeObjectId = 0;
        currentFile = path;
        pushUndo(before);
        frameAll();
        status = "LOADED";
        return true;
    }

    void saveCommand(bool saveAs) {
        std::filesystem::path path = currentFile;
        if (path.empty() || saveAs) {
            static const wchar_t filter[] = L"PixelForge 16 (*.pf16)\0*.pf16\0All Files (*.*)\0*.*\0";
            path = fileDialog(true, filter, L"pf16");
        }
        if (path.empty()) {
            status = "SAVE CANCEL";
            return;
        }
        if (savePf16(path)) {
            currentFile = path;
            status = "SAVED";
        } else {
            status = "SAVE FAILED";
        }
    }

    void openCommand() {
        static const wchar_t filter[] = L"PixelForge 16 (*.pf16)\0*.pf16\0All Files (*.*)\0*.*\0";
        std::filesystem::path path = fileDialog(false, filter, L"pf16");
        if (path.empty()) {
            status = "OPEN CANCEL";
            return;
        }
        if (!loadPf16(path)) status = "OPEN FAILED";
    }

    bool exportObj(const std::filesystem::path &path) {
        std::ofstream objOut(path, std::ios::binary);
        if (!objOut) return false;
        std::filesystem::path p(path);
        std::string mtlName = "PixelForge16.mtl";
        objOut << "mtllib " << mtlName << "\n";
        objOut << "o PixelForge16\n";
        int index = 1;
        std::map<std::pair<uint32_t, uint32_t>, int> vertexIndex;
        for (const auto &obj : scene.objects) {
            for (const auto &v : obj.mesh.vertices) {
                Vec3 w = transformPoint(v.position, obj.transform);
                objOut << "v " << w.x << " " << w.y << " " << w.z << "\n";
                vertexIndex[{obj.id, v.id}] = index++;
            }
        }
        int currentMat = -1;
        for (const auto &obj : scene.objects) {
            objOut << "g " << obj.name << "\n";
            for (const auto &f : obj.mesh.faces) {
                if (currentMat != int(f.color)) {
                    currentMat = int(f.color);
                    objOut << "usemtl PF16_" << currentMat << "_" << kPaletteNames[f.color] << "\n";
                }
                objOut << "f";
                for (uint32_t vid : f.vertices) objOut << " " << vertexIndex[{obj.id, vid}];
                objOut << "\n";
            }
        }
        std::ofstream mtlOut(p.parent_path() / mtlName, std::ios::binary);
        if (!mtlOut) return false;
        for (int i = 0; i < 16; ++i) {
            Rgb c = kPalette[i];
            mtlOut << "newmtl PF16_" << i << "_" << kPaletteNames[i] << "\n";
            mtlOut << "Kd " << (c.r / 255.0f) << " " << (c.g / 255.0f) << " " << (c.b / 255.0f) << "\n";
            mtlOut << "Ka 0 0 0\nKs 0 0 0\n\n";
        }
        return true;
    }

    void exportObjCommand() {
        static const wchar_t filter[] = L"Wavefront OBJ (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
        std::filesystem::path path = fileDialog(true, filter, L"obj");
        if (path.empty()) {
            status = "EXPORT CANCEL";
            return;
        }
        status = exportObj(path) ? "OBJ EXPORTED" : "OBJ EXPORT FAILED";
    }

    bool importObj(const std::filesystem::path &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        Snapshot before = makeSnapshot();
        Object3D &obj = addObject("OBJ");
        std::vector<uint32_t> ids;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            std::string tag;
            ss >> tag;
            if (tag == "v") {
                Vec3 p;
                ss >> p.x >> p.y >> p.z;
                ids.push_back(addVertex(obj, p));
            } else if (tag == "f") {
                std::vector<uint32_t> verts;
                std::string token;
                while (ss >> token) {
                    size_t slash = token.find('/');
                    std::string raw = slash == std::string::npos ? token : token.substr(0, slash);
                    if (raw.empty()) continue;
                    int idx = std::stoi(raw);
                    if (idx < 0) idx = int(ids.size()) + idx + 1;
                    if (idx >= 1 && idx <= int(ids.size())) verts.push_back(ids[size_t(idx - 1)]);
                }
                if (verts.size() >= 3) addFace(obj, verts, scene.currentColor);
            }
        }
        if (obj.mesh.vertices.empty()) {
            scene.objects.pop_back();
            return false;
        }
        clearSelection();
        addSelection({ElementType::Object, obj.id, obj.id});
        pushUndo(before);
        frameAll();
        status = "OBJ IMPORTED";
        return true;
    }

    void importObjCommand() {
        static const wchar_t filter[] = L"Wavefront OBJ (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
        std::filesystem::path path = fileDialog(false, filter, L"obj");
        if (path.empty()) {
            status = "IMPORT CANCEL";
            return;
        }
        if (!importObj(path)) status = "OBJ IMPORT FAILED";
    }

    bool writeBmp(const std::filesystem::path &path, int w, int h, const std::vector<uint32_t> &pixels) {
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        uint32_t fileHeaderSize = 14;
        uint32_t dibHeaderSize = 40;
        uint32_t dataSize = uint32_t(w * h * 4);
        uint32_t fileSize = fileHeaderSize + dibHeaderSize + dataSize;
        uint8_t fileHeader[14] = {
            'B','M',
            uint8_t(fileSize), uint8_t(fileSize >> 8), uint8_t(fileSize >> 16), uint8_t(fileSize >> 24),
            0,0,0,0,
            uint8_t(fileHeaderSize + dibHeaderSize),0,0,0
        };
        uint8_t dib[40] {};
        auto put32 = [&](int off, uint32_t v) {
            dib[off] = uint8_t(v);
            dib[off + 1] = uint8_t(v >> 8);
            dib[off + 2] = uint8_t(v >> 16);
            dib[off + 3] = uint8_t(v >> 24);
        };
        auto put16 = [&](int off, uint16_t v) {
            dib[off] = uint8_t(v);
            dib[off + 1] = uint8_t(v >> 8);
        };
        put32(0, dibHeaderSize);
        put32(4, uint32_t(w));
        put32(8, uint32_t(h));
        put16(12, 1);
        put16(14, 32);
        put32(20, dataSize);
        out.write(reinterpret_cast<const char *>(fileHeader), sizeof(fileHeader));
        out.write(reinterpret_cast<const char *>(dib), sizeof(dib));
        for (int y = h - 1; y >= 0; --y) {
            out.write(reinterpret_cast<const char *>(&pixels[size_t(y) * w]), w * 4);
        }
        return true;
    }

    void screenshot() {
        render();
        IRect src {};
        for (const auto &vd : layoutViewports()) {
            if (vd.state->kind == ViewKind::Perspective) {
                src = vd.rect;
                break;
            }
        }
        if (src.w == 0 || src.h == 0) src = {0, 14, kLogicalW, kLogicalH - 52};
        std::vector<uint32_t> out(size_t(kLogicalW) * size_t(kLogicalH), packColor(15));
        for (int y = 0; y < kLogicalH; ++y) {
            for (int x = 0; x < kLogicalW; ++x) {
                int sx = src.x + x * src.w / kLogicalW;
                int sy = src.y + y * src.h / kLogicalH;
                out[size_t(y) * kLogicalW + x] = fb.pixels[size_t(sy) * kLogicalW + sx];
            }
        }
        auto dir = ensureDir(executableDir() / "screenshots");
        SYSTEMTIME t {};
        GetLocalTime(&t);
        char name[128];
        std::snprintf(name, sizeof(name), "pf16_%04d%02d%02d_%02d%02d%02d.bmp",
                      t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
        if (writeBmp(dir / name, kLogicalW, kLogicalH, out)) {
            status = std::string("SCREENSHOT ") + name;
        } else {
            status = "SCREENSHOT FAILED";
        }
    }

    void openMenu(const std::string &name) {
        menu.open = true;
        menu.name = name;
        int x = 126;
        for (const auto &[id, rect] : menuBarRects()) {
            if (id == name) {
                x = rect.x;
                break;
            }
        }
        int minWidth = 104;
        if (name == "FILE") {
            menu.items = {"NEW", "OPEN", "SAVE", "SAVE AS", "IMPORT OBJ", "EXPORT OBJ", "EXPORT IMAGE", "EXIT"};
            minWidth = 122;
        } else if (name == "EDIT") {
            menu.items = {"UNDO", "REDO", "COPY", "PASTE", "DUPLICATE", "DELETE"};
            minWidth = 104;
        } else if (name == "ADD") {
            menu.items = {"VERTEX", "PLANE", "CUBE", "PYRAMID", "PRISM", "CYLINDER", "SPHERE"};
            minWidth = 94;
        } else if (name == "VIEW") {
            menu.items = {"GRID", "TOGGLE SNAP", "WIREFRAME", "FLAT", "FLAT+WIRE", "LIGHT", "FRAME ALL", "FRAME SELECTED", "RESET VIEW", "RESET ALL VIEWS"};
            minWidth = 126;
        } else if (name == "LANG") {
            menu.items = {"LANG EN", "LANG KO", "LANG JA"};
            minWidth = 104;
        } else {
            menu.items = {"PIXELFORGE 16", "F12 SCREENSHOT", "README"};
            minWidth = 112;
        }
        placePopup(x, 14, minWidth);
    }

    bool clickMenuBar(int x, int y) {
        if (y >= 14) return false;
        for (const auto &[id, rect] : menuBarRects()) {
            if (rect.contains(x, y)) {
                if (menu.open && menu.name == id) menu.open = false;
                else openMenu(id);
                return true;
            }
        }
        return false;
    }

    bool clickMenuPopup(int x, int y) {
        if (!menu.open || !menu.rect.contains(x, y)) return false;
        int index = (y - menu.rect.y - 1) / menuItemHeight();
        if (index < 0 || index >= int(menu.items.size())) return true;
        std::string item = menu.items[size_t(index)];
        menu.open = false;
        dispatchMenu(item);
        return true;
    }

    void dispatchMenu(const std::string &item) {
        if (item == "NEW") newScene();
        else if (item == "OPEN") openCommand();
        else if (item == "SAVE") saveCommand(false);
        else if (item == "SAVE AS") saveCommand(true);
        else if (item == "IMPORT OBJ") importObjCommand();
        else if (item == "EXPORT OBJ") exportObjCommand();
        else if (item == "EXPORT IMAGE") screenshot();
        else if (item == "EXIT") PostMessage(hwnd, WM_CLOSE, 0, 0);
        else if (item == "UNDO") undo();
        else if (item == "REDO") redo();
        else if (item == "COPY") copySelection();
        else if (item == "PASTE") pasteClipboard();
        else if (item == "DUPLICATE") duplicateSelection();
        else if (item == "DELETE") deleteSelection();
        else if (item == "EXTRUDE") extrude();
        else if (item == "INSET") inset();
        else if (item == "FLIP" || item == "FLIP FACE") flipFace();
        else if (item == "SNAP TO GRID") snapSelectedVertices();
        else if (item == "MOVE") status = "DRAG VERTEX DIRECTLY";
        else if (item == "MOVE Z") beginVertexZMove(mouseX, mouseY);
        else if (item == "COLOR") status = "CLICK PALETTE COLOR";
        else if (item == "ADD") openMenu("ADD");
        else if (item == "VERTEX") addSingleVertex();
        else if (item == "PLANE") addPlane();
        else if (item == "CUBE") addCube();
        else if (item == "PYRAMID") addPyramid();
        else if (item == "PRISM") addPrism();
        else if (item == "CYLINDER") addCylinder();
        else if (item == "SPHERE") addLowPolySphere();
        else if (item == "GRID") { scene.grid = !scene.grid; status = scene.grid ? "GRID ON" : "GRID OFF"; }
        else if (item == "TOGGLE SNAP") { scene.snap = !scene.snap; status = scene.snap ? "SNAP ON" : "SNAP OFF"; }
        else if (item == "WIREFRAME") { scene.renderMode = RenderMode::Wireframe; status = "WIREFRAME"; }
        else if (item == "FLAT") { scene.renderMode = RenderMode::Flat; status = "FLAT"; }
        else if (item == "FLAT+WIRE") { scene.renderMode = RenderMode::FlatWire; status = "FLAT+WIRE"; }
        else if (item == "PAN") status = "PAN VIEW";
        else if (item == "FRAME ALL") frameAll();
        else if (item == "FRAME SELECTED") focusSelection();
        else if (item == "RESET VIEW") resetActiveView();
        else if (item == "RESET VIEWS" || item == "RESET ALL VIEWS") resetViews();
        else if (item == "LANG EN") setLanguage(Locale::English);
        else if (item == "LANG KO") setLanguage(Locale::Korean);
        else if (item == "LANG JA") setLanguage(Locale::Japanese);
        else if (item == "README") {
            std::filesystem::path readme = executableDir() / L"README.md";
            ShellExecuteW(hwnd, L"open", readme.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    void onMouseMove(int x, int y, WPARAM keys) {
        (void)keys;
        mouseX = x;
        mouseY = y;
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (drag.mode == DragMode::VertexZMove) {
            updateVertexZMove(x, y, ctrl);
            return;
        }
        if (drag.mode == DragMode::ObjectDrag) {
            updateObjectDrag(x, y, ctrl);
            return;
        }
        if (drag.mode == DragMode::VertexDrag) {
            updateVertexDrag(x, y, ctrl);
            return;
        }
        if (drag.mode == DragMode::BoxSelect) {
            drag.lastX = x;
            drag.lastY = y;
            drag.changed = true;
            return;
        }
        if (drag.mode == DragMode::Orbit || drag.mode == DragMode::Pan) {
            updateCameraDrag(x, y);
            return;
        }
        hover = pick(x, y);
        if (hover.type != ElementType::None) activeView = hover.view;
        auto vd = viewportAt(x, y);
        if (vd) activeView = vd->state->kind;
    }

    void onLeftDown(int x, int y) {
        mouseX = x;
        mouseY = y;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if (drag.mode == DragMode::VertexZMove) {
            captureMouse();
            updateVertexZMove(x, y, ctrl);
            return;
        }
        if (clickMenuPopup(x, y) || clickMenuBar(x, y)) return;

        int pal = paletteIndexAt(x, y);
        if (pal >= 0) {
            applyColorToSelection(uint8_t(pal));
            return;
        }

        auto vd = viewportAt(x, y);
        if (!vd) {
            menu.open = false;
            return;
        }
        activeView = vd->state->kind;
        hover = pick(x, y);
        menu.open = false;
        if (alt && hover.objectId != 0) {
            selectObjectFromHover(shift, ctrl);
            if (isObjectSelected(hover.objectId)) beginObjectDrag(x, y, vd->state->kind);
        } else if (hover.type == ElementType::Vertex) {
            ElementRef hit {ElementType::Vertex, hover.objectId, hover.id};
            if (isSelected(hit.type, hit.objectId, hit.id)) {
                beginVertexDrag(x, y, vd->state->kind, shift, ctrl);
            } else if (hoverOnDraggableObject() && !shift && !ctrl) {
                beginObjectDrag(x, y, vd->state->kind);
            } else {
                beginVertexDrag(x, y, vd->state->kind, shift, ctrl);
            }
        } else if (hoverOnDraggableObject() && !shift && !ctrl) {
            beginObjectDrag(x, y, vd->state->kind);
        } else if (hover.type == ElementType::Face) {
            selectFromHover(shift, ctrl);
        } else {
            beginBoxSelect(x, y, vd->state->kind);
        }
    }

    void onLeftUp(int x, int y) {
        mouseX = x;
        mouseY = y;
        if (drag.mode == DragMode::BoxSelect) {
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            drag.lastX = x;
            drag.lastY = y;
            if (std::abs(drag.startX - drag.lastX) < 3 && std::abs(drag.startY - drag.lastY) < 3) {
                clearSelection();
                activeObjectId = 0;
                status = "SELECT EMPTY";
            } else {
                finishBoxSelect(shift, ctrl);
            }
            drag = DragState {};
            releaseMouseCapture();
            refreshHoverUnderMouse();
            return;
        }
        if (drag.mode != DragMode::None) {
            finishDrag();
        }
    }

    void onMiddleDown(int x, int y) {
        auto vd = viewportAt(x, y);
        if (!vd) return;
        activeView = vd->state->kind;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool pan = shift || vd->state->kind != ViewKind::Perspective;
        beginCameraDrag(x, y, vd->state->kind, pan);
    }

    void onCaptureChanged(HWND newCapture) {
        if (!mouseCaptured || newCapture == hwnd) return;
        mouseCaptured = false;
        if (drag.mode != DragMode::None) cancelDrag();
    }

    void onFocusLost() {
        if (drag.mode != DragMode::None) cancelDrag();
        else releaseMouseCapture();
    }

    void onMouseWheel(int x, int y, int delta) {
        POINT pt {x, y};
        ScreenToClient(hwnd, &pt);
        auto logical = screenToLogical(pt.x, pt.y);
        auto vd = viewportAt(logical.first, logical.second);
        if (!vd) return;
        ViewState *v = vd->state;
        activeView = v->kind;
        float step = delta > 0 ? 0.9f : 1.1f;
        if (v->kind == ViewKind::Perspective) {
            v->distance = clampf(v->distance * step, 1.0f, 200.0f);
        } else {
            v->zoom = clampf(v->zoom * (delta > 0 ? 1.15f : 0.87f), 2.0f, 160.0f);
        }
        status = "ZOOM";
    }

    void onRightDown(int x, int y) {
        hover = pick(x, y);
        if (hover.type == ElementType::Face) {
            menu.open = true;
            menu.name = "FACE";
            menu.items = {"EXTRUDE", "INSET", "COLOR", "FLIP FACE", "DUPLICATE", "DELETE"};
            placePopup(x, y, 104);
        } else if (hover.type == ElementType::Vertex) {
            menu.open = true;
            menu.name = "VERTEX";
            menu.items = {"MOVE", "MOVE Z", "MERGE", "DUPLICATE", "SNAP TO GRID", "DELETE"};
            placePopup(x, y, 106);
        } else if (hover.type == ElementType::Edge) {
            menu.open = true;
            menu.name = "EDGE";
            menu.items = {"EXTRUDE", "DUPLICATE", "DELETE"};
            placePopup(x, y, 96);
        } else {
            menu.open = true;
            menu.name = "EMPTY";
            menu.items = {"PAN", "FRAME ALL", "FRAME SELECTED", "RESET VIEW", "RESET ALL VIEWS", "ADD", "PASTE", "GRID"};
            placePopup(x, y, 128);
        }
    }

    void onKeyDown(WPARAM key) {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if (drag.mode == DragMode::VertexZMove) {
            if (key == VK_ESCAPE) cancelDrag();
            else if (key == VK_RETURN) commitVertexZMove();
            else if (key == VK_BACK && !drag.typedValue.empty()) {
                drag.typedValue.pop_back();
                status = drag.typedValue.empty() ? "MOVE Z | DRAG OR TYPE VALUE | ENTER APPLY | ESC CANCEL"
                                                 : "Z: " + drag.typedValue;
            }
            return;
        }
        if (key == VK_ESCAPE) {
            if (drag.mode != DragMode::None) cancelDrag();
            else { menu.open = false; status = "ESC"; }
            return;
        }
        if (ctrl && key == 'Z' && shift) { redo(); return; }
        if (ctrl && key == 'Z') { undo(); return; }
        if (ctrl && key == 'Y') { redo(); return; }
        if (ctrl && key == 'N') { newScene(); return; }
        if (ctrl && key == 'S' && shift) { saveCommand(true); return; }
        if (ctrl && key == 'S') { saveCommand(false); return; }
        if (ctrl && key == 'O') { openCommand(); return; }
        if (ctrl && key == 'C') { copySelection(); return; }
        if (ctrl && key == 'V') { pasteClipboard(); return; }
        if (ctrl && key == VK_HOME) { resetViews(); return; }
        if (key == 'Z') { beginVertexZMove(mouseX, mouseY); return; }

        if (key == VK_SPACE) {
            if (maximizedView.has_value()) maximizedView.reset();
            else maximizedView = activeView;
            status = maximizedView.has_value() ? "MAXIMIZE VIEW" : "4 VIEWPORTS";
            return;
        }
        if (key == VK_TAB) {
            openMenu("ADD");
            status = "ADD MENU";
            return;
        }
        if (key == VK_HOME) { frameAll(); return; }
        if (key == VK_F12) { screenshot(); return; }
        if (key == 'E') { extrude(); return; }
        if (key == 'I') { inset(); return; }
        if (key == 'F') {
            if (selectedVertices().size() >= 3) createFaceFromSelection();
            else focusSelection();
            return;
        }
        if (key == 'D') { duplicateSelection(); return; }
        if (key == 'R') { rotateSelection(shift ? -15.0f : 15.0f); return; }
        if (key == 'S') { scaleSelection(shift ? 0.9f : 1.1f); return; }
        if (key == 'M') {
            if (selectedVertices().size() >= 2) mergeSelectedVertices();
            else cycleRenderMode();
            return;
        }
        if (key == 'X' || key == VK_DELETE) { deleteSelection(); return; }
        if (key == 'A' && shift) { openMenu("ADD"); return; }
        if (key >= '0' && key <= '9') {
            applyColorToSelection(uint8_t(key - '0'));
            return;
        }
        if (key == VK_UP || key == 'W') { nudgeSelection(viewDelta(activeView, 0, -1), shift, alt); return; }
        if (key == VK_DOWN || key == 'S') { nudgeSelection(viewDelta(activeView, 0, 1), shift, alt); return; }
        if (key == VK_LEFT || key == 'A') { nudgeSelection(viewDelta(activeView, -1, 0), shift, alt); return; }
        if (key == VK_RIGHT || key == 'D') { nudgeSelection(viewDelta(activeView, 1, 0), shift, alt); return; }
        if (key == VK_PRIOR) { nudgeSelection({0.0f, 0.0f, 1.0f}, shift, alt); return; }
        if (key == VK_NEXT) { nudgeSelection({0.0f, 0.0f, -1.0f}, shift, alt); return; }
    }

    void onChar(WPARAM ch) {
        if (drag.mode != DragMode::VertexZMove) return;
        wchar_t wc = wchar_t(ch);
        if ((wc >= L'0' && wc <= L'9') || wc == L'-' || wc == L'+' || wc == L'.') {
            if ((wc == L'-' || wc == L'+') && !drag.typedValue.empty()) return;
            if (wc == L'.' && drag.typedValue.find('.') != std::string::npos) return;
            if (drag.typedValue.size() < 16) drag.typedValue.push_back(char(wc));
            status = "Z: " + drag.typedValue;
        } else if (wc == L'\r') {
            commitVertexZMove();
        }
    }

    std::pair<int, int> screenToLogical(int sx, int sy) const {
        if (scale <= 0) return {0, 0};
        int lx = (sx - offsetX) / scale;
        int ly = (sy - offsetY) / scale;
        return {clampi(lx, 0, kLogicalW - 1), clampi(ly, 0, kLogicalH - 1)};
    }

    void updateScale(int w, int h) {
        clientW = std::max(1, w);
        clientH = std::max(1, h);
        scale = std::max(1, std::min(clientW / kLogicalW, clientH / kLogicalH));
        offsetX = (clientW - kLogicalW * scale) / 2;
        offsetY = (clientH - kLogicalH * scale) / 2;
    }

    void paint(HDC hdc) {
        render();
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        RECT rc {0, 0, clientW, clientH};
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchDIBits(hdc, offsetX, offsetY, kLogicalW * scale, kLogicalH * scale,
                      0, 0, kLogicalW, kLogicalH, fb.pixels.data(), &fb.bmi, DIB_RGB_COLORS, SRCCOPY);
    }

    void saveOnExitPrompt() {
        int r = MessageBoxW(hwnd, L"SAVE CHANGES?", L"PIXELFORGE 16", MB_YESNOCANCEL | MB_ICONQUESTION);
        if (r == IDYES) saveCommand(false);
    }
};

static int runSelfTest() {
    App app;
    if (app.scene.objects.empty()) return 10;
    Object3D &obj = app.scene.objects.front();
    if (obj.mesh.vertices.size() != 8 || obj.mesh.faces.size() != 6) return 11;

    app.clearSelection();
    app.addSelection({ElementType::Face, obj.id, obj.mesh.faces.front().id});
    app.extrude();
    if (app.scene.objects.front().mesh.faces.size() <= 6) return 12;
    app.inset();
    if (app.undoStack.empty()) return 13;

    app.clearSelection();
    Object3D &zObj = app.scene.objects.front();
    if (zObj.mesh.vertices.size() < 2) return 22;
    uint32_t zV0 = zObj.mesh.vertices[0].id;
    uint32_t zV1 = zObj.mesh.vertices[1].id;
    float beforeZ0 = zObj.mesh.vertices[0].position.z;
    float beforeZ1 = zObj.mesh.vertices[1].position.z;
    app.addSelection({ElementType::Vertex, zObj.id, zV0});
    app.addSelection({ElementType::Vertex, zObj.id, zV1});
    app.beginVertexZMove(100, 100);
    app.drag.typedValue = "10";
    app.commitVertexZMove();
    const Vertex *afterZ0 = findVertex(app.scene.objects.front(), zV0);
    const Vertex *afterZ1 = findVertex(app.scene.objects.front(), zV1);
    if (!afterZ0 || !afterZ1) return 23;
    if (std::fabs(afterZ0->position.z - (beforeZ0 + 10.0f)) > 0.01f) return 24;
    if (std::fabs(afterZ1->position.z - (beforeZ1 + 10.0f)) > 0.01f) return 25;
    app.undo();
    afterZ0 = findVertex(app.scene.objects.front(), zV0);
    afterZ1 = findVertex(app.scene.objects.front(), zV1);
    if (!afterZ0 || !afterZ1) return 26;
    if (std::fabs(afterZ0->position.z - beforeZ0) > 0.01f) return 27;
    if (std::fabs(afterZ1->position.z - beforeZ1) > 0.01f) return 28;

    app.scene.snap = false;
    uint32_t moveObjectId = app.scene.objects.front().id;
    Object3D *moveObj = findObject(app.scene, moveObjectId);
    if (!moveObj || moveObj->mesh.vertices.empty()) return 33;
    Vec3 localBefore = moveObj->mesh.vertices.front().position;
    Vec3 objectBefore = moveObj->transform.position;
    app.clearSelection();
    app.addSelection({ElementType::Object, moveObjectId, moveObjectId});
    app.beginObjectDrag(100, 100, ViewKind::Top);
    app.updateObjectDrag(136, 82, false);
    app.finishDrag();
    moveObj = findObject(app.scene, moveObjectId);
    if (!moveObj) return 34;
    if (std::fabs(moveObj->mesh.vertices.front().position.x - localBefore.x) > 0.001f) return 35;
    if (std::fabs(moveObj->transform.position.x - (objectBefore.x + 2.0f)) > 0.01f) return 36;
    if (std::fabs(moveObj->transform.position.z - (objectBefore.z + 1.0f)) > 0.01f) return 37;
    app.undo();
    moveObj = findObject(app.scene, moveObjectId);
    if (!moveObj) return 38;
    if (std::fabs(moveObj->transform.position.x - objectBefore.x) > 0.01f) return 39;
    app.redo();
    moveObj = findObject(app.scene, moveObjectId);
    if (!moveObj) return 40;
    if (std::fabs(moveObj->transform.position.x - (objectBefore.x + 2.0f)) > 0.01f) return 41;

    Vec3 frontDelta = app.objectDragDelta(ViewKind::Front, 18, -18);
    Vec3 sideDelta = app.objectDragDelta(ViewKind::Side, 18, -18);
    if (std::fabs(frontDelta.x - 1.0f) > 0.01f || std::fabs(frontDelta.y - 1.0f) > 0.01f || std::fabs(frontDelta.z) > 0.01f) return 42;
    if (std::fabs(sideDelta.z - 1.0f) > 0.01f || std::fabs(sideDelta.y - 1.0f) > 0.01f || std::fabs(sideDelta.x) > 0.01f) return 43;
    Vec3 perspectiveDelta = app.objectDragDelta(ViewKind::Perspective, 20, -12);
    if (length(perspectiveDelta) <= 0.001f) return 44;

    app.addCube(false);
    uint32_t secondObjectId = app.scene.objects.back().id;
    Vec3 firstBeforeMulti = findObject(app.scene, moveObjectId)->transform.position;
    Vec3 secondBeforeMulti = app.scene.objects.back().transform.position;
    app.clearSelection();
    app.addSelection({ElementType::Object, moveObjectId, moveObjectId});
    app.addSelection({ElementType::Object, secondObjectId, secondObjectId});
    app.beginObjectDrag(50, 50, ViewKind::Front);
    app.updateObjectDrag(68, 50, false);
    app.finishDrag();
    Object3D *firstMulti = findObject(app.scene, moveObjectId);
    Object3D *secondMulti = findObject(app.scene, secondObjectId);
    if (!firstMulti || !secondMulti) return 45;
    if (std::fabs(firstMulti->transform.position.x - (firstBeforeMulti.x + 1.0f)) > 0.01f) return 46;
    if (std::fabs(secondMulti->transform.position.x - (secondBeforeMulti.x + 1.0f)) > 0.01f) return 47;

    ViewState *topView = app.stateFor(ViewKind::Top);
    ViewState *frontView = app.stateFor(ViewKind::Front);
    ViewState *sideView = app.stateFor(ViewKind::Side);
    ViewState *perspectiveView = app.stateFor(ViewKind::Perspective);
    auto viewportFor = [&](ViewKind kind) {
        auto vds = app.layoutViewports();
        for (const auto &vd : vds) {
            if (vd.state->kind == kind) return vd;
        }
        return vds.front();
    };
    auto originProjection = [&](ViewKind kind) {
        ViewportDraw vd = viewportFor(kind);
        return app.projectPoint(vd, {0, 0, 0});
    };
    auto dragObjectByInput = [&](uint32_t objectId, ViewKind kind, int dx, int dy, int code) -> int {
        Object3D *target = findObject(app.scene, objectId);
        if (!target) return code;
        ViewState *state = app.stateFor(kind);
        state->center = target->transform.position;
        ViewportDraw dragVd = viewportFor(kind);
        Projected hit = app.projectPoint(dragVd, target->transform.position);
        if (!hit.ok || !dragVd.rect.contains(hit.x, hit.y)) return code + 1;
        app.onLeftDown(hit.x, hit.y);
        if (app.drag.mode != DragMode::ObjectDrag) return code + 2;
        app.onMouseMove(hit.x + dx, hit.y + dy, 0);
        app.onLeftUp(hit.x + dx, hit.y + dy);
        if (app.drag.mode != DragMode::None) return code + 3;
        return 0;
    };
    app.clearSelection();
    app.addSelection({ElementType::Object, moveObjectId, moveObjectId});
    Object3D *repeatedObj = findObject(app.scene, moveObjectId);
    if (!repeatedObj) return 71;
    float repeatedStartX = repeatedObj->transform.position.x;
    float repeatedStep = 18.0f / std::max(1.0f, app.stateFor(ViewKind::Top)->zoom);
    for (int i = 0; i < 50; ++i) {
        repeatedObj = findObject(app.scene, moveObjectId);
        if (!repeatedObj) return 72;
        int dragResult = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 73);
        if (dragResult != 0) return dragResult;
        if (!app.isObjectSelected(moveObjectId)) return 76;
    }
    repeatedObj = findObject(app.scene, moveObjectId);
    if (!repeatedObj) return 77;
    if (std::fabs(repeatedObj->transform.position.x - (repeatedStartX + repeatedStep * 50.0f)) > 0.01f) return 78;

    repeatedObj = findObject(app.scene, moveObjectId);
    if (!repeatedObj || repeatedObj->mesh.vertices.empty()) return 80;
    uint32_t vertexMoveId = repeatedObj->mesh.vertices.front().id;
    app.clearSelection();
    app.addSelection({ElementType::Vertex, moveObjectId, vertexMoveId});
    app.stateFor(ViewKind::Top)->center = repeatedObj->transform.position;
    ViewportDraw vertexVd = viewportFor(ViewKind::Top);
    Projected vertexHit = app.projectPoint(vertexVd, transformPoint(repeatedObj->mesh.vertices.front().position, repeatedObj->transform));
    if (!vertexHit.ok) return 81;
    app.onLeftDown(vertexHit.x, vertexHit.y);
    if (app.drag.mode != DragMode::VertexDrag) return 82;
    app.onMouseMove(vertexHit.x + 18, vertexHit.y, 0);
    app.onLeftUp(vertexHit.x + 18, vertexHit.y);
    if (app.drag.mode != DragMode::None) return 83;
    Vec3 afterVertexObjectPos = findObject(app.scene, moveObjectId)->transform.position;
    int vertexThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 84);
    if (vertexThenDrag != 0) return vertexThenDrag;
    if (findObject(app.scene, moveObjectId)->transform.position.x <= afterVertexObjectPos.x) return 88;

    Object3D *editObj = findObject(app.scene, moveObjectId);
    if (!editObj || editObj->mesh.faces.empty()) return 89;
    app.clearSelection();
    app.addSelection({ElementType::Face, moveObjectId, editObj->mesh.faces.front().id});
    app.extrude();
    int extrudeThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 90);
    if (extrudeThenDrag != 0) return extrudeThenDrag;

    app.inset();
    int insetThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 95);
    if (insetThenDrag != 0) return insetThenDrag;

    editObj = findObject(app.scene, moveObjectId);
    if (!editObj || editObj->mesh.faces.empty()) return 99;
    app.clearSelection();
    app.addSelection({ElementType::Face, moveObjectId, editObj->mesh.faces.front().id});
    app.applyColorToSelection(LIGHT_RED);
    int colorThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 100);
    if (colorThenDrag != 0) return colorThenDrag;

    Vec3 beforeUndoDrag = findObject(app.scene, moveObjectId)->transform.position;
    int undoSetupDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 104);
    if (undoSetupDrag != 0) return undoSetupDrag;
    app.undo();
    if (std::fabs(findObject(app.scene, moveObjectId)->transform.position.x - beforeUndoDrag.x) > 0.01f) return 108;
    int undoThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 109);
    if (undoThenDrag != 0) return undoThenDrag;

    std::filesystem::path dragTestDir = ensureDir(executableDir() / "selftest");
    if (!app.savePf16(dragTestDir / "drag_cycle_scene.pf16")) return 113;
    int saveThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 114);
    if (saveThenDrag != 0) return saveThenDrag;

    editObj = findObject(app.scene, moveObjectId);
    if (!editObj || editObj->mesh.vertices.size() < 2) return 118;
    app.clearSelection();
    app.addSelection({ElementType::Vertex, moveObjectId, editObj->mesh.vertices[0].id});
    app.addSelection({ElementType::Vertex, moveObjectId, editObj->mesh.vertices[1].id});
    app.scaleSelection(1.1f);
    int scaleThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 119);
    if (scaleThenDrag != 0) return scaleThenDrag;

    editObj = findObject(app.scene, moveObjectId);
    if (!editObj || editObj->mesh.vertices.size() < 2) return 123;
    app.clearSelection();
    app.addSelection({ElementType::Vertex, moveObjectId, editObj->mesh.vertices[0].id});
    app.addSelection({ElementType::Vertex, moveObjectId, editObj->mesh.vertices[1].id});
    app.mergeSelectedVertices();
    int mergeThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 124);
    if (mergeThenDrag != 0) return mergeThenDrag;

    editObj = findObject(app.scene, moveObjectId);
    if (!editObj || editObj->mesh.faces.empty()) return 128;
    app.clearSelection();
    app.addSelection({ElementType::Face, moveObjectId, editObj->mesh.faces.front().id});
    app.deleteSelection();
    int deleteThenDrag = dragObjectByInput(moveObjectId, ViewKind::Top, 18, 0, 129);
    if (deleteThenDrag != 0) return deleteThenDrag;

    std::array<uint32_t, 4> alternatingObjects {{moveObjectId, secondObjectId, moveObjectId, secondObjectId}};
    for (size_t i = 0; i < alternatingObjects.size(); ++i) {
        app.clearSelection();
        app.addSelection({ElementType::Object, alternatingObjects[i], alternatingObjects[i]});
        int alternateDrag = dragObjectByInput(alternatingObjects[i], i % 2 == 0 ? ViewKind::Front : ViewKind::Side, 18, 0, int(133 + i * 4));
        if (alternateDrag != 0) return alternateDrag;
    }

    firstMulti = findObject(app.scene, moveObjectId);
    secondMulti = findObject(app.scene, secondObjectId);
    if (!firstMulti || !secondMulti) return 149;

    topView->center = {0, 0, 0};
    frontView->center = {0, 0, 0};
    sideView->center = {0, 0, 0};
    perspectiveView->center = {0, 0, 0};
    ViewportDraw topVdBefore = viewportFor(ViewKind::Top);
    Projected topOriginBefore = app.projectPoint(topVdBefore, {0, 0, 0});
    if (!topOriginBefore.ok) return 55;
    if (std::abs(topOriginBefore.x - (topVdBefore.rect.x + topVdBefore.rect.w / 2)) > 1 ||
        std::abs(topOriginBefore.y - (topVdBefore.rect.y + topVdBefore.rect.h / 2)) > 1) return 56;
    Vec3 topCenterBefore = topView->center;
    Vec3 frontCenterBefore = frontView->center;
    Vec3 objectPosBeforePan = firstMulti->transform.position;
    app.beginCameraDrag(100, 100, ViewKind::Top, true);
    app.updateCameraDrag(118, 82);
    app.finishDrag();
    ViewportDraw topVdAfter = viewportFor(ViewKind::Top);
    Projected topOriginAfter = app.projectPoint(topVdAfter, {0, 0, 0});
    if (!topOriginAfter.ok) return 57;
    if (std::abs(topOriginAfter.x - (topVdAfter.rect.x + topVdAfter.rect.w / 2)) <= 1 &&
        std::abs(topOriginAfter.y - (topVdAfter.rect.y + topVdAfter.rect.h / 2)) <= 1) return 58;
    if (std::abs(topOriginAfter.x - topOriginBefore.x) <= 1 &&
        std::abs(topOriginAfter.y - topOriginBefore.y) <= 1) return 59;
    if (topOriginAfter.x <= topOriginBefore.x || topOriginAfter.y >= topOriginBefore.y) return 66;
    if (std::fabs(firstMulti->transform.position.x - objectPosBeforePan.x) > 0.001f) return 48;
    if (std::fabs(topView->center.x - topCenterBefore.x) <= 0.001f && std::fabs(topView->center.z - topCenterBefore.z) <= 0.001f) return 49;
    if (std::fabs(frontView->center.x - frontCenterBefore.x) > 0.001f ||
        std::fabs(frontView->center.y - frontCenterBefore.y) > 0.001f ||
        std::fabs(frontView->center.z - frontCenterBefore.z) > 0.001f) return 50;

    Projected frontOriginBefore = originProjection(ViewKind::Front);
    app.beginCameraDrag(100, 100, ViewKind::Front, true);
    app.updateCameraDrag(100, 80);
    app.finishDrag();
    Projected frontOriginAfter = originProjection(ViewKind::Front);
    if (!frontOriginBefore.ok || !frontOriginAfter.ok) return 60;
    if (std::abs(frontOriginAfter.y - frontOriginBefore.y) <= 1) return 61;
    if (frontOriginAfter.y >= frontOriginBefore.y) return 67;

    Projected sideOriginBefore = originProjection(ViewKind::Side);
    Vec3 sideCenterBefore = sideView->center;
    app.beginCameraDrag(100, 100, ViewKind::Side, true);
    app.updateCameraDrag(118, 80);
    app.finishDrag();
    Projected sideOriginAfter = originProjection(ViewKind::Side);
    if (!sideOriginBefore.ok || !sideOriginAfter.ok) return 62;
    if (std::abs(sideOriginAfter.x - sideOriginBefore.x) <= 1 ||
        std::abs(sideOriginAfter.y - sideOriginBefore.y) <= 1) return 63;
    if (sideOriginAfter.x <= sideOriginBefore.x || sideOriginAfter.y >= sideOriginBefore.y) return 68;
    if (std::fabs(sideView->center.x - sideCenterBefore.x) > 0.001f) return 64;

    app.frameAll();
    Projected topOriginAfterFrame = originProjection(ViewKind::Top);
    if (!topOriginAfterFrame.ok) return 65;

    Vec3 perspectiveCenterBefore = perspectiveView->center;
    Projected perspectiveOriginBeforePan = originProjection(ViewKind::Perspective);
    float perspectiveYawBefore = perspectiveView->yaw;
    app.beginCameraDrag(100, 100, ViewKind::Perspective, true);
    app.updateCameraDrag(130, 80);
    app.finishDrag();
    Projected perspectiveOriginAfterPan = originProjection(ViewKind::Perspective);
    if (length(perspectiveView->center - perspectiveCenterBefore) <= 0.001f) return 51;
    if (std::fabs(perspectiveView->yaw - perspectiveYawBefore) > 0.001f) return 52;
    if (!perspectiveOriginBeforePan.ok || !perspectiveOriginAfterPan.ok) return 69;
    if (std::abs(perspectiveOriginAfterPan.x - perspectiveOriginBeforePan.x) <= 1 &&
        std::abs(perspectiveOriginAfterPan.y - perspectiveOriginBeforePan.y) <= 1) return 70;
    if (perspectiveOriginAfterPan.x <= perspectiveOriginBeforePan.x ||
        perspectiveOriginAfterPan.y >= perspectiveOriginBeforePan.y) return 79;
    perspectiveCenterBefore = perspectiveView->center;
    perspectiveYawBefore = perspectiveView->yaw;
    app.beginCameraDrag(100, 100, ViewKind::Perspective, false);
    app.updateCameraDrag(130, 80);
    app.finishDrag();
    if (std::fabs(perspectiveView->yaw - perspectiveYawBefore) <= 0.001f) return 53;
    if (length(perspectiveView->center - perspectiveCenterBefore) > 0.001f) return 54;

    FrameBuffer textFb;
    textFb.clear(BLACK);
    drawText(textFb, 2, 4, "파일 日本", WHITE);
    size_t litPixels = std::count_if(textFb.pixels.begin(), textFb.pixels.end(), [](uint32_t px) {
        return px == packColor(WHITE);
    });
    if (litPixels < 12) return 29;

    std::filesystem::path dir = ensureDir(executableDir() / "selftest");
    std::filesystem::path pf16 = dir / "selftest_scene.pf16";
    std::filesystem::path objPath = dir / "selftest_scene.obj";
    if (!app.savePf16(pf16)) return 14;
    std::filesystem::path unicodeDir = ensureDir(dir / L"모델");
    std::filesystem::path unicodePf16 = unicodeDir / L"테스트_모델.pf16";
    if (!app.savePf16(unicodePf16)) return 30;

    App loaded;
    if (!loaded.loadPf16(pf16)) return 15;
    if (!loaded.loadPf16(unicodePf16)) return 31;
    if (loaded.scene.objects.empty()) return 16;
    if (!loaded.exportObj(objPath)) return 17;
    std::filesystem::path unicodeObj = unicodeDir / L"테스트_모델.obj";
    if (!loaded.exportObj(unicodeObj)) return 32;
    loaded.language = Locale::Korean;
    loaded.render();
    if (loaded.fb.pixels.empty()) return 18;
    auto pixelAt = [&](int x, int y) {
        return loaded.fb.pixels[size_t(y) * kLogicalW + size_t(x)];
    };
    if (pixelAt(620, 5) != packColor(kTheme.panel)) return 19;
    if (pixelAt(635, 25) != packColor(kTheme.viewportBackground)) return 20;
    if (pixelAt(620, 445) != packColor(kTheme.panel)) return 21;
    return 0;
}

static App g_app;

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_SIZE:
        g_app.updateScale(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE: {
        auto [x, y] = g_app.screenToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        g_app.onMouseMove(x, y, wParam);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        auto [x, y] = g_app.screenToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        g_app.onLeftDown(x, y);
        return 0;
    }
    case WM_LBUTTONUP: {
        auto [x, y] = g_app.screenToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        g_app.onLeftUp(x, y);
        return 0;
    }
    case WM_MBUTTONDOWN: {
        auto [x, y] = g_app.screenToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        g_app.onMiddleDown(x, y);
        return 0;
    }
    case WM_MBUTTONUP:
        g_app.finishDrag();
        return 0;
    case WM_CAPTURECHANGED:
        g_app.onCaptureChanged(reinterpret_cast<HWND>(lParam));
        return 0;
    case WM_KILLFOCUS:
        g_app.onFocusLost();
        return 0;
    case WM_RBUTTONDOWN: {
        auto [x, y] = g_app.screenToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        g_app.onRightDown(x, y);
        return 0;
    }
    case WM_MOUSEWHEEL:
        g_app.onMouseWheel(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_KEYDOWN:
        g_app.onKeyDown(wParam);
        return 0;
    case WM_CHAR:
        g_app.onChar(wParam);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps {};
        HDC hdc = BeginPaint(hwnd, &ps);
        g_app.paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        g_app.saveOnExitPrompt();
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

} // namespace pf16

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    using namespace pf16;
    if (std::wstring(GetCommandLineW()).find(L"--self-test") != std::wstring::npos) {
        return runSelfTest();
    }

    g_app.instance = hInstance;

    WNDCLASSW wc {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"PixelForge16Window";
    if (!RegisterClassW(&wc)) {
        MessageBoxW(nullptr, L"RegisterClass failed.", L"PIXELFORGE 16", MB_ICONERROR);
        return 1;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc {0, 0, kDefaultWindowW, kDefaultWindowH};
    AdjustWindowRect(&rc, style, FALSE);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"PIXELFORGE 16",
                                style, CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top,
                                nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) {
        MessageBoxW(nullptr, L"CreateWindow failed.", L"PIXELFORGE 16", MB_ICONERROR);
        return 1;
    }
    g_app.hwnd = hwnd;
    g_app.updateScale(kDefaultWindowW, kDefaultWindowH);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return int(msg.wParam);
}
