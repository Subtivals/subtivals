#include <QtCore/QSettings>
#include <QtGui/QDesktopWidget>

#include "configdialog.h"
#include "ui_configdialog.h"

#include "script.h"
#include "styleeditor.h"


ConfigDialog::ConfigDialog(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ConfigDialog),
    m_styleEditor(new StyleEditor())
{
    ui->setupUi(this);
    setFeatures(ConfigDialog::NoDockWidgetFeatures);
    ui->tabStyles->setLayout(m_styleEditor->layout());
    adjustSize();

    QDesktopWidget *dw = QApplication::desktop();
    for(int i = 0; i < dw->screenCount(); i++)
    {
        ui->screens->addItem(QString(tr("Monitor %1")).arg(i));
    }
}

void ConfigDialog::setScript(Script* script)
{
    m_styleEditor->setScript(script);
}

ConfigDialog::~ConfigDialog()
{
    delete m_styleEditor;
    delete ui;
}

void ConfigDialog::screenChanged(const QRect& r)
{
    // Show values in form
    ui->x->setValue(r.x());
    ui->y->setValue(r.y());
    ui->w->setValue(r.width());
    ui->h->setValue(r.height());
    save();
}

void ConfigDialog::restore()
{
    // Apply default screen size
    QSettings settings;
    settings.remove("SubtitlesForm");
    reset();
}

void ConfigDialog::reset()
{
    // Reload from settings
    QSettings settings;
    settings.beginGroup("SubtitlesForm");
    int screen = settings.value("screen", 0).toInt();
    int x = settings.value("x", 0).toInt();
    int y = settings.value("y", 0).toInt();
    int w = settings.value("w", qApp->desktop()->screen(qApp->desktop()->primaryScreen())->width()).toInt();
    int h = settings.value("h", 300).toInt();
    settings.endGroup();

    ui->screens->setCurrentIndex(screen);
    screenChanged(QRect(x, y, w, h));
    m_styleEditor->reset();
}

void ConfigDialog::save()
{
    // Save settings
    QSettings settings;
    settings.beginGroup("SubtitlesForm");
    settings.setValue("screen", ui->screens->currentIndex());
    settings.setValue("x", ui->x->text());
    settings.setValue("y", ui->y->text());
    settings.setValue("w", ui->w->text());
    settings.setValue("h", ui->h->text());
    settings.endGroup();
    m_styleEditor->save();
}

void ConfigDialog::apply()
{
    int screen = ui->screens->currentIndex();
    QRect r(ui->x->text().toInt(),
            ui->y->text().toInt(),
            ui->w->text().toInt(),
            ui->h->text().toInt());
    emit changeScreen(screen, r);
    m_styleEditor->apply();
}
