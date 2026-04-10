#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSet>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>

#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>
#include <pcl/visualization/pcl_visualizer.h>

#include <vtkGenericOpenGLRenderWindow.h>


class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow() {
    setWindowTitle("实时点云查看工具 (VS2026 + C++)");
    resize(1360, 860);

    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);

    auto *leftPanel = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPanel);

    auto *chooseBtn = new QPushButton("选择点云文件夹", this);
    folderLabel_ = new QLabel("当前文件夹: 未选择", this);
    fileList_ = new QListWidget(this);
    fileList_->setSelectionMode(QAbstractItemView::ExtendedSelection);

    colorR_ = createSlider(255);
    colorG_ = createSlider(255);
    colorB_ = createSlider(255);
    pointSize_ = new QSpinBox(this);
    pointSize_->setRange(1, 12);
    pointSize_->setValue(2);

    leftLayout->addWidget(chooseBtn);
    leftLayout->addWidget(folderLabel_);
    leftLayout->addWidget(new QLabel("点云文件(可多选):", this));
    leftLayout->addWidget(fileList_, 1);
    leftLayout->addWidget(new QLabel("颜色 R:", this));
    leftLayout->addWidget(colorR_);
    leftLayout->addWidget(new QLabel("颜色 G:", this));
    leftLayout->addWidget(colorG_);
    leftLayout->addWidget(new QLabel("颜色 B:", this));
    leftLayout->addWidget(colorB_);
    leftLayout->addWidget(new QLabel("点大小:", this));
    leftLayout->addWidget(pointSize_);

    vtkWidget_ = new QVTKOpenGLNativeWidget(this);
    renderWindow_ = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkWidget_->setRenderWindow(renderWindow_);

    viewer_ = std::make_shared<pcl::visualization::PCLVisualizer>("viewer", false);
    viewer_->setBackgroundColor(0.08, 0.08, 0.10);
    viewer_->addCoordinateSystem(0.15);
    viewer_->initCameraParameters();
    renderWindow_->AddRenderer(viewer_->getRendererCollection()->GetFirstRenderer());

    root->addWidget(leftPanel, 0);
    root->addWidget(vtkWidget_, 1);
    setCentralWidget(central);

    fsWatcher_ = new QFileSystemWatcher(this);
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(1200);

    connect(chooseBtn, &QPushButton::clicked, this, &MainWindow::pickFolder);
    connect(fileList_, &QListWidget::itemSelectionChanged, this, &MainWindow::refreshView);
    connect(colorR_, &QSlider::valueChanged, this, &MainWindow::refreshView);
    connect(colorG_, &QSlider::valueChanged, this, &MainWindow::refreshView);
    connect(colorB_, &QSlider::valueChanged, this, &MainWindow::refreshView);
    connect(pointSize_, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::refreshView);
    connect(fsWatcher_, &QFileSystemWatcher::directoryChanged, this, &MainWindow::reloadFileList);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::reloadFileList);

    refreshTimer_->start();
  }

 private slots:
  void pickFolder() {
    const auto folder = QFileDialog::getExistingDirectory(this, "选择点云目录", currentFolder_);
    if (folder.isEmpty()) {
      return;
    }

    currentFolder_ = folder;
    folderLabel_->setText(QString("当前文件夹: %1").arg(currentFolder_));

    if (!watchedPath_.isEmpty()) {
      fsWatcher_->removePath(watchedPath_);
    }
    watchedPath_ = currentFolder_;
    fsWatcher_->addPath(watchedPath_);

    reloadFileList();
  }

  void reloadFileList() {
    if (currentFolder_.isEmpty()) {
      return;
    }

    const QStringList filters = {"*.pcd", "*.ply"};
    QDir dir(currentFolder_);
    const auto files = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    QSet<QString> existing;
    for (int i = 0; i < fileList_->count(); ++i) {
      existing.insert(fileList_->item(i)->text());
    }

    QStringList now;
    now.reserve(files.size());
    for (const auto &fi : files) {
      now << fi.fileName();
    }

    if (QStringList(existing.values()) == now) {
      return;
    }

    QSet<QString> selected;
    for (const auto *item : fileList_->selectedItems()) {
      selected.insert(item->text());
    }

    fileList_->clear();
    for (const auto &name : now) {
      auto *item = new QListWidgetItem(name, fileList_);
      if (selected.contains(name)) {
        item->setSelected(true);
      }
    }

    refreshView();
  }

  void refreshView() {
    viewer_->removeAllPointClouds();
    viewer_->removeAllShapes();

    const auto selectedItems = fileList_->selectedItems();
    if (selectedItems.empty()) {
      vtkWidget_->renderWindow()->Render();
      return;
    }

    const double r = static_cast<double>(colorR_->value()) / 255.0;
    const double g = static_cast<double>(colorG_->value()) / 255.0;
    const double b = static_cast<double>(colorB_->value()) / 255.0;
    const int size = pointSize_->value();

    int index = 0;
    for (const auto *item : selectedItems) {
      const auto fullPath = QDir(currentFolder_).filePath(item->text());
      auto cloud = pcl::PointCloud<pcl::PointXYZ>::Ptr(new pcl::PointCloud<pcl::PointXYZ>());

      bool ok = false;
      if (fullPath.endsWith(".pcd", Qt::CaseInsensitive)) {
        ok = pcl::io::loadPCDFile<pcl::PointXYZ>(fullPath.toStdString(), *cloud) == 0;
      } else if (fullPath.endsWith(".ply", Qt::CaseInsensitive)) {
        ok = pcl::io::loadPLYFile<pcl::PointXYZ>(fullPath.toStdString(), *cloud) == 0;
      }

      if (!ok || cloud->empty()) {
        continue;
      }

      const std::string cloudId = QString("cloud_%1").arg(index).toStdString();
      pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> color(cloud, colorR_->value(),
                                                                            colorG_->value(), colorB_->value());
      viewer_->addPointCloud<pcl::PointXYZ>(cloud, color, cloudId);
      viewer_->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, size, cloudId);

      // 多选时沿 X 方向平移，避免完全重叠
      viewer_->addText3D(item->text().toStdString(), pcl::PointXYZ(index * 1.5f, 0.0f, 0.0f), 0.07, r, g, b,
                         "label_" + cloudId);
      viewer_->updatePointCloudPose(cloudId,
                                    Eigen::Affine3f(Eigen::Translation3f(index * 1.5f, 0.0f, 0.0f)).matrix());
      ++index;
    }

    viewer_->resetCamera();
    vtkWidget_->renderWindow()->Render();
  }

 private:
  static QSlider *createSlider(int initial) {
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 255);
    slider->setValue(initial);
    return slider;
  }

  QLabel *folderLabel_{nullptr};
  QListWidget *fileList_{nullptr};
  QSlider *colorR_{nullptr};
  QSlider *colorG_{nullptr};
  QSlider *colorB_{nullptr};
  QSpinBox *pointSize_{nullptr};

  QVTKOpenGLNativeWidget *vtkWidget_{nullptr};
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow_;
  std::shared_ptr<pcl::visualization::PCLVisualizer> viewer_;

  QFileSystemWatcher *fsWatcher_{nullptr};
  QTimer *refreshTimer_{nullptr};

  QString currentFolder_;
  QString watchedPath_;
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow window;
  window.show();
  return app.exec();
}

#include "main.moc"
