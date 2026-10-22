// *****************************************************************************
// PaletaVlastnosti - paleta s vlastnostmi vybranych prvku
// *****************************************************************************

#include "PropertyPalette.hpp"

#include "Color.hpp"

#include <algorithm>

using PropData::GetUIString;

namespace {

const GS::Guid	PaletteGuid ("{88094033-EFF5-4B26-A9FB-C87E6A24AD3D}");

const Gfx::Color ReadOnlyColor (128, 128, 128);

enum UIStringIds {
	Str_HeaderName			= 1,
	Str_HeaderValue			= 2,
	Str_NothingSelected		= 3,
	Str_SelectedCount		= 4,
	Str_Mixed				= 5,
	Str_Yes					= 6,
	Str_No					= 7,
	Str_ReadOnly			= 8,
	Str_AppliedTo			= 9,
	Str_FailedFor			= 10,
	Str_InvalidValue		= 11,
	Str_UndoName			= 12,
	Str_ResetFor			= 13,
	Str_ListReadOnly		= 14,
	Str_LimitedSelection	= 15,
	Str_SelectProperty		= 16,
	Str_Collapsed			= 17,
	Str_Expanded			= 18,
	Str_Separator			= 19,
	Str_TooLong				= 20,
	Str_NoValue				= 21
};

enum PopupValues : DGUserData {
	PopupValue_Mixed	= 0,
	PopupValue_True		= 1,
	PopupValue_False	= 2
	// u vyctu: index hodnoty + 1
};

void AppendPopupItem (DG::PopUp& popup, const GS::UniString& text, DGUserData value)
{
	popup.AppendItem ();
	const short item = popup.GetItemCount ();
	popup.SetItemText (item, text);
	popup.SetItemValue (item, value);
}

GS::UniString NumberToString (UInt32 value)
{
	return GS::UniString::Printf ("%u", static_cast<unsigned int> (value));
}

}

GS::Ref<PropertyPalette>	PropertyPalette::instance;

// =============================================================================
// Zivotni cyklus
// =============================================================================

PropertyPalette::PropertyPalette () :
	DG::Palette		(ACAPI_GetOwnResModule (), ID_PALETTE_DLG, ACAPI_GetOwnResModule (), PaletteGuid),
	infoText		(GetReference (), InfoTextId),
	filterLabel		(GetReference (), FilterLabelId),
	filterEdit		(GetReference (), FilterEditId),
	builtInCheck	(GetReference (), BuiltInCheckId),
	propList		(GetReference (), PropListId),
	propNameText	(GetReference (), PropNameTextId),
	valueEdit		(GetReference (), ValueEditId),
	valuePopup		(GetReference (), ValuePopupId),
	applyButton		(GetReference (), ApplyButtonId),
	defaultButton	(GetReference (), DefaultButtonId),
	statusText		(GetReference (), StatusTextId)
{
	DG::PanelObserver& panelObserver = *this;
	Attach (panelObserver);

	propList.Attach (static_cast<DG::ListBoxObserver&> (*this));
	applyButton.Attach (static_cast<DG::ButtonItemObserver&> (*this));
	defaultButton.Attach (static_cast<DG::ButtonItemObserver&> (*this));
	filterEdit.Attach (static_cast<DG::TextEditBaseObserver&> (*this));
	builtInCheck.Attach (static_cast<DG::CheckItemObserver&> (*this));

	returnHotKey	= RegisterHotKey (DG::Key::Return);
	enterHotKey		= RegisterHotKey (DG::Key::Enter);

	propList.SetTabFieldCount (2);
	propList.SetHeaderItemCount (2);
	propList.SetHeaderSynchronState (false);
	propList.SetHeaderItemText (NameColumn, GetUIString (Str_HeaderName));
	propList.SetHeaderItemText (ValueColumn, GetUIString (Str_HeaderValue));
	UpdateColumns ();

	builtInCheck.Check ();
	valuePopup.Hide ();

	ACAPI_ProjectOperation_CatchProjectEvent (APINotify_New | APINotify_NewAndReset | APINotify_Open | APINotify_Close | APINotify_Quit,
											  ProjectEventHandler);

	EnableIdleEvent ();
	BeginEventProcessing ();
	RefreshFromSelection ();
}


