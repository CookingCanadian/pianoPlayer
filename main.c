#include "raylib.h"
#include <stdio.h>

// Helper function to convert hex colour (e.g., "#FFFFFF") to raylib color
Color toHex(const char* hex) {
    if (hex[0] == '#') hex++;
    
    unsigned int r, g, b;
    sscanf(hex, "%2x%2x%2x", &r, &g, &b); 
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 }; 
}

int main(void) {
    const int screenWidth = 720;
    const int screenHeight = 360;

    InitWindow(screenWidth, screenHeight, "noctivox | a virtual piano player");
    SetTargetFPS(60);

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
    EndTextureMode();

    while (!WindowShouldClose()) {
        BeginDrawing();
            DrawTextureRec(backgroundTexture.texture, 
                          (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                          (Vector2){ 0, 0 }, 
                          WHITE);


        EndDrawing();
    }

    // Cleanup
    UnloadRenderTexture(backgroundTexture);
    CloseWindow();
    return 0;
}