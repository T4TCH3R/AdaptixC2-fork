#include <UI/Widgets/BrowserFilesWidget.h>
#include <Utils/FileSystem.h>
#include <QFileIconProvider>

void BrowserFileData::CreateBrowserFileData(QString path, int type )
{
    Fullpath = path;
    Type     = type;
    Name     = GetBasenameWindows(path);

    TreeItem = new FileBrowserTreeItem(this);

    QFileIconProvider iconProvider;
    if ( type == TYPE_FILE ) {
        TreeItem->setIcon(0, iconProvider.icon(QFileIconProvider::File));
    }
    else if ( type == TYPE_DISK) {
        TreeItem->setIcon(0, iconProvider.icon(QFileIconProvider::Drive));
    }
    else {
        TreeItem->setIcon(0, iconProvider.icon(QFileIconProvider::Folder));
    }
}

void BrowserFileData::SetType(int type)
{
    this->Type = type;

    QFileIconProvider iconProvider;
    if (type == TYPE_FILE)
        this->TreeItem->setIcon(0, iconProvider.icon(QFileIconProvider::File));
    else if (type == TYPE_DISK)
        this->TreeItem->setIcon(0, iconProvider.icon(QFileIconProvider::Drive));
    else
        this->TreeItem->setIcon(0, iconProvider.icon(QFileIconProvider::Folder));
}



BrowserFilesWidget::BrowserFilesWidget(Agent* a)
{
    agent = a;
    this->createUI();

    connect(buttonDisks,  &QPushButton::clicked,     this, &BrowserFilesWidget::onDisks);
    connect(buttonList,   &QPushButton::clicked,     this, &BrowserFilesWidget::onList);
    connect(buttonParent, &QPushButton::clicked,     this, &BrowserFilesWidget::onParent);
    connect(buttonReload, &QPushButton::clicked,     this, &BrowserFilesWidget::onReload);
    connect(inputPath,    &QLineEdit::returnPressed, this, &BrowserFilesWidget::onList);

    connect(tableWidget,       &QTableWidget::doubleClicked,    this, &BrowserFilesWidget::handleTableDoubleClicked);
    connect(treeBrowserWidget, &QTreeWidget::itemDoubleClicked, this, &BrowserFilesWidget::handleTreeDoubleClicked);
}

BrowserFilesWidget::~BrowserFilesWidget() = default;

