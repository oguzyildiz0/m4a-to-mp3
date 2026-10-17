#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QDir>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QIcon>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QPainter>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QLinearGradient>

namespace {
constexpr auto kApplicationName = "SesDönüştür";
constexpr auto kApplicationVersion = "1.1.0";

QIcon createApplicationIcon()
{
    QPixmap pixmap(128, 128);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient background(0, 0, 128, 128);
    background.setColorAt(0.0, QColor(QStringLiteral("#2563eb")));
    background.setColorAt(1.0, QColor(QStringLiteral("#7c3aed")));
    painter.setBrush(background);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(4, 4, 120, 120), 28, 28);

    const int heights[] = {18, 32, 48, 27, 42, 56, 34, 22, 39};
    painter.setBrush(QColor(QStringLiteral("#ffffff")));
    for (int i = 0; i < 9; ++i) {
        const qreal x = 22 + i * 10;
        const qreal y = 48 + (56 - heights[i]) / 2.0;
        painter.drawRoundedRect(QRectF(x, y, 6, heights[i]), 3, 3);
    }

    painter.setPen(QColor(QStringLiteral("#ffffff")));
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(18);
    painter.setFont(font);
    painter.drawText(QRect(12, 88, 104, 24), Qt::AlignCenter, QStringLiteral("MP3"));
    return QIcon(pixmap);
}
}

class MainWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle(QStringLiteral("SesDönüştür — M4A'dan MP3'e"));
        setWindowIcon(createApplicationIcon());
        setMinimumSize(620, 500);
        resize(760, 590);
        setAcceptDrops(true);

        auto *title = new QLabel(QStringLiteral("SesDönüştür"), this);
        title->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 600;"));

        auto *subtitle = new QLabel(
            QStringLiteral("M4A, M4B, AAC ve MP4 dosyalarınızı kolayca MP3'e dönüştürün."),
            this);
        subtitle->setWordWrap(true);

        auto *infoButton = new QPushButton(QStringLiteral("ⓘ Bilgi"), this);
        infoButton->setToolTip(QStringLiteral("Uygulama ve sürüm bilgileri"));
        auto *headingLayout = new QHBoxLayout;
        auto *headingTextLayout = new QVBoxLayout;
        headingTextLayout->addWidget(title);
        headingTextLayout->addWidget(subtitle);
        headingLayout->addLayout(headingTextLayout, 1);
        headingLayout->addWidget(infoButton, 0, Qt::AlignTop);

        m_filesList = new QListWidget(this);
        m_filesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_filesList->setAlternatingRowColors(true);

        m_addButton = new QPushButton(QStringLiteral("Dosya ekle…"), this);
        m_removeButton = new QPushButton(QStringLiteral("Seçileni kaldır"), this);
        m_clearButton = new QPushButton(QStringLiteral("Listeyi temizle"), this);

        auto *fileButtons = new QHBoxLayout;
        fileButtons->addWidget(m_addButton);
        fileButtons->addWidget(m_removeButton);
        fileButtons->addStretch();
        fileButtons->addWidget(m_clearButton);

        auto *dropHint = new QLabel(
            QStringLiteral("Dosyaları buraya sürükleyip bırakabilir veya “Dosya ekle”yi kullanabilirsiniz."),
            this);
        dropHint->setWordWrap(true);

        m_outputPath = new QLineEdit(this);
        m_outputPath->setPlaceholderText(QStringLiteral("Çıkış klasörünü seçin"));
        m_outputPath->setText(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
        m_browseButton = new QPushButton(QStringLiteral("Gözat…"), this);

        auto *outputLayout = new QHBoxLayout;
        outputLayout->addWidget(m_outputPath, 1);
        outputLayout->addWidget(m_browseButton);

        m_progress = new QProgressBar(this);
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
        m_progress->setTextVisible(true);

        m_status = new QLabel(QStringLiteral("Dönüştürmeye hazır."), this);
        m_status->setWordWrap(true);

        m_convertButton = new QPushButton(QStringLiteral("Dönüştür"), this);
        m_convertButton->setDefault(true);
        m_cancelButton = new QPushButton(QStringLiteral("İptal"), this);
        m_cancelButton->setEnabled(false);

        auto *actionLayout = new QHBoxLayout;
        actionLayout->addStretch();
        actionLayout->addWidget(m_cancelButton);
        actionLayout->addWidget(m_convertButton);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 22, 24, 22);
        layout->setSpacing(12);
        layout->addLayout(headingLayout);
        layout->addLayout(fileButtons);
        layout->addWidget(m_filesList, 1);
        layout->addWidget(dropHint);
        layout->addWidget(new QLabel(QStringLiteral("Çıkış klasörü"), this));
        layout->addLayout(outputLayout);
        layout->addWidget(m_progress);
        layout->addWidget(m_status);
        layout->addLayout(actionLayout);

        connect(m_addButton, &QPushButton::clicked, this, &MainWindow::chooseFiles);
        connect(m_removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedFiles);
        connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::clearFiles);
        connect(m_browseButton, &QPushButton::clicked, this, &MainWindow::chooseOutputDirectory);
        connect(m_convertButton, &QPushButton::clicked, this, &MainWindow::startConversion);
        connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelConversion);
        connect(infoButton, &QPushButton::clicked, this, &MainWindow::showAbout);
        connect(&m_process, &QProcess::readyReadStandardOutput,
                this, &MainWindow::readProgress);
        connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
            m_errorOutput += m_process.readAllStandardError();
        });
        connect(&m_process,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &MainWindow::processFinished);
        connect(&m_process, &QProcess::errorOccurred,
                this, &MainWindow::processError);

        updateButtons();
    }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (!m_running && event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent *event) override
    {
        if (m_running) {
            return;
        }

        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                addFile(url.toLocalFile());
            }
        }
        event->acceptProposedAction();
    }

