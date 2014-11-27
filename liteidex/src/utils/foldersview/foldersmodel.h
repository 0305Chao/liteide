#ifndef FOLDERSMODEL_H
#define FOLDERSMODEL_H

#include <QAbstractItemModel>
#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QFileSystemWatcher>

class QIdentityProxyModel;
class QSortFilterProxyModel;
class FoldersModel;
class MyFileSystemModel : public QFileSystemModel
{
public:
    explicit MyFileSystemModel(QObject *parent = 0);
    friend class FoldersModel;
};

struct SourceModel
{
    MyFileSystemModel* model;
    QString      rootPath;
    QString      watchPath;
    QModelIndex  rootIndex;
    QModelIndex  rootSourceIndex;
};

class FoldersModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit FoldersModel(QObject *parent = 0);
    virtual ~FoldersModel();
    void clear();
    QModelIndex addRootPath(const QString &path);
    void removeRootPath(const QString &path);
    void removeRoot(const QModelIndex &index);
    bool isRootPath(const QString &path);
    QList<QModelIndex> rootIndexs() const;
    QStringList rootPathList() const;
    QString filePath(const QModelIndex &index) const;
    QString fileName(const QModelIndex &index) const;
    QFileInfo fileInfo(const QModelIndex &index) const;
    bool isDir(const QModelIndex &index) const;
    QModelIndex mkdir(const QModelIndex &parent,const QString &name);
    bool rmdir(const QModelIndex &index);
    bool remove(const QModelIndex &index);
    void setFilter(QDir::Filters filters);
    QDir::Filters filter() const;
    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;
    void setNameFilterDisables(bool enable);
    bool nameFilterDisables() const;
    void setResolveSymlinks(bool enable);
    bool resolveSymlinks() const;
    bool isRootIndex(const QModelIndex &index) const;
protected:
    MyFileSystemModel *findSource(const QModelIndex &proxyIndex) const;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
    bool isRootSourceIndex(const QModelIndex &sourceIndex) const;
public:
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QMap<int, QVariant> itemData(const QModelIndex &index) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    bool setItemData(const QModelIndex& index, const QMap<int, QVariant> &roles);
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole);

    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    QSize span(const QModelIndex &index) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex buddy(const QModelIndex &index) const;

    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex());
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
    bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
public slots:
    void directoryChanged(const QString &path);

    void sourceRowsAboutToBeInserted(const QModelIndex &,int,int);
    void sourceRowsInserted(const QModelIndex &,int,int);
    void sourceRowsAboutToBeRemoved(const QModelIndex &,int,int);
    void sourceRowsRemoved(const QModelIndex &,int,int);
    void sourceRowsAboutToBeMoved(const QModelIndex &,int,int,const QModelIndex &,int);
    void sourceRowsMoved(const QModelIndex &,int,int,const QModelIndex &,int);

    void sourceColumnsAboutToBeInserted(const QModelIndex &,int,int);
    void sourceColumnsInserted(const QModelIndex &,int,int);
    void sourceColumnsAboutToBeRemoved(const QModelIndex &,int,int);
    void sourceColumnsRemoved(const QModelIndex &,int,int);
    void sourceColumnsAboutToBeMoved(const QModelIndex &,int,int,const QModelIndex &,int);
    void sourceColumnsMoved(const QModelIndex &,int,int,const QModelIndex &,int);

    void sourceDataChanged(const QModelIndex &,const QModelIndex &);
    void sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last);

    void sourceLayoutAboutToBeChanged();
    void sourceLayoutChanged();
    void sourceModelAboutToBeReset();
    void sourceModelReset();

protected:
    QList<SourceModel> m_modelList;
    mutable QHash<qint64,QAbstractItemModel*> m_indexMap;
    bool ignoreNextLayoutAboutToBeChanged;
    bool ignoreNextLayoutChanged;
    QList<QPersistentModelIndex> layoutChangePersistentIndexes;
    QModelIndexList proxyIndexes;
    QFileSystemWatcher *m_watcher;
    QDir::Filters       m_filters;
    QStringList         m_nameFilters;
    bool                m_resolveSymlinks;
    bool                m_nameFilterDisables;
};

#endif // FOLDERSMODEL_H
