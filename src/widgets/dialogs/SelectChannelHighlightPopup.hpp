#pragma once

#include "controllers/highlights/HighlightModel.hpp"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardItemModel>
#include <QString>

class QTableView;
class QHBoxLayout;
class QVBoxLayout;
class QStandardItemModel;
class QString;

namespace chatterino {
class SelectChannelWidget : public QDialog
{
public:
    SelectChannelWidget(QWidget *parent, int selected);

private:
    struct {
        QVBoxLayout mainLayout;
        QHBoxLayout *buttons_;
        QStandardItemModel *model_;
        QTableView *tableView_;
    } ui_;

};

}  // namespace chatterino