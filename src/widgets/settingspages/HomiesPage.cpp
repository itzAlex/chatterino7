#include "HomiesPage.hpp"

#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#include "singletons/Settings.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"
#include "widgets/settingspages/SettingWidget.hpp"

#include <QDesktopServices>
#include <QFileDialog>

namespace chatterino {

HomiesPage::HomiesPage()
{
    auto y = new QVBoxLayout;
    auto x = new QHBoxLayout;
    auto view = GeneralPageView::withNavigation(this);
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

    layout.addTitle("Apperance");
    SettingWidget::checkbox("Gray-out historical messages",
                            s.grayOutRecents)
        ->addTo(layout);

    layout.addTitle("Behaviour");
    layout.addDropdown("Search Engine",
                       {"Google", "Bing", "DuckDuckGo", "Qwant", "Startpage",
                        "Yahoo", "Yandex", "Ecosia", "Baidu", "Ask", "Aol"},
                       s.searchEngine);
    SettingWidget::checkbox("Enable Homies global emotes auto-completation",
                            s.enableHomiesCompletion)
        ->addTo(layout);
    SettingWidget::checkbox("Enable 7TV global emotes auto-completation",
                            s.enable7TVCompletion)
        ->addTo(layout);
    SettingWidget::checkbox("Enable BTTV global emotes auto-completation",
                            s.enableBTTVCompletion)
        ->addTo(layout);
    SettingWidget::checkbox("Enable FFZ global emotes auto-completation",
                            s.enableFFZCompletion)
        ->addTo(layout);
    SettingWidget::checkbox("Mention users with an at sign (@User)",
                            s.mentionUsersWithAt)
        ->addTo(layout);

    layout.addStretch();
    auto inv = new BaseWidget(this);
    layout.addWidget(inv);
}

}  // namespace chatterino
