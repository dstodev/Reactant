#include "reactant_ui.h"

menu_item_t * create_button(char * label, int (* callback)(WINDOW *)) {
    if (label != NULL) {
        menu_item_t * button = malloc(sizeof(struct _menu_item_t));

        button->label = malloc(strlen(label) + 1);
        strcpy(button->label, label);

        button->callback = callback;
        button->selected = 0;

        return button;
    }
    return NULL;
}

void free_button(menu_item_t * button) {
    if (button != NULL) {
        free(button->label);
        free(button);
    }
}

panel_t * create_panel(char * label, int y, int x) {
    if (label != NULL) {
        // TODO: Don't allow panels to be created inside of eachother

        panel_t * panel = malloc(sizeof(struct _panel_t));

        panel->label = malloc(strlen(label) + 1);
        strcpy(panel->label, label);

        panel->items = NULL;
        panel->num_items = 0;
        panel->selected = 0;
        panel->coord.y = y;
        panel->coord.x = x;

        panel->window = newwin(1, (int) strlen(label), y, x);
        wattron(panel->window, COLOR_PAIR(1));
        wattron(panel->window, A_DIM);

        return panel;
    }
    return NULL;
}

void free_panel(panel_t * panel) {
    if (panel != NULL) {
        wclear(panel->window);
        wrefresh(panel->window);
        delwin(panel->window);

        free(panel->label);

        for (int i = 0; i < panel->num_items; ++i) {
            free_button(panel->items[i]);
        }
        free(panel->items);
        free(panel);
    }
}

void add_panel_button(panel_t * panel, menu_item_t * button)
{
    int y, x;

    if (panel != NULL) {
        panel->items = realloc(panel->items, sizeof(menu_item_t *) * ((size_t) panel->num_items + 1));
        panel->items[panel->num_items] = button;

        ++panel->num_items;

        // Resize panel window if button label length is too great
        getmaxyx(panel->window, y, x);

        if (button && button->label && (int) strlen(button->label) > x - 2) {
            x = (int) strlen(button->label) + 2;
        }
        y += 1;

        wresize(panel->window, y, x);
        wrefresh(panel->window);
    }
}

void draw_panel(panel_t * panel) {
    if (panel) {
        // int max_y = getmaxy(panel->window);

        // TODO: DEBUG
        // wbkgd(panel->window, COLOR_PAIR(3));

        if (panel->selected == 1) {
            wattroff(panel->window, A_DIM);
            wattron(panel->window, A_BOLD);
        }

        // Draw label
        mvwprintw(panel->window, 0, 0, panel->label);

        // Draw buttons inside panel
        for (int i = 0; i < panel->num_items; ++i) {
            mvwaddch(panel->window, 1 + i, 0, ACS_BULLET);

            if (panel->items[i]->selected == 1) {
                wattroff(panel->window, A_BOLD);
                wattron(panel->window, COLOR_PAIR(2));
            }

            mvwprintw(panel->window, 1 + i, 2, panel->items[i]->label);

            if (panel->items[i]->selected == 1) {
                wattron(panel->window, A_BOLD);
                wattroff(panel->window, COLOR_PAIR(2));
            }
        }

        if (panel->selected == 1) {
            wattron(panel->window, A_DIM);
            wattroff(panel->window, A_BOLD);
        }
        wrefresh(panel->window);
    }
}

menu_t * create_menu(char * label) {
    if (label != NULL) {
        menu_t * menu = malloc(sizeof(struct _menu_t));

        menu->label = malloc(strlen(label) + 1);
        strcpy(menu->label, label);

        menu->panels = NULL;
        menu->num_panels = 0;

        menu->window = newwin(0, 0, 0, 0);
        wclear(menu->window);

        wattron(menu->window, COLOR_PAIR(1));
        wattron(menu->window, A_BOLD);

        keypad(menu->window, TRUE);

        // Draw border
        wborder(menu->window
                , ACS_VLINE, ACS_VLINE // Side lines
                , ACS_HLINE, ACS_HLINE // Top & bottom lines
                , ACS_ULCORNER, ACS_URCORNER // Top corners
                , ACS_LLCORNER, ACS_LRCORNER // Bottom corners
        );

        // Draw title
        mvwprintw(menu->window, 0, 5, " %s ", menu->label);
        wrefresh(menu->window);

        return menu;
    }
    return NULL;
}

void free_menu(menu_t * menu) {
    if (menu != NULL) {
        wclear(menu->window);
        wrefresh(menu->window);
        delwin(menu->window);

        free(menu->label);
        for (int i = 0; i < menu->num_panels; ++i) {
            free_panel(menu->panels[i]);
        }
        free(menu->panels);
        free(menu);
    }
}

