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

// @Study: How do we identify columns? By index? Via an ID (would have to be added)? Via the name (names would have to be unique then)?
// Currently done via index, but this might be a bad idea...
typedef struct {
    String_View name;
    Datatype    type;
    Type_Opts   opts;
} Column;

typedef String_View Value_Str;
typedef i32 Value_Select; // -1 means no element is selected, otherwise acts as index into collection of available values
typedef u32* Value_Tag;   // List of indexes. Unsigned, because -1 isn't needed to signify an empty list of selected values (list is simply empty then)
typedef struct {
    u8 day;   // must be between 1 and 31 | 0 is interpreted as an empty date
    u8 month; // must be between 1 and 12 | 0 is interpreted as an empty date
    i16 year; // Negative years are BC    | 0 is interpreted as an empty date
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