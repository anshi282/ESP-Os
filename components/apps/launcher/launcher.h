#pragma once

/**
 * @brief Show the launcher (home) screen.
 *        Displays Music / Video / Settings buttons.
 *        Must be called after display_init().
 */
void launcher_show(void);

/**
 * @brief Return to the launcher from any app.
 */
void launcher_return(void);
