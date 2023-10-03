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

const char TD_FILENAME[] = "./tables.def";


///////////////
// Functions //
///////////////

Table readTabFile(String_View tablename)
{
    char *filename = util_memadd(tablename.data, tablename.count, ".tab", 5);
    if (!FileExists(filename)) return (Table) {0};
    Buffer buf = buf_fromFile(filename);
    free(filename);

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
            PANIC("Unexpected column type '%d' in reading table file %s", tab.cols[c].type, tablename.data);
        }
    }
    buf_free(buf);
    return tab;
}

bool writeTabFile(String_View tablename, Table table)
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
            PANIC("Unexpected column type '%d' in writing table '%s'", table.cols[i].type, tablename.data);
        }
    }
    *((i32*)(&buf.data[rowslen_idx])) = rowslen;
    char *filename = util_memadd(tablename.data, tablename.count, ".tab", 5);
    bool out = buf_toFile(&buf, filename);
    free(filename);
    return out;
}

// Assumes that the file under the path fpath exists and can be read from
// Assumes all '.tab' files to be in the current directory
Table_Defs readDefFile(const char *fpath)
{
    Buffer buf = buf_fromFile(fpath);
    Table_Defs td = { .names = NULL, .tabs = NULL };

    while (buf_iter_cond(buf)) {
        String_View name = buf_readSV(&buf);
        Table table = readTabFile(name);
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

        if (write_tables && !writeTabFile(name, td.tabs[i])) return false;
    }
    return buf_toFile(&buf, fpath);
}

i32 getTableRowsLen(Table table)
{
    if (UNLIKELY(stbds_arrlen(table.vals)) == 0) return 0;
    switch (table.cols[0].type)
    {
    case TYPE_STR:
        return stbds_arrlen(table.vals[0].strs);
    case TYPE_SELECT:
        return stbds_arrlen(table.vals[0].selects);
    case TYPE_TAG:
        return stbds_arrlen(table.vals[0].tags);
    case TYPE_DATE:
        return stbds_arrlen(table.vals[0].dates);
    case TYPE_LEN:
        PANIC("Received illegal column type 'len'");
    }
    return 0;
}

Table newTable(Table_Defs *td, String_View name)
{
    Table out = {0};
    stbds_arrput(td->names, name);
    stbds_arrput(td->tabs,  out);
    // Save new table
    chdir("./data");
    writeTabFile(name, out);
    writeDefFile(TD_FILENAME, *td, false);
    chdir("..");
    return out;
}

bool addColumn(Table_Defs td, u32 tdidx, String_View name, Datatype type)
{
    if (UNLIKELY(stbds_arrlen(td.tabs) <= tdidx)) return false;
    Table *table = &td.tabs[tdidx];
    i32 rowslen = getTableRowsLen(*table);
    Column col = {0};
    col.name = name;
    col.type = type;
    stbds_arrput(table->cols, col);

    // Fill column in all rows with default value
    Values vals = {0};
    switch (type)
    {
    case TYPE_STR:
        stbds_arrsetcap(vals.strs, rowslen);
        stbds_arrput(vals.strs, (Value_Str) {0});
        memset(vals.strs, 0, rowslen * sizeof(Value_Str));
        break;
    case TYPE_SELECT:
        stbds_arrsetcap(vals.selects, rowslen);
        memset(vals.selects, -1, rowslen * sizeof(Value_Select));
        break;
    case TYPE_TAG:
        stbds_arrsetcap(vals.tags, rowslen);
        memset(vals.tags, 0, rowslen * sizeof(Value_Tag));
        break;
    case TYPE_DATE:
        stbds_arrsetcap(vals.strs, rowslen);
        memset(vals.strs, 0, rowslen * sizeof(Value_Date));
        break;
    case TYPE_LEN:
        PANIC("Cannot add a column of type 'len'");
    }
    stbds_arrput(table->vals, vals);

    // Save updated Table
    chdir("./data");
    bool out = writeTabFile(td.names[tdidx], *table);
    chdir("..");
    return out;
}

bool rmColumn(Table_Defs td, u32 tdidx, u32 colidx)
{
    if (UNLIKELY(stbds_arrlen(td.tabs) <= tdidx)) return false;
    Table *table = &td.tabs[tdidx];
    if (UNLIKELY(stbds_arrlen(table->cols) <= colidx)) return false;

    // @Memory: This is probably leaking memory. I probably have to go through each row and manually free everything there -_-
    stbds_arrdel(table->vals, colidx);
    stbds_arrdel(table->cols, colidx);

    chdir("./data");
    bool out = writeTabFile(td.names[tdidx], *table);
    chdir("..");
    return out;
}

bool renameColumn(Table_Defs td, u32 tdidx, u32 colidx, String_View newname)
{
    if (UNLIKELY(stbds_arrlen(td.tabs) <= tdidx)) return false;
    Table *table = &td.tabs[tdidx];
    if (UNLIKELY(stbds_arrlen(table->cols) <= colidx)) return false;
    Column col = table->cols[colidx];
    free(col.name.data);
    col.name = newname;

    chdir("./data");
    bool out = writeTabFile(td.names[tdidx], *table);
    chdir("..");
    return out;
}

// Add options to a column of type SELECT or TAG
bool addOptSelectableColumn(Table_Defs td, u32 tdidx, u32 colidx, String_View sv)
{
    if (UNLIKELY(stbds_arrlen(td.tabs) <= tdidx)) return false;
    Table *table = &td.tabs[tdidx];
    if (UNLIKELY(stbds_arrlen(table->cols) <= colidx)) return false;
    Column col = table->cols[colidx];
    stbds_arrput(col.opts.strs, sv);

    chdir("./data");
    bool out = writeTabFile(td.names[tdidx], *table);
    chdir("..");
    return out;
}

bool renameTable(Table_Defs td, u32 idx, String_View new_name)
{
    if (UNLIKELY(stbds_arrlen(td.tabs) <= idx)) return false;
    String_View old_name = td.names[idx];
    td.names[idx] = new_name;
    chdir("./data");
    if (UNLIKELY(!writeDefFile(TD_FILENAME, td, false))) return false;
    char *old_fname = util_memadd(old_name.data, old_name.count, ".tab", 5);
    char *new_fname = util_memadd(new_name.data, new_name.count, ".tab", 5);
    i32 out = rename(old_fname, new_fname);
    free(old_fname);
    free(new_fname);
    // free(old_name.data);
    chdir("..");
    return out == 0;
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
    if (FileExists(TD_FILENAME)) {
        td = readDefFile(TD_FILENAME);
        chdir("..");
    } else {
        // @TODO: Only for debugging at the beginning now
        chdir("..");
        newTable(&td, sv_from_cstr("Books"));
        addColumn(td, 0, sv_from_cstr("Name"), TYPE_STR);
        renameTable(td, 0, sv_from_cstr("Reading List"));
        addColumn(td, 0, sv_from_cstr("Author"), TYPE_TAG);
        addOptSelectableColumn(td, 0, 1, sv_from_cstr("Errico Malateste"));
        addOptSelectableColumn(td, 0, 1, sv_from_cstr("Karl Marx"));
        newTable(&td, sv_from_cstr("Uni Courses"));
        writeDefFile(TD_FILENAME, td, true);
    }

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