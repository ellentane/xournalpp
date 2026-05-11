#include "FileChooserFiltersHelper.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "util/i18n.h"

namespace xoj {

namespace {

/**
 * Add a glob for @p extension (with leading dot) to @p filter, plus an uppercase sibling.
 *
 * Rationale for the uppercase duplicate:
 *  - Linux GTK: gtk_file_filter_add_pattern() goes through glib's g_pattern_spec, which
 *    matches case-sensitively. Without the sibling, a file named SCAN.PDF is hidden.
 *  - Win32 native (IFileDialog): file system matching is case-insensitive, so the sibling
 *    is redundant but harmless -- critically, it is still a plain pattern glob, which
 *    keeps the filter translatable to COMDLG_FILTERSPEC and prevents the fallback to the
 *    built-in GtkFileChooserDialog.
 *  - macOS native: also case-insensitive by default; same reasoning applies.
 *
 * Only the extension portion is upper-cased; the leading "*." is locale-invariant and is
 * appended verbatim to keep the pattern syntactically identical.
 */
void addExtensionPattern(GtkFileFilter* filter, const char* extension) {
    std::string lowerPattern = "*";
    lowerPattern += extension;
    gtk_file_filter_add_pattern(filter, lowerPattern.c_str());

    std::string upperExt = extension;
    std::transform(upperExt.begin(), upperExt.end(), upperExt.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (upperExt != extension) {
        std::string upperPattern = "*";
        upperPattern += upperExt;
        gtk_file_filter_add_pattern(filter, upperPattern.c_str());
    }
}

}  // namespace

void addFilterByExtension(GtkFileChooser* fc, const char* name, std::initializer_list<const char*> extensions) {
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, name);
    for (const char* extension: extensions) {
        addExtensionPattern(filter, extension);
    }
    gtk_file_chooser_add_filter(fc, filter);
}

void addFilterAllFiles(GtkFileChooser* fc) {
    GtkFileFilter* filterAll = gtk_file_filter_new();
    gtk_file_filter_set_name(filterAll, _("All files"));
    gtk_file_filter_add_pattern(filterAll, "*");
    gtk_file_chooser_add_filter(fc, filterAll);
}

void addFilterSupported(GtkFileChooser* fc) {
    addFilterByExtension(fc, _("Supported files"),
                         {
                                 ".xopp",
                                 ".xoj",
                                 ".xopt",
                                 ".pdf",
                                 ".moj",  // MrWriter
                         });
}

void addFilterPdf(GtkFileChooser* fc) { addFilterByExtension(fc, _("PDF files"), {".pdf"}); }
void addFilterXoj(GtkFileChooser* fc) { addFilterByExtension(fc, _("Xournal files"), {".xoj"}); }
void addFilterXopp(GtkFileChooser* fc) { addFilterByExtension(fc, _("Xournal++ files"), {".xopp"}); }
void addFilterXopt(GtkFileChooser* fc) { addFilterByExtension(fc, _("Xournal++ template"), {".xopt"}); }
void addFilterSvg(GtkFileChooser* fc) { addFilterByExtension(fc, _("SVG graphics"), {".svg"}); }
void addFilterPng(GtkFileChooser* fc) { addFilterByExtension(fc, _("PNG graphics"), {".png"}); }
void addFilterZip(GtkFileChooser* fc) { addFilterByExtension(fc, _("ZIP archive"), {".zip"}); }

void addFilterImages(GtkFileChooser* fc) {
    // gtk_file_filter_add_pixbuf_formats() would be shorter, but the resulting filter is
    // backed by GTK's pixbuf loader introspection rather than a plain extension glob, so
    // the Win32 native backend cannot translate it and GTK falls back to its own dialog.
    // Hard-coding the extensions keeps the native file picker alive.
    addFilterByExtension(fc, _("Image files"),
                         {
                                 ".png",
                                 ".jpg",
                                 ".jpeg",
                                 ".bmp",
                                 ".gif",
                                 ".tif",
                                 ".tiff",
                                 ".webp",
                                 ".svg",
                                 ".ico",
                         });
}
};  // namespace xoj
