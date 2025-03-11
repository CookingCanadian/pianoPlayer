#include <stdio.h>
#include "raylib.h"
#include "resources/GFSNeohellenic_Italic.h"
#include "resources/GFSNeohellenic_Bold.h"
#include "resources/GFSNeohellenic_BoldItalic.h"

// Convert hex color (e.g., "#FFFFFF") to raylib Color
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

// Load fonts from memory
void loadFonts(void) {
    italicGFS = LoadFontFromMemory(".ttf", font_data_italic, sizeof(font_data_italic), 12, NULL, 0);
    boldGFS_h1 = LoadFontFromMemory(".ttf", font_data_bold, sizeof(font_data_bold), 20, NULL, 0);
    boldGFS_h2 = LoadFontFromMemory(".ttf", font_data_bold, sizeof(font_data_bold), 14, NULL, 0);
    boldItalicGFS = LoadFontFromMemory(".ttf", font_data_bold_italic, sizeof(font_data_bold_italic), 12, NULL, 0);
}

// Unload fonts
void unloadFonts(void) {
    UnloadFont(italicGFS);
    UnloadFont(boldGFS_h1);
    UnloadFont(boldGFS_h2);
    UnloadFont(boldItalicGFS);
}

// Textbox struct for editable input fields
typedef struct {
    Rectangle bounds;           // Position and size
    char text[64];              // Text buffer (63 chars + null terminator)
    int textLength;             // Current length of text
    bool editing;               // Is this textbox active?
    float cursorBlink;          // Blink timer for cursor
    bool numericOnly;           // Restrict to positive nonzero numbers?
    Font font;                  // Custom font
    float fontSize;             // Font size
    Color backgroundColor;      // Background color
    float cornerRadius;         // Radius for rounded corners (0.0 to 1.0)
} Textbox;

int main(void) {
    const int screenWidth = 720;
    const int screenHeight = 360;

    InitWindow(screenWidth, screenHeight, "noctivox | a virtual piano player");
    SetTargetFPS(60);

    // Load fonts
    loadFonts();

    // Static background buffer
    RenderTexture2D backgroundTexture = LoadRenderTexture(screenWidth, screenHeight);
    BeginTextureMode(backgroundTexture);
        ClearBackground(toHex("#191A1F"));
        DrawRectangle(210, 48, 510, 312, toHex("#121215"));
        DrawRectangle(216, 54, 498, 246, toHex("#272930"));
        DrawRectangle(216, 306, 498, 48, toHex("#272930"));
        DrawRectangle(14, 48, 180, 1, toHex("#272C3C"));
        DrawRectangle(222, 85, 210, 1, toHex("#494D5A"));
        DrawRectangle(355, 312, 1, 36, toHex("#494D5A"));
        DrawTextEx(boldGFS_h1, "noctivox", (Vector2){ 14, 8 }, 20, 1, toHex("#FFFFFF"));
        DrawTextEx(boldGFS_h1, "tempo:", (Vector2){ 226, 318 }, 20, 1, toHex("#9CA2B7"));
        DrawTextEx(italicGFS, "a virtual piano player", (Vector2){ 14, 30 }, 12, 1, toHex("#FFFFFF"));
    EndTextureMode();

    // Textboxes for file (song) searching and tempo input
    Textbox songInput = { 
        { 14, 55, 150, 24 }, "", 0, false, 0.0f, 
        false, italicGFS, 12, toHex("#222329"), 0.5f 
    };
    Textbox tempoInput = { 
        { 280, 315, 60, 30 }, "", 0, false, 0.0f, 
        true, italicGFS, 12, toHex("#222329"), 0.5f 
    };

    while (!WindowShouldClose()) {
        Vector2 mousePosition = GetMousePosition();

        // Textbox cursor handling
        if (CheckCollisionPointRec(mousePosition, songInput.bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            songInput.editing = true;
            tempoInput.editing = false;
            SetMouseCursor(MOUSE_CURSOR_IBEAM);
        } else if (CheckCollisionPointRec(mousePosition, tempoInput.bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { 
            songInput.editing = false;
            tempoInput.editing = true;
            SetMouseCursor(MOUSE_CURSOR_IBEAM);
        } else if (IsKeyPressed(KEY_ENTER)) {
            songInput.editing = false;
            tempoInput.editing = false;
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        // Handle input for active textbox
        Textbox* activeBox = songInput.editing ? &songInput : (tempoInput.editing ? &tempoInput : NULL);
        if (activeBox) {
            int key = GetCharPressed();
            while (key > 0) {
                if (activeBox->textLength < 63) {
                    if (activeBox->numericOnly) {
                        if (key >= '1' && key <= '9') {
                            activeBox->text[activeBox->textLength] = (char)key;
                            activeBox->textLength++;
                            activeBox->text[activeBox->textLength] = '\0';
                        }
                    } else {
                        if (key >= 32 && key <= 126) {
                            activeBox->text[activeBox->textLength] = (char)key;
                            activeBox->textLength++;
                            activeBox->text[activeBox->textLength] = '\0';
                        }
                    }
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && activeBox->textLength > 0) {
                activeBox->textLength--;
                activeBox->text[activeBox->textLength] = '\0';
            }

            activeBox->cursorBlink += GetFrameTime();
            if (activeBox->cursorBlink > 1.0f) activeBox->cursorBlink = 0.0f;
        }

        // Reset cursor if not over any textbox
        if (!CheckCollisionPointRec(mousePosition, songInput.bounds) && !CheckCollisionPointRec(mousePosition, tempoInput.bounds) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (songInput.editing) {
                songInput.editing = false;
            }
            if (tempoInput.editing) {
                tempoInput.editing = false;
            }
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        BeginDrawing();
            DrawTextureRec(backgroundTexture.texture, 
                          (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                          (Vector2){ 0, 0 }, WHITE);

            // Draw songInput
            DrawRectangleRounded(songInput.bounds, songInput.cornerRadius, 8, songInput.backgroundColor);
            DrawTextEx(songInput.font, songInput.text, 
                       (Vector2){ songInput.bounds.x + 5, songInput.bounds.y + 5 }, 
                       songInput.fontSize, 1, toHex("#D0D0D0"));
            if (songInput.editing && songInput.cursorBlink < 0.5f) {
                float textWidth = MeasureTextEx(songInput.font, songInput.text, songInput.fontSize, 1).x;
                DrawRectangle(songInput.bounds.x + 5 + textWidth, songInput.bounds.y + 5, 2, 14, toHex("#D0D0D0"));
            }

            // Draw tempoInput
            DrawRectangleRounded(tempoInput.bounds, tempoInput.cornerRadius, 8, tempoInput.backgroundColor);
            DrawTextEx(tempoInput.font, tempoInput.text, 
                       (Vector2){ tempoInput.bounds.x + 5, tempoInput.bounds.y + 8 }, 
                       tempoInput.fontSize, 1, toHex("#D0D0D0"));
            if (tempoInput.editing && tempoInput.cursorBlink < 0.5f) {
                float textWidth = MeasureTextEx(tempoInput.font, tempoInput.text, tempoInput.fontSize, 1).x;
                DrawRectangle(tempoInput.bounds.x + 5 + textWidth, tempoInput.bounds.y + 8, 2, 14, toHex("#D0D0D0"));
            }
        EndDrawing();
    }

    // Cleanup
    UnloadRenderTexture(backgroundTexture);
    unloadFonts();
    CloseWindow();
    return 0;
}