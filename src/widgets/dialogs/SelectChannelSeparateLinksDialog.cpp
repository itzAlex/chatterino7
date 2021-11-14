#include "SelectChannelSeparateLinksDialog.hpp"

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

SelectChannelSeparateLinksDialog::SelectChannelSeparateLinksDialog(QWidget *parent)
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
    this->setWindowTitle("Separate links configuration");
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

    add->show();
    remove->show();

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

    this->ui_.model_ = new QStandardItemModel(0, 1, this);
    this->ui_.tableView_ = new QTableView(this);

    this->ui_.model_->setParent(this);
    this->ui_.model_->setHeaderData(0, Qt::Horizontal, QObject::tr("Channels"));

    this->ui_.tableView_->setModel(this->ui_.model_);

    this->ui_.tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    this->ui_.tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->ui_.tableView_->horizontalHeader()->setStretchLastSection(true);
    this->ui_.tableView_->verticalHeader()->setVisible(false);

    this->ui_.tableView_->show();

    // Widgets and layouts
    this->ui_.mainLayout.addLayout(buttons);
    this->ui_.mainLayout.addWidget(this->ui_.tableView_);

    this->ui_.buttons_ = buttons;

    // OK and Cancel buttons & Checkbox
    auto buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttonBox->setCenterButtons(true);
    this->ui_.mainLayout.addWidget(buttonBox);

    // Add previous elements from config
    for (QString channel : getSettings()->separateLinksChannels)
    {
        QStandardItem *item =
                new QStandardItem(channel);
        this->ui_.model_->appendRow(item);
    }


    // Signals
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
        getSettings()->clearSeparatedLinkChannels();

        for (int row = 0; row < this->ui_.model_->rowCount(); row++)
        {
            getSettings()->addSeparatedLinkChannel(this->ui_.model_->index(row, 0)
                                       .data()
                                       .toString());
        }

        this->accept();
        this->close();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, [this]() {
        this->reject();
        this->close();
    });
}
}