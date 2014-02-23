#pragma once

#include <string>
#include <wx/textctrl.h>
#include "event/SingleIdleCallback.h"

class wxWindow;

namespace wxutil
{

class ConsoleView :
	public wxTextCtrl,
	private SingleIdleCallback
{
public:
	// The text modes determining the colour
	enum TextMode
	{
		ModeStandard,
		ModeWarning,
		ModeError,
	};

private:
	wxTextAttr _errorAttr;
	wxTextAttr _warningAttr;
	wxTextAttr _standardAttr;

	TextMode _bufferMode;
	std::string _buffer;

public:
	ConsoleView(wxWindow* parent);

	// Appends new text to the end of the buffer
	void appendText(const std::string& text, TextMode mode);

protected:
	void onIdle();
};

}

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textview.h>
#include <gtkmm/texttag.h>
#include <gtkmm/textmark.h>

namespace gtkutil
{

/**
 * greebo: A console view provides a scrolled textview with a backend
 * GtkTextBuffer plus some convenient methods to write additional text
 * to that buffer. The ConsoleView will take care that the new text is
 * always "on screen".
 *
 * There are three "modes" available for writing text: STD, WARNING, ERROR
 */
class ConsoleView :
	public Gtk::ScrolledWindow
{
private:
	GtkWidget* _scrolledFrame;

	Gtk::TextView* _textView;
	Glib::RefPtr<Gtk::TextBuffer> _buffer;

	// The tags for colouring the output text
	Glib::RefPtr<Gtk::TextBuffer::Tag> _errorTag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> _warningTag;
	Glib::RefPtr<Gtk::TextBuffer::Tag> _standardTag;

	Glib::RefPtr<Gtk::TextMark> _end;

	Glib::Mutex _mutex;

public:
	ConsoleView();

	virtual ~ConsoleView() {}

	// The text modes determining the colour
	enum ETextMode
	{
		MSTANDARD,
		MWARNING,
		MERROR,
	};

	// Appends new text to the end of the buffer
	void appendText(const std::string& text, ETextMode mode);

	// Clears the text buffer
	void clear();

private:
	// Static GTK callbacks
	void onClearConsole();
	void onPopulatePopup(Gtk::Menu* menu);
};

} // namespace gtkutil
