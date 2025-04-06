#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "raylib.h"
#include "resources/GFSNeohellenic_Italic.h"
#include "resources/GFSNeohellenic_Bold.h"
#include "resources/GFSNeohellenic_BoldItalic.h"

// Fixed-size textbox for most inputs
typedef struct {
    Rectangle bounds;           // Position and size
    char text[256];             // Fixed buffer for text
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

// Dynamic textbox for paste area
typedef struct {
    Rectangle bounds;           // Position and size
    char* text;                 // Dynamic buffer for text
    int textLength;             // Current length of text
    int textCapacity;           // Allocated capacity of text buffer
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
} DynamicTextbox;

// Structure for saved songs
typedef struct {
    char* filename;        // Full path to the JSON file
    char* songName;        // Extracted song name for display
} SavedSong;

Color toHex(const char* hex);
void loadFonts(void);
void unloadFonts(void);
void drawTextboxText(Textbox* textbox, Color textColor, bool isPasteArea);
void drawDynamicTextboxText(DynamicTextbox* textbox, Color textColor);
void handleTextboxInput(Textbox* textbox, bool isPasteArea);
void handleDynamicTextboxInput(DynamicTextbox* textbox);
void loadSavedSongs(SavedSong** songs, int* songCount, const char* directory);
void freeSavedSongs(SavedSong* songs, int songCount);
char* getUniqueFilename(const char* baseName, const char* directory);
char* sanitizeFilename(const char* input);

// Global font variables
Font italicGFS;
Font boldGFS_h1;
Font boldGFS_h2;
Font boldItalicGFS;

// Convert hex color (e.g., "#FFFFFF") to raylib Color
Color toHex(const char* hex) {
    if (hex[0] == '#') hex++;
    unsigned int r, g, b;
    sscanf(hex, "%2x%2x%2x", &r, &g, &b);
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
}

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

// Render fixed-size textbox text
void drawTextboxText(Textbox* textbox, Color textColor, bool isPasteArea) {
    const char* displayText = (textbox->textLength == 0 && !textbox->editing) ? textbox->placeholder : textbox->text;
    Vector2 textSize = MeasureTextEx(textbox->font, displayText, textbox->fontSize, 1);
    float maxTextWidth = textbox->bounds.width - 10;
    float maxTextHeight = textbox->bounds.height - 10;

    textbox->horizontalOffset = (textSize.x > maxTextWidth) ? (textSize.x - maxTextWidth) : 0;

    Rectangle scissorRect = { textbox->bounds.x + 5, textbox->bounds.y + 5, maxTextWidth, maxTextHeight };
    BeginScissorMode(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);

    float yPos = textbox->bounds.y + 5;
    DrawTextEx(textbox->font, displayText, 
               (Vector2){ textbox->bounds.x + 5 - textbox->horizontalOffset, yPos }, 
               textbox->fontSize, 1, textColor);

    // Draw selection highlight
    if (textbox->selectionStart != -1 && textbox->selectionEnd != -1 && textbox->selectionStart != textbox->selectionEnd) {
        int start = textbox->selectionStart < textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
        int end = textbox->selectionStart > textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
        Vector2 lineSize = MeasureTextEx(textbox->font, displayText, textbox->fontSize, 1);
        float startX = textbox->bounds.x + 5 + (start * lineSize.x / textbox->textLength) - textbox->horizontalOffset;
        float endX = textbox->bounds.x + 5 + (end * lineSize.x / textbox->textLength) - textbox->horizontalOffset;
        DrawRectangle(startX, yPos, endX - startX, textbox->fontSize, Fade(toHex("#FFFFFF"), 0.3f));
    }

    // Draw cursor
    if (textbox->editing && textbox->cursorBlink < 0.5f) {
        if (textbox->textLength == 0) {
            float cursorX = textbox->bounds.x + 5 - textbox->horizontalOffset;
            DrawRectangle(cursorX, yPos, 2, textbox->fontSize, textColor);
        } else {
            float cursorX = textbox->bounds.x + 5 + MeasureTextEx(textbox->font, displayText, textbox->fontSize, 1).x * textbox->cursorPos / (float)textbox->textLength - textbox->horizontalOffset;
            DrawRectangle(cursorX, yPos, 2, textbox->fontSize, textColor);
        }
    }

    EndScissorMode();
}

// Render dynamic textbox text (paste area)
void drawDynamicTextboxText(DynamicTextbox* textbox, Color textColor) {
    const char* displayText = (textbox->textLength == 0 && !textbox->editing) ? textbox->placeholder : textbox->text;
    float maxTextWidth = textbox->bounds.width - 15;
    float maxTextHeight = textbox->bounds.height - 10;

    // Calculate horizontal offset based on the longest line
    char* tempCopy = strdup(displayText);
    char* tempLine = strtok(tempCopy, "\n");
    float maxLineWidth = 0;
    while (tempLine) {
        Vector2 lineSize = MeasureTextEx(textbox->font, tempLine, textbox->fontSize, 1);
        if (lineSize.x > maxLineWidth) maxLineWidth = lineSize.x;
        tempLine = strtok(NULL, "\n");
    }
    free(tempCopy);
    textbox->horizontalOffset = (maxLineWidth > maxTextWidth) ? (maxLineWidth - maxTextWidth) : 0;

    Rectangle scissorRect = { textbox->bounds.x + 5, textbox->bounds.y + 5, maxTextWidth, maxTextHeight };
    BeginScissorMode(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);

    float yPos = textbox->bounds.y + 5 - textbox->verticalOffset;
    int charIndex = 0;
    float totalHeight = 0;

    // Render each line
    char* textCopy = strdup(displayText);
    char* line = strtok(textCopy, "\n");
    while (line) {
        DrawTextEx(textbox->font, line, 
                   (Vector2){ textbox->bounds.x + 5 - textbox->horizontalOffset, yPos }, 
                   textbox->fontSize, 1, textColor);
        totalHeight += textbox->fontSize + 2;
        yPos += textbox->fontSize + 2;
        charIndex += strlen(line) + 1;
        line = strtok(NULL, "\n");
    }
    free(textCopy);

    // Draw selection highlight
    if (textbox->selectionStart != -1 && textbox->selectionEnd != -1 && textbox->selectionStart != textbox->selectionEnd) {
        int start = textbox->selectionStart < textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
        int end = textbox->selectionStart > textbox->selectionEnd ? textbox->selectionStart : textbox->selectionEnd;
        yPos = textbox->bounds.y + 5 - textbox->verticalOffset;
        charIndex = 0;
        textCopy = strdup(displayText);
        line = strtok(textCopy, "\n");

        while (line != NULL) {
            int lineLen = strlen(line);
            if (charIndex + lineLen >= start && charIndex <= end) {
                Vector2 lineSize = MeasureTextEx(textbox->font, line, textbox->fontSize, 1);
                float startX = textbox->bounds.x + 5 - textbox->horizontalOffset;
                float endX = startX + lineSize.x;
                if (charIndex < start) startX += (start - charIndex) * lineSize.x / lineLen;
                if (charIndex + lineLen > end) endX = startX + (end - charIndex) * lineSize.x / lineLen;
                DrawRectangle(startX, yPos, endX - startX, textbox->fontSize, Fade(toHex("#FFFFFF"), 0.3f));
            }
            charIndex += lineLen + 1;
            yPos += textbox->fontSize + 2;
            line = strtok(NULL, "\n");
        }
        free(textCopy);
    }

    // Draw cursor
    if (textbox->editing && textbox->cursorBlink < 0.5f) {
        yPos = textbox->bounds.y + 5 - textbox->verticalOffset;
        if (textbox->textLength == 0) {
            float cursorX = textbox->bounds.x + 5 - textbox->horizontalOffset;
            DrawRectangle(cursorX, yPos, 2, textbox->fontSize, textColor);
        } else {
            charIndex = 0;
            textCopy = strdup(displayText);
            line = strtok(textCopy, "\n");
            while (line != NULL) {
                int lineLen = strlen(line);
                if (charIndex + lineLen >= textbox->cursorPos) {
                    float cursorX = textbox->bounds.x + 5 + 
                                   MeasureTextEx(textbox->font, line, textbox->fontSize, 1).x * 
                                   (textbox->cursorPos - charIndex) / (float)(lineLen ? lineLen : 1) - 
                                   textbox->horizontalOffset;
                    DrawRectangle(cursorX, yPos, 2, textbox->fontSize, textColor);
                    break;
                }
                charIndex += lineLen + 1;
                yPos += textbox->fontSize + 2;
                line = strtok(NULL, "\n");
            }
            free(textCopy);
        }
    }

    // Scrolling logic
    Vector2 mousePos = GetMousePosition();
    if (CheckCollisionPointRec(mousePos, textbox->bounds)) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            if (totalHeight > maxTextHeight) {
                textbox->verticalOffset -= wheel * 20.0f;
                if (textbox->verticalOffset < 0) textbox->verticalOffset = 0;
                if (textbox->verticalOffset > totalHeight - maxTextHeight) textbox->verticalOffset = totalHeight - maxTextHeight;
            } else {
                textbox->verticalOffset = 0;
            }
        }
    }

    EndScissorMode();

    // Scrollbar
    if (totalHeight > maxTextHeight) {
        float scrollBarHeight = maxTextHeight * maxTextHeight / totalHeight;
        float scrollBarY = textbox->bounds.y + 5 + (textbox->verticalOffset * (maxTextHeight - scrollBarHeight) / (totalHeight - maxTextHeight));
        DrawRectangle(textbox->bounds.x + textbox->bounds.width - 10, scrollBarY, 5, scrollBarHeight, toHex("#494D5A"));
    }
}