PropertyPalette::~PropertyPalette ()
{
	EndEventProcessing ();

	for (const API_Guid& guid : observedElements)
		ACAPI_Element_DetachObserver (guid);
	observedElements.Clear ();

	propList.Detach (static_cast<DG::ListBoxObserver&> (*this));
	applyButton.Detach (static_cast<DG::ButtonItemObserver&> (*this));
	defaultButton.Detach (static_cast<DG::ButtonItemObserver&> (*this));
	filterEdit.Detach (static_cast<DG::TextEditBaseObserver&> (*this));
	builtInCheck.Detach (static_cast<DG::CheckItemObserver&> (*this));

	DG::PanelObserver& panelObserver = *this;
	Detach (panelObserver);
}


bool PropertyPalette::HasInstance ()
{
	return instance != nullptr;
}


void PropertyPalette::CreateInstance ()
{
	DBASSERT (!HasInstance ());
	instance = new PropertyPalette ();
	ACAPI_KeepInMemory (true);
}


PropertyPalette& PropertyPalette::GetInstance ()
{
	DBASSERT (HasInstance ());
	return *instance;
}


void PropertyPalette::DestroyInstance ()
{
	instance = nullptr;
}


void PropertyPalette::Show ()
{
	DG::Palette::Show ();
	SetMenuItemCheckedState (true);
	if (dataIsDirty)
		RefreshFromSelection ();
}


void PropertyPalette::Hide ()
{
	DG::Palette::Hide ();
	SetMenuItemCheckedState (false);
}


void PropertyPalette::SetMenuItemCheckedState (bool isChecked)
{
	API_MenuItemRef	itemRef {};
	GSFlags			itemFlags {};

	itemRef.menuResID = MenuResId;
	itemRef.itemIndex = MenuItemIndex;

	ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
	if (isChecked)
		itemFlags |= API_MenuItemChecked;
	else
		itemFlags &= ~API_MenuItemChecked;
	ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
}

// =============================================================================
// Nacteni dat
// =============================================================================

void PropertyPalette::RefreshFromSelection ()
{
	selection.Clear ();

	API_SelectionInfo	selectionInfo {};
	GS::Array<API_Neig>	selNeigs;
	if (ACAPI_Selection_Get (&selectionInfo, &selNeigs, false, false) == NoError) {
		GS::HashSet<GS::Guid> unique;
		for (const API_Neig& neig : selNeigs) {
			if (neig.guid == APINULLGuid)
				continue;
			if (unique.Add (APIGuid2GSGuid (neig.guid)))
				selection.Push (neig.guid);
		}
	}
	BMKillHandle (reinterpret_cast<GSHandle*> (&selectionInfo.marquee.coords));

	UpdateObservers ();
	RefreshData ();
}


void PropertyPalette::RefreshData ()
{
	pendingDataRefresh = false;
	if (!IsVisible ()) {
		dataIsDirty = true;
		return;
	}
	dataIsDirty = false;

	groups = PropData::Collect (selection, builtInCheck.IsChecked ());
	UpdateInfoText ();
	RebuildList ();
}


void PropertyPalette::UpdateObservers ()
{
	for (const API_Guid& guid : observedElements) {
		if (!IsElementInSelection (guid))
			ACAPI_Element_DetachObserver (guid);
	}

	GS::HashSet<GS::Guid> alreadyObserved;
	for (const API_Guid& guid : observedElements)
		alreadyObserved.Add (APIGuid2GSGuid (guid));

	GS::Array<API_Guid> newObserved;
	const UInt32 limit = std::min<UInt32> (selection.GetSize (), PropData::MaxElemsToEvaluate);
	for (UInt32 i = 0; i < limit; ++i) {
		const API_Guid& guid = selection[i];
		if (alreadyObserved.Contains (APIGuid2GSGuid (guid)) || ACAPI_Element_AttachObserver (guid) == NoError)
			newObserved.Push (guid);
	}
	observedElements = newObserved;
}


