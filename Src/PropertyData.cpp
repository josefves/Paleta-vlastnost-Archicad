// *****************************************************************************
// PaletaVlastnosti - cteni a zapis vlastnosti vybranych prvku
// *****************************************************************************

#include "PropertyData.hpp"
#include "ResourceIds.hpp"

#include "HashSet.hpp"
#include "HashTable.hpp"
#include "RS.hpp"

#include <algorithm>

namespace PropData {

// -----------------------------------------------------------------------------
// Prevod textu na hodnotu vlastnosti v jednotkach z Predvolby projektu > Jednotky vypoctu
// -----------------------------------------------------------------------------

class ConversionUtils : public API_PropertyConversionUtilsInterface {
public:
	ConversionUtils ()
	{
		API_CalcUnitPrefs prefs {};
		if (ACAPI_ProjectSetting_GetPreferences (&prefs, APIPrefs_CalcUnitsID) == NoError) {
			lengthType	= prefs.length.unit;
			areaType	= prefs.area.unit;
			volumeType	= prefs.volume.unit;
			angleType	= prefs.angle.unit;
		}
	}

	virtual const GS::UniString& GetDegreeSymbol1	() const override { return degreeSymbol; }
	virtual const GS::UniString& GetDegreeSymbol2	() const override { return degreeSymbol; }
	virtual const GS::UniString& GetMinuteSymbol	() const override { return minuteSymbol; }
	virtual const GS::UniString& GetSecondSymbol	() const override { return secondSymbol; }
	virtual const GS::UniString& GetGradientSymbol	() const override { return gradientSymbol; }
	virtual const GS::UniString& GetRadianSymbol	() const override { return radianSymbol; }
	virtual const GS::UniString& GetNorthSymbol		() const override { return northSymbol; }
	virtual const GS::UniString& GetSouthSymbol		() const override { return southSymbol; }
	virtual const GS::UniString& GetEastSymbol		() const override { return eastSymbol; }
	virtual const GS::UniString& GetWestSymbol		() const override { return westSymbol; }

	virtual GS::uchar_t					GetDecimalDelimiterChar		() const override { return '.'; }
	virtual GS::Optional<GS::UniChar>	GetThousandSeparatorChar	() const override { return GS::NoValue; }

	virtual API_LengthTypeID	GetLengthType	() const override { return lengthType; }
	virtual API_AreaTypeID		GetAreaType		() const override { return areaType; }
	virtual API_VolumeTypeID	GetVolumeType	() const override { return volumeType; }
	virtual API_AngleTypeID		GetAngleType	() const override { return angleType; }

private:
	const GS::UniString degreeSymbol	= GS::UniString ("\xC2\xB0", CC_UTF8);
	const GS::UniString minuteSymbol	= "'";
	const GS::UniString secondSymbol	= "\"";
	const GS::UniString gradientSymbol	= "G";
	const GS::UniString radianSymbol	= "R";
	const GS::UniString northSymbol		= "N";
	const GS::UniString southSymbol		= "S";
	const GS::UniString eastSymbol		= "E";
	const GS::UniString westSymbol		= "W";