private:
    void showAbout()
    {
        QDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("SesDönüştür hakkında"));
        dialog.setWindowIcon(windowIcon());
        dialog.resize(480, 390);

        auto *content = new QTextBrowser(&dialog);
        content->setOpenExternalLinks(true);
        content->setFrameShape(QFrame::NoFrame);
        content->setHtml(QStringLiteral(
            "<h2>SesDönüştür</h2>"
            "<p><b>M4A'dan MP3'e, zahmetsizce.</b></p>"
            "<p>Ses dosyalarınızı FFmpeg gücüyle hızlı ve güvenilir biçimde "
            "MP3'e dönüştürün. Birden fazla dosyayı sıraya alın, işlemi takip edin "
            "ve gerektiğinde iptal edin.</p>"
            "<p><b>Sürüm:</b> %1</p>"
            "<p><b>Sürüm notları</b></p>"
            "<ul>"
            "<li>Birden fazla ses dosyasını sırayla dönüştürme</li>"
            "<li>Sürükle-bırak ile kolay dosya ekleme</li>"
            "<li>Dönüştürme ilerlemesini izleme ve işlemi iptal etme</li>"
            "<li>Mevcut dosyaları koruyarak benzersiz çıktı adları oluşturma</li>"
            "</ul>"
            "<p><b>Geliştirici:</b> Oğuz YILDIZ<br>"
            "<b>E-posta:</b> <a href=\"mailto:oguzyildiz@live.com\">"
            "oguzyildiz@live.com</a></p>")
                              .arg(QString::fromLatin1(kApplicationVersion)));

        auto *closeButton = new QPushButton(QStringLiteral("Kapat"), &dialog);
        closeButton->setDefault(true);
        connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

        auto *layout = new QVBoxLayout(&dialog);
        layout->addWidget(content);
        layout->addWidget(closeButton, 0, Qt::AlignRight);
        dialog.exec();
    }

    void chooseFiles()
    {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this,
            QStringLiteral("Ses dosyalarını seçin"),
            QDir::homePath(),
            QStringLiteral("Ses dosyaları (*.m4a *.m4b *.aac *.mp4);;Tüm dosyalar (*)"));

        for (const QString &path : paths) {
            addFile(path);
        }
    }

    void addFile(const QString &path)
    {
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile()) {
            return;
        }

        const QString absolutePath = info.absoluteFilePath();
        for (int row = 0; row < m_filesList->count(); ++row) {
            if (m_filesList->item(row)->data(Qt::UserRole).toString() == absolutePath) {
                return;
            }
        }

        auto *item = new QListWidgetItem(info.fileName(), m_filesList);
        item->setData(Qt::UserRole, absolutePath);
        item->setToolTip(absolutePath);
        updateButtons();
        setStatus(QStringLiteral("%1 dosya sırada.").arg(m_filesList->count()));
    }

    void removeSelectedFiles()
    {
        const QList<QListWidgetItem *> selected = m_filesList->selectedItems();
        for (QListWidgetItem *item : selected) {
            delete m_filesList->takeItem(m_filesList->row(item));
        }
        updateButtons();
        setStatus(QStringLiteral("%1 dosya sırada.").arg(m_filesList->count()));
    }

    void clearFiles()
    {
        m_filesList->clear();
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
        updateButtons();
        setStatus(QStringLiteral("Dosya listesi temizlendi."));
    }

    void chooseOutputDirectory()
    {
        const QString current = m_outputPath->text().trimmed();
        const QString directory = QFileDialog::getExistingDirectory(
            this,
            QStringLiteral("Çıkış klasörünü seçin"),
            current.isEmpty() ? QDir::homePath() : current);
        if (!directory.isEmpty()) {
            m_outputPath->setText(directory);
        }
    }

    void startConversion()
    {
        if (m_filesList->count() == 0) {
            QMessageBox::warning(this, QStringLiteral("Dosya yok"),
                                 QStringLiteral("Önce dönüştürülecek ses dosyalarını ekleyin."));
            return;
        }

        const QString selectedDirectory = m_outputPath->text().trimmed();
        if (selectedDirectory.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Çıkış klasörü gerekli"),
                                 QStringLiteral("MP3 dosyalarının kaydedileceği klasörü seçin."));
            return;
        }
        const QString outputDirectory = QDir::cleanPath(selectedDirectory);

        const QString ffmpegPath = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
        if (ffmpegPath.isEmpty()) {
            QMessageBox::critical(
                this, QStringLiteral("FFmpeg bulunamadı"),
                QStringLiteral("FFmpeg kurulu değil veya PATH içinde bulunamadı.\n"
                               "Ubuntu/Debian üzerinde ffmpeg paketini yükleyip yeniden deneyin."));
            return;
        }

        QDir outputDir(outputDirectory);
        if (!outputDir.exists() && !QDir().mkpath(outputDirectory)) {
            QMessageBox::critical(
                this, QStringLiteral("Klasör oluşturulamadı"),
                QStringLiteral("Çıkış klasörü oluşturulamadı:\n%1").arg(outputDirectory));
            return;
        }
        if (!QFileInfo(outputDirectory).isDir() || !QFileInfo(outputDirectory).isWritable()) {
            QMessageBox::critical(
                this, QStringLiteral("Klasöre erişilemiyor"),
                QStringLiteral("Çıkış klasörü yazılabilir bir klasör değil:\n%1").arg(outputDirectory));
            return;
        }

        m_files.clear();
        for (int row = 0; row < m_filesList->count(); ++row) {
            const QString path = m_filesList->item(row)->data(Qt::UserRole).toString();
            if (!QFileInfo(path).isFile()) {
                QMessageBox::critical(
                    this, QStringLiteral("Girdi dosyası bulunamadı"),
                    QStringLiteral("Dosya artık mevcut değil:\n%1").arg(path));
                return;
            }
            m_files.append(path);
        }

        m_ffmpegPath = ffmpegPath;
        m_ffprobePath = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
        m_outputDirectory = outputDirectory;
        m_currentIndex = 0;
        m_successCount = 0;
        m_failures.clear();
        m_running = true;
        m_cancelled = false;
        m_activeOutput.clear();
        m_progressBuffer.clear();
        m_errorOutput.clear();
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
        updateButtons();
        startNextFile();
    }

    void startNextFile()
    {
        if (!m_running) {
            return;
        }
        if (m_currentIndex >= m_files.size()) {
            finishConversion();
            return;
        }

        const QString inputPath = m_files.at(m_currentIndex);
        const QFileInfo inputInfo(inputPath);
        const QString baseName = inputInfo.completeBaseName();
        QString outputPath = QDir(m_outputDirectory).filePath(baseName + QStringLiteral(".mp3"));
        int suffix = 1;
        while (QFileInfo::exists(outputPath)) {
            outputPath = QDir(m_outputDirectory).filePath(
                QStringLiteral("%1_%2.mp3").arg(baseName).arg(suffix++));
        }

        m_activeOutput = outputPath;
        m_durationUs = 0;
        m_fileProgress = 0;
        m_progressBuffer.clear();
        m_errorOutput.clear();
        updateOverallProgress();
        setStatus(QStringLiteral("Dosya %1/%2: %3")
                      .arg(m_currentIndex + 1)
                      .arg(m_files.size())
                      .arg(inputInfo.fileName()));

        if (m_ffprobePath.isEmpty()) {
            startFfmpeg();
            return;
        }

        m_isProbing = true;
        m_process.start(m_ffprobePath,
                        {QStringLiteral("-v"), QStringLiteral("error"),
                         QStringLiteral("-show_entries"), QStringLiteral("format=duration"),
                         QStringLiteral("-of"), QStringLiteral("default=noprint_wrappers=1:nokey=1"),
                         inputPath});
    }

    void startFfmpeg()
    {
        m_isProbing = false;
        m_progressBuffer.clear();
        m_errorOutput.clear();

        if (m_durationUs <= 0) {
            m_progress->setRange(0, 0);
        } else {
            m_progress->setRange(0, 100);
        }

        const QString inputPath = m_files.at(m_currentIndex);
        m_process.start(m_ffmpegPath,
                        {QStringLiteral("-nostdin"),
                         QStringLiteral("-n"),
                         QStringLiteral("-i"), inputPath,
                         QStringLiteral("-vn"),
                         QStringLiteral("-c:a"), QStringLiteral("libmp3lame"),
                         QStringLiteral("-q:a"), QStringLiteral("2"),
                         QStringLiteral("-progress"), QStringLiteral("pipe:1"),
                         QStringLiteral("-nostats"),
                         m_activeOutput});
    }

    void readProgress()
    {
        m_progressBuffer += m_process.readAllStandardOutput();
        int newline = -1;
        while ((newline = m_progressBuffer.indexOf('\n')) >= 0) {
            const QByteArray line = m_progressBuffer.left(newline).trimmed();
            m_progressBuffer.remove(0, newline + 1);

            if (m_isProbing) {
                bool ok = false;
                const double seconds = QString::fromUtf8(line).trimmed().toDouble(&ok);
                if (ok && seconds > 0.0) {
                    m_durationUs = static_cast<qint64>(seconds * 1000000.0);
                }
                continue;
            }

            if (line.startsWith("out_time_us=") || line.startsWith("out_time_ms=")) {
                const int separator = line.indexOf('=');
                bool ok = false;
                const qint64 elapsedUs = line.mid(separator + 1).toLongLong(&ok);
                if (ok && m_durationUs > 0) {
                    const double percent = static_cast<double>(elapsedUs)
                        * 100.0 / static_cast<double>(m_durationUs);
                    m_fileProgress = qBound(0, static_cast<int>(qBound(0.0, percent, 100.0)), 100);
                    updateOverallProgress();
                }
            }
        }
    }

    void processFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        if (!m_running) {
            return;
        }

        if (m_cancelled) {
            QFile::remove(m_activeOutput);
            finishConversion();
            return;
        }

        if (m_isProbing) {
            if (exitStatus == QProcess::NormalExit && exitCode == 0 && m_durationUs == 0) {
                m_progressBuffer += m_process.readAllStandardOutput();
                bool ok = false;
                const double seconds = QString::fromUtf8(m_progressBuffer).trimmed().toDouble(&ok);
                if (ok && seconds > 0.0) {
                    m_durationUs = static_cast<qint64>(seconds * 1000000.0);
                }
            }
            startFfmpeg();
            return;
        }

        m_errorOutput += m_process.readAllStandardError();
        const bool converted = exitStatus == QProcess::NormalExit
            && exitCode == 0
            && QFileInfo(m_activeOutput).isFile()
            && QFileInfo(m_activeOutput).size() > 0;

        if (converted) {
            ++m_successCount;
        } else {
            QFile::remove(m_activeOutput);
            QString detail = QString::fromLocal8Bit(m_errorOutput).trimmed();
            if (detail.isEmpty()) {
                detail = QStringLiteral("FFmpeg dönüştürmeyi tamamlayamadı.");
            }
            m_failures.append(QStringLiteral("%1: %2")
                                  .arg(QFileInfo(m_files.at(m_currentIndex)).fileName(), detail));
        }

        ++m_currentIndex;
        m_progress->setRange(0, 100);
        updateOverallProgress();
        startNextFile();
    }

    void processError(QProcess::ProcessError error)
    {
        if (!m_running || error != QProcess::FailedToStart) {
            return;
        }

        if (m_cancelled) {
            finishConversion();
            return;
        }

        if (m_isProbing) {
            startFfmpeg();
            return;
        }

        failCurrentFile(m_process.errorString());
    }

    void failCurrentFile(const QString &reason)
    {
        QFile::remove(m_activeOutput);
        m_failures.append(QStringLiteral("%1: %2")
                              .arg(QFileInfo(m_files.at(m_currentIndex)).fileName(), reason));
        ++m_currentIndex;
        m_progress->setRange(0, 100);
        updateOverallProgress();
        startNextFile();
    }

    void cancelConversion()
    {
        if (!m_running) {
            return;
        }

        m_cancelled = true;
        setStatus(QStringLiteral("İptal ediliyor…"));
        if (m_process.state() == QProcess::NotRunning) {
            QFile::remove(m_activeOutput);
            finishConversion();
        } else {
            m_process.kill();
        }
    }

    void finishConversion()
    {
        m_running = false;
        m_progress->setRange(0, 100);
        if (m_cancelled) {
            setStatus(QStringLiteral("Dönüştürme iptal edildi. Tamamlanan: %1, başarısız: %2.")
                          .arg(m_successCount)
                          .arg(m_failures.size()));
        } else if (m_failures.isEmpty()) {
            m_progress->setValue(100);
            setStatus(QStringLiteral("%1 dosya başarıyla dönüştürüldü.").arg(m_successCount));
        } else {
            m_progress->setValue(100);
            const QString summary = QStringLiteral("%1 başarılı, %2 başarısız.")
                                        .arg(m_successCount)
                                        .arg(m_failures.size());
            setStatus(summary);
            QMessageBox::warning(this, QStringLiteral("Dönüştürme tamamlandı"),
                                 summary + QStringLiteral("\n\n") + m_failures.join(QStringLiteral("\n\n")));
        }
        updateButtons();
    }

    void updateOverallProgress()
    {
        if (m_files.isEmpty()) {
            m_progress->setValue(0);
            return;
        }

        const int overall = (m_currentIndex * 100 + m_fileProgress) / m_files.size();
        m_progress->setRange(0, 100);
        m_progress->setValue(qBound(0, overall, 100));
    }

    void updateButtons()
    {
        m_filesList->setEnabled(!m_running);
        m_outputPath->setEnabled(!m_running);
        m_addButton->setEnabled(!m_running);
        m_removeButton->setEnabled(!m_running);
        m_clearButton->setEnabled(!m_running);
        m_browseButton->setEnabled(!m_running);
        m_convertButton->setEnabled(!m_running && m_filesList->count() > 0);
        m_cancelButton->setEnabled(m_running);
    }

    void setStatus(const QString &message)
    {
        m_status->setText(message);
    }

    QListWidget *m_filesList = nullptr;
    QLineEdit *m_outputPath = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_browseButton = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_convertButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QProcess m_process;
    QStringList m_files;
    QStringList m_failures;
    QString m_ffmpegPath;
    QString m_ffprobePath;
    QString m_outputDirectory;
    QString m_activeOutput;
    QByteArray m_progressBuffer;
    QByteArray m_errorOutput;
    qint64 m_durationUs = 0;
    int m_currentIndex = 0;
    int m_fileProgress = 0;
    int m_successCount = 0;
    bool m_running = false;
    bool m_cancelled = false;
    bool m_isProbing = false;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QString::fromLatin1(kApplicationName));
    app.setApplicationDisplayName(QStringLiteral("SesDönüştür — M4A'dan MP3'e"));
    app.setOrganizationName(QStringLiteral("M4AToMP3"));
    app.setApplicationVersion(QString::fromLatin1(kApplicationVersion));
    app.setWindowIcon(createApplicationIcon());

    MainWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"
