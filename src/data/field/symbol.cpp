//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2017 Twan van Laarhoven and Sean Hunt             |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <data/field/symbol.hpp>
#include <render/symbol/filter.hpp>

// ----------------------------------------------------------------------------- : SymbolField

IMPLEMENT_FIELD_TYPE(Symbol, "symbol");

IMPLEMENT_REFLECTION(SymbolField) {
  REFLECT_BASE(Field);
}


// ----------------------------------------------------------------------------- : SymbolStyle

IMPLEMENT_REFLECTION(SymbolStyle) {
  REFLECT_BASE(Style);
  REFLECT(min_aspect_ratio);
  REFLECT(max_aspect_ratio);
  REFLECT(variations);
}

SymbolVariation::SymbolVariation()
  : border_radius(0.05)
{}
SymbolVariation::~SymbolVariation() {}

bool SymbolVariation::operator == (const SymbolVariation& that) const {
  return name          == that.name
      && border_radius == that.border_radius
      && *filter       == *that.filter;
}

IMPLEMENT_REFLECTION_NO_SCRIPT(SymbolVariation) {
  REFLECT(name);
  REFLECT(border_radius);
  REFLECT_NAMELESS(filter);
}

// ----------------------------------------------------------------------------- : SymbolValue

String SymbolValue::toString() const {
  return filename.empty() ? wxEmptyString : _("<symbol>");
}

IMPLEMENT_REFLECTION_NAMELESS(SymbolValue) {
  if (fieldP->save_value || tag.scripting() || tag.reading()) REFLECT_NAMELESS(filename);
}
