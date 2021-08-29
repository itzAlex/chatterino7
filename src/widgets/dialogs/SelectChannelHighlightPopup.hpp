#pragma once

#include "controllers/highlights/HighlightModel.hpp"

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
class SelectChannelWidget : public QDialog
{
public:
    SelectChannelWidget(int selected, QWidget *parent = 0);

private:
    struct {
        QVBoxLayout mainLayout;
        QHBoxLayout *buttons_;
        QStandardItemModel *model_;
        QTableView *tableView_;
    } ui_;
};

class ExcludeChannelWidget : public QDialog
{
public:
    ExcludeChannelWidget(int selected, QWidget *parent = 0);

private:
    struct {
        QVBoxLayout mainLayout;
        QHBoxLayout *buttons_;
        QStandardItemModel *model_;
        QTableView *tableView_;
    } ui_;
};

}  // namespace chatterino
