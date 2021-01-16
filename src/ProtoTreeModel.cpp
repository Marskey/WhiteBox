#include "ProtoTreeModel.h"
#include <list>
#include <QtGui>

static auto TREE_NODE_MIME_TYPE = "application/x-treenode";

ProtoTreeModel::ProtoTreeModel(const ::google::protobuf::Message& data,
                               QObject* parent /*= nullptr*/)
  : QAbstractItemModel(parent) {
  QStringList headers;
  headers << "Name" << "Type" << "Value";

  QVector<QVariant> rootData;
  foreach(QString header, headers) {
    rootData << header;
  }

  rootItem = new ProtoTreeItem(rootData);
  setupModelData(data, rootItem);
}

ProtoTreeModel::~ProtoTreeModel() {
  delete rootItem;
}

int ProtoTreeModel::columnCount(const QModelIndex& /* parent */) const {
  return rootItem->columnCount();
}

QVariant ProtoTreeModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  ProtoTreeItem* item = getItem(index);
  if (item == nullptr) {
    return QVariant();
  }

  return item->data(index.column(), role);
}

Qt::ItemFlags ProtoTreeModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) {
    return Qt::NoItemFlags;
  }

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  ProtoTreeItem* item = getItem(index);
  if (item != nullptr) {
    flags |= item->getFlag();
  }

  if (index.column() != kColumnTypeValue) {
    flags = flags & ~Qt::ItemIsEditable;
  }

  return flags;
}

ProtoTreeItem* ProtoTreeModel::getItem(const QModelIndex& index) const {
  if (index.isValid()) {
    auto* item = static_cast<ProtoTreeItem*>(index.internalPointer());
    if (item) {
      return item;
    }
  }
  return rootItem;
}

std::string ProtoTreeModel::getMsgTypeName(const google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFd) {
  if (pFd->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
    const google::protobuf::Descriptor* desc = pFd->message_type();
    return desc->name();
  }

  return pFd->type_name();
}

