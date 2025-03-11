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
    Textbox songSearchInput = { 
        { 14, 55, 150, 24 }, "", 0, false, 0.0f, 
        false, italicGFS, 14, 0, 0, "search for songs" 
    };
    Textbox bpmValueEdit = { 
        { 274, 319, 60, 30 }, "", 0, false, 0.0f, 
        true, italicGFS, 14, 0, 0, "100" 
    };

    // Define selectable areas and toggle elements
    Rectangle plusButton = { 170, 55, 24, 24 };
    Rectangle uploadPanel = { 160, 45, 400, 270 };
    Rectangle cancelButton = { 343, 276, 96, 30 }; 
    Rectangle saveButton = { 454, 276, 96, 30 };
    Rectangle uploadMidi = { 454, 66, 96, 20 };
    Rectangle pasteArea = { 170, 90, 380, 120 };
    Rectangle songNameInput = { 170, 226, 297, 20 };
    Rectangle bpmValueInput = { 486, 226, 64, 20 };
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
                if (CheckCollisionPointRec(mousePosition, cancelButton)) {
                    isUploadVisible = false;
                    sceneTextureNeedsUpdate = true; // Redraw base layer when closing
                }
            }
            // Cursor for upload panel
            if (CheckCollisionPointRec(mousePosition, cancelButton)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
        } else {
            // Base layer (beneath blur)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                bool wasEditing = songSearchInput.editing || bpmValueEdit.editing;
                songSearchInput.editing = CheckCollisionPointRec(mousePosition, songSearchInput.bounds);
                bpmValueEdit.editing = CheckCollisionPointRec(mousePosition, bpmValueEdit.bounds);

                if (CheckCollisionPointRec(mousePosition, plusButton)) {
                    isUploadVisible = true;
                    sceneTextureNeedsUpdate = false; // Stop updating beneath blur
                }

                if (songSearchInput.editing && !wasEditing && songSearchInput.textLength == 0) {
                    songSearchInput.text[0] = '\0';
                    songSearchInput.textLength = 0;
                }
                if (bpmValueEdit.editing && !wasEditing && bpmValueEdit.textLength == 0) {
                    bpmValueEdit.text[0] = '\0';
                    bpmValueEdit.textLength = 0;
                }
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (songSearchInput.editing && songSearchInput.textLength == 0) {
                    strcpy(songSearchInput.text, songSearchInput.placeholder);
                    songSearchInput.textLength = strlen(songSearchInput.placeholder);
                }
                if (bpmValueEdit.editing && bpmValueEdit.textLength == 0) {
                    strcpy(bpmValueEdit.text, bpmValueEdit.placeholder);
                    bpmValueEdit.textLength = strlen(bpmValueEdit.placeholder);
                }
                songSearchInput.editing = false;
                bpmValueEdit.editing = false;
            }

            if (songSearchInput.editing) handleTextboxInput(&songSearchInput);
            if (bpmValueEdit.editing) handleTextboxInput(&bpmValueEdit);

            // Cursor for base layer
            if (songSearchInput.editing || bpmValueEdit.editing) {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
            } else if (CheckCollisionPointRec(mousePosition, songSearchInput.bounds) ||
                       CheckCollisionPointRec(mousePosition, bpmValueEdit.bounds) ||
                       CheckCollisionPointRec(mousePosition, plusButton)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
        }

        // Update bound variables
        if (songSearchInput.textLength > 0 && strcmp(songSearchInput.text, songSearchInput.placeholder) != 0) {
            strcpy(songName, songSearchInput.text);
        } else {
            songName[0] = '\0';
        }
        if (bpmValueEdit.textLength > 0 && strcmp(bpmValueEdit.text, bpmValueEdit.placeholder) != 0) {
            bpm = atoi(bpmValueEdit.text);
        } else {
            bpm = atoi(bpmValueEdit.placeholder);
        }

        // Render scene to texture only when needed
        if (sceneTextureNeedsUpdate && !isUploadVisible) {
            BeginTextureMode(sceneTexture);
                DrawTextureRec(backgroundTexture.texture, 
                               (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                               (Vector2){ 0, 0 }, WHITE);
                drawTextboxText(&songSearchInput, songSearchInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"));
                drawTextboxText(&bpmValueEdit, bpmValueEdit.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"));
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
                DrawRectangleRounded(pasteArea, 0.1f, 6, toHex("#2C2E36"));  
                DrawRectangleRounded(songNameInput, 0.5f, 6, toHex("#222329"));  
                DrawRectangleRounded(bpmValueInput, 0.5f, 6, toHex("#222329")); 
                DrawRectangleRounded(uploadMidi, 0.5f, 6, toHex("#222329")); 
                DrawRectangleRounded(cancelButton, 0.5f, 6, toHex("#222329"));
                DrawRectangleRounded(saveButton, 0.5f, 6, toHex("#222329"));
                DrawRectangle(170, 73, 150, 1, toHex("#494D5A"));
                DrawRectangle(476, 226, 1, 20, toHex("#494D5A"));
                DrawTextEx(boldGFS_h1, "upload song", (Vector2){ 170, 50 }, 20, 1, toHex("#F0F2FE"));
                DrawTextEx(boldGFS_h2, "upload midi", (Vector2){ 470, 69 }, 14, 1, toHex("#9CA2B7"));
                DrawTextEx(italicGFS, "paste music sheet:", (Vector2){ 170, 74 }, 14, 1, toHex("#979EBB"));
                DrawTextEx(italicGFS, "or", (Vector2){ 438, 74 }, 14, 1, toHex("#979EBB"));
                DrawTextEx(italicGFS, "song name:", (Vector2){ 170, 212 }, 14, 1, toHex("#979EBB"));
                DrawTextEx(italicGFS, "bpm:", (Vector2){ 486, 212 }, 14, 1, toHex("#979EBB"));
                DrawTextEx(boldGFS_h2, "cancel", (Vector2){ 375, 284 }, 14, 1, toHex("#F0F2FE"));
                DrawTextEx(boldGFS_h2, "save", (Vector2){ 491, 284 }, 14, 1, toHex("#F0F2FE"));
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