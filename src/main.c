//////////////
// Includes //
//////////////

#include <stdint.h>    // For specifically sized ints
#include <stdbool.h>   // For boolean definitions
#include <stdio.h>     // For printf - only used for debugging
#include <assert.h>    // For assert
#include <string.h>    // For memcpy
#include "raylib.h"    // For immediate UI framework

#define FS_IMPLEMENTATION
#include "fs.h"        // For mkdir, open, read, stat, close, readFile, writeFile

#define SV_IMPLEMENTATION
#include "sv.h"        // For String_View

#define STB_DS_IMPLEMENTATION
#define STBDS_NO_SHORT_NAMES
#include "stb_ds.h"  // For dynamic arrays


//////////////////////
// Type definitions //
//////////////////////

typedef enum __attribute__((__packed__)) {
    TYPE_STR,    // Single String
    TYPE_SELECT, // Single value of collection can be selected
    TYPE_TAG,    // Several values of collection can be selected
    TYPE_DATE,   // Simple Date (no exact timestamp)
} Datatype;

typedef union {
    char** strs; // For Select or Tag
} Type_Opts;

typedef struct {
    Datatype  type;
    Type_Opts opts;
} Column;

typedef String_View Value_Str;
typedef int Value_Select;
typedef int* Value_Tag;
typedef struct {
    uint8_t day;   // must be between 1 and 31
    uint8_t month; // must be between 1 and 12
    int16_t year;  // Negative years are BC
} Value_Date;

typedef struct {
    Column *cols; // List of columns
    void  **vals; // List of values in Column-Major order, so all values in vals[i] are of the same type
} Table;

typedef struct {
    Table       *tabs;
    String_View *names;
} Table_Defs;


///////////////
// Functions //
///////////////

Table readTabFile(const char *fpath)
{
    uint32_t size;
    char* buf = readFile(fpath, &size);
    Table tab = { .cols = NULL, .vals = NULL };

    // @TODO: Parse file as Table

    free(buf);
    return tab;
}

bool writeTabFile(const char *fpath, Table table)
{
    // @TODO
}

// Assumes that the file under the path fpath exists and can be read from
// Assumes all '.tab' files to be in the current directory
Table_Defs readDefFile(const char *fpath)
{
    uint32_t size;
    char* buf = readFile(fpath, &size);
    Table_Defs tables = { .names = NULL, .tabs = NULL };

    uint32_t i = 0;
    while (i < size) {
        Table table = { .cols = NULL, .vals = NULL };
        String_View name = {
            .count = (size_t) buf[i],
            .data  = malloc(buf[i])
        };
        memcpy(name.data, &buf[i+1], name.count);
        if (FileExists(name.data)) table = readTabFile(name.data);

        stbds_arrput(tables.tabs, table);
        stbds_arrput(tables.names, name);
        i += name.count + 1;
    }

    free(buf);
    return tables;
}

bool writeDefFile(const char *fpath, Table_Defs *tables)
{
    // @TODO
}

int main(void)
{
    int32_t win_width  = 1200;
    int32_t win_height = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(win_width, win_height, "RL");
    SetTargetFPS(60);

    float size_default = 50;
    float size_max     = size_default;
    float spacing      = 2;
    Font font = LoadFontEx("./assets/Roboto-Regular.ttf", size_max, NULL, 95);

    // Read Data
    Table_Defs tables = { .names = NULL, .tabs = NULL };
    if (!DirectoryExists("./data")) mkdir("./data", 0777);
    chdir("./data");
    if (FileExists("./tables.def")) tables = readDefFile("./tables.def");
    chdir("..");

    while (!WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        BeginDrawing();

        if (IsWindowResized()) {
            win_width  = GetScreenWidth();
            win_height = GetScreenHeight();
        }
        ClearBackground(BLACK);

        char* text = "Reading List";
        Vector2 v = MeasureTextEx(font, text, size_default, spacing);
        DrawTextEx(font, text, (Vector2){.x = (win_width - v.x)/2, .y = (win_height - size_default)/2}, size_default, spacing, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}