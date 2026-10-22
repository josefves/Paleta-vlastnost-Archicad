// *****************************************************************************
// PaletaVlastnosti - paleta s vlastnostmi vybranych prvku
// *****************************************************************************

#ifndef PROPERTYPALETTE_HPP
#define PROPERTYPALETTE_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "HashSet.hpp"

#include "PropertyData.hpp"
#include "ResourceIds.hpp"

class PropertyPalette final : public DG::Palette,
							  public DG::PanelObserver,
							  public DG::ListBoxObserver,
							  public DG::ButtonItemObserver,
							  public DG::TextEditBaseObserver,
							  public DG::CheckItemObserver
{
public:
	static constexpr short MenuResId		= ID_PALETTE_MENU;
	static constexpr short MenuItemIndex	= 1;

	virtual ~PropertyPalette ();

	static bool				HasInstance ();
	static void				CreateInstance ();
	static PropertyPalette&	GetInstance ();
	static void				DestroyInstance ();

	void Show ();
	void Hide ();

	static GSErrCode	RegisterPaletteControlCallBack ();
	static GSErrCode	SelectionChangeHandler (const API_Neig* selElemNeig);
	static GSErrCode	ElementEventHandler (const API_NotifyElementType* elemType);
	static GSErrCode	ProjectEventHandler (API_NotifyEventID notifID, Int32 param);

private:
	enum ItemIds {
		InfoTextId		= 1,
		FilterLabelId	= 2,
		FilterEditId	= 3,
		BuiltInCheckId	= 4,
		PropListId		= 5,
		PropNameTextId	= 6,
		ValueEditId		= 7,
		ValuePopupId	= 8,
		ApplyButtonId	= 9,
		DefaultButtonId	= 10,
		StatusTextId	= 11
	};

	enum Columns {
		NameColumn	= 1,
		ValueColumn	= 2
	};

	struct ListEntry {
		UInt32	groupIndex	= 0;
		Int32	propIndex	= -1;	// -1 = radek se skupinou
	};

	DG::LeftText			infoText;
	DG::LeftText			filterLabel;
	DG::TextEdit			filterEdit;
	DG::CheckBox			builtInCheck;
	DG::SingleSelListBox	propList;
	DG::LeftText			propNameText;
	DG::TextEdit			valueEdit;
	DG::PopUp				valuePopup;
	DG::Button				applyButton;
	DG::Button				defaultButton;
	DG::LeftText			statusText;

	short					returnHotKey	= 0;
	short					enterHotKey		= 0;

	GS::Array<API_Guid>				selection;
	GS::Array<API_Guid>				observedElements;
	GS::Array<PropData::GroupRow>	groups;
	GS::Array<ListEntry>			entries;
	GS::HashSet<GS::Guid>			collapsedGroups;
	API_Guid						selectedDefinitionGuid	= APINULLGuid;
	bool							dataIsDirty				= true;
	bool							isApplying				= false;
	bool							pendingDataRefresh		= false;

	static GS::Ref<PropertyPalette>	instance;

	PropertyPalette ();

	void	RefreshFromSelection ();
	void	RefreshData ();
	void	RebuildList ();
	void	UpdateEditor ();
	void	UpdateColumns ();
	void	UpdateInfoText ();
	void	UpdateObservers ();
	void	Apply ();
	void	ResetSelectedToDefault ();
	void	SetStatus (const GS::UniString& text);
	void	SetMenuItemCheckedState (bool isChecked);
	bool	IsElementInSelection (const API_Guid& elemGuid) const;

	const PropData::PropRow*	GetSelectedRow () const;
	const ListEntry*			GetEntry (short listItem) const;

	// DG::PanelObserver
	virtual void	PanelResized (const DG::PanelResizeEvent& ev) override;
	virtual void	PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	virtual void	PanelIdle (const DG::PanelIdleEvent& ev) override;
	virtual void	PanelHotkeyPressed (const DG::PanelHotKeyEvent& ev, bool* processed) override;

	// DG::ListBoxObserver
	virtual void	ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev) override;
	virtual void	ListBoxClicked (const DG::ListBoxClickEvent& ev) override;
	virtual void	ListBoxDoubleClicked (const DG::ListBoxDoubleClickEvent& ev) override;

	// DG::ButtonItemObserver
	virtual void	ButtonClicked (const DG::ButtonClickEvent& ev) override;

	// DG::TextEditBaseObserver
	virtual void	TextEditChanged (const DG::TextEditChangeEvent& ev) override;

	// DG::CheckItemObserver
	virtual void	CheckItemChanged (const DG::CheckItemChangeEvent& ev) override;

	static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);
};

#endif // PROPERTYPALETTE_HPP
