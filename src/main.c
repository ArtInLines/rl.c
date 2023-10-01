//////////////
// Includes //
//////////////

#include "main.h"
#include <stdbool.h>   // For boolean definitions
#include <stdio.h>     // For printf - only used for debugging
#include <assert.h>    // For assert
#include <string.h>    // For memcpy
#include "raylib.h"    // For immediate UI framework

#define UTIL_IMPLEMENTATION
#include "util.h"
#define BUF_IMPLEMENTATION
#include "buf.h"
#define SV_IMPLEMENTATION
#include "sv.h"
#define STB_DS_IMPLEMENTATION
#define STBDS_NO_SHORT_NAMES
#include "stb_ds.h"  // For dynamic arrays


///////////////
// Functions //
///////////////

Table readTabFile(const char *fpath)
{
    u64 size;
    char* buf = util_readFile(fpath, &size);
    Table tab = { .cols = NULL, .vals = NULL };

    // @TODO: Parse file as Table

    free(buf);
    return tab;
}

bool writeTabFile(const char *fpath, Table table)
{
    (void)fpath;
    (void)table;
    // @TODO: Rewrite to use buffer
    i32 colslen    = stbds_arrlen(table.cols);
    char *cols_buf = malloc(colslen * sizeof(Type_Opts));
    u32 row_size   = 0;
    (void)row_size;
    u32 buf_idx    = 0;
    for (i32 i = 0; i < colslen; i++) {
        Column col = table.cols[i];
        cols_buf[buf_idx++] = col.type;
        STATIC_ASSERT(TYPE_LEN == 4);
        switch (col.type)
        {
        case TYPE_SELECT:
        case TYPE_TAG:
            STATIC_ASSERT(sizeof(size_t) == sizeof(char**));
            size_t opts = (size_t) col.opts.strs;

            // @TODO: Gotta store the strings too ugh
            for (u32 j = 0; j < sizeof(char**); j++) {
                cols_buf[buf_idx+j] = (char) (opts & (0xff << (j*8)));
            }
            buf_idx += sizeof(char**);
            break;
        default:
            break;
        }
    }
    u64 cols_size = buf_idx;
    buf_idx       = 0;
    (void)cols_size;

    for (i32 i = 0; i < colslen; i++) {

    }

    TODO();
}

// Assumes that the file under the path fpath exists and can be read from
// Assumes all '.tab' files to be in the current directory
Table_Defs readDefFile(const char *fpath)
{
    u64 size;
    char *buf = util_readFile(fpath, &size);
    Table_Defs td = { .names = NULL, .tabs = NULL };

    u64 i = 0;
    while (i < size) {
        Table table = { .cols = NULL, .vals = NULL };
        String_View name = {
            .count = (size_t) buf[i],
            .data  = malloc(buf[i])
        };
        memcpy(name.data, &buf[i+1], name.count);

        char *tab_fname = util_memadd(name.data, name.count, ".tab", 5);
        if (FileExists(tab_fname)) table = readTabFile(tab_fname);
        free(tab_fname);

        stbds_arrput(td.tabs, table);
        stbds_arrput(td.names, name);
        i += name.count + 1;
    }

    free(buf);
    return td;
}

// Also writes the '.tab' files for each table in td into the current working directory
// To write everything into the same directory, you should therefore change into that directory first before calling this function
bool writeDefFile(const char *fpath, Table_Defs td)
{
    u64 size = 0;
    i32 len  = stbds_arrlen(td.names);
    for (i32 i = 0; i < len; i++) {
        size += 1 + td.names[i].count;
    }

    char *buf = malloc(size);
    for (i32 i = 0, j = 0; j < len; j++) {
        String_View name = td.names[j];
        buf[i] = (u8) name.count;
        memcpy(&buf[i+1], name.data, name.count);
        i += 1 + name.count;

        char *tab_fname = util_memadd(name.data, name.count, ".tab", 5);
        bool res = writeTabFile(tab_fname, td.tabs[j]);
        free(tab_fname);
        if (!res) return false;
    }
    if (!util_writeFile(fpath, buf, size)) return false;
    return true;
}

int main(void)
{
    i32 win_width  = 1200;
    i32 win_height = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(win_width, win_height, "RL");
    SetTargetFPS(60);

    float size_default = 50;
    float size_max     = size_default;
    float spacing      = 2;
    float margin       = 5;
    Font font = LoadFontEx("./assets/Roboto-Regular.ttf", size_max, NULL, 95);

    // Read Data
    Table_Defs td = { .names = NULL, .tabs = NULL };
    if (!DirectoryExists("./data")) mkdir("./data");
    chdir("./data");
    if (FileExists("./td.def")) td = readDefFile("./td.def");
    chdir("..");

    stbds_arrput(td.names, sv_from_cstr("Test"));
    stbds_arrput(td.tabs,  (Table) {0});

    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        BeginDrawing();

        if (IsWindowResized()) {
            win_width  = GetScreenWidth();
            win_height = GetScreenHeight();
        }
        ClearBackground(BLACK);

        int tables_amount = stbds_arrlen(td.names);
        int total_height  = tables_amount * size_default + (tables_amount - 1) * margin;
        int text_y        = MAX(((win_height - total_height)/2), margin);
        for (int i = 0; i < tables_amount && text_y + size_default + margin < win_height; i++, text_y += size_default + margin) {
            const char *table_name = td.names[i].data;
            int   text_width = MeasureTextEx(font, table_name, size_default, spacing).x;
            Vector2 v = { .x = (win_width - text_width)/2, .y = text_y };
            DrawTextEx(font, table_name, v, size_default, spacing, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}