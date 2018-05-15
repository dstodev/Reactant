#ifndef REACTANT_UI_H
#define REACTANT_UI_H

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <string.h>


typedef struct _coordinate_t
{
    int y;
    int x;

} coordinate_t;

typedef struct _menu_item_t
{
    char * label;
    int (* callback)(WINDOW *);
    uint8_t selected;

} menu_item_t;

typedef struct _panel_t
{
    char * label;
    menu_item_t ** items;
    int num_items;
    uint8_t selected;
    coordinate_t coord;
    WINDOW * window;

} panel_t;

typedef struct _menu_t
{
    char * label;
    panel_t ** panels;
    int num_panels;
    WINDOW * window;

} menu_t;

typedef enum _ui_rval_t
{
    UI_INVALID = -1,
    UI_SUCCESS,
    UI_BACK,
    UI_QUIT

} ui_rval_t;

// Button functions
menu_item_t * create_button(char * label, int (* callback)(WINDOW *));
void free_button(menu_item_t * button);

// Panel functions
panel_t * create_panel(char * label, int y, int x);
void free_panel(panel_t * panel);
void add_panel_button(panel_t * panel, menu_item_t * button);
void draw_panel(panel_t * panel);

// Menu functions
menu_t * create_menu(char * label);
void free_menu(menu_t * menu);
void add_menu_panel(menu_t * menu, panel_t * panel);
int operate_menu(menu_t * menu);

// Utility functions
int get_panel_height(panel_t * panel);

// Callbacks
void prompt(WINDOW * window, char * label, char * var, int var_size);

void curses_init();
void curses_term();

#endif // REACTANT_UI_H