// Input handling for fixed-size textbox
void handleTextboxInput(Textbox* textbox, bool isPasteArea) {
    Vector2 mousePos = GetMousePosition();
    bool mouseOver = CheckCollisionPointRec(mousePos, textbox->bounds);

    if (mouseOver && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        textbox->selectionStart = -1;
        textbox->selectionEnd = -1;
        float xOffset = mousePos.x - (textbox->bounds.x + 5) + textbox->horizontalOffset;
        Vector2 lineSize = MeasureTextEx(textbox->font, textbox->text, textbox->fontSize, 1);
        int pos = (int)((xOffset / lineSize.x) * textbox->textLength);
        textbox->cursorPos = (pos < textbox->textLength) ? pos : textbox->textLength;
    }

    if (mouseOver && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float xOffset = mousePos.x - (textbox->bounds.x + 5) + textbox->horizontalOffset;
        Vector2 lineSize = MeasureTextEx(textbox->font, textbox->text, textbox->fontSize, 1);
        int pos = (int)((xOffset / lineSize.x) * textbox->textLength);
        textbox->selectionEnd = (pos < textbox->textLength) ? pos : textbox->textLength;
        if (textbox->selectionStart == -1) textbox->selectionStart = textbox->cursorPos;
    }

    int key = GetCharPressed();
    while (key > 0 && textbox->textLength < 255) {
        if (textbox->numericOnly) {
            if (key >= '0' && key <= '9') { 
                textbox->text[textbox->textLength++] = (char)key;
                textbox->text[textbox->textLength] = '\0';
                textbox->cursorPos++;
            }
        } else {
            if (key >= 32 && key <= 126) {
                textbox->text[textbox->textLength++] = (char)key;
                textbox->text[textbox->textLength] = '\0';
                textbox->cursorPos++;
            }
        }
        key = GetCharPressed();
    }

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

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_A)) {
        textbox->selectionStart = 0;
        textbox->selectionEnd = textbox->textLength;
        textbox->cursorPos = textbox->textLength;
    }

    if (IsKeyPressed(KEY_LEFT) && textbox->cursorPos > 0) {
        textbox->cursorPos--;
        textbox->selectionStart = textbox->selectionEnd = -1;
    }
    if (IsKeyPressed(KEY_RIGHT) && textbox->cursorPos < textbox->textLength) {
        textbox->cursorPos++;
        textbox->selectionStart = textbox->selectionEnd = -1;
    }

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
        textbox->backspaceTimer = 0.3f;
    }

    if (IsKeyDown(KEY_BACKSPACE) && textbox->textLength > 0 && textbox->cursorPos > 0) {
        textbox->backspaceTimer -= GetFrameTime();
        if (textbox->backspaceTimer <= 0) {
            memmove(textbox->text + textbox->cursorPos - 1, textbox->text + textbox->cursorPos, textbox->textLength - textbox->cursorPos + 1);
            textbox->textLength--;
            textbox->cursorPos--;
            textbox->backspaceTimer = 0.05f;
        }
    }

    if (IsKeyReleased(KEY_BACKSPACE)) {
        textbox->backspaceTimer = 0;
    }

    textbox->cursorBlink += GetFrameTime();
    if (textbox->cursorBlink > 1.0f) textbox->cursorBlink = 0.0f;
}