void BrowserFilesWidget::createUI()
{
    buttonReload = new QPushButton(QIcon(":/icons/reload"), "", this);
    buttonReload->setIconSize( QSize( 24,24 ));
    buttonReload->setFixedSize(37, 28);
    buttonReload->setToolTip("Reload");

    buttonParent = new QPushButton(QIcon(":/icons/folder"), "", this);
    buttonParent->setIconSize(QSize(24, 24 ));
    buttonParent->setFixedSize(37, 28);
    buttonParent->setToolTip("Up folder");

    inputPath = new QLineEdit(this);

    buttonList = new QPushButton(QIcon(":/icons/arrow_right"), "", this);
    buttonList->setIconSize(QSize(24, 24 ));
    buttonList->setFixedSize(37, 28);

    line_1 = new QFrame(this);
    line_1->setFrameShape(QFrame::VLine);
    line_1->setMinimumHeight(25);

    buttonDisks = new QPushButton(QIcon(":/icons/storage"), "", this);
    buttonDisks->setIconSize( QSize( 24,24 ));
    buttonDisks->setFixedSize(37, 28);
    buttonDisks->setToolTip("Disks list");

    buttonUpload = new QPushButton(QIcon(":/icons/upload"), "", this);
    buttonUpload->setIconSize( QSize( 24,24 ));
    buttonUpload->setFixedSize(37, 28);
    buttonUpload->setToolTip("Upload File");

    line_2 = new QFrame(this);
    line_2->setFrameShape(QFrame::VLine);
    line_2->setMinimumHeight(25);

    statusLabel = new QLabel(this);
    statusLabel->setText("Status: ");

    tableWidget = new QTableWidget(this );
    tableWidget->setContextMenuPolicy( Qt::CustomContextMenu );
    tableWidget->setAutoFillBackground( false );
    tableWidget->setShowGrid( false );
    tableWidget->setSortingEnabled( false );
    tableWidget->setWordWrap( true );
    tableWidget->setCornerButtonEnabled( true );
    tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
    tableWidget->setFocusPolicy( Qt::NoFocus );
    tableWidget->setAlternatingRowColors( true );
    tableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
    tableWidget->horizontalHeader()->setCascadingSectionResizes( true );
    tableWidget->horizontalHeader()->setHighlightSections( false );
    tableWidget->verticalHeader()->setVisible( false );
    tableWidget->setColumnCount(3);
    tableWidget->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Name" ) );
    tableWidget->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Size" ) );
    tableWidget->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Last Modified" ) );

    listGridLayout = new QGridLayout(this);
    listGridLayout->setContentsMargins(5, 4, 1, 1);
    listGridLayout->setVerticalSpacing(4);
    listGridLayout->setHorizontalSpacing(8);

    listGridLayout->addWidget( buttonReload,  0, 0,  1, 1  );
    listGridLayout->addWidget(buttonParent, 0, 1, 1, 1  );
    listGridLayout->addWidget(inputPath, 0, 2, 1, 5  );
    listGridLayout->addWidget(buttonList, 0, 7, 1, 1  );
    listGridLayout->addWidget( line_1,        0, 8,  1, 1  );
    listGridLayout->addWidget( buttonUpload,  0, 9,  1, 1  );
    listGridLayout->addWidget( buttonDisks,   0, 10, 1, 1  );
    listGridLayout->addWidget( line_2,        0, 11, 1, 1  );
    listGridLayout->addWidget( statusLabel,   0, 12, 1, 1  );
    listGridLayout->addWidget( tableWidget,   1, 0,  1, 13 );

    listBrowserWidget = new QWidget(this);
    listBrowserWidget->setLayout(listGridLayout);

    treeBrowserWidget = new QTreeWidget();
    treeBrowserWidget->setSortingEnabled(false);
    treeBrowserWidget->headerItem()->setText( 0, "Directory Tree" );

    splitter = new QSplitter( this );
    splitter->setOrientation( Qt::Horizontal );
    splitter->addWidget( treeBrowserWidget );
    splitter->addWidget( listBrowserWidget );
    splitter->setSizes( QList<int>() << 1 << 250 );

    mainGridLayout = new QGridLayout(this );
    mainGridLayout->setContentsMargins(0, 0, 0, 0 );
    mainGridLayout->addWidget(splitter, 0, 0, 1, 1 );

    this->setLayout(mainGridLayout);
}

void BrowserFilesWidget::SetDisks(qint64 time, int msgType, QString message, QString data)
{
    QString sTime  = UnixTimestampGlobalToStringLocal(time);
    QString status;
    if( msgType == CONSOLE_OUT_LOCAL_ERROR || msgType == CONSOLE_OUT_ERROR ) {
        status = TextColorHtml(message, COLOR_ChiliPepper) + " >> " + sTime;
    }
    else {
        status = TextColorHtml(message, COLOR_NeonGreen) + " >> " + sTime;
    }

    statusLabel->setText(status);

    QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    if (!jsonDoc.isArray())
        return;

    QVector<BrowserFileData> disks;

    QJsonArray jsonArray = jsonDoc.array();
    for (const QJsonValue& value : jsonArray) {
        QJsonObject jsonObject = value.toObject();
        QString path = jsonObject["b_name"].toString();
        path = path.toLower();

        BrowserFileData* diskData = getFileData(path);
        diskData->Size = jsonObject["b_type"].toString();
        disks.push_back(*diskData);
    }
    this->tableShowItems(disks);

    curentPath = "";
    inputPath->setText(curentPath);
}

