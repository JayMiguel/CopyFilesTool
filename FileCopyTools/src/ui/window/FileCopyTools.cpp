#include "FileCopyTools.h"
#include <QDebug>

FileCopyTools::FileCopyTools(QWidget *parent) : QMainWindow(parent)
{
	this->setWindowTitle(tr("GD Twincity 多源数据轻量化工具"));
	this->setWindowIcon(QIcon(":/logo.png"));
	this->setFixedSize(800, 600);
	
	// 中心控件
	this->m_centralWidget = new QWidget(this);
	
	// 成员组件
	this->m_layout = new QGridLayout(m_centralWidget);

	this->m_btnLoadFile = new QPushButton(tr("载入文件"));
	connect(m_btnLoadFile, &QPushButton::clicked, this, &FileCopyTools::onLoadFileBtnClicked);
	
	this->m_dirName = new QLineEdit;
	this->m_dirName->setReadOnly(true);

	this->m_btnStart = new QPushButton(tr("开始处理"));
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

	// 布局
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
		QMessageBox::warning(this, tr("提示"), tr("日志文件不存在"));
		return;
	}

	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, tr("提示"), tr("日志文件打开失败"));
		return;
	}

	QString content = file.readAll();
	m_logMsg = content.split("\n");
}

void FileCopyTools::onLoadFileBtnClicked()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("选择文件夹"), QDir::homePath());
	if (dirName.isEmpty())
	{
		QMessageBox::warning(this, tr("提示"), tr("未选择文件夹"));
		return;
	}

	this->m_dirName->setText(dirName);
	this->m_progress->setValue(0);
}

/**
 * @brief 启动计时器和文件复制线程
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
 * @brief 更新进度条
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
 * @brief 输出日志
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
 * @brief 递归读取目录中的文件
 * @author pjm
 * @param path
 */
void FileCopyTools::ListFilesRecursively(const QString& path)
{
	QDir dir(path);
	dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

	// 获取当前目录下的所有文件
	QFileInfoList fileInfoList = dir.entryInfoList();

	foreach(const QFileInfo& fileInfo, fileInfoList) {
		// 输出文件的绝对路径
		m_fileList << fileInfo.absoluteFilePath();
	}

	// 获取当前目录下的所有子目录
	QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	foreach(const QString& subDir, subDirs) {
		// 递归遍历子目录
		ListFilesRecursively(dir.filePath(subDir));
	}
}

/**
 * @brief 把当前路径下的temp目录中的文件复制到选择的文件夹
 * @author pjm
 */
void FileCopyTools::onProcessResult()
{
	QDateTime t1 = QDateTime::currentDateTime();
	qDebug() << "开始复制文件：" << t1.toString();

	m_fileList.clear();

	// 基础目录名
	QDir selectedDir(m_dirName->text());
	QString baseDirName = selectedDir.dirName();
	
	// 目标目录
	selectedDir.cdUp();
	QString targetDirPath = selectedDir.absolutePath();

	// 计算源目录路径
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

	// 获取源目录所有文件路径
	ListFilesRecursively(sourceDirPath);

	m_completedFiles = 0;
	m_fileCount = m_fileList.size();
	for (QString& fileName : m_fileList)
	{
		// 源文件
		QString sourcefilePath = fileName;
		QFileInfo sourceFileInfo(fileName);

		// 从路径字符串中获取文件名，并拼接完整路径
		QString destinationFilePath = targetDirPath + sourcefilePath.remove(tempDir.absolutePath());
		// 确保目标目录已存在
		QFileInfo fileInfo(destinationFilePath);
		QDir dest(fileInfo.absolutePath());
		if (!dest.exists()) {
			if (!dest.mkpath(fileInfo.absolutePath())) {
				return;
			}
		}

		// 执行复制操作
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
	qDebug() << "文件复制完成：" << t2.toString();

	QMessageBox::warning(this, tr("提示"), tr("轻量化处理成功！"));

	this->m_btnLoadFile->setDisabled(false);
	this->m_btnStart->setDisabled(false);
}
