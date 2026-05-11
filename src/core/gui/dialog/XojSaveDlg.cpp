#include "XojSaveDlg.h"

#include <utility>

#include "control/settings/Settings.h"
#include "util/PathUtil.h"  // for fromGFile, toGFile, clearExtensions
#include "util/XojMsgBox.h"
#include "util/gtk4_helper.h"       // for gtk_file_chooser_set_current_folder
#include "util/i18n.h"              // for _
#include "util/raii/GObjectSPtr.h"  // for GObjectSPtr

#include "FileChooserFiltersHelper.h"
#include "NativeFileChooserHelper.h"

namespace {

xoj::util::GObjectSPtr<GtkFileChooserNative> makeSaveDialog(Settings* settings, const fs::path& suggestedPath,
                                                            const char* title) {
    xoj::util::GObjectSPtr<GtkFileChooserNative> dialog(
            gtk_file_chooser_native_new(title, nullptr, GTK_FILE_CHOOSER_ACTION_SAVE, nullptr, nullptr),
            xoj::util::adopt);

    auto* fc = GTK_FILE_CHOOSER(dialog.get());

    // Let the native dialog show its own "file exists" prompt when it can. The post-dialog
    // replaceFileQuestion() still runs because pathValidation may append an extension that
    // turns the chosen name into a pre-existing file, which the OS dialog cannot foresee.
    gtk_file_chooser_set_do_overwrite_confirmation(fc, TRUE);

    if (!suggestedPath.empty()) {
        gtk_file_chooser_set_current_folder(fc, Util::toGFile(suggestedPath.parent_path()).get(), nullptr);
        gtk_file_chooser_set_current_name(fc, Util::toGFilename(suggestedPath.filename()).c_str());
    }
    if (settings) {
#if !defined(__APPLE__)
        // Shortcut folders are honoured by the Win32 native backend (mapped to IFileDialog
        // "places") but the macOS native panel does not support them at all, so skip the
        // call there to avoid touching features that could still surprise the backend.
        if (!settings->getLastOpenPath().empty()) {
            gtk_file_chooser_add_shortcut_folder(fc, Util::toGFile(settings->getLastOpenPath()).get(), nullptr);
        }
#endif
    }

    return dialog;
}

bool xoppPathValidation(fs::path& p, const char* /*filterName*/) {
    Util::clearExtensions(p);
    p += ".xopp";
    return true;
}

}  // namespace

void xoj::dlg::showSaveDialog(GtkWindow* parent, Settings* settings, fs::path suggestedPath, const char* title,
                              SaveConfigurator configure, SavePathValidator pathValidation, SavePathCallback callback) {
    auto dialog = makeSaveDialog(settings, suggestedPath, title);
    if (configure) {
        configure(GTK_FILE_CHOOSER(dialog.get()));
    }

    showNativeFileChooser(
            std::move(dialog), parent,
            [parent, settings, pathValidation = std::move(pathValidation), callback = std::move(callback)](
                    GtkFileChooser* fc, gint response) mutable {
                if (response != GTK_RESPONSE_ACCEPT) {
                    callback(std::nullopt);
                    return;
                }

                auto file = Util::fromGFile(
                        xoj::util::GObjectSPtr<GFile>(gtk_file_chooser_get_file(fc), xoj::util::adopt).get());
                const char* filterName = nullptr;
                if (GtkFileFilter* filter = gtk_file_chooser_get_filter(fc)) {
                    filterName = gtk_file_filter_get_name(filter);
                }

                if (!pathValidation(file, filterName)) {
                    // Validation rejected the path (e.g. writing over the background PDF).
                    callback(std::nullopt);
                    return;
                }

                if (settings && !file.empty()) {
                    settings->setLastSavePath(file.parent_path());
                }

                // Post-dialog overwrite confirmation. Also covers the case where
                // pathValidation appended an extension that turns the target into an
                // already-existing file, which the native dialog could not anticipate.
                auto onReplace = [callback](const fs::path& confirmed) { callback(confirmed); };
                auto onDecline = [callback = std::move(callback)]() { callback(std::nullopt); };
                XojMsgBox::replaceFileQuestion(parent, std::move(file), std::move(onReplace), std::move(onDecline));
            });
}

void xoj::dlg::showXoppSaveDialog(GtkWindow* parent, Settings* settings, fs::path suggestedPath,
                                  SavePathCallback callback) {
    showSaveDialog(
            parent, settings, std::move(suggestedPath), _("Save File"),
            [](GtkFileChooser* fc) { xoj::addFilterXopp(fc); }, xoppPathValidation, std::move(callback));
}