void BrowserFilesWidget::AddFiles(qint64 time, int msgType, QString message, QString path, QString data)
{
    QString sTime  = UnixTimestampGlobalToStringLocal(time);
    QString status;
    if( msgType == CONSOLE_OUT_LOCAL_ERROR || msgType == CONSOLE_OUT_ERROR ) {
        status = TextColorHtml(message, COLOR_ChiliPepper) + " >> " + sTime;
        statusLabel->setText(status);
        return;
    }
    else {
        status = TextColorHtml(message, COLOR_NeonGreen) + " >> " + sTime;
        statusLabel->setText(status);
    }

    path = path.toLower();

    BrowserFileData* currenFileData = this->getFileData(path);
    currenFileData->Stored = true;
    currenFileData->Status = status;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    if (!jsonDoc.isArray())
        return;

    QJsonArray jsonArray = jsonDoc.array();
    this->updateFileData(currenFileData, path, jsonArray);
    this->tableShowItems(currenFileData->Files);

    treeBrowserWidget->setCurrentItem(currenFileData->TreeItem);
    currenFileData->TreeItem->setExpanded(true);

    curentPath = path;
    inputPath->setText(curentPath);
}

/// PRIVATE

BrowserFileData BrowserFilesWidget::createFileData(QString path)
{
    int type = TYPE_DIR;
    QString rootPath = GetRootPathWindows(path);
    if(rootPath == path)
        type = TYPE_DISK;

    BrowserFileData fileData;
    fileData.CreateBrowserFileData(path, type);

    if( type == TYPE_DISK )
        treeBrowserWidget->addTopLevelItem(fileData.TreeItem);

    return fileData;
}

BrowserFileData* BrowserFilesWidget::getFileData(QString path)
{
    if( browserStore.contains(path) ) {
        return &browserStore[path];
    }
    else {
        QString rootPath = GetRootPathWindows(path);
        BrowserFileData fileData = this->createFileData(path);
        browserStore[path] = fileData;
        if ( rootPath == path )
            return &browserStore[path];

        QString parentPath = GetParentPathWindows(path);
        BrowserFileData* parentFileData = this->getFileData(parentPath);

        parentFileData->TreeItem->addChild(fileData.TreeItem);

        return &browserStore[path];
    }
}

void BrowserFilesWidget::updateFileData(BrowserFileData* currenFileData, QString path, QJsonArray jsonArray)
{
    QMap<QString, BrowserFileData> oldFiles;
    for (BrowserFileData oldData : currenFileData->Files)
        oldFiles[oldData.Name] = oldData;

    currenFileData->TreeItem->takeChildren();
    currenFileData->Files.clear();

    for ( QJsonValue value : jsonArray ) {
        QJsonObject jsonObject = value.toObject();
        QString filename = jsonObject["b_filename"].toString();
        filename = filename.toLower();

        qint64  b_size = jsonObject["b_size"].toDouble();
        qint64  b_date = jsonObject["b_date"].toDouble();
        int     b_type = jsonObject["b_is_dir"].toBool();

        QString fullname = path + "\\" + filename;

        BrowserFileData* childData = this->getFileData(fullname);

        childData->Modified = UnixTimestampGlobalToStringLocalFull(b_date);

        if ( b_type == TYPE_FILE ) {
            childData->Size = BytesToFormat(b_size);
            childData->SetType(b_type);
        }

        if( oldFiles.contains(filename) ) {
            oldFiles.remove(filename);
        }
        else {
            childData->SetType(b_type);
            if ( b_type != TYPE_FILE )
                browserStore[fullname] = *childData;
        }

        currenFileData->TreeItem->addChild(childData->TreeItem);
        currenFileData->Files.push_back(*childData);
    }

    for( QString oldPath : oldFiles.keys() ) {
        BrowserFileData data = oldFiles[oldPath];

        QString oldFullpath = data.Fullpath + "\\";

        for(QString storeKey : browserStore.keys())
            if (storeKey.startsWith(oldFullpath, Qt::CaseInsensitive))
                browserStore.remove(storeKey);

        browserStore.remove(data.Fullpath);
        oldFiles.remove(oldPath);
    }
}

