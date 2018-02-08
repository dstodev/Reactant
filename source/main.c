#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reactant_peripherals.h"
#include "reactant_network.h"
#include "reactant_util.h"
#include "reactant_ui.h"

#include <curses.h>

void spi_test();
void ui_test();
void queue_test();

int configure_callback(WINDOW * PH(window));
int test_callback(WINDOW * window);

int main()
{
	// start_discovery_server(10112);

	spi_test();

    return 0;
}

void spi_test()
{
	peripheral_init();
	peripheral_spi_init();

	int rval = 0;

	rval = mcp3008_read_channel(1);
	return;

	while(rval < 950)
    {

		fprintf(stderr, "Ch %d: %d\n", 1, rval);
		
		bcm2835_delay(100);
	}

	peripheral_spi_term();
	peripheral_term();
}

void ui_test()
{
    /*##############
    # Run Reactant #
    ##############*/
    curses_init();

    // Menu items
    panel_t * panels[4];

    panels[0] = create_panel("Operation", 2, 3); // 2, 3
    add_panel_button(panels[0], create_button("Start Reactant Core server", test_callback));
    add_panel_button(panels[0], create_button("Configure", configure_callback));
    add_panel_button(panels[0], create_button("Exit", NULL));

    panels[1] = create_panel("Other", 2, 40);
    add_panel_button(panels[1], create_button("About", test_callback));
    add_panel_button(panels[1], create_button("Test Reactant Node device", test_callback));

    panels[2] = create_panel("TESTPANEL1", 7, 3);
    add_panel_button(panels[2], create_button("TESTBUTTON1", test_callback));
    add_panel_button(panels[2], create_button("TESTBUTTON2", test_callback));
    add_panel_button(panels[2], create_button("TESTBUTTON3", test_callback));

    panels[3] = create_panel("TESTPANEL2", 7, 40);
    add_panel_button(panels[3], create_button("TESTBUTTON4", test_callback));
    add_panel_button(panels[3], create_button("TESTBUTTON5", test_callback));
    add_panel_button(panels[3], create_button("TESTBUTTON6", test_callback));

    panels[0]->selected = 1;
    panels[0]->items[0]->selected = 1;

    menu_t * menu = create_menu("Reactant Primary Control");
    add_menu_panel(menu, panels[0]);
    add_menu_panel(menu, panels[1]);
    add_menu_panel(menu, panels[2]);
    add_menu_panel(menu, panels[3]);

    operate_menu(menu);

    //    int status = start_server(10801);
    //    if (status > 0)
    //    {
    //        printw("Failed to establish server!\n");
    //        return 1;
    //    }

    free_menu(menu);
    curses_term();
}

void queue_test()
{
    queue_t q;

    int items[10];
    int rval = 0;

    debug_control(ENABLE);

    queue_construct(&q, 10);

    for(int i = 0; i < q.capacity; ++i)
    {
        items[i] = i;
        queue_check(enqueue(&q, &items[i]));
    }

    printf("Capacity: %d, Size: %d\n", q.capacity, q.size);

    printf("test\n");
    queue_check(enqueue(&q, &items[0]));

    for(int i = 0; i < 5; ++i)
    {
        queue_check(dequeue(&q, &rval));
        printf("%d\n", rval);
    }

    for(int i = 0; i < 5; ++i)
    {
        queue_check(enqueue_blocking(&q, &items[7]));
    }

    for(int i = 0; i < q.capacity; ++i)
    {
        queue_check(dequeue_blocking(&q, &rval));
        printf("%d\n", rval);
    }

    printf("test\n");

    queue_check(dequeue(&q, &rval));
    printf("%d\n", rval);

    queue_destruct(&q);
}

int configure_callback(WINDOW * PH(window))
{
    int rval = 0;

    panel_t * panel = create_panel("Operation", 2, 3);
    add_panel_button(panel, create_button("Back", NULL));

    menu_t * menu = create_menu("Reactant Configuration");
    add_menu_panel(menu, panel);

    rval = operate_menu(menu);

    free_menu(menu);

    if (rval == UI_QUIT)
    {
        return UI_QUIT;
    }

    return rval;
}

int test_callback(WINDOW * window)
{
    int y, x;
    getmaxyx(stdscr, y, x);
    mvwprintw(window, y / 2, x / 2 - 7, "This is a test!");

    return 0;
}