bool PropertyPalette::IsElementInSelection (const API_Guid& elemGuid) const
{
	for (const API_Guid& guid : selection) {
		if (guid == elemGuid)
			return true;
	}
	return false;
}


void PropertyPalette::UpdateInfoText ()
{
	if (selection.IsEmpty ()) {
		infoText.SetText (GetUIString (Str_NothingSelected));
		return;
	}

	if (selection.GetSize () == 1) {
		API_Elem_Head elemHead {};
		elemHead.guid = selection[0];
		GS::UniString text;
		if (ACAPI_Element_GetHeader (&elemHead) == NoError) {
			ACAPI_Element_GetElemTypeName (elemHead.type, text);
			GS::UniString elemId;
			if (ACAPI_Element_GetElementInfoString (&elemHead.guid, &elemId) == NoError && !elemId.IsEmpty ())
				text += GetUIString (Str_Separator) + elemId;
		}
		infoText.SetText (text);
		return;
	}

	GS::UniString text = GetUIString (Str_SelectedCount) + NumberToString (selection.GetSize ());
	if (selection.GetSize () > PropData::MaxElemsToEvaluate)
		text += GetUIString (Str_LimitedSelection);
	infoText.SetText (text);
}

// =============================================================================
// Seznam
// =============================================================================

void PropertyPalette::UpdateColumns ()
{
	const short width		= propList.GetItemWidth ();
	const short nameWidth	= static_cast<short> (width / 2);

	propList.SetHeaderItemSize (NameColumn, nameWidth);
	propList.SetHeaderItemSize (ValueColumn, static_cast<short> (width - nameWidth));
	propList.SetTabFieldProperties (NameColumn, 0, nameWidth, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
	propList.SetTabFieldProperties (ValueColumn, nameWidth, width, DG::ListBox::Left, DG::ListBox::EndTruncate, false);
}


void PropertyPalette::RebuildList ()
{
	constexpr short MaxListItems = 32000;

	const short scrollPosition = propList.GetScrollPosition ();
	const GS::UniString filter = filterEdit.GetText ().ToLowerCase ();
	const bool filterActive = !filter.IsEmpty ();
	const GS::UniString indent = "      ";

	short itemToSelect = 0;

	propList.DisableDraw ();
	propList.DeleteItem (DG::ListBox::AllItems);
	entries.Clear ();

	for (UInt32 groupIndex = 0; groupIndex < groups.GetSize (); ++groupIndex) {
		const PropData::GroupRow& group = groups[groupIndex];
		const bool groupMatches = filterActive && group.name.ToLowerCase ().Contains (filter);

		GS::Array<UInt32> visibleProps;
		for (UInt32 propIndex = 0; propIndex < group.props.GetSize (); ++propIndex) {
			const PropData::PropRow& row = group.props[propIndex];
			if (!filterActive || groupMatches ||
				row.definition.name.ToLowerCase ().Contains (filter) ||
				row.valueText.ToLowerCase ().Contains (filter))
			{
				visibleProps.Push (propIndex);
			}
		}
		if (visibleProps.IsEmpty ())
			continue;

		if (propList.GetItemCount () >= MaxListItems)
			break;

		const bool collapsed = !filterActive && collapsedGroups.Contains (APIGuid2GSGuid (group.guid));

		propList.AppendItem ();
		short listItem = propList.GetItemCount ();
		propList.SetTabItemText (listItem, NameColumn, GetUIString (collapsed ? Str_Collapsed : Str_Expanded) + group.name);
		propList.SetTabItemText (listItem, ValueColumn, GS::UniString ("(") + NumberToString (visibleProps.GetSize ()) + ")");
		propList.SetItemFontStyle (listItem, DG::Font::Bold);
		ListEntry groupEntry;
		groupEntry.groupIndex = groupIndex;
		entries.Push (groupEntry);

		if (collapsed)
			continue;

		for (UInt32 propIndex : visibleProps) {
			if (propList.GetItemCount () >= MaxListItems)
				break;

			const PropData::PropRow& row = group.props[propIndex];
			propList.AppendItem ();
			listItem = propList.GetItemCount ();
			propList.SetTabItemText (listItem, NameColumn, indent + row.definition.name);
			if (row.mixed) {
				propList.SetTabItemText (listItem, ValueColumn, GetUIString (Str_Mixed));
				propList.SetTabItemFontStyle (listItem, ValueColumn, DG::Font::Italic);
			} else {
				propList.SetTabItemText (listItem, ValueColumn, row.valueText);
			}
			if (PropData::GetEditKind (row) == PropData::EditKind::ReadOnly)
				propList.SetItemColor (listItem, ReadOnlyColor);

			ListEntry propEntry;
			propEntry.groupIndex	= groupIndex;
			propEntry.propIndex		= static_cast<Int32> (propIndex);
			entries.Push (propEntry);

			if (row.definition.guid == selectedDefinitionGuid)
				itemToSelect = listItem;
		}
	}

	if (itemToSelect > 0)
		propList.SelectItem (itemToSelect);
	propList.SetScrollPosition (scrollPosition);
	propList.EnableDraw ();
	propList.Redraw ();

	UpdateEditor ();
}


const PropertyPalette::ListEntry* PropertyPalette::GetEntry (short listItem) const
{
	if (listItem < 1 || static_cast<UInt32> (listItem) > entries.GetSize ())
		return nullptr;
	return &entries[static_cast<UInt32> (listItem - 1)];
}


const PropData::PropRow* PropertyPalette::GetSelectedRow () const
{
	const ListEntry* entry = GetEntry (propList.GetSelectedItem ());
	if (entry == nullptr || entry->propIndex < 0)
		return nullptr;
	return &groups[entry->groupIndex].props[static_cast<UInt32> (entry->propIndex)];
}

// =============================================================================
// Editor hodnoty
// =============================================================================

void PropertyPalette::SetStatus (const GS::UniString& text)
{
	statusText.SetText (text);
}


void PropertyPalette::UpdateEditor ()
{
	const PropData::PropRow* row = GetSelectedRow ();

	valuePopup.Hide ();
	valueEdit.Show ();
	SetStatus (GS::EmptyUniString);

	if (row == nullptr) {
		propNameText.SetText (GS::EmptyUniString);
		valueEdit.SetText (GS::EmptyUniString);
		valueEdit.Disable ();
		applyButton.Disable ();
		defaultButton.Disable ();
		if (!groups.IsEmpty ())
			SetStatus (GetUIString (Str_SelectProperty));
		return;
	}

	propNameText.SetText (row->definition.name);
	const PropData::EditKind kind = PropData::GetEditKind (*row);
	const bool canReset = row->editable && row->definition.definitionType == API_PropertyCustomDefinitionType;

	switch (kind) {
		case PropData::EditKind::Text:
		case PropData::EditKind::Number:
			valueEdit.SetText (row->valueText);
			valueEdit.Enable ();
			applyButton.Enable ();
			break;

		case PropData::EditKind::Bool:
		case PropData::EditKind::Enum:
		{
			valueEdit.Hide ();
			valuePopup.DeleteItem (DG::PopUp::AllItems);

			const bool			hasSingleValue	= !row->mixed && row->hasValue &&
												  row->firstValue.variantStatus == API_VariantStatusNormal;
			const API_Variant&	currentValue	= row->firstValue.singleVariant.variant;
			const GS::Array<API_SingleEnumerationVariant>& enumValues = row->definition.possibleEnumValues;

			// index aktualni hodnoty mezi moznostmi (u vyctu), -1 = nenalezeno
			Int32 currentEnumIndex = -1;
			if (hasSingleValue && kind == PropData::EditKind::Enum) {
				for (UInt32 i = 0; i < enumValues.GetSize () && currentEnumIndex < 0; ++i) {
					if (PropData::VariantEquals (enumValues[i].keyVariant, currentValue))
						currentEnumIndex = static_cast<Int32> (i);
				}
			}
			const bool knownValue = hasSingleValue && (kind == PropData::EditKind::Bool || currentEnumIndex >= 0);

			short itemToSelect = 0;
			if (!knownValue) {
				// prvni polozka "ruzne" / "nezadano" - jejim potvrzenim se nic nezmeni
				AppendPopupItem (valuePopup, GetUIString (row->mixed ? Str_Mixed : Str_NoValue), PopupValue_Mixed);
				itemToSelect = valuePopup.GetItemCount ();
			}

			if (kind == PropData::EditKind::Bool) {
				AppendPopupItem (valuePopup, GetUIString (Str_Yes), PopupValue_True);
				if (knownValue && currentValue.boolValue)
					itemToSelect = valuePopup.GetItemCount ();

				AppendPopupItem (valuePopup, GetUIString (Str_No), PopupValue_False);
				if (knownValue && !currentValue.boolValue)
					itemToSelect = valuePopup.GetItemCount ();
			} else {
				for (UInt32 i = 0; i < enumValues.GetSize (); ++i) {
					AppendPopupItem (valuePopup, PropData::VariantToString (enumValues[i].displayVariant), static_cast<DGUserData> (i) + 1);
					if (knownValue && static_cast<Int32> (i) == currentEnumIndex)
						itemToSelect = valuePopup.GetItemCount ();
				}
			}

			if (itemToSelect > 0)
				valuePopup.SelectItem (itemToSelect);
			valuePopup.Show ();
			valuePopup.Enable ();
			applyButton.Enable ();
			break;
		}

		case PropData::EditKind::ListReadOnly:
		case PropData::EditKind::TooLong:
		case PropData::EditKind::ReadOnly:
		default:
			valueEdit.SetText (row->valueText);
			valueEdit.Disable ();
			applyButton.Disable ();
			SetStatus (GetUIString (kind == PropData::EditKind::ListReadOnly ? Str_ListReadOnly :
									kind == PropData::EditKind::TooLong ? Str_TooLong : Str_ReadOnly));
			break;
	}

	if (canReset)
		defaultButton.Enable ();
	else
		defaultButton.Disable ();
}


void PropertyPalette::Apply ()
{
	const PropData::PropRow* row = GetSelectedRow ();
	if (row == nullptr || selection.IsEmpty ())
		return;

	const PropData::EditKind		kind		= PropData::GetEditKind (*row);
	const API_PropertyDefinition	definition	= row->definition;
	const GS::Array<API_Guid>		elems		= selection;

	PropData::ApplyResult result;
	bool doApply = true;

	GS::UniString	text;
	DGUserData		popupValue = PopupValue_Mixed;

	switch (kind) {
		case PropData::EditKind::Text:
		case PropData::EditKind::Number:
			text = valueEdit.GetText ();
			break;
		case PropData::EditKind::Bool:
		case PropData::EditKind::Enum:
			popupValue = valuePopup.GetItemValue (valuePopup.GetSelectedItem ());
			if (popupValue == PopupValue_Mixed)
				doApply = false;
			break;
		default:
			doApply = false;
			break;
	}

	if (!doApply)
		return;

	isApplying = true;
	ACAPI_CallUndoableCommand (GetUIString (Str_UndoName), [&] () -> GSErrCode {
		switch (kind) {
			case PropData::EditKind::Text:
			case PropData::EditKind::Number:
				result = PropData::ApplyText (elems, definition, text);
				break;
			case PropData::EditKind::Bool:
				result = PropData::ApplyBool (elems, definition.guid, popupValue == PopupValue_True);
				break;
			case PropData::EditKind::Enum:
			{
				const UInt32 enumIndex = static_cast<UInt32> (popupValue - 1);
				if (enumIndex < definition.possibleEnumValues.GetSize ())
					result = PropData::ApplyEnumKey (elems, definition.guid, definition.possibleEnumValues[enumIndex].keyVariant);
				break;
			}
			default:
				break;
		}
		return NoError;
	});
	isApplying = false;

	RefreshData ();

	if (result.invalidValue) {
		SetStatus (GetUIString (Str_InvalidValue));
		return;
	}

	GS::UniString message = GetUIString (Str_AppliedTo) + NumberToString (result.succeeded);
	if (result.failed > 0)
		message += GS::UniString ("\n") + GetUIString (Str_FailedFor) + NumberToString (result.failed);
	SetStatus (message);
}


void PropertyPalette::ResetSelectedToDefault ()
{
	const PropData::PropRow* row = GetSelectedRow ();
	if (row == nullptr || selection.IsEmpty ())
		return;

	const API_Guid				definitionGuid	= row->definition.guid;
	const GS::Array<API_Guid>	elems			= selection;
	PropData::ApplyResult		result;

	isApplying = true;
	ACAPI_CallUndoableCommand (GetUIString (Str_UndoName), [&] () -> GSErrCode {
		result = PropData::ResetToDefault (elems, definitionGuid);
		return NoError;
	});
	isApplying = false;

	RefreshData ();

	GS::UniString message = GetUIString (Str_ResetFor) + NumberToString (result.succeeded);
	if (result.failed > 0)
		message += GS::UniString ("\n") + GetUIString (Str_FailedFor) + NumberToString (result.failed);
	SetStatus (message);
}

// =============================================================================
// Udalosti palety
// =============================================================================

void PropertyPalette::PanelResized (const DG::PanelResizeEvent& ev)
{
	const short h = ev.GetHorizontalChange ();
	const short v = ev.GetVerticalChange ();

	BeginMoveResizeItems ();
	infoText.Resize (h, 0);
	filterEdit.Resize (h, 0);
	builtInCheck.Move (h, 0);
	propList.Resize (h, v);
	propNameText.MoveAndResize (0, v, h, 0);
	valueEdit.MoveAndResize (0, v, h, 0);
	valuePopup.MoveAndResize (0, v, h, 0);
	applyButton.Move (h, v);
	defaultButton.Move (h, v);
	statusText.MoveAndResize (0, v, h, 0);
	EndMoveResizeItems ();

	UpdateColumns ();
}


void PropertyPalette::PanelIdle (const DG::PanelIdleEvent&)
{
	if (pendingDataRefresh && !isApplying)
		RefreshData ();
}


void PropertyPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
	Hide ();
	*accepted = true;
}


