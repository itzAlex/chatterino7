#pragma once

#include <QWidget>
#include <QPushButton>

#include <pajlada/signals/signal.hpp>

class QAbstractTableModel;
class QTableView;
class QHBoxLayout;
class QPushButton;

namespace chatterino {

class EditableModelView : public QWidget
{
public:
    EditableModelView(QAbstractTableModel *model, bool movable = true);

    void setTitles(std::initializer_list<QString> titles);

    QTableView *getTableView();
    QAbstractTableModel *getModel();
    QPushButton *selectChannel;

    pajlada::Signals::NoArgSignal addButtonPressed;
    pajlada::Signals::NoArgSignal selectChannelPressed;

    void addCustomButton(QWidget *widget);
    void addSelectChannelHighlight();
    void disableSelectChannelButton();
    void enableSelectChannelButton();
    void addRegexHelpLink();

private:
    QTableView *tableView_{};
    QAbstractTableModel *model_{};
    QHBoxLayout *buttons_{};

    void moveRow(int dir);
    void selectRow(int row);
};

}  // namespace chatterino
