#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    // Sanity checks
    if(m_script == 0)
    {
        return;
    }
    if(m_timer.isActive())
    {
        return;
    }
    // Update the GUI
    ui->actionPlay->setEnabled(false);
    ui->actionStop->setEnabled(true);
    ui->actionAdd1Sec->setEnabled(true);
    ui->actionSub1Sec->setEnabled(true);
    // Start the timer
    m_msseStartTime = QDateTime::currentMSecsSinceEpoch();
    m_userDelay = 0;
    m_timer.start(100);
}

void MainWindow::actionStop()
{
    // Sanity check
    if (m_script == 0)
    {
        return;
    }
    if(!m_timer.isActive())
    {
        return;
    }
    // Update the GUI
    ui->actionPlay->setEnabled(true);
    ui->actionStop->setEnabled(false);
    ui->actionAdd1Sec->setEnabled(false);
    ui->actionSub1Sec->setEnabled(false);
    ui->timer->setText("-");
    ui->userDelay->setText("-");
    // Stop the timer
    m_timer.stop();
}

void MainWindow::actionConfig()
{
    // Show the config dialog
    ConfigDialog *d = new ConfigDialog(this);
    d->setModal(true);
    d->show();
    // Config changed, emit signal
    QObject::connect(d, SIGNAL(accepted()), this, SIGNAL(configChanged()));
}

void MainWindow::actionAdd1Sec()
{
    // Add 1000 msecs
    m_userDelay += 1000;
    updateUserDelay();
}

void MainWindow::actionSub1Sec()
{
    // Sub 100 msecs
    m_userDelay -= 1000;
    updateUserDelay();
}

void MainWindow::timeout()
{
    // Sanity check
    if(m_script == 0)
    {
        return;
    }
    // Gets the elapsed time in milliseconds
    qint64 msseCurrentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 msecsElapsed = (msseCurrentTime - m_msseStartTime) + m_userDelay;
    // Find events that match elapsed time
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
    // Compare events that match elapsed time with events that matched elapsed time last
    // time the timer was fired to find the differences
    i = QListIterator<Event *>(m_lastEvents);
    while (i.hasNext())
    {
        // Events that where presents and that are no more presents : suppress
        Event *e = i.next();
        if(!currentEvents.contains(e))
        {
            emit eventEnd(e);
        }
    }
    i = QListIterator<Event *>(currentEvents);
    while(i.hasNext())
    {
        // Events that are presents and that were not presents : add
        Event *e = i.next();
        if(!m_lastEvents.contains(e))
        {
            emit eventStart(e);
        }
    }
    // Update the GUI
    m_lastEvents = currentEvents;
    ui->timer->setText(QTime().addMSecs(msecsElapsed).toString());
    if(currentEvents.size() > 0)
    {
        ui->tableWidget->selectRow(m_tableMapping[currentEvents.last()]);
    }
}

void MainWindow::closeEvent(QCloseEvent *)
{
    // When the main window is close : end of the app
    qApp->exit();
}

void MainWindow::updateUserDelay()
{
    // "Pretty" set the user delay in the GUI
    if (m_userDelay >= 0)
    {
        ui->userDelay->setText("+" + QTime().addMSecs(m_userDelay).toString());
    } else {
        ui->userDelay->setText("-" + QTime().addMSecs(-m_userDelay).toString());
    }
}
