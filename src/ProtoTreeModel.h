#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include "ProtoTreeItem.h"
#include "google/protobuf/message.h"

class ProtoTreeModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum ColumnType
  {
    kColumnTypeName = 0,
    kColumnTypeType,
    kColumnTypeValue,
    kColumnSize,
  };

  ProtoTreeModel(const ::google::protobuf::Message& data,
                 QObject* parent = nullptr);
  ~ProtoTreeModel();

  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  QModelIndex index(int row, int column,
                    const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;
  bool setHeaderData(int section, Qt::Orientation orientation,
                     const QVariant& value, int role = Qt::DisplayRole) override;

  bool insertColumns(int position, int columns,
                     const QModelIndex& parent = QModelIndex()) override;
  bool removeColumns(int position, int columns,
                     const QModelIndex& parent = QModelIndex()) override;
  bool insertRows(int position, int rows,
                  const QModelIndex& parent = QModelIndex()) override;
  bool removeRows(int position, int rows,
                  const QModelIndex& parent = QModelIndex()) override;
  QStringList mimeTypes() const override;
  QMimeData* mimeData(const QModelIndexList& indexes) const override;
  bool canDropMimeData(const QMimeData* data, Qt::DropAction action,
                       int row, int column, const QModelIndex& parent) const override;
  bool dropMimeData(const QMimeData* data, Qt::DropAction action,
                            int row, int column, const QModelIndex& parent) override;
  Qt::DropActions supportedDropActions() const override;

  Qt::DropActions supportedDragActions() const override;

  ProtoTreeItem* getRootItem();
  void getMessage(google::protobuf::Message& data);

private:
  void setupModelData(const ::google::protobuf::Message& data, ProtoTreeItem* parent);
  void loadModelData(google::protobuf::Message& data, ProtoTreeItem* parent);

  ProtoTreeItem* getItem(const QModelIndex& index) const;
  std::string getMsgTypeName(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd);

  QVariant getMsgValue(const google::protobuf::Message& data, const google::protobuf::FieldDescriptor* fieldDesc);
  QVariant getMsgValue(const google::protobuf::Message& data, const google::protobuf::FieldDescriptor* fieldDesc, int index);

  ProtoTreeItem* rootItem;
};
