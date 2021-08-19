#include "AccountsPage.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/accounts/AccountModel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/LoginDialog.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
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

    auto accountsSettings =
        tabs.appendTab(new QVBoxLayout, "Accounts settings");
    {
        auto anyways = accountsSettings.emplace<QHBoxLayout>().withoutMargin();
        {
            anyways.emplace<QLabel>("Select username:");
            combo = anyways.emplace<QComboBox>().getElement();

            for (const auto &userName : app->accounts->twitch.getUsernames())
            {
                combo->addItem(userName);
            }

            app->accounts->twitch.userListUpdated.connect([=]() {
                combo->blockSignals(true);

                combo->clear();

                for (const auto &userName :
                     app->accounts->twitch.getUsernames())
                {
                    combo->addItem(userName);
                }

                combo->blockSignals(false);
            });

            anyways->addStretch(1);
            anyways->setAlignment(Qt::AlignTop);
        }

        auto hashes_form =
            accountsSettings.emplace<QFormLayout>().withoutMargin();
        {
            hashes_form->addRow("Follow hash", &this->followHashInput);
            hashes_form->addRow("Unfollow hash", &this->unfollowHashInput);
            hashes_form->addRow("OAuth token", &this->OAuthTokenInput);
        }

        QHBoxLayout *buttons = new QHBoxLayout();

        clearFieldsButton = new QPushButton();
        {
            clearFieldsButton->setText("Clear fields");
        }

        accountsSettingsButton = new QPushButton();
        {
            accountsSettingsButton->setText("Save settings");
        }

        buttons->addWidget(clearFieldsButton);
        buttons->addWidget(accountsSettingsButton);

        refreshButtons();

        accountsSettings->addLayout(buttons);

        connect(&followHashInput, &QLineEdit::textChanged, [=]() {
            refreshButtons();
        });

        connect(&unfollowHashInput, &QLineEdit::textChanged, [=]() {
            refreshButtons();
        });

        connect(&OAuthTokenInput, &QLineEdit::textChanged, [=]() {
            refreshButtons();
        });

        connect(clearFieldsButton, &QPushButton::clicked, [=]() {
            this->unfollowHashInput.clear();
            this->followHashInput.clear();
            this->OAuthTokenInput.clear();
        });

        label = new QLabel;
        accountsSettings->addWidget(label);

        connect(accountsSettingsButton, &QPushButton::clicked, [=]() {
            auto keys =
                pajlada::Settings::SettingManager::getObjectKeys("/accounts");

            for (const auto &uid : keys)
            {
                auto username = pajlada::Settings::Setting<QString>::get(
                    "/accounts/" + uid + "/username");
                auto userID = pajlada::Settings::Setting<QString>::get(
                    "/accounts/" + uid + "/userID");

                if (username == combo->currentText())
                {
                    std::string basePath =
                        "/accounts/uid" + userID.toStdString();

                    pajlada::Settings::Setting<QString>::set(
                        basePath + "/followHash", this->followHashInput.text());
                    pajlada::Settings::Setting<QString>::set(
                        basePath + "/unfollowHash",
                        this->unfollowHashInput.text());
                    pajlada::Settings::Setting<QString>::set(
                        basePath + "/followToken",
                        this->OAuthTokenInput.text());
                }
            }

            this->unfollowHashInput.clear();
            this->followHashInput.clear();
            this->OAuthTokenInput.clear();

            AnimatedSave();
        });
    }
}

void AccountsPage::refreshButtons()
{
    if (this->unfollowHashInput.text().isEmpty() ||
        this->followHashInput.text().isEmpty() ||
        this->OAuthTokenInput.text().isEmpty())
    {
        accountsSettingsButton->setEnabled(false);
    }
    else
    {
        accountsSettingsButton->setEnabled(true);
    }
}

void AccountsPage::AnimatedSave()
{
    QFont f("Arial", 14, QFont::Bold);
    label->setFont(f);
    label->setText("Settings saved correctly!");
    label->setAlignment(Qt::AlignCenter);

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect();
    label->setGraphicsEffect(effect);
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(1800);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::InQuad);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

}  // namespace chatterino
