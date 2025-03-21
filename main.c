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

// Textbox struct with selection support
typedef struct {
    Rectangle bounds;           // Position and size
    char text[256];             // Buffer for larger text
    int textLength;             // Current length of text
    bool editing;               // Is this textbox active?
    float cursorBlink;          // Blink timer for cursor
    bool numericOnly;           // Restrict to positive nonzero numbers?
    Font font;                  // Custom font
    float fontSize;             // Font size
    float horizontalOffset;     // Horizontal offset for long text
    float verticalOffset;       // Vertical offset for scrolling
    float backspaceTimer;       // Time for multi-character removal
    const char* placeholder;    // Placeholder text
    int cursorPos;              // Cursor position in text
    int selectionStart;         // Start of text selection (-1 if none)
    int selectionEnd;           // End of text selection
} Textbox;

// Function to render textbox text with scrolling and selection
void drawTextboxText(Textbox* textbox, Color textColor, bool isPasteArea) {
    const char* displayText = (textbox->textLength == 0 && !textbox->editing) ? textbox->placeholder : textbox->text;
    Vector2 textSize = MeasureTextEx(textbox->font, displayText, textbox->fontSize, 1);
    float maxTextWidth = isPasteArea ? textbox->bounds.width - 15 : textbox->bounds.width - 10; // Scrollbar space for paste area
    float maxTextHeight = textbox->bounds.height - 10;

    // Horizontal offset
    if (textSize.x > maxTextWidth) {
        textbox->horizontalOffset = textSize.x - maxTextWidth;
    } else {
        textbox->horizontalOffset = 0;
    }

    // Scissor mode for clipping
    Rectangle scissorRect = { textbox->bounds.x + 5, textbox->bounds.y + 5, maxTextWidth, maxTextHeight };
    BeginScissorMode(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);

    // Draw text with vertical offset and line breaks (for paste area)
    float yPos = textbox->bounds.y + 5 - (isPasteArea ? textbox->verticalOffset : 0);
    char* textCopy = strdup(displayText);
    char* line = isPasteArea ? strtok(textCopy, "\n") : textCopy;
    int charIndex = 0;
    while (line != NULL) {
        DrawTextEx(textbox->font, line, 
                   (Vector2){ textbox->bounds.x + 5 - textbox->horizontalOffset, yPos }, 
                   textbox->fontSize, 1, textColor);
        if (isPasteArea) {
            yPos += textbox->fontSize + 2; // Line spacing
            line = strtok(NULL, "\n");
        } else {
            break;
        }
    }

    // Draw selection highlight
    if (textbox->selectionStart != -1 && textbox->selectionEnd != -1 && textbox->selectionStart != textbox->selectionEnd) {
        int start = textbox->selectionStart < textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
        int end = textbox->selectionStart > textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
        yPos = textbox->bounds.y + 5 - (isPasteArea ? textbox->verticalOffset : 0);
        charIndex = 0;
        line = isPasteArea ? strtok(strdup(displayText), "\n") : displayText;
        while (line != NULL) {
            int lineLen = strlen(line);
            if (charIndex + lineLen >= start && charIndex <= end) {
                float startX = textbox->bounds.x + 5 + MeasureTextEx(textbox->font, line, textbox->fontSize, 1).x * (start - charIndex) / (float)lineLen - textbox->horizontalOffset;
                float endX = textbox->bounds.x + 5 + MeasureTextEx(textbox->font, line, textbox->fontSize, 1).x * (end > charIndex + lineLen ? 1 : (end - charIndex) / (float)lineLen) - textbox->horizontalOffset;
                DrawRectangle(startX, yPos, endX - startX, textbox->fontSize, Fade(toHex("#FFFFFF"), 0.3f));
            }
            charIndex += lineLen + (isPasteArea ? 1 : 0);
            yPos += textbox->fontSize + (isPasteArea ? 2 : 0);
            if (!isPasteArea) break;
            line = strtok(NULL, "\n");
        }
    }

    // Draw cursor
    if (textbox->editing && textbox->cursorBlink < 0.5f) {
        yPos = textbox->bounds.y + 5 - (isPasteArea ? textbox->verticalOffset : 0);
        charIndex = 0;
        line = isPasteArea ? strtok(strdup(displayText), "\n") : displayText;
        while (line != NULL) {
            int lineLen = strlen(line);
            if (charIndex + lineLen >= textbox->cursorPos) {
                float cursorX = textbox->bounds.x + 5 + MeasureTextEx(textbox->font, line, textbox->fontSize, 1).x * (textbox->cursorPos - charIndex) / (float)lineLen - textbox->horizontalOffset;
                DrawRectangle(cursorX, yPos, 2, textbox->fontSize, textColor);
                break;
            }
            charIndex += lineLen + (isPasteArea ? 1 : 0);
            yPos += textbox->fontSize + (isPasteArea ? 2 : 0);
            if (!isPasteArea) break;
            line = strtok(NULL, "\n");
        }
    }
    free(textCopy);
    EndScissorMode();

    // Draw scrollbar (only for paste area)
    if (isPasteArea) {
        float totalHeight = yPos - (textbox->bounds.y + 5 - textbox->verticalOffset);
        if (totalHeight > maxTextHeight) {
            float scrollBarHeight = maxTextHeight * maxTextHeight / totalHeight;
            float scrollBarY = textbox->bounds.y + 5 + (textbox->verticalOffset * (maxTextHeight - scrollBarHeight) / (totalHeight - maxTextHeight));
            DrawRectangle(textbox->bounds.x + textbox->bounds.width - 10, scrollBarY, 5, scrollBarHeight, toHex("#494D5A"));
        }
    }
}

