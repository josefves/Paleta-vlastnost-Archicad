// *****************************************************************************
// PaletaVlastnosti - hlavni soubor doplnku
// Paleta "Vlastnosti prvku" v menu Okno > Paletky
// *****************************************************************************

#include "APIEnvir.h"
#include "ACAPinc.h"

#include "RS.hpp"

#include "ResourceIds.hpp"
#include "PropertyPalette.hpp"

// -----------------------------------------------------------------------------
// Zobrazeni / skryti palety z menu
// -----------------------------------------------------------------------------

static void ShowOrHidePalette ()
{
	if (PropertyPalette::HasInstance () && PropertyPalette::GetInstance ().IsVisible ()) {
		PropertyPalette::GetInstance ().Hide ();
	} else {
		if (!PropertyPalette::HasInstance ())
			PropertyPalette::CreateInstance ();
		PropertyPalette::GetInstance ().Show ();
	}
}


static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
	if (menuParams->menuItemRef.menuResID == PropertyPalette::MenuResId &&
		menuParams->menuItemRef.itemIndex == PropertyPalette::MenuItemIndex)
	{
		ShowOrHidePalette ();
	}
	return NoError;
}

// =============================================================================
// Povinne funkce doplnku
// =============================================================================

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, 2, ACAPI_GetOwnResModule ());

	// Preload: doplnek zustava nacteny, aby si Archicad pamatoval stav palety
	// a aby paleta reagovala na zmenu vyberu.
	return APIAddon_Preload;
}


GSErrCode RegisterInterface (void)
{
	return ACAPI_MenuItem_RegisterMenu (ID_PALETTE_MENU, 0, MenuCode_Palettes, MenuFlag_Default);
}


GSErrCode Initialize (void)
{
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (ID_PALETTE_MENU, MenuCommandHandler);
	if (err != NoError)
		return err;

	err = ACAPI_Notification_CatchSelectionChange (PropertyPalette::SelectionChangeHandler);
	if (err != NoError)
		return err;

	err = ACAPI_Element_InstallElementObserver (PropertyPalette::ElementEventHandler);
	if (err != NoError)
		return err;

	return PropertyPalette::RegisterPaletteControlCallBack ();
}


GSErrCode FreeData (void)
{
	return NoError;
}
