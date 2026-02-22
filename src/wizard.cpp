#include "wizard.h"
#include "ui_wizard.h"

#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QApplication>
#include <QScreen>

static QPixmap svgToPixmap(const QString &svgPath, const QSize &targetSize) {
  QSvgRenderer renderer(svgPath);
  if (!renderer.isValid())
    return QPixmap();
  QImage image(targetSize, QImage::Format_ARGB32);
  image.fill(Qt::transparent);

  QPainter p(&image);
  renderer.render(&p);
  p.end();

  return QPixmap::fromImage(image);
}

Wizard::Wizard(QWidget *parent) : QWizard(parent), ui(new Ui::Wizard) {
  ui->setupUi(this);

  QSize size = QSize(64, 64);
  ui->labelHelp->setPixmap(svgToPixmap(":/icons/help.svg", size));
  ui->labelScreen->setPixmap(
      svgToPixmap(":/icons/preferences-desktop.svg", size));
  ui->labelCharacters->setPixmap(svgToPixmap(":/icons/characters.svg", size));
  ui->labelShortcuts->setPixmap(svgToPixmap(":/icons/shortcuts.svg", size));
  ui->labelTextIcon->setPixmap(svgToPixmap(":/icons/text-icon.svg", size));
  ui->labelAutohide->setPixmap(svgToPixmap(":/icons/auto-hide.svg", size));

  ui->communityLabel->setText(ui->communityLabel->text().replace(
      "COMMUNITY_WEBSITE", QString(COMMUNITY_WEBSITE)));

  adjustSize();
}

Wizard::~Wizard() { delete ui; }