void add_menu_panel(menu_t * menu, panel_t * panel) {
    if (menu != NULL && panel != NULL) {
        menu->panels = realloc(menu->panels, sizeof(struct _panel_t *) * ((size_t) menu->num_panels + 1));
        menu->panels[menu->num_panels] = panel;
        ++menu->num_panels;
    }
}

int operate_menu(menu_t * menu) {
    // Return value of this function
    int quit = 0;

    if (menu) {
        // Menu
        int sel_p = 0; // Selected panel index
        int sel_b = 0; // Selected button index

        // Navigational
        int panel_next = 0;
        int panel_offset_y = 0;
        int panel_offset_x = 0;
        // Up & down
        int panel_width = 0;
        int panel_width_next = 0;
        int offset_y = 0;
        // Left & right
        int sel_x = 0;
        int offset_x = 0;

        // Key that started iteration
        int key = 0;

        // Return value of callback
        int rval = 0;

        // Dimensions of the terminal
        int max_y = 0;
        int max_x = 0;
        getmaxyx(menu->window, max_y, max_x);

        if (menu->num_panels > 0 && menu->panels[0]->num_items > 0) {
            menu->panels[0]->selected = 1;
            menu->panels[0]->items[0]->selected = 1;
        }

        do {
            // Clear screen
            wclear(menu->window);

            // Process the pressed key
            switch (key) {
                case KEY_RESIZE: // Window resize
                    getmaxyx(menu->window, max_y, max_x);

                    // TODO: Panel crashes if resized too small?

                    if (max_y == 0 || max_x == 0) {
                        continue;
                    }
                    break;

                case 0x57: // 'W'
                case 0x77: // 'w'
                case KEY_UP: // Up arrow
                    if(sel_b > 0 && sel_b <= menu->panels[sel_p]->num_items - 1) {
                        // Moving within panel
                        menu->panels[sel_p]->items[sel_b]->selected = 0;
                        menu->panels[sel_p]->items[--sel_b]->selected = 1;
                    } else {
                        // Moving between panels
                        panel_next = -1;
                        panel_offset_y = -1;
                        panel_offset_x = -1;
                        offset_y = 0;

                        // Width of currently selected panel
                        panel_width = getmaxx(menu->panels[sel_p]->window);

                        // Get closest panel beneath currently selected panel
                        for(int i = 0; i < menu->num_panels; ++i) {
                            // Width of iteration panel
                            panel_width_next = getmaxx(menu->panels[i]->window);

                            // If panel is below selected panel and intersects current X bounds
                            if(menu->panels[i]->coord.y < menu->panels[sel_p]->coord.y
                            && menu->panels[i]->coord.x + panel_width_next - 1 >= menu->panels[sel_p]->coord.x
                            && menu->panels[i]->coord.x <= menu->panels[sel_p]->coord.x + panel_width - 1) {
                                // If panel is closer (Y axis) to selected panel
                                offset_y = menu->panels[sel_p]->coord.y - menu->panels[i]->coord.y;
                                if(offset_y < panel_offset_y || panel_offset_y == -1) {
                                    panel_next = i;
                                    panel_offset_y = offset_y;
                                    panel_offset_x = menu->panels[i]->coord.x;
                                    break;
                                } else if(offset_y == panel_offset_y) {
                                    // If panel is on the same Y axis
                                    // Select leftmost item. panel_offset_x will always be set at this point, because
                                    // panel_offset_y must be set, which requires the previous condition to be met
                                    // at least once
                                    if(menu->panels[i]->coord.x < panel_offset_x) {
                                        panel_next = i;
                                        panel_offset_x = menu->panels[i]->coord.x;
                                    }
                                }
                            }
                        }
                        // If panel found
                        if(panel_next != -1 && menu->panels[panel_next]->num_items > 0) {
                            menu->panels[sel_p]->selected = 0;
                            menu->panels[sel_p]->items[sel_b]->selected = 0;

                            sel_p = panel_next;
                            sel_b = menu->panels[sel_p]->num_items - 1;

                            menu->panels[sel_p]->selected = 1;
                            menu->panels[sel_p]->items[sel_b]->selected = 1;
                        }
                    }
                    break;

                case 0x53: // 'S'
                case 0x73: // 's'
                case KEY_DOWN: // Down arrow
                    if(sel_b >= 0 && sel_b < menu->panels[sel_p]->num_items - 1) {
                        // Moving within panel
                        menu->panels[sel_p]->items[sel_b]->selected = 0;
                        menu->panels[sel_p]->items[++sel_b]->selected = 1;
                    } else {
                        // Moving between panels
                        panel_next = -1;
                        panel_offset_y = -1;
                        panel_offset_x = -1;
                        offset_y = 0;

                        // Width of currently selected panel
                        panel_width = getmaxx(menu->panels[sel_p]->window);

                        // Get closest panel beneath currently selected panel
                        for(int i = 0; i < menu->num_panels; ++i) {
                            // Width of iteration panel
                            panel_width_next = getmaxx(menu->panels[i]->window);

                            // If panel is below selected panel and intersects current X bounds
                            if(menu->panels[i]->coord.y > menu->panels[sel_p]->coord.y
                            && menu->panels[i]->coord.x + panel_width_next - 1 >= menu->panels[sel_p]->coord.x
                            && menu->panels[i]->coord.x <= menu->panels[sel_p]->coord.x + panel_width - 1) {
                                // If panel is closer (Y axis) to selected panel
                                offset_y = menu->panels[i]->coord.y - menu->panels[sel_p]->coord.y;
                                if(offset_y < panel_offset_y || panel_offset_y == -1) {
                                    panel_next = i;
                                    panel_offset_y = offset_y;
                                    panel_offset_x = menu->panels[i]->coord.x;
                                    break;
                                } else if(offset_y == panel_offset_y) {
                                    // If panel is on the same Y axis
                                    // Select leftmost item. panel_offset_x will always be set at this point, because
                                    // panel_offset_y must be set, which requires the previous condition to be met
                                    // at least once
                                    if(menu->panels[i]->coord.x < panel_offset_x) {
                                        panel_next = i;
                                        panel_offset_x = menu->panels[i]->coord.x;
                                    }
                                }
                            }
                        }

                        // If panel found
                        if(panel_next != -1 && menu->panels[panel_next]->num_items > 0) {
                            menu->panels[sel_p]->selected = 0;
                            menu->panels[sel_p]->items[sel_b]->selected = 0;

                            sel_p = panel_next;
                            sel_b = 0;

                            menu->panels[sel_p]->selected = 1;
                            menu->panels[sel_p]->items[sel_b]->selected = 1;
                        }
                    }
                    break;

                case 0x41: // 'A'
                case 0x61: // 'a'
                case KEY_LEFT: // Left arrow
                    panel_next = -1;
                    panel_offset_y = -1;
                    panel_offset_x = -1;
                    offset_y = 0;
                    offset_x = 0;

                    // Get leftmost coordinate of panel
                    sel_x = menu->panels[sel_p]->coord.x;

                    // Find closest intersecting upper left panel
                    for(int i = 0; i < menu->num_panels; ++i) {
                        // If panel is to the left of the selected panel and intersects Y bounds
                        if(menu->panels[i]->coord.x < sel_x
                        && menu->panels[i]->coord.y + menu->panels[i]->num_items >= menu->panels[sel_p]->coord.y
                        && menu->panels[i]->coord.y <= menu->panels[sel_p]->coord.y + menu->panels[sel_p]->num_items) {
                            // Prioritize panels closer on the x axis
                            offset_x = sel_x - menu->panels[i]->coord.x;
                            if(offset_x < panel_offset_x || panel_offset_x == -1) {
                                panel_next = i;
                                panel_offset_x = offset_x;
                                panel_offset_y = menu->panels[i]->coord.y;
                            } else if(offset_x == panel_offset_x) {
                                // Select topmost panel
                                if(menu->panels[i]->coord.y < panel_offset_y) {
                                    panel_next = i;
                                    panel_offset_y = menu->panels[i]->coord.y;
                                }
                            }
                        }
                    }

                    // If panel found
                    if(panel_next != -1 && menu->panels[panel_next]->num_items > 0) {
                        menu->panels[sel_p]->selected = 0;
                        menu->panels[sel_p]->items[sel_b]->selected = 0;

                        sel_p = panel_next;

                        if(sel_b >= menu->panels[sel_p]->num_items) {
                            sel_b = menu->panels[sel_p]->num_items - 1;
                        }

                        menu->panels[sel_p]->selected = 1;
                        menu->panels[sel_p]->items[sel_b]->selected = 1;
                    }
                    break;

                case 0x44: // 'D'
                case 0x64: // 'd'
                case KEY_RIGHT: // Right arrow
                    panel_next = -1;
                    panel_offset_y = -1;
                    panel_offset_x = -1;
                    offset_y = 0;
                    offset_x = 0;

                    // Get rightmost coordinate of panel
                    sel_x = getmaxx(menu->panels[sel_p]->window) + menu->panels[sel_p]->coord.x - 1;

                    // Find closest intersecting upper right panel
                    for(int i = 0; i < menu->num_panels; ++i) {
                        // If panel is to the right of the selected panel and intersects Y bounds
                        if(menu->panels[i]->coord.x > sel_x
                        && menu->panels[i]->coord.y + menu->panels[i]->num_items >= menu->panels[sel_p]->coord.y
                        && menu->panels[i]->coord.y <= menu->panels[sel_p]->coord.y + menu->panels[sel_p]->num_items) {
                            // Prioritize panels closer on the x axis
                            offset_x = menu->panels[i]->coord.x - sel_x;
                            if(offset_x < panel_offset_x || panel_offset_x == -1) {
                                panel_next = i;
                                panel_offset_x = offset_x;
                            }
                            else if(offset_x == panel_offset_x) {
                                // Select topmost panel
                                if(menu->panels[i]->coord.y < panel_offset_y) {
                                    panel_next = i;
                                    panel_offset_y = menu->panels[i]->coord.y;
                                }
                            }
                        }
                    }

                    // If panel found
                    if(panel_next != -1 && menu->panels[panel_next]->num_items > 0) {
                        menu->panels[sel_p]->selected = 0;
                        menu->panels[sel_p]->items[sel_b]->selected = 0;

                        sel_p = panel_next;

                        if(sel_b >= menu->panels[sel_p]->num_items) {
                            sel_b = menu->panels[sel_p]->num_items - 1;
                        }

                        menu->panels[sel_p]->selected = 1;
                        menu->panels[sel_p]->items[sel_b]->selected = 1;
                    }

                    // TODO?: If no panels intersect Y bounds, select closest X axis, closest upper Y axis panel
                    break;

                case 0x45: // 'E'
                case 0x65: // 'e'
                case 0x0A: // Newline (enter)
                case 0x20: // Spacebar
                    if (menu->panels[sel_p]->items[sel_b]->callback == NULL) {
                        quit = UI_BACK;
                    } else {
                        rval = menu->panels[sel_p]->items[sel_b]->callback(menu->window);
                        if (rval == UI_QUIT) {
                            quit = UI_QUIT;
                        }
                    }
                    break;

                case 0x51: // 'Q'
                case 0x71: // 'q'
                case 0x1B: // Escape
                case KEY_BACKSPACE: // Backspace
                    quit = UI_BACK;
                    break;

                default:;
            }

            // Draw border
            wborder(menu->window
                    , ACS_VLINE, ACS_VLINE // Side lines
                    , ACS_HLINE, ACS_HLINE // Top & bottom lines
                    , ACS_ULCORNER, ACS_URCORNER // Top corners
                    , ACS_LLCORNER, ACS_LRCORNER // Bottom corners
            );

            // Draw title
            mvwprintw(menu->window, 0, 5, " %s ", menu->label);

            // Re-draw the screen
            wrefresh(menu->window);

            for (int i = 0; i < menu->num_panels; ++i) {
                panel_width = getmaxx(menu->panels[i]->window);
                if(menu->panels[i]->coord.y + menu->panels[i]->num_items < max_y - 1
                && menu->panels[i]->coord.x + panel_width - 1 < max_x - 1) {
                    draw_panel(menu->panels[i]);
                }
            }

        } while (!quit && (key = wgetch(menu->window)) != 3); // 3 is CTRL+C'

        if (key == 3) {
            quit = UI_QUIT;
        } else {
            quit = UI_SUCCESS;
        }
    } else {
        quit = UI_INVALID;
    }
    return quit;
}

