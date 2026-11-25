// *****************************************************************************
// PaletaVlastnosti - cteni a zapis vlastnosti vybranych prvku
// *****************************************************************************

#ifndef PROPERTYDATA_HPP
#define PROPERTYDATA_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"

namespace PropData {

// Pocet prvku vyberu, ze kterych se vlastnosti nacitaji (kvuli rychlosti).
// Zapis hodnoty se provadi vzdy u vsech vybranych prvku.
constexpr UInt32 MaxElemsToEvaluate = 200;

// Maximalni delka textu, ktery lze upravit v palete (limit editacniho pole).
constexpr UInt32 MaxEditableTextLength = 1000;

struct PropRow {
	API_PropertyDefinition	definition;
	GS::UniString			valueText;			// zobrazovana hodnota (u ruznych hodnot prazdna)
	bool					mixed		= false;	// vybrane prvky maji ruzne hodnoty
	bool					editable	= false;	// lze upravit u vsech nactenych prvku
	bool					hasValue	= false;	// prvni prvek ma vyhodnocenou hodnotu
	API_PropertyValue		firstValue;				// hodnota u prvniho prvku

	// Kategorie prvku (Funkce konstrukce, Umisteni, Stav pri rekonstrukci, Zobrazeni s filtrem rekonstrukce)
	// - nejsou to vlastnosti, meni se pres API kategorii. Undefined = bezna vlastnost.
	API_ElemCategoryID				categoryID		= API_ElemCategory_Undefined;
	API_ElemCategory				category		{};
	GS::Array<API_ElemCategoryValue>	categoryValues;		// nabidka voleb
	Int32							categoryIndex	= -1;	// spolecna hodnota (-1 = ruzne)
};

struct GroupRow {
	API_Guid			guid	= APINULLGuid;
	GS::UniString		name;
	bool				builtIn	= false;
	GS::Array<PropRow>	props;
};

enum class EditKind {
	ReadOnly,
	ListReadOnly,
	TooLong,
	Text,
	Number,
	Bool,
	Enum,
	Category
};

struct ApplyResult {
	UInt32	succeeded		= 0;
	UInt32	failed			= 0;
	bool	invalidValue	= false;
};

GS::Array<GroupRow>	Collect (const GS::Array<API_Guid>& elems, bool includeBuiltIn);

EditKind			GetEditKind (const PropRow& row);

GS::UniString		VariantToString (const API_Variant& variant);
bool				VariantEquals (const API_Variant& a, const API_Variant& b);

ApplyResult			ApplyText		(const GS::Array<API_Guid>& elems, const API_PropertyDefinition& definition, const GS::UniString& text);
ApplyResult			ApplyBool		(const GS::Array<API_Guid>& elems, const API_Guid& definitionGuid, bool value);
ApplyResult			ApplyEnumKey	(const GS::Array<API_Guid>& elems, const API_Guid& definitionGuid, const API_Variant& keyVariant);
ApplyResult			ApplyCategory	(const GS::Array<API_Guid>& elems, const API_ElemCategory& category, const API_ElemCategoryValue& value);
ApplyResult			ResetToDefault	(const GS::Array<API_Guid>& elems, const API_Guid& definitionGuid);

GS::UniString		GetUIString (Int32 index);

// Guid skupiny "Kategorie a rekonstrukce" (neni ve Spravci vlastnosti)
extern const API_Guid CategoryGroupGuid;

}

#endif // PROPERTYDATA_HPP
