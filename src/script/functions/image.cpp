//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <script/functions/functions.hpp>
#include <script/functions/util.hpp>
#include <script/image.hpp>
// used by the functions
#include <data/set.hpp>
#include <data/stylesheet.hpp>
#include <data/symbol.hpp>
#include <data/field/symbol.hpp>
#include <render/symbol/filter.hpp>
#include <gui/util.hpp> // load_resource_image

#include <gfx/generated_image.hpp>

DECLARE_TYPEOF_COLLECTION(SymbolVariationP);

bool parse_enum(const String&, ImageCombine& out);

// ----------------------------------------------------------------------------- : Macros

#define SCRIPT_IMAGE_FUNCTION(name)			\
		SCRIPT_FUNCTION(name) {				\
			if (last_update_age() == 0)
#define SCRIPT_IMAGE_FUNCTION_UP_TO_DATE	}

template <> inline ScriptImageP from_script<ScriptImageP>(const ScriptValueP& value) {
	return to_script_image(value);
}

#define SCRIPT_IMAGE_PARAM_UP_TO_DATE(name) script_image_up_to_date(ctx.getVariable(_(#name)))

// ----------------------------------------------------------------------------- : Utility

// TODO : use this system

template <> inline GeneratedImageP from_script<GeneratedImageP>(const ScriptValueP& value) {
	return image_from_script(value);
}

// ----------------------------------------------------------------------------- : Image functions

SCRIPT_FUNCTION(linear_blend2) {
	SCRIPT_PARAM(GeneratedImageP, image1);
	SCRIPT_PARAM(GeneratedImageP, image2);
	SCRIPT_PARAM(double, x1); SCRIPT_PARAM(double, y1);
	SCRIPT_PARAM(double, x2); SCRIPT_PARAM(double, y2);
	return new_intrusive6<LinearBlendImage>(image1, image2, x1,y1, x2,y2);
}

SCRIPT_FUNCTION(masked_blend2) {
	SCRIPT_PARAM(GeneratedImageP, light);
	SCRIPT_PARAM(GeneratedImageP, dark);
	SCRIPT_PARAM(GeneratedImageP, mask);
	return new_intrusive3<MaskedBlendImage>(light, dark, mask);
}

SCRIPT_FUNCTION(combine_blend2) {
	SCRIPT_PARAM(String, combine);
	SCRIPT_PARAM(GeneratedImageP, image1);
	SCRIPT_PARAM(GeneratedImageP, image2);
	ImageCombine image_combine;
	if (!parse_enum(combine, image_combine)) {
		throw ScriptError(_("Not a valid combine mode: '") + combine + _("'"));
	}
	return new_intrusive3<CombineBlendImage>(image1, image2, image_combine);
}

SCRIPT_FUNCTION(set_mask2) {
	SCRIPT_PARAM(GeneratedImageP, image);
	SCRIPT_PARAM(GeneratedImageP, mask);
	return new_intrusive2<SetMaskImage>(image, mask);
}

SCRIPT_FUNCTION(set_combine2) {
	SCRIPT_PARAM(String, combine);
	SCRIPT_PARAM(GeneratedImageP, input);
	ImageCombine image_combine;
	if (!parse_enum(combine, image_combine)) {
		throw ScriptError(_("Not a valid combine mode: '") + combine + _("'"));
	}
	return new_intrusive2<SetCombineImage>(input, image_combine);
}

SCRIPT_FUNCTION(symbol_variation2) {
	// find symbol
	SCRIPT_PARAM(ValueP, symbol);
	SymbolValueP value = dynamic_pointer_cast<SymbolValue>(symbol);
	SCRIPT_PARAM(String, variation);
	// find style
	SCRIPT_PARAM(Set*, set);
	SCRIPT_OPTIONAL_PARAM_(CardP, card);
	SymbolStyleP style = dynamic_pointer_cast<SymbolStyle>(set->stylesheetFor(card)->styleFor(value->fieldP));
	if (!style) throw InternalError(_("Symbol value has a style of the wrong type"));
	// find variation
	FOR_EACH(v, style->variations) {
		if (v->name == variation) {
			// found it
			return new_intrusive3<SymbolToImage>(value->filename, value->last_update, v);
		}
	}
	throw ScriptError(_("Variation of symbol not found ('") + variation + _("')"));
}

SCRIPT_FUNCTION(built_in_image2) {
	SCRIPT_PARAM(String, input);
	return new_intrusive1<BuiltInImage>(input);
}


// ----------------------------------------------------------------------------- : Image functions

SCRIPT_IMAGE_FUNCTION(linear_blend) {
	SCRIPT_PARAM(ScriptImageP, image1);
	SCRIPT_PARAM(ScriptImageP, image2);
	SCRIPT_PARAM(double, x1); SCRIPT_PARAM(double, y1);
	SCRIPT_PARAM(double, x2); SCRIPT_PARAM(double, y2);
	linear_blend(image1->image, image2->image, x1, y1, x2, y2);
	return image1;
SCRIPT_IMAGE_FUNCTION_UP_TO_DATE
	SCRIPT_RETURN(
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(image1) &&
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(image2)
	);
}

SCRIPT_IMAGE_FUNCTION(masked_blend) {
	SCRIPT_PARAM(ScriptImageP, light);
	SCRIPT_PARAM(ScriptImageP, dark);
	SCRIPT_PARAM(ScriptImageP, mask);
	mask_blend(light->image, dark->image, mask->image);
	return light;
SCRIPT_IMAGE_FUNCTION_UP_TO_DATE
	SCRIPT_RETURN(
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(light) &&
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(dark)  &&
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(mask)
	);
}

SCRIPT_IMAGE_FUNCTION(combine_blend) {
	SCRIPT_PARAM(String, combine);
	SCRIPT_PARAM(ScriptImageP, image1);
	SCRIPT_PARAM(ScriptImageP, image2);
	if (!parse_enum(combine, image1->combine)) {
		throw ScriptError(_("Not a valid combine mode: '") + combine + _("'"));
	}
	combine_image(image1->image, image2->image, image1->combine);
	return image1;
SCRIPT_IMAGE_FUNCTION_UP_TO_DATE
	SCRIPT_RETURN(
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(image1) &&
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(image2)
	);
}

SCRIPT_IMAGE_FUNCTION(set_mask) {
	SCRIPT_PARAM(ScriptImageP, image);
	SCRIPT_PARAM(ScriptImageP, mask);
	set_alpha(image->image, mask->image);
	return image;
SCRIPT_IMAGE_FUNCTION_UP_TO_DATE
	SCRIPT_RETURN(
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(image) &&
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(mask)
	);
}

SCRIPT_IMAGE_FUNCTION(set_combine) {
	SCRIPT_PARAM(String, combine);
	SCRIPT_PARAM(ScriptImageP, input);
	// parse and set combine
	if (!parse_enum(combine, input->combine)) {
		throw ScriptError(_("Not a valid combine mode: '") + combine + _("'"));
	}
	return input;
SCRIPT_IMAGE_FUNCTION_UP_TO_DATE
	SCRIPT_RETURN(
		SCRIPT_IMAGE_PARAM_UP_TO_DATE(input)
	);
}

SCRIPT_IMAGE_FUNCTION(symbol_variation) {
	SCRIPT_PARAM(ValueP, symbol);
	SymbolValueP value = dynamic_pointer_cast<SymbolValue>(symbol);
	SCRIPT_PARAM(String, variation);
	// find set & style
	SCRIPT_PARAM(Set*, set);
	SCRIPT_OPTIONAL_PARAM_(CardP, card);
	SymbolStyleP style = dynamic_pointer_cast<SymbolStyle>(set->stylesheetFor(card)->styleFor(value->fieldP));
	if (!style) throw InternalError(_("Symbol value has a style of the wrong type"));
	// load symbol
	SymbolP the_symbol;
	if (value->filename.empty()) {
		the_symbol = default_symbol();
	} else {
		the_symbol = set->readFile<SymbolP>(value->filename);
	}
	// determine filter & render
	FOR_EACH(v, style->variations) {
		if (v->name == variation) {
			// render & filter
			return new_intrusive1<ScriptImage>(render_symbol(the_symbol, *v->filter, v->border_radius));
		}
	}
	throw ScriptError(_("Variation of symbol not found ('") + variation + _("')"));
SCRIPT_IMAGE_FUNCTION_UP_TO_DATE
//	SCRIPT_RETURN(last_update_age() >= value->filename.last_update_age);
	SCRIPT_RETURN(last_update_age() > 1); // the symbol was created/loaded after program start,
	                                      // don't use cached images
}

SCRIPT_IMAGE_FUNCTION(buildin_image) {
	SCRIPT_PARAM(String, input);
	Image img = load_resource_image(input);
	if (!img.Ok()) {
		throw ScriptError(_("There is no build in image '") + input + _("'"));
	}
	return new_intrusive1<ScriptImage>(img);
SCRIPT_IMAGE_FUNCTION_UP_TO_DATE
	SCRIPT_RETURN(true); // always up to date
}

// ----------------------------------------------------------------------------- : Init

void init_script_image_functions(Context& ctx) {
	ctx.setVariable(_("linear blend"),     script_linear_blend);
	ctx.setVariable(_("masked blend"),     script_masked_blend);
	ctx.setVariable(_("combine blend"),    script_combine_blend);
	ctx.setVariable(_("set mask"),         script_set_mask);
	ctx.setVariable(_("set combine"),      script_set_combine);
	ctx.setVariable(_("symbol variation"), script_symbol_variation);
	ctx.setVariable(_("buildin image"),    script_buildin_image);
}