void BrowserFilesWidget::setStoredFileData(QString path, BrowserFileData currenFileData)
{

    treeBrowserWidget->setCurrentItem(currenFileData.TreeItem);
    currenFileData.TreeItem->setExpanded(true);

    this->tableShowItems(currenFileData.Files);

    statusLabel->setText( currenFileData.Status );

    curentPath = path;
    inputPath->setText(curentPath);
}

void BrowserFilesWidget::tableShowItems(QVector<BrowserFileData> files )
{
    for (int index = tableWidget->rowCount(); index > 0; index-- )
        tableWidget->removeRow(index -1 );

    tableWidget->setRowCount(files.size());
    for (int row = 0; row < files.size(); ++row) {
        QTableWidgetItem* item_Name = new QTableWidgetItem(files[row].Name);
        QTableWidgetItem* item_Size = new QTableWidgetItem(files[row].Size);
        QTableWidgetItem* item_Date = new QTableWidgetItem(files[row].Modified);

        item_Name->setFlags( item_Name->flags() ^ Qt::ItemIsEditable );
        item_Size->setFlags( item_Size->flags() ^ Qt::ItemIsEditable );
        item_Date->setFlags( item_Date->flags() ^ Qt::ItemIsEditable );

        QFileIconProvider iconProvider;
        if ( files[row].Type == TYPE_FILE )
            item_Name->setIcon(iconProvider.icon(QFileIconProvider::File));
        else if ( files[row].Type == TYPE_DISK)
            item_Name->setIcon(iconProvider.icon(QFileIconProvider::Drive));
        else
            item_Name->setIcon(iconProvider.icon(QFileIconProvider::Folder));

        tableWidget->setItem(row, 0, item_Name);
        tableWidget->setItem(row, 1, item_Size);
        tableWidget->setItem(row, 2, item_Date);
    }
}

void BrowserFilesWidget::cdBroser(QString path)
{
    if ( !browserStore.contains(path) )
        return;

    BrowserFileData fileData = browserStore[path];
    if (fileData.Type == TYPE_FILE)
        return;

    if (fileData.Stored) {
        this->setStoredFileData(path, fileData);
    } else {
        QString status = agent->BrowserList(path);
        statusLabel->setText(status);
    }
}



/// SLOTS

void BrowserFilesWidget::onDisks()
{
    QString status = agent->BrowserDisks();
    statusLabel->setText(status);
}

void BrowserFilesWidget::onList()
{
    QString path = inputPath->text();
    QString status = agent->BrowserList(path);
    statusLabel->setText(status);
}

void BrowserFilesWidget::onParent()
{
    QString path = GetParentPathWindows(curentPath);
    if (path == curentPath)
        return;

    this->cdBroser(path);
}

void BrowserFilesWidget::onReload()
{
    if ( !curentPath.isEmpty() ){
        QString status = agent->BrowserList(curentPath);
        statusLabel->setText(status);
    }
}

void BrowserFilesWidget::handleTableDoubleClicked(const QModelIndex &index)
{
    QString filename = tableWidget->item(index.row(),0)->text();

    QString path = curentPath + "\\" + filename;
    if(curentPath.isEmpty())
        path = filename;

    this->cdBroser(path);
}

void BrowserFilesWidget::handleTreeDoubleClicked(QTreeWidgetItem* item, int column)
{
    FileBrowserTreeItem* treeItem = (FileBrowserTreeItem*) item;

    QString path = treeItem->Data.Fullpath;
    if ( path == curentPath )
        return;

    this->cdBroser(path);
}