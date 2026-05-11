#include "NativeFileChooserHelper.h"

#include <utility>

namespace xoj::dlg {

namespace {

struct NativeFileChooserSession {
    xoj::util::GObjectSPtr<GtkFileChooserNative> dialog;
    xoj::util::move_only_function<void(GtkFileChooser*, gint)> onResponse;
    gulong signalId = 0;
};

void onResponseTrampoline(GtkNativeDialog* nativeDialog, gint response, gpointer userData) {
    auto* session = static_cast<NativeFileChooserSession*>(userData);

    // Detach the handler first so destroying the dialog below cannot re-enter this callback.
    g_signal_handler_disconnect(nativeDialog, session->signalId);

    // Move state to locals: the callback may take a long time (e.g. show another dialog), and must
    // not observe `session` being destroyed while it runs.
    auto dialog = std::move(session->dialog);
    auto onResponse = std::move(session->onResponse);
    delete session;

    onResponse(GTK_FILE_CHOOSER(dialog.get()), response);

    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(dialog.get()));
}

}  // namespace

void showNativeFileChooser(xoj::util::GObjectSPtr<GtkFileChooserNative> dialog, GtkWindow* parent,
                           xoj::util::move_only_function<void(GtkFileChooser*, gint)> onResponse) {
    auto* nativeDialog = GTK_NATIVE_DIALOG(dialog.get());
    gtk_native_dialog_set_transient_for(nativeDialog, parent);
    gtk_native_dialog_set_modal(nativeDialog, TRUE);

    auto* session = new NativeFileChooserSession{std::move(dialog), std::move(onResponse), 0};
    session->signalId = g_signal_connect(nativeDialog, "response", G_CALLBACK(onResponseTrampoline), session);

    gtk_native_dialog_show(nativeDialog);
}

}  // namespace xoj::dlg
