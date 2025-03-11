#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
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
    italicGFS = LoadFontFromMemory(".ttf", font_data_italic, sizeof(font_data_italic), 14, NULL, 0);
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
    float horizontalOffset;     // Horizontal offset for long text
    float backspaceTimer;       // Time for multi-character removal
    const char* placeholder;    // Placeholder text
} Textbox;

// Function to render textbox text
void drawTextboxText(Textbox* textbox, Color textColor) {
    const char* displayText = (textbox->textLength == 0 && !textbox->editing) ? textbox->placeholder : textbox->text;
    float textWidth = MeasureTextEx(textbox->font, displayText, textbox->fontSize, 1).x;
    float maxTextWidth = textbox->bounds.width - 10; // 5px padding on both sides

    if (textWidth > maxTextWidth) {
        textbox->horizontalOffset = textWidth - maxTextWidth;
    } else {
        textbox->horizontalOffset = 0;
    }

    Rectangle scissorRect = { textbox->bounds.x + 5, textbox->bounds.y + 5, maxTextWidth, textbox->bounds.height - 10 };
    BeginScissorMode(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);
    DrawTextEx(textbox->font, displayText, 
               (Vector2){ textbox->bounds.x + 5 - textbox->horizontalOffset, textbox->bounds.y + 5 }, 
               textbox->fontSize, 1, textColor);
    EndScissorMode();

    if (textbox->editing && textbox->cursorBlink < 0.5f) {
        float cursorX = textbox->bounds.x + 5 + textWidth - textbox->horizontalOffset;
        DrawRectangle(cursorX, textbox->bounds.y + 5, 2, textbox->fontSize, textColor);
    }
}

