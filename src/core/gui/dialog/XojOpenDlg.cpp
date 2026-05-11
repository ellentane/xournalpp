#include "XojOpenDlg.h"

#include <cstring>  // for strcmp
#include <utility>

#include "control/settings/Settings.h"  // for Settings
#include "util/PathUtil.h"              // for fromGFile, toGFile
#include "util/Util.h"                  // for execInUiThread
#include "util/gtk4_helper.h"           // for gtk_file_chooser_set_current_folder
#include "util/i18n.h"                  // for _
#include "util/raii/GObjectSPtr.h"      // for GObjectSPtr

#include "FileChooserFiltersHelper.h"
#include "NativeFileChooserHelper.h"

namespace {

void addlastSavePathShortcut(GtkFileChooser* fc, Settings* settings) {
    auto lastSavePath = settings->getLastSavePath();
    if (!lastSavePath.empty()) {
#if GTK_MAJOR_VERSION == 3
        gtk_file_chooser_add_shortcut_folder(fc, char_cast(lastSavePath.u8string().c_str()), nullptr);
#else
        gtk_file_chooser_add_shortcut_folder(fc, Util::toGFile(lastSavePath).get(), nullptr);
#endif
    }
}

void setCurrentFolderToLastOpenPath(GtkFileChooser* fc, Settings* settings) {
    xoj::util::GObjectSPtr<GFile> currentFolder;
    if (settings && !settings->getLastOpenPath().empty()) {
        currentFolder = Util::toGFile(settings->getLastOpenPath());
    } else {
        currentFolder.reset(g_file_new_for_path(g_get_home_dir()), xoj::util::adopt);
    }
    gtk_file_chooser_set_current_folder(fc, currentFolder.get(), nullptr);
}

template <class... Args>
std::function<void(fs::path, Args...)> addSetLastSavePathToCallback(std::function<void(fs::path, Args...)> callback,
                                                                    Settings* settings) {
    return [cb = std::move(callback), settings](fs::path path, Args... args) {
        if (settings && !path.empty()) {
            settings->setLastOpenPath(path.parent_path());
        }
        cb(std::move(path), std::forward<Args>(args)...);
    };
}

constexpr auto ATTACH_CHOICE_ID = "attachPdfChoice";

void addAttachChoice(GtkFileChooser* fc) {
    // GtkFileChooserNative ignores add_choice on the Win32 and macOS backends; on those
    // platforms the attach option is not shown and the default ("false") is used.
    gtk_file_chooser_add_choice(fc, ATTACH_CHOICE_ID, _("Attach file to the journal"), nullptr, nullptr);
    gtk_file_chooser_set_choice(fc, ATTACH_CHOICE_ID, "false");
}

xoj::util::GObjectSPtr<GtkFileChooserNative> makeOpenDialog(const char* title) {
    return {gtk_file_chooser_native_new(title, nullptr, GTK_FILE_CHOOSER_ACTION_OPEN, nullptr, nullptr),
            xoj::util::adopt};
}

/// Build a response handler that extracts the selected path and the attach-choice, then forwards
/// them to @p callback. The callback is posted via Util::execInUiThread so that any follow-up
/// dialog is spawned after the native chooser is fully destroyed.
auto makeOpenHandler(std::function<void(fs::path, bool)> callback)
        -> xoj::util::move_only_function<void(GtkFileChooser*, gint)> {
    return [cb = std::move(callback)](GtkFileChooser* fc, gint response) mutable {
        if (response != GTK_RESPONSE_ACCEPT) {
            return;
        }
        auto path =
                Util::fromGFile(xoj::util::GObjectSPtr<GFile>(gtk_file_chooser_get_file(fc), xoj::util::adopt).get());

        bool attach = false;
        if (const char* choice = gtk_file_chooser_get_choice(fc, ATTACH_CHOICE_ID)) {
            attach = std::strcmp(choice, "true") == 0;
        }

        Util::execInUiThread(
                [cb = std::move(cb), path = std::move(path), attach]() mutable { cb(std::move(path), attach); });
    };
}

}  // namespace

