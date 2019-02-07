#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include <QDialog>
#include <QItemSelection>

#include "Message.h"

namespace Ui {
class MessageWindow;
}

class MessageWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MessageWindow(QWidget *parent = 0);
    ~MessageWindow();

signals:
    void replyMessage(const QString &call);

public slots:
    void setCall(const QString &call);
    void populateMessages(QList<Message> msgs);
    QString prepareReplyMessage(QString path, QString text);

private slots:
    void on_messageTableWidget_selectionChanged(const QItemSelection &/*selected*/, const QItemSelection &/*deselected*/);
    void on_replyPushButton_clicked();

private:
    Ui::MessageWindow *ui;
};

#endif // MESSAGEWINDOW_H
