#include "HomiesPage.hpp"

#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#include "Application.hpp"
#include "common/Version.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/helper/Line.hpp"
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

    layout.addTitle("Emotes");
    layout.addCheckbox("Enable Homies global emotes (requires restart)",
                       s.enableHomiesGlobalEmotes);
    layout.addTitle("Apperance");
    layout.addCheckbox("Gray-out historical messages", s.grayOutRecents);

    layout.addTitle("Behaviour");
    layout.addCheckbox("Mention users with an at sign (@User)", s.mentionUsersWithAt);

    layout.addStretch();
    auto inv = new BaseWidget(this);
    layout.addWidget(inv);
}

} // namespace chatterino