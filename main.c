#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/tcp.h"
#include "lib/ui.h"

#include <curses.h>


void curses_init();
void curses_term();

void exit_callback(WINDOW * window);
void configure_callback(WINDOW * window);
void test_callback(WINDOW * window);

int QUIT = 0;

int main()
{
    curses_init();

    // Start using defined color
    attron(COLOR_PAIR(1));

    // Enable bright text
    attron(A_BOLD);

    // Menu items
    panel * panels[2];

    panels[0] = create_panel("Operation", 5, 30, 2, 3);
    add_panel_button(panels[0], create_button("Start Reactant Core server", test_callback));
    add_panel_button(panels[0], create_button("Start Reactant Node test", test_callback));
    add_panel_button(panels[0], create_button("Configure", configure_callback));
    add_panel_button(panels[0], create_button("Exit", NULL));

    panels[1] = create_panel("Other", 2, 30, 8, 3);
    add_panel_button(panels[1], create_button("About", test_callback));

    panels[0]->selected = 1;
    panels[0]->items[0]->selected = 1;

    menu * menu = create_menu("Reactant Primary Control");
    add_menu_panel(menu, panels[0]);
    add_menu_panel(menu, panels[1]);

    operate_menu(menu);

//    int status = start_server(10801);
//    if (status > 0)
//    {
//        printw("Failed to establish server!\n");
//        return 1;
//    }

    free_menu(menu);

    curses_term();
    return 0;
}

void curses_init()
{
    initscr();
    start_color();

    raw();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
}

void curses_term()
{
    clear();
    refresh();
    endwin();
}

void exit_callback(WINDOW * window)
{
    QUIT = 1;
}

void configure_callback(WINDOW * window)
{
    WINDOW * win = newwin(0, 0, 0, 0);

    int key = 0;

    menuitem * buttons[1];
    buttons[0] = create_button("Back", exit_callback);

    do
    {
        wclear(win);

        switch (key)
        {
            case 10:
            case KEY_ENTER:
                buttons[0]->callback(window);

                break;
            default:;
        }

        // TODO: Create a create_menu or similar named function to abstract this process

        // Draw border
        wborder(win
                , ACS_VLINE, ACS_VLINE // Side lines
                , ACS_HLINE, ACS_HLINE // Top & bottom lines
                , ACS_ULCORNER, ACS_URCORNER // Top corners
                , ACS_LLCORNER, ACS_LRCORNER // Bottom corners
        );

        // Draw title
        mvwprintw(win, 0, 5, " Reactant Configuration ");

        mvwprintw(win, 2, 3, buttons[0]->label);

        wrefresh(win);

    } while (!QUIT && (key = wgetch(win)) != 3);

    // If user exits by selecting "back", QUIT should reset to 0.
    // If user exits by using CTRL+C, QUIT should be set to 1 so that the program exits.
    QUIT ^= 1;

    free(buttons[0]->label);
    free(buttons[0]);

    wclear(win);
    wrefresh(win);
    delwin(win);
}

void test_callback(WINDOW * window)
{
    int y, x;
    getmaxyx(stdscr, y, x);
    mvwprintw(window, y / 2, x / 2 - 7, "This is a test!");
}