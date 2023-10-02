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
    Buffer buf = buf_fromFile(fpath);
    Table tab = { .cols = NULL, .vals = NULL };
    i32 colslen = buf_read4i(&buf);
    stbds_arrsetcap(tab.cols, colslen);
    stbds_arrsetcap(tab.vals, colslen);
    for (i32 i = 0; i < colslen; i++) {
        tab.cols[i] = buf_readColumn(&buf);
    }
    i32 rowslen = buf_read4i(&buf);
    for (i32 c = 0; c < colslen; c++) {
        switch (tab.cols[c].type)
        {
        case TYPE_STR:
            stbds_arrsetcap(tab.vals[c].strs, rowslen);
            for (i32 r = 0; r < rowslen; r++) {
                Value_Str sv = buf_readSV(&buf);
                stbds_arrput(tab.vals[c].strs, sv);
            }
            break;
        case TYPE_SELECT:
            stbds_arrsetcap(tab.vals[c].selects, rowslen);
            for (i32 r = 0; r < rowslen; r++) {
                Value_Select idx = buf_read4i(&buf);
                stbds_arrput(tab.vals[c].selects, idx);
            }
            break;
        case TYPE_TAG:
            stbds_arrsetcap(tab.vals[c].tags, rowslen);
            for (i32 r = 0; r < rowslen; r++) {
                Value_Tag tag = NULL;
                i32 amount    = buf_read4i(&buf);
                stbds_arrsetcap(tag, amount);
                for (i32 k = 0; k < amount; k++) {
                    i32 idx = buf_read4i(&buf);
                    stbds_arrput(tag, idx);
                }
                stbds_arrput(tab.vals[c].tags, tag);
            }
            break;
        case TYPE_DATE:
            stbds_arrsetcap(tab.vals[c].dates, rowslen);
            for (i32 r = 0; r < rowslen; r++) {
                u8  d = buf_read1(&buf);
                u8  m = buf_read1(&buf);
                u16 y = buf_read2(&buf);
                Value_Date date = { .day = d, .month = m, .year = y };
                stbds_arrput(tab.vals[c].dates, date);
            }
            break;
        default:
            PANIC("Unexpected column type '%d' in reading table file %s", tab.cols[c].type, fpath);
        }
    }
    buf_free(buf);
    return tab;
}

bool writeTabFile(const char *fpath, Table table)
{
    i32 colslen = stbds_arrlen(table.cols);
    Buffer buf  = buf_new(64 * 1028);
    buf_write4i(&buf, colslen);
    for (i32 i = 0; i < colslen; i++) {
        buf_writeColumn(&buf, table.cols[i]);
    }
    i32 rowslen     = 0;
    u64 rowslen_idx = buf.idx;
    buf_write4i(&buf, rowslen);
    for (i32 i = 0; i < colslen; i++) {
        Values col = table.vals[i];
        switch (table.cols[i].type)
        {
        case TYPE_STR:
            rowslen = stbds_arrlen(col.strs);
            for (i32 j = 0; j < rowslen; j++) {
                Value_Str sv = col.strs[j];
                buf_writeStr(&buf, sv.data, sv.count);
            }
            break;

        case TYPE_SELECT:
            rowslen = stbds_arrlen(col.selects);
            for (i32 j = 0; j < rowslen; j++) {
                Value_Select idx = col.selects[j];
                buf_write4i(&buf, idx);
            }
            break;

        case TYPE_TAG:
            rowslen = stbds_arrlen(col.tags);
            for (i32 j = 0; j < rowslen; j++) {
                Value_Tag tags = col.tags[j];
                i32 tagslen    = stbds_arrlen(tags);
                buf_write4i(&buf, tagslen);
                for (i32 k = 0; k < tagslen; k++) {
                    buf_write4i(&buf, tags[k]);
                }
            }
            break;

        case TYPE_DATE:
            rowslen = stbds_arrlen(col.dates);
            for (i32 j = 0; j < rowslen; j++) {
                Value_Date date = col.dates[j];
                buf_write1(&buf, date.day);
                buf_write1(&buf, date.month);
                buf_write2(&buf, date.year);
            }
            break;
        default:
            PANIC("Unexpected column type '%d' in writing table file %s", table.cols[i].type, fpath);
        }
    }
    *((i32*)(&buf.data[rowslen_idx])) = rowslen;
    return buf_toFile(&buf, fpath);
}

// Assumes that the file under the path fpath exists and can be read from
// Assumes all '.tab' files to be in the current directory
Table_Defs readDefFile(const char *fpath)
{
    Buffer buf = buf_fromFile(fpath);
    Table_Defs td = { .names = NULL, .tabs = NULL };

    while (buf_iter_cond(buf)) {
        Table table = { .cols = NULL, .vals = NULL };
        String_View name = buf_readSV(&buf);

        char *tab_fname = util_memadd(name.data, name.count, ".tab", 5);
        if (FileExists(tab_fname)) table = readTabFile(tab_fname);
        free(tab_fname);

        stbds_arrput(td.tabs, table);
        stbds_arrput(td.names, name);
    }

    buf_free(buf);
    return td;
}

// If `write_tables` is true, it writes the '.tab' files for each table in td into the current working directory
// To write everything into the same directory, you should therefore change into that directory first before calling this function
bool writeDefFile(const char *fpath, Table_Defs td, bool write_tables)
{
    u64 size = 0;
    i32 len  = stbds_arrlen(td.names);
    for (i32 i = 0; i < len; i++) {
        size += 1 + td.names[i].count;
    }
    Buffer buf = buf_new(size);
    for (i32 i = 0; i < len; i++) {
        String_View name = td.names[i];
        buf_writeStr(&buf, name.data, name.count);

        if (write_tables) {
            char *tab_fname = util_memadd(name.data, name.count, ".tab", 5);
            bool res = writeTabFile(tab_fname, td.tabs[i]);
            free(tab_fname);
            if (!res) return false;
        }
    }
    printf("Buffer Size: %lld\n", buf.size);
    return buf_toFile(&buf, fpath);
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
    if (FileExists("./tables.def")) td = readDefFile("./tables.def");
    else {
        // @TODO: Only for debugging at the beginning now
        stbds_arrput(td.names, sv_from_cstr("Test"));
        stbds_arrput(td.tabs,  (Table) {0});
        stbds_arrput(td.names, sv_from_cstr("Table 2"));
        stbds_arrput(td.tabs,  (Table) {0});
        writeDefFile("./tables.def", td, true);
    }
    chdir("..");

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