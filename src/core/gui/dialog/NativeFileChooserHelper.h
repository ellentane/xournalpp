/*
 * Xournal++
 *
 * Shared lifecycle helper for asynchronous GtkFileChooserNative dialogs.
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <gtk/gtk.h>

#include "util/move_only_function.h"
#include "util/raii/GObjectSPtr.h"

namespace xoj::dlg {

/**
 * @brief Show a GtkFileChooserNative asynchronously and destroy it once the user responds.
 *
 * Takes ownership of @p dialog, sets it modal and transient for @p parent, connects a response
 * handler and shows it. When the user responds (accept, cancel, or closes the dialog), @p onResponse
 * is invoked exactly once with the dialog's GtkFileChooser view and the GTK response id, then the
 * dialog is destroyed and any internal state is freed.
 *
 * @param dialog     The native chooser to show. The helper takes ownership.
 * @param parent     The parent window for transient_for, or nullptr.
 * @param onResponse Invoked once with the GtkFileChooser* view of @p dialog and the response id.
 *                   Safe to capture state by move; destroyed after it returns.
 */
void showNativeFileChooser(xoj::util::GObjectSPtr<GtkFileChooserNative> dialog, GtkWindow* parent,
                           xoj::util::move_only_function<void(GtkFileChooser*, gint)> onResponse);

}  // namespace xoj::dlg
