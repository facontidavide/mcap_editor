#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <set>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableTopics->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile()
{
    QSettings settings;

    QString dir = settings.value("MainWindow.lastDirectoryLoad",
                                 QDir::currentPath()).toString();

    auto filename = QFileDialog::getOpenFileName(
        this, "Open a MCAP file", dir, "MCAP files (*.mcap)");

    if(!filename.isEmpty())
    {
        dir = QFileInfo(filename).absolutePath();
        settings.setValue("MainWindow.lastDirectoryLoad", dir);

        auto res = reader_.open(filename.toStdString());
        if(!res.ok())
        {
            QMessageBox::warning(this, "Error opening file",
                                 QString::fromStdString(res.message));
            return;
        }
        readMCAP();
    }
}


void MainWindow::openFileWASM()
{
    auto fileContentReady = [this](const QString &fileName,
                                   const QByteArray &fileContent) {
        if (!fileName.isEmpty())
        {
            mcap::BufferReader buffer;
            buffer.reset(reinterpret_cast<const std::byte*>(fileContent.data()),
                         fileContent.size(), fileContent.size());
            auto res = reader_.open(buffer);
            if(!res.ok())
            {
                QMessageBox::warning(this, "Error opening file",
                                     QString::fromStdString(res.message));
                return;
            }

            readMCAP();
        }
    };

    QFileDialog::getOpenFileContent("MCAP files (*.mcap)",
                                    fileContentReady);
}

void MainWindow::on_buttonLoad_clicked()
{
    openFile();
}

void MainWindow::readMCAP()
{
    ui->lineProfile->setText({});

    ui->tableTopics->clearContents();
    ui->tableTopics->model()->removeRows(0, ui->tableTopics->rowCount());
    ui->tableTopics->setRowCount(0);

    schema_by_id_.clear();
    schema_id_by_channel_.clear();
    channel_encoding_.clear();
    ui->widgetSave->setEnabled(false);

    auto status = reader_.readSummary(mcap::ReadSummaryMethod::ForceScan);

    if(!status.ok())
    {
        QMessageBox::warning(this, "Error opening file",
                             QString::fromStdString(status.message));
        return;
    }

    auto stats_pt = reader_.statistics();
    if (!stats_pt)
    {
        QMessageBox::warning(this, "Error opening file",
                             "Can't read file statistics");
        return;
    }
    ui->lineProfile->setText(QString::fromStdString(reader_.header()->profile));

    const auto& statistics = stats_pt.value();
    time_start_ = statistics.messageStartTime;
    time_end_ = statistics.messageEndTime;

    for (const auto& [schema_id, schema] : reader_.schemas())
    {
        std::string schema_str(reinterpret_cast<const char*>(schema->data.data()),
                               schema->data.size());

        SchemaInfo info = {schema->name, schema->encoding, schema_str};
        schema_by_id_[schema_id] = info;
    }

    for (const auto& [channel_id, channel] : reader_.channels())
    {
        const auto schema = reader_.schemas().at(channel->schemaId);
        const auto channel_name = QString::fromStdString(channel->topic);
        const auto schema_name = QString::fromStdString(schema->name);
        const auto encoding = QString::fromStdString(channel->messageEncoding);
        uint64_t msg_count = 0;
        {
            auto it =  statistics.channelMessageCounts.find(channel_id);
            if( it != statistics.channelMessageCounts.end())
            {
                msg_count = it->second;
            }
        }

        const int row = ui->tableTopics->rowCount();
        ui->tableTopics->insertRow (row);
        auto channel_item = new QTableWidgetItem(channel_name);
        channel_item->setCheckState(Qt::Checked);
        ui->tableTopics->setItem(row, 0, channel_item);
        ui->tableTopics->setItem(row, 1, new QTableWidgetItem(schema_name));
        ui->tableTopics->setItem(row, 2, new QTableWidgetItem(encoding));
        ui->tableTopics->setItem(row, 3, new QTableWidgetItem(QString::number(msg_count)));

        schema_id_by_channel_[channel->topic] = channel->schemaId;
        channel_encoding_[channel->topic] = channel->messageEncoding;
    }

    auto start_date = QDateTime::fromMSecsSinceEpoch(time_start_ / 1000000);
    auto end_date = QDateTime::fromMSecsSinceEpoch(time_end_ / 1000000);

    for(auto* edit: {ui->dateTimeStart, ui->dateTimeStartNew,
                     ui->dateTimeEnd, ui->dateTimeEndNew})
    {
        if(start_date.date() < QDate(1999, 1, 1))
        {
            edit->setTimeSpec(Qt::TimeSpec::UTC);
            edit->setDisplayFormat("H:mm:ss");
        }
        else {
            edit->setTimeSpec(Qt::TimeSpec::LocalTime);
            edit->setDisplayFormat("yyyy/M/d - H:mm:ss");
        }
    }

    ui->dateTimeStart->setDateTime(start_date);
    ui->dateTimeEnd->setDateTime(end_date);
    on_buttonResetTimeRange_clicked();

    auto horizontalHeader = ui->tableTopics->horizontalHeader();
    horizontalHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    horizontalHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    ui->widgetSave->setEnabled(true);
}

