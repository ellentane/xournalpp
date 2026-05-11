#include "BaseExportJob.h"

#include <utility>  // for move

#include <glib.h>  // for g_warning

#include "control/Control.h"                      // for Control
#include "control/jobs/BlockingJob.h"             // for BlockingJob
#include "control/settings/Settings.h"            // for Settings
#include "gui/MainWindow.h"                       // for MainWindow
#include "gui/dialog/FileChooserFiltersHelper.h"  // for addFilterByExtension
#include "gui/dialog/XojSaveDlg.h"
#include "model/Document.h"           // for Document, Document::PDF
#include "util/PathUtil.h"            // for clearExtensions
#include "util/PopupWindowWrapper.h"  // for PopupWindowWrapper
#include "util/XojMsgBox.h"           // for XojMsgBox
#include "util/glib_casts.h"          // for wrap_for_g_callback_v
#include "util/i18n.h"                // for _, FS, _F

BaseExportJob::BaseExportJob(Control* control, const std::string& name): BlockingJob(control, name) {}

BaseExportJob::~BaseExportJob() = default;

void BaseExportJob::addFileFilterToDialog(GtkFileChooser* dialog, const std::string& name,
                                          const std::string& extension) {
    // Route through the shared native-compatible helper. Using gtk_file_filter_add_mime_type()
    // here would make GtkFileChooserNative fall back to the GTK dialog on Windows.
    xoj::addFilterByExtension(dialog, name.c_str(), {extension.c_str()});
}

auto BaseExportJob::checkOverwriteBackgroundPDF(fs::path const& file) const -> bool {
    auto backgroundPDF = control->getDocument()->getPdfFilepath();
    try {
        if (backgroundPDF.empty() || !fs::exists(backgroundPDF)) {
            // If there is no background, we can return
            return true;
        }

        if (fs::weakly_canonical(file) == fs::weakly_canonical(backgroundPDF)) {
            // If the new file name (with the selected extension) is the previously selected pdf, warn the user
            std::string msg = _("Do not overwrite the background PDF! This will cause errors!");
            XojMsgBox::showErrorToUser(control->getGtkWindow(), msg);
            return false;
        }
    } catch (const fs::filesystem_error& fe) {
        g_warning("%s", fe.what());
        auto msg = std::string(_("The check for overwriting the background failed with:\n")) + fe.what();
        XojMsgBox::showErrorToUser(control->getGtkWindow(), msg);
        return false;
    }
    return true;
}

void BaseExportJob::showFileChooser(std::function<void()> onFileSelected, std::function<void()> onCancel) {
    Settings* settings = control->getSettings();
    Document* doc = control->getDocument();
    doc->lock_shared();
    fs::path suggestedPath = doc->createSaveFoldername(settings->getLastSavePath());
    suggestedPath /=
            doc->createSaveFilename(Document::PDF, settings->getDefaultSaveName(), settings->getDefaultPdfExportName());
    doc->unlock_shared();

    auto pathValidation = [job = this](fs::path& p, const char* filterName) {
        job->setExtensionFromFilter(p, filterName);
        return job->testAndSetFilepath(p);
    };

    auto configure = [job = this](GtkFileChooser* fc) { job->addFilterToDialog(fc); };

    auto callback = [onFileSelected = std::move(onFileSelected),
                     onCancel = std::move(onCancel)](std::optional<fs::path> p) {
        if (p && !p->empty()) {
            onFileSelected();
        } else {
            onCancel();
        }
    };

    xoj::dlg::showSaveDialog(GTK_WINDOW(this->control->getWindow()->getWindow()), control->getSettings(),
                             std::move(suggestedPath), _("Export File"), std::move(configure),
                             std::move(pathValidation), std::move(callback));
}

auto BaseExportJob::testAndSetFilepath(const fs::path& file) -> bool {
    try {
        if (!file.empty() && fs::is_directory(file.parent_path()) && checkOverwriteBackgroundPDF(file)) {
            this->filepath = file;
            return true;
        }
    } catch (const fs::filesystem_error& e) {
        std::string msg = FS(_F("Failed to resolve path with the following error:\n{1}") % e.what());
        XojMsgBox::showErrorToUser(control->getGtkWindow(), msg);
    }
    return false;
}

void BaseExportJob::afterRun() {
    if (!this->errorMsg.empty()) {
        XojMsgBox::showErrorToUser(control->getGtkWindow(), this->errorMsg);
    }
}
