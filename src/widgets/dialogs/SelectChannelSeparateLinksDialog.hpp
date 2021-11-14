#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QStandardItemModel>
#include <QString>
#include <QVBoxLayout>

class QTableView;
class QHBoxLayout;
class QVBoxLayout;
class QStandardItemModel;
class QString;

namespace chatterino {
class SelectChannelSeparateLinksDialog : public QDialog
{
public:
    SelectChannelSeparateLinksDialog(QWidget *parent = 0);

private:
    struct {
        QVBoxLayout mainLayout;
        QHBoxLayout *buttons_;
        QStandardItemModel *model_;
        QTableView *tableView_;
    } ui_;

};
} // namespace chatterino