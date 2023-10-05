//////////////
// Includes //
//////////////

#include "main.h"
#include <stdbool.h>   // For boolean definitions
#include <stdio.h>     // For printf - only used for debugging
#include <assert.h>    // For assert
#include <string.h>    // For memcpy
#include "raylib.h"    // For immediate UI framework
#include "raygui.h"

#define UTIL_IMPLEMENTATION
#include "util.h"
#define BUF_IMPLEMENTATION
#include "buf.h"
#define GUI_IMPLEMENTATION
#include "gui.h"
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
    stbds_arrsetlen(tab.cols, colslen);
    stbds_arrsetlen(tab.vals, colslen);
    memset(tab.vals, 0, colslen * sizeof(Values));
    for (i32 i = 0; i < colslen; i++) {
        tab.cols[i] = buf_readColumn(&buf);
    }
    i32 rowslen = buf_read4i(&buf);
    for (i32 c = 0; c < colslen; c++) {
        switch (tab.cols[c].type)
        {
        case TYPE_STR:
            stbds_arrsetlen(tab.vals[c].strs, rowslen);
            for (i32 r = 0; r < rowslen; r++) {
                Value_Str sv = buf_readSV(&buf);
                stbds_arrput(tab.vals[c].strs, sv);
            }
            break;
        case TYPE_SELECT:
            stbds_arrsetlen(tab.vals[c].selects, rowslen);
            for (i32 r = 0; r < rowslen; r++) {
                Value_Select idx = buf_read4i(&buf);
                stbds_arrput(tab.vals[c].selects, idx);
            }
            break;
        case TYPE_TAG:
            stbds_arrsetlen(tab.vals[c].tags, rowslen);
            for (i32 r = 0; r < rowslen; r++) {
                Value_Tag tag = NULL;
                i32 amount    = buf_read4i(&buf);
                stbds_arrsetlen(tag, amount);
                for (i32 k = 0; k < amount; k++) {
                    i32 idx = buf_read4i(&buf);
                    stbds_arrput(tag, idx);
                }
                stbds_arrput(tab.vals[c].tags, tag);
            }
            break;
        case TYPE_DATE:
            stbds_arrsetlen(tab.vals[c].dates, rowslen);
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

bool writeTabFile(String_View tablename, Table table, char *dir)
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
    if (UNLIKELY(table.vals != NULL)) {
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
    }

    if (dir != NULL) chdir(dir);
    char *filename = util_memadd(tablename.data, tablename.count, ".tab", 5);
    bool out = buf_toFile(&buf, filename);
    free(filename);
    if (dir != NULL) chdir("..");
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

        if (write_tables && !writeTabFile(name, td.tabs[i], NULL)) return false;
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
    stbds_arrsetcap(out.cols, 16);
    stbds_arrsetcap(out.cols, 32);
    stbds_arrput(td->names, name);
    stbds_arrput(td->tabs,  out);
    // Save new table
    chdir("./data");
    writeTabFile(name, out, NULL);
    writeDefFile(TD_FILENAME, *td, false);
    chdir("..");
    return out;
}

bool addColumn(Table_Defs td, u32 tdidx, String_View name, Datatype type)
{
    if (UNLIKELY(stbds_arrlen((td).tabs) <= (tdidx))) return false;
    Table *table = &(td).tabs[(tdidx)];
    i32 rowslen = getTableRowsLen(*table);
    Column col = {0};
    col.name = name;
    col.type = type;
    stbds_arrput(table->cols, col);

    // Fill column in all rows with default value
    if (rowslen > 0) {
        Values vals = {0};
        switch (type)
        {
        case TYPE_STR:
            stbds_arrsetlen(vals.strs, rowslen);
            memset(vals.strs, VALUE_DEFAULT_STR, rowslen * sizeof(Value_Str));
            break;
        case TYPE_SELECT:
            stbds_arrsetlen(vals.selects, rowslen);
            memset(vals.selects, VALUE_DEFAULT_SELECT, rowslen * sizeof(Value_Select));
            break;
        case TYPE_TAG:
            stbds_arrsetlen(vals.tags, rowslen);
            memset(vals.tags, VALUE_DEFAULT_TAG, rowslen * sizeof(Value_Tag));
            break;
        case TYPE_DATE:
            stbds_arrsetlen(vals.dates, rowslen);
            memset(vals.dates, VALUE_DEFAULT_DATE, rowslen * sizeof(Value_Date));
            break;
        case TYPE_LEN:
            PANIC("Cannot add a column of type 'len'");
        }
        stbds_arrput(table->vals, vals);
    } else {
        stbds_arrput(table->vals, (Values){0});
    }
    return writeTabFile(td.names[tdidx], *table, "./data");
}

bool rmColumn(Table_Defs td, u32 tdidx, u32 colidx)
{
    if (UNLIKELY(stbds_arrlen((td).tabs) <= (tdidx))) return false;
    Table *table = &(td).tabs[(tdidx)];
    if (UNLIKELY(stbds_arrlen(table->cols) <= colidx)) return false;

    // @Memory: This is probably leaking memory. I probably have to go through each row and manually free everything there -_-
    stbds_arrdel(table->vals, colidx);
    stbds_arrdel(table->cols, colidx);
    return writeTabFile(td.names[tdidx], *table, "./data");
}

bool renameColumn(Table_Defs td, u32 tdidx, u32 colidx, String_View newname)
{
    if (UNLIKELY(stbds_arrlen((td).tabs) <= (tdidx))) return false;
    Table *table = &(td).tabs[(tdidx)];
    if (UNLIKELY(stbds_arrlen(table->cols) <= colidx)) return false;
    Column col = table->cols[colidx];
    free(col.name.data);
    col.name = newname;
    return writeTabFile(td.names[tdidx], *table, "./data");
}

// Add options to a column of type SELECT or TAG
bool addOptSelectableColumn(Table_Defs td, u32 tdidx, u32 colidx, String_View sv)
{
    if (UNLIKELY(stbds_arrlen((td).tabs) <= (tdidx))) return false;
    Table *table = &(td).tabs[(tdidx)];
    if (UNLIKELY(stbds_arrlen(table->cols) <= colidx)) return false;
    stbds_arrput(table->cols[colidx].opts.strs, sv);
    return writeTabFile(td.names[tdidx], *table, "./data");
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

bool addRow(Table_Defs td, u32 tdidx)
{
    if (UNLIKELY(stbds_arrlen((td).tabs) <= (tdidx))) return false;
    Table *table = &(td).tabs[(tdidx)];
    for (i32 i = 0; i < stbds_arrlen(table->cols); i++) {
        switch (table->cols[i].type)
        {
        case TYPE_STR:
            stbds_arrput(table->vals[i].strs,    (Value_Str){VALUE_DEFAULT_STR});
            break;
        case TYPE_SELECT:
            stbds_arrput(table->vals[i].selects, (Value_Select){VALUE_DEFAULT_SELECT});
            break;
        case TYPE_TAG:
            stbds_arrput(table->vals[i].tags,    (Value_Tag){VALUE_DEFAULT_TAG});
            break;
        case TYPE_DATE:
            stbds_arrput(table->vals[i].dates,   (Value_Date){VALUE_DEFAULT_DATE});
            break;
        case TYPE_LEN:
            PANIC("Can't add values to a column of type 'len'");
        }
    }
    return writeTabFile(td.names[tdidx], *table, "./data");
}

bool setValue(Table_Defs td, u32 tdidx, u32 colidx, u32 rowidx, Value val)
{
    if (UNLIKELY(stbds_arrlen((td).tabs) <= (tdidx))) return false;
    Table *table = &(td).tabs[(tdidx)];
    if (UNLIKELY(stbds_arrlen(table->cols) <= colidx)) return false;
    Column col  = table->cols[colidx];
    Values vals = table->vals[colidx];
    switch (col.type)
    {
    case TYPE_STR:
        if (UNLIKELY(stbds_arrlen(vals.strs) <= rowidx)) return false;
        vals.strs[rowidx] = val.str;
        break;
    case TYPE_SELECT:
        if (UNLIKELY(stbds_arrlen(vals.selects) <= rowidx)) return false;
        vals.selects[rowidx] = val.select;
        break;
    case TYPE_TAG:
        if (UNLIKELY(stbds_arrlen(vals.tags) <= rowidx)) {
            printf("len: %lld\n", stbds_arrlen(vals.tags));
            return false;}
        vals.tags[rowidx] = val.tag;
        break;
    case TYPE_DATE:
        if (UNLIKELY(stbds_arrlen(vals.dates) <= rowidx)) return false;
        vals.dates[rowidx] = val.date;
        break;
    case TYPE_LEN:
        PANIC("Cannot set a value for a column of type 'len'");
    }
    // table->vals[colidx] = vals;
    return writeTabFile(td.names[tdidx], *table, "./data");
}

int main(void)
{
    i32 win_width  = 1200;
    i32 win_height = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(win_width, win_height, "RL");
    SetTargetFPS(60);

    UI_View  view  = UI_STATE_START;
    UI_State state = {0};

    float spacing      = 2;
    float padding      = 5;
    float margin       = 8;
    float size_default = 50;
    float size_max     = size_default;
    Font font = LoadFontEx("./assets/Roboto-Regular.ttf", size_max, NULL, 95);

    Gui_El_Style style_default = {
        .bg = LIGHTGRAY,
        .border_color = BLANK,
        .border_width = 5,
        .color = BLACK,
        .font = font,
        .font_size = size_default,
        .pad = 2*padding,
        .spacing = spacing,
    };
    Gui_El_Style style_hover = gui_cloneStyle(style_default);
    style_hover.border_color = BLUE;
    (void)style_hover;

    // Read Data
    Table_Defs td = { .names = NULL, .tabs = NULL };
    if (!DirectoryExists("./data")) mkdir("./data");
    chdir("./data");
    if (FileExists(TD_FILENAME)) {
        td = readDefFile(TD_FILENAME);
        chdir("..");
    } else {
        // @TODO: Only for debugging at the beginning now
        // Can be removed once all of these functions can be done via the UI
        chdir("..");
        // newTable(&td, sv_from_cstr("Books"));
        // addColumn(td, 0, sv_from_cstr("Name"), TYPE_STR);
        // renameTable(td, 0, sv_from_cstr("Reading List"));
        // addRow(td, 0);
        // addRow(td, 0);
        // addColumn(td, 0, sv_from_cstr("Author"), TYPE_TAG);
        // addOptSelectableColumn(td, 0, 1, sv_from_cstr("Errico Malateste"));
        // addOptSelectableColumn(td, 0, 1, sv_from_cstr("Karl Marx"));
        // newTable(&td, sv_from_cstr("Uni Courses"));
        // Value val = { .tag = NULL };
        // stbds_arrput(val.tag, 1);
        // stbds_arrput(val.tag, 0);
        // setValue(td, 0, 1, 1, val);
    }

    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        BeginDrawing();

        bool isResized = IsWindowResized();
        if (isResized) {
            win_width  = GetScreenWidth();
            win_height = GetScreenHeight();
        }
        ClearBackground(BLACK);
        SetMouseCursor(MOUSE_CURSOR_DEFAULT); // Reset to default

        switch (view)
        {
        case UI_STATE_START: {
            // List all tables in center of screen
            i32 tables_amount = stbds_arrlen(td.names);
            i32 total_height  = tables_amount * size_default + (tables_amount - 1) * margin;
            i32 text_y        = MAX(((win_height - total_height)/2), margin);
            i32 max_width     = 0;
            i32 *text_widths  = malloc((tables_amount + 1) * sizeof(i32));
            Vector2 mouse     = GetMousePosition();

            for (i32 i = -1; i < tables_amount; i++) {
                char *table_name = i == -1 ? "New Table" : td.names[i].data;
                text_widths[i+1] = MeasureTextEx(font, table_name, size_default, spacing).x;
                if (text_widths[i+1] > max_width) max_width = text_widths[i+1];
            }

            for (i32 i = -1; i < tables_amount && text_y + size_default + margin < win_height; i++, text_y += size_default + margin + 2*padding) {
                char *table_name = i == -1 ? "New Table" : td.names[i].data;
                i32   text_width = text_widths[i+1];
                Vector2   v      = { .x = (win_width - text_width)/2, .y = text_y + padding };
                Rectangle r      = { .x = (win_width - max_width)/2 - padding, .y = text_y, .width = max_width + 2*padding, .height = size_default + 2*padding };
                DrawRectangleRounded(r, 0.5f, 4, i == -1 ? GREEN : GRAY);
                DrawTextEx(font, table_name, v, size_default, spacing, WHITE);
                if (mouse.x >= r.x && mouse.x <= r.x + r.width && mouse.y >= r.y && mouse.y <= r.y + r.height) {
                    // Mouse is hovering the button
                    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        if (i == -1) {
                            view  = UI_STATE_NEW_TABLE;
                            state.newtable.input = gui_newInputBox("Tablename", false, false, true, gui_newCenteredLabel((Rectangle){.x=0, .y=0, .width=win_width, .height=win_height}, win_width/2, NULL, style_default, style_default));
                        } else {
                            view = UI_STATE_TABLE;
                            state.table.tdidx = i;
                        }
                    }
                }
            }
            break;
        }

        case UI_STATE_NEW_TABLE: {
            state.newtable.input.selected = true;
            if (isResized) gui_centerLabel(&state.newtable.input.label, (Rectangle){.x=0, .y=0, .width=win_width, .height=win_height}, win_width/2);

            gui_drawInputBox(&state.newtable.input);
            if (IsKeyPressed(KEY_ENTER)) {
                char *name = state.newtable.input.label.text;
                if (stbds_arrlen(name) > 1) {
                    newTable(&td, sv_from_cstr(name));
                    view = UI_STATE_START; // UI_STATE_TABLE;
                    state = (UI_State) {0};
                }
            } else if (IsKeyPressed(KEY_ESCAPE)) {
                view = UI_STATE_START;
                state = (UI_State) {0};
            }
            break;
        }

        case UI_STATE_TABLE: {
            TODO();
        }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}