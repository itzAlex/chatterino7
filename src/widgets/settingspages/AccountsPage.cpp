#include "AccountsPage.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/accounts/AccountModel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/LoginDialog.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <typeinfo>
#include <QLabel>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>
#include <algorithm>

namespace chatterino {

AccountsPage::AccountsPage()
{
    auto *app = getApp();
    LayoutCreator<AccountsPage> layoutCreator(this);

    auto tabs = layoutCreator.emplace<QTabWidget>();

    auto accounts_tab = tabs.appendTab(new QVBoxLayout, "Accounts");
    {
        EditableModelView *view =
                accounts_tab
                        .emplace<EditableModelView>(app->accounts->createModel(nullptr),
                                                    false)
                        .getElement();

        view->getTableView()->horizontalHeader()->setVisible(false);
        view->getTableView()->horizontalHeader()->setStretchLastSection(true);

        view->addButtonPressed.connect([this] {
            static auto loginWidget = new LoginWidget(this);

            loginWidget->show();
            loginWidget->raise();
        });

        view->getTableView()->setStyleSheet("background: #333");
    }

    auto accountsSettings = tabs.appendTab(new QVBoxLayout, "Accounts settings");
    {
        auto anyways = accountsSettings.emplace<QHBoxLayout>().withoutMargin();
        {
            anyways.emplace<QLabel>("Select username:");
            auto combo = anyways.emplace<QComboBox>().getElement();

            for (const auto &userName : app->accounts->twitch.getUsernames())
            {
                combo->addItem(userName);
            }

            app->accounts->twitch.userListUpdated.connect([=]() {
                combo->blockSignals(true);

                combo->clear();

                for (const auto &userName : app->accounts->twitch.getUsernames())
                {
                    combo->addItem(userName);
                }

                combo->blockSignals(false);
            });

            anyways->addStretch(1);
            anyways->setAlignment(Qt::AlignTop);
        }

        auto hashes_form = accountsSettings.emplace<QFormLayout>().withoutMargin();
        {
            hashes_form->addRow("Follow hash", &this->followHashInput);
            hashes_form->addRow("Unfollow hash", &this->unfollowHashInput);
        }

        clearFieldsButton = new QPushButton();
        {
            clearFieldsButton->setText("Clear fields");
        }

        accountsSettingsButton = new QPushButton();
        {
            accountsSettingsButton->setText("Save settings");
        }

        refreshButtons();

        accountsSettings->addWidget(clearFieldsButton);
        accountsSettings->addWidget(accountsSettingsButton);

        connect(&followHashInput, &QLineEdit::textChanged, [=]() {
            refreshButtons();
        });

        connect(&unfollowHashInput, &QLineEdit::textChanged, [=]() {
            refreshButtons();
        });

        connect(clearFieldsButton, &QPushButton::clicked, [=]() {
            this->unfollowHashInput.clear();
            this->followHashInput.clear();
        });

        connect(accountsSettingsButton, &QPushButton::clicked, [=]() {

        });
    }
}

void AccountsPage::refreshButtons()
{
    if (this->unfollowHashInput.text().isEmpty() ||
        this->followHashInput.text().isEmpty())
    {
        accountsSettingsButton->setEnabled(false);
    } else
    {
        accountsSettingsButton->setEnabled(true);
    }
}
} // namespace chatterino
