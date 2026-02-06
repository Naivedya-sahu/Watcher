/**
 * HABIT TRACKER PLUGIN - Local (EEPROM Persistence)
 *
 * Track daily habits with visual progress
 * Data persists across reboots using Preferences
 *
 * Inspired by TRMNL's tracking plugins
 */

#include <Arduino.h>
#include <Preferences.h>
#include "EPD.h"
#include "GUI_Paint.h"

#define MAX_HABITS 5
#define DAYS_TO_TRACK 30

class HabitTrackerPlugin {
private:
    Preferences prefs;

    struct Habit {
        char name[20];
        bool completed[DAYS_TO_TRACK];  // Last 30 days
        int currentDay;
        int streak;
    };

    Habit habits[MAX_HABITS];
    int habitCount = 0;

public:
    void init() {
        prefs.begin("habits", false);
        loadHabits();

        // Default habits if empty
        if (habitCount == 0) {
            addHabit("Exercise");
            addHabit("Meditate");
            addHabit("Read");
        }
    }

    void addHabit(const char* name) {
        if (habitCount >= MAX_HABITS) return;

        strncpy(habits[habitCount].name, name, 19);
        habits[habitCount].name[19] = '\0';
        habits[habitCount].currentDay = 0;
        habits[habitCount].streak = 0;

        for (int i = 0; i < DAYS_TO_TRACK; i++) {
            habits[habitCount].completed[i] = false;
        }

        habitCount++;
        saveHabits();
    }

    void markComplete(int habitIndex) {
        if (habitIndex >= habitCount) return;

        Habit& h = habits[habitIndex];
        h.completed[h.currentDay] = true;

        // Update streak
        h.streak = 0;
        for (int i = h.currentDay; i >= 0 && h.completed[i]; i--) {
            h.streak++;
        }

        saveHabits();
    }

    void nextDay() {
        // Move to next day for all habits
        for (int i = 0; i < habitCount; i++) {
            habits[i].currentDay = (habits[i].currentDay + 1) % DAYS_TO_TRACK;
            habits[i].completed[habits[i].currentDay] = false;
        }
        saveHabits();
    }

    void render(UBYTE* buffer, int width, int height) {
        Paint_SelectImage(buffer);
        Paint_Clear(WHITE);

        // Title
        Paint_DrawString_EN(10, 10, "Daily Habits", &Font20, WHITE, BLACK);

        int yOffset = 50;
        int rowHeight = 50;
        int boxSize = 8;

        for (int i = 0; i < habitCount; i++) {
            Habit& h = habits[i];

            // Habit name
            Paint_DrawString_EN(10, yOffset, h.name, &Font16, WHITE, BLACK);

            // Streak counter
            char streakStr[20];
            sprintf(streakStr, "%d day", h.streak);
            if (h.streak != 1) strcat(streakStr, "s");
            Paint_DrawString_EN(150, yOffset, streakStr, &Font12, WHITE, BLACK);

            // Last 7 days visual (boxes)
            int boxX = 250;
            for (int day = 0; day < 7; day++) {
                int dayIndex = (h.currentDay - day + DAYS_TO_TRACK) % DAYS_TO_TRACK;

                if (h.completed[dayIndex]) {
                    // Filled box for completed day
                    Paint_DrawRectangle(boxX, yOffset, boxX + boxSize,
                                      yOffset + boxSize, BLACK,
                                      DOT_PIXEL_1X1, DRAW_FILL_FULL);
                } else {
                    // Empty box for incomplete day
                    Paint_DrawRectangle(boxX, yOffset, boxX + boxSize,
                                      yOffset + boxSize, BLACK,
                                      DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
                }

                boxX += boxSize + 3;
            }

            yOffset += rowHeight;
        }

        // Instructions
        Paint_DrawString_EN(10, height - 30,
                          "Press buttons to mark complete",
                          &Font12, WHITE, BLACK);
    }

private:
    void saveHabits() {
        prefs.putInt("count", habitCount);

        for (int i = 0; i < habitCount; i++) {
            char key[20];
            sprintf(key, "habit%d", i);
            prefs.putBytes(key, &habits[i], sizeof(Habit));
        }
    }

    void loadHabits() {
        habitCount = prefs.getInt("count", 0);

        for (int i = 0; i < habitCount; i++) {
            char key[20];
            sprintf(key, "habit%d", i);
            prefs.getBytes(key, &habits[i], sizeof(Habit));
        }
    }

public:
    const char* getName() { return "Habit Tracker"; }
};

// Example usage with buttons
/*
HabitTrackerPlugin tracker;
UBYTE* buffer = (UBYTE*)malloc(15000);
int selectedHabit = 0;

void setup() {
    DEV_Module_Init();
    EPD_4IN2_V2_Init();
    tracker.init();

    pinMode(35, INPUT_PULLUP);  // Left: Previous habit
    pinMode(36, INPUT_PULLUP);  // Middle: Mark complete
    pinMode(37, INPUT_PULLUP);  // Right: Next habit
}

void loop() {
    if (digitalRead(35) == LOW) {
        selectedHabit = (selectedHabit - 1 + MAX_HABITS) % MAX_HABITS;
        delay(200);
    }

    if (digitalRead(36) == LOW) {
        tracker.markComplete(selectedHabit);
        delay(200);
    }

    if (digitalRead(37) == LOW) {
        selectedHabit = (selectedHabit + 1) % MAX_HABITS;
        delay(200);
    }

    tracker.render(buffer, 400, 300);
    EPD_4IN2_V2_Display(buffer);
    delay(100);
}
*/
