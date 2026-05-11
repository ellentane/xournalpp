/*
 * Xournal++
 *
 * Helper functions to add native-compatible filters to GtkFileChoosers
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <initializer_list>

#include <gtk/gtk.h>

namespace xoj {

/**
 * Add a file-extension filter using GtkFileFilter patterns.
 *
 * GtkFileChooserNative's Win32 backend cannot translate MIME or custom filters to the
 * IFileDialog COMDLG_FILTERSPEC structures it uses. When GTK sees an unsupported filter
 * configuration it silently falls back to the internal GtkFileChooserDialog, which is
 * why the app would otherwise still show the GTK-styled picker on Windows.
 *
 * Pattern filters map directly to COMDLG_FILTERSPEC and keep the native backend active.
 * Each @p extensions entry is expected to include the leading dot (e.g. ".xopp").
 */
void addFilterByExtension(GtkFileChooser* fc, const char* name, std::initializer_list<const char*> extensions);

void addFilterAllFiles(GtkFileChooser* fc);
void addFilterSupported(GtkFileChooser* fc);
void addFilterPdf(GtkFileChooser* fc);
void addFilterXoj(GtkFileChooser* fc);
void addFilterXopp(GtkFileChooser* fc);
void addFilterXopt(GtkFileChooser* fc);
void addFilterSvg(GtkFileChooser* fc);
void addFilterPng(GtkFileChooser* fc);
void addFilterZip(GtkFileChooser* fc);
void addFilterImages(GtkFileChooser* fc);  ///< Common image file extensions, pattern-based for native choosers
};  // namespace xoj