QVariant ProtoTreeModel::getMsgValue(const google::protobuf::Message& data, const google::protobuf::FieldDescriptor* fieldDesc) {
  auto type = fieldDesc->cpp_type();
  switch (type) {
  case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
    return QVariant(data.GetReflection()->GetInt32(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
    return QVariant(data.GetReflection()->GetInt64(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
    return QVariant(data.GetReflection()->GetUInt32(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
    return QVariant(data.GetReflection()->GetUInt64(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
    return QVariant(data.GetReflection()->GetDouble(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
    return QVariant(data.GetReflection()->GetFloat(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
    return QVariant(data.GetReflection()->GetBool(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
    return QVariant(data.GetReflection()->GetEnum(data, fieldDesc)->name().c_str());
  case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
    return QVariant(data.GetReflection()->GetString(data, fieldDesc).c_str());
  default:
    return QVariant();
  }
}

QVariant ProtoTreeModel::getMsgValue(const google::protobuf::Message& data, const google::protobuf::FieldDescriptor* fieldDesc, int index) {
  auto type = fieldDesc->cpp_type();
  switch (type) {
  case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
    return QVariant(data.GetReflection()->GetRepeatedInt32(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
    return QVariant(data.GetReflection()->GetRepeatedInt64(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
    return QVariant(data.GetReflection()->GetRepeatedUInt32(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
    return QVariant(data.GetReflection()->GetRepeatedUInt64(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
    return QVariant(data.GetReflection()->GetRepeatedDouble(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
    return QVariant(data.GetReflection()->GetRepeatedFloat(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
    return QVariant(data.GetReflection()->GetRepeatedBool(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
    return QVariant(data.GetReflection()->GetRepeatedEnum(data, fieldDesc, index)->name().c_str());
  case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
    return QVariant(data.GetReflection()->GetRepeatedString(data, fieldDesc, index).c_str());
  default:
    return QVariant();
  }
}

QVariant ProtoTreeModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return rootItem->data(section, role);

  return QVariant();
}

QModelIndex ProtoTreeModel::index(int row, int column, const QModelIndex& parent) const {
  if (parent.isValid() && parent.column() != 0)
    return QModelIndex();

  ProtoTreeItem* parentItem = getItem(parent);

  ProtoTreeItem* childItem = parentItem->child(row);
  if (childItem) {
    return createIndex(row, column, childItem);
  }
  return QModelIndex();
}

bool ProtoTreeModel::insertColumns(int position, int columns, const QModelIndex& parent) {

  beginInsertColumns(parent, position, position + columns - 1);
  bool success = rootItem->insertColumns(position, columns);
  endInsertColumns();

  return success;
}

bool ProtoTreeModel::insertRows(int position, int rows, const QModelIndex& parent) {
  ProtoTreeItem* parentItem = getItem(parent);

  beginInsertRows(parent, position, position + rows - 1);
  bool success = parentItem->insertChildren(position, rows, rootItem->columnCount());
  endInsertRows();

  return success;
}

QModelIndex ProtoTreeModel::parent(const QModelIndex& index) const {
  if (!index.isValid())
    return QModelIndex();

  ProtoTreeItem* childItem = getItem(index);
  ProtoTreeItem* parentItem = childItem->parent();

  if (parentItem == rootItem)
    return QModelIndex();

  return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool ProtoTreeModel::removeColumns(int position, int columns, const QModelIndex& parent) {
  beginRemoveColumns(parent, position, position + columns - 1);
  bool success = rootItem->removeColumns(position, columns);
  endRemoveColumns();

  if (rootItem->columnCount() == 0)
    removeRows(0, rowCount());

  return success;
}

bool ProtoTreeModel::removeRows(int position, int rows, const QModelIndex& parent) {
  ProtoTreeItem* parentItem = getItem(parent);
  beginRemoveRows(parent, position, position + rows - 1);
  bool success = parentItem->removeChildren(position, rows);
  endRemoveRows();

  return success;
}

QStringList ProtoTreeModel::mimeTypes() const
{
    return QAbstractItemModel::mimeTypes() << TREE_NODE_MIME_TYPE;
}

QMimeData* ProtoTreeModel::mimeData(const QModelIndexList& indexes) const {
  QMimeData* mimeData = new QMimeData;
  QByteArray data; //a kind of RAW format for datas
  QDataStream stream(&data, QIODevice::WriteOnly);
  QList<ProtoTreeItem*> nodes;
  foreach(const QModelIndex & index, indexes) {
    ProtoTreeItem* node = getItem(index);

    //  Do i make a tree structure here by recursion function for the tree item
    if (!nodes.contains(node))
      nodes << node;
  }
  stream << QCoreApplication::applicationPid();
  stream << nodes.count();
  foreach(ProtoTreeItem* node, nodes) {
    stream << reinterpret_cast<qlonglong>(node);
  }
  mimeData->setData(TREE_NODE_MIME_TYPE, data);
  return mimeData;
}

bool ProtoTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const {
  if (action == Qt::MoveAction
      && row != -1 && column != -1) {
    return true;
  }
  return false;
}

bool ProtoTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
  if (!data->hasFormat(TREE_NODE_MIME_TYPE)) {
    return false;
  }
  QByteArray byteData = data->data(TREE_NODE_MIME_TYPE);
  QDataStream stream(&byteData, QIODevice::ReadOnly);
  qint64 senderPid;
  stream >> senderPid;
  if (senderPid != QCoreApplication::applicationPid()) {
    // Let's not cast pointers that come from another process...
    return false;
  }
  ProtoTreeItem* parentNode = getItem(parent);
  int count;
  stream >> count;

  int beginRow;

  if (row != -1)
    beginRow = row;
  else if (parent.isValid())
    beginRow = parent.row();
  else
    beginRow = rowCount(QModelIndex());

  if (action == Qt::MoveAction)
    removeRow(row, parent);
  insertRow(beginRow, parent);

  setData(parent.child(beginRow, kColumnTypeName), "Name");
  setData(parent.child(beginRow, kColumnTypeType), "Type");
  setData(parent.child(beginRow, kColumnTypeValue), "Value");

  return true;
}

Qt::DropActions ProtoTreeModel::supportedDropActions() const {
  return Qt::MoveAction;
}

Qt::DropActions ProtoTreeModel::supportedDragActions() const {
  return Qt::MoveAction;
}

int ProtoTreeModel::rowCount(const QModelIndex& parent) const {
  ProtoTreeItem* parentItem = getItem(parent);

  return parentItem->childCount();
}

bool ProtoTreeModel::setData(const QModelIndex& index, const QVariant& value,
                             int role) {
  if (role != Qt::EditRole)
    return false;

  ProtoTreeItem* item = getItem(index);
  bool result = item->setData(index.column(), value, role);

  if (result) {
    emit dataChanged(index, index);
  }

  return result;
}

bool ProtoTreeModel::setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant& value, int role) {
  if (role != Qt::EditRole || orientation != Qt::Horizontal)
    return false;

  bool result = rootItem->setData(section, value, role);

  if (result)
    emit headerDataChanged(orientation, section, section);

  return result;
}

void ProtoTreeModel::setupModelData(const ::google::protobuf::Message& data, ProtoTreeItem* parent) {
  int cnt = data.GetDescriptor()->field_count();
  for (int i = 0; i < cnt; ++i) {
    const google::protobuf::FieldDescriptor* fieldDesc = data.GetDescriptor()->field(i);
    if (fieldDesc == nullptr) {
      continue;
    }

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    ProtoTreeItem* child = parent->child(parent->childCount() - 1);

    QVariant keyIndex, keyData, keyType;

    keyIndex = QVariant(tr(fieldDesc->name().c_str()));
    QString typeName = getMsgTypeName(data, fieldDesc).c_str();
    keyType = QVariant(typeName);

    if (fieldDesc->is_repeated()) {
      keyType = QVariant("repeated " + typeName);
      child->setFlag(Qt::ItemIsDropEnabled);
    } else if (google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE == fieldDesc->cpp_type()) {
#pragma push_macro("GetMessage")
#undef GetMessage
      const auto& subData = data.GetReflection()->GetMessage(data, fieldDesc);
#pragma pop_macro("GetMessage")
      setupModelData(subData, child);
    } else {
      keyData = getMsgValue(data, fieldDesc);
      child->setFlag(Qt::ItemIsEditable);
    }

    child->setData(kColumnTypeName, keyIndex);
    child->setData(kColumnTypeType, keyType);
    child->setData(kColumnTypeValue, keyData);
    child->setData(kColumnTypeValue, fieldDesc->cpp_type(), Qt::UserRole);
    if (fieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
      QStringList enumData;
      const auto* enumDesc = fieldDesc->enum_type();
      if (enumDesc) {
        for (int idx = 0; idx < enumDesc->value_count(); ++idx) {
          const auto* enumValue = enumDesc->value(idx);
          if (enumValue) {
            enumData.append(enumValue->name().c_str());
          }
        }
      }
      child->setData(kColumnTypeValue, enumData, Qt::UserRole + 1);
    }

    if (fieldDesc->is_repeated()) {
      auto fieldSize = data.GetReflection()->FieldSize(data, fieldDesc);
      for (int index = 0; index < fieldSize; ++index) {
        keyIndex = QVariant(index);
        keyType = typeName;

        child->insertChildren(child->childCount(), 1, rootItem->columnCount());
        ProtoTreeItem* repeatedChild = child->child(child->childCount() - 1);
        repeatedChild->setData(kColumnTypeName, keyIndex);
        repeatedChild->setData(kColumnTypeType, keyType);
        repeatedChild->setData(kColumnTypeValue, fieldDesc->cpp_type(), Qt::UserRole);
        repeatedChild->setFlag(Qt::ItemIsDragEnabled);

        if (fieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
          const auto& repMessage = data.GetReflection()->GetRepeatedMessage(data, fieldDesc, index);
          setupModelData(repMessage, repeatedChild);
        } else {
          keyData = getMsgValue(data, fieldDesc, index);
          repeatedChild->setData(kColumnTypeValue, keyData);
        }
      }
    }
  }
}

void ProtoTreeModel::loadModelData(google::protobuf::Message& data, ProtoTreeItem* parent) {
  for (int idx = 0; idx < parent->childCount(); ++idx) {
    ProtoTreeItem* child = parent->child(idx);
    if (nullptr == child) {
      continue;
    }

    const auto* fieldDesc = data.GetDescriptor()->field(idx);
    if (nullptr == fieldDesc) {
      continue;
    }

    if (fieldDesc->is_repeated()) {
      for (int subIdx = 0; subIdx < child->childCount(); ++subIdx) {
        auto subChild = child->child(subIdx);
        if (nullptr == subChild) {
          continue;
        }

        QVariant value = subChild->data(kColumnTypeValue, Qt::DisplayRole);
        auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(subChild->data(kColumnTypeValue, Qt::UserRole).toInt());
        switch (type) {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
          data.GetReflection()->AddInt32(&data, fieldDesc, value.toInt());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
          data.GetReflection()->AddInt64(&data, fieldDesc, value.toLongLong());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
          data.GetReflection()->AddUInt32(&data, fieldDesc, value.toUInt());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
          data.GetReflection()->AddUInt64(&data, fieldDesc, value.toULongLong());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
          data.GetReflection()->AddDouble(&data, fieldDesc, value.toDouble());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
          data.GetReflection()->AddFloat(&data, fieldDesc, value.toFloat());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
          data.GetReflection()->AddBool(&data, fieldDesc, value.toBool());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
          if (fieldDesc->enum_type()) {
            auto enumValueDesc
              = fieldDesc->enum_type()->FindValueByName(value.toString().toStdString());
            if (enumValueDesc) {
              data.GetReflection()->AddEnum(&data, fieldDesc, enumValueDesc);
            }
          }
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
          data.GetReflection()->AddString(&data, fieldDesc, value.toString().toStdString());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
          {
            auto* subData = data.GetReflection()->AddMessage(&data, fieldDesc);
            loadModelData(*subData, child);
          }
          break;
        default:
          break;
        }
      }
    } else {
      QVariant value = child->data(kColumnTypeValue, Qt::DisplayRole);
      auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(child->data(kColumnTypeValue, Qt::UserRole).toInt());
      switch (type) {
      case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        data.GetReflection()->SetInt32(&data, fieldDesc, value.toInt());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        data.GetReflection()->SetInt64(&data, fieldDesc, value.toLongLong());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        data.GetReflection()->SetUInt32(&data, fieldDesc, value.toUInt());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        data.GetReflection()->SetUInt64(&data, fieldDesc, value.toULongLong());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        data.GetReflection()->SetDouble(&data, fieldDesc, value.toDouble());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        data.GetReflection()->SetFloat(&data, fieldDesc, value.toFloat());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        data.GetReflection()->SetBool(&data, fieldDesc, value.toBool());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        if (fieldDesc->enum_type()) {
          auto enumValueDesc
            = fieldDesc->enum_type()->FindValueByName(value.toString().toStdString());
          if (enumValueDesc) {
            data.GetReflection()->SetEnum(&data, fieldDesc, enumValueDesc);
          }
        }
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        data.GetReflection()->SetString(&data, fieldDesc, value.toString().toStdString());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
          auto* subData = data.GetReflection()->MutableMessage(&data, fieldDesc);
          loadModelData(*subData, child);
        }
      default:
        break;;
      }
    }
  }
}

ProtoTreeItem* ProtoTreeModel::getRootItem() {
  return rootItem;
}

void ProtoTreeModel::getMessage(google::protobuf::Message& data) {
  loadModelData(data, rootItem);
}
