#include "AnglePropertyEditor.h"

#include "iradiant.h"
#include "gtkutil/IconTextButton.h"

#include <gtk/gtk.h>

namespace ui
{

// Constructor
AnglePropertyEditor::AnglePropertyEditor(Entity* entity, const std::string& key)
{
    // Construct a 3x3 table to contain the directional buttons
    GtkWidget* table = gtk_table_new(3, 3, TRUE);

    // Add buttons
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_n24.png"), false
        ),
        1, 2,   // column boundaries
        0, 1    // row boundaries
    );
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_s24.png"), false
        ),
        1, 2,
        2, 3
    );
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_e24.png"), false
        ),
        2, 3,
        1, 2
    );
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_w24.png"), false
        ),
        0, 1,
        1, 2
    );
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_ne24.png"), false
        ),
        2, 3,
        0, 1
    );
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_se24.png"), false
        ),
        2, 3,
        2, 3
    );
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_sw24.png"), false
        ),
        0, 1,
        2, 3
    );
    gtk_table_attach_defaults(
        GTK_TABLE(table), 
        gtkutil::IconTextButton(
            "", GlobalRadiant().getLocalPixbuf("arrow_nw24.png"), false
        ),
        0, 1,
        0, 1
    );

    // Pack table into an hbox/vbox and set as the widget
    GtkWidget* hbx = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbx), table, TRUE, FALSE, 0);

    _widget = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(_widget), hbx, FALSE, FALSE, 0);
}

}
