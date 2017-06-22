#include "vupdater.h"

#include <QtWidgets>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include "vconfigmanager.h"
#include "vdownloader.h"

extern VConfigManager vconfig;

VUpdater::VUpdater(QWidget *p_parent)
    : QDialog(p_parent)
{
    setupUI();
}

void VUpdater::setupUI()
{
    QImage img(":/resources/icons/vnote.svg");
    QSize imgSize(128, 128);
    QLabel *imgLabel = new QLabel();
    imgLabel->setPixmap(QPixmap::fromImage(img.scaled(imgSize)));

    m_versionLabel = new QLabel(tr("Current Version: v%1")
                                  .arg(vconfig.c_version));

    m_proLabel = new QLabel(tr("Checking for updates..."));
    m_proLabel->setOpenExternalLinks(true);
    m_proBar = new QProgressBar();
    m_proBar->setTextVisible(false);

    m_descriptionTb = new QTextBrowser();

    m_btnBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(m_btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    QVBoxLayout *verLayout = new QVBoxLayout();
    verLayout->addStretch();
    verLayout->addWidget(m_versionLabel);
    verLayout->addStretch();
    verLayout->addWidget(m_proLabel);
    verLayout->addWidget(m_proBar);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(imgLabel);
    topLayout->addLayout(verLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_descriptionTb, 1);
    mainLayout->addWidget(m_btnBox);

    m_proLabel->hide();
    m_proBar->hide();
    m_descriptionTb->hide();

    setLayout(mainLayout);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setWindowTitle(tr("VNote Update"));
}

void VUpdater::showEvent(QShowEvent *p_event)
{
    Q_UNUSED(p_event);
    QDialog::showEvent(p_event);

    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout,
            this, [this]() {
                this->checkUpdates();
            });

    timer->start();
}

void VUpdater::checkUpdates()
{
    // Change UI.
    m_proLabel->setText(tr("Checking for updates..."));
    m_proLabel->show();

    m_proBar->setEnabled(true);
    m_proBar->setMinimum(0);
    m_proBar->setMaximum(100);
    m_proBar->reset();
    m_proBar->show();

    QString url("https://api.github.com/repos/tamlok/vnote/releases/latest");
    VDownloader *downloader = new VDownloader(this);
    connect(downloader, &VDownloader::downloadFinished,
            this, &VUpdater::parseResult);
    downloader->download(url);

    m_proBar->setValue(20);
}

// Return if @p_latestVersion is newer than p_curVersion.
// They are both in format xx.xx
bool isNewerVersion(const QString &p_curVersion, const QString &p_latestVersion)
{
    QStringList curList = p_curVersion.split('.', QString::SkipEmptyParts);
    QStringList latestList = p_latestVersion.split('.', QString::SkipEmptyParts);

    if (curList.size() != 2 || latestList.size() != 2) {
        return false;
    }

    int a = curList[0].toInt();
    int b = curList[1].toInt();
    int c = latestList[0].toInt();
    int d = latestList[1].toInt();

    if (a > c) {
        return false;
    } else if (a < c) {
        return true;
    } else {
        return d > b;
    }
}

void VUpdater::parseResult(const QByteArray &p_data)
{
    m_proBar->setValue(40);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(p_data);
    QJsonObject json = jsonDoc.object();

    if (jsonDoc.isNull() || json.empty()) {
        m_proBar->setEnabled(false);
        m_proLabel->setText(tr(":( Fail to check for updates.\n"
                               "Please try it later."));

        return;
    }

    m_proBar->setValue(100);

    QString tag = json["tag_name"].toString();
    if (tag.startsWith('v') && tag.size() > 3) {
        tag = tag.right(tag.size() - 1);
    }

    QString releaseName = json["name"].toString();
    QString releaseUrl = json["html_url"].toString();
    QString body = json["body"].toString();

    m_versionLabel->setText(tr("Current Version: v%1\nLatest Version: v%2")
                              .arg(vconfig.c_version).arg(tag));
    if (isNewerVersion(vconfig.c_version, tag)) {
        m_proLabel->setText(tr("<span style=\"font-weight: bold;\">Updates Available!</span><br/>"
                               "Please visit <a href=\"%1\">Github Releases</a> to download the latest version.")
                              .arg(releaseUrl));
    } else {
        m_proLabel->setText(tr("VNote is already the latest version."));
    }

    m_descriptionTb->setText(releaseName + "\n==========\n" + body);
    m_descriptionTb->show();
    m_proBar->hide();
}
