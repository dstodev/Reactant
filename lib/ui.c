#include "ui.h"

menuitem * create_button(char * label, void (* callback)(WINDOW *))
{
    if (label != NULL)
    {
        menuitem * button = malloc(sizeof(struct menuitem_t));

        button->label = malloc(strlen(label) + 1);
        strcpy(button->label, label);

        button->callback = callback;

        button->selected = 0;

        return button;
    }

    return NULL;
}

void free_button(menuitem * button)
{
    if (button != NULL)
    {
        free(button->label);
        free(button);
    }
}

panel * create_panel(char * label, int y_size, int x_size, int y, int x)
{
    if (label != NULL)
    {
        panel * panel = malloc(sizeof(struct panel_t));

        panel->label = malloc(strlen(label) + 1);
        strcpy(panel->label, label);

        panel->items = NULL;
        panel->num_items = 0;

        panel->selected = 0;

        panel->coord.y = y;
        panel->coord.x = x;

        panel->window = newwin(y_size, x_size, y, x);
        wattron(panel->window, COLOR_PAIR(1));
        wattron(panel->window, A_DIM);

        return panel;
    }
    return NULL;
}

void free_panel(panel * panel)
{
    if (panel != NULL)
    {
        wclear(panel->window);
        wrefresh(panel->window);
        delwin(panel->window);

        free(panel->label);

        for (int i = 0; i < panel->num_items; ++i)
        {
            free_button(panel->items[i]);
        }

        free(panel->items);
        free(panel);
    }
}

void add_panel_button(panel * panel, menuitem * button)
{
    if (panel != NULL)
    {
        panel->items = realloc(panel->items, sizeof(menuitem *) * ((size_t) panel->num_items + 1));
        panel->items[panel->num_items] = button;

        ++panel->num_items;
    }
}

void draw_panel(panel * panel)
{
    if (panel != NULL)
    {
        if (panel->selected == 1)
        {
            wattroff(panel->window, A_DIM);
            wattron(panel->window, A_BOLD);
        }

        // Draw label
        mvwprintw(panel->window, 0, 0, panel->label);

        // Draw buttons inside panel
        for (int i = 0; i < panel->num_items; ++i)
        {
            mvwaddch(panel->window, 1 + i, 0, ACS_BULLET);

            if (panel->items[i]->selected == 1)
            {
                wattroff(panel->window, A_BOLD);
                wattron(panel->window, COLOR_PAIR(2));
            }

            mvwprintw(panel->window, 1 + i, 2, panel->items[i]->label);

            if (panel->items[i]->selected == 1)
            {
                wattron(panel->window, A_BOLD);
                wattroff(panel->window, COLOR_PAIR(2));
            }
        }

        if (panel->selected == 1)
        {
            wattron(panel->window, A_DIM);
            wattroff(panel->window, A_BOLD);
        }

        wrefresh(panel->window);
    }
}

menu * create_menu(char * label)
{
    if (label != NULL)
    {
        menu * menu = malloc(sizeof(struct menu_t));

        menu->label = malloc(strlen(label) + 1);
        strcpy(menu->label, label);

        menu->panels = NULL;
        menu->num_panels = 0;

        menu->window = newwin(0, 0, 0, 0);
        wclear(menu->window);

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

void free_menu(menu * menu)
{
    if (menu != NULL)
    {
        wclear(menu->window);
        wrefresh(menu->window);
        delwin(menu->window);

        free(menu->label);

        for (int i = 0; i < menu->num_panels; ++i)
        {
            free_panel(menu->panels[i]);
        }

        free(menu->panels);
        free(menu);
    }
}

void add_menu_panel(menu * menu, panel * panel)
{
    if (menu != NULL && panel != NULL)
    {
        menu->panels = realloc(menu->panels, sizeof(struct panel_t *) * ((size_t) menu->num_panels + 1));
        menu->panels[menu->num_panels] = panel;

        ++menu->num_panels;
    }
}

void operate_menu(menu * menu)
{
    if (menu != NULL)
    {
        wattron(menu->window, COLOR_PAIR(1));
        wattron(menu->window, A_BOLD);

        keypad(menu->window, TRUE);

        // Menu
        int sel_p = 0; // Selected panel
        int sel_b = 0; // Selected button

        // Key that started iteration
        int key = 0;
        int quit = 0;

        // Dimensions of the terminal
        int max_y = 0;
        int max_x = 0;
        getmaxyx(menu->window, max_y, max_x);

        do
        {
            // Clear screen
            wclear(menu->window);

            // Process the pressed key
            switch (key)
            {
                case KEY_RESIZE:
                    getmaxyx(menu->window, max_y, max_x);

                    if (max_y == 0 || max_x == 0)
                    {
                        continue;
                    }

                    break;
                case KEY_UP:
                    menu->panels[sel_p]->selected = 0;
                    menu->panels[sel_p]->items[sel_b]->selected = 0;
                    if (sel_b > 0)
                    {
                        --sel_b;
                    }
                    else
                    {
                        if (sel_p > 0)
                        {
                            --sel_p;
                            sel_b = menu->panels[sel_b]->num_items - 1;
                        }
                    }
                    menu->panels[sel_p]->selected = 1;
                    menu->panels[sel_p]->items[sel_b]->selected = 1;

                    break;
                case KEY_DOWN:
                    menu->panels[sel_p]->selected = 0;
                    menu->panels[sel_p]->items[sel_b]->selected = 0;
                    if (sel_b < menu->panels[sel_p]->num_items - 1)
                    {
                        ++sel_b;
                    }
                    else
                    {
                        if (sel_p < menu->num_panels - 1)
                        {
                            ++sel_p;
                            sel_b = 0;
                        }
                    }
                    menu->panels[sel_p]->selected = 1;
                    menu->panels[sel_p]->items[sel_b]->selected = 1;

                    break;
                case 10: // 10 is newline
                case KEY_ENTER:
                    if (menu->panels[sel_p]->items[sel_b]->callback == NULL)
                    {
                        quit = 1;
                    }
                    else
                    {
                        menu->panels[sel_p]->items[sel_b]->callback(menu->window);
                    }

                    break;
                default:;
            }

            // TODO: Allow CTRL+C to propogate back and end program

            if (max_y < 25 || max_x < 100)
            {
                continue;
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

            for (int i = 0; i < menu->num_panels; ++i)
            {
                draw_panel(menu->panels[i]);
            }

        } while (!quit && (key = wgetch(menu->window)) != 3); // 3 is CTRL+C
    }
}