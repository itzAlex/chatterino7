#include "TokenDialog.hpp"

#ifdef USEWINSDK
#    include <Windows.h>
#endif

#include "Application.hpp"
#include "singletons/Settings.hpp"

#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStandardItem>
#include <QString>
#include <QTableView>

namespace chatterino {

    TokenDialog::TokenDialog(int provider, QWidget *parent)
            : QDialog(parent)
    {
#ifdef USEWINSDK
        ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif

        // Layout properties
        this->setModal(true);
        this->setMinimumWidth(250);
        this->adjustSize();
        if (provider == 0)
            this->setWindowTitle("7TV Token");
        if (provider == 1)
            this->setWindowTitle("BetterTTV Token");
        this->setLayout(&this->ui_.mainLayout);
        this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        this->setWindowFlags(
                (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
                Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        this->setWindowFlag(Qt::WindowStaysOnTopHint, true);

        // Text field
        auto *tokenTextField = new QLineEdit;
        tokenTextField->setPlaceholderText("Paste here the token");
        tokenTextField->setEchoMode(QLineEdit::Password);
        tokenTextField->setFocus();
        this->ui_.mainLayout.addWidget(tokenTextField);

        // OK and Cancel buttons & Checkbox
        auto buttonBox =
                new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        buttonBox->setCenterButtons(true);
        this->ui_.mainLayout.addWidget(buttonBox);

        // Signals
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
            std::string path = "";
            QString token = tokenTextField->text();

            if (provider == 0)
                path = "/SevenTVToken";
            if (provider == 1)
                path = "/BTTVToken";

            pajlada::Settings::Setting<QString>::set(path, token);

            this->accept();
            this->close();
        });

        QObject::connect(buttonBox, &QDialogButtonBox::rejected, [this]() {
            this->reject();
            this->close();
        });
    }
}  // namespace chatterino