void PropertyPalette::PanelHotkeyPressed (const DG::PanelHotKeyEvent& ev, bool* processed)
{
	*processed = false;
	const short keyId = ev.GetKeyId ();
	if (keyId != returnHotKey && keyId != enterHotKey)
		return;

	if (valueEdit.IsFocused () && valueEdit.IsEnabled ()) {
		Apply ();
		*processed = true;
	} else if (filterEdit.IsFocused ()) {
		propList.SetFocus ();
		*processed = true;
	}
}


void PropertyPalette::ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev)
{
	if (ev.GetSource () != &propList)
		return;

	const PropData::PropRow* row = GetSelectedRow ();
	selectedDefinitionGuid = (row != nullptr) ? row->definition.guid : APINULLGuid;
	UpdateEditor ();
}


void PropertyPalette::ListBoxClicked (const DG::ListBoxClickEvent& ev)
{
	if (ev.GetSource () != &propList)
		return;

	const ListEntry* entry = GetEntry (ev.GetListItem ());
	if (entry == nullptr || entry->propIndex >= 0 || !filterEdit.GetText ().IsEmpty ())
		return;

	const GS::Guid groupGuid = APIGuid2GSGuid (groups[entry->groupIndex].guid);
	if (collapsedGroups.Contains (groupGuid))
		collapsedGroups.Delete (groupGuid);
	else
		collapsedGroups.Add (groupGuid);

	RebuildList ();
}