void xoj::OpenDlg::showOpenTemplateDialog(GtkWindow* parent, Settings* settings,
                                          std::function<void(fs::path)> callback) {
    auto dialog = makeOpenDialog(_("Open template file"));
    auto* fc = GTK_FILE_CHOOSER(dialog.get());

    xoj::addFilterAllFiles(fc);
    xoj::addFilterXopt(fc);
    setCurrentFolderToLastOpenPath(fc, settings);

    auto wrapped = addSetLastSavePathToCallback(std::move(callback), settings);
    xoj::dlg::showNativeFileChooser(
            std::move(dialog), parent,
            makeOpenHandler([cb = std::move(wrapped)](fs::path path, bool) mutable { cb(std::move(path)); }));
}

void xoj::OpenDlg::showOpenFileDialog(GtkWindow* parent, Settings* settings, std::function<void(fs::path)> callback) {
    auto dialog = makeOpenDialog(_("Open file"));
    auto* fc = GTK_FILE_CHOOSER(dialog.get());

    xoj::addFilterSupported(fc);
    xoj::addFilterXoj(fc);
    xoj::addFilterXopt(fc);
    xoj::addFilterXopp(fc);
    xoj::addFilterPdf(fc);
    xoj::addFilterAllFiles(fc);

    addlastSavePathShortcut(fc, settings);
    setCurrentFolderToLastOpenPath(fc, settings);

    auto wrapped = addSetLastSavePathToCallback(std::move(callback), settings);
    xoj::dlg::showNativeFileChooser(
            std::move(dialog), parent,
            makeOpenHandler([cb = std::move(wrapped)](fs::path path, bool) mutable { cb(std::move(path)); }));
}

void xoj::OpenDlg::showAnnotatePdfDialog(GtkWindow* parent, Settings* settings,
                                         std::function<void(fs::path, bool)> callback) {
    auto dialog = makeOpenDialog(_("Annotate Pdf file"));
    auto* fc = GTK_FILE_CHOOSER(dialog.get());

    xoj::addFilterPdf(fc);
    xoj::addFilterAllFiles(fc);

    addlastSavePathShortcut(fc, settings);
    setCurrentFolderToLastOpenPath(fc, settings);

    addAttachChoice(fc);

    auto wrapped = addSetLastSavePathToCallback(std::move(callback), settings);
    xoj::dlg::showNativeFileChooser(std::move(dialog), parent, makeOpenHandler(std::move(wrapped)));
}

void xoj::OpenDlg::showOpenImageDialog(GtkWindow* parent, Settings* settings,
                                       std::function<void(fs::path, bool)> callback) {
    auto dialog = makeOpenDialog(_("Choose image file"));
    auto* fc = GTK_FILE_CHOOSER(dialog.get());

    xoj::addFilterImages(fc);
    xoj::addFilterAllFiles(fc);

    if (!settings->getLastImagePath().empty()) {
        gtk_file_chooser_set_current_folder(fc, Util::toGFile(settings->getLastImagePath()).get(), nullptr);
    }

    addAttachChoice(fc);

    auto rememberImagePath = [settings, cb = std::move(callback)](fs::path path, bool attach) mutable {
        if (settings && !path.empty()) {
            if (auto folder = path.parent_path(); !folder.empty()) {
                settings->setLastImagePath(folder);
            }
        }
        cb(std::move(path), attach);
    };
    xoj::dlg::showNativeFileChooser(std::move(dialog), parent, makeOpenHandler(std::move(rememberImagePath)));
}

void xoj::OpenDlg::showMultiFormatDialog(GtkWindow* parent, std::vector<std::string> formats,
                                         std::function<void(fs::path)> callback) {
    auto dialog = makeOpenDialog(_("Open file"));
    auto* fc = GTK_FILE_CHOOSER(dialog.get());

    if (formats.size() > 0) {
        GtkFileFilter* filterSupported = gtk_file_filter_new();
        gtk_file_filter_set_name(filterSupported, _("Supported files"));
        for (std::string format: formats) {
            gtk_file_filter_add_pattern(filterSupported, format.c_str());
        }
        gtk_file_chooser_add_filter(fc, filterSupported);
    }

    xoj::dlg::showNativeFileChooser(
            std::move(dialog), parent,
            makeOpenHandler([cb = std::move(callback)](fs::path path, bool) mutable { cb(std::move(path)); }));
}
