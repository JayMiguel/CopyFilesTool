#pragma once
#pragma execution_character_set("utf-8")
#include <QMainWindow>
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QTimer>
#include <QRandomGenerator>
#include <QDateTime>
#include <QThread>

class FileCopyTools : public QMainWindow
{
    Q_OBJECT

public:
    FileCopyTools(QWidget *parent = nullptr);
    ~FileCopyTools();

private:
    void loadLogFile();

signals:
    void processResult();
    void copyFinished();
    void processSuccess();


private slots:
    void onLoadFileBtnClicked();
    void startTimer();
    void updateProgressBar();
    void updateLogText();
    void checkStatus();
    void onProcessResult();
    void onCopyFinished();
    void onProcessSuccess();

private:
    void ListFilesRecursively(const QString& path);

private:
    QStringList m_logMsg;
    int m_currentLogIndex;

    QWidget* m_centralWidget;
    QGridLayout* m_layout;
    QPushButton* m_btnLoadFile;
    QLineEdit* m_dirName;
    QPushButton* m_btnStart;
    QProgressBar* m_progress;
    QTextEdit* m_textEditLog;
    QTimer* m_progTimer;
    QTimer* m_logTimer;
    QTimer* m_completeTimer;

    QStringList m_fileList;
    int m_fileCount;
    int m_completedFiles;
};
