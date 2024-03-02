#include "HomiesPage.hpp"

#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#include "singletons/Settings.hpp"
#include "widgets/dialogs/SelectChannelSeparateLinksDialog.hpp"
#include "widgets/dialogs/TokenDialog.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"

#include <QDesktopServices>
#include <QFileDialog>

namespace chatterino {

HomiesPage::HomiesPage()
{
    auto y = new QVBoxLayout;
    auto x = new QHBoxLayout;
    auto view = new GeneralPageView;
    this->view_ = view;
    x->addWidget(view);
    auto z = new QFrame;
    z->setLayout(x);
    y->addWidget(z);
    this->setLayout(y);
    this->initLayout(*view);
}

bool HomiesPage::filterElements(const QString &query)
{
    if (this->view_)
        return this->view_->filterElements(query) || query.isEmpty();
    else
        return false;
}

void HomiesPage::initLayout(GeneralPageView &layout)
{
    auto &s = *getSettings();

    QPushButton *sevenTVTokenButton = new QPushButton();
    {
        sevenTVTokenButton->setText("7TV Authentication Token");
        sevenTVTokenButton->adjustSize();
    }

    connect(sevenTVTokenButton, &QPushButton::clicked, [=]() {
        auto SevenTVTokenWidget = new TokenDialog(0);

        SevenTVTokenWidget->show();
        SevenTVTokenWidget->raise();
    });

    QPushButton *BTTVTokenButton = new QPushButton();
    {
        BTTVTokenButton->setText("BetterTTV Authentication Token");
        BTTVTokenButton->adjustSize();
    }

    connect(BTTVTokenButton, &QPushButton::clicked, [=]() {
        auto BTTVTokenWidget = new TokenDialog(1);

        BTTVTokenWidget->show();
        BTTVTokenWidget->raise();
    });

    layout.addTitle("Providers tokens");

    layout.addWidget(sevenTVTokenButton);
    layout.addWidget(BTTVTokenButton);

    layout.addTitle("Apperance");
    layout.addCheckbox("Gray-out historical messages", s.grayOutRecents);

    layout.addTitle("Behaviour");
    layout.addDropdown("Search Engine",
                       {"Google", "Bing", "DuckDuckGo", "Qwant", "Startpage",
                        "Yahoo", "Yandex", "Ecosia", "Baidu", "Ask", "Aol"},
                       s.searchEngine);
    layout.addCheckbox("Mention users with an at sign (@User)",
                       s.mentionUsersWithAt);
    layout.addCheckbox(
        "Automatically join separated links (http<s>:/ / → http<s>://)",
        s.joinSeparatedLinks);
    layout.addCheckbox("Separate links of clips",
                       s.separateClipsLinks);
    layout.addCheckbox(
        "Automatically separate links (http<s>:// → http<s>:/ /)",
        s.separateLinks);

    QPushButton *selectChannelsSeparateLinksButton = new QPushButton();
    {
        selectChannelsSeparateLinksButton->setText("Select channels");
        selectChannelsSeparateLinksButton->adjustSize();
    }

    s.separateLinks.connect(
        [selectChannelsSeparateLinksButton](const bool &value, auto) {
            if (value == 1)
                selectChannelsSeparateLinksButton->setEnabled(true);
            else
                selectChannelsSeparateLinksButton->setEnabled(false);
        });

    connect(selectChannelsSeparateLinksButton, &QPushButton::clicked, [=]() {
        auto selectChannelsSeparateLinksWidget =
            new SelectChannelSeparateLinksDialog();

        selectChannelsSeparateLinksWidget->show();
        selectChannelsSeparateLinksWidget->raise();
    });

    layout.addWidget(selectChannelsSeparateLinksButton);

    layout.addStretch();
    auto inv = new BaseWidget(this);
    layout.addWidget(inv);
}

}  // namespace chatterino