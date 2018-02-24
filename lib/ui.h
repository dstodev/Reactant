#ifndef UI_H
#define UI_H

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <string.h>

typedef struct coordinate_t
{
    int y;
    int x;

} coordinate;

typedef struct menuitem_t
{
    char * label;
    void (* callback)(WINDOW *);
    uint8_t selected;

} menuitem;

typedef struct panel_t
{
    char * label;
    menuitem ** items;
    int num_items;
    uint8_t selected;
    coordinate coord;
    WINDOW * window;

} panel;

typedef struct menu_t
{
    char * label;
    panel ** panels;
    int num_panels;
    WINDOW * window;
} menu;

menuitem * create_button(char * label, void (* callback)(WINDOW *));
void free_button(menuitem * button);

panel * create_panel(char * label, int y_size, int x_size, int y, int x);
void free_panel(panel * panel);
void add_panel_button(panel * panel, menuitem * button);
void draw_panel(panel * panel);

menu * create_menu(char * label);
void free_menu(menu * menu);
void add_menu_panel(menu * menu, panel * panel);
void operate_menu(menu * menu);


#endif // UI_H
