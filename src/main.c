#include <stdio.h>
#include "raylib.h"
#include "hpdf.h"

int main(void)
{
    InitWindow(800, 450, "raylib [core] example - basic window");

    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) {
        printf ("ERROR: cannot create pdf object.\n");
        return 1;
    }
    HPDF_Page page_1 = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page_1, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    HPDF_Page_BeginText(page_1);
    HPDF_SaveToFile(pdf, "test.pdf");
    HPDF_Free(pdf);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}