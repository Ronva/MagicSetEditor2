//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include "reader.hpp"
#include <util/vector2d.hpp>
#include <util/error.hpp>
#include <util/io/package_manager.hpp>

// ----------------------------------------------------------------------------- : Reader

Reader::Reader(const InputStreamP& input, const String& filename, bool ignore_invalid)
	: indent(0), expected_indent(0), state(OUTSIDE)
	, filename(filename), line_number(0), previous_line_number(0)
	, ignore_invalid(ignore_invalid)
	, input(input)
{
	moveNext();
	handleAppVersion();
}

Reader::Reader(const String& filename)
	: indent(0), expected_indent(0), state(OUTSIDE)
	, filename(filename), line_number(0), previous_line_number(0)
	, input(packages.openFileFromPackage(filename))
{
	moveNext();
	handleAppVersion();
}

void Reader::addAlias(Version end_version, const Char* a, const Char* b) {
	Alias& alias = aliasses[a];
	alias.new_key     = b;
	alias.end_version = end_version;
}

void Reader::handleAppVersion() {
	if (enterBlock(_("mse_version"))) {
		handle(file_app_version);
		if (app_version < file_app_version) {
			handle_warning(_ERROR_2_("newer version", filename, file_app_version.toString()), false);
		}
		exitBlock();
	}
}

void Reader::warning(const String& msg, int line_number_delta, bool warn_on_previous_line) {
	warnings += String(_("\nOn line "))
	         << ((warn_on_previous_line ? previous_line_number : line_number) + line_number_delta)
	         << _(": \t") << msg;
}

void Reader::showWarnings() {
	if (!warnings.empty()) {
		handle_warning(_("Warnings while reading file:\n") + filename + _("\n") + warnings, false);
		warnings.clear();
	}
}

bool Reader::enterAnyBlock() {
	if (state == ENTERED) moveNext(); // on the key of the parent block, first move inside it
	if (indent != expected_indent) return false; // not enough indentation
	state = ENTERED;
	expected_indent += 1; // the indent inside the block must be at least this much
	return true;
}

bool Reader::enterBlock(const Char* name) {
	if (state == ENTERED) moveNext(); // on the key of the parent block, first move inside it
	if (indent != expected_indent) return false; // not enough indentation
	if (cannocial_name_compare(key, name)) {
		state = ENTERED;
		expected_indent += 1; // the indent inside the block must be at least this much
		return true;
	} else {
		return false;
	}
}

void Reader::exitBlock() {
	assert(expected_indent > 0);
	expected_indent -= 1;
	assert(state != UNHANDLED);
	previous_value.clear();
	if (state == ENTERED) moveNext(); // leave this key
	// Dump the remainder of the block
	// TODO: issue warnings?
	while (indent > expected_indent) {
		moveNext();
	}
	state = HANDLED;
}

void Reader::moveNext() {
	previous_line_number = line_number;
	state = HANDLED;
	key.clear();
	indent = -1; // if no line is read it never has the expected indentation
	// repeat until we have a good line
	while (key.empty() && !input->Eof()) {
		readLine();
	}
	// did we reach the end of the file?
	if (key.empty() && input->Eof()) {
		line_number += 1;
		indent = -1;
	}
}

/// Read an UTF-8 encoded line from an input stream
/** As opposed to wx functions, this one actually reports errors
 */
String read_utf8_line(wxInputStream& input, bool eat_bom = true, bool until_eof = false);
String read_utf8_line(wxInputStream& input, bool eat_bom, bool until_eof) {
	vector<char> buffer;
	while (!input.Eof()) {
		Byte c = input.GetC(); if (input.LastRead() <= 0) break;
		if (!until_eof) {
			if (c == '\n') break;
			if (c == '\r') {
				if (input.Eof()) break;
				c = input.GetC(); if (input.LastRead() <= 0) break;
				if (c != '\n') {
					input.Ungetch(c); // \r but not \r\n
				}
				break; 
			}
		}
		buffer.push_back(c);
	}
	// convert to string
	buffer.push_back('\0');
	size_t size = wxConvUTF8.MB2WC(nullptr, &buffer[0], 0);
	if (size == -1) {
		throw ParseError(_("Invalid UTF-8 sequence"));
	} else if (size == 0) {
		return _("");
	}
	String result;
	#ifdef UNICODE
		// NOTE: wx doc is wrong, parameter to GetWritableChar is numer of characters, not bytes
		Char* result_buf = result.GetWriteBuf(size + 1);
		wxConvUTF8.MB2WC(result_buf, &buffer[0], size + 1);
		result.UngetWriteBuf(size);
	#else
		result = String(&buffer[0], *wxConvCurrent);
	#endif
	return eat_bom ? decodeUTF8BOM(result) : result;
}

void Reader::readLine(bool in_string) {
	line_number += 1;
	// We have to do our own line reading, because wxTextInputStream is insane
	try {
		line = read_utf8_line(*input, line_number == 1);
	} catch (const ParseError& e) {
		throw ParseError(e.what() + String(_(" on line ")) << line_number);
	}
	// read indentation
	indent = 0;
	while ((UInt)indent < line.size() && line.GetChar(indent) == _('\t')) {
		indent += 1;
	}
	// read key / value
	size_t pos = line.find_first_of(_(':'), indent);
	if (trim(line).empty() || line.GetChar(indent) == _('#')) {
		// empty line or comment
		key.clear();
		return;
	}
	key   = line.substr(indent, pos - indent);
	if (!ignore_invalid && !in_string && starts_with(key, _(" "))) {
		warning(_("key: '") + key + _("' starts with a space; only use TABs for indentation!"), 0, false);
		// try to fix up: 8 spaces is a tab
		while (starts_with(key, _("        "))) {
			key = key.substr(8);
			indent += 1;
		}
	}
	key   = cannocial_name_form(trim(key));
	value = pos == String::npos ? _("") : trim_left(line.substr(pos+1));
	if (key.empty() && pos!=String::npos) key = _(" "); // we don't want an empty key if there was a colon
}

void Reader::unknownKey() {
	// ignore?
	if (ignore_invalid) {
		do {
			moveNext();
		} while (indent > expected_indent);
		return;
	}
	// aliasses?
	map<String,Alias>::const_iterator it = aliasses.find(key);
	if (it != aliasses.end()) {
		if (aliasses.find(it->second.new_key) != aliasses.end()) {
			// alias points to another alias, don't follow it, there is the risk of infinite loops
		} else if (it->second.end_version <= file_app_version) {
			// alias not used for this version, use in warning
			if (indent == expected_indent) {
				warning(_("Unexpected key: '") + key + _("'  use  '") + it->second.new_key + _("'"), 0, false);
				do {
					moveNext();
				} while (indent > expected_indent);
				return;
			}
		} else {
			// try this key instead
			key = it->second.new_key;
			return;
		}
	}
	if (indent >= expected_indent) {
		warning(_("Unexpected key: '") + key + _("'"), 0, false);
		do {
			moveNext();
		} while (indent > expected_indent);
	}
	// else: could be a nameless value, which doesn't call exitBlock to move past its own key
}

// ----------------------------------------------------------------------------- : Handling basic types

void Reader::unhandle() {
	assert(state == HANDLED);
	state = UNHANDLED;
}

const String& Reader::getValue() {
	assert(state != HANDLED); // don't try to handle things twice
	if (state == UNHANDLED) {
		state = HANDLED;
		return previous_value;
	} else if (value.empty()) {
		// a multiline string
		previous_value.clear();
		bool first = true;
		// read all lines that are indented enough
		readLine();
		previous_line_number = line_number;
		while (indent >= expected_indent && !input->Eof()) {
			if (!first) previous_value += _('\n');
			first = false;
			previous_value += line.substr(expected_indent); // strip expected indent
			readLine(true);
		}
		// moveNext(), but without the initial readLine()
		state = HANDLED;
		while (key.empty() && !input->Eof()) {
			readLine();
		}
		// did we reach the end of the file?
		if (key.empty() && input->Eof()) {
			line_number += 1;
			indent = -1;
		}
		if (indent >= expected_indent) {
			warning(_("Blank line or comment in text block, that is insufficiently indented.\n")
			        _("\t\tEither indent the comment/blank line, or add a 'key:' after it.\n")
			        _("\t\tThis could cause more more error messages.\n"), -1, false);
		}
		return previous_value;
	} else {
		previous_value = value;
		moveNext();
		return previous_value;
	}
}

template <> void Reader::handle(String& s) {
	s = getValue();
}
template <> void Reader::handle(int& i) {
	long l = 0;
	getValue().ToLong(&l);
	i = l;
}
template <> void Reader::handle(unsigned int& i) {
	long l = 0;
	getValue().ToLong(&l);
	i = abs(l); // abs, because it will seem strange if -1 comes out as MAX_INT
}
template <> void Reader::handle(double& d) {
	getValue().ToDouble(&d);
}
template <> void Reader::handle(bool& b) {
	const String& v = getValue();
	b = (v==_("true") || v==_("1") || v==_("yes"));
}
// ----------------------------------------------------------------------------- : Handling less basic util types

template <> void Reader::handle(Vector2D& vec) {
	if (!wxSscanf(getValue().c_str(), _("(%lf,%lf)"), &vec.x, &vec.y)) {
		throw ParseError(_("Expected (x,y)"));
	}
}

template <> void Reader::handle(Color& col) {
	UInt r,g,b;
	if (wxSscanf(getValue().c_str(),_("rgb(%u,%u,%u)"),&r,&g,&b)) {
		col.Set(r, g, b);
	}
}

template <> void Reader::handle(FileName& f) {
	if (clipboard_package()) {
		String str = getValue();
		if (!str.empty()) {
			// copy file into current package
			try {
				String packaged_name = clipboard_package()->newFileName(_("image"),_("")); // a new unique name in the package, assume it's an image
				OutputStreamP out    = clipboard_package()->openOut(packaged_name);
				InputStreamP  in     = Package::openAbsoluteFile(str);
				out->Write(*in); // copy
				f.assign(packaged_name);
			} catch (Error) {
				// ignore errors
			}
		} else {
			f.assign(str);
		}
	} else {
		handle(static_cast<String&>(f));
	}
}
