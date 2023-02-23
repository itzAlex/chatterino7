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
class TokenDialog : public QDialog
{
public:
    TokenDialog(int provider, QWidget *parent = 0);

private:
    struct {
        QVBoxLayout mainLayout;
    } ui_;
};
}  // namespace chatterino