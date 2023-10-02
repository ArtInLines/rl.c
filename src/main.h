#ifndef MAIN_H_
#define MAIN_H_

#include "util.h"
#include "sv.h"

typedef enum __attribute__((__packed__)) {
    TYPE_STR,    // Single String
    TYPE_SELECT, // Single value of collection can be selected
    TYPE_TAG,    // Several values of collection can be selected
    TYPE_DATE,   // Simple Date (no exact timestamp)
    TYPE_LEN,    // Amount of elements in this enum
} Datatype;

typedef union {
    String_View *strs; // For Select or Tag
} Type_Opts;

typedef struct {
    Datatype  type;
    Type_Opts opts;
} Column;

typedef String_View Value_Str;
typedef i32 Value_Select;
typedef i32* Value_Tag;
typedef struct {
    u8 day;   // must be between 1 and 31
    u8 month; // must be between 1 and 12
    i16 year;  // Negative years are BC
} Value_Date;

typedef union {
    Value_Str    *strs;
    Value_Select *selects;
    Value_Tag    *tags;
    Value_Date   *dates;
} Values;

typedef struct {
    Column *cols; // List of columns
    Values *vals; // List of values in Column-Major order, so all values in vals[i] are of the same type
} Table;

typedef struct {
    // The attributes are parralel arrays
    Table       *tabs;
    String_View *names;
} Table_Defs;

#endif // MAIN_H_