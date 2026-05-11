/*
 * Xournal++
 *
 * Native save/export file chooser helpers.
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <functional>
#include <optional>

#include <gtk/gtk.h>  // for GtkWindow

#include "util/move_only_function.h"

#include "filesystem.h"  // for path

class Settings;

namespace xoj::dlg {

/// Predicate used to post-process and validate the path returned by the save dialog.
/// Implementations may modify @p path (for example, appending a file extension based on the chosen
/// filter) and return whether the result is acceptable.
using SavePathValidator = std::function<bool(fs::path& path, const char* filterName)>;

/// Invoked after the save dialog closes. The argument holds the chosen path on success, or
/// std::nullopt if the user cancelled or closed the dialog.
using SavePathCallback = std::function<void(std::optional<fs::path>)>;

/// Optional hook run on the dialog before it is shown. Lets callers add filters, shortcuts, or
/// other chooser-specific configuration in one place.
using SaveConfigurator = xoj::util::move_only_function<void(GtkFileChooser*)>;

/**
 * Show a native save/export file chooser.
 *
 * The dialog uses @p suggestedPath to seed the current folder and filename, honours the optional
 * @p configure hook, and prompts the user about overwriting an existing file before invoking
 * @p callback. The overwrite prompt also catches the case where @p pathValidation appends an
 * extension that changes the target path after the native dialog's own confirmation.
 *
 * @p callback is invoked with std::nullopt on cancel/close.
 */
void showSaveDialog(GtkWindow* parent, Settings* settings, fs::path suggestedPath, const char* title,
                    SaveConfigurator configure, SavePathValidator pathValidation, SavePathCallback callback);

/// Convenience wrapper for the main ".xopp" save flow.
void showXoppSaveDialog(GtkWindow* parent, Settings* settings, fs::path suggestedPath, SavePathCallback callback);

}  // namespace xoj::dlg