void MainWindow::on_buttonSelect_clicked()
{
    for(int row=0; row<ui->tableTopics->rowCount(); row++)
    {
        ui->tableTopics->item(row, 0)->setCheckState(Qt::Checked);
    }
}


void MainWindow::on_buttonDeselect_clicked()
{
    for(int row=0; row<ui->tableTopics->rowCount(); row++)
    {
        ui->tableTopics->item(row, 0)->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::on_buttonResetTimeRange_clicked()
{
    ui->dateTimeStartNew->setDateTime( ui->dateTimeStart->dateTime() );
    ui->dateTimeEndNew->setDateTime( ui->dateTimeEnd->dateTime() );
}

void MainWindow::on_tableTopics_itemSelectionChanged()
{
    QModelIndexList selection = ui->tableTopics->selectionModel()->selectedRows();

    ui->textSchema->setPlainText("");

    if(selection.count() == 1)
    {
        QModelIndex index = selection.front();
        auto channel = ui->tableTopics->item(index.row(), 0)->text();
        auto it = schema_id_by_channel_.find(channel.toStdString());
        if( it != schema_id_by_channel_.end())
        {
            auto schema = QString::fromStdString(schema_by_id_.at(it->second).text);
            ui->textSchema->setPlainText(schema);
        }
    }
}


void MainWindow::on_buttonSave_clicked()
{
    QSettings settings;

    QString dir = settings.value("MainWindow.lastDirectorySave",
                                 QDir::currentPath()).toString();

    auto filename = QFileDialog::getSaveFileName(
        this, "Open a MCAP file", dir, "MCAP files (*.mcap)");

    if(filename.isEmpty())
    {
        return;
    }
    dir = QFileInfo(filename).absolutePath();
    settings.setValue("MainWindow.lastDirectorySave", dir);

    std::set<std::string> select_topics;

    for(int row=0; row<ui->tableTopics->rowCount(); row++)
    {
        auto item = ui->tableTopics->item(row, 0);
        if(item->checkState() == Qt::Checked)
        {
            select_topics.insert(item->text().toStdString());
        }
    }
    // Read and write loop
    mcap::McapWriter writer;
    mcap::McapWriterOptions options(reader_.header()->profile);
    if(ui->radioLZ4->isChecked()) {
        options.compression = mcap::Compression::Lz4;
    }
    else if(ui->radioZSTD->isChecked()) {
        options.compression = mcap::Compression::Zstd;
    }
    else {
        options.compression = mcap::Compression::None;
    }

    auto status = writer.open(filename.toStdString(), options);
    if (!status.ok())
    {
        QMessageBox::warning(this, "Error opening file",
                             "Can't open the file for writing");
        return;
    }
    saveMCAP(reader_, writer, select_topics);
}


void MainWindow::saveMCAP(mcap::McapReader& reader,
                          mcap::McapWriter& writer,
                          const std::set<std::string>& channels)
{
    std::map<mcap::SchemaId, mcap::SchemaId> old_to_new_schema_id;
    std::map<std::string, mcap::ChannelId> channel_ids_;

    for(const auto& channel_name: channels)
    {
        auto old_id = schema_id_by_channel_.at(channel_name);
        auto it = old_to_new_schema_id.find(old_id);
        // add if missing
        if( it == old_to_new_schema_id.end()) {
            const auto& schema = schema_by_id_.at(old_id);
            mcap::Schema mcap_schema(schema.name, schema.encoding, schema.text);
            writer.addSchema(mcap_schema);
            it = old_to_new_schema_id.insert({old_id, mcap_schema.id}).first;
        }
        mcap::SchemaId new_schema_id = it->second;

        mcap::Channel channel(channel_name,
                              channel_encoding_.at(channel_name),
                              new_schema_id);
        writer.addChannel(channel);
        channel_ids_.insert({channel.topic, channel.id});
    }
    mcap::ReadMessageOptions options;
    mcap::ProblemCallback problem = [](const mcap::Status&) {};
    options.topicFilter = [&channels](std::string_view name) -> bool
    {
        return channels.count(std::string(name)) != 0;
    };

    for (const auto& msg : reader.readMessages(problem, options))
    {
        auto new_channel_id = channel_ids_.at(msg.channel->topic);

        mcap::Message new_msg = msg.message;
        new_msg.channelId = new_channel_id;
        auto status = writer.write(new_msg);
        if (!status.ok())
        {
            QMessageBox::warning(this, "Error writing file",
                                 "Can't write a message");
            break;
        }
    }
}