	API_LengthTypeID	lengthType	= API_LengthTypeID::Meter;
	API_AreaTypeID		areaType	= API_AreaTypeID::SquareMeter;
	API_VolumeTypeID	volumeType	= API_VolumeTypeID::CubicMeter;
	API_AngleTypeID		angleType	= API_AngleTypeID::DecimalDegree;
};


GS::UniString GetUIString (Int32 index)
{
	GS::UniString result;
	RSGetIndString (&result, ID_UI_STRINGS, index, ACAPI_GetOwnResModule ());
	return result;
}


GS::UniString VariantToString (const API_Variant& variant)
{
	switch (variant.type) {
		case API_PropertyIntegerValueType:	return GS::UniString::Printf ("%d", static_cast<int> (variant.intValue));
		case API_PropertyRealValueType:		return GS::UniString::Printf ("%g", variant.doubleValue);
		case API_PropertyStringValueType:	return variant.uniStringValue;
		case API_PropertyBooleanValueType:	return GetUIString (variant.boolValue ? 6 : 7);
		case API_PropertyGuidValueType:		return APIGuidToString (variant.guidValue);
		default:							return GS::EmptyUniString;
	}
}


bool VariantEquals (const API_Variant& a, const API_Variant& b)
{
	if (a.type != b.type)
		return false;

	switch (a.type) {
		case API_PropertyIntegerValueType:	return a.intValue == b.intValue;
		case API_PropertyRealValueType:		return a.doubleValue == b.doubleValue;
		case API_PropertyStringValueType:	return a.uniStringValue == b.uniStringValue;
		case API_PropertyBooleanValueType:	return a.boolValue == b.boolValue;
		case API_PropertyGuidValueType:		return a.guidValue == b.guidValue;
		default:							return true;
	}
}


static GS::UniString PropertyToDisplayString (const API_Property& property)
{
	if (property.status != API_Property_HasValue)
		return GS::EmptyUniString;

	GS::UniString result;
	if (ACAPI_Property_GetPropertyValueString (property, &result) == NoError)
		return result;

	if (property.definition.collectionType == API_PropertySingleCollectionType)
		return VariantToString (property.value.singleVariant.variant);

	return GS::EmptyUniString;
}


static bool IsVisibleForElement (const API_Guid& elemGuid, const API_PropertyDefinition& definition)
{
	// Viditelnost podle klasifikace se tyka jen vlastnich (uzivatelskych) vlastnosti.
	if (definition.definitionType != API_PropertyCustomDefinitionType)
		return true;
	return ACAPI_Element_IsPropertyDefinitionVisible (elemGuid, definition.guid);
}


GS::Array<GroupRow> Collect (const GS::Array<API_Guid>& elems, bool includeBuiltIn)
{
	GS::Array<GroupRow> groups;
	if (elems.IsEmpty ())
		return groups;

	const UInt32 elemCount = std::min<UInt32> (elems.GetSize (), MaxElemsToEvaluate);
	const API_PropertyDefinitionFilter filter = includeBuiltIn ? API_PropertyDefinitionFilter_All : API_PropertyDefinitionFilter_UserDefined;

	// 1) Definice vlastnosti spolecne vsem nactenym prvkum
	GS::Array<API_PropertyDefinition> definitions;
	{
		GS::Array<API_PropertyDefinition> firstDefinitions;
		if (ACAPI_Element_GetPropertyDefinitions (elems[0], filter, firstDefinitions) != NoError)
			return groups;

		GS::HashSet<GS::Guid> commonGuids;
		for (const API_PropertyDefinition& definition : firstDefinitions) {
			if (IsVisibleForElement (elems[0], definition))
				commonGuids.Add (APIGuid2GSGuid (definition.guid));
		}

		for (UInt32 i = 1; i < elemCount && !commonGuids.IsEmpty (); ++i) {
			GS::Array<API_PropertyDefinition> otherDefinitions;
			if (ACAPI_Element_GetPropertyDefinitions (elems[i], filter, otherDefinitions) != NoError) {
				commonGuids.Clear ();
				break;
			}
			GS::HashSet<GS::Guid> otherGuids;
			for (const API_PropertyDefinition& definition : otherDefinitions) {
				if (IsVisibleForElement (elems[i], definition))
					otherGuids.Add (APIGuid2GSGuid (definition.guid));
			}
			GS::HashSet<GS::Guid> intersection;
			for (const GS::Guid& guid : commonGuids) {
				if (otherGuids.Contains (guid))
					intersection.Add (guid);
			}
			commonGuids = intersection;
		}

		for (const API_PropertyDefinition& definition : firstDefinitions) {
			if (commonGuids.Contains (APIGuid2GSGuid (definition.guid)))
				definitions.Push (definition);
		}
	}

	if (definitions.IsEmpty ())
		return groups;

	// 2) Hodnoty
	GS::Array<PropRow> rows;
	for (const API_PropertyDefinition& definition : definitions) {
		PropRow row;
		row.definition	= definition;
		row.editable	= true;
		rows.Push (row);
	}

	GS::HashTable<GS::Guid, UInt32> rowIndexByGuid;
	for (UInt32 k = 0; k < rows.GetSize (); ++k)
		rowIndexByGuid.Add (APIGuid2GSGuid (rows[k].definition.guid), k);

	for (UInt32 i = 0; i < elemCount; ++i) {
		GS::Array<API_Property> properties;
		if (ACAPI_Element_GetPropertyValues (elems[i], definitions, properties) != NoError)
			continue;

		for (const API_Property& property : properties) {
			const GS::Guid key = APIGuid2GSGuid (property.definition.guid);
			if (!rowIndexByGuid.ContainsKey (key))
				continue;

			PropRow& row = rows[rowIndexByGuid.Get (key)];
			const GS::UniString valueText = PropertyToDisplayString (property);
			if (i == 0) {
				row.valueText	= valueText;
				row.hasValue	= property.status == API_Property_HasValue;
				row.firstValue	= property.value;
			}
			else if (!row.mixed && valueText != row.valueText)
				row.mixed = true;

			if (row.editable && !ACAPI_Element_IsPropertyDefinitionValueEditable (elems[i], property.definition.guid))
				row.editable = false;
		}
	}

	for (PropRow& row : rows) {
		if (row.mixed)
			row.valueText.Clear ();
	}

	// 3) Seskupeni podle skupin ve Spravci vlastnosti (vlastni skupiny prvni, pak vestavene)
	GS::Array<API_PropertyGroup> allGroups;
	ACAPI_Property_GetPropertyGroups (allGroups);

	GS::HashTable<GS::Guid, UInt32> groupIndexByGuid;
	auto addGroup = [&] (const API_PropertyGroup& group) {
		const GS::Guid key = APIGuid2GSGuid (group.guid);
		if (groupIndexByGuid.ContainsKey (key))
			return;
		GroupRow groupRow;
		groupRow.guid		= group.guid;
		groupRow.name		= group.name;
		groupRow.builtIn	= group.groupType != API_PropertyCustomGroupType;
		groupIndexByGuid.Add (key, groups.GetSize ());
		groups.Push (groupRow);
	};

	GS::HashSet<GS::Guid> usedGroupGuids;
	for (const PropRow& row : rows)
		usedGroupGuids.Add (APIGuid2GSGuid (row.definition.groupGuid));

	for (const API_PropertyGroup& group : allGroups) {
		if (group.groupType == API_PropertyCustomGroupType && usedGroupGuids.Contains (APIGuid2GSGuid (group.guid)))
			addGroup (group);
	}
	for (const API_PropertyGroup& group : allGroups) {
		if (group.groupType != API_PropertyCustomGroupType && usedGroupGuids.Contains (APIGuid2GSGuid (group.guid)))
			addGroup (group);
	}

	for (const PropRow& row : rows) {
		const GS::Guid key = APIGuid2GSGuid (row.definition.groupGuid);
		if (!groupIndexByGuid.ContainsKey (key)) {
			API_PropertyGroup group;
			group.guid = row.definition.groupGuid;
			if (ACAPI_Property_GetPropertyGroup (group) != NoError)
				group.name = "?";
			addGroup (group);
		}
		groups[groupIndexByGuid.Get (key)].props.Push (row);
	}

	return groups;
}


EditKind GetEditKind (const PropRow& row)
{
	if (!row.editable)
		return EditKind::ReadOnly;

	switch (row.definition.collectionType) {
		case API_PropertyListCollectionType:
		case API_PropertyMultipleChoiceEnumerationCollectionType:
			return EditKind::ListReadOnly;

		case API_PropertySingleChoiceEnumerationCollectionType:
			return EditKind::Enum;

		case API_PropertySingleCollectionType:
			switch (row.definition.valueType) {
				case API_PropertyBooleanValueType:
					return EditKind::Bool;
				case API_PropertyStringValueType:
					return row.valueText.GetLength () > MaxEditableTextLength ? EditKind::TooLong : EditKind::Text;
				case API_PropertyIntegerValueType:
				case API_PropertyRealValueType:
					return EditKind::Number;
				default:
					return EditKind::ReadOnly;
			}

		default:
			return EditKind::ReadOnly;
	}
}


// -----------------------------------------------------------------------------
// Zapis
// -----------------------------------------------------------------------------

template <typename ModifyFn>
static ApplyResult ApplyToElements (const GS::Array<API_Guid>& elems, const API_Guid& definitionGuid, ModifyFn modify)
{
	ApplyResult result;
	for (const API_Guid& elemGuid : elems) {
		API_Property property;
		if (ACAPI_Element_GetPropertyValue (elemGuid, definitionGuid, property) != NoError ||
			!ACAPI_Element_IsPropertyDefinitionValueEditable (elemGuid, definitionGuid))
		{
			++result.failed;
			continue;
		}

		if (!modify (property)) {
			result.invalidValue = true;
			return result;
		}

		if (ACAPI_Element_SetProperty (elemGuid, property) == NoError)
			++result.succeeded;
		else
			++result.failed;
	}
	return result;
}


static GS::UniString NormalizeNumberText (const GS::UniString& text)
{
	GS::UniString result;
	for (UIndex i = 0; i < text.GetLength (); ++i) {
		const GS::UniChar ch = text[i];
		if (ch == ' ' || ch == '\t' || ch == GS::UniChar (static_cast<char16_t> (0x00A0)))
			continue;
		result.Append (ch == ',' ? GS::UniChar ('.') : ch);
	}
	return result;
}


static void MarkAsCustomValue (API_Property& property)
{
	property.isDefault				= false;
	property.status					= API_Property_HasValue;
	property.value.variantStatus	= API_VariantStatusNormal;
}


ApplyResult ApplyText (const GS::Array<API_Guid>& elems, const API_PropertyDefinition& definition, const GS::UniString& text)
{
	if (definition.valueType == API_PropertyStringValueType) {
		return ApplyToElements (elems, definition.guid, [&] (API_Property& property) {
			property.value.singleVariant.variant.type			= API_PropertyStringValueType;
			property.value.singleVariant.variant.uniStringValue	= text;
			MarkAsCustomValue (property);
			return true;
		});
	}

	const GS::UniString normalized = NormalizeNumberText (text);
	if (normalized.IsEmpty ()) {
		ApplyResult result;
		result.invalidValue = true;
		return result;
	}

	const ConversionUtils conversionUtils;
	return ApplyToElements (elems, definition.guid, [&] (API_Property& property) {
		if (ACAPI_Property_SetPropertyValueFromString (normalized, conversionUtils, &property) != NoError)
			return false;
		MarkAsCustomValue (property);
		return true;
	});
}


ApplyResult ApplyBool (const GS::Array<API_Guid>& elems, const API_Guid& definitionGuid, bool value)
{
	return ApplyToElements (elems, definitionGuid, [&] (API_Property& property) {
		property.value.singleVariant.variant.type		= API_PropertyBooleanValueType;
		property.value.singleVariant.variant.boolValue	= value;
		MarkAsCustomValue (property);
		return true;
	});
}


ApplyResult ApplyEnumKey (const GS::Array<API_Guid>& elems, const API_Guid& definitionGuid, const API_Variant& keyVariant)
{
	return ApplyToElements (elems, definitionGuid, [&] (API_Property& property) {
		property.value.singleVariant.variant = keyVariant;
		MarkAsCustomValue (property);
		return true;
	});
}


ApplyResult ResetToDefault (const GS::Array<API_Guid>& elems, const API_Guid& definitionGuid)
{
	return ApplyToElements (elems, definitionGuid, [] (API_Property& property) {
		property.isDefault = true;
		return true;
	});
}

}