void PropertyPalette::ListBoxDoubleClicked (const DG::ListBoxDoubleClickEvent& ev)
{
	if (ev.GetSource () != &propList)
		return;

	if (valueEdit.IsVisible () && valueEdit.IsEnabled ()) {
		valueEdit.SetFocus ();
		valueEdit.SelectAll ();
	}
}


void PropertyPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	if (ev.GetSource () == &applyButton)
		Apply ();
	else if (ev.GetSource () == &defaultButton)
		ResetSelectedToDefault ();
}


void PropertyPalette::TextEditChanged (const DG::TextEditChangeEvent& ev)
{
	if (ev.GetSource () == &filterEdit)
		RebuildList ();
}


void PropertyPalette::CheckItemChanged (const DG::CheckItemChangeEvent& ev)
{
	if (ev.GetSource () == &builtInCheck)
		RefreshData ();
}

// =============================================================================
// Notifikace z Archicadu
// =============================================================================

GSErrCode PropertyPalette::SelectionChangeHandler (const API_Neig*)
{
	if (HasInstance ())
		GetInstance ().RefreshFromSelection ();
	return NoError;
}


GSErrCode PropertyPalette::ElementEventHandler (const API_NotifyElementType* elemType)
{
	if (!HasInstance () || elemType == nullptr)
		return NoError;

	PropertyPalette& palette = GetInstance ();
	if (palette.isApplying || !palette.IsElementInSelection (elemType->elemHead.guid))
		return NoError;

	switch (elemType->notifID) {
		case APINotifyElement_Change:
		case APINotifyElement_Edit:
		case APINotifyElement_Undo_Modified:
		case APINotifyElement_Redo_Modified:
		case APINotifyElement_PropertyValueChange:
		case APINotifyElement_ClassificationChange:
			// obnovi se az v klidu (PanelIdle), aby hromadne zmeny nevolaly obnovu pro kazdy prvek
			palette.pendingDataRefresh = true;
			break;
		default:
			break;
	}
	return NoError;
}


