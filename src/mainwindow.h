#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <set>

#include <mcap/writer.hpp>
#include <mcap/reader.hpp>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_buttonLoad_clicked();

  void on_buttonResetTimeRange_clicked();

  void on_tableTopics_itemSelectionChanged();

  void on_buttonSave_clicked();

  void on_buttonToggleSelected_clicked();

  private:
  Ui::MainWindow *ui;

  void openFile();
  void openFileWASM();

  void saveFile(mcap::McapWriterOptions options);
  void saveFileWASM(mcap::McapWriterOptions options);

  void readMCAP();
  void writeMCAP(mcap::McapWriter& writer);

  struct SchemaInfo
  {
      std::string name;
      std::string encoding;
      std::string text;
  };

  std::map<mcap::SchemaId, SchemaInfo> schema_by_id_;
  std::map<std::string, mcap::SchemaId> schema_id_by_channel_;
  std::map<std::string, std::string> channel_encoding_;

  uint64_t time_start_;
  uint64_t time_end_;
  mcap::McapReader reader_;

  QString file_opened_;
};

#endif // MAINWINDOW_H