// Input handling for dynamic textbox (paste area)
void handleDynamicTextboxInput(DynamicTextbox* textbox) {
    Vector2 mousePos = GetMousePosition();
    bool mouseOver = CheckCollisionPointRec(mousePos, textbox->bounds);

    if (mouseOver && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        textbox->selectionStart = -1;
        textbox->selectionEnd = -1;
        float xOffset = mousePos.x - (textbox->bounds.x + 5) + textbox->horizontalOffset;
        float yOffset = mousePos.y - (textbox->bounds.y + 5) + textbox->verticalOffset;
        int charIndex = 0;
        char* line = strtok(strdup(textbox->text), "\n");
        int lineNum = (int)(yOffset / (textbox->fontSize + 2));

        for (int i = 0; i < lineNum && line; i++) {
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

    if (mouseOver && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float xOffset = mousePos.x - (textbox->bounds.x + 5) + textbox->horizontalOffset;
        float yOffset = mousePos.y - (textbox->bounds.y + 5) + textbox->verticalOffset;
        int lineNum = (int)(yOffset / (textbox->fontSize + 2));
        int charIndex = 0;
        char* line = strtok(strdup(textbox->text), "\n");

        for (int i = 0; i < lineNum && line; i++) {
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
        if (textbox->textLength + 1 >= textbox->textCapacity) {
            textbox->textCapacity *= 2;
            textbox->text = realloc(textbox->text, textbox->textCapacity);
        }
        if (textbox->numericOnly) {
            if (key >= '0' && key <= '9') { 
                textbox->text[textbox->textLength++] = (char)key;
                textbox->text[textbox->textLength] = '\0';
                textbox->cursorPos++;
            }
        } else {
            if ((key >= 32 && key <= 126) || key == '\n') {
                textbox->text[textbox->textLength++] = (char)key;
                textbox->text[textbox->textLength] = '\0';
                textbox->cursorPos++;
            }
        }
        key = GetCharPressed();
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
        const char* clipboard = GetClipboardText();
        if (clipboard) {
            int len = strlen(clipboard);
            int newLength = textbox->textLength + len;
            if (newLength + 1 > textbox->textCapacity) {
                textbox->textCapacity = newLength + 256;
                textbox->text = realloc(textbox->text, textbox->textCapacity);
            }
            char* cleanClipboard = malloc(len + 1);
            int cleanLen = 0;
            for (int i = 0; i < len; i++) {
                if (clipboard[i] == '\r' && i + 1 < len && clipboard[i + 1] == '\n') {
                    cleanClipboard[cleanLen++] = '\n';
                    i++;
                } else if (clipboard[i] != '\r') {
                    cleanClipboard[cleanLen++] = clipboard[i];
                }
            }
            cleanClipboard[cleanLen] = '\0';

            if (textbox->numericOnly) {
                for (int i = 0; i < cleanLen; i++) {
                    if (cleanClipboard[i] >= '0' && cleanClipboard[i] <= '9') {
                        textbox->text[textbox->textLength++] = cleanClipboard[i];
                        textbox->cursorPos++;
                    }
                }
            } else {
                memcpy(textbox->text + textbox->textLength, cleanClipboard, cleanLen);
                textbox->textLength += cleanLen;
                textbox->cursorPos += cleanLen;
            }
            textbox->text[textbox->textLength] = '\0';
            free(cleanClipboard);
        }
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_A)) {
        textbox->selectionStart = 0;
        textbox->selectionEnd = textbox->textLength;
        textbox->cursorPos = textbox->textLength;
    }

    if (IsKeyPressed(KEY_LEFT) && textbox->cursorPos > 0) {
        textbox->cursorPos--;
        textbox->selectionStart = textbox->selectionEnd = -1;
    }
    if (IsKeyPressed(KEY_RIGHT) && textbox->cursorPos < textbox->textLength) {
        textbox->cursorPos++;
        textbox->selectionStart = textbox->selectionEnd = -1;
    }

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
        textbox->backspaceTimer = 0.3f;
    }

    if (IsKeyDown(KEY_BACKSPACE) && textbox->textLength > 0 && textbox->cursorPos > 0) {
        textbox->backspaceTimer -= GetFrameTime();
        if (textbox->backspaceTimer <= 0) {
            memmove(textbox->text + textbox->cursorPos - 1, textbox->text + textbox->cursorPos, textbox->textLength - textbox->cursorPos + 1);
            textbox->textLength--;
            textbox->cursorPos--;
            textbox->backspaceTimer = 0.05f;
        }
    }

    if (IsKeyReleased(KEY_BACKSPACE)) {
        textbox->backspaceTimer = 0;
    }

    textbox->cursorBlink += GetFrameTime();
    if (textbox->cursorBlink > 1.0f) textbox->cursorBlink = 0.0f;
}

// Sanitize filename by replacing invalid characters
char* sanitizeFilename(const char* input) {
    char* output = strdup(input);
    for (int i = 0; output[i]; i++) {
        if (strchr("\\/:*?\"<>|", output[i])) {
            output[i] = '_';
        }
    }
    return output;
}

// Load saved songs from noctivoxFiles directory
void loadSavedSongs(SavedSong** songs, int* songCount, const char* directory) {
    FilePathList files = LoadDirectoryFiles(directory);
    *songCount = 0;
    *songs = NULL;

    for (int i = 0; i < files.count; i++) {
        if (IsFileExtension(files.paths[i], ".json")) {
            char* content = LoadFileText(files.paths[i]);
            if (content) {
                char* songNameStart = strstr(content, "\"songName\": \"");
                if (songNameStart) {
                    songNameStart += 12; // Skip past '"songName": "'
                    char* songNameEnd = strchr(songNameStart, '"');
                    if (songNameEnd) {
                        int songNameLen = songNameEnd - songNameStart;
                        *songs = realloc(*songs, (*songCount + 1) * sizeof(SavedSong));
                        (*songs)[*songCount].filename = strdup(files.paths[i]);
                        (*songs)[*songCount].songName = malloc(songNameLen + 1);
                        strncpy((*songs)[*songCount].songName, songNameStart, songNameLen);
                        (*songs)[*songCount].songName[songNameLen] = '\0';
                        (*songCount)++;
                    }
                }
                UnloadFileText(content);
            }
        }
    }
    UnloadDirectoryFiles(files);
    TraceLog(LOG_INFO, "Loaded %d songs from %s", *songCount, directory);
}

// Free memory allocated for saved songs
void freeSavedSongs(SavedSong* songs, int songCount) {
    for (int i = 0; i < songCount; i++) {
        free(songs[i].filename);
        free(songs[i].songName);
    }
    free(songs);
}

// Generate a unique filename by appending (1), (2), etc.
char* getUniqueFilename(const char* baseName, const char* directory) {
    char* sanitizedBase = sanitizeFilename(baseName);
    char* filename = malloc(512);
#ifdef _WIN32
    snprintf(filename, 512, "%s\\%s.json", directory, sanitizedBase);
#else
    snprintf(filename, 512, "%s/%s.json", directory, sanitizedBase);
#endif
    if (!FileExists(filename)) {
        free(sanitizedBase);
        return filename;
    }

    int counter = 1;
    do {
        free(filename);
        filename = malloc(512);
#ifdef _WIN32
        snprintf(filename, 512, "%s\\%s(%d).json", directory, sanitizedBase, counter);
#else
        snprintf(filename, 512, "%s/%s(%d).json", directory, sanitizedBase, counter);
#endif
        counter++;
    } while (FileExists(filename));

    free(sanitizedBase);
    return filename;
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
    "}";

int main(void) {
    const int screenWidth = 720;
    const int screenHeight = 360;
    InitWindow(screenWidth, screenHeight, "noctivox | a virtual piano player");
    SetTargetFPS(60);
    SetTraceLogLevel(LOG_ALL); // Enable all logging for debugging

    loadFonts();

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
        DrawRectangle(14, 90, 180, 270, toHex("#272930")); // Container for saved songs
    EndTextureMode();

    RenderTexture2D sceneTexture = LoadRenderTexture(screenWidth, screenHeight);
    bool sceneTextureNeedsUpdate = true;

    Shader blurShader = LoadShaderFromMemory(0, blurShaderCode);
    int resolutionLoc = GetShaderLocation(blurShader, "resolution");
    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(blurShader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    // Fixed-size textboxes (base layer)
    Textbox songSearchInput = { 
        { 14, 55, 150, 24 }, "", 0, false, 0.0f, 
        false, italicGFS, 14, 0, 0, 0, "search for songs", 0, -1, -1 
    };
    Textbox bpmValueEdit = { 
        { 274, 319, 60, 30 }, "", 0, false, 0.0f, 
        true, italicGFS, 14, 0, 0, 0, "100", 0, -1, -1 
    };

    // Dynamic textbox for paste area
    DynamicTextbox pasteAreaInput = { 
        { 170, 90, 380, 120 }, malloc(256), 0, 256, false, 0.0f, 
        false, italicGFS, 14, 0, 0, 0, "", 0, -1, -1 
    };
    pasteAreaInput.text[0] = '\0';

    // Fixed-size textboxes for upload panel
    Textbox songNameInput = { 
        { 170, 226, 297, 24 }, "", 0, false, 0.0f, 
        false, italicGFS, 14, 0, 0, 0, "", 0, -1, -1 
    };
    Textbox bpmValueInput = { 
        { 486, 226, 64, 24 }, "", 0, false, 0.0f, 
        true, italicGFS, 14, 0, 0, 0, "", 0, -1, -1 
    };

    Rectangle plusButton = { 170, 55, 24, 24 };
    Rectangle uploadPanel = { 160, 45, 400, 270 };
    Rectangle cancelButton = { 343, 276, 96, 30 }; 
    Rectangle saveButton = { 454, 276, 96, 30 };
    bool isUploadVisible = false;
    char* selectedMidiPath = NULL;

    char songName[64] = "";
    int bpm = 100;

    // Saved songs list
    SavedSong* savedSongs = NULL;
    int songCount = 0;
    float songListOffset = 0.0f; // Vertical scroll offset for song list
    Rectangle songListBounds = { 14, 90, 180, 270 }; // Scrolling frame area

    // Setup noctivoxFiles directory
    char noctivoxDir[512];
    const char* homeDir = getenv("USERPROFILE"); // Windows
    if (!homeDir) homeDir = getenv("HOME");      // Unix-like
    if (!homeDir) {
        homeDir = GetWorkingDirectory();
        TraceLog(LOG_WARNING, "No HOME or USERPROFILE found, using working directory: %s", homeDir);
    }

#ifdef _WIN32
    snprintf(noctivoxDir, sizeof(noctivoxDir), "%s\\Downloads\\noctivoxFiles", homeDir);
#else
    snprintf(noctivoxDir, sizeof(noctivoxDir), "%s/Downloads/noctivoxFiles", homeDir);
#endif

    if (!DirectoryExists(noctivoxDir)) {
        int result = MakeDirectory(noctivoxDir);
        if (result == 0) {
            TraceLog(LOG_INFO, "Created directory: %s", noctivoxDir);
        } else {
            TraceLog(LOG_ERROR, "Failed to create %s (error %d), falling back to working directory", noctivoxDir, result);
            snprintf(noctivoxDir, sizeof(noctivoxDir), "%s/noctivoxFiles", GetWorkingDirectory());
            if (!DirectoryExists(noctivoxDir)) {
                result = MakeDirectory(noctivoxDir);
                if (result == 0) {
                    TraceLog(LOG_INFO, "Created fallback directory: %s", noctivoxDir);
                } else {
                    TraceLog(LOG_ERROR, "Failed to create fallback directory: %s (error %d)", noctivoxDir, result);
                }
            }
        }
    } else {
        TraceLog(LOG_INFO, "Directory already exists: %s", noctivoxDir);
    }
    loadSavedSongs(&savedSongs, &songCount, noctivoxDir);

    while (!WindowShouldClose()) {
        Vector2 mousePosition = GetMousePosition();

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

        bool canSave = (pasteAreaInput.textLength > 0 || selectedMidiPath != NULL) && 
                       songNameInput.textLength > 0 && 
                       bpmValueInput.textLength > 0 && atoi(bpmValueInput.text) > 0;

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
                    char baseName[256];
                    snprintf(baseName, sizeof(baseName), "%s_%s", songNameInput.text, bpmValueInput.text);
                    char* filename = getUniqueFilename(baseName, noctivoxDir);
                    FILE* file = fopen(filename, "w");
                    if (file) {
                        fprintf(file, "{\n");
                        fprintf(file, "  \"songName\": \"%s\",\n", songNameInput.text);
                        fprintf(file, "  \"BPM\": \"%s\",\n", bpmValueInput.text);
                        fprintf(file, "  \"songInfo\": \"");
                        for (int i = 0; i < pasteAreaInput.textLength; i++) {
                            if (pasteAreaInput.text[i] == '\n') fputs("\\n", file);
                            else if (pasteAreaInput.text[i] == '"') fputs("\\\"", file);
                            else fputc(pasteAreaInput.text[i], file);
                        }
                        fprintf(file, "\"\n}\n");
                        fclose(file);
                        if (FileExists(filename)) {
                            TraceLog(LOG_INFO, "Successfully saved song to: %s", filename);
                        } else {
                            TraceLog(LOG_ERROR, "File not found after save: %s", filename);
                        }
                    } else {
                        TraceLog(LOG_ERROR, "Failed to open file for writing: %s", filename);
                    }
                    free(filename);

                    // Reload saved songs
                    freeSavedSongs(savedSongs, songCount);
                    loadSavedSongs(&savedSongs, &songCount, noctivoxDir);

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

            if (pasteAreaInput.editing) handleDynamicTextboxInput(&pasteAreaInput);
            if (songNameInput.editing) handleTextboxInput(&songNameInput, false);
            if (bpmValueInput.editing) handleTextboxInput(&bpmValueInput, false);

            if (pasteAreaInput.editing || songNameInput.editing || bpmValueInput.editing) {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
            } else if (CheckCollisionPointRec(mousePosition, pasteAreaInput.bounds) ||
                       CheckCollisionPointRec(mousePosition, songNameInput.bounds) ||
                       CheckCollisionPointRec(mousePosition, bpmValueInput.bounds)) {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
            } else if (CheckCollisionPointRec(mousePosition, saveButton) && canSave) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else if (CheckCollisionPointRec(mousePosition, cancelButton)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else if (CheckCollisionPointRec(mousePosition, saveButton) && !canSave) {
                SetMouseCursor(MOUSE_CURSOR_NOT_ALLOWED);
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

                // Check for song selection in the scrolling list
                if (CheckCollisionPointRec(mousePosition, songListBounds)) {
                    float yOffset = mousePosition.y - songListBounds.y + songListOffset;
                    int songHeight = 30;
                    int selectedIndex = (int)(yOffset / songHeight);
                    if (selectedIndex >= 0 && selectedIndex < songCount) {
                        char* content = LoadFileText(savedSongs[selectedIndex].filename);
                        if (content) {
                            char* songNameStart = strstr(content, "\"songName\": \"");
                            char* bpmStart = strstr(content, "\"BPM\": \"");
                            char* songInfoStart = strstr(content, "\"songInfo\": \"");
                            if (songNameStart && bpmStart && songInfoStart) {
                                songNameStart += 12;
                                bpmStart += 7;
                                songInfoStart += 12;
                                char* songNameEnd = strchr(songNameStart, '"');
                                char* bpmEnd = strchr(bpmStart, '"');
                                char* songInfoEnd = strchr(songInfoStart, '"');
                                if (songNameEnd && bpmEnd && songInfoEnd) {
                                    // Load song name
                                    int songNameLen = songNameEnd - songNameStart;
                                    strncpy(songSearchInput.text, songNameStart, songNameLen);
                                    songSearchInput.text[songNameLen] = '\0';
                                    songSearchInput.textLength = songNameLen;

                                    // Load BPM
                                    int bpmLen = bpmEnd - bpmStart;
                                    strncpy(bpmValueEdit.text, bpmStart, bpmLen);
                                    bpmValueEdit.text[bpmLen] = '\0';
                                    bpmValueEdit.textLength = bpmLen;

                                    // Load song info into paste area
                                    int songInfoLen = songInfoEnd - songInfoStart;
                                    char* tempSongInfo = malloc(songInfoLen + 1);
                                    strncpy(tempSongInfo, songInfoStart, songInfoLen);
                                    tempSongInfo[songInfoLen] = '\0';

                                    // Unescape songInfo
                                    char* unescaped = malloc(songInfoLen + 1);
                                    int unescapedLen = 0;
                                    for (int i = 0; i < songInfoLen; i++) {
                                        if (tempSongInfo[i] == '\\' && i + 1 < songInfoLen) {
                                            if (tempSongInfo[i + 1] == 'n') unescaped[unescapedLen++] = '\n';
                                            else if (tempSongInfo[i + 1] == '"') unescaped[unescapedLen++] = '"';
                                            i++;
                                        } else {
                                            unescaped[unescapedLen++] = tempSongInfo[i];
                                        }
                                    }
                                    unescaped[unescapedLen] = '\0';

                                    if (unescapedLen + 1 > pasteAreaInput.textCapacity) {
                                        pasteAreaInput.textCapacity = unescapedLen + 256;
                                        pasteAreaInput.text = realloc(pasteAreaInput.text, pasteAreaInput.textCapacity);
                                    }
                                    strcpy(pasteAreaInput.text, unescaped);
                                    pasteAreaInput.textLength = unescapedLen;

                                    free(tempSongInfo);
                                    free(unescaped);
                                    sceneTextureNeedsUpdate = true;
                                    TraceLog(LOG_INFO, "Loaded song: %s", savedSongs[selectedIndex].songName);
                                }
                            }
                            UnloadFileText(content);
                        }
                    }
                }

                if (songSearchInput.editing && !wasEditing) songSearchInput.cursorPos = songSearchInput.textLength;
                if (bpmValueEdit.editing && !wasEditing) bpmValueEdit.cursorPos = bpmValueEdit.textLength;
            }

            // Handle scrolling in the song list
            if (CheckCollisionPointRec(mousePosition, songListBounds)) {
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    float totalHeight = songCount * 30; // Each song is 30px tall
                    float maxHeight = songListBounds.height;
                    songListOffset -= wheel * 20.0f;
                    if (songListOffset < 0) songListOffset = 0;
                    if (totalHeight > maxHeight && songListOffset > totalHeight - maxHeight) {
                        songListOffset = totalHeight - maxHeight;
                    }
                }
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
                       CheckCollisionPointRec(mousePosition, plusButton) ||
                       CheckCollisionPointRec(mousePosition, songListBounds)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
        }

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

        if (sceneTextureNeedsUpdate && !isUploadVisible) {
            BeginTextureMode(sceneTexture);
                DrawTextureRec(backgroundTexture.texture, 
                               (Rectangle){ 0, 0, screenWidth, -screenHeight }, 
                               (Vector2){ 0, 0 }, WHITE);
                drawTextboxText(&songSearchInput, songSearchInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"), false);
                drawTextboxText(&bpmValueEdit, bpmValueEdit.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"), false);
                drawDynamicTextboxText(&pasteAreaInput, pasteAreaInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"));

                // Draw scrolling song list
                BeginScissorMode(songListBounds.x, songListBounds.y, songListBounds.width, songListBounds.height);
                float yPos = songListBounds.y - songListOffset;
                for (int i = 0; i < songCount; i++) {
                    Rectangle songRect = { songListBounds.x + 5, yPos, songListBounds.width - 10, 24 };
                    bool hovered = CheckCollisionPointRec(mousePosition, songRect);
                    DrawRectangleRounded(songRect, 0.5f, 6, hovered ? toHex("#393F5F") : toHex("#222329"));
                    DrawTextEx(italicGFS, savedSongs[i].songName, 
                               (Vector2){ songRect.x + 5, songRect.y + 5 }, 14, 1, toHex("#D0D0D0"));
                    yPos += 30;
                }
                EndScissorMode();

                // Scrollbar for song list
                float totalHeight = songCount * 30;
                if (totalHeight > songListBounds.height) {
                    float scrollBarHeight = songListBounds.height * songListBounds.height / totalHeight;
                    float scrollBarY = songListBounds.y + (songListOffset * (songListBounds.height - scrollBarHeight) / (totalHeight - songListBounds.height));
                    DrawRectangle(songListBounds.x + songListBounds.width - 5, scrollBarY, 3, scrollBarHeight, toHex("#494D5A"));
                }
            EndTextureMode();
            sceneTextureNeedsUpdate = false;
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
                    canSave ? (CheckCollisionPointRec(mousePosition, saveButton) ? toHex("#494D5A") : toHex("#393F5F")) : 
                    (CheckCollisionPointRec(mousePosition, saveButton) ? toHex("#2A2C33") : toHex("#222329")));
                drawDynamicTextboxText(&pasteAreaInput, pasteAreaInput.editing ? toHex("#FFFFFF") : toHex("#D0D0D0"));
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
    free(pasteAreaInput.text);
    freeSavedSongs(savedSongs, songCount);
    UnloadRenderTexture(backgroundTexture);
    UnloadRenderTexture(sceneTexture);
    UnloadShader(blurShader);
    unloadFonts();
    CloseWindow();
    return 0;
}