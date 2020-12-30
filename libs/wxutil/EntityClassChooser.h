#pragma once

#include "iradiant.h"
#include "ieclass.h"

#include "wxutil/preview/ModelPreview.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/TreeModelFilter.h"
#include "wxutil/TreeModel.h"
#include "wxutil/TreeView.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/menu/PopupMenu.h"

#include <memory>
#include <sigc++/connection.h>

namespace wxutil
{

class EntityClassChooser;
typedef std::shared_ptr<EntityClassChooser> EntityClassChooserPtr;

/**
 * Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location.
 */
class EntityClassChooser :
    public DialogBase,
    private XmlResourceBasedWidget
{
public:
    // Treemodel definition
    struct TreeColumns :
        public TreeModel::ColumnRecord
    {
        TreeColumns() :
            name(add(TreeModel::Column::IconText)),
            isFolder(add(TreeModel::Column::Boolean)),
            isFavourite(add(TreeModel::Column::Boolean))
        {}

        TreeModel::Column name;
        TreeModel::Column isFolder;
        TreeModel::Column isFavourite;
    };

private:
    TreeColumns _columns;

    // Tree model holding the classnames
    TreeModel::Ptr _treeStore;
    wxutil::TreeModelFilter::Ptr _treeModelFilter;
    TreeView* _treeView;

    // Delegated object for loading entity classes in a separate thread
    class ThreadedEntityClassLoader;
    std::unique_ptr<ThreadedEntityClassLoader> _eclassLoader; // PIMPL idiom

    // Last selected classname
    std::string _selectedName;

    // Class we should select when the treemodel is populated
    std::string _classToHighlight;

    // Model preview widget
    ModelPreviewPtr _modelPreview;

    PanedPosition _panedPosition;

    sigc::connection _defsReloaded;

    PopupMenuPtr _popupMenu;

    
    wxDataViewItem _emptyFavouritesLabel;

    enum class TreeMode
    {
        ShowAll,
        ShowFavourites,
    };
    TreeMode _mode;

private:
    // Constructor. Creates the GTK widgets.
    EntityClassChooser();

    void setTreeViewModel();
    void loadEntityClasses();

    // Widget construction helpers
    void setupTreeView();

    // Update the usage panel with information from the provided entityclass
    void updateUsageInfo(const std::string& eclass);

    // Updates the member variables based on the current tree selection
    void updateSelection();

    // Button callbacks
    void onCancel(wxCommandEvent& ev);
    void onOK(wxCommandEvent& ev);
    void onSelectionChanged(wxDataViewEvent& ev);
    void onDeleteEvent(wxCloseEvent& ev);
    void onTreeStorePopulationFinished(TreeModel::PopulationFinishedEvent& ev);
    
    bool _testAddToFavourites();
    bool _testRemoveFromFavourites();
    void _onSetFavourite(bool isFavourite);
    void setFavouriteRecursively(wxutil::TreeModel::Row& row, bool isFavourite);

    void onMainFrameShuttingDown();
    
    // This is where the static shared_ptr of the singleton instance is held.
    static EntityClassChooserPtr& InstancePtr();

public:
    // Public accessor to the singleton instance
    static EntityClassChooser& Instance();

    // Sets the tree selection to the given entity class
    void setSelectedEntityClass(const std::string& eclass);

    // Sets the tree selection to the given entity class
    const std::string& getSelectedEntityClass() const;

    int ShowModal() override;

    /**
     * Convenience function:
     * Display the dialog and block awaiting the selection of an entity class,
     * which is returned to the caller. If the dialog is cancelled or no
     * selection is made, and empty string will be returned.
     */
    static std::string chooseEntityClass(const std::string& preselectEclass = std::string());
};

} // namespace ui