// Input handling with selection and scrolling
void handleTextboxInput(Textbox* textbox, bool isPasteArea) {
    Vector2 mousePos = GetMousePosition();
    bool mouseOver = CheckCollisionPointRec(mousePos, textbox->bounds);

    // Handle clicking to set cursor or start selection
    if (mouseOver && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        textbox->selectionStart = -1;
        textbox->selectionEnd = -1;
        float xOffset = mousePos.x - (textbox->bounds.x + 5) + textbox->horizontalOffset;
        float yOffset = mousePos.y - (textbox->bounds.y + 5) + (isPasteArea ? textbox->verticalOffset : 0);
        int charIndex = 0;
        char* line = isPasteArea ? strtok(strdup(textbox->text), "\n") : textbox->text;
        int lineNum = isPasteArea ? (int)(yOffset / (textbox->fontSize + 2)) : 0;
        for (int i = 0; i < lineNum && line && isPasteArea; i++) {
            charIndex += strlen(line) + 1;
            line = strtok(NULL, "\n");
        }
        if (line) {
            float lineWidth = MeasureTextEx(textbox->font, line, textbox->fontSize, 1).x;
            int pos = (int)((xOffset / lineWidth) * strlen(line));
            textbox->cursorPos = charIndex + (pos < strlen(line) ? pos : strlen(line));
        } else {
            textbox->cursorPos = textbox->textLength;
        }
    }

    // Handle dragging for selection
    if (mouseOver && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float xOffset = mousePos.x - (textbox->bounds.x + 5) + textbox->horizontalOffset;
        float yOffset = mousePos.y - (textbox->bounds.y + 5) + (isPasteArea ? textbox->verticalOffset : 0);
        int lineNum = isPasteArea ? (int)(yOffset / (textbox->fontSize + 2)) : 0;
        int charIndex = 0;
        char* line = isPasteArea ? strtok(strdup(textbox->text), "\n") : textbox->text;
        for (int i = 0; i < lineNum && line && isPasteArea; i++) {
            charIndex += strlen(line) + 1;
            line = strtok(NULL, "\n");
        }
        if (line) {
            float lineWidth = MeasureTextEx(textbox->font, line, textbox->fontSize, 1).x;
            int pos = (int)((xOffset / lineWidth) * strlen(line));
            textbox->selectionEnd = charIndex + (pos < strlen(line) ? pos : strlen(line));
            if (textbox->selectionStart == -1) textbox->selectionStart = textbox->cursorPos;
        }
    }

    int key = GetCharPressed();
    while (key > 0) {
        if (textbox->textLength < 255) {
            if (textbox->numericOnly) {
                if (key >= '0' && key <= '9') { 
                    textbox->text[textbox->textLength++] = (char)key;
                    textbox->text[textbox->textLength] = '\0';
                    textbox->cursorPos++;
                }
            } else {
                if ((key >= 32 && key <= 126) || (isPasteArea && key == '\n')) {
                    textbox->text[textbox->textLength++] = (char)key;
                    textbox->text[textbox->textLength] = '\0';
                    textbox->cursorPos++;
                }
            }
        }
        key = GetCharPressed();
    }

    // Paste support (Ctrl+V)
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
        const char* clipboard = GetClipboardText();
        if (clipboard) {
            int len = strlen(clipboard);
            int spaceLeft = 255 - textbox->textLength;
            int charsToCopy = (len < spaceLeft) ? len : spaceLeft;
            if (textbox->numericOnly) {
                for (int i = 0; i < charsToCopy; i++) {
                    if (clipboard[i] >= '0' && clipboard[i] <= '9') {
                        textbox->text[textbox->textLength++] = clipboard[i];
                        textbox->cursorPos++;
                    }
                }
            } else {
                strncpy(textbox->text + textbox->textLength, clipboard, charsToCopy);
                textbox->textLength += charsToCopy;
                textbox->cursorPos += charsToCopy;
            }
            textbox->text[textbox->textLength] = '\0';
        }
    }

    // Ctrl+A to select all
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_A)) {
        textbox->selectionStart = 0;
        textbox->selectionEnd = textbox->textLength;
        textbox->cursorPos = textbox->textLength;
    }

    // Handle backspace with selection
    if (IsKeyPressed(KEY_BACKSPACE) && textbox->textLength > 0) {
        if (textbox->selectionStart != -1 && textbox->selectionEnd != -1 && textbox->selectionStart != textbox->selectionEnd) {
            int start = textbox->selectionStart < textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
            int end = textbox->selectionStart > textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
            int lenToRemove = end - start;
            memmove(textbox->text + start, textbox->text + end, textbox->textLength - end + 1);
            textbox->textLength -= lenToRemove;
            textbox->cursorPos = start;
            textbox->selectionStart = textbox->selectionEnd = -1;
        } else if (textbox->cursorPos > 0) {
            memmove(textbox->text + textbox->cursorPos - 1, textbox->text + textbox->cursorPos, textbox->textLength - textbox->cursorPos + 1);
            textbox->textLength--;
            textbox->cursorPos--;
        }
    }
    if (IsKeyDown(KEY_BACKSPACE)) {
        textbox->backspaceTimer += GetFrameTime();
        if (textbox->backspaceTimer > 0.3f && textbox->cursorPos > 0) {
            memmove(textbox->text + textbox->cursorPos - 1, textbox->text + textbox->cursorPos, textbox->textLength - textbox->cursorPos + 1);
            textbox->textLength--;
            textbox->cursorPos--;
            textbox->backspaceTimer = 0.25f;
        }
    }
    if (IsKeyReleased(KEY_BACKSPACE)) {
        textbox->backspaceTimer = 0;
    }

    // Scrolling with mouse wheel (only for paste area)
    if (isPasteArea && mouseOver) {
        float wheel = GetMouseWheelMove();
        float totalHeight = 0;
        char* line = strtok(strdup(textbox->text), "\n");
        while (line) {
            totalHeight += textbox->fontSize + 2;
            line = strtok(NULL, "\n");
        }
        float maxHeight = textbox->bounds.height - 10;
        if (totalHeight > maxHeight) {
            textbox->verticalOffset -= wheel * 20.0f;
            if (textbox->verticalOffset < 0) textbox->verticalOffset = 0;
            if (textbox->verticalOffset > totalHeight - maxHeight) textbox->verticalOffset = totalHeight - maxHeight;
        } else {
            textbox->verticalOffset = 0;
        }
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

    // Textboxes with placeholders (base layer)
    Textbox songSearchInput = { 
        { 14, 55, 150, 24 }, "", 0, false, 0.0f, 
        false, italicGFS, 14, 0, 0, 0, "search for songs", 0, -1, -1 
    };
    Textbox bpmValueEdit = { 
        { 274, 319, 60, 30 }, "", 0, false, 0.0f, 
        true, italicGFS, 14, 0, 0, 0, "100", 0, -1, -1 
    };

    // Textboxes for upload panel
    Textbox pasteAreaInput = { 
        { 170, 90, 380, 120 }, "", 0, false, 0.0f, 
        false, italicGFS, 14, 0, 0, 0, "", 0, -1, -1 
    };
    Textbox songNameInput = { 
        { 170, 226, 297, 20 }, "", 0, false, 0.0f, 
        false, italicGFS, 14, 0, 0, 0, "", 0, -1, -1 
    };
    Textbox bpmValueInput = { 
        { 486, 226, 64, 20 }, "", 0, false, 0.0f, 
        true, italicGFS, 14, 0, 0, 0, "", 0, -1, -1 
    };

    // Define selectable areas and toggle elements
    Rectangle plusButton = { 170, 55, 24, 24 };
    Rectangle uploadPanel = { 160, 45, 400, 270 };
    Rectangle cancelButton = { 343, 276, 96, 30 }; 
    Rectangle saveButton = { 454, 276, 96, 30 };
    bool isUploadVisible = false;
    char* selectedMidiPath = NULL;

    // Bound variables
    char songName[64] = "";
    int bpm = 100;

    while (!WindowShouldClose()) {
        Vector2 mousePosition = GetMousePosition();

        // Check for dropped files
        if (IsFileDropped() && isUploadVisible) {
            FilePathList droppedFiles = LoadDroppedFiles();
            for (int i = 0; i < droppedFiles.count; i++) {
                if (IsFileExtension(droppedFiles.paths[i], ".mid")) {
                    if (selectedMidiPath) free(selectedMidiPath);
                    selectedMidiPath = strdup(droppedFiles.paths[i]);
                    break;
                }
            }
            UnloadDroppedFiles(droppedFiles);
        }

        // Define save conditions
        bool canSave = (pasteAreaInput.textLength > 0 || selectedMidiPath != NULL) && 
                       songNameInput.textLength > 0 && 
                       bpmValueInput.textLength > 0 && atoi(bpmValueInput.text) > 0;

        // Input handling based on z-index
        if (isUploadVisible) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                bool wasEditing = pasteAreaInput.editing || songNameInput.editing || bpmValueInput.editing;
                pasteAreaInput.editing = CheckCollisionPointRec(mousePosition, pasteAreaInput.bounds);
                songNameInput.editing = CheckCollisionPointRec(mousePosition, songNameInput.bounds);
                bpmValueInput.editing = CheckCollisionPointRec(mousePosition, bpmValueInput.bounds);

                if (CheckCollisionPointRec(mousePosition, cancelButton)) {
                    isUploadVisible = false;
                    sceneTextureNeedsUpdate = true;
                }
                if (CheckCollisionPointRec(mousePosition, saveButton) && canSave) {
                    isUploadVisible = false;
                    sceneTextureNeedsUpdate = true;
                }

                if (pasteAreaInput.editing && !wasEditing) pasteAreaInput.cursorPos = pasteAreaInput.textLength;
                if (songNameInput.editing && !wasEditing) songNameInput.cursorPos = songNameInput.textLength;
                if (bpmValueInput.editing && !wasEditing) bpmValueInput.cursorPos = bpmValueInput.textLength;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                pasteAreaInput.editing = false;
                songNameInput.editing = false;
                bpmValueInput.editing = false;
            }

            if (pasteAreaInput.editing) handleTextboxInput(&pasteAreaInput, true);
            if (songNameInput.editing) handleTextboxInput(&songNameInput, false);
            if (bpmValueInput.editing) handleTextboxInput(&bpmValueInput, false);

            if (pasteAreaInput.editing || songNameInput.editing || bpmValueInput.editing) {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
            } else if (CheckCollisionPointRec(mousePosition, pasteAreaInput.bounds) ||
                       CheckCollisionPointRec(mousePosition, songNameInput.bounds) ||
                       CheckCollisionPointRec(mousePosition, bpmValueInput.bounds)) {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
            } else if (CheckCollisionPointRec(mousePosition, saveButton)) {
                if (canSave) {
                    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                } else {
                    SetMouseCursor(MOUSE_CURSOR_NOT_ALLOWED);
                }
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
        } else {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                bool wasEditing = songSearchInput.editing || bpmValueEdit.editing;
                songSearchInput.editing = CheckCollisionPointRec(mousePosition, songSearchInput.bounds);
                bpmValueEdit.editing = CheckCollisionPointRec(mousePosition, bpmValueEdit.bounds);

                if (CheckCollisionPointRec(mousePosition, plusButton)) {
                    isUploadVisible = true;
                    sceneTextureNeedsUpdate = false;
                }

                if (songSearchInput.editing && !wasEditing) songSearchInput.cursorPos = songSearchInput.textLength;
                if (bpmValueEdit.editing && !wasEditing) bpmValueEdit.cursorPos = bpmValueEdit.textLength;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (songSearchInput.editing && songSearchInput.textLength == 0) {
                    strcpy(songSearchInput.text, songSearchInput.placeholder);
                    songSearchInput.textLength = strlen(songSearchInput.placeholder);
                    songSearchInput.cursorPos = songSearchInput.textLength;
                }
                if (bpmValueEdit.editing && bpmValueEdit.textLength == 0) {
                    strcpy(bpmValueEdit.text, bpmValueEdit.placeholder);
                    bpmValueEdit.textLength = strlen(bpmValueEdit.placeholder);
                    bpmValueEdit.cursorPos = bpmValueEdit.textLength;
                }
                songSearchInput.editing = false;
                bpmValueEdit.editing = false;
            }

            if (songSearchInput.editing) handleTextboxInput(&songSearchInput, false);
            if (bpmValueEdit.editing) handleTextboxInput(&bpmValueEdit, false);

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
                drawTextboxText(&songSearchInput, songSearchInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"), false);
                drawTextboxText(&bpmValueEdit, bpmValueEdit.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"), false);
            EndTextureMode();
        }

        BeginDrawing();
            ClearBackground(BLACK);

            if (isUploadVisible) {
                BeginShaderMode(blurShader);
                    DrawTextureRec(sceneTexture.texture, 
                                   (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                                   (Vector2){ 0, 0 }, WHITE);
                EndShaderMode();

                DrawRectangleRec(uploadPanel, toHex("#272930"));         
                DrawRectangleRounded(pasteAreaInput.bounds, 0.1f, 6, 
                    CheckCollisionPointRec(mousePosition, pasteAreaInput.bounds) ? toHex("#33353E") : toHex("#2C2E36"));  
                DrawRectangleRounded(songNameInput.bounds, 0.5f, 6, 
                    CheckCollisionPointRec(mousePosition, songNameInput.bounds) ? toHex("#2A2C33") : toHex("#222329"));  
                DrawRectangleRounded(bpmValueInput.bounds, 0.5f, 6, 
                    CheckCollisionPointRec(mousePosition, bpmValueInput.bounds) ? toHex("#2A2C33") : toHex("#222329")); 
                DrawRectangleRounded(cancelButton, 0.5f, 6, 
                    CheckCollisionPointRec(mousePosition, cancelButton) ? toHex("#2A2C33") : toHex("#222329"));
                DrawRectangleRounded(saveButton, 0.5f, 6, 
                    canSave ? toHex("#393F5F") : 
                    (CheckCollisionPointRec(mousePosition, saveButton) ? toHex("#2A2C33") : toHex("#222329")));

                drawTextboxText(&pasteAreaInput, pasteAreaInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"), true);
                drawTextboxText(&songNameInput, songNameInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"), false);
                drawTextboxText(&bpmValueInput, bpmValueInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"), false);

                DrawRectangle(170, 73, 150, 1, toHex("#494D5A"));
                DrawRectangle(476, 226, 1, 20, toHex("#494D5A"));
                DrawTextEx(boldGFS_h1, "upload song", (Vector2){ 170, 50 }, 20, 1, toHex("#F0F2FE"));
                DrawTextEx(italicGFS, "paste music sheet:", (Vector2){ 170, 74 }, 14, 1, toHex("#979EBB"));
                DrawTextEx(italicGFS, "song name:", (Vector2){ 170, 212 }, 14, 1, toHex("#979EBB"));
                DrawTextEx(italicGFS, "bpm:", (Vector2){ 486, 212 }, 14, 1, toHex("#979EBB"));
                DrawTextEx(boldGFS_h2, "cancel", (Vector2){ 375, 284 }, 14, 1, toHex("#F0F2FE"));
                DrawTextEx(boldGFS_h2, "save", (Vector2){ 491, 284 }, 14, 1, toHex("#F0F2FE"));

                if (selectedMidiPath) {
                    DrawTextEx(italicGFS, selectedMidiPath, (Vector2){ 170, 250 }, 14, 1, toHex("#D0D0D0"));
                    DrawTextEx(italicGFS, "file uploaded", (Vector2){ 480, 74 }, 14, 1, toHex("#D0D0D0"));
                } else {
                    DrawTextEx(italicGFS, "or drag and drop a .midi file", (Vector2){ 394, 74 }, 14, 1, toHex("#D0D0D0"));
                }
            } else {
                DrawTextureRec(sceneTexture.texture, 
                               (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                               (Vector2){ 0, 0 }, WHITE);
            }
        EndDrawing();
    }

    if (selectedMidiPath) free(selectedMidiPath);
    UnloadRenderTexture(backgroundTexture);
    UnloadRenderTexture(sceneTexture);
    UnloadShader(blurShader);
    unloadFonts();
    CloseWindow();
    return 0;
}