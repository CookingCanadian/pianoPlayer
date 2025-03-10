#include "raylib.h"
#include <stdio.h>

// Helper function to convert hex color (e.g., "#FFFFFF") to raylib color
Color toHex(const char* hex) {
    if (hex[0] == '#') hex++;
    
    unsigned int r, g, b;
    sscanf(hex, "%2x%2x%2x", &r, &g, &b); 
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 }; 
}

int main(void) {
    const int screenWidth = 720;
    const int screenHeight = 360;

    InitWindow(screenWidth, screenHeight, "noctivox");
    SetTargetFPS(60);

    // Static background buffer
    RenderTexture2D backgroundTexture = LoadRenderTexture(screenWidth, screenHeight);

    // Draw background once at startup for cheap operation
    BeginTextureMode(backgroundTexture);
        ClearBackground(toHex("#191A1F"));
        DrawRectangle(210, 48, 510, 312, toHex("#121215")); 

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