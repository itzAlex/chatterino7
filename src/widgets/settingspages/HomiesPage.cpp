#include "HomiesPage.hpp"

#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#include "Application.hpp"
#include "common/Version.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/helper/Line.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"
#include "widgets/dialogs/SelectChannelSeparateLinksDialog.hpp"

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

    layout.addTitle("Emotes");
    layout.addCheckbox("Enable Homies global emotes (requires restart)",
                       s.enableHomiesGlobalEmotes);
    layout.addCheckbox("Enable 7TV global emotes (requires restart)",
                       s.enable7TVGlobalEmotes);
    layout.addCheckbox("Enable BTTV global emotes (requires restart)",
                       s.enableBTTVGlobalEmotes);
    layout.addCheckbox("Enable FFZ global emotes (requires restart)",
                       s.enableFFZGlobalEmotes);
    layout.addTitle("Apperance");
    layout.addCheckbox("Gray-out historical messages", s.grayOutRecents);

    layout.addTitle("Behaviour");
    layout.addCheckbox("Enable Homies global emotes auto-completation",
                       s.enableHomiesCompletion);
    layout.addCheckbox("Enable 7TV global emotes auto-completation",
                       s.enable7TVCompletion);
    layout.addCheckbox("Enable BTTV global emotes auto-completation",
                       s.enableBTTVCompletion);
    layout.addCheckbox("Enable FFZ global emotes auto-completation",
                       s.enableFFZCompletion);
    layout.addCheckbox("Mention users with an at sign (@User)",
                       s.mentionUsersWithAt);
    layout.addCheckbox("Automatically join separated links (http<s>:/ / → http<s>://)",
                       s.joinSeparatedLinks);

    layout.addCheckbox("Automatically separate links (http<s>:// → http<s>:/ /)",
                       s.separateLinks);

    QPushButton *selectChannelsSeparateLinksButton = new QPushButton();
    {
        selectChannelsSeparateLinksButton->setText("Select channels");
        selectChannelsSeparateLinksButton->adjustSize();
    }

    s.separateLinks.connect(
            [selectChannelsSeparateLinksButton](const bool &value, auto) {
                if (value == 1) selectChannelsSeparateLinksButton->setEnabled(true);
                else selectChannelsSeparateLinksButton->setEnabled(false);
    });

    connect(selectChannelsSeparateLinksButton, &QPushButton::clicked, [=]() {
        auto selectChannelsSeparateLinksWidget = new SelectChannelSeparateLinksDialog();

        selectChannelsSeparateLinksWidget->show();
        selectChannelsSeparateLinksWidget->raise();
    });

    layout.addWidget(selectChannelsSeparateLinksButton);

    layout.addStretch();
    auto inv = new BaseWidget(this);
    layout.addWidget(inv);
}

}  // namespace chatterino