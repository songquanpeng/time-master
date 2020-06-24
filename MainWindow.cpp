#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QModelIndex>
#include <QModelIndexList>
#include <QTableView>
#include <QSqlField>
#include <cmath>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    createActions();
    createTrayIcon();
    initializeTable();
    isReminderWorking = false;
    isWorking = true;
    isDoingTask = false;
    isNewTask = true;
    minuteTimer = startTimer(60 * 1000);
    secondTimer = startTimer(1000);
    reminderMinuteCounter = 0;
    reminderSecondCounter = 0;
    taskMinuteCounter = 0;
    taskSecondCounter = 0;
    workTimeLength = ui->workTimeSlider->value();
    breakTimeLength = ui->breakTimeSlider->value();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::timerEvent(QTimerEvent *event) {
    if (event->timerId() == secondTimer) secondLapse();
    if (event->timerId() == minuteTimer) minuteLapse();
}

void MainWindow::minuteLapse() {
    if (isDoingTask) {
        taskMinuteCounter--;
        ui->minuteLCD->display(taskMinuteCounter);
        if (taskMinuteCounter < 0) {
            ui->minuteLCD->display(0);
            ui->secondLCD->display(0);
            isDoingTask = false;
            remindUser("Task " + task.description + " has run out of time.");
        }
    }

    if (isReminderWorking && isWorking) {
        reminderMinuteCounter++;
        ui->timeProgressBar->setValue(ceil(100 * reminderMinuteCounter / workTimeLength));
        if (reminderMinuteCounter >= workTimeLength) {
            reminderMinuteCounter = 0;
            remindUser("Time to take a break.");
            statusBar()->showMessage("Breaking time.");
            isWorking = !isWorking;
        }
    }

}

void MainWindow::secondLapse() {
    if (isDoingTask) {
        taskSecondCounter--;
        if (taskSecondCounter == -1) taskSecondCounter = 60;
        ui->secondLCD->display(taskSecondCounter);
    }

    if (isReminderWorking && !isWorking) {
        reminderSecondCounter++;
        ui->timeProgressBar->setValue(ceil(100 * reminderSecondCounter / breakTimeLength));
        if (reminderSecondCounter >= breakTimeLength) {
            reminderSecondCounter = 0;
            remindUser("Time to move on.");
            statusBar()->showMessage("Working time.");
            isWorking = !isWorking;
            ui->timeProgressBar->setValue(0);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    hide();
    event->ignore();
}

void MainWindow::on_startReminderBtn_clicked() {
    isReminderWorking = !isReminderWorking;
    ui->startReminderBtn->setText(!isReminderWorking ? "Start" : "Pause");
    statusBar()->showMessage(!isReminderWorking ? "Timer paused." : (isWorking ? "Working time." : "Breaking time."));
}

void MainWindow::on_stopReminderBtn_clicked() {
    isReminderWorking = false;
    isWorking = true;
    ui->startReminderBtn->setText("Start");
    ui->timeProgressBar->setValue(0);
    reminderMinuteCounter = 0;
    reminderSecondCounter = 0;
    statusBar()->showMessage("Timer stopped.");
}

void MainWindow::on_startTaskBtn_clicked() {
    if (isNewTask) {
        if (model->rowCount() == 0) return;
        int currentRow = ui->tableView->currentIndex().row();
        if (currentRow == -1) currentRow = 0;
        auto record = model->record(currentRow);
        task.description = record.field(1).value().toString();
        task.timeLimit = record.field(2).value().toInt();
        taskMinuteCounter = task.timeLimit - 1;
        taskSecondCounter = 60;
        isDoingTask = true;
        isNewTask = false;
        ui->minuteLCD->display(taskMinuteCounter);
    } else {
        isDoingTask = !isDoingTask;
    }
    statusBar()->showMessage(isDoingTask ? "Current task: " + task.description : "Task paused");
    ui->startTaskBtn->setText(isDoingTask ? "Pause" : "Resume");
}

void MainWindow::on_stopTaskBtn_clicked() {
    isDoingTask = false;
    isNewTask = true;
    ui->startTaskBtn->setText("Start");
    ui->minuteLCD->display(0);
    ui->secondLCD->display(0);
    taskMinuteCounter = 0;
    taskSecondCounter = 0;
    statusBar()->showMessage("You have quited current task.");
}

void MainWindow::on_workTimeSlider_valueChanged(int val) {
    workTimeLength = val;
    ui->workTimeLabel->setText(QString::number(workTimeLength) + " min");
}

void MainWindow::on_breakTimeSlider_valueChanged(int val) {
    breakTimeLength = val;
    ui->breakTimeLabel->setText(QString::number(breakTimeLength) + " sec");
}

void MainWindow::remindUser(QString prompt) {
    trayIcon->showMessage("Time Master", prompt);
}

void MainWindow::createActions() {
    optionsAction = new QAction(tr("&Options"), this);
    connect(optionsAction, &QAction::triggered, this, &QWidget::showNormal);
    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::createTrayIcon() {
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    trayIcon->setToolTip("Time Master");
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/image/icon"));
    trayIcon->setVisible(true);
    trayIcon->show();
}

void MainWindow::initializeTable() {
    // Initialse connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("data.db");
    if (!db.open()) {
        statusBar()->showMessage("Unable to establish a database connection.");
        return;
    }
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS 'task' ("
               "'id'	INTEGER,"
               "'task_name'	TEXT DEFAULT 'Study',"
               "'time_limit'	INTEGER DEFAULT 60,"
               "PRIMARY KEY('id' AUTOINCREMENT)"
               ");");
    // Initialize table model
    model = new QSqlTableModel;
    model->setTable("task");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select();
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Task"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Time"));
    // Setup UI
    ui->tableView->setModel(model);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->hideColumn(0);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Unknown:
        break;
    case QSystemTrayIcon::Context:
        break;
    case QSystemTrayIcon::DoubleClick:
        QWidget::showNormal();
        break;
    case QSystemTrayIcon::Trigger:
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        break;
    }
}

void MainWindow::on_addTaskBtn_clicked() {
    int rowNum = model->rowCount();
    model->insertRow(rowNum);
    ui->tableView->scrollToBottom();
    model->submitAll();
}

void MainWindow::on_deleteTaskBtn_clicked() {
    if (QMessageBox::warning(this, "Delete task", "Are you sure delete selected task?",
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        int currentRow = ui->tableView->currentIndex().row();
        model->removeRow(currentRow);
        model->select();
    }
}