GSErrCode PropertyPalette::ProjectEventHandler (API_NotifyEventID notifID, Int32)
{
	if (!HasInstance ())
		return NoError;

	switch (notifID) {
		case APINotify_Quit:
			DestroyInstance ();
			break;
		case APINotify_Close:
		case APINotify_New:
		case APINotify_NewAndReset:
		case APINotify_Open:
		{
			PropertyPalette& palette = GetInstance ();
			palette.observedElements.Clear ();
			palette.selection.Clear ();
			palette.groups.Clear ();
			palette.collapsedGroups.Clear ();
			palette.selectedDefinitionGuid = APINULLGuid;
			palette.RefreshFromSelection ();
			break;
		}
		default:
			break;
	}
	return NoError;
}


GSErrCode PropertyPalette::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
	switch (messageID) {
		case APIPalMsg_OpenPalette:
			if (!HasInstance ())
				CreateInstance ();
			GetInstance ().Show ();
			break;

		case APIPalMsg_ClosePalette:
			if (HasInstance ())
				GetInstance ().Hide ();
			break;

		case APIPalMsg_HidePalette_Begin:
			if (HasInstance () && GetInstance ().IsVisible ())
				GetInstance ().DG::Palette::Hide ();
			break;

		case APIPalMsg_HidePalette_End:
			if (HasInstance () && !GetInstance ().IsVisible ())
				GetInstance ().DG::Palette::Show ();
			break;

		case APIPalMsg_DisableItems_Begin:
			if (HasInstance () && GetInstance ().IsVisible ())
				GetInstance ().DisableItems ();
			break;

		case APIPalMsg_DisableItems_End:
			if (HasInstance () && GetInstance ().IsVisible ())
				GetInstance ().EnableItems ();
			break;

		case APIPalMsg_IsPaletteVisible:
			*(reinterpret_cast<bool*> (param)) = HasInstance () && GetInstance ().IsVisible ();
			break;

		default:
			break;
	}

	return NoError;
}


GSErrCode PropertyPalette::RegisterPaletteControlCallBack ()
{
	return ACAPI_RegisterModelessWindow (
		GS::CalculateHashValue (PaletteGuid),
		PaletteControlCallBack,
		API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
		API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
		API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
		GSGuid2APIGuid (PaletteGuid));
}
