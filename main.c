#include <stdio.h>
#include "raylib.h"
#include "GFSNeohellenic_Italic.h"
#include "GFSNeohellenic_Bold.h"
#include "GFSNeohellenic_BoldItalic.h"

// Function to convert hex color (e.g., "#FFFFFF") to raylib Color
Color toHex(const char* hex) {
    if (hex[0] == '#') hex++;
    
    unsigned int r, g, b;
    sscanf(hex, "%2x%2x%2x", &r, &g, &b); 
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 }; 
}

// Global font variables
Font italicGFS;
Font boldGFS_h1;
Font boldGFS_h2;
Font boldItalicGFS;

// Function to load fonts
void loadFonts() {
    italicGFS = LoadFontFromMemory(".ttf", font_data_italic, sizeof(font_data_italic), 12, NULL, 0);
    boldGFS_h1 = LoadFontFromMemory(".ttf", font_data_bold, sizeof(font_data_bold), 20, NULL, 0);
    boldGFS_h2 = LoadFontFromMemory(".ttf", font_data_bold, sizeof(font_data_bold), 14, NULL, 0);
    boldItalicGFS = LoadFontFromMemory(".ttf", font_data_bold_italic, sizeof(font_data_bold_italic), 12, NULL, 0);
}

// Function to unload fonts
void unloadFonts() {
    UnloadFont(italicGFS);
    UnloadFont(boldGFS_h1);
    UnloadFont(boldGFS_h2);
    UnloadFont(boldItalicGFS);
}

int main(void) {
    const int screenWidth = 720;
    const int screenHeight = 360;

    InitWindow(screenWidth, screenHeight, "noctivox | a virtual piano player");
    SetTargetFPS(60);

    // Load fonts 
    loadFonts();

    // Static background buffer
    RenderTexture2D backgroundTexture = LoadRenderTexture(screenWidth, screenHeight);

    // Draw background once at startup for cheap operation
    BeginTextureMode(backgroundTexture);
        // Main colours
        ClearBackground(toHex("#191A1F"));
        DrawRectangle(210, 48, 510, 312, toHex("#121215"));         
        DrawRectangle(216, 54, 498, 246, toHex("#272930")); 
        DrawRectangle(216, 306, 498, 48, toHex("#272930")); 

        // Decor
        DrawRectangle(14, 48, 180, 1, toHex("#272C3C")); 
        DrawRectangle(222, 85, 210, 1, toHex("#494D5A")); 
        DrawRectangle(355, 312, 1, 36, toHex("#494D5A")); 

        // Unchanging Text
        DrawTextEx(boldGFS_h1, "noctivox", (Vector2){ 14, 8 }, 20, 1, toHex("#FFFFFF"));
        DrawTextEx(boldGFS_h1, "tempo:", (Vector2){ 226, 318 }, 20, 1, toHex("#9CA2B7"));
        DrawTextEx(italicGFS, "a virtual piano player", (Vector2){ 14, 30 }, 12, 1, toHex("#FFFFFF"));
    EndTextureMode();

    while (!WindowShouldClose()) {
        Vector2 mousePosition = GetMousePosition();

        BeginDrawing();
            DrawTextureRec(backgroundTexture.texture, (Rectangle){ 0, 0, screenWidth, -screenHeight }, (Vector2){ 0, 0 }, WHITE);

        
        EndDrawing();
    }

    // Cleanup
    UnloadRenderTexture(backgroundTexture);
    unloadFonts(); 
    CloseWindow();
    return 0;
}