#include "PickpocketComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
PickpocketComponentEditor::RegHelper PickpocketComponentEditor::regHelper;

// Constructor
PickpocketComponentEditor::PickpocketComponentEditor(Component& component) :
	_component(&component),
	_itemSpec(SpecifierType::SET_ITEM())
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Item:</b>"),
        FALSE, FALSE, 0
    );
	gtk_box_pack_start(
		GTK_BOX(_widget), _itemSpec.getWidget(), FALSE, FALSE, 0
	);

    // Populate the SpecifierEditCombo with the first specifier
    _itemSpec.setSpecifier(
        component.getSpecifier(Specifier::FIRST_SPECIFIER)
    );
}

// Destructor
PickpocketComponentEditor::~PickpocketComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* PickpocketComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void PickpocketComponentEditor::writeToComponent() const {
    assert(_component);
    _component->setSpecifier(
        Specifier::FIRST_SPECIFIER, _itemSpec.getSpecifier()
    );
}

} // namespace ce

} // namespace objectives
