#include "SkinChooser.h"

#include "modelskin.h"
#include "groupdialog.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */

namespace {
	
	// Tree column enum
	enum {
		DISPLAYNAME_COL,
		FULLNAME_COL,
		N_COLUMNS
	};
	
}

// Constructor
SkinChooser::SkinChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Set up window
	GtkWindow* gd = GroupDialog_getWindow();

	gtk_window_set_transient_for(GTK_WINDOW(_widget), gd);
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose skin");

	// Set the default size of the window
	gint w, h;
	gtk_window_get_size(gd, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w, h);
	
	// Vbox contains treeview and buttons panel
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
}

// Create the TreeView
GtkWidget* SkinChooser::createTreeView() {
	
	// Create the treestore
	_treeStore = gtk_tree_store_new(N_COLUMNS, 
  									G_TYPE_STRING, 
  									G_TYPE_STRING);
  									
	// Create the tree view
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	
	// Single column to display the skin name
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), 
								gtkutil::TextColumn("Skin", DISPLAYNAME_COL));
	
	// Pack treeview into a ScrolledFrame and return
	return gtkutil::ScrolledFrame(_treeView);
}

// Create the buttons panel
GtkWidget* SkinChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);
	gtk_box_pack_end(GTK_BOX(hbx), 
					   gtk_button_new_from_stock(GTK_STOCK_OK),
					   TRUE, TRUE, 0);	
	gtk_box_pack_end(GTK_BOX(hbx), 
					   gtk_button_new_from_stock(GTK_STOCK_CANCEL),
					   TRUE, TRUE, 0);
					   
	return gtkutil::RightAlignment(hbx);	
}

// Show the dialog and block for a selection
std::string SkinChooser::showAndBlock(const std::string& model) {
	
	// Set the model and populate the skins
	_model = model;
	populateSkins();
	
	// Show the dialog
	gtk_widget_show_all(_widget);
	return "_none";
}

// Populate the list of skins
void SkinChooser::populateSkins() {
	
	// Get the list of skins for the model
	ModelSkinList skins = GlobalModelSkinCache().getSkinsForModel(_model);
		
	// Add each skin to the tree view
	for (ModelSkinList::iterator i = skins.begin();
		 i != skins.end();
		 ++i)
	{
		GtkTreeIter iter;
		gtk_tree_store_append(_treeStore, &iter, NULL);
		gtk_tree_store_set(_treeStore, &iter, DISPLAYNAME_COL, i->c_str(), -1); 		
	}
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin(const std::string& model) {
	
	// The static instance
	static SkinChooser _instance;
	
	// Show and block the instance, returning the selected skin
	return _instance.showAndBlock(model);	
}

}