int get_panel_height(panel_t * panel) {
    int height = -1;

    if (panel != NULL) {
        // Number of panel buttons + label
        height = panel->num_items + 1;
    }
    return height;
}

void prompt(WINDOW * window, char * label, char * var, int var_size) {
    WINDOW * win;
    int y, x;

    if (window && label) {
        win = newwin(0, 0, 0, 0);
        wclear(win);

        getmaxyx(win, y, x);

        wattron(win, COLOR_PAIR(1));
        wattron(win, A_BOLD);

        echo();
        curs_set(1);
        keypad(win, TRUE);

        // Draw border
        wborder(win
                , ACS_VLINE, ACS_VLINE // Side lines
                , ACS_HLINE, ACS_HLINE // Top & bottom lines
                , ACS_ULCORNER, ACS_URCORNER // Top corners
                , ACS_LLCORNER, ACS_LRCORNER // Bottom corners
        );

        // Draw title
        mvwprintw(win, 0, 5, " %s ", "Prompt");

        // Draw prompt
        mvwprintw(win, y / 2 - 1, x / 2 - strlen(label) / 2, label);

        // Get string
        mvwgetnstr(win, y / 2, x / 2 - strlen(label) / 2, var, var_size - 1);

        noecho();
        curs_set(0);

        delwin(win);
    }
}

void curses_init() {
    initscr();
    start_color();

    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);

    // Debug color pair
    init_pair(3, COLOR_WHITE, COLOR_GREEN);

    // Start using defined color
    //attron(COLOR_PAIR(1));

    // Enable bright text
    //attron(A_BOLD);
}

void curses_term() {
    clear();
    refresh();
    endwin();
}
