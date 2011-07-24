#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFileDialog>


#include "configdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_script = 0;
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::actionOpen() {
    // Ask the user for an *.ass file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open subtitles"), QString::null, tr("Subtitle Files (*.ass)"));
    // Ass file selected ?
    if (!fileName.isEmpty()) {
        // Clean-up previously allocated resources & reset GUI
        if(m_script != 0)
        {
            delete m_script;
        }
        if(m_timer.isActive())
        {
            m_timer.stop();
        }
        m_msseStartTime = 0;
        m_userDelay = 0;
        m_lastEvents.clear();
        m_tableMapping.clear();
        // Create the script & setup the GUI
        m_script = new Script(fileName, this);
        setWindowTitle(m_script->title());
        ui->tableWidget->setRowCount(m_script->eventsCount());
        QListIterator<Event *> i = m_script->events();
        int row = 0;
        while (i.hasNext())
        {
            Event *event = i.next();
            m_tableMapping[event] = row;
            QTableWidgetItem *startItem = new QTableWidgetItem(QTime().addMSecs(event->msseStart()).toString());
            ui->tableWidget->setItem(row, 0, startItem);
            QTableWidgetItem *endItem = new QTableWidgetItem(QTime().addMSecs(event->msseEnd()).toString());
            ui->tableWidget->setItem(row, 1, endItem);
            QTableWidgetItem *styleItem = new QTableWidgetItem(event->style()->name());
            ui->tableWidget->setItem(row, 2, styleItem);
            QTableWidgetItem *textItem = new QTableWidgetItem(event->text());
            ui->tableWidget->setItem(row, 3, textItem);
            row++;
        }
        ui->actionPlay->setEnabled(true);
        ui->actionStop->setEnabled(false);
        ui->actionAdd1Sec->setEnabled(false);
        ui->actionSub1Sec->setEnabled(false);
    }
}

void MainWindow::actionPlay()
{
    if(m_script == 0)
    {
        return;
    }
    if(m_timer.isActive())
    {
        return;
    }
    ui->actionPlay->setEnabled(false);
    ui->actionStop->setEnabled(true);
    ui->actionAdd1Sec->setEnabled(true);
    ui->actionSub1Sec->setEnabled(true);
    m_msseStartTime = QDateTime::currentMSecsSinceEpoch();
    m_userDelay = 0;
    m_timer.start(100);
}

void MainWindow::actionStop()
{
    if (m_script == 0)
    {
        return;
    }
    if(!m_timer.isActive())
    {
        return;
    }
    ui->actionPlay->setEnabled(true);
    ui->actionStop->setEnabled(false);
    ui->actionAdd1Sec->setEnabled(false);
    ui->actionSub1Sec->setEnabled(false);
    ui->timer->setText("-");
    m_timer.stop();
}

void MainWindow::actionConfig()
{
    ConfigDialog *d = new ConfigDialog(this);
    d->setModal(true);
    d->show();
    QObject::connect(d, SIGNAL(accepted()), this, SIGNAL(configChanged()));
}

void MainWindow::actionAdd1Sec()
{
    m_userDelay += 1000;
    updateUserDelay();
}

void MainWindow::actionSub1Sec()
{
    m_userDelay -= 1000;
    updateUserDelay();
}

void MainWindow::timeout()
{
    if(m_script == 0)
    {
        return;
    }
    qint64 msseCurrentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 msecsElapsed = (msseCurrentTime - m_msseStartTime) + m_userDelay;

    QList<Event *> currentEvents;
    QListIterator<Event *> i = m_script->events();
    while(i.hasNext())
    {
        Event *e = i.next();
        if(e->match(msecsElapsed))
        {
            currentEvents.append(e);
        }

    }

    i = QListIterator<Event *>(m_lastEvents);
    while (i.hasNext())
    {
        Event *e = i.next();
        if(!currentEvents.contains(e))
        {
            emit eventEnd(e);
        }
    }

    i = QListIterator<Event *>(currentEvents);
    while(i.hasNext())
    {
        Event *e = i.next();
        if(!m_lastEvents.contains(e))
        {
            emit eventStart(e);
        }
    }
    m_lastEvents = currentEvents;
    ui->timer->setText(QTime().addMSecs(msecsElapsed).toString());
    if(currentEvents.size() > 0)
    {
        ui->tableWidget->selectRow(m_tableMapping[currentEvents.last()]);
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    qApp->exit();
}

void MainWindow::updateUserDelay()
{
    if (m_userDelay >= 0)
    {
        ui->userDelay->setText("+" + QTime().addMSecs(m_userDelay).toString());
    } else {
        ui->userDelay->setText("-" + QTime().addMSecs(-m_userDelay).toString());
    }
}