// Input handling for textboxes
void handleTextboxInput(Textbox* textbox) {
    int key = GetCharPressed();
    while (key > 0) {
        if (textbox->textLength < 63) {
            if (textbox->numericOnly) {
                if (key >= '0' && key <= '9') { 
                    textbox->text[textbox->textLength++] = (char)key;
                    textbox->text[textbox->textLength] = '\0';
                }
            } else {
                if (key >= 32 && key <= 126) {
                    textbox->text[textbox->textLength++] = (char)key;
                    textbox->text[textbox->textLength] = '\0';
                }
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && textbox->textLength > 0) {
        textbox->text[--textbox->textLength] = '\0';
    }
    if (IsKeyDown(KEY_BACKSPACE)) {
        textbox->backspaceTimer += GetFrameTime();
        if (textbox->backspaceTimer > 0.3f) {
            if (textbox->textLength > 0) {
                textbox->text[--textbox->textLength] = '\0';
                textbox->backspaceTimer = 0.25f;
            }
        }
    }
    if (IsKeyReleased(KEY_BACKSPACE)) {
        textbox->backspaceTimer = 0;
    }

    textbox->cursorBlink += GetFrameTime();
    if (textbox->cursorBlink > 1.0f) textbox->cursorBlink = 0.0f;
}

// Gaussian blur fragment shader 
const char* blurShaderCode = 
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec2 resolution;\n"
    "void main() {\n"
    "    vec2 texelSize = 1.0 / resolution;\n"
    "    vec4 color = vec4(0.0);\n"
    "    float weights[3] = float[](0.3, 0.2, 0.2);\n" 
    "    color += texture(texture0, fragTexCoord) * weights[0];\n"
    "    color += texture(texture0, fragTexCoord + vec2(texelSize.x, 0.0)) * weights[1];\n"
    "    color += texture(texture0, fragTexCoord - vec2(texelSize.x, 0.0)) * weights[1];\n"
    "    color += texture(texture0, fragTexCoord + vec2(0.0, texelSize.y)) * weights[1];\n"
    "    color += texture(texture0, fragTexCoord - vec2(0.0, texelSize.y)) * weights[1];\n"
    "    fragColor = color * 0.8f;\n" 
    "}\n";

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
        DrawRectangleRounded((Rectangle){ 14, 55, 150, 24 }, 0.5f, 6, toHex("#222329"));
        DrawRectangleRounded((Rectangle){ 170, 55, 24, 24 }, 0.5f, 6, toHex("#222329"));
        DrawRectangle(181, 61, 2, 12, toHex("#979EBB"));
        DrawRectangle(176, 66, 12, 2, toHex("#979EBB"));
        DrawRectangleRounded((Rectangle){ 274, 316, 65, 30 }, 0.5f, 6, toHex("#222329"));
        DrawTextEx(boldGFS_h1, "noctivox", (Vector2){ 14, 8 }, 20, 1, toHex("#FFFFFF"));
        DrawTextEx(boldGFS_h1, "bpm:", (Vector2){ 226, 318 }, 20, 1, toHex("#9CA2B7"));
        DrawTextEx(italicGFS, "a virtual piano player", (Vector2){ 14, 30 }, 14, 1, toHex("#FFFFFF"));
    EndTextureMode();

    // Render texture for full scene
    RenderTexture2D sceneTexture = LoadRenderTexture(screenWidth, screenHeight);
    bool sceneTextureNeedsUpdate = true;

    // Load blur shader
    Shader blurShader = LoadShaderFromMemory(0, blurShaderCode);
    int resolutionLoc = GetShaderLocation(blurShader, "resolution");
    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(blurShader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    // Textboxes with placeholders
    Textbox songInput = { 
        { 14, 55, 150, 24 }, "", 0, false, 0.0f, 
        false, italicGFS, 14, 0, 0, "search for songs" 
    };
    Textbox bpmInput = { 
        { 274, 319, 60, 30 }, "", 0, false, 0.0f, 
        true, italicGFS, 14, 0, 0, "100" 
    };

    // Define selectable areas and toggle elements
    Rectangle plusButton = { 170, 55, 24, 24 };
    Rectangle uploadPanel = { 160, 45, 400, 270 };
    Rectangle closeButton = { 540, 45, 20, 20 }; // Top-right corner of uploadPanel
    bool isUploadVisible = false;

    // Bound variables
    char songName[64] = "";
    int bpm = 100;

    while (!WindowShouldClose()) {
        Vector2 mousePosition = GetMousePosition();

        // Input handling based on z-index
        if (isUploadVisible) {
            // Upload panel layer (above blur)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mousePosition, closeButton)) {
                    isUploadVisible = false;
                    sceneTextureNeedsUpdate = true; // Redraw base layer when closing
                }
            }
            // Cursor for upload panel
            if (CheckCollisionPointRec(mousePosition, closeButton)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
        } else {
            // Base layer (beneath blur)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                bool wasEditing = songInput.editing || bpmInput.editing;
                songInput.editing = CheckCollisionPointRec(mousePosition, songInput.bounds);
                bpmInput.editing = CheckCollisionPointRec(mousePosition, bpmInput.bounds);

                if (CheckCollisionPointRec(mousePosition, plusButton)) {
                    isUploadVisible = true;
                    sceneTextureNeedsUpdate = false; // Stop updating beneath blur
                }

                if (songInput.editing && !wasEditing && songInput.textLength == 0) {
                    songInput.text[0] = '\0';
                    songInput.textLength = 0;
                }
                if (bpmInput.editing && !wasEditing && bpmInput.textLength == 0) {
                    bpmInput.text[0] = '\0';
                    bpmInput.textLength = 0;
                }
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (songInput.editing && songInput.textLength == 0) {
                    strcpy(songInput.text, songInput.placeholder);
                    songInput.textLength = strlen(songInput.placeholder);
                }
                if (bpmInput.editing && bpmInput.textLength == 0) {
                    strcpy(bpmInput.text, bpmInput.placeholder);
                    bpmInput.textLength = strlen(bpmInput.placeholder);
                }
                songInput.editing = false;
                bpmInput.editing = false;
            }

            if (songInput.editing) handleTextboxInput(&songInput);
            if (bpmInput.editing) handleTextboxInput(&bpmInput);

            // Cursor for base layer
            if (songInput.editing || bpmInput.editing) {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
            } else if (CheckCollisionPointRec(mousePosition, songInput.bounds) ||
                       CheckCollisionPointRec(mousePosition, bpmInput.bounds) ||
                       CheckCollisionPointRec(mousePosition, plusButton)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
        }

        // Update bound variables
        if (songInput.textLength > 0 && strcmp(songInput.text, songInput.placeholder) != 0) {
            strcpy(songName, songInput.text);
        } else {
            songName[0] = '\0';
        }
        if (bpmInput.textLength > 0 && strcmp(bpmInput.text, bpmInput.placeholder) != 0) {
            bpm = atoi(bpmInput.text);
        } else {
            bpm = atoi(bpmInput.placeholder);
        }

        // Render scene to texture only when needed
        if (sceneTextureNeedsUpdate && !isUploadVisible) {
            BeginTextureMode(sceneTexture);
                DrawTextureRec(backgroundTexture.texture, 
                               (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                               (Vector2){ 0, 0 }, WHITE);
                drawTextboxText(&songInput, songInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"));
                drawTextboxText(&bpmInput, bpmInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"));
            EndTextureMode();
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (isUploadVisible) {
                // Draw blurred scene (cached)
                BeginShaderMode(blurShader);
                    DrawTextureRec(sceneTexture.texture, 
                                   (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                                   (Vector2){ 0, 0 }, WHITE);
                EndShaderMode();

                // Draw upload panel above blur
                DrawRectangleRec(uploadPanel, toHex("#272930"));
                // Draw close button
                DrawRectangleRec(closeButton, toHex("#494D5A"));
                DrawTextEx(boldGFS_h2, "X", (Vector2){ closeButton.x + 5, closeButton.y + 3 }, 14, 1, toHex("#FFFFFF"));
            } else {
                // Normal render without blur
                DrawTextureRec(sceneTexture.texture, 
                               (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                               (Vector2){ 0, 0 }, WHITE);
            }
        EndDrawing();
    }

    // Cleanup
    UnloadRenderTexture(backgroundTexture);
    UnloadRenderTexture(sceneTexture);
    UnloadShader(blurShader);
    unloadFonts();
    CloseWindow();
    return 0;
}