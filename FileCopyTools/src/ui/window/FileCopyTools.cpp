#include "FileCopyTools.h"
#include <QDebug>

FileCopyTools::FileCopyTools(QWidget *parent) : QMainWindow(parent)
{
	this->setWindowTitle(tr("GD Twincity ��Դ��������������"));
	this->setWindowIcon(QIcon(":/logo.png"));
	this->setFixedSize(800, 600);
	
	// ���Ŀؼ�
	this->m_centralWidget = new QWidget(this);
	
	// ��Ա���
	this->m_layout = new QGridLayout(m_centralWidget);

	this->m_btnLoadFile = new QPushButton(tr("�����ļ�"));
	connect(m_btnLoadFile, &QPushButton::clicked, this, &FileCopyTools::onLoadFileBtnClicked);
	
	this->m_dirName = new QLineEdit;
	this->m_dirName->setReadOnly(true);

	this->m_btnStart = new QPushButton(tr("��ʼ����"));
	connect(m_btnStart, &QPushButton::clicked, this, &FileCopyTools::startTimer);

	this->m_progress = new QProgressBar;
	this->m_progress->setRange(0, 100);
	this->m_progress->setFormat("%p%");
	this->m_progress->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	this->m_textEditLog = new QTextEdit;
	this->m_textEditLog->setReadOnly(true);

	this->m_progTimer = new QTimer(this);
	connect(m_progTimer, &QTimer::timeout, this, &FileCopyTools::updateProgressBar);
	
	this->m_logTimer = new QTimer(this);
	connect(m_logTimer, &QTimer::timeout, this, &FileCopyTools::updateLogText);

	connect(this, &FileCopyTools::processResult, this, &FileCopyTools::onProcessResult);
	connect(this, &FileCopyTools::copyFinished, this, &FileCopyTools::onCopyFinished);
	connect(this, &FileCopyTools::processSuccess, this, &FileCopyTools::onProcessSuccess);

	this->m_completeTimer = new QTimer(this);
	connect(m_completeTimer, &QTimer::timeout, this, &FileCopyTools::checkStatus);

	// ����
	this->m_layout->addWidget(m_btnLoadFile, 0, 0);
	this->m_layout->addWidget(m_dirName, 0, 1, 1, 4);
	this->m_layout->addWidget(m_btnStart, 0, 5);
	this->m_layout->addWidget(m_progress, 1, 0, 1, 6);
	this->m_layout->addWidget(m_textEditLog, 2, 0, 3, 6);

	this->setCentralWidget(m_centralWidget);

	this->loadLogFile();
}

FileCopyTools::~FileCopyTools()
{

}

void FileCopyTools::loadLogFile()
{
	QFile file("./log/log.txt");
	if (!file.exists())
	{
		QMessageBox::warning(this, tr("��ʾ"), tr("��־�ļ�������"));
		return;
	}

	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, tr("��ʾ"), tr("��־�ļ���ʧ��"));
		return;
	}

	QString content = file.readAll();
	m_logMsg = content.split("\n");
}

void FileCopyTools::onLoadFileBtnClicked()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("ѡ���ļ���"), QDir::homePath());
	if (dirName.isEmpty())
	{
		QMessageBox::warning(this, tr("��ʾ"), tr("δѡ���ļ���"));
		return;
	}

	this->m_dirName->setText(dirName);
	this->m_progress->setValue(0);
}

/**
 * @brief ������ʱ�����ļ������߳�
 * @author pjm
 */
void FileCopyTools::startTimer()
{
	this->m_btnLoadFile->setDisabled(true);
	this->m_btnStart->setDisabled(true);

	this->m_progress->setValue(0);
	this->m_progTimer->start(1000);

	this->m_textEditLog->clear();
	this->m_currentLogIndex = -1;
	this->m_logTimer->start(1000);

	this->m_completeTimer->start(1000);

	//emit processResult();
	QThread* worker = QThread::create([this]() {this->onProcessResult(); });
	worker->start();
}

/**
 * @brief ���½�����
 * @author pjm
 */
void FileCopyTools::updateProgressBar()
{
	//int value = std::round((double)m_completedFiles / (double)m_fileCount * 100);
	//this->m_progress->setValue(value);
	//if (value >= this->m_progress->maximum())
	//{
	//	this->m_progTimer->stop();
	//}

	int updateValue = QRandomGenerator::global()->bounded(6, 8);
	if (this->m_progress->value() + updateValue >= this->m_progress->maximum())
	{
		this->m_progress->setValue(100);
	}
	else
	{
		this->m_progress->setValue(this->m_progress->value() + updateValue);
	}
	if (this->m_progress->value() >= this->m_progress->maximum())
	{
		this->m_progTimer->stop();
	}
}

/**
 * @brief �����־
 * @author pjm
 */
void FileCopyTools::updateLogText()
{
	int logIndex = QRandomGenerator::global()->bounded(0, this->m_logMsg.size() - 5);
	if (logIndex < 0 || logIndex == m_currentLogIndex)
	{
		return;
	}

	for (int i = 0; i < 5; i++)
	{
		this->m_textEditLog->append(this->m_logMsg[logIndex + i] + "\n");
		if (this->m_progress->value() == this->m_progress->maximum())
		{
			this->m_logTimer->stop();
			break;
		}
	}
}

void FileCopyTools::checkStatus()
{
	if (m_completedFiles == m_fileCount && m_progress->value() == m_progress->maximum() && !m_logTimer->isActive())
	{
		m_completeTimer->stop();
		emit processSuccess();
	}
}

/**
 * @brief �ݹ��ȡĿ¼�е��ļ�
 * @author pjm
 * @param path
 */
void FileCopyTools::ListFilesRecursively(const QString& path)
{
	QDir dir(path);
	dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

	// ��ȡ��ǰĿ¼�µ������ļ�
	QFileInfoList fileInfoList = dir.entryInfoList();

	foreach(const QFileInfo& fileInfo, fileInfoList) {
		// ����ļ��ľ���·��
		m_fileList << fileInfo.absoluteFilePath();
	}

	// ��ȡ��ǰĿ¼�µ�������Ŀ¼
	QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	foreach(const QString& subDir, subDirs) {
		// �ݹ������Ŀ¼
		ListFilesRecursively(dir.filePath(subDir));
	}
}

/**
 * @brief �ѵ�ǰ·���µ�tempĿ¼�е��ļ����Ƶ�ѡ����ļ���
 * @author pjm
 */
void FileCopyTools::onProcessResult()
{
	QDateTime t1 = QDateTime::currentDateTime();
	qDebug() << "��ʼ�����ļ���" << t1.toString();

	m_fileList.clear();

	// ����Ŀ¼��
	QDir selectedDir(m_dirName->text());
	QString baseDirName = selectedDir.dirName();
	
	// Ŀ��Ŀ¼
	selectedDir.cdUp();
	QString targetDirPath = selectedDir.absolutePath();

	// ����ԴĿ¼·��
	QString tempPath = "./temp";
	QDir tempDir(tempPath);
	QString sourceDirName;
	QStringList subDirs = tempDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	foreach(const QString & subDir, subDirs)
	{
		if (subDir.startsWith(baseDirName))
		{
			sourceDirName = subDir;
		}
	}
	QString sourceDirPath = tempDir.absolutePath() + "/" + sourceDirName;

	// ��ȡԴĿ¼�����ļ�·��
	ListFilesRecursively(sourceDirPath);

	m_completedFiles = 0;
	m_fileCount = m_fileList.size();
	for (QString& fileName : m_fileList)
	{
		// Դ�ļ�
		QString sourcefilePath = fileName;
		QFileInfo sourceFileInfo(fileName);

		// ��·���ַ����л�ȡ�ļ�������ƴ������·��
		QString destinationFilePath = targetDirPath + sourcefilePath.remove(tempDir.absolutePath());
		// ȷ��Ŀ��Ŀ¼�Ѵ���
		QFileInfo fileInfo(destinationFilePath);
		QDir dest(fileInfo.absolutePath());
		if (!dest.exists()) {
			if (!dest.mkpath(fileInfo.absolutePath())) {
				return;
			}
		}

		// ִ�и��Ʋ���
		if (QFile::copy(fileName, destinationFilePath)) {
			emit copyFinished();
		}
	}
}

void FileCopyTools::onCopyFinished()
{
	m_completedFiles++;
}

void FileCopyTools::onProcessSuccess()
{
	QDateTime t2 = QDateTime::currentDateTime();
	qDebug() << "�ļ�������ɣ�" << t2.toString();

	QMessageBox::warning(this, tr("��ʾ"), tr("����������ɹ���"));

	this->m_btnLoadFile->setDisabled(false);
	this->m_btnStart->setDisabled(false);
}
