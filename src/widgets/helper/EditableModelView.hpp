#pragma once

#include <QPushButton>
#include <QWidget>

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
    void setValidationRegexp(QRegularExpression regexp);

    QTableView *getTableView();
    QAbstractTableModel *getModel();
    QPushButton *selectChannel;
    QPushButton *excludeChannel;

    pajlada::Signals::NoArgSignal addButtonPressed;
    pajlada::Signals::NoArgSignal selectChannelPressed;
    pajlada::Signals::NoArgSignal excludeChannelPressed;

    void addCustomButton(QWidget *widget);
    void addSelectChannelHighlight();
    void addExcludeChannelHighlight();
    void disableSelectChannelButton();
    void enableSelectChannelButton();
    void disableExcludeChannelButton();
    void enableExcludeChannelButton();
    void addRegexHelpLink();

private:
    QTableView *tableView_{};
    QAbstractTableModel *model_{};
    QHBoxLayout *buttons_{};

    void moveRow(int dir);

public:
    void selectRow(int row);
};

}  // namespace chatterino
