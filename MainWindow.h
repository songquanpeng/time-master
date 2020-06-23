#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QSystemTrayIcon>
#include <QSqlTableModel>

struct Task {
    QString description;
    int timeLimit = -1;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    virtual void timerEvent(QTimerEvent* event);
    virtual void closeEvent(QCloseEvent* event);
    ~MainWindow();

public slots:
    void on_startReminderBtn_clicked();
    void on_stopReminderBtn_clicked();
    void on_workTimeSlider_valueChanged(int val);
    void on_breakTimeSlider_valueChanged(int val);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void on_addTaskBtn_clicked();
    void on_deleteTaskBtn_clicked();
    void on_startTaskBtn_clicked();
    void on_stopTaskBtn_clicked();

private:
    Ui::MainWindow *ui;
    int minuteTimer;
    int secondTimer;
    int minuteCounter;
    int secondCounter;
    bool isTimerPaused;
    bool isWorking;
    bool isDoingTask;
    int workTimeLength;
    int breakTimeLength;
    Task task;

    QAction* optionsAction;
    QAction* quitAction;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    QSqlTableModel* model;

    void remindUser(QString prompt);
    void minuteLapse();
    void secondLapse();
    void createActions();
    void createTrayIcon();
    void initializeTable();

};
#endif // MAINWINDOW_H
