#include "SelectChannelHighlightPopup.hpp"

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

HighlightPhrase getRow(QString type, int selected)
{
    if (type == "messages")
    {
        return getSettings()->highlightedMessages[selected];
    }

    else if (type == "users")
    {
        return getSettings()->highlightedUsers[selected];
    }
}

SelectChannelWidget::SelectChannelWidget(int selected, QString type,
                                         QWidget *parent)
    : QDialog(parent)
{
#ifdef USEWINSDK
    ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif

    // Highlight
    HighlightPhrase row = getRow(type, selected);

    // Layout properties
    this->setModal(true);
    this->setMinimumWidth(250);
    this->adjustSize();
    this->setWindowTitle("Highlight configuration");
    this->setLayout(&this->ui_.mainLayout);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    this->setWindowFlag(Qt::WindowStaysOnTopHint, true);

    // Separator line
    QFrame *line = new QFrame(this);
    line->setStyleSheet("background-color: rgb(128,128,128)");
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    line->hide();

    // Checkbox - Global highlight
    auto checkbox = new QCheckBox("Set highlight to global", this);

    // Add & Remove buttons
    QHBoxLayout *buttons = new QHBoxLayout();

    QPushButton *add = new QPushButton("Add");
    buttons->addWidget(add);
    QObject::connect(add, &QPushButton::clicked, [this] {
        QStandardItem *item = new QStandardItem("Username");
        this->ui_.model_->appendRow(item);
    });

    QPushButton *remove = new QPushButton("Remove");
    buttons->addWidget(remove);

    add->hide();
    remove->hide();

    QObject::connect(remove, &QPushButton::clicked, [this] {
        auto selected = this->ui_.tableView_->selectionModel()->selectedRows(0);

        // Remove rows backwards so indices don't shift.
        std::vector<int> rows;
        for (auto &&index : selected)
            rows.push_back(index.row());

        std::sort(rows.begin(), rows.end(), std::greater{});

        for (auto &&row : rows)
            this->ui_.model_->removeRow(row);
    });

    buttons->addStretch();

    // Table
    this->ui_.model_ = new QStandardItemModel(0, 1, this);
    this->ui_.tableView_ = new QTableView(this);

    this->ui_.model_->setParent(this);
    this->ui_.model_->setHeaderData(0, Qt::Horizontal, QObject::tr("Channels"));

    this->ui_.tableView_->setModel(this->ui_.model_);

    this->ui_.tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    this->ui_.tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->ui_.tableView_->horizontalHeader()->setStretchLastSection(true);
    this->ui_.tableView_->verticalHeader()->setVisible(false);

    this->ui_.tableView_->hide();

    // Widgets and layouts
    this->ui_.mainLayout.addWidget(checkbox);
    this->ui_.mainLayout.addWidget(line);
    this->ui_.mainLayout.addLayout(buttons);
    this->ui_.mainLayout.addWidget(this->ui_.tableView_);

    this->ui_.buttons_ = buttons;

    // OK and Cancel buttons & Checkbox
    auto buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttonBox->setCenterButtons(true);
    this->ui_.mainLayout.addWidget(buttonBox);

    // Add previous elements from config
    if (row.isGlobalHighlight())
    {
        checkbox->setChecked(true);
    }
    else
    {
        checkbox->setChecked(false);
        add->show();
        remove->show();
        this->ui_.tableView_->show();
        line->show();
        this->adjustSize();
    }

    for (std::string channel : row.getChannels())
    {
        QStandardItem *item =
            new QStandardItem(QString::fromStdString(channel));
        this->ui_.model_->appendRow(item);
    }

    // Signals
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
        std::vector<std::string> channels;

        for (int row = 0; row < this->ui_.model_->rowCount(); row++)
        {
            channels.push_back(this->ui_.model_->index(row, 0)
                                   .data()
                                   .toString()
                                   .toStdString());
        }

        if (type == "messages")
        {
            getSettings()->highlightedMessages.insert(
                HighlightPhrase{row.getPattern(), row.showInMentions(),
                                row.hasAlert(), row.hasSound(), row.isRegex(),
                                row.isCaseSensitive(),
                                row.getSoundUrl().toString(), row.getColor(),
                                checkbox->isChecked(), channels},
                selected);

            getSettings()->highlightedMessages.removeAt(selected + 1);
        }

        if (type == "users")
        {
            getSettings()->highlightedUsers.insert(
                HighlightPhrase{row.getPattern(), row.showInMentions(),
                                row.hasAlert(), row.hasSound(), row.isRegex(),
                                row.isCaseSensitive(),
                                row.getSoundUrl().toString(), row.getColor(),
                                checkbox->isChecked(), channels},
                selected);

            getSettings()->highlightedUsers.removeAt(selected + 1);
        }

        this->accept();
        this->close();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, [this]() {
        this->reject();
        this->close();
    });

    QObject::connect(checkbox, &QCheckBox::stateChanged, [=](int state) {
        if (state)
        {
            add->hide();
            remove->hide();
            this->ui_.tableView_->hide();
            line->hide();
            this->adjustSize();
        }
        else
        {
            add->show();
            remove->show();
            this->ui_.tableView_->show();
            line->show();
            this->adjustSize();
        }
    });
}

ExcludeChannelWidget::ExcludeChannelWidget(int selected, QString type,
                                           QWidget *parent)
    : QDialog(parent)
{
#ifdef USEWINSDK
    ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif

    // Highlight
    HighlightPhrase row = getRow(type, selected);

    // Layout properties
    this->setModal(true);
    this->setMinimumWidth(250);
    this->adjustSize();
    this->setWindowTitle("Blacklist channels");
    this->setLayout(&this->ui_.mainLayout);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    this->setWindowFlag(Qt::WindowStaysOnTopHint, true);

    // Add & Remove buttons
    QHBoxLayout *buttons = new QHBoxLayout();

    QPushButton *add = new QPushButton("Add");
    buttons->addWidget(add);

    QObject::connect(add, &QPushButton::clicked, [this] {
        QStandardItem *item = new QStandardItem("Username");
        this->ui_.model_->appendRow(item);
    });

    QPushButton *remove = new QPushButton("Remove");
    buttons->addWidget(remove);

    QObject::connect(remove, &QPushButton::clicked, [this] {
        auto selected = this->ui_.tableView_->selectionModel()->selectedRows(0);

        // Remove rows backwards so indices don't shift.
        std::vector<int> rows;
        for (auto &&index : selected)
            rows.push_back(index.row());

        std::sort(rows.begin(), rows.end(), std::greater{});

        for (auto &&row : rows)
            this->ui_.model_->removeRow(row);
    });

    buttons->addStretch();

    // Table
    this->ui_.model_ = new QStandardItemModel(0, 1, this);
    this->ui_.tableView_ = new QTableView(this);

    this->ui_.model_->setParent(this);
    this->ui_.model_->setHeaderData(0, Qt::Horizontal, QObject::tr("Channels"));

    this->ui_.tableView_->setModel(this->ui_.model_);

    this->ui_.tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    this->ui_.tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->ui_.tableView_->horizontalHeader()->setStretchLastSection(true);
    this->ui_.tableView_->verticalHeader()->setVisible(false);

    // Widgets and layouts
    this->ui_.mainLayout.addLayout(buttons);
    this->ui_.mainLayout.addWidget(this->ui_.tableView_);

    this->ui_.buttons_ = buttons;

    // OK and Cancel buttons & Checkbox
    auto buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttonBox->setCenterButtons(true);
    this->ui_.mainLayout.addWidget(buttonBox);

    for (std::string channel : row.getExcludedChannels())
    {
        QStandardItem *item =
            new QStandardItem(QString::fromStdString(channel));
        this->ui_.model_->appendRow(item);
    }

    // Signals
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
        std::vector<std::string> ExcludedChannels;

        for (int row = 0; row < this->ui_.model_->rowCount(); row++)
        {
            ExcludedChannels.push_back(this->ui_.model_->index(row, 0)
                                           .data()
                                           .toString()
                                           .toStdString());
        }

        if (type == "messages")
        {
            getSettings()->highlightedMessages.insert(
                HighlightPhrase{row.getPattern(), row.showInMentions(),
                                row.hasAlert(), row.hasSound(), row.isRegex(),
                                row.isCaseSensitive(),
                                row.getSoundUrl().toString(), row.getColor(),
                                row.isGlobalHighlight(), row.getChannels(),
                                ExcludedChannels},
                selected);

            getSettings()->highlightedMessages.removeAt(selected + 1);
        }

        if (type == "users")
        {
            getSettings()->highlightedUsers.insert(
                HighlightPhrase{row.getPattern(), row.showInMentions(),
                                row.hasAlert(), row.hasSound(), row.isRegex(),
                                row.isCaseSensitive(),
                                row.getSoundUrl().toString(), row.getColor(),
                                row.isGlobalHighlight(), row.getChannels(),
                                ExcludedChannels},
                selected);

            getSettings()->highlightedUsers.removeAt(selected + 1);
        }

        this->accept();
        this->close();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, [this]() {
        this->reject();
        this->close();
    });
}
}  // namespace chatterino